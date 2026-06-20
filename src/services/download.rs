use crate::db::repo::{DownloadTrack, DownloadsRepo};
use once_cell::sync::Lazy;
use regex::Regex;
use std::collections::HashMap;
use std::sync::atomic::{AtomicBool, Ordering};
use std::sync::{Arc, Mutex};
use tokio::sync::{mpsc, Semaphore};

#[derive(Debug, Clone, PartialEq)]
pub enum DownloadStatus {
    Queued,
    Resolving,
    Downloading(f64),
    Completed,
    Failed(String),
    Cancelled,
}

impl DownloadStatus {
    pub fn as_str(&self) -> &str {
        match self {
            Self::Queued => "queued",
            Self::Resolving => "resolving",
            Self::Downloading(_) => "downloading",
            Self::Completed => "completed",
            Self::Failed(_) => "failed",
            Self::Cancelled => "cancelled",
        }
    }

    pub fn progress(&self) -> f64 {
        match self {
            Self::Downloading(p) => *p,
            Self::Completed => 100.0,
            _ => 0.0,
        }
    }

    pub fn error(&self) -> &str {
        match self {
            Self::Failed(e) => e.as_str(),
            _ => "",
        }
    }
}

#[derive(Debug, Clone)]
pub struct DownloadTask {
    pub video_id: String,
    pub title: String,
    pub artist: String,
    pub thumbnail_url: String,
    pub parent_playlist_id: Option<String>,
    pub parent_playlist_title: Option<String>,
    pub parent_playlist_thumbnail_url: Option<String>,
}

pub struct DownloadManager {
    sender: mpsc::UnboundedSender<DownloadTask>,
    semaphore: Arc<Semaphore>,
    cancel_flags: Arc<Mutex<HashMap<String, Arc<AtomicBool>>>>,
    child_pids: Arc<Mutex<HashMap<String, u32>>>,
}

