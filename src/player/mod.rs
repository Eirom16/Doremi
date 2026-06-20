pub mod audio;
pub mod queue;
pub mod resolver;
pub mod state;
pub mod stream_proxy;
pub mod vlc_check;

use once_cell::sync::Lazy;
use std::sync::atomic::AtomicU64;
use std::sync::atomic::{AtomicBool, Ordering};
use std::sync::{Arc, Mutex};
use std::time::{Duration, Instant};

use audio::AudioEngine;
use queue::{PlaybackQueue, TrackInfo};
use state::PlayState;

static QUEUE_PERSIST_GENERATION: AtomicU64 = AtomicU64::new(0);
static QUEUE_PERSIST_LOCK: Lazy<Mutex<()>> = Lazy::new(|| Mutex::new(()));

static PREFETCH_GENERATION: AtomicU64 = AtomicU64::new(0);

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
    auto_queue_loading: Arc<AtomicBool>,
    preload_next: bool,
    prefetch_task: Arc<Mutex<Option<tokio::task::JoinHandle<()>>>>,
    crossfade_active: Arc<std::sync::atomic::AtomicBool>,
    crossfade_start_time: Arc<Mutex<Option<Instant>>>,
    playback_generation: Arc<AtomicU64>,
}

impl PlayerService {
    pub fn new() -> Self {
        Self::new_with_preload(true)
    }

