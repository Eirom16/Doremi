use crate::db::cache::ResponseCache;
use crate::utils::errors::DoremiError;
use once_cell::sync::Lazy;
use reqwest::header::{
    HeaderMap, HeaderValue, ACCEPT, ACCEPT_ENCODING, ORIGIN, RANGE, REFERER, USER_AGENT,
};
use reqwest::Url;
use serde_json::json;
use std::collections::HashMap;
use std::process::Stdio;
use std::sync::Arc;
use std::time::Duration;
use std::time::{SystemTime, UNIX_EPOCH};
use tokio::process::Command;
use tokio::sync::{Mutex as AsyncMutex, OnceCell as TokioOnceCell};

pub struct StreamResolver;

const INNERTUBE_TIMEOUT: Duration = Duration::from_secs(6);
const STREAM_PROBE_TIMEOUT: Duration = Duration::from_secs(6);
const YT_DLP_TIMEOUT: Duration = Duration::from_secs(45);
const YOUTUBE_ANDROID_USER_AGENT: &str =
    "com.google.android.youtube/20.10.38 (Linux; U; Android 15)";

type ResolveCell = TokioOnceCell<Result<String, DoremiError>>;

static IN_FLIGHT_RESOLVES: Lazy<AsyncMutex<HashMap<String, Arc<ResolveCell>>>> =
    Lazy::new(|| AsyncMutex::new(HashMap::new()));

impl StreamResolver {
    pub async fn resolve(video_id: &str) -> Result<String, DoremiError> {
        let video_id = video_id.to_string();
        let cell = {
            let mut in_flight = IN_FLIGHT_RESOLVES.lock().await;
            if let Some(cell) = in_flight.get(&video_id) {
                cell.clone()
            } else {
                let cell = Arc::new(TokioOnceCell::new());
                in_flight.insert(video_id.clone(), cell.clone());
                cell
            }
        };

        let result = cell
            .get_or_init(|| async { Self::resolve_inner(&video_id).await })
            .await
            .clone();

        let mut in_flight = IN_FLIGHT_RESOLVES.lock().await;
        if in_flight
            .get(&video_id)
            .map(|stored| Arc::ptr_eq(stored, &cell))
            .unwrap_or(false)
        {
            in_flight.remove(&video_id);
        }

        result
    }

    async fn resolve_inner(video_id: &str) -> Result<String, DoremiError> {
        let cache_key = format!("stream_url:ytdlp:{video_id}");
        let legacy_cache_key = format!("stream_url:{video_id}");
        let _ = ResponseCache::invalidate(&legacy_cache_key);

        // 1. Check the yt-dlp cache first. Older cache keys may contain fast
        // InnerTube URLs that start playing and then fail on later byte ranges.
        if let Some(entry) = ResponseCache::get::<String>(&cache_key) {
            let url = entry.data;
            if !Self::is_url_expired(&url) {
                match Self::probe_stream_url(&url).await {
                    Ok(()) => {
                        log::info!("yt-dlp stream URL loaded from cache for {video_id}");
                        return Ok(url);
                    }
                    Err(error) => {
                        log::warn!(
                            "Cached yt-dlp stream URL for {video_id} is not playable ({error}). Re-resolving..."
                        );
                        let _ = ResponseCache::invalidate(&cache_key);
                    }
                }
            } else {
                log::info!("Cached stream URL for {video_id} has expired or is close to expiring. Re-resolving...");
                let _ = ResponseCache::invalidate(&cache_key);
            }
        }

        // 2. Resolve using yt-dlp first. Its URLs accept the bounded range
        // sequence that the local proxy feeds to VLC.
        match Self::resolve_ytdlp(video_id).await {
            Ok(resolved_url) => {
                Self::probe_stream_url(&resolved_url).await?;
                Self::cache_resolved_url(video_id, &cache_key, &resolved_url);
                return Ok(resolved_url);
            }
            Err(error) => {
                log::warn!(
                    "yt-dlp stream resolution failed for {video_id}: {error}; falling back to InnerTube"
                );
            }
        }

        // 3. Last resort: fast Android InnerTube. Do not cache this path because
        // some returned URLs reject later byte ranges even when the first probe
        // succeeds.
        let resolved_url = Self::resolve_innertube(video_id).await?;
        Self::probe_stream_url(&resolved_url).await?;
        Ok(resolved_url)
    }

    async fn resolve_ytdlp(video_id: &str) -> Result<String, DoremiError> {
        match Self::resolve_ytdlp_once(video_id, false).await {
            Ok(url) => return Ok(url),
            Err(anonymous_error) => {
                if Self::is_ytdlp_timeout(&anonymous_error) {
                    return Err(anonymous_error);
                }
                log::warn!(
                    "Anonymous yt-dlp stream resolution failed for {video_id}: {anonymous_error}; retrying with stored YouTube session"
                );
                match Self::resolve_ytdlp_once(video_id, true).await {
                    Ok(url) => Ok(url),
                    Err(auth_error) => Err(DoremiError::Network(format!(
                        "yt-dlp anonymous failed: {anonymous_error}; authenticated retry failed: {auth_error}"
                    ))),
                }
            }
        }
    }