static INSTANCE: Lazy<DownloadManager> = Lazy::new(|| {
    let (sender, receiver) = mpsc::unbounded_channel();
    let manager = DownloadManager {
        sender,
        semaphore: Arc::new(Semaphore::new(2)),
        cancel_flags: Arc::new(Mutex::new(HashMap::new())),
        child_pids: Arc::new(Mutex::new(HashMap::new())),
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

        let dummy = DownloadTrack {
            video_id: video_id.to_string(),
            title: title.to_string(),
            artist: artist.to_string(),
            album: String::new(),
            file_path: String::new(),
            thumbnail_url: thumbnail_url.to_string(),
            duration_ms: 0,
            downloaded_at: String::new(),
            parent_playlist_id: None,
            parent_playlist_title: None,
            parent_playlist_thumbnail_url: None,
            status: "queued".to_string(),
            progress: 0.0,
            error: String::new(),
            cancelled: false,
        };
        if let Err(e) = DownloadsRepo::add(&dummy) {
            log::error!("Failed to save queued download: {e}");
        }

        let task = DownloadTask {
            video_id: video_id.to_string(),
            title: title.to_string(),
            artist: artist.to_string(),
            thumbnail_url: thumbnail_url.to_string(),
            parent_playlist_id: None,
            parent_playlist_title: None,
            parent_playlist_thumbnail_url: None,
        };
        let _ = self.sender.send(task);
        Self::refresh_downloads_ui();
        crate::bridge::bridge::show_notification(
            &format!("Descarga añadida a la cola: {}", title),
            "info",
        );
    }

    pub fn add_batch_download(
        &self,
        tracks: &[(String, String, String)],
        parent_id: &str,
        parent_title: &str,
        parent_thumbnail: &str,
    ) {
        log::info!(
            "Queued batch download: {} ({} tracks)",
            parent_title,
            tracks.len()
        );

        for (video_id, title, artist) in tracks {
            let dummy = DownloadTrack {
                video_id: video_id.clone(),
                title: title.clone(),
                artist: artist.clone(),
                album: String::new(),
                file_path: String::new(),
                thumbnail_url: parent_thumbnail.to_string(),
                duration_ms: 0,
                downloaded_at: String::new(),
                parent_playlist_id: Some(parent_id.to_string()),
                parent_playlist_title: Some(parent_title.to_string()),
                parent_playlist_thumbnail_url: Some(parent_thumbnail.to_string()),
                status: "queued".to_string(),
                progress: 0.0,
                error: String::new(),
                cancelled: false,
            };
            if let Err(e) = DownloadsRepo::add(&dummy) {
                log::error!("Failed to save queued batch download: {e}");
            }

            let task = DownloadTask {
                video_id: video_id.clone(),
                title: title.clone(),
                artist: artist.clone(),
                thumbnail_url: parent_thumbnail.to_string(),
                parent_playlist_id: Some(parent_id.to_string()),
                parent_playlist_title: Some(parent_title.to_string()),
                parent_playlist_thumbnail_url: Some(parent_thumbnail.to_string()),
            };
            let _ = self.sender.send(task);
        }

        Self::refresh_downloads_ui();
        crate::bridge::bridge::show_notification(
            &format!(
                "Descarga en lote añadida: {} ({} tracks)",
                parent_title,
                tracks.len()
            ),
            "info",
        );
    }

    pub fn cancel_download(&self, video_id: &str) {
        log::info!("Cancelling download: {}", video_id);

        if let Ok(flags) = self.cancel_flags.lock() {
            if let Some(flag) = flags.get(video_id) {
                flag.store(true, Ordering::SeqCst);
            }
        }

        if let Ok(pids) = self.child_pids.lock() {
            if let Some(pid) = pids.get(video_id) {
                let _ = std::process::Command::new("kill")
                    .arg(pid.to_string())
                    .output();
            }
        }

        let _ = DownloadsRepo::mark_cancelled(video_id);
        Self::refresh_downloads_ui();
    }

    pub fn clear_all(&self) {
        let ids_to_cancel: Vec<String> = {
            let flags = self.cancel_flags.lock().unwrap();
            flags.keys().cloned().collect()
        };
        for id in &ids_to_cancel {
            self.cancel_download(id);
        }

        let _ = DownloadsRepo::clear_all();
        Self::refresh_downloads_ui();
    }

    pub fn resume_unfinished_downloads(&self) {
        log::info!("Checking for unfinished downloads to resume...");
        if let Ok(tracks) = DownloadsRepo::all() {
            for track in tracks {
                if track.status == "queued"
                    || track.status == "resolving"
                    || track.status == "downloading"
                {
                    log::info!(
                        "Resuming unfinished download: {} - {}",
                        track.artist,
                        track.title
                    );
                    let _ = DownloadsRepo::update_status(&track.video_id, "queued", 0.0, "");
                    let task = DownloadTask {
                        video_id: track.video_id.clone(),
                        title: track.title.clone(),
                        artist: track.artist.clone(),
                        thumbnail_url: track.thumbnail_url.clone(),
                        parent_playlist_id: track.parent_playlist_id.clone(),
                        parent_playlist_title: track.parent_playlist_title.clone(),
                        parent_playlist_thumbnail_url: track.parent_playlist_thumbnail_url.clone(),
                    };
                    let _ = self.sender.send(task);
                }
            }
        }
    }

    fn save_status(video_id: &str, status: &DownloadStatus) {
        let _ = DownloadsRepo::update_status(
            video_id,
            status.as_str(),
            status.progress(),
            status.error(),
        );
        crate::bridge::bridge::set_download_progress(video_id, status.progress(), status.as_str());
    }

    fn start_worker_loop(&self, mut receiver: mpsc::UnboundedReceiver<DownloadTask>) {
        let semaphore = self.semaphore.clone();
        let cancel_flags = self.cancel_flags.clone();
        let child_pids = self.child_pids.clone();

        tokio::spawn(async move {
            while let Some(task) = receiver.recv().await {
                let permit = match semaphore.clone().acquire_owned().await {
                    Ok(p) => p,
                    Err(e) => {
                        log::error!("Download semaphore acquisition error: {e}");
                        continue;
                    }
                };

                let cflags = cancel_flags.clone();
                let cpids = child_pids.clone();

                tokio::spawn(async move {
                    log::info!("Starting download for video_id: {}", task.video_id);

                    let cancel_flag = Arc::new(AtomicBool::new(false));
                    {
                        let mut flags = cflags.lock().unwrap();
                        flags.insert(task.video_id.clone(), cancel_flag.clone());
                    }

                    Self::save_status(&task.video_id, &DownloadStatus::Resolving);

                    match Self::download_track_impl(&task, &cancel_flag, &cpids).await {
                        Ok(track) => {
                            if cancel_flag.load(Ordering::SeqCst) {
                                log::info!(
                                    "Download cancelled after completion check: {}",
                                    task.title
                                );
                            } else {
                                log::info!("Download completed successfully: {}", task.title);
                                if let Err(e) = DownloadsRepo::add(&track) {
                                    log::error!("Failed to save download to database: {}", e);
                                    Self::save_status(
                                        &task.video_id,
                                        &DownloadStatus::Failed(e.to_string()),
                                    );
                                } else {
                                    Self::save_status(&task.video_id, &DownloadStatus::Completed);
                                    Self::refresh_downloads_ui();
                                    if let Some(ref parent_id) = task.parent_playlist_id {
                                        Self::notify_batch_progress(parent_id);
                                    }
                                    crate::bridge::bridge::show_notification(
                                        &format!("Descarga completada: {}", task.title),
                                        "info",
                                    );
                                }
                            }
                        }
                        Err(e) => {
                            if cancel_flag.load(Ordering::SeqCst) {
                                log::info!("Download cancelled: {}", task.title);
                            } else {
                                log::error!("Download failed for {}: {}", task.title, e);
                                Self::save_status(
                                    &task.video_id,
                                    &DownloadStatus::Failed(e.clone()),
                                );
                                if let Some(ref parent_id) = task.parent_playlist_id {
                                    Self::notify_batch_progress(parent_id);
                                }
                                crate::bridge::bridge::show_notification(
                                    &format!("Error al descargar: {}", task.title),
                                    "error",
                                );
                            }
                        }
                    }

                    {
                        let mut flags = cflags.lock().unwrap();
                        flags.remove(&task.video_id);
                    }
                    {
                        let mut pids = cpids.lock().unwrap();
                        pids.remove(&task.video_id);
                    }

                    drop(permit);
                });
            }
        });
    }

    async fn download_track_impl(
        task: &DownloadTask,
        cancel_flag: &AtomicBool,
        child_pids: &Arc<Mutex<HashMap<String, u32>>>,
    ) -> Result<DownloadTrack, String> {
        let res = Self::download_track_impl_inner(task, cancel_flag, child_pids).await;
        if res.is_err() {
            let dirs = crate::config::paths::AppDirs::global();
            let settings = crate::config::settings::AppSettings::load(&dirs.settings_path());
            let out_dir = if !settings.downloads.location.is_empty() {
                std::path::PathBuf::from(&settings.downloads.location)
            } else {
                dirs.cache_dir().join("downloads")
            };
            let temp_prefix = format!("{}_download.tmp", task.video_id);
            if let Ok(entries) = std::fs::read_dir(&out_dir) {
                for entry in entries.flatten() {
                    let path = entry.path();
                    if let Some(name) = path.file_name().and_then(|n| n.to_str()) {
                        if name.starts_with(&temp_prefix) {
                            let _ = std::fs::remove_file(path);
                        }
                    }
                }
            }
        }
        res
    }

    async fn download_track_impl_inner(
        task: &DownloadTask,
        cancel_flag: &AtomicBool,
        child_pids: &Arc<Mutex<HashMap<String, u32>>>,
    ) -> Result<DownloadTrack, String> {
        let dirs = crate::config::paths::AppDirs::global();
        let settings = crate::config::settings::AppSettings::load(&dirs.settings_path());

        let out_dir = if !settings.downloads.location.is_empty() {
            std::path::PathBuf::from(&settings.downloads.location)
        } else {
            dirs.cache_dir().join("downloads")
        };

        std::fs::create_dir_all(&out_dir)
            .map_err(|e| format!("Failed to create downloads folder: {e}"))?;

        if cancel_flag.load(Ordering::SeqCst) {
            return Err("Cancelled".to_string());
        }

        let has_ffmpeg = std::process::Command::new("ffmpeg")
            .arg("-version")
            .output()
            .is_ok();

        let clean_artist = task.artist.replace("/", "_").replace("\\", "_");
        let clean_title = task.title.replace("/", "_").replace("\\", "_");
        let filename_base = format!("{} - {}", clean_artist, clean_title);

        let temp_filename_base = format!("{}_download.tmp", task.video_id);
        let out_tmpl = out_dir.join(format!("{}.%(ext)s", temp_filename_base));
        let out_tmpl_str = out_tmpl.to_str().ok_or("Invalid output path")?;

        log::info!("Running yt-dlp for download: {}", task.video_id);

        let mut child = tokio::process::Command::new("yt-dlp");
        child
            .arg("-f")
            .arg("bestaudio/best")
            .arg("--no-playlist")
            .arg("--no-warnings")
            .arg("--newline")
            .arg("-o")
            .arg(out_tmpl_str)
            .stdout(std::process::Stdio::piped())
            .stderr(std::process::Stdio::piped())
            .kill_on_drop(true);

        let dl_format = settings.downloads.format.to_lowercase();
        let dl_quality = settings.downloads.quality.to_lowercase();

        if has_ffmpeg {
            if dl_format != "original" {
                child
                    .arg("--extract-audio")
                    .arg("--audio-format")
                    .arg(&dl_format);

                let q_arg = match dl_quality.as_str() {
                    "best" => "0",
                    "320k" => "320K",
                    "256k" => "256K",
                    "192k" => "192K",
                    "128k" => "128K",
                    other => other,
                };
                child.arg("--audio-quality").arg(q_arg);
            }
        }

        if let Some(auth) = crate::utils::ytdlp_auth::prepare_ytdlp_auth() {
            child.arg("--cookies").arg(&auth.cookie_path);
            for (key, val) in &auth.extra_headers {
                child.arg("--add-header").arg(format!("{}:{}", key, val));
            }
            // auth is dropped after spawn, cleaning up temp file
        }

        let url = format!("https://music.youtube.com/watch?v={}", task.video_id);
        child.arg(&url);

        let mut child = child
            .spawn()
            .map_err(|e| format!("Failed to start yt-dlp: {e}"))?;

        {
            let mut pids = child_pids.lock().unwrap();
            pids.insert(task.video_id.clone(), child.id().unwrap_or(0));
        }

        if cancel_flag.load(Ordering::SeqCst) {
            let _ = child.kill().await;
            return Err("Cancelled".to_string());
        }

        Self::save_status(&task.video_id, &DownloadStatus::Downloading(0.0));

        let progress_re = Regex::new(r"\[download\]\s+(\d+\.?\d*)%").unwrap();

        let stdout = child.stdout.take().ok_or("No stdout")?;
        use tokio::io::AsyncBufReadExt;
        let reader = tokio::io::BufReader::new(stdout);
        let mut lines = reader.lines();

        loop {
            if cancel_flag.load(Ordering::SeqCst) {
                let _ = child.kill().await;
                return Err("Cancelled".to_string());
            }

            match tokio::time::timeout(std::time::Duration::from_millis(200), lines.next_line())
                .await
            {
                Ok(Ok(Some(line))) => {
                    if let Some(caps) = progress_re.captures(&line) {
                        if let Ok(pct) = caps[1].parse::<f64>() {
                            Self::save_status(&task.video_id, &DownloadStatus::Downloading(pct));
                        }
                    }
                    log::debug!("yt-dlp: {}", line);
                }
                Ok(Ok(None)) => break,
                Ok(Err(e)) => {
                    log::warn!("yt-dlp stdout read error: {e}");
                    break;
                }
                Err(_timeout) => {}
            }
        }

        let output = child
            .wait()
            .await
            .map_err(|e| format!("yt-dlp wait error: {e}"))?;

        if cancel_flag.load(Ordering::SeqCst) {
            return Err("Cancelled".to_string());
        }

        if !output.success() {
            let stderr = child
                .stderr
                .take()
                .map(|s| {
                    use tokio::io::AsyncReadExt;
                    let mut buf = String::new();
                    let mut reader = tokio::io::BufReader::new(s);
                    let _ = reader.read_to_string(&mut buf);
                    buf
                })
                .unwrap_or_default();
            let err_msg = if stderr.trim().is_empty() {
                "yt-dlp exited with error".to_string()
            } else {
                stderr.trim().to_string()
            };
            return Err(err_msg);
        }

        let ext = if has_ffmpeg {
            if dl_format == "original" {
                "m4a"
            } else {
                &dl_format
            }
        } else {
            "m4a"
        };
        let mut temp_final_path = out_dir.join(format!("{}.{}", temp_filename_base, ext));

        if !temp_final_path.exists() {
            let webm_path = out_dir.join(format!("{}.webm", temp_filename_base));
            if webm_path.exists() {
                temp_final_path = webm_path;
            } else {
                let mut found = false;
                if let Ok(entries) = std::fs::read_dir(&out_dir) {
                    for entry in entries.flatten() {
                        let path = entry.path();
                        if let Some(stem) = path.file_stem().and_then(|s| s.to_str()) {
                            if stem == temp_filename_base {
                                temp_final_path = path;
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

        let final_ext = temp_final_path
            .extension()
            .and_then(|e| e.to_str())
            .unwrap_or(ext);
        let final_path = out_dir.join(format!("{}.{}", filename_base, final_ext));

        // Atomic rename
        std::fs::rename(&temp_final_path, &final_path)
            .map_err(|e| format!("Failed to rename temporary file to final path: {e}"))?;

        let file_path_str = final_path.to_str().unwrap_or_default().to_string();

        Ok(DownloadTrack {
            video_id: task.video_id.clone(),
            title: task.title.clone(),
            artist: task.artist.clone(),
            album: String::new(),
            file_path: file_path_str,
            thumbnail_url: task.thumbnail_url.clone(),
            duration_ms: 0,
            downloaded_at: String::new(),
            parent_playlist_id: task.parent_playlist_id.clone(),
            parent_playlist_title: task.parent_playlist_title.clone(),
            parent_playlist_thumbnail_url: task.parent_playlist_thumbnail_url.clone(),
            status: "completed".to_string(),
            progress: 100.0,
            error: String::new(),
            cancelled: false,
        })
    }

    pub fn refresh_downloads_ui() {
        if let Ok(downloads) = DownloadsRepo::all() {
            let titles: Vec<String> = downloads.iter().map(|d| d.title.clone()).collect();
            let artists: Vec<String> = downloads.iter().map(|d| d.artist.clone()).collect();
            let thumbnails: Vec<String> =
                downloads.iter().map(|d| d.thumbnail_url.clone()).collect();
            let video_ids: Vec<String> = downloads.iter().map(|d| d.video_id.clone()).collect();
            let statuses: Vec<String> = downloads.iter().map(|d| d.status.clone()).collect();
            let progresses: Vec<f64> = downloads.iter().map(|d| d.progress).collect();
            crate::bridge::bridge::set_downloads_list(
                titles, artists, thumbnails, video_ids, statuses, progresses,
            );
        }
    }

    fn notify_batch_progress(parent_id: &str) {
        if let Ok((total, completed, avg_progress)) = DownloadsRepo::batch_progress(parent_id) {
            crate::bridge::bridge::set_batch_download_progress(
                parent_id,
                total as i32,
                completed as i32,
                avg_progress,
            );
        }
    }
}
