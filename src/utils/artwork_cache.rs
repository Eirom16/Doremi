use once_cell::sync::Lazy;
use std::collections::HashSet;
use std::path::{Path, PathBuf};
use std::time::Duration;
use tokio::sync::Mutex;

const DEFAULT_MAX_BYTES: u64 = 512 * 1024 * 1024;

static IN_FLIGHT: Lazy<Mutex<HashSet<String>>> = Lazy::new(|| Mutex::new(HashSet::new()));
static CLIENT: Lazy<reqwest::Client> = Lazy::new(|| {
    reqwest::Client::builder()
        .timeout(Duration::from_secs(8))
        .build()
        .unwrap_or_default()
});

#[derive(Debug, Clone)]
pub struct ArtworkCache {
    root: PathBuf,
    max_bytes: u64,
}

impl ArtworkCache {
    pub fn global() -> Self {
        Self::new(
            crate::config::paths::AppDirs::global().artwork_cache_dir(),
            DEFAULT_MAX_BYTES,
        )
    }

    pub fn new(root: PathBuf, max_bytes: u64) -> Self {
        Self { root, max_bytes }
    }

    pub async fn get_or_fetch(&self, url: &str, key_hint: &str) -> Option<PathBuf> {
        if !url.starts_with("http://") && !url.starts_with("https://") {
            return None;
        }
        tokio::fs::create_dir_all(&self.root).await.ok()?;

        let key = cache_key(url, key_hint);
        let path = self.root.join(format!("{key}.img"));
        if path.exists() {
            return Some(path);
        }

        let acquired = acquire_in_flight(&key).await;
        if !acquired {
            for _ in 0..80 {
                if path.exists() {
                    return Some(path);
                }
                tokio::time::sleep(Duration::from_millis(50)).await;
            }
            return None;
        }

        let result = self.download_to_path(url, &path).await;
        release_in_flight(&key).await;
        result
    }

    async fn download_to_path(&self, url: &str, path: &Path) -> Option<PathBuf> {
        let response = CLIENT.get(url).send().await.ok()?;
        if !response.status().is_success() {
            return None;
        }
        let bytes = response.bytes().await.ok()?;
        if bytes.is_empty() {
            return None;
        }

        let tmp_path = path.with_extension("tmp");
        tokio::fs::write(&tmp_path, bytes).await.ok()?;
        tokio::fs::rename(&tmp_path, path).await.ok()?;
        let _ = enforce_limit(&self.root, self.max_bytes);
        Some(path.to_path_buf())
    }
}

async fn acquire_in_flight(key: &str) -> bool {
    let mut in_flight = IN_FLIGHT.lock().await;
    in_flight.insert(key.to_string())
}

async fn release_in_flight(key: &str) {
    let mut in_flight = IN_FLIGHT.lock().await;
    in_flight.remove(key);
}

fn cache_key(url: &str, key_hint: &str) -> String {
    let safe_hint: String = key_hint
        .chars()
        .filter(|character| character.is_ascii_alphanumeric() || matches!(character, '-' | '_'))
        .take(80)
        .collect();
    if !safe_hint.is_empty() {
        return safe_hint;
    }
    format!("{:x}", md5::compute(url.as_bytes()))
}

fn enforce_limit(root: &Path, max_bytes: u64) -> std::io::Result<()> {
    if max_bytes == 0 || !root.exists() {
        return Ok(());
    }

    let mut files = Vec::new();
    let mut total = 0_u64;
    for entry in std::fs::read_dir(root)? {
        let entry = entry?;
        let path = entry.path();
        if !path.is_file() {
            continue;
        }
        let meta = entry.metadata()?;
        total = total.saturating_add(meta.len());
        files.push((path, meta.modified().ok(), meta.len()));
    }

    if total <= max_bytes {
        return Ok(());
    }

    files.sort_by_key(|(_, modified, _)| *modified);
    for (path, _, size) in files {
        if total <= max_bytes {
            break;
        }
        if std::fs::remove_file(&path).is_ok() {
            total = total.saturating_sub(size);
        }
    }
    Ok(())
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn cache_key_sanitizes_hint_and_hashes_fallback() {
        assert_eq!(cache_key("https://example.invalid/a.jpg", "A/B C"), "ABC");
        assert_eq!(
            cache_key("https://example.invalid/a.jpg", ""),
            format!("{:x}", md5::compute("https://example.invalid/a.jpg".as_bytes()))
        );
    }

    #[test]
    fn enforce_limit_removes_old_files() {
        let root = std::env::temp_dir().join(format!(
            "doremi-artwork-cache-test-{}",
            std::process::id()
        ));
        let _ = std::fs::remove_dir_all(&root);
        std::fs::create_dir_all(&root).unwrap();
        std::fs::write(root.join("a.img"), vec![1_u8; 8]).unwrap();
        std::thread::sleep(Duration::from_millis(5));
        std::fs::write(root.join("b.img"), vec![2_u8; 8]).unwrap();

        enforce_limit(&root, 8).unwrap();

        assert!(!root.join("a.img").exists());
        assert!(root.join("b.img").exists());
        let _ = std::fs::remove_dir_all(&root);
    }
}
