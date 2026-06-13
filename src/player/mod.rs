pub mod vlc_check;
pub mod state;
pub mod queue;
pub mod audio;
pub mod resolver;

use std::sync::{Arc, Mutex};
use std::time::{Duration, Instant};

use audio::AudioEngine;
use queue::{PlaybackQueue, TrackInfo};
use state::PlayState;

pub struct PlayerService {
    audio: AudioEngine,
    queue: Arc<Mutex<PlaybackQueue>>,
    last_poll: Arc<Mutex<Instant>>,
    sleep_timer_end: Arc<Mutex<Option<Instant>>>,
    last_track_id: Mutex<String>,
    last_is_playing: std::sync::atomic::AtomicBool,
    lastfm_scrobbled: std::sync::atomic::AtomicBool,
    accumulated_playback_ms: std::sync::atomic::AtomicI64,
    last_playback_poll: Mutex<Instant>,
    last_retried_track_id: Mutex<Option<String>>,
}

impl PlayerService {
    pub fn new() -> Self {
        let audio = AudioEngine::new();
        if !audio.is_available() {
            log::warn!("VLC audio engine not available");
        } else {
            log::info!("VLC audio engine initialized");
        }

        Self {
            audio,
            queue: Arc::new(Mutex::new(PlaybackQueue::new())),
            last_poll: Arc::new(Mutex::new(Instant::now())),
            sleep_timer_end: Arc::new(Mutex::new(None)),
            last_track_id: Mutex::new(String::new()),
            last_is_playing: std::sync::atomic::AtomicBool::new(false),
            lastfm_scrobbled: std::sync::atomic::AtomicBool::new(false),
            accumulated_playback_ms: std::sync::atomic::AtomicI64::new(0),
            last_playback_poll: Mutex::new(Instant::now()),
            last_retried_track_id: Mutex::new(None),
        }
    }

    pub fn is_available(&self) -> bool {
        self.audio.is_available()
    }

    // ── Queue management ──

    pub fn enqueue(&self, track: TrackInfo) {
        let mut queue = self.queue.lock().unwrap();
        queue.enqueue(track);
    }

    pub fn enqueue_front(&self, track: TrackInfo) {
        let mut queue = self.queue.lock().unwrap();
        queue.enqueue_front(track);
    }

    pub fn remove(&self, index: usize) -> Option<TrackInfo> {
        self.queue.lock().unwrap().remove(index)
    }

    pub fn clear_queue(&self) {
        self.queue.lock().unwrap().clear();
    }

    pub fn current_track(&self) -> Option<TrackInfo> {
        self.queue.lock().unwrap().current().cloned()
    }

    // ── Playback control ──

    pub fn play_search_result(&self, title: &str, artist: &str, video_id: Option<&str>) {
        log::info!("Play search result: {title} — {artist}");
        let track = crate::player::queue::TrackInfo {
            id: video_id.unwrap_or("").to_string(),
            title: title.to_string(),
            artist: artist.to_string(),
            album: String::new(),
            thumbnail: String::new(),
            duration_ms: 0,
            stream_url: String::new(),
        };
        {
            let mut queue = self.queue.lock().unwrap();
            queue.clear();
            queue.enqueue_front(track);
        }
        self.play_index(0);
    }

    pub fn play_track_dto(&self, track: crate::bridge::bridge::Track) {
        log::info!("Play track DTO: {} — {}", track.title, track.artist);
        let track_info = crate::player::queue::TrackInfo {
            id: track.id,
            title: track.title,
            artist: track.artist,
            album: track.album,
            thumbnail: track.thumbnail,
            duration_ms: track.duration_ms,
            stream_url: String::new(),
        };
        {
            let mut queue = self.queue.lock().unwrap();
            queue.clear();
            queue.enqueue_front(track_info);
        }
        self.play_index(0);
    }

    pub fn stop(&self) {
        self.audio.stop();
    }

    pub fn play_url(&self, url: &str) {
        self.audio.play_url(url);
        drop(self.last_poll.lock().unwrap());
    }

    pub fn play_index(&self, index: usize) {
        let track = {
            let mut queue = self.queue.lock().unwrap();
            queue.jump_to(index).cloned()
        };
        if let Some(t) = track {
            self.play_track_info(t);
        }
    }