    pub fn new_with_preload(preload_next: bool) -> Self {
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
            auto_queue_loading: Arc::new(AtomicBool::new(false)),
            preload_next,
            prefetch_task: Arc::new(Mutex::new(None)),
            crossfade_active: Arc::new(std::sync::atomic::AtomicBool::new(false)),
            crossfade_start_time: Arc::new(Mutex::new(None)),
            playback_generation: Arc::new(AtomicU64::new(0)),
        }
    }

    pub fn is_available(&self) -> bool {
        self.audio.is_available()
    }

    // ── Queue management ──

    pub fn enqueue(&self, track: TrackInfo) {
        self.queue.lock().unwrap().enqueue(track);
        self.sync_queue_ui();
        self.schedule_prefetch();
        self.persist_queue_session();
    }

    pub fn enqueue_next(&self, track: TrackInfo) {
        self.queue.lock().unwrap().enqueue_next(track);
        self.sync_queue_ui();
        self.schedule_prefetch();
        self.persist_queue_session();
    }

    pub fn remove_queue_item(&self, index: usize) -> bool {
        let (removed, was_current, replacement) = {
            let mut queue = self.queue.lock().unwrap();
            let was_current = !queue.is_empty() && queue.current_index() == index;
            let removed = queue.remove(index).is_some();
            let replacement = if removed && was_current {
                queue.current().cloned()
            } else {
                None
            };
            (removed, was_current, replacement)
        };

        if !removed {
            return false;
        }
        if was_current {
            if let Some(track) = replacement {
                self.play_track_info(track);
            } else {
                self.audio.stop();
            }
        }
        self.sync_queue_ui();
        self.schedule_prefetch();
        self.persist_queue_session();
        true
    }

    pub fn move_queue_item(&self, from: usize, to: usize) -> bool {
        let moved = self.queue.lock().unwrap().move_item(from, to);
        if moved {
            self.sync_queue_ui();
            self.schedule_prefetch();
            self.persist_queue_session();
        }
        moved
    }

    pub fn clear_queue(&self) {
        self.playback_generation.fetch_add(1, Ordering::AcqRel);
        self.queue.lock().unwrap().clear();
        self.audio.stop();
        self.sync_queue_ui();
        self.cancel_prefetch();
        self.persist_queue_session();
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
            queue.enqueue(track);
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
            queue.enqueue(track_info);
        }
        self.play_index(0);
    }

    pub fn play_all_tracks(&self, tracks: Vec<TrackInfo>, shuffle: bool) {
        log::info!("Play all {} tracks (shuffle={shuffle})", tracks.len());
        {
            let mut queue = self.queue.lock().unwrap();
            queue.clear();
            for t in tracks {
                queue.enqueue(t);
            }
            if shuffle {
                queue.toggle_shuffle();
            }
        }
        self.sync_queue_ui();
        self.play_index(0);
    }

    pub fn stop(&self) {
        self.playback_generation.fetch_add(1, Ordering::AcqRel);
        if self
            .crossfade_active
            .swap(false, std::sync::atomic::Ordering::Relaxed)
        {
            *self.crossfade_start_time.lock().unwrap() = None;
            self.audio.complete_crossfade();
        }
        self.audio.stop();
    }

    fn should_normalize(&self) -> bool {
        let dirs = crate::config::paths::AppDirs::global();
        let settings = crate::config::settings::AppSettings::load(&dirs.settings_path());
        settings.player.normalize_audio
    }

    pub fn play_url(&self, url: &str) {
        self.playback_generation.fetch_add(1, Ordering::AcqRel);
        let normalize = self.should_normalize();
        self.audio.play_url(url, normalize);
        drop(self.last_poll.lock().unwrap());
    }

    pub fn play_index(&self, index: usize) {
        let track = {
            let mut queue = self.queue.lock().unwrap();
            queue.jump_to(index).cloned()
        };
        if let Some(t) = track {
            self.play_track_info(t);
            self.schedule_prefetch();
            self.persist_queue_session();
        }
    }

    fn play_track_info(&self, t: TrackInfo) {
        let generation = self.playback_generation.fetch_add(1, Ordering::AcqRel) + 1;

        if self
            .crossfade_active
            .swap(false, std::sync::atomic::Ordering::Relaxed)
        {
            *self.crossfade_start_time.lock().unwrap() = None;
            self.audio.complete_crossfade();
        }

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
            self.audio.play_url(&path, false);
            return;
        }

        let mut needs_resolution = t.stream_url.is_empty();
        if !needs_resolution && t.stream_url.starts_with("http") {
            if crate::player::resolver::StreamResolver::is_url_expired(&t.stream_url) {
                log::info!("Stream URL for {} is expired, forcing re-resolution", t.id);
                needs_resolution = true;
            } else if crate::player::resolver::StreamResolver::is_youtube_stream_url(&t.stream_url)
            {
                log::info!(
                    "Stream URL for {} needs a playability probe, forcing resolver path",
                    t.id
                );
                needs_resolution = true;
            }
        }

        let normalize = self.should_normalize();

        if needs_resolution {
            if t.id.is_empty() {
                log::warn!("Cannot play track with empty ID and no stream URL");
                return;
            }
            let queue = self.queue.clone();
            let audio = self.audio.clone();
            let id = t.id.clone();
            let playback_generation = self.playback_generation.clone();

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
                            if playback_generation.load(Ordering::Acquire) != generation {
                                log::info!("Resolved URL for {id}, but a newer playback request exists. Ignoring.");
                                return;
                            }
                            log::info!("Playing resolved URL for {id}");
                            audio.play_url(&resolved_url, normalize);
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
            if self.playback_generation.load(Ordering::Acquire) != generation {
                log::info!("Skipping stale playback request for {}", t.id);
                return;
            }
            self.audio.play_url(&t.stream_url, normalize);
        }
    }

    pub fn toggle_play_pause(&self) {
        self.audio.toggle_play_pause();
    }

    pub fn next(&self) {
        let (next_track, seed) = {
            let mut queue = self.queue.lock().unwrap();
            let seed = queue.current().cloned();
            (queue.next().cloned(), seed)
        };
        if let Some(t) = next_track {
            self.play_track_info(t);
            self.schedule_prefetch();
            self.persist_queue_session();
        } else if let Some(seed) = seed {
            self.audio.stop();
            self.audio.set_state(PlayState::Loading);
            self.request_auto_queue(seed);
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
            self.schedule_prefetch();
            self.persist_queue_session();
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
        self.schedule_prefetch();
        self.persist_queue_session();
    }

    pub fn cycle_repeat(&self) {
        self.queue.lock().unwrap().cycle_repeat();
        self.persist_queue_session();
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

        let dirs = crate::config::paths::AppDirs::global();
        let mut settings = crate::config::settings::AppSettings::load(&dirs.settings_path());

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
            settings.player.sleep_timer_minutes = 0;
            let _ = settings.save(&dirs.settings_path());
            crate::bridge::bridge::show_notification("Temporizador de apagado finalizado.", "info");
        }

        self.audio.poll_position();

        let mut crossfade_completed = false;
        if self
            .crossfade_active
            .load(std::sync::atomic::Ordering::Relaxed)
        {
            let mut elapsed_secs = 0.0;
            let mut start_time_present = false;
            {
                if let Some(start) = *self.crossfade_start_time.lock().unwrap() {
                    elapsed_secs = start.elapsed().as_secs_f64();
                    start_time_present = true;
                }
            }

            if start_time_present {
                let crossfade_duration = settings.player.crossfade_duration_sec.max(1) as f64;
                let progress = (elapsed_secs / crossfade_duration).min(1.0);
                self.audio.set_fade_volumes(1.0 - progress, progress);

                if progress >= 1.0 || self.audio.has_ended() {
                    log::info!("Crossfade completed or active track ended. Swapping players.");
                    self.audio.complete_crossfade();
                    self.crossfade_active
                        .store(false, std::sync::atomic::Ordering::Relaxed);
                    *self.crossfade_start_time.lock().unwrap() = None;

                    // Trigger integrations and UI updates for the new track
                    if let Some(track) = self.current_track() {
                        let mut last_id = self.last_track_id.lock().unwrap();
                        *last_id = track.id.clone();
                        crate::services::discord::update_presence(
                            &track.title,
                            &track.artist,
                            &track.album,
                            true,
                        );
                        crate::services::lastfm::update_now_playing(
                            &track.artist,
                            &track.title,
                            &track.album,
                        );
                        let title = track.title.clone();
                        let artist = track.artist.clone();
                        tokio::spawn(async move {
                            let lyrics_service = crate::services::lyrics::LyricsService::new();
                            let _ = lyrics_service.fetch_lyrics(&title, &artist).await;
                        });
                    }

                    self.schedule_prefetch();
                    crossfade_completed = true;
                }
            }
        }

        if !crossfade_completed {
            if self.audio.has_ended()
                && self
                    .last_is_playing
                    .load(std::sync::atomic::Ordering::Relaxed)
            {
                self.next();
            } else if settings.player.crossfade_enabled
                && self.audio.state().is_playing()
                && !self
                    .crossfade_active
                    .load(std::sync::atomic::Ordering::Relaxed)
            {
                let remaining_ms = self.audio.duration_ms() - self.audio.position_ms();
                let crossfade_ms = (settings.player.crossfade_duration_sec * 1000) as i64;
                if remaining_ms > 0 && remaining_ms <= crossfade_ms {
                    let next_track = self
                        .queue
                        .lock()
                        .ok()
                        .and_then(|q| q.next_candidate().cloned());
                    if let Some(track) = next_track {
                        log::info!(
                            "Initiating crossfade transition from current track to {}",
                            track.title
                        );
                        let normalize = settings.player.normalize_audio;
                        let queue = self.queue.clone();
                        let audio = self.audio.clone();
                        let crossfade_active = self.crossfade_active.clone();
                        let crossfade_start_time = self.crossfade_start_time.clone();
                        let id = track.id.clone();

                        tokio::spawn(async move {
                            let resolved_url = if track.stream_url.is_empty() {
                                log::info!("Resolving stream URL for crossfade track {id}...");
                                crate::player::resolver::StreamResolver::resolve(&id)
                                    .await
                                    .ok()
                            } else {
                                Some(track.stream_url.clone())
                            };

                            if let Some(url) = resolved_url {
                                let still_next = {
                                    if let Ok(q) = queue.lock() {
                                        q.next_candidate().map(|t| t.id == id).unwrap_or(false)
                                    } else {
                                        false
                                    }
                                };

                                if still_next {
                                    log::info!("Playing crossfade URL for next track {id}");
                                    if let Ok(mut q) = queue.lock() {
                                        if let Some(target) =
                                            q.all_tracks().iter().position(|t| t.id == id)
                                        {
                                            if let Some(t) = q.track_mut(target) {
                                                t.stream_url = url.clone();
                                            }
                                        }
                                        let _ = q.next();
                                    }

                                    audio.play_crossfade(&url, normalize);
                                    crossfade_active
                                        .store(true, std::sync::atomic::Ordering::Relaxed);
                                    *crossfade_start_time.lock().unwrap() =
                                        Some(std::time::Instant::now());

                                    sync_queue_to_ui(&queue);
                                }
                            }
                        });
                    }
                }
            }
        }

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
                        "error",
                    );
                }
            }
        }

        // Push state to UI via bridge
        self.sync_ui();
    }

    pub fn apply_equalizer(&self, enabled: bool, preamp: f64, bands: &[f64], preset_name: &str) {
        self.audio
            .apply_equalizer(enabled, preamp, bands, preset_name);
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

        crate::bridge::bridge::update_player_state(state as i32, pos as i32, dur as i32);
        crate::bridge::bridge::set_playing(is_playing);

        self.sync_queue_ui();

        if let Some(track) = self.current_track() {
            let mut thumb = track.thumbnail.clone();
            if thumb.is_empty() {
                thumb = crate::bridge::bridge::get_or_create_thumbnail(&track.title, 0);
            }
            crate::bridge::bridge::set_mini_player(&track.title, &track.artist, &thumb);
            crate::bridge::bridge::set_current_track(crate::bridge::bridge::Track {
                id: track.id.clone(),
                title: track.title.clone(),
                artist: track.artist.clone(),
                album: track.album.clone(),
                duration_ms: track.duration_ms,
                thumbnail: thumb.clone(),
            });
            // Record in play history (only when playing new track at start)
            if is_playing && pos < 500 {
                let _ = crate::db::repo::PlayHistoryRepo::record(
                    &track.id,
                    &track.title,
                    &track.artist,
                    &track.album,
                    track.duration_ms,
                    &thumb,
                    dur as i64,
                    false,
                );
            }

            // Sync integrations
            let mut last_id = self.last_track_id.lock().unwrap();
            let mut last_poll = self.last_playback_poll.lock().unwrap();
            let now = Instant::now();
            let elapsed_ms = now.duration_since(*last_poll).as_millis() as i64;
            *last_poll = now;

            let track_changed = *last_id != track.id;
            let play_state_changed = is_playing
                != self
                    .last_is_playing
                    .load(std::sync::atomic::Ordering::Relaxed);

            if track_changed {
                *last_id = track.id.clone();
                self.last_is_playing
                    .store(is_playing, std::sync::atomic::Ordering::Relaxed);
                self.lastfm_scrobbled
                    .store(false, std::sync::atomic::Ordering::Relaxed);
                self.accumulated_playback_ms
                    .store(0, std::sync::atomic::Ordering::Relaxed);

                // Trigger Discord RPC
                crate::services::discord::update_presence(
                    &track.title,
                    &track.artist,
                    &track.album,
                    is_playing,
                );
                // Trigger Last.fm Now Playing
                if is_playing {
                    crate::services::lastfm::update_now_playing(
                        &track.artist,
                        &track.title,
                        &track.album,
                    );
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
                self.last_is_playing
                    .store(is_playing, std::sync::atomic::Ordering::Relaxed);

                // Trigger Discord RPC
                crate::services::discord::update_presence(
                    &track.title,
                    &track.artist,
                    &track.album,
                    is_playing,
                );
                // Trigger Last.fm Now Playing if resuming
                if is_playing {
                    crate::services::lastfm::update_now_playing(
                        &track.artist,
                        &track.title,
                        &track.album,
                    );
                }
            }

            // Accumulate playback time and check for scrobbling
            if is_playing {
                let accumulated = self
                    .accumulated_playback_ms
                    .fetch_add(elapsed_ms, std::sync::atomic::Ordering::Relaxed)
                    + elapsed_ms;
                let duration_ms = if track.duration_ms > 0 {
                    track.duration_ms
                } else {
                    dur
                };

                if duration_ms >= 30_000 {
                    let scrobble_threshold = std::cmp::min(duration_ms / 2, 240_000);
                    if accumulated >= scrobble_threshold
                        && !self
                            .lastfm_scrobbled
                            .load(std::sync::atomic::Ordering::Relaxed)
                    {
                        self.lastfm_scrobbled
                            .store(true, std::sync::atomic::Ordering::Relaxed);
                        crate::services::lastfm::scrobble(
                            &track.artist,
                            &track.title,
                            &track.album,
                        );
                    }
                }
            }
        } else {
            let mut last_id = self.last_track_id.lock().unwrap();
            if !last_id.is_empty() {
                last_id.clear();
                self.last_is_playing
                    .store(false, std::sync::atomic::Ordering::Relaxed);
                self.lastfm_scrobbled
                    .store(false, std::sync::atomic::Ordering::Relaxed);
                self.accumulated_playback_ms
                    .store(0, std::sync::atomic::Ordering::Relaxed);

                crate::services::discord::disconnect();
            }
            crate::bridge::bridge::set_mini_player("", "", "");
            crate::bridge::bridge::set_current_track(crate::bridge::bridge::Track {
                id: String::new(),
                title: String::new(),
                artist: String::new(),
                album: String::new(),
                duration_ms: 0,
                thumbnail: String::new(),
            });
        }
    }

    fn sync_queue_ui(&self) {
        sync_queue_to_ui(&self.queue);
    }

    fn request_auto_queue(&self, seed: TrackInfo) {
        if seed.id.is_empty() || self.auto_queue_loading.swap(true, Ordering::AcqRel) {
            return;
        }

        let generation = self.playback_generation.fetch_add(1, Ordering::AcqRel) + 1;
        let queue = self.queue.clone();
        let audio = self.audio.clone();
        let loading = self.auto_queue_loading.clone();
        let prefetch_task = self.prefetch_task.clone();
        let preload_next = self.preload_next;
        let playback_generation = self.playback_generation.clone();
        tokio::spawn(async move {
            let seed_id = seed.id.clone();
            let result = crate::api::innertube::related_tracks(&seed_id).await;

            let related = match result {
                Ok(tracks) => tracks,
                Err(error) => {
                    log::warn!("Could not build auto queue for {}: {error}", seed.id);
                    loading.store(false, Ordering::Release);
                    return;
                }
            };

            let mut related_bridge = Vec::new();
            let mut candidates = Vec::new();
            for track in related {
                let artist = track.artists.into_iter().next().unwrap_or_default();
                let album = track.album.unwrap_or_default();
                let thumbnail = if track.thumbnail.is_empty() {
                    crate::bridge::bridge::get_or_create_thumbnail(&track.title, 0)
                } else {
                    track.thumbnail.clone()
                };
                related_bridge.push(crate::bridge::bridge::Track {
                    id: track.id.clone(),
                    title: track.title.clone(),
                    artist: artist.clone(),
                    album: album.clone(),
                    duration_ms: track.duration_ms,
                    thumbnail,
                });
                candidates.push(TrackInfo {
                    id: track.id,
                    title: track.title,
                    artist,
                    album,
                    duration_ms: track.duration_ms,
                    thumbnail: track.thumbnail,
                    stream_url: track.stream_url.unwrap_or_default(),
                });
            }
            crate::bridge::bridge::set_related_tracks(related_bridge);

            let next_track = {
                let mut queue_guard = queue.lock().unwrap();
                let added = queue_guard.append_unique(candidates, 25);
                log::info!("Auto queue added {added} related tracks for {}", seed.id);
                if added > 0 {
                    queue_guard.next().cloned()
                } else {
                    None
                }
            };
            sync_queue_to_ui(&queue);
            persist_queue_snapshot(queue.clone());

            if playback_generation.load(Ordering::Acquire) != generation {
                log::info!(
                    "Auto queue for {} is stale. Not starting playback.",
                    seed.id
                );
                loading.store(false, Ordering::Release);
                return;
            }

            let normalize = {
                let dirs = crate::config::paths::AppDirs::global();
                let settings = crate::config::settings::AppSettings::load(&dirs.settings_path());
                settings.player.normalize_audio
            };

            if let Some(mut track) = next_track {
                if track.stream_url.is_empty() {
                    match crate::player::resolver::StreamResolver::resolve(&track.id).await {
                        Ok(url) => {
                            track.stream_url = url.clone();
                            if let Ok(mut queue_guard) = queue.lock() {
                                if let Some(current) = queue_guard.current_mut() {
                                    if current.id == track.id {
                                        current.stream_url = url;
                                    }
                                }
                            }
                        }
                        Err(error) => {
                            log::warn!("Could not resolve auto queue track {}: {error}", track.id);
                            loading.store(false, Ordering::Release);
                            return;
                        }
                    }
                }
                if playback_generation.load(Ordering::Acquire) != generation {
                    log::info!(
                        "Resolved auto queue track {} is stale. Not starting playback.",
                        track.id
                    );
                    loading.store(false, Ordering::Release);
                    return;
                }
                audio.play_url(&track.stream_url, normalize);
                if preload_next {
                    let gen = PREFETCH_GENERATION.fetch_add(1, Ordering::AcqRel) + 1;
                    schedule_prefetch_task(queue.clone(), prefetch_task, gen);
                }
            }
            loading.store(false, Ordering::Release);
        });
    }

    fn schedule_prefetch(&self) {
        if self.preload_next {
            let gen = PREFETCH_GENERATION.fetch_add(1, Ordering::AcqRel) + 1;
            schedule_prefetch_task(self.queue.clone(), self.prefetch_task.clone(), gen);
        } else {
            self.cancel_prefetch();
        }
    }

    fn cancel_prefetch(&self) {
        if let Ok(mut task) = self.prefetch_task.lock() {
            if let Some(handle) = task.take() {
                handle.abort();
            }
        }
    }

    pub fn restore_queue_session(&self) -> bool {
        let path = queue_session_path();
        let Ok(content) = std::fs::read_to_string(&path) else {
            return false;
        };
        let Ok(snapshot) = serde_json::from_str(&content) else {
            log::warn!("Ignoring invalid queue session at {}", path.display());
            return false;
        };
        self.queue
            .lock()
            .map(|mut queue| queue.restore(snapshot))
            .unwrap_or(false)
    }

    pub fn refresh_queue_ui(&self) {
        self.sync_queue_ui();
        self.schedule_prefetch();
    }

    fn persist_queue_session(&self) {
        persist_queue_snapshot(self.queue.clone());
    }
}

