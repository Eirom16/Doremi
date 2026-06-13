use crate::utils::errors::DoremiError;
use crate::db::cache::ResponseCache;
use std::time::{SystemTime, UNIX_EPOCH};
use reqwest::Url;
use tokio::process::Command;

pub struct StreamResolver;

impl StreamResolver {
    pub async fn resolve(video_id: &str) -> Result<String, DoremiError> {
        let cache_key = format!("stream_url:{video_id}");
        
        // 1. Check cache first
        if let Some(entry) = ResponseCache::get::<String>(&cache_key) {
            let url = entry.data;
            if !Self::is_url_expired(&url) {
                log::info!("Stream URL loaded from cache (resolver) for {video_id}");
                return Ok(url);
            } else {
                log::info!("Cached stream URL for {video_id} has expired or is close to expiring. Re-resolving...");
                let _ = ResponseCache::invalidate(&cache_key);
            }
        }

        // 2. Resolve using yt-dlp
        let url = format!("https://music.youtube.com/watch?v={video_id}");
        let mut cmd = Command::new("yt-dlp");
        cmd.args([
            "-f", "bestaudio/best",
            "--get-url",
            "--no-playlist",
            "--no-warnings",
        ]);

        // Load YouTube Music authenticated headers from Secret Service
        match crate::utils::secure_storage::load_youtube_headers() {
            Ok(Some(headers_json)) => {
                if let Ok(val) = serde_json::from_str::<serde_json::Value>(&headers_json) {
                    if let Some(headers_obj) = val.as_object() {
                        for (key, val) in headers_obj {
                            if let Some(val_str) = val.as_str() {
                                cmd.arg("--add-header");
                                cmd.arg(format!("{}:{}", key, val_str));
                            }
                        }
                    }
                }
            }
            Ok(None) => {}
            Err(e) => {
                log::debug!("Could not load YouTube authenticated headers: {e}");
            }
        }

        cmd.arg(&url);

        log::info!("Resolving stream URL for {} using yt-dlp...", video_id);
        let output = cmd.output().await.map_err(|e| {
            DoremiError::ExternalDependency(format!("No se pudo ejecutar 'yt-dlp': {e}"))
        })?;

        if !output.status.success() {
            let stderr = String::from_utf8_lossy(&output.stderr).to_string();
            return Err(DoremiError::Network(format!(
                "Error al resolver el stream con 'yt-dlp': {}",
                stderr.trim()
            )));
        }

        let resolved_url = String::from_utf8_lossy(&output.stdout).trim().to_string();
        if resolved_url.is_empty() {
            return Err(DoremiError::Network(
                "La respuesta de 'yt-dlp' está vacía al resolver el stream".to_string()
            ));
        }

        // 3. Cache the resolved URL
        // Expiration parsing
        let ttl_secs = if let Some(expire_timestamp) = Self::parse_expiration(&resolved_url) {
            let current_secs = SystemTime::now()
                .duration_since(UNIX_EPOCH)
                .map(|d| d.as_secs())
                .unwrap_or(0);
            if expire_timestamp > current_secs {
                // Safety margin of 30 minutes
                let remaining = expire_timestamp - current_secs;
                if remaining > 1800 {
                    Some(remaining - 1800)
                } else {
                    Some(remaining)
                }
            } else {
                Some(14400) // Default fallback 4 hours
            }
        } else {
            Some(14400) // Default fallback 4 hours
        };

        if let Err(e) = ResponseCache::set(&cache_key, &resolved_url, ttl_secs) {
            log::warn!("Failed to cache stream URL for {}: {}", video_id, e);
        }

        Ok(resolved_url)
    }

    pub fn parse_expiration(url_str: &str) -> Option<u64> {
        let url = Url::parse(url_str).ok()?;
        for (key, val) in url.query_pairs() {
            if key == "expire" {
                return val.parse::<u64>().ok();
            }
        }
        None
    }

    pub fn is_url_expired(url_str: &str) -> bool {
        if let Some(expire_timestamp) = Self::parse_expiration(url_str) {
            let current_secs = SystemTime::now()
                .duration_since(UNIX_EPOCH)
                .map(|d| d.as_secs())
                .unwrap_or(0);
            // Expire if current time + 30 minutes safety margin is past expire timestamp
            current_secs + 1800 >= expire_timestamp
        } else {
            true
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_parse_expiration() {
        let url = "https://rr2---sn-ab5sznzs.googlevideo.com/videoplayback?expire=1718330000&ei=abc";
        assert_eq!(StreamResolver::parse_expiration(url), Some(1718330000));

        let url_no_expire = "https://example.com/audio.mp3";
        assert_eq!(StreamResolver::parse_expiration(url_no_expire), None);
    }

    #[test]
    fn test_is_url_expired() {
        // Future expiration (expire = current time + 2 hours)
        let current_secs = SystemTime::now()
            .duration_since(UNIX_EPOCH)
            .unwrap()
            .as_secs();
        let future_expire = current_secs + 7200;
        let url = format!("https://googlevideo.com/videoplayback?expire={}", future_expire);
        assert!(!StreamResolver::is_url_expired(&url));

        // Past expiration
        let past_expire = current_secs - 100;
        let expired_url = format!("https://googlevideo.com/videoplayback?expire={}", past_expire);
        assert!(StreamResolver::is_url_expired(&expired_url));
    }
}