    fn play_track_info(&self, t: TrackInfo) {
        // Check if the track is downloaded locally
        let mut local_path = None;
        if !t.id.is_empty() {
            if let Ok(Some(download)) = crate::db::repo::DownloadsRepo::get(&t.id) {
                local_path = Some(download.file_path);
            }
        }

        if let Some(path) = local_path {
            log::info!("Playing offline downloaded track from path: {}", path);
            {
                if let Ok(mut q) = self.queue.lock() {
                    if let Some(current_track) = q.current_mut() {
                        if current_track.id == t.id {
                            current_track.stream_url = path.clone();
                        }
                    }
                }
            }
            self.audio.play_url(&path);
            return;
        }

        let mut needs_resolution = t.stream_url.is_empty();
        if !needs_resolution && t.stream_url.starts_with("http") && crate::player::resolver::StreamResolver::is_url_expired(&t.stream_url) {
            log::info!("Stream URL for {} is expired, forcing re-resolution", t.id);
            needs_resolution = true;
        }

        if needs_resolution {
            if t.id.is_empty() {
                log::warn!("Cannot play track with empty ID and no stream URL");
                return;
            }
            let queue = self.queue.clone();
            let audio = self.audio.clone();
            let id = t.id.clone();

            self.audio.set_state(PlayState::Loading);

            tokio::spawn(async move {
                log::info!("Resolving stream URL in background for {id} using StreamResolver...");
                let resolve_result = crate::player::resolver::StreamResolver::resolve(&id).await;
                match resolve_result {
                    Ok(resolved_url) => {
                        let still_current = {
                            if let Ok(mut q) = queue.lock() {
                                if let Some(current_track) = q.current_mut() {
                                    if current_track.id == id {
                                        current_track.stream_url = resolved_url.clone();
                                        true
                                    } else {
                                        false
                                    }
                                } else {
                                    false
                                }
                            } else {
                                false
                            }
                        };

                        if still_current {
                            log::info!("Playing resolved URL for {id}");
                            audio.play_url(&resolved_url);
                        } else {
                            log::info!("Resolved URL for {id}, but user already switched tracks.");
                        }
                    }
                    Err(e) => {
                        log::error!("Failed to resolve stream URL for {id}: {e:?}");
                        audio.set_state(PlayState::Stopped);
                        crate::bridge::bridge::show_notification(
                            &format!("Error al resolver stream: {e}"),
                            "error",
                        );
                    }
                }
            });
        } else {
            self.audio.play_url(&t.stream_url);
        }
    }

    pub fn toggle_play_pause(&self) {
        self.audio.toggle_play_pause();
    }

    pub fn next(&self) {
        let next_track = {
            let mut queue = self.queue.lock().unwrap();
            queue.next().cloned()
        };
        if let Some(t) = next_track {
            self.play_track_info(t);
        } else {
            self.audio.stop();
        }
    }

    pub fn previous(&self) {
        let prev_track = {
            let mut queue = self.queue.lock().unwrap();
            if self.audio.position_ms() > 3000 {
                self.audio.seek(0);
                return;
            }
            queue.previous().cloned()
        };
        if let Some(t) = prev_track {
            self.play_track_info(t);
        }
    }

    pub fn seek(&self, position_ms: i64) {
        self.audio.seek(position_ms);
    }

    pub fn seek_relative(&self, delta_ms: i64) {
        let pos = self.audio.position_ms().max(0);
        self.audio.seek((pos + delta_ms).max(0));
    }

    pub fn volume(&self) -> i32 {
        self.audio.volume()
    }

    pub fn set_volume(&self, vol: i32) {
        self.audio.set_volume(vol);
    }

    pub fn adjust_volume(&self, delta: i32) {
        self.audio.adjust_volume(delta);
    }

    // ── State queries ──

    pub fn state(&self) -> PlayState {
        self.audio.state()
    }

    pub fn position_ms(&self) -> i64 {
        self.audio.position_ms()
    }

    pub fn duration_ms(&self) -> i64 {
        self.audio.duration_ms()
    }

    pub fn is_playing(&self) -> bool {
        self.audio.state().is_playing()
    }

    pub fn toggle_shuffle(&self) {
        self.queue.lock().unwrap().toggle_shuffle();
    }

    pub fn cycle_repeat(&self) {
        self.queue.lock().unwrap().cycle_repeat();
    }

    pub fn shuffle_mode(&self) -> bool {
        self.queue.lock().unwrap().shuffle_mode()
    }

    pub fn repeat_mode(&self) -> i32 {
        match self.queue.lock().unwrap().repeat_mode() {
            queue::RepeatMode::None => 0,
            queue::RepeatMode::All => 1,
            queue::RepeatMode::One => 2,
        }
    }

    // ── Polling (called periodically from the bridge or a timer) ──

