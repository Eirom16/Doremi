use futures::StreamExt;
use serde::{Deserialize, Serialize};
use std::path::{Path, PathBuf};
use std::process::Command;
use tokio::fs::File;
use tokio::io::{AsyncReadExt, AsyncWriteExt};
use sha2::Digest;
use zeroize::Zeroize;

const GITHUB_API_URL: &str = "https://api.github.com/repos/Eirom16/Doremi/releases/latest";
const USER_AGENT: &str = "Doremi-Updater/2.0.0 (Linux)";

/// Whitelist regex for asset names coming from GitHub Releases JSON.
/// Reject any name with shell metacharacters, path traversal, or spaces.
static ASSET_NAME_RE: once_cell::sync::Lazy<regex::Regex> =
    once_cell::sync::Lazy::new(|| regex::Regex::new(r"^[A-Za-z0-9._-]+$").unwrap());

fn is_safe_asset_name(name: &str) -> bool {
    // Must match the whitelist AND must not contain path-traversal sequences.
    ASSET_NAME_RE.is_match(name) && !name.contains("..")
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct GithubAsset {
    pub name: String,
    pub browser_download_url: String,
    pub size: u64,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct GithubRelease {
    pub tag_name: String,
    pub body: Option<String>,
    pub html_url: String,
    pub assets: Vec<GithubAsset>,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct ReleaseInfo {
    pub version: String,
    pub notes: String,
    pub url: String,
    pub asset_url: String,
    pub asset_name: String,
    pub asset_size: u64,
}

pub fn detect_package_manager() -> String {
    get_detected_package_manager()
}

// Simple dependency-free fallback for which() if which crate is not present
// Doremi has no which in Cargo.toml. Let's write our own check:
fn is_command_available(cmd: &str) -> bool {
    if let Ok(paths) = std::env::var("PATH") {
        for path in std::env::split_paths(&paths) {
            let p = path.join(cmd);
            if p.exists() && p.is_file() {
                // Check execute permission on unix
                #[cfg(unix)]
                {
                    use std::os::unix::fs::MetadataExt;
                    if let Ok(meta) = p.metadata() {
                        if meta.mode() & 0o111 != 0 {
                            return true;
                        }
                    }
                }
                #[cfg(not(unix))]
                return true;
            }
        }
    }
    false
}

pub fn get_detected_package_manager() -> String {
    let managers = ["pacman", "apt", "dnf", "zypper", "emerge"];
    for mgr in &managers {
        if is_command_available(mgr) {
            return mgr.to_string();
        }
    }
    "unknown".to_string()
}

fn parse_version(tag: &str) -> Vec<u32> {
    tag.trim_start_matches('v')
        .split('.')
        .filter_map(|s| s.parse::<u32>().ok())
        .collect()
}

pub async fn check_for_updates() -> Option<ReleaseInfo> {
    log::info!("Checking for updates from: {GITHUB_API_URL}");
    let client = reqwest::Client::new();
    let res = client
        .get(GITHUB_API_URL)
        .header("User-Agent", USER_AGENT)
        .header("Accept", "application/vnd.github+json")
        .send()
        .await;

    match res {
        Ok(resp) => {
            if let Ok(release) = resp.json::<GithubRelease>().await {
                let latest_ver = parse_version(&release.tag_name);
                let current_ver = parse_version(crate::VERSION);

                if latest_ver.is_empty() || current_ver.is_empty() {
                    return None;
                }

                // Compare version numbers
                if latest_ver > current_ver {
                    // Match asset for Linux platform package manager
                    let pkg_mgr = get_detected_package_manager();
                    let suffix = match pkg_mgr.as_str() {
                        "pacman" => ".pkg.tar.zst",
                        "apt" => "_amd64.deb",
                        "dnf" | "zypper" | "emerge" => ".x86_64.rpm",
                        _ => ".pkg.tar.zst",
                    };

                    if let Some(asset) = release.assets.iter().find(|a| {
                        a.name.ends_with(suffix) && is_safe_asset_name(&a.name)
                    }) {
                        return Some(ReleaseInfo {
                            version: release.tag_name.clone(),
                            notes: release
                                .body
                                .unwrap_or_else(|| "Sin notas de versión.".to_string()),
                            url: release.html_url.clone(),
                            asset_url: asset.browser_download_url.clone(),
                            asset_name: asset.name.clone(),
                            asset_size: asset.size,
                        });
                    } else {
                        log::warn!(
                            "No asset with a safe name found for package manager '{pkg_mgr}'"
                        );
                    }
                }
            }
        }
        Err(e) => {
            log::warn!("Failed to query GitHub Releases API: {e}");
        }
    }
    None
}

// Progress callback interface for download
pub async fn download_update_package<F>(url: &str, name: &str, mut progress: F) -> Option<PathBuf>
where
    F: FnMut(f64, &str),
{
    // BF0.1: validate name before using it as a filesystem component
    if !is_safe_asset_name(name) {
        log::error!("Refusing to download package with unsafe asset name: {name:?}");
        return None;
    }

    let dest = std::env::temp_dir().join(name);
    log::info!(
        "Downloading update package from {} to {dest:?}",
        crate::utils::security::redact_url(url)
    );

    let client = reqwest::Client::new();
    let res = client
        .get(url)
        .header("User-Agent", USER_AGENT)
        .send()
        .await;

    match res {
        Ok(resp) => {
            if !resp.status().is_success() {
                log::error!("Failed to download package: HTTP status {}", resp.status());
                return None;
            }

            let total_size = resp.content_length().unwrap_or(0);
            let mut file = match File::create(&dest).await {
                Ok(f) => f,
                Err(e) => {
                    log::error!("Failed to create temp file: {e}");
                    return None;
                }
            };

            let mut stream = resp.bytes_stream();
            let mut downloaded: u64 = 0;

            while let Some(chunk_result) = stream.next().await {
                match chunk_result {
                    Ok(chunk) => {
                        if let Err(e) = file.write_all(&chunk).await {
                            log::error!("Failed to write chunk: {e}");
                            return None;
                        }
                        downloaded += chunk.len() as u64;
                        if total_size > 0 {
                            let pct = (downloaded as f64 / total_size as f64) * 100.0;
                            let mb_done = downloaded as f64 / 1_048_576.0;
                            let mb_total = total_size as f64 / 1_048_576.0;
                            progress(pct, &format!("Descargando {mb_done:.1} / {mb_total:.1} MB"));
                        }
                    }
                    Err(e) => {
                        log::error!("Stream error during download: {e}");
                        return None;
                    }
                }
            }

            // Drop file to flush and close it before calculating hash
            drop(file);

            progress(99.0, "Verificando checksum...");
            log::info!("Verifying update package checksum...");

            // 1. Fetch checksum
            let checksum_text = match fetch_checksum(url).await {
                Some(text) => text,
                None => {
                    log::error!("Failed to download checksum file for update");
                    let _ = tokio::fs::remove_file(&dest).await;
                    return None;
                }
            };

            // 2. Parse hash
            let expected_hash = match parse_sha256(&checksum_text, name) {
                Some(hash) => hash,
                None => {
                    log::error!("Failed to parse SHA-256 hash from checksum file");
                    let _ = tokio::fs::remove_file(&dest).await;
                    return None;
                }
            };

            // 3. Compute file hash
            let actual_hash = match calculate_file_sha256(&dest).await {
                Ok(hash) => hash,
                Err(e) => {
                    log::error!("Failed to calculate SHA-256 of downloaded file: {e}");
                    let _ = tokio::fs::remove_file(&dest).await;
                    return None;
                }
            };

            // 4. Compare
            if actual_hash != expected_hash {
                log::error!("Checksum mismatch! Expected: {}, got: {}", expected_hash, actual_hash);
                let _ = tokio::fs::remove_file(&dest).await;
                return None;
            }

            log::info!("Checksum verification succeeded: {}", actual_hash);
            progress(100.0, "Verificación exitosa");
            Some(dest)
        }
        Err(e) => {
            log::error!("Failed to download update package: {e}");
            None
        }
    }
}

pub async fn install_update_async(package_path: &Path, password: Option<String>) -> bool {
    let path_str = package_path.to_string_lossy().to_string();
    let pkg_mgr = get_detected_package_manager();

    if let Some(mut pwd) = password {
        // BF0.1: construct args without interpolating into shell strings.
        // Each argument is passed separately to sudo; no bash -c involved.
        // BF0.1: fix Arch branch — was generating "sudo -S pacman pacman -U ..."
        //        (pacman duplicated). Now: sudo -S <pkg_mgr> <args...>
        let (manager, install_args): (&str, Vec<&str>) = match pkg_mgr.as_str() {
            "pacman" => ("pacman", vec!["-U", "--noconfirm", &path_str]),
            "apt" => ("apt", vec!["install", "-y", &path_str]),
            "dnf" => ("dnf", vec!["install", "-y", &path_str]),
            "zypper" => ("zypper", vec!["install", "-y", &path_str]),
            _ => {
                pwd.zeroize();
                return false;
            }
        };

        log::info!("Installing package via sudo using {pkg_mgr}");

        // Build: sudo -S <manager> <install_args...>
        let mut sudo_args = vec!["-S", manager];
        sudo_args.extend_from_slice(&install_args);

        let mut child = match tokio::process::Command::new("sudo")
            .args(&sudo_args)
            .stdin(std::process::Stdio::piped())
            .stdout(std::process::Stdio::piped())
            .stderr(std::process::Stdio::piped())
            .spawn()
        {
            Ok(c) => c,
            Err(e) => {
                log::error!("Failed to spawn sudo install command: {e}");
                pwd.zeroize();
                return false;
            }
        };

        // BF0.1: propagate stdin write error; zeroize password immediately after use.
        let write_result = if let Some(mut stdin) = child.stdin.take() {
            let bytes = format!("{pwd}\n");
            pwd.zeroize();
            stdin.write_all(bytes.as_bytes()).await
        } else {
            pwd.zeroize();
            Err(std::io::Error::other("sudo stdin unavailable"))
        };

        if let Err(e) = write_result {
            log::error!("Failed to send password to sudo: {e}");
            let _ = child.kill().await;
            return false;
        }

        match child.wait().await {
            Ok(status) => {
                log::info!("Install process finished with status: {status}");
                status.success()
            }
            Err(e) => {
                log::error!("Error waiting for install process: {e}");
                false
            }
        }
    } else {
        // Prefer pkexec (PolicyKit) — no password transport needed.
        let (manager, install_args): (&str, Vec<&str>) = match pkg_mgr.as_str() {
            "pacman" => ("pacman", vec!["-U", "--noconfirm", &path_str]),
            "apt" => ("apt", vec!["install", "-y", &path_str]),
            "dnf" => ("dnf", vec!["install", "-y", &path_str]),
            "zypper" => ("zypper", vec!["install", "-y", &path_str]),
            _ => return false,
        };

        // Build: pkexec <manager> <install_args...>
        let mut pkexec_args = vec![manager];
        pkexec_args.extend_from_slice(&install_args);

        log::info!("Installing package via pkexec using {pkg_mgr}");

        match Command::new("pkexec").args(&pkexec_args).spawn() {
            Ok(_) => true,
            Err(e) => {
                log::warn!("Failed to launch pkexec: {e}; trying terminal fallback");
                // BF0.1: terminal fallback uses separate args — no bash -c with interpolated path.
                // The path has already been validated as safe (alphanumeric/._-) by is_safe_asset_name
                // before download, but we pass it as a discrete argument rather than in a shell string.
                for term in &["konsole", "gnome-terminal", "xterm", "alacritty"] {
                    if is_command_available(term) {
                        // Pass manager and args as discrete tokens; the terminal emulator
                        // launches them directly rather than through a shell string.
                        let mut term_args: Vec<&str> = vec!["-e", manager];
                        term_args.extend_from_slice(&install_args);
                        let _ = Command::new(term).args(&term_args).spawn();
                        return true;
                    }
                }
                false
            }
        }
    }
}

pub fn validate_sudo_password(password: &str) -> bool {
    let mut child = match Command::new("sudo")
        .args(["-S", "-k", "true"])
        .stdin(std::process::Stdio::piped())
        .stdout(std::process::Stdio::piped())
        .stderr(std::process::Stdio::piped())
        .spawn()
    {
        Ok(c) => c,
        Err(_) => return false,
    };

    // BF0.1: propagate stdin write error instead of silently ignoring it.
    if let Some(mut stdin) = child.stdin.take() {
        use std::io::Write;
        if stdin.write_all(format!("{password}\n").as_bytes()).is_err() {
            log::warn!("Failed to write password to sudo stdin during validation");
            let _ = child.wait();
            return false;
        }
    }

    match child.wait() {
        Ok(status) => status.success(),
        Err(_) => false,
    }
}

async fn fetch_checksum(asset_url: &str) -> Option<String> {
    let client = reqwest::Client::new();
    let urls = vec![
        format!("{}.sha256", asset_url),
        format!("{}.sha256sum", asset_url),
    ];
    for url in urls {
        log::info!("Trying to fetch checksum from: {}", crate::utils::security::redact_url(&url));
        if let Ok(resp) = client.get(&url).header("User-Agent", USER_AGENT).send().await {
            if resp.status().is_success() {
                if let Ok(text) = resp.text().await {
                    return Some(text);
                }
            }
        }
    }
    if let Some(last_slash) = asset_url.rfind('/') {
        let base_url = &asset_url[..last_slash];
        let fallback_urls = vec![
            format!("{}/SHA256SUMS", base_url),
            format!("{}/checksums.txt", base_url),
            format!("{}/SHA256SUM", base_url),
        ];
        for url in fallback_urls {
            log::info!("Trying fallback checksum URL: {}", crate::utils::security::redact_url(&url));
            if let Ok(resp) = client.get(&url).header("User-Agent", USER_AGENT).send().await {
                if resp.status().is_success() {
                    if let Ok(text) = resp.text().await {
                        return Some(text);
                    }
                }
            }
        }
    }
    None
}

fn find_64_char_hex(text: &str) -> Option<String> {
    let chars: Vec<char> = text.chars().collect();
    let mut start = None;
    let mut count = 0;
    for (i, &c) in chars.iter().enumerate() {
        if c.is_ascii_hexdigit() {
            if start.is_none() {
                start = Some(i);
            }
            count += 1;
        } else {
            if count == 64 {
                if let Some(s) = start {
                    return Some(chars[s..s+64].iter().collect::<String>().to_lowercase());
                }
            }
            start = None;
            count = 0;
        }
    }
    if count == 64 {
        if let Some(s) = start {
            return Some(chars[s..s+64].iter().collect::<String>().to_lowercase());
        }
    }
    None
}

fn parse_sha256(checksum_text: &str, asset_name: &str) -> Option<String> {
    for line in checksum_text.lines() {
        let line = line.trim();
        if line.is_empty() || line.starts_with('#') {
            continue;
        }
        if line.contains(asset_name) {
            if let Some(hash) = find_64_char_hex(line) {
                return Some(hash);
            }
        }
    }
    find_64_char_hex(checksum_text)
}

async fn calculate_file_sha256(path: &Path) -> Result<String, std::io::Error> {
    let mut file = File::open(path).await?;
    let mut hasher = sha2::Sha256::new();
    let mut buffer = [0u8; 8192];
    loop {
        let n = file.read(&mut buffer).await?;
        if n == 0 {
            break;
        }
        hasher.update(&buffer[..n]);
    }
    let result = hasher.finalize();
    Ok(format!("{:x}", result))
}
