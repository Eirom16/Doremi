use futures::StreamExt;
use serde::{Deserialize, Serialize};
use std::path::{Path, PathBuf};
use std::process::Command;
use tokio::fs::File;
use tokio::io::AsyncWriteExt;

const GITHUB_API_URL: &str = "https://api.github.com/repos/Eirom16/Doremi/releases/latest";
const USER_AGENT: &str = "Doremi-Updater/2.0.0 (Linux)";

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

                    if let Some(asset) = release.assets.iter().find(|a| a.name.ends_with(suffix)) {
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

    if let Some(pwd) = password {
        // Run with sudo -S
        let install_args = match pkg_mgr.as_str() {
            "pacman" => vec!["-S", "pacman", "-U", "--noconfirm", &path_str],
            "apt" => vec!["-S", "apt", "install", "-y", &path_str],
            "dnf" => vec!["-S", "dnf", "install", "-y", &path_str],
            "zypper" => vec!["-S", "zypper", "install", "-y", &path_str],
            _ => return false,
        };

        log::info!("Installing package via sudo -S using {pkg_mgr}");

        let mut child = match tokio::process::Command::new("sudo")
            .args(&install_args)
            .stdin(std::process::Stdio::piped())
            .stdout(std::process::Stdio::piped())
            .stderr(std::process::Stdio::piped())
            .spawn()
        {
            Ok(c) => c,
            Err(e) => {
                log::error!("Failed to spawn sudo install command: {e}");
                return false;
            }
        };

        if let Some(mut stdin) = child.stdin.take() {
            let _ = stdin.write_all(format!("{pwd}\n").as_bytes()).await;
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
        // Fallback to pkexec
        let cmd = match pkg_mgr.as_str() {
            "pacman" => vec!["pkexec", "pacman", "-U", "--noconfirm", &path_str],
            "apt" => vec!["pkexec", "apt", "install", "-y", &path_str],
            "dnf" => vec!["pkexec", "dnf", "install", "-y", &path_str],
            "zypper" => vec!["pkexec", "zypper", "install", "-y", &path_str],
            _ => return false,
        };

        log::info!("Installing package via pkexec using {pkg_mgr}");

        match Command::new(cmd[0]).args(&cmd[1..]).spawn() {
            Ok(_) => true,
            Err(e) => {
                log::error!("Failed to launch pkexec: {e}");
                // Try terminal fallback
                let term_cmd = match pkg_mgr.as_str() {
                    "pacman" => format!("sudo pacman -U --noconfirm {path_str}"),
                    "apt" => format!("sudo apt install -y {path_str}"),
                    "dnf" => format!("sudo dnf install -y {path_str}"),
                    "zypper" => format!("sudo zypper install -y {path_str}"),
                    _ => return false,
                };

                for term in &["konsole", "gnome-terminal", "xterm", "alacritty"] {
                    if is_command_available(term) {
                        let _ = Command::new(term)
                            .args([
                                "-e",
                                "bash",
                                "-c",
                                &format!("{}; read -p 'Presiona Enter para cerrar'", term_cmd),
                            ])
                            .spawn();
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

    if let Some(mut stdin) = child.stdin.take() {
        use std::io::Write;
        let _ = stdin.write_all(format!("{password}\n").as_bytes());
    }

    match child.wait() {
        Ok(status) => status.success(),
        Err(_) => false,
    }
}
