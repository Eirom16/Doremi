// Downloads — extracted from bridge.rs
use super::bridge;

pub fn on_download_requested(track: bridge::Track) {
    let track = super::sanitize_track_dto(track);
    log::info!("Download requested: {} — {}", track.title, track.artist);
    if super::is_playable_video_id(&track.id) {
        crate::services::download::DownloadManager::get_instance().add_download(
            &track.id,
            &track.title,
            &track.artist,
            "",
        );
    } else {
        log::warn!(
            "Cannot download: no track id provided for {} — {}",
            track.title,
            track.artist
        );
    }
}

pub fn on_download_requested_with_parent(
    track: bridge::Track,
    parent_id: &str,
    parent_title: &str,
    parent_thumbnail: &str,
) {
    let track = super::sanitize_track_dto(track);
    log::info!(
        "Download requested with parent: {} — {} (parent: {})",
        track.title,
        track.artist,
        parent_title
    );
    if super::is_playable_video_id(&track.id) {
        crate::services::download::DownloadManager::get_instance().add_download_with_parent(
            &track.id,
            &track.title,
            &track.artist,
            &track.thumbnail,
            parent_id,
            parent_title,
            parent_thumbnail,
        );
    } else {
        log::warn!(
            "Cannot download with parent: no track id provided for {} — {}",
            track.title,
            track.artist
        );
    }
}

pub fn on_downloads_requested() {
    crate::services::download::DownloadManager::refresh_downloads_ui();
}

pub fn on_batch_download_requested(
    tracks: Vec<bridge::Track>,
    parent_id: &str,
    parent_title: &str,
    parent_thumbnail: &str,
) {
    let batch: Vec<(String, String, String)> = tracks
        .iter()
        .map(|t| (t.id.clone(), t.title.clone(), t.artist.clone()))
        .collect();
    crate::services::download::DownloadManager::get_instance().add_batch_download(
        &batch,
        parent_id,
        parent_title,
        parent_thumbnail,
    );
}

pub fn on_download_cancel_requested(video_id: &str) {
    crate::services::download::DownloadManager::get_instance().cancel_download(video_id);
}

pub fn on_delete_download(video_id: &str, delete_file: bool) {
    log::info!("Delete download requested for: {video_id} (delete_file={delete_file})");
    let video_id_owned = video_id.to_string();
    tokio::task::spawn_blocking(move || {
        let path_res = crate::db::with_db(|conn| {
            let mut stmt = conn.prepare("SELECT file_path FROM downloads WHERE video_id = ?1")?;
            let path: Result<String, _> =
                stmt.query_row(rusqlite::params![video_id_owned], |r| r.get(0));
            match path {
                Ok(p) => Ok(Some(p)),
                Err(rusqlite::Error::QueryReturnedNoRows) => Ok(None),
                Err(e) => Err(e),
            }
        });

        if let Ok(Some(file_path)) = path_res {
            if let Err(e) = crate::db::repo::DownloadsRepo::remove(&video_id_owned) {
                log::error!("Failed to remove download from DB: {e}");
            }
            if delete_file && !file_path.is_empty() {
                let path = std::path::Path::new(&file_path);
                if path.exists() {
                    if let Err(e) = std::fs::remove_file(path) {
                        log::error!("Failed to remove downloaded file from disk: {e}");
                    } else {
                        log::info!("Deleted file from disk: {file_path}");
                    }
                }
            }
        }

        crate::services::download::DownloadManager::refresh_downloads_ui();
    });
}
