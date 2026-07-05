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
        if let Err(e) = self.sender.send(task) {
            log::error!("Failed to enqueue download task: {e}");
        }
        Self::refresh_downloads_ui();
        crate::bridge::bridge::show_notification(
            &format!("Descarga añadida a la cola: {}", title),
            "info",
        );
    }

    pub fn add_download_with_parent(
        &self,
        video_id: &str,
        title: &str,
        artist: &str,
        thumbnail_url: &str,
        parent_playlist_id: &str,
        parent_playlist_title: &str,
        parent_playlist_thumbnail_url: &str,
    ) {
        log::info!(
            "Queued download with parent: {} - {} (parent: {})",
            artist,
            title,
            parent_playlist_title
        );

        let dummy = DownloadTrack {
            video_id: video_id.to_string(),
            title: title.to_string(),
            artist: artist.to_string(),
            album: String::new(),
            file_path: String::new(),
            thumbnail_url: thumbnail_url.to_string(),
            duration_ms: 0,
            downloaded_at: String::new(),
            parent_playlist_id: Some(parent_playlist_id.to_string()),
            parent_playlist_title: Some(parent_playlist_title.to_string()),
            parent_playlist_thumbnail_url: Some(parent_playlist_thumbnail_url.to_string()),
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
            parent_playlist_id: Some(parent_playlist_id.to_string()),
            parent_playlist_title: Some(parent_playlist_title.to_string()),
            parent_playlist_thumbnail_url: Some(parent_playlist_thumbnail_url.to_string()),
        };
        if let Err(e) = self.sender.send(task) {
            log::error!("Failed to enqueue download task: {e}");
        }
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
            if let Err(e) = self.sender.send(task) {
                log::error!("Failed to enqueue batch download task: {e}");
            }
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
            if let Some(&pid) = pids.get(video_id) {
                // BF1.2: never send kill with pid=0 — in POSIX that signals the
                // entire process group, which would terminate the whole application.
                if pid > 0 {
                    if let Err(e) = std::process::Command::new("kill")
                        .arg(pid.to_string())
                        .output()
                    {
                        log::warn!("Failed to execute kill command for pid {pid}: {e}");
                    }
                }
            }
        }

        if let Err(e) = DownloadsRepo::mark_cancelled(video_id) {
            log::warn!("Failed to mark download as cancelled in DB for {video_id}: {e}");
        }
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

        if let Err(e) = DownloadsRepo::clear_all() {
            log::warn!("Failed to clear all downloads in DB: {e}");
        }
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
                    if let Err(e) = DownloadsRepo::update_status(&track.video_id, "queued", 0.0, "") {
                        log::warn!("Failed to reset download status in DB for {}: {}", track.video_id, e);
                    }
                    let task = DownloadTask {
                        video_id: track.video_id.clone(),
                        title: track.title.clone(),
                        artist: track.artist.clone(),
                        thumbnail_url: track.thumbnail_url.clone(),
                        parent_playlist_id: track.parent_playlist_id.clone(),
                        parent_playlist_title: track.parent_playlist_title.clone(),
                        parent_playlist_thumbnail_url: track.parent_playlist_thumbnail_url.clone(),
                    };
                    if let Err(e) = self.sender.send(task) {
                        log::error!("Failed to enqueue resumed download task: {e}");
                    }
                }
            }
        }
    }

    fn save_status(video_id: &str, status: &DownloadStatus) {
        if let Err(e) = DownloadsRepo::update_status(
            video_id,
            status.as_str(),
            status.progress(),
            status.error(),
        ) {
            log::warn!("Failed to save status in DB for {video_id}: {e}");
        }
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

        tokio::fs::create_dir_all(&out_dir)
            .await
            .map_err(|e| format!("Failed to create downloads folder: {e}"))?;

        if cancel_flag.load(Ordering::SeqCst) {
            return Err("Cancelled".to_string());
        }

        let has_ffmpeg = tokio::process::Command::new("ffmpeg")
            .arg("-version")
            .output()
            .await
            .is_ok();

        let clean_artist = sanitize_filename(&task.artist);
        let clean_title = sanitize_filename(&task.title);
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
            .arg("--extractor-args")
            .arg("youtube:player_client=android")
            .arg("--socket-timeout")
            .arg("15")
            .arg("--extractor-retries")
            .arg("1")
            .arg("--retries")
            .arg("2")
            .arg("--fragment-retries")
            .arg("2")
            .arg("-o")
            .arg(out_tmpl_str)
            .stdout(std::process::Stdio::piped())
            .stderr(std::process::Stdio::piped())
            .kill_on_drop(true);

        let dl_format = settings.downloads.format.to_lowercase();
        let dl_quality = settings.downloads.quality.to_lowercase();

        if has_ffmpeg {
            child.arg("--embed-metadata");
            child.arg("--embed-thumbnail");
            child.arg("--convert-thumbnails").arg("jpg");
            // Also write thumbnail as separate file in case embedding fails
            // (e.g. for webm/opus containers that don't support cover art)
            child.arg("--write-thumbnail");

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

        let url = format!("https://www.youtube.com/watch?v={}", task.video_id);
        child.arg(&url);

        let mut child = child
            .spawn()
            .map_err(|e| format!("Failed to start yt-dlp: {e}"))?;

        if let Some(pid) = child.id() {
            let mut pids = child_pids.lock().unwrap();
            pids.insert(task.video_id.clone(), pid);
        }

        if cancel_flag.load(Ordering::SeqCst) {
            let _ = child.kill().await;
            return Err("Cancelled".to_string());
        }

        Self::save_status(&task.video_id, &DownloadStatus::Downloading(0.0));

        // BF2.7: Regex is compiled once at first use instead of once per download.
        static PROGRESS_RE: std::sync::LazyLock<Regex> =
            std::sync::LazyLock::new(|| Regex::new(r"\[download\]\s+(\d+\.?\d*)%").unwrap());
        let progress_re = &*PROGRESS_RE;

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

        // BF1.3: Drain stderr in a concurrent task so that the pipe buffer cannot fill
        // up and block the yt-dlp process while we are waiting on stdout.
        let stderr_handle = child.stderr.take().map(|stderr| {
            tokio::spawn(async move {
                use tokio::io::AsyncReadExt;
                let mut buf = String::new();
                let mut reader = tokio::io::BufReader::new(stderr);
                let _ = reader.read_to_string(&mut buf).await;
                buf
            })
        });

        let output = child
            .wait()
            .await
            .map_err(|e| format!("yt-dlp wait error: {e}"))?;

        if cancel_flag.load(Ordering::SeqCst) {
            return Err("Cancelled".to_string());
        }

        // Collect stderr output that was being drained concurrently.
        let stderr_str = if let Some(handle) = stderr_handle {
            handle.await.unwrap_or_default()
        } else {
            String::new()
        };

        if !output.success() {
            let err_msg = if stderr_str.trim().is_empty() {
                "yt-dlp exited with error".to_string()
            } else {
                stderr_str.trim().to_string()
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

        if tokio::fs::metadata(&temp_final_path).await.is_err() {
            let webm_path = out_dir.join(format!("{}.webm", temp_filename_base));
            if tokio::fs::metadata(&webm_path).await.is_ok() {
                temp_final_path = webm_path;
            } else {
                let mut found = false;
                let out_dir_clone = out_dir.clone();
                let temp_filename_base_clone = temp_filename_base.clone();
                let search_res = tokio::task::spawn_blocking(move || {
                    if let Ok(entries) = std::fs::read_dir(&out_dir_clone) {
                        for entry in entries.flatten() {
                            let path = entry.path();
                            if let Some(stem) = path.file_stem().and_then(|s| s.to_str()) {
                                if stem == temp_filename_base_clone {
                                    return Some(path);
                                }
                            }
                        }
                    }
                    None
                }).await;
                if let Ok(Some(path)) = search_res {
                    temp_final_path = path;
                    found = true;
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
        let mut final_path = out_dir.join(format!("{}.{}", filename_base, final_ext));
        let mut index = 1;
        while tokio::fs::metadata(&final_path).await.is_ok() {
            final_path = out_dir.join(format!("{} ({}).{}", filename_base, index, final_ext));
            index += 1;
        }

        // Atomic rename
        tokio::fs::rename(&temp_final_path, &final_path)
            .await
            .map_err(|e| format!("Failed to rename temporary file to final path: {e}"))?;

        if tokio::fs::metadata(&final_path).await.is_err() {
            return Err("Downloaded file does not exist at final path".to_string());
        }
        let size = tokio::fs::metadata(&final_path).await.map(|m| m.len()).unwrap_or(0);
        if size == 0 {
            return Err("Downloaded file is empty (0 bytes)".to_string());
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

    pub fn reconcile_downloads() {
        log::info!("Reconciling downloads database with disk files...");
        if let Ok(downloads) = DownloadsRepo::all() {
            for track in downloads {
                if track.status == "completed" {
                    let path = std::path::Path::new(&track.file_path);
                    if !path.exists() || !path.is_file() {
                        log::warn!(
                            "File for downloaded track {} - {} not found at {}. Marking as failed.",
                            track.artist,
                            track.title,
                            track.file_path
                        );
                        let _ = DownloadsRepo::update_status(
                            &track.video_id,
                            "failed",
                            0.0,
                            "El archivo fue movido o eliminado externamente",
                        );
                    }
                }
            }
        }
    }

    pub fn refresh_downloads_ui() {
        Self::reconcile_downloads();
        if let Ok(downloads) = DownloadsRepo::all() {
            let items: Vec<crate::bridge::bridge::DownloadItem> = downloads
                .iter()
                .map(|d| crate::bridge::bridge::DownloadItem {
                    video_id: d.video_id.clone(),
                    title: d.title.clone(),
                    artist: d.artist.clone(),
                    album: d.album.clone(),
                    thumbnail_url: d.thumbnail_url.clone(),
                    parent_playlist_id: d.parent_playlist_id.clone().unwrap_or_default(),
                    parent_playlist_title: d.parent_playlist_title.clone().unwrap_or_default(),
                    parent_playlist_thumbnail_url: d
                        .parent_playlist_thumbnail_url
                        .clone()
                        .unwrap_or_default(),
                    status: d.status.clone(),
                    progress: d.progress,
                })
                .collect();
            crate::bridge::bridge::set_downloads_list(items);
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

fn sanitize_filename(name: &str) -> String {
    let mut clean = String::new();
    for c in name.chars() {
        if c.is_control() || "/\\?%*:|\"<>".contains(c) {
            clean.push('_');
        } else {
            clean.push(c);
        }
    }
    // Collapse multiple underscores
    let mut collapsed = String::new();
    let mut last_was_underscore = false;
    for c in clean.chars() {
        if c == '_' {
            if !last_was_underscore {
                collapsed.push('_');
                last_was_underscore = true;
            }
        } else {
            collapsed.push(c);
            last_was_underscore = false;
        }
    }
    let trimmed = collapsed.trim_matches(|c| c == '_' || c == ' ' || c == '.');
    if trimmed.is_empty() {
        "Track".to_string()
    } else {
        trimmed.to_string()
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_sanitize_filename() {
        assert_eq!(sanitize_filename("Normal Title"), "Normal Title");
        assert_eq!(sanitize_filename("Title / Slash"), "Title _ Slash");
        assert_eq!(sanitize_filename("Title? * : | \" < > ."), "Title");
        assert_eq!(sanitize_filename("   "), "Track");
        assert_eq!(sanitize_filename("..."), "Track");
        assert_eq!(
            sanitize_filename("Title with dot.mp3"),
            "Title with dot.mp3"
        );
    }
}
