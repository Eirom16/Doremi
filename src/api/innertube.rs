//! Compatibility facade for existing callers.
//!
//! New Innertube work belongs in `auth`, `transport`, `endpoints`, or `parsers`.

pub use super::endpoints::{charts, home_sections, related_tracks, search, search_suggestions};

pub fn get_stream_url(video_id: &str) -> Option<String> {
    let cache_key = format!("stream_url:{video_id}");
    if let Some(entry) = crate::db::cache::ResponseCache::get::<String>(&cache_key) {
        log::info!("Stream URL loaded from cache (sync) for {video_id}");
        return Some(entry.data);
    }

    let output = std::process::Command::new("yt-dlp")
        .args([
            "-f", "bestaudio/best", "--get-url", "--no-playlist", "--no-warnings",
            &format!("https://music.youtube.com/watch?v={video_id}"),
        ])
        .output()
        .ok()?;
    cache_stream_output(video_id, output.status.success(), &output.stdout)
}

pub async fn get_stream_url_async(video_id: &str) -> Option<String> {
    let cache_key = format!("stream_url:{video_id}");
    if let Some(entry) = crate::db::cache::ResponseCache::get::<String>(&cache_key) {
        log::info!("Stream URL loaded from cache (async) for {video_id}");
        return Some(entry.data);
    }

    let output = tokio::process::Command::new("yt-dlp")
        .args([
            "-f", "bestaudio/best", "--get-url", "--no-playlist", "--no-warnings",
            &format!("https://music.youtube.com/watch?v={video_id}"),
        ])
        .output()
        .await
        .ok()?;
    cache_stream_output(video_id, output.status.success(), &output.stdout)
}

fn cache_stream_output(video_id: &str, success: bool, stdout: &[u8]) -> Option<String> {
    if !success {
        return None;
    }
    let url = String::from_utf8_lossy(stdout).trim().to_string();
    if url.is_empty() {
        return None;
    }
    let cache_key = format!("stream_url:{video_id}");
    let _ = crate::db::cache::ResponseCache::set(&cache_key, &url, Some(14_400));
    Some(url)
}