    fn is_ytdlp_timeout(error: &DoremiError) -> bool {
        matches!(error, DoremiError::Network(message) if message.contains("tardó más"))
    }

    async fn resolve_ytdlp_once(video_id: &str, use_auth: bool) -> Result<String, DoremiError> {
        let url = format!("https://music.youtube.com/watch?v={video_id}");
        let format_selector = Self::audio_format_selector();
        let mut cmd = Command::new("yt-dlp");
        cmd.args([
            "-f",
            format_selector.as_str(),
            "--get-url",
            "--no-playlist",
            "--no-warnings",
            "--skip-download",
            "--socket-timeout",
            "8",
            "--extractor-retries",
            "1",
        ]);
        cmd.stdout(Stdio::piped());
        cmd.stderr(Stdio::piped());

        // Load YouTube Music authenticated headers from Secret Service
        let _auth = if use_auth {
            if let Some(auth) = crate::utils::ytdlp_auth::prepare_ytdlp_auth() {
                cmd.arg("--cookies");
                cmd.arg(&auth.cookie_path);
                for (key, val) in &auth.extra_headers {
                    cmd.arg("--add-header");
                    cmd.arg(format!("{}:{}", key, val));
                }
                Some(auth)
            } else {
                return Err(DoremiError::ExternalDependency(
                    "No hay sesión de YouTube guardada para yt-dlp".to_string(),
                ));
            }
        } else {
            None
        };

        cmd.arg(&url);

        log::info!(
            "Resolving stream URL for {} using yt-dlp{}...",
            video_id,
            if use_auth { " with auth" } else { "" }
        );
        let output = match tokio::time::timeout(YT_DLP_TIMEOUT, cmd.output()).await {
            Ok(Ok(output)) => output,
            Ok(Err(e)) => {
                return Err(DoremiError::ExternalDependency(format!(
                    "No se pudo ejecutar 'yt-dlp': {e}"
                )));
            }
            Err(_) => {
                return Err(DoremiError::Network(format!(
                    "yt-dlp tardó más de {} segundos en resolver el stream",
                    YT_DLP_TIMEOUT.as_secs()
                )));
            }
        };

        if !output.status.success() {
            let stderr = String::from_utf8_lossy(&output.stderr).to_string();
            return Err(DoremiError::Network(format!(
                "Error al resolver el stream con 'yt-dlp': {}",
                stderr.trim()
            )));
        }

        let resolved_url = Self::parse_stream_url(&output.stdout).ok_or_else(|| {
            DoremiError::Network(
                "La respuesta de 'yt-dlp' no contiene una URL de stream válida".to_string(),
            )
        })?;

        Ok(resolved_url)
    }

    async fn probe_stream_url(url: &str) -> Result<(), DoremiError> {
        if !Self::is_youtube_stream_url(url) {
            return Ok(());
        }

        let client = reqwest::Client::builder()
            .connect_timeout(Duration::from_secs(3))
            .timeout(STREAM_PROBE_TIMEOUT)
            .http1_only()
            .build()
            .map_err(|error| DoremiError::Network(error.to_string()))?;

        let response = client
            .get(url)
            .headers(Self::stream_request_headers(Some("bytes=0-32767")))
            .send()
            .await
            .map_err(|error| DoremiError::Network(error.to_string()))?;

        if response.status().is_success() {
            Ok(())
        } else {
            Err(DoremiError::Network(format!(
                "stream probe returned HTTP {}",
                response.status()
            )))
        }
    }

    fn stream_request_headers(range: Option<&str>) -> HeaderMap {
        let mut headers = HeaderMap::new();
        headers.insert(
            USER_AGENT,
            HeaderValue::from_static(YOUTUBE_ANDROID_USER_AGENT),
        );
        headers.insert(ORIGIN, HeaderValue::from_static("https://www.youtube.com"));
        headers.insert(
            REFERER,
            HeaderValue::from_static("https://www.youtube.com/"),
        );
        headers.insert(ACCEPT, HeaderValue::from_static("*/*"));
        headers.insert(ACCEPT_ENCODING, HeaderValue::from_static("identity"));
        if let Some(range) = range.and_then(|value| HeaderValue::from_str(value).ok()) {
            headers.insert(RANGE, range);
        }
        headers
    }

