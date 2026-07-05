//! Compatibility facade for existing callers.
//!
//! New Innertube work belongs in `auth`, `transport`, `endpoints`, or `parsers`.

pub use super::endpoints::{
    add_playlist_items, album_detail, artist_detail, charts, create_playlist, delete_playlist,
    edit_playlist, home_sections, home_sections_page, library_albums, library_artists,
    library_playlists, library_songs, mix_detail, playlist_detail, rate_song, related_tracks,
    remote_history, remove_playlist_items, remove_remote_history_items, search,
    search_suggestions, show_detail, song_like_status,
};

pub fn get_stream_url(video_id: &str) -> Option<String> {
    let cache_key = format!("stream_url:{video_id}");
    if let Some(entry) = crate::db::cache::ResponseCache::get::<String>(&cache_key) {
        log::info!("Stream URL loaded from cache (sync) for {video_id}");
        return Some(entry.data);
    }

    let mut cmd = std::process::Command::new("yt-dlp");
    cmd.args([
        "-f",
        "bestaudio/best",
        "--get-url",
        "--no-playlist",
        "--no-warnings",
    ]);
    if let Some(auth) = crate::utils::ytdlp_auth::prepare_ytdlp_auth() {
        cmd.arg("--cookies");
        cmd.arg(&auth.cookie_path);
        for (key, val) in &auth.extra_headers {
            cmd.arg("--add-header");
            cmd.arg(format!("{}:{}", key, val));
        }
        // auth is dropped after cmd runs, cleaning up temp file
    }
    cmd.arg(&format!("https://music.youtube.com/watch?v={video_id}"));

    let output = cmd.output().ok()?;
    cache_stream_output(video_id, output.status.success(), &output.stdout)
}

pub async fn get_stream_url_async(video_id: &str) -> Option<String> {
    let cache_key = format!("stream_url:{video_id}");
    if let Some(entry) = crate::db::cache::ResponseCache::get::<String>(&cache_key) {
        log::info!("Stream URL loaded from cache (async) for {video_id}");
        return Some(entry.data);
    }

    let mut cmd = tokio::process::Command::new("yt-dlp");
    cmd.args([
        "-f",
        "bestaudio/best",
        "--get-url",
        "--no-playlist",
        "--no-warnings",
    ]);
    if let Some(auth) = crate::utils::ytdlp_auth::prepare_ytdlp_auth() {
        cmd.arg("--cookies");
        cmd.arg(&auth.cookie_path);
        for (key, val) in &auth.extra_headers {
            cmd.arg("--add-header");
            cmd.arg(format!("{}:{}", key, val));
        }
        // auth is dropped after cmd runs, cleaning up temp file
    }
    cmd.arg(&format!("https://music.youtube.com/watch?v={video_id}"));

    let output = cmd.output().await.ok()?;
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
