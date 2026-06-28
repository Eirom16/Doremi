// Browse / Detail — extracted from bridge.rs

pub fn on_home_retry_requested() {
    tokio::spawn(async { crate::services::home::HomeService::new().load_home().await });
}

pub fn on_show_requested(browse_id: &str) {
    let browse_id = browse_id.to_string();
    tokio::spawn(async move {
        crate::services::browse::load_show(&browse_id).await;
    });
}

pub fn on_home_load_more_requested() {
    tokio::spawn(async { crate::services::home::HomeService::new().load_more().await });
}

pub fn on_trending_retry_requested() {
    tokio::spawn(async {
        crate::services::trending::TrendingService::new()
            .load()
            .await
    });
}

pub fn on_album_requested(browse_id: &str) {
    let browse_id = browse_id.to_string();
    tokio::spawn(async move {
        crate::services::browse::load_album(&browse_id).await;
    });
}

pub fn on_artist_requested(browse_id: &str) {
    let browse_id = browse_id.to_string();
    tokio::spawn(async move {
        crate::services::browse::load_artist(&browse_id).await;
    });
}

pub fn on_playlist_requested(playlist_id: &str) {
    let playlist_id = playlist_id.to_string();
    tokio::spawn(async move {
        crate::services::browse::load_playlist(&playlist_id).await;
    });
}

pub fn on_playlist_requested_with_context(
    playlist_id: &str,
    title: &str,
    subtitle: &str,
    thumbnail: &str,
) {
    let playlist_id = playlist_id.to_string();
    let preview = crate::services::browse::PlaylistPreview {
        title: title.to_string(),
        subtitle: subtitle.to_string(),
        thumbnail: thumbnail.to_string(),
    };
    tokio::spawn(async move {
        crate::services::browse::load_playlist_with_preview(&playlist_id, Some(preview)).await;
    });
}