    async fn resolve_innertube(video_id: &str) -> Result<String, DoremiError> {
        let dirs = crate::config::paths::AppDirs::global();
        let settings = crate::config::settings::AppSettings::load(&dirs.settings_path());
        let endpoint = "https://www.youtube.com/youtubei/v1/player?prettyPrint=false";
        let body = json!({
            "context": {
                "client": {
                    "clientName": "ANDROID",
                    "clientVersion": "20.10.38",
                    "androidSdkVersion": 35,
                    "hl": settings.language,
                    "gl": settings.network.region
                }
            },
            "videoId": video_id,
            "contentCheckOk": true,
            "racyCheckOk": true
        });

        let client = reqwest::Client::builder()
            .connect_timeout(Duration::from_secs(3))
            .timeout(INNERTUBE_TIMEOUT)
            .build()
            .map_err(|error| DoremiError::Network(error.to_string()))?;

        let response = client
            .post(endpoint)
            .header(
                "User-Agent",
                "com.google.android.youtube/20.10.38 (Linux; U; Android 15)",
            )
            .header("X-YouTube-Client-Name", "3")
            .header("X-YouTube-Client-Version", "20.10.38")
            .json(&body)
            .send()
            .await
            .map_err(|error| DoremiError::Network(error.to_string()))?;

        let status = response.status();
        let payload: serde_json::Value = response
            .json()
            .await
            .map_err(|error| DoremiError::Network(error.to_string()))?;

        if !status.is_success() {
            return Err(DoremiError::Network(format!(
                "InnerTube player returned HTTP {status}"
            )));
        }

        Self::select_innertube_audio_url(&payload).ok_or_else(|| {
            let reason = payload
                .pointer("/playabilityStatus/reason")
                .and_then(serde_json::Value::as_str)
                .unwrap_or("InnerTube no devolvió un formato de audio directo");
            DoremiError::Network(reason.to_string())
        })
    }

    fn cache_resolved_url(video_id: &str, cache_key: &str, resolved_url: &str) {
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
    }

    fn parse_stream_url(stdout: &[u8]) -> Option<String> {
        String::from_utf8_lossy(stdout)
            .lines()
            .map(str::trim)
            .find(|line| line.starts_with("http://") || line.starts_with("https://"))
            .map(str::to_string)
    }

    fn audio_format_selector() -> String {
        let dirs = crate::config::paths::AppDirs::global();
        let settings = crate::config::settings::AppSettings::load(&dirs.settings_path());
        match settings.network.stream_quality.to_lowercase().as_str() {
            "low" | "128k" => "bestaudio[ext=webm][acodec^=opus][abr<=128]/bestaudio[ext=webm][abr<=128]/bestaudio[abr<=128]".to_string(),
            "medium" | "192k" => "bestaudio[ext=webm][acodec^=opus][abr<=192]/bestaudio[ext=webm][abr<=192]/bestaudio[abr<=192]".to_string(),
            _ => "bestaudio[ext=webm][acodec^=opus]/bestaudio[ext=webm]/bestaudio".to_string(),
        }
    }

    fn select_innertube_audio_url(payload: &serde_json::Value) -> Option<String> {
        let dirs = crate::config::paths::AppDirs::global();
        let settings = crate::config::settings::AppSettings::load(&dirs.settings_path());
        let max_bitrate = match settings.network.stream_quality.to_lowercase().as_str() {
            "low" | "128k" => Some(128_000_u64),
            "medium" | "192k" => Some(192_000_u64),
            _ => None,
        };

        payload
            .pointer("/streamingData/adaptiveFormats")?
            .as_array()?
            .iter()
            .filter_map(|format| {
                let mime = format.get("mimeType")?.as_str()?;
                let url = format.get("url")?.as_str()?;
                if !mime.starts_with("audio/") {
                    return None;
                }
                let bitrate = format
                    .get("bitrate")
                    .and_then(|value| value.as_u64())
                    .unwrap_or(0);
                if max_bitrate.is_some_and(|max| bitrate > max) {
                    return None;
                }
                let opus_score = u8::from(mime.contains("webm") && mime.contains("opus"));
                Some((opus_score, bitrate, url))
            })
            .max_by_key(|(opus_score, bitrate, _)| (*opus_score, *bitrate))
            .map(|(_, _, url)| url.to_string())
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

    pub fn is_youtube_stream_url(url: &str) -> bool {
        url.contains("googlevideo.com") || url.contains(".googlevideo.com/")
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_parse_expiration() {
        let url =
            "https://rr2---sn-ab5sznzs.googlevideo.com/videoplayback?expire=1718330000&ei=abc";
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
        let url = format!(
            "https://googlevideo.com/videoplayback?expire={}",
            future_expire
        );
        assert!(!StreamResolver::is_url_expired(&url));

        // Past expiration
        let past_expire = current_secs - 100;
        let expired_url = format!(
            "https://googlevideo.com/videoplayback?expire={}",
            past_expire
        );
        assert!(StreamResolver::is_url_expired(&expired_url));
    }
}
