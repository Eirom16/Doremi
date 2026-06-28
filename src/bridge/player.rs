use super::bridge;
use super::with_player;

pub fn on_play_pause_triggered() {
    log::info!("Play/Pause triggered");
    with_player(|p| p.toggle_play_pause());
}

pub fn on_next_triggered() {
    log::info!("Next triggered");
    with_player(|p| p.next());
}

pub fn on_previous_triggered() {
    log::info!("Previous triggered");
    with_player(|p| p.previous());
}

pub fn on_shuffle_toggled(on: bool) {
    log::info!("Shuffle toggled: {on}");
    with_player(|p| p.toggle_shuffle());
}

pub fn on_repeat_cycled() {
    log::info!("Repeat cycled");
    with_player(|p| p.cycle_repeat());
}

pub fn on_volume_change(delta: i32) {
    with_player(|p| p.adjust_volume(delta));
}

pub fn on_volume_set(volume: i32) {
    with_player(|p| p.set_volume(volume));
}

pub fn on_seek_relative(delta_ms: i32) {
    with_player(|p| p.seek_relative(delta_ms as i64));
}

pub fn on_seek_absolute(position_ms: i32) {
    with_player(|p| p.seek(position_ms as i64));
}

pub fn on_timer_tick() {
    with_player(|p| {
        p.poll();
        crate::bridge::bridge::set_player_volume(p.volume());
    });
    with_player(|p| {
        use std::sync::atomic::{AtomicBool, AtomicI32, Ordering};
        static LAST_SHUFFLE: AtomicBool = AtomicBool::new(false);
        static LAST_REPEAT: AtomicI32 = AtomicI32::new(0);
        let s = p.shuffle_mode();
        let r = p.repeat_mode();
        if s != LAST_SHUFFLE.load(Ordering::Relaxed) {
            LAST_SHUFFLE.store(s, Ordering::Relaxed);
            crate::bridge::bridge::set_player_shuffle(s);
        }
        if r != LAST_REPEAT.load(Ordering::Relaxed) {
            LAST_REPEAT.store(r, Ordering::Relaxed);
            crate::bridge::bridge::set_player_repeat(r);
        }
    });
}

pub fn on_queue_item_clicked(index: i32) {
    if index < 0 {
        log::warn!("Ignoring invalid queue index {index}");
        return;
    }
    log::info!("Queue item clicked: jump to index {index}");
    with_player(|p| p.play_index(index as usize));
}

pub fn on_queue_item_removed(index: i32) {
    if index < 0 {
        log::warn!("Ignoring invalid queue removal index {index}");
        return;
    }
    with_player(|player| {
        if !player.remove_queue_item(index as usize) {
            log::warn!("Queue removal index {index} is out of bounds");
        }
    });
}

pub fn on_queue_item_moved(from: i32, to: i32) {
    if from < 0 || to < 0 {
        log::warn!("Ignoring invalid queue move {from} -> {to}");
        return;
    }
    with_player(|player| {
        if !player.move_queue_item(from as usize, to as usize) {
            log::warn!("Queue move {from} -> {to} is invalid");
        }
    });
}

pub fn on_queue_clear_requested() {
    log::info!("Clearing playback queue");
    with_player(|player| player.clear_queue());
}

pub fn on_play_all(tracks: Vec<bridge::Track>, shuffle: bool) {
    let count = tracks.len();
    log::info!("Playing all {count} tracks (shuffle={shuffle})");
    let track_infos: Vec<crate::player::queue::TrackInfo> =
        tracks.into_iter().map(queue_track_from_dto).collect();
    with_player(|p| p.play_all_tracks(track_infos, shuffle));
}

fn queue_track_from_dto(track: bridge::Track) -> crate::player::queue::TrackInfo {
    let track = sanitize_track_dto(track);
    crate::player::queue::TrackInfo {
        id: track.id,
        title: track.title,
        artist: track.artist,
        album: track.album,
        duration_ms: track.duration_ms,
        thumbnail: track.thumbnail,
        stream_url: String::new(),
    }
}

pub fn on_add_to_queue_next(track: bridge::Track) {
    if !is_playable_video_id(&track.id) {
        log::warn!(
            "Ignoring non-video queue-next item: {} (id: {})",
            track.title,
            track.id
        );
        return;
    }
    log::info!("Adding track {} next in queue", track.id);
    with_player(|player| player.enqueue_next(queue_track_from_dto(track)));
}

pub fn on_add_to_queue_end(track: bridge::Track) {
    if !is_playable_video_id(&track.id) {
        log::warn!(
            "Ignoring non-video queue-end item: {} (id: {})",
            track.title,
            track.id
        );
        return;
    }
    log::info!("Adding track {} to end of queue", track.id);
    with_player(|player| player.enqueue(queue_track_from_dto(track)));
}

pub fn on_window_close_requested() {
    log::info!("Window close requested — pausing playback");
    with_player(|p| {
        if p.is_playing() {
            p.toggle_play_pause();
        }
    });
}

pub fn on_search_item_clicked(track: bridge::Track) {
    let track = sanitize_track_dto(track);
    log::info!(
        "Search item clicked: {} — {} (id: {})",
        track.title,
        track.artist,
        track.id
    );
    if !is_playable_video_id(&track.id) {
        log::warn!(
            "Ignoring non-video search item for playback: {} (id: {})",
            track.title,
            track.id
        );
        return;
    }
    with_player(|p| p.play_track_dto(track));
}

pub(super) fn sanitize_track_dto(mut track: bridge::Track) -> bridge::Track {
    track.artist = clean_display_metadata(&track.artist);
    track.album = clean_display_metadata(&track.album);
    track
}

fn clean_display_metadata(value: &str) -> String {
    value
        .split('•')
        .map(str::trim)
        .find(|part| !is_display_metadata_noise(part))
        .unwrap_or_default()
        .to_string()
}

fn is_display_metadata_noise(value: &str) -> bool {
    let value = value.trim().to_lowercase();
    value.is_empty()
        || matches!(
            value.as_str(),
            "canción"
                | "cancion"
                | "song"
                | "video"
                | "sencillo"
                | "single"
                | "artista"
                | "artist"
                | "álbum"
                | "album"
                | "playlist"
                | "podcast"
                | "show"
        )
        || value.contains("visualizaci")
        || value.contains("views")
        || value.contains("reproducciones")
        || value.contains("subscribers")
        || value.contains("suscriptores")
}

pub(super) fn is_playable_video_id(id: &str) -> bool {
    id.len() == 11
        && !id.starts_with("UC")
        && id
            .chars()
            .all(|ch| ch.is_ascii_alphanumeric() || ch == '_' || ch == '-')
}
