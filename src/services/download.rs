use crate::db::repo::{DownloadTrack, DownloadsRepo};
use once_cell::sync::Lazy;
use std::sync::Arc;
use tokio::sync::{mpsc, Semaphore};

#[derive(Debug, Clone)]
pub struct DownloadTask {
    pub video_id: String,
    pub title: String,
    pub artist: String,
    pub thumbnail_url: String,
}

pub struct DownloadManager {
    sender: mpsc::UnboundedSender<DownloadTask>,
    semaphore: Arc<Semaphore>,
}

static INSTANCE: Lazy<DownloadManager> = Lazy::new(|| {
    let (sender, receiver) = mpsc::unbounded_channel();
    let manager = DownloadManager {
        sender,
        semaphore: Arc::new(Semaphore::new(2)),
    };
    manager.start_worker_loop(receiver);
    manager
});

impl DownloadManager {
    pub fn get_instance() -> &'static Self {
        &INSTANCE
    }

    pub fn add_download(&self, video_id: &str, title: &str, artist: &str, thumbnail_url: &str) {
        log::info!("Queued download: {} - {}", artist, title);
        let task = DownloadTask {
            video_id: video_id.to_string(),
            title: title.to_string(),
            artist: artist.to_string(),
            thumbnail_url: thumbnail_url.to_string(),
        };
        let _ = self.sender.send(task);
        crate::bridge::bridge::show_notification(
            &format!("Descarga añadida a la cola: {}", title),
            "info",
        );
    }

    fn start_worker_loop(&self, mut receiver: mpsc::UnboundedReceiver<DownloadTask>) {
        let semaphore = self.semaphore.clone();
        tokio::spawn(async move {
            while let Some(task) = receiver.recv().await {
                // Limit concurrency using semaphore permit acquisition
                let permit = match semaphore.clone().acquire_owned().await {
                    Ok(p) => p,
                    Err(e) => {
                        log::error!("Download semaphore acquisition error: {e}");
                        continue;
                    }
                };

                tokio::spawn(async move {
                    log::info!("Starting download for video_id: {}", task.video_id);
                    match Self::download_track_impl(&task).await {
                        Ok(track) => {
                            log::info!("Download completed successfully: {}", task.title);
                            if let Err(e) = DownloadsRepo::add(&track) {
                                log::error!("Failed to save download to database: {}", e);
                            } else {
                                // Notify UI to refresh list
                                Self::refresh_downloads_ui();
                                crate::bridge::bridge::show_notification(
                                    &format!("Descarga completada: {}", task.title),
                                    "info",
                                );
                            }
                        }
                        Err(e) => {
                            log::error!("Download failed for {}: {}", task.title, e);
                            crate::bridge::bridge::show_notification(
                                &format!("Error al descargar: {}", task.title),
                                "error",
                            );
                        }
                    }
                    drop(permit);
                });
            }
        });
    }

    async fn download_track_impl(task: &DownloadTask) -> Result<DownloadTrack, String> {
        let out_dir = crate::config::paths::AppDirs::global()
            .cache_dir()
            .join("downloads");
        std::fs::create_dir_all(&out_dir)
            .map_err(|e| format!("Failed to create downloads folder: {e}"))?;

        // Determine if ffmpeg is available
        let has_ffmpeg = std::process::Command::new("ffmpeg")
            .arg("-version")
            .output()
            .is_ok();

        // Safe filename format
        let clean_artist = task.artist.replace("/", "_").replace("\\", "_");
        let clean_title = task.title.replace("/", "_").replace("\\", "_");
        let filename_base = format!("{} - {}", clean_artist, clean_title);

        let out_tmpl = out_dir.join(format!("{}.%(ext)s", filename_base));
        let out_tmpl_str = out_tmpl.to_str().ok_or("Invalid output path")?;

        let mut args = vec![
            "-f",
            "bestaudio/best",
            "--no-playlist",
            "--no-warnings",
            "-o",
            out_tmpl_str,
        ];

        if has_ffmpeg {
            args.extend([
                "--extract-audio",
                "--audio-format",
                "mp3",
                "--audio-quality",
                "192K",
            ]);
        }

        let url = format!("https://music.youtube.com/watch?v={}", task.video_id);
        args.push(&url);

        log::info!("Running yt-dlp with arguments: {:?}", args);
        let status = tokio::process::Command::new("yt-dlp")
            .args(&args)
            .status()
            .await
            .map_err(|e| format!("Failed to start yt-dlp: {e}"))?;

        if !status.success() {
            return Err("yt-dlp exited with error".to_string());
        }

        // Locate final file path
        let ext = if has_ffmpeg { "mp3" } else { "m4a" };
        let mut final_path = out_dir.join(format!("{}.{}", filename_base, ext));

        if !final_path.exists() {
            // Try checking for webm
            let webm_path = out_dir.join(format!("{}.webm", filename_base));
            if webm_path.exists() {
                final_path = webm_path;
            } else {
                // Search directory for matching filename
                let mut found = false;
                if let Ok(entries) = std::fs::read_dir(&out_dir) {
                    for entry in entries.flatten() {
                        let path = entry.path();
                        if let Some(stem) = path.file_stem().and_then(|s| s.to_str()) {
                            if stem == filename_base {
                                final_path = path;
                                found = true;
                                break;
                            }
                        }
                    }
                }
                if !found {
                    return Err("Could not locate downloaded file".to_string());
                }
            }
        }

        let file_path_str = final_path.to_str().unwrap_or_default().to_string();

        Ok(DownloadTrack {
            video_id: task.video_id.clone(),
            title: task.title.clone(),
            artist: task.artist.clone(),
            album: String::new(),
            file_path: file_path_str,
            thumbnail_url: task.thumbnail_url.clone(),
            duration_ms: 0,
            downloaded_at: String::new(), // DB defaults to current time
            parent_playlist_id: None,
            parent_playlist_title: None,
            parent_playlist_thumbnail_url: None,
        })
    }

    pub fn refresh_downloads_ui() {
        if let Ok(downloads) = DownloadsRepo::all() {
            let titles: Vec<String> = downloads.iter().map(|d| d.title.clone()).collect();
            let artists: Vec<String> = downloads.iter().map(|d| d.artist.clone()).collect();
            let thumbnails: Vec<String> =
                downloads.iter().map(|d| d.thumbnail_url.clone()).collect();
            crate::bridge::bridge::set_downloads_list(titles, artists, thumbnails);
        }
    }
}
