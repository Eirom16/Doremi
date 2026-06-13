use std::collections::HashMap;
use std::sync::Arc;
use std::time::Duration;
use tokio;
use zbus::connection;
use zbus::interface;
use zbus::zvariant::{OwnedObjectPath, OwnedValue, Value};

use crate::player::PlayerService;

pub struct MprisService {
    player: Arc<PlayerService>,
}

impl MprisService {
    pub fn new(player: Arc<PlayerService>) -> Self {
        Self { player }
    }

    pub async fn start(self) -> zbus::Result<()> {
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

        // Keep the connection alive
        loop {
            tokio::time::sleep(Duration::from_secs(60)).await;
        }
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
    }

    fn quit(&mut self) {
        log::info!("MPRIS: quit");
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
        let id = self.player.current_track()
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

    // ── Properties ──

    #[zbus(property)]
    fn playback_status(&self) -> &str {
        if self.player.is_playing() { "Playing" } else { "Paused" }
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
            dict.insert("xesam:artist".into(), Self::str_vec_val(vec![&track.artist]));
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

pub fn spawn_mpris(player: Arc<PlayerService>) {
    tokio::spawn(async move {
        let service = MprisService::new(player);
        if let Err(e) = service.start().await {
            log::error!("MPRIS server failed: {e}");
        }
    });
}