fn queue_session_path() -> std::path::PathBuf {
    crate::config::paths::AppDirs::global()
        .data_dir()
        .join("queue-session.json")
}

fn persist_queue_snapshot(queue: Arc<Mutex<PlaybackQueue>>) {
    let snapshot = match queue.lock() {
        Ok(queue) => queue.snapshot(),
        Err(_) => return,
    };
    let path = queue_session_path();
    let generation = QUEUE_PERSIST_GENERATION.fetch_add(1, Ordering::AcqRel) + 1;
    tokio::task::spawn_blocking(move || {
        let Ok(_guard) = QUEUE_PERSIST_LOCK.lock() else {
            return;
        };
        if QUEUE_PERSIST_GENERATION.load(Ordering::Acquire) != generation {
            return;
        }
        let Ok(data) = serde_json::to_vec_pretty(&snapshot) else {
            return;
        };
        if let Err(error) = crate::config::settings::write_private_file(&path, &data) {
            log::warn!("Could not persist queue session: {error}");
        }
    });
}

fn schedule_prefetch_task(
    queue: Arc<Mutex<PlaybackQueue>>,
    task_slot: Arc<Mutex<Option<tokio::task::JoinHandle<()>>>>,
    generation: u64,
) {
    let candidate = queue
        .lock()
        .ok()
        .and_then(|queue| queue.next_candidate().cloned());
    let Ok(mut slot) = task_slot.lock() else {
        return;
    };
    if let Some(handle) = slot.take() {
        handle.abort();
    }
    let Some(candidate) = candidate else {
        return;
    };

    let handle = tokio::spawn(async move {
        // Debounce: short delay before starting work
        tokio::time::sleep(Duration::from_millis(50)).await;
        if PREFETCH_GENERATION.load(Ordering::Acquire) != generation {
            return; // stale, a newer prefetch has been scheduled
        }

        crate::bridge::bridge::set_prefetch_status(&candidate.id, "loading");

        let stream_future = async {
            if candidate.stream_url.is_empty() {
                tokio::time::timeout(
                    Duration::from_secs(30),
                    crate::player::resolver::StreamResolver::resolve(&candidate.id),
                )
                .await
                .ok()
                .and_then(|r| r.ok())
            } else {
                Some(candidate.stream_url.clone())
            }
        };
        let lyrics_service = crate::services::lyrics::LyricsService::new();
        let lyrics_future = lyrics_service.fetch_lyrics(&candidate.title, &candidate.artist);
        let artwork_future = prefetch_artwork(&candidate);
        let (stream_url, _, artwork_path) =
            tokio::join!(stream_future, lyrics_future, artwork_future);

        if PREFETCH_GENERATION.load(Ordering::Acquire) != generation {
            return; // stale after long-running work
        }

        if let Ok(mut queue_guard) = queue.lock() {
            let still_next = queue_guard
                .next_candidate()
                .map(|track| track.id == candidate.id)
                .unwrap_or(false);
            if !still_next {
                crate::bridge::bridge::set_prefetch_status(&candidate.id, "stale");
                return;
            }
            if let Some(target) = queue_guard
                .all_tracks()
                .iter()
                .position(|track| track.id == candidate.id)
            {
                if let Some(track) = queue_guard.track_mut(target) {
                    if let Some(url) = stream_url {
                        track.stream_url = url;
                    }
                    if let Some(path) = artwork_path {
                        track.thumbnail = path;
                    }
                }
            }
        }
        sync_queue_to_ui(&queue);
        crate::bridge::bridge::set_prefetch_status(&candidate.id, "ready");
    });
    *slot = Some(handle);
}

