use once_cell::sync::Lazy;
use std::collections::HashMap;
use std::sync::Arc;
use std::time::Duration;
use tokio;
use zbus::connection;
use zbus::interface;
use zbus::zvariant::{OwnedObjectPath, OwnedValue, Value};

use crate::player::PlayerService;

static MPRIS_TX: Lazy<std::sync::Mutex<Option<tokio::sync::oneshot::Sender<()>>>> =
    Lazy::new(|| std::sync::Mutex::new(None));

pub fn stop_mpris() {
    let mut lock = MPRIS_TX.lock().unwrap();
    if let Some(tx) = lock.take() {
        let _ = tx.send(());
        log::info!("MPRIS service stopped");
    }
}

pub fn start_mpris(player: Arc<PlayerService>) {
    stop_mpris();

    let (tx, rx) = tokio::sync::oneshot::channel();
    *MPRIS_TX.lock().unwrap() = Some(tx);

    tokio::spawn(async move {
        let service = MprisService::new(player);
        if let Err(e) = service.start_with_shutdown(rx).await {
            log::error!("MPRIS server failed: {e}");
        }
    });
}

pub fn spawn_mpris(player: Arc<PlayerService>) {
    let dirs = crate::config::paths::AppDirs::global();
    let settings = crate::config::settings::AppSettings::load(&dirs.settings_path());
    if settings.integrations.mpris_enabled {
        start_mpris(player);
    }
}

pub struct MprisService {
    player: Arc<PlayerService>,
}

impl MprisService {
    pub fn new(player: Arc<PlayerService>) -> Self {
        Self { player }
    }

    pub async fn start_with_shutdown(
        self,
        rx: tokio::sync::oneshot::Receiver<()>,
    ) -> zbus::Result<()> {
        let conn = connection::Builder::session()?
            .name("org.mpris.MediaPlayer2.doremi")?
            .build()
            .await?;

        let root = MprisRoot::new();
        let player_iface = MprisPlayer::new(self.player.clone());

        conn.object_server()
            .at("/org/mpris/MediaPlayer2", root)
            .await?;
        conn.object_server()
            .at("/org/mpris/MediaPlayer2", player_iface)
            .await?;

        log::info!("MPRIS server running on org.mpris.MediaPlayer2.doremi");

        let iface_ref = conn
            .object_server()
            .interface::<_, MprisPlayer>("/org/mpris/MediaPlayer2")
            .await?;

        let player = self.player.clone();
        let (poll_tx, mut poll_rx) = tokio::sync::oneshot::channel::<()>();

        let poll_task = tokio::spawn(async move {
            let mut last_status = String::new();
            let mut last_track_id = String::new();
            let mut last_volume = -1.0;
            let mut last_position = -1;

            loop {
                tokio::select! {
                    _ = tokio::time::sleep(Duration::from_millis(500)) => {
                        let emitter = iface_ref.signal_context();

                        // 1. PlaybackStatus
                        let is_playing = player.is_playing();
                        let status = if is_playing { "Playing" } else { "Paused" };
                        if status != last_status {
                            last_status = status.to_string();
                            let player_obj = iface_ref.get().await;
                            let _ = player_obj.playback_status_changed(emitter).await;
                        }

                        // 2. Metadata
                        let current_track_id = player.current_track()
                            .map(|t| t.id.clone())
                            .unwrap_or_default();
                        if current_track_id != last_track_id {
                            last_track_id = current_track_id;
                            let player_obj = iface_ref.get().await;
                            let _ = player_obj.metadata_changed(emitter).await;
                        }

                        // 3. Volume
                        let volume = player.volume() as f64 / 100.0;
                        if (volume - last_volume).abs() > 0.01 {
                            last_volume = volume;
                            let player_obj = iface_ref.get().await;
                            let _ = player_obj.volume_changed(emitter).await;
                        }

                        // 4. Position & Seeked Signal
                        let position = player.position_ms() * 1000; // microseconds
                        if last_position != -1 {
                            let expected_delta = if is_playing { 500 * 1000 } else { 0 };
                            let actual_delta = (position - last_position).abs();
                            if (actual_delta - expected_delta).abs() > 1_500_000 {
                                let _ = MprisPlayer::seeked(emitter, position).await;
                            }
                        }
                        last_position = position;
                    }
                    _ = &mut poll_rx => {
                        break;
                    }
                }
            }
        });

        // Keep the connection alive until shutdown signal
        let _ = rx.await;

        let _ = poll_tx.send(());
        let _ = poll_task.await;

        Ok(())
    }
}

// ── org.mpris.MediaPlayer2 (root interface) ──

pub struct MprisRoot {
    identity: String,
    desktop_entry: String,
}

impl MprisRoot {
    pub fn new() -> Self {
        Self {
            identity: "Doremi".into(),
            desktop_entry: "doremi".into(),
        }
    }
}

#[interface(name = "org.mpris.MediaPlayer2")]
impl MprisRoot {
    #[zbus(property)]
    fn can_quit(&self) -> bool {
        true
    }

    #[zbus(property)]
    fn can_raise(&self) -> bool {
        true
    }

    #[zbus(property)]
    fn has_track_list(&self) -> bool {
        false
    }

    #[zbus(property)]
    fn identity(&self) -> &str {
        &self.identity
    }

    #[zbus(property)]
    fn desktop_entry(&self) -> &str {
        &self.desktop_entry
    }

    #[zbus(property)]
    fn supported_uri_schemes(&self) -> Vec<&str> {
        vec!["file", "http", "https"]
    }