    pub fn poll(&self) {
        let now = Instant::now();
        let mut last = self.last_poll.lock().unwrap();
        if now.duration_since(*last) < Duration::from_millis(250) {
            return;
        }
        *last = now;

        // Check sleep timer
        let trigger_stop = {
            let mut timer = self.sleep_timer_end.lock().unwrap();
            if let Some(end_time) = *timer {
                if Instant::now() >= end_time {
                    *timer = None; // Reset timer
                    true
                } else {
                    false
                }
            } else {
                false
            }
        };

        if trigger_stop {
            log::info!("Sleep timer expired. Stopping playback.");
            self.audio.stop();
            // Update settings to reset the sleep timer value
            let dirs = crate::config::paths::AppDirs::global();
            let mut settings = crate::config::settings::AppSettings::load(&dirs.settings_path());
            settings.player.sleep_timer_minutes = 0;
            let _ = settings.save(&dirs.settings_path());
            crate::bridge::bridge::show_notification("Temporizador de apagado finalizado.", "info");
        }

        self.audio.poll_position();

        // Check for playback error and handle retry
        if self.audio.has_error() {
            if let Some(track) = self.current_track() {
                let mut retried = false;
                {
                    let mut retried_id = self.last_retried_track_id.lock().unwrap();
                    if *retried_id == Some(track.id.clone()) {
                        retried = true;
                    } else {
                        *retried_id = Some(track.id.clone());
                    }
                }

                if !retried {
                    log::warn!(
                        "Playback error detected for track {} ({}). Retrying resolution and playback once...",
                        track.id,
                        track.title
                    );
                    
                    // Invalidate cache
                    let cache_key = format!("stream_url:{}", track.id);
                    let _ = crate::db::cache::ResponseCache::invalidate(&cache_key);

                    // Clear stream url in the queue
                    {
                        if let Ok(mut q) = self.queue.lock() {
                            if let Some(current_track) = q.current_mut() {
                                if current_track.id == track.id {
                                    current_track.stream_url.clear();
                                }
                            }
                        }
                    }

                    // Reset audio player state to stopped to clear error state
                    self.audio.stop();

                    // Re-trigger playback
                    let mut retried_track = track.clone();
                    retried_track.stream_url.clear();
                    self.play_track_info(retried_track);
                } else {
                    log::error!(
                        "Playback failed again after retry for track {} ({}). Stopping.",
                        track.id,
                        track.title
                    );
                    
                    // Clear the retried ID so we can try playing it again manually later
                    {
                        let mut retried_id = self.last_retried_track_id.lock().unwrap();
                        *retried_id = None;
                    }

                    // Stop player and show notification
                    self.audio.stop();
                    crate::bridge::bridge::show_notification(
                        &format!("Error de reproducción persistente en: {}", track.title),
                        "error"
                    );
                }
            }
        }

        // Push state to UI via bridge
        self.sync_ui();
    }

    pub fn apply_equalizer(&self, enabled: bool, preamp: f64, bands: &[f64], preset_name: &str) {
        self.audio.apply_equalizer(enabled, preamp, bands, preset_name);
    }

    pub fn set_sleep_timer(&self, minutes: i32) {
        let mut timer = self.sleep_timer_end.lock().unwrap();
        if minutes <= 0 {
            *timer = None;
            log::info!("Sleep timer disabled");
        } else {
            *timer = Some(Instant::now() + Duration::from_secs((minutes * 60) as u64));
            log::info!("Sleep timer set for {minutes} minutes");
        }
    }