async fn prefetch_artwork(track: &TrackInfo) -> Option<String> {
    if track.thumbnail.is_empty() || !track.thumbnail.starts_with("http") {
        return None;
    }
    let safe_id: String = track
        .id
        .chars()
        .filter(|character| character.is_ascii_alphanumeric() || matches!(character, '-' | '_'))
        .take(80)
        .collect();
    if safe_id.is_empty() {
        return None;
    }
    let path = crate::config::paths::AppDirs::global()
        .artwork_cache_dir()
        .join(format!("{safe_id}.img"));
    if path.exists() {
        return Some(path.to_string_lossy().into_owned());
    }
    let response = reqwest::Client::builder()
        .timeout(Duration::from_secs(8))
        .build()
        .ok()?
        .get(&track.thumbnail)
        .send()
        .await
        .ok()?;
    if !response.status().is_success() {
        return None;
    }
    let bytes = response.bytes().await.ok()?;
    tokio::fs::write(&path, bytes).await.ok()?;
    Some(path.to_string_lossy().into_owned())
}

fn sync_queue_to_ui(queue: &Arc<Mutex<PlaybackQueue>>) {
    let (tracks, current_index) = {
        let queue = queue.lock().unwrap();
        (queue.all_tracks().to_vec(), queue.current_index() as i32)
    };
    let queue_list = tracks
        .into_iter()
        .map(|track| {
            let thumbnail = if track.thumbnail.is_empty() {
                crate::bridge::bridge::get_or_create_thumbnail(&track.title, 0)
            } else {
                track.thumbnail
            };
            crate::bridge::bridge::Track {
                id: track.id,
                title: track.title,
                artist: track.artist,
                album: track.album,
                duration_ms: track.duration_ms,
                thumbnail,
            }
        })
        .collect();
    crate::bridge::bridge::set_playback_queue(queue_list, current_index);
}

impl Default for PlayerService {
    fn default() -> Self {
        Self::new()
    }
}