    #[zbus(property)]
    fn supported_mime_types(&self) -> Vec<&str> {
        vec![
            "audio/mpeg",
            "audio/ogg",
            "audio/flac",
            "audio/x-wav",
            "audio/x-m4a",
        ]
    }

    fn raise(&mut self) {
        log::info!("MPRIS: raise");
        crate::bridge::bridge::show_main_window();
    }

    fn quit(&mut self) {
        log::info!("MPRIS: quit");
        crate::bridge::on_app_quit();
        std::process::exit(0);
    }
}

// ── org.mpris.MediaPlayer2.Player ──

pub struct MprisPlayer {
    player: Arc<PlayerService>,
}

impl MprisPlayer {
    pub fn new(player: Arc<PlayerService>) -> Self {
        Self { player }
    }

    fn str_val(s: &str) -> OwnedValue {
        Value::new(s).try_into().unwrap_or(OwnedValue::from(false))
    }

    fn str_vec_val(v: Vec<&str>) -> OwnedValue {
        Value::new(v).try_into().unwrap_or(OwnedValue::from(false))
    }

    fn track_id(&self) -> OwnedObjectPath {
        let id = self
            .player
            .current_track()
            .map(|t| format!("/doremi/track/{}", t.id))
            .unwrap_or_else(|| "/doremi/track/none".into());
        zbus::zvariant::OwnedObjectPath::try_from(id.as_str()).unwrap_or_else(|_| {
            zbus::zvariant::OwnedObjectPath::try_from("/doremi/track/none").unwrap()
        })
    }
}

#[interface(name = "org.mpris.MediaPlayer2.Player")]
impl MprisPlayer {
    fn next(&mut self) {
        log::info!("MPRIS: next");
        self.player.next();
    }

    fn previous(&mut self) {
        log::info!("MPRIS: previous");
        self.player.previous();
    }

    fn pause(&mut self) {
        log::info!("MPRIS: pause");
        if self.player.is_playing() {
            self.player.toggle_play_pause();
        }
    }

    fn play_pause(&mut self) {
        log::info!("MPRIS: play_pause");
        self.player.toggle_play_pause();
    }

    fn stop(&mut self) {
        log::info!("MPRIS: stop");
        if self.player.is_playing() {
            self.player.toggle_play_pause();
        }
        self.player.seek(0);
    }

    fn play(&mut self) {
        log::info!("MPRIS: play");
        if !self.player.is_playing() {
            self.player.toggle_play_pause();
        }
    }

    fn seek(&mut self, offset: i64) {
        log::info!("MPRIS: seek by {offset}μs");
        self.player.seek_relative(offset / 1000);
    }

    fn set_position(&mut self, _track_id: zbus::zvariant::ObjectPath<'_>, position: i64) {
        log::info!("MPRIS: set position {position}μs");
        self.player.seek(position / 1000);
    }

    fn open_uri(&mut self, uri: &str) {
        log::info!("MPRIS: open URI {uri}");
        self.player.clear_queue();
        self.player.play_url(uri);
    }

    #[zbus(signal)]
    async fn seeked(signal_context: &zbus::SignalContext<'_>, position: i64) -> zbus::Result<()>;

    // ── Properties ──

    #[zbus(property)]
    fn playback_status(&self) -> &str {
        if self.player.is_playing() {
            "Playing"
        } else {
            "Paused"
        }
    }

    #[zbus(property)]
    fn loop_status(&self) -> &str {
        "None"
    }

    #[zbus(property)]
    fn rate(&self) -> f64 {
        1.0
    }

    #[zbus(property)]
    fn shuffle(&self) -> bool {
        false
    }

    #[zbus(property)]
    fn metadata(&self) -> HashMap<String, OwnedValue> {
        let mut dict = HashMap::new();
        let tid = self.track_id();

        dict.insert("mpris:trackid".into(), Self::str_val(&format!("{tid}")));

        if let Some(track) = self.player.current_track() {
            let len = (track.duration_ms.max(0) as i64) * 1000;
            dict.insert("mpris:length".into(), OwnedValue::from(len));
            dict.insert("xesam:title".into(), Self::str_val(&track.title));
            dict.insert(
                "xesam:artist".into(),
                Self::str_vec_val(vec![&track.artist]),
            );
            if !track.album.is_empty() {
                dict.insert("xesam:album".into(), Self::str_val(&track.album));
            }
            if !track.thumbnail.is_empty() {
                dict.insert("mpris:artUrl".into(), Self::str_val(&track.thumbnail));
            }
        } else {
            dict.insert("mpris:length".into(), OwnedValue::from(0_i64));
        }

        dict
    }

    #[zbus(property)]
    fn volume(&self) -> f64 {
        self.player.volume() as f64 / 100.0
    }

    #[zbus(property)]
    fn set_volume(&mut self, vol: f64) {
        self.player.set_volume((vol * 100.0) as i32);
    }

    #[zbus(property)]
    fn position(&self) -> i64 {
        self.player.position_ms() * 1000 // μs
    }

    #[zbus(property)]
    fn minimum_rate(&self) -> f64 {
        1.0
    }

    #[zbus(property)]
    fn maximum_rate(&self) -> f64 {
        1.0
    }

    #[zbus(property)]
    fn can_go_next(&self) -> bool {
        true
    }

    #[zbus(property)]
    fn can_go_previous(&self) -> bool {
        true
    }

    #[zbus(property)]
    fn can_play(&self) -> bool {
        true
    }

    #[zbus(property)]
    fn can_pause(&self) -> bool {
        true
    }

    #[zbus(property)]
    fn can_seek(&self) -> bool {
        true
    }

    #[zbus(property)]
    fn can_control(&self) -> bool {
        true
    }
}