    fn sync_ui(&self) {
        let state = self.audio.state();
        let pos = self.audio.position_ms();
        let dur = self.audio.duration_ms();
        let is_playing = state.is_playing();

        crate::bridge::bridge::update_player_state(
            state as i32, pos as i32, dur as i32,
        );
        crate::bridge::bridge::set_playing(is_playing);

        // Sync queue to C++
        {
            if let Ok(q) = self.queue.lock() {
                let mut queue_list = Vec::new();
                for track in q.all_tracks() {
                    let mut thumb = track.thumbnail.clone();
                    if thumb.is_empty() {
                        thumb = crate::bridge::bridge::get_or_create_thumbnail(&track.title, 0);
                    }
                    queue_list.push(crate::bridge::bridge::Track {
                        id: track.id.clone(),
                        title: track.title.clone(),
                        artist: track.artist.clone(),
                        album: track.album.clone(),
                        duration_ms: track.duration_ms,
                        thumbnail: thumb,
                    });
                }
                crate::bridge::bridge::set_playback_queue(queue_list, q.current_index() as i32);
            }
        }

        if let Some(track) = self.current_track() {
            let mut thumb = track.thumbnail.clone();
            if thumb.is_empty() {
                thumb = crate::bridge::bridge::get_or_create_thumbnail(&track.title, 0);
            }
            crate::bridge::bridge::set_mini_player(
                &track.title, &track.artist, &thumb,
            );
            // Record in recently played (only when playing new track)
            if is_playing && pos < 500 {
                let _ = crate::db::repo::RecentlyPlayedRepo::record(
                    &track.id, &track.title, &track.artist, &track.album,
                    track.duration_ms, &thumb,
                );
            }

            // Sync integrations
            let mut last_id = self.last_track_id.lock().unwrap();
            let mut last_poll = self.last_playback_poll.lock().unwrap();
            let now = Instant::now();
            let elapsed_ms = now.duration_since(*last_poll).as_millis() as i64;
            *last_poll = now;

            let track_changed = *last_id != track.id;
            let play_state_changed = is_playing != self.last_is_playing.load(std::sync::atomic::Ordering::Relaxed);

            if track_changed {
                *last_id = track.id.clone();
                self.last_is_playing.store(is_playing, std::sync::atomic::Ordering::Relaxed);
                self.lastfm_scrobbled.store(false, std::sync::atomic::Ordering::Relaxed);
                self.accumulated_playback_ms.store(0, std::sync::atomic::Ordering::Relaxed);

                // Trigger Discord RPC
                crate::services::discord::update_presence(&track.title, &track.artist, &track.album, is_playing);
                // Trigger Last.fm Now Playing
                if is_playing {
                    crate::services::lastfm::update_now_playing(&track.artist, &track.title, &track.album);
                }

                // Trigger Dominant Colors extraction in background
                let thumb_path = thumb.clone();
                tokio::spawn(async move {
                    log::info!("Extracting dominant colors for: {}", thumb_path);
                    let colors = crate::utils::color::extract_dominant_colors(&thumb_path);
                    crate::bridge::bridge::set_dominant_colors(colors);
                });

                // Trigger Lyrics fetch
                let title = track.title.clone();
                let artist = track.artist.clone();
                tokio::spawn(async move {
                    log::info!("Fetching lyrics in background for: {} by {}", title, artist);
                    let lyrics_service = crate::services::lyrics::LyricsService::new();
                    match lyrics_service.fetch_lyrics(&title, &artist).await {
                        Ok(Some(resp)) => {
                            let plain = resp.plain_lyrics.unwrap_or_default();
                            let synced = resp.synced_lyrics.unwrap_or_default();
                            crate::bridge::bridge::set_track_lyrics(&plain, &synced);
                        }
                        Ok(None) => {
                            crate::bridge::bridge::set_track_lyrics("", "");
                        }
                        Err(e) => {
                            log::warn!("Failed to fetch lyrics: {e}");
                            crate::bridge::bridge::set_track_lyrics("", "");
                        }
                    }
                });
            } else if play_state_changed {
                self.last_is_playing.store(is_playing, std::sync::atomic::Ordering::Relaxed);

                // Trigger Discord RPC
                crate::services::discord::update_presence(&track.title, &track.artist, &track.album, is_playing);
                // Trigger Last.fm Now Playing if resuming
                if is_playing {
                    crate::services::lastfm::update_now_playing(&track.artist, &track.title, &track.album);
                }
            }

            // Accumulate playback time and check for scrobbling
            if is_playing {
                let accumulated = self.accumulated_playback_ms.fetch_add(elapsed_ms, std::sync::atomic::Ordering::Relaxed) + elapsed_ms;
                let duration_ms = if track.duration_ms > 0 {
                    track.duration_ms
                } else {
                    dur
                };

                if duration_ms >= 30_000 {
                    let scrobble_threshold = std::cmp::min(duration_ms / 2, 240_000);
                    if accumulated >= scrobble_threshold && !self.lastfm_scrobbled.load(std::sync::atomic::Ordering::Relaxed) {
                        self.lastfm_scrobbled.store(true, std::sync::atomic::Ordering::Relaxed);
                        crate::services::lastfm::scrobble(&track.artist, &track.title, &track.album);
                    }
                }
            }
        } else {
            let mut last_id = self.last_track_id.lock().unwrap();
            if !last_id.is_empty() {
                last_id.clear();
                self.last_is_playing.store(false, std::sync::atomic::Ordering::Relaxed);
                self.lastfm_scrobbled.store(false, std::sync::atomic::Ordering::Relaxed);
                self.accumulated_playback_ms.store(0, std::sync::atomic::Ordering::Relaxed);

                crate::services::discord::disconnect();
            }
        }
    }
}

impl Default for PlayerService {
    fn default() -> Self {
        Self::new()
    }
}
