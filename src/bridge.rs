use crate::player::PlayerService;
use crate::services::search::SearchService;
use once_cell::sync::OnceCell;
use std::sync::Arc;

static PLAYER: OnceCell<Arc<PlayerService>> = OnceCell::new();
static SEARCH: OnceCell<SearchService> = OnceCell::new();

pub const CONTRACT_MAJOR: u16 = 1;
pub const CONTRACT_MINOR: u16 = 2;

fn versions_are_compatible(
    required_major: u16,
    required_minor: u16,
    provided_major: u16,
    provided_minor: u16,
) -> bool {
    provided_major == required_major && provided_minor >= required_minor
}

pub fn is_contract_compatible(cpp_major: u16, cpp_minor: u16) -> bool {
    versions_are_compatible(CONTRACT_MAJOR, CONTRACT_MINOR, cpp_major, cpp_minor)
}

pub fn verify_contract() -> Result<(), String> {
    let cpp_major = bridge::bridge_contract_major();
    let cpp_minor = bridge::bridge_contract_minor();
    if is_contract_compatible(cpp_major, cpp_minor) {
        log::info!(
            "Rust/C++ bridge contract compatible: Rust {}.{}, C++ {}.{}",
            CONTRACT_MAJOR,
            CONTRACT_MINOR,
            cpp_major,
            cpp_minor
        );
        Ok(())
    } else {
        Err(format!(
            "incompatible Rust/C++ bridge contract: Rust requires {}.{}, C++ provides {}.{}",
            CONTRACT_MAJOR, CONTRACT_MINOR, cpp_major, cpp_minor
        ))
    }
}

pub fn init_player(player: Arc<PlayerService>) {
    let _ = PLAYER.set(player);
}

pub fn init_search(search: SearchService) {
    let _ = SEARCH.set(search);
}

fn with_player<F, R>(f: F) -> R
where
    F: FnOnce(&PlayerService) -> R,
    R: Default,
{
    PLAYER.get().map(|p| f(p.as_ref())).unwrap_or_default()
}

#[cxx::bridge]
pub mod bridge {
    struct Track {
        id: String,
        title: String,
        artist: String,
        album: String,
        duration_ms: i64,
        thumbnail: String,
    }

    struct Playlist {
        id: String,
        name: String,
        description: String,
        thumbnail: String,
        track_count: i32,
    }

    struct Album {
        id: String,
        title: String,
        artist: String,
        year: String,
        thumbnail: String,
        track_count: i32,
    }

    struct Artist {
        id: String,
        name: String,
        thumbnail: String,
        description: String,
        subscribers: String,
    }

    struct StatsData {
        total_play_time: String,
        total_plays: i32,
        unique_artists: i32,
        weekly_activity: Vec<i32>,
        top_tracks: Vec<Track>,
    }

    // Rust callbacks invoked from C++
    extern "Rust" {
        fn on_play_pause_triggered();
        fn on_next_triggered();
        fn on_previous_triggered();
        fn on_shuffle_toggled(on: bool);
        fn on_repeat_cycled();
        fn on_search_submitted(query: &str, filter: &str);
        fn on_search_suggestions_requested(query: &str);
        fn on_search_history_requested();
        fn on_album_requested(browse_id: &str);
        fn on_artist_requested(browse_id: &str);
        fn on_playlist_requested(playlist_id: &str);
        fn on_volume_change(delta: i32);
        fn on_volume_set(volume: i32);
        fn on_seek_relative(delta_ms: i32);
        fn on_seek_absolute(position_ms: i32);
        fn on_window_close_requested();
        fn on_setting_changed(key: &str, value: &str);
        fn on_timer_tick();
        fn on_search_item_clicked(track: Track);
        fn on_app_quit();
        fn on_library_tab_changed(tab: &str);
        fn on_add_favorite(track: Track);
        fn on_remove_favorite(track_id: &str);
        fn on_download_requested(track: Track);
        fn on_add_to_queue_next(track: Track);
        fn on_add_to_queue_end(track: Track);
        fn on_lastfm_auth_requested(
            api_key: &str,
            api_secret: &str,
            username: &str,
            password: &str,
        );
        fn on_lastfm_disconnect_requested();
        fn on_queue_item_clicked(index: i32);
        fn on_queue_item_removed(index: i32);
        fn on_queue_item_moved(from: i32, to: i32);
        fn on_queue_clear_requested();
        fn on_stats_requested();
        fn on_history_requested();
        fn on_youtube_login_success(headers_json: &str, name: &str, avatar_url: &str);
        fn on_youtube_logout();
        fn is_youtube_authenticated() -> bool;
        fn on_check_for_updates_requested();
        fn on_download_update_requested(asset_url: &str, asset_name: &str);
        fn on_validate_sudo_password(password: &str) -> bool;
        fn on_install_update_requested(package_path: &str, password: &str);
        fn get_app_version() -> String;
        fn doremi_tr(key: &str) -> String;
        fn get_storage_sizes() -> Vec<f64>;
        fn clear_cache();
        fn clear_downloads();
        fn export_backup(zip_path: &str) -> bool;
        fn import_backup(zip_path: &str) -> bool;
    }

    // C++ functions called from Rust
    unsafe extern "C++" {
        include!("main_window.h");
        fn bridge_contract_major() -> u16;
        fn bridge_contract_minor() -> u16;
        fn create_main_window(app_name: &str, theme_mode: &str, accent_color: &str, font_size: i32);
        fn show_main_window();
        fn navigate_to(route: &str);
        fn show_notification(message: &str, kind: &str);
        fn apply_theme(theme_mode: &str, accent_color: &str);
        fn update_player_state(state: i32, position_ms: i32, duration_ms: i32);
        fn set_mini_player(title: &str, artist: &str, thumbnail: &str);
        fn get_search_bar_text() -> String;
        fn set_search_bar_text(text: &str);
        fn set_window_title(title: &str);
        fn set_playing(playing: bool);
        fn run_event_loop();
        fn set_player_volume(volume: i32);
        fn set_library_songs(songs: Vec<Track>);
        fn set_library_playlists(playlists: Vec<Playlist>);
        fn set_library_albums(albums: Vec<Album>);
        fn set_library_artists(artists: Vec<Artist>);
        fn set_search_history(queries: Vec<String>);
        fn set_search_suggestions(suggestions: Vec<String>);
        fn apply_settings_to_ui();
        fn set_settings_theme(mode: &str);
        fn set_settings_accent(color: &str);
        fn set_settings_font_size(size: i32);
        fn set_settings_language(lang: &str);
        fn set_settings_normalize(on: bool);
        fn set_settings_crossfade(on: bool);
        fn set_settings_equalizer_enabled(on: bool);
        fn set_settings_equalizer_preset(preset: &str);
        fn set_settings_equalizer_values(preamp: f64, bands: Vec<f64>);
        fn set_settings_sleep_timer(minutes: i32);
        fn set_settings_discord_rpc(on: bool);
        fn set_settings_lastfm_enabled(on: bool);
        fn set_settings_lastfm_session(
            authenticated: bool,
            username: &str,
            api_key: &str,
            api_secret: &str,
        );
        fn set_track_lyrics(plain: &str, synced: &str);
        fn set_settings_subtitle_alignment(align: &str);
        fn set_settings_subtitle_font_size(size: i32);
        fn set_settings_subtitle_line_spacing(spacing: f64);
        fn set_settings_subtitle_auto_scroll(on: bool);
        fn set_settings_subtitle_glow_effect(on: bool);
        fn set_settings_stop_on_close(stop: bool);
        fn set_settings_mpris_enabled(on: bool);

        // Data service functions
        fn set_search_results(
            songs: Vec<Track>,
            artists: Vec<Artist>,
            albums: Vec<Album>,
            playlists: Vec<Playlist>,
        );
        fn add_home_section(title: &str, items: Vec<String>);
        fn clear_home_sections();
        fn set_trending_items(titles: Vec<String>, subtitles: Vec<String>, thumbnails: Vec<String>);
        fn set_downloads_list(titles: Vec<String>, artists: Vec<String>, thumbnails: Vec<String>);
        fn set_player_shuffle(on: bool);
        fn set_player_repeat(mode: i32);
        fn get_or_create_thumbnail(title: &str, variant: i32) -> String;
        fn set_dominant_colors(colors: Vec<String>);
        fn set_playback_queue(queue: Vec<Track>, current_index: i32);
        fn set_stats_data(stats: StatsData);

        fn set_history_data(history: Vec<Track>, played_at: Vec<String>);

        fn set_album_detail(album: Album, tracks: Vec<Track>);

        fn set_artist_detail(artist: Artist, tracks: Vec<Track>, albums: Vec<Album>);

        fn set_playlist_detail(playlist: Playlist, tracks: Vec<Track>);
        fn update_youtube_auth_state(authenticated: bool, name: &str, avatar_url: &str);
        fn set_update_available(
            version: &str,
            notes: &str,
            url: &str,
            asset_url: &str,
            asset_name: &str,
            asset_size: i64,
        );
        fn set_no_update_available();
        fn set_update_download_progress(percent: f64, message: &str);
        fn set_update_download_finished(package_path: &str);
        fn set_update_download_failed(error: &str);
        fn set_update_install_finished(success: bool);
    }
}

// Rust callback implementations (called from C++)
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

pub fn on_search_submitted(query: &str, filter: &str) {
    if query.trim().is_empty() {
        return;
    }
    log::info!("Search: {query} with filter: {filter}");
    let query = query.to_string();
    let filter = filter.to_string();
    tokio::spawn(async move {
        let q_clone = query.clone();
        let f_clone = filter.clone();
        tokio::task::spawn_blocking(move || {
            let _ = crate::db::repo::SearchHistoryRepo::record(&q_clone, &f_clone);
        })
        .await
        .ok();

        push_search_history_to_ui().await;

        if let Some(search) = SEARCH.get() {
            let res = search.search(&query, &filter).await;
            search.push_to_ui(&res);
        }
    });
}

pub fn on_search_suggestions_requested(query: &str) {
    let query = query.to_string();
    tokio::spawn(async move {
        let suggestions = match SEARCH.get() {
            Some(search) => search.suggestions(&query).await,
            None => Vec::new(),
        };
        crate::bridge::bridge::set_search_suggestions(suggestions);
    });
}

async fn push_search_history_to_ui() {
    let queries = tokio::task::spawn_blocking(|| {
        crate::db::repo::SearchHistoryRepo::recent(20)
            .unwrap_or_default()
            .into_iter()
            .map(|entry| entry.query)
            .collect::<Vec<_>>()
    })
    .await
    .unwrap_or_default();
    crate::bridge::bridge::set_search_history(queries);
}

pub fn on_search_history_requested() {
    tokio::spawn(push_search_history_to_ui());
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

pub fn on_window_close_requested() {
    log::info!("Window close requested — pausing playback");
    with_player(|p| {
        if p.is_playing() {
            p.toggle_play_pause();
        }
    });
}

pub fn handle_forwarded_args(args: Vec<String>) {
    log::info!("Received forwarded arguments: {:?}", args);
    bridge::show_main_window();
    for arg in args {
        if arg.starts_with("http://") || arg.starts_with("https://") {
            log::info!("Playing forwarded URL: {}", arg);
            if let Some(player) = PLAYER.get() {
                player.clear_queue();
                player.play_url(&arg);
            }
            break;
        }
    }
}

pub fn on_app_quit() {
    log::info!("App quit requested — cleaning up");
    let vol = with_player(|p| p.volume());
    with_player(|p| p.stop());
    let dirs = crate::config::paths::AppDirs::global();
    let mut settings = crate::config::settings::AppSettings::load(&dirs.settings_path());
    settings.player.volume = vol;
    if let Err(e) = settings.save(&dirs.settings_path()) {
        log::error!("Failed to save settings on quit: {e}");
    }
    let _ = crate::db::take_connection();
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
enum LibraryTab {
    Songs,
    Albums,
    Artists,
    Playlists,
}

impl LibraryTab {
    fn from_key(key: &str) -> Option<Self> {
        match key {
            "songs" => Some(Self::Songs),
            "albums" => Some(Self::Albums),
            "artists" => Some(Self::Artists),
            "playlists" => Some(Self::Playlists),
            _ => None,
        }
    }
}

pub fn on_library_tab_changed(tab_key: &str) {
    log::info!("Library tab changed: {tab_key}");
    let tab_key = tab_key.to_string();
    tokio::spawn(async move {
        let Some(tab) = LibraryTab::from_key(&tab_key) else {
            log::warn!("Ignoring unknown library tab key: {tab_key}");
            return;
        };

        if is_youtube_authenticated() {
            let service = crate::services::library::LibraryService::new();
            match tab {
                LibraryTab::Songs => service.load_songs().await,
                LibraryTab::Albums => service.load_albums().await,
                LibraryTab::Artists => service.load_artists().await,
                LibraryTab::Playlists => service.load_playlists().await,
            }
        } else {
            tokio::task::spawn_blocking(move || {
                use crate::db::repo::*;
                match tab {
                    LibraryTab::Songs => {
                        if let Ok(tracks) = FavoritesRepo::all_tracks() {
                            let songs: Vec<crate::bridge::bridge::Track> = tracks
                                .iter()
                                .map(|t| crate::bridge::bridge::Track {
                                    id: t.id.clone(),
                                    title: t.title.clone(),
                                    artist: t.artist.clone(),
                                    album: t.album.clone(),
                                    duration_ms: t.duration_ms,
                                    thumbnail: t.thumbnail.clone(),
                                })
                                .collect();
                            crate::bridge::bridge::set_library_songs(songs);
                        }
                    }
                    LibraryTab::Albums => {
                        if let Ok(albums) = FavoritesRepo::all_albums() {
                            let a_list: Vec<crate::bridge::bridge::Album> = albums
                                .iter()
                                .map(|a| crate::bridge::bridge::Album {
                                    id: a.id.clone(),
                                    title: a.title.clone(),
                                    artist: a.artist.clone(),
                                    year: a.year.map(|y| y.to_string()).unwrap_or_default(),
                                    thumbnail: a.thumbnail.clone(),
                                    track_count: 0,
                                })
                                .collect();
                            crate::bridge::bridge::set_library_albums(a_list);
                        }
                    }
                    LibraryTab::Artists => {
                        if let Ok(artists) = FavoritesRepo::all_artists() {
                            let art_list: Vec<crate::bridge::bridge::Artist> = artists
                                .iter()
                                .map(|a| crate::bridge::bridge::Artist {
                                    id: a.id.clone(),
                                    name: a.name.clone(),
                                    thumbnail: a.thumbnail.clone(),
                                    description: String::new(),
                                    subscribers: String::new(),
                                })
                                .collect();
                            crate::bridge::bridge::set_library_artists(art_list);
                        }
                    }
                    LibraryTab::Playlists => {
                        if let Ok(playlists) = PlaylistRepo::all() {
                            let p_list: Vec<crate::bridge::bridge::Playlist> = playlists
                                .iter()
                                .map(|p| {
                                    let count = PlaylistRepo::tracks(&p.id)
                                        .ok()
                                        .map(|t| t.len() as i32)
                                        .unwrap_or(0);
                                    crate::bridge::bridge::Playlist {
                                        id: p.id.clone(),
                                        name: p.name.clone(),
                                        description: p.description.clone(),
                                        thumbnail: p.artwork.clone(),
                                        track_count: count,
                                    }
                                })
                                .collect();
                            crate::bridge::bridge::set_library_playlists(p_list);
                        }
                    }
                }
            })
            .await
            .ok();
        }
    });
}

#[cfg(test)]
mod contract_tests {
    use super::{
        is_contract_compatible, versions_are_compatible, LibraryTab, CONTRACT_MAJOR, CONTRACT_MINOR,
    };

    #[test]
    fn bridge_contract_rejects_incompatible_versions() {
        assert!(is_contract_compatible(CONTRACT_MAJOR, CONTRACT_MINOR));
        assert!(is_contract_compatible(CONTRACT_MAJOR, CONTRACT_MINOR + 1));
        assert!(!is_contract_compatible(CONTRACT_MAJOR + 1, CONTRACT_MINOR));
        assert!(!versions_are_compatible(1, 2, 1, 1));
    }

    #[test]
    fn library_tabs_use_stable_non_localized_keys() {
        assert_eq!(LibraryTab::from_key("songs"), Some(LibraryTab::Songs));
        assert_eq!(LibraryTab::from_key("albums"), Some(LibraryTab::Albums));
        assert_eq!(LibraryTab::from_key("artists"), Some(LibraryTab::Artists));
        assert_eq!(
            LibraryTab::from_key("playlists"),
            Some(LibraryTab::Playlists)
        );
        assert_eq!(LibraryTab::from_key("Canciones"), None);
        assert_eq!(LibraryTab::from_key("Songs"), None);
        assert_eq!(LibraryTab::from_key(""), None);
    }

    fn run_in_test_db<F, R>(f: F) -> R
    where
        F: FnOnce() -> R,
    {
        use crate::db::{init_connection, take_connection, Database};
        use rusqlite::Connection;

        let conn = Connection::open_in_memory().unwrap();
        Database::run_migrations(&conn).unwrap();
        init_connection(conn);

        let res = f();

        let _ = take_connection(); // Clean up
        res
    }

    #[test]
    fn test_track_unicode_round_trip() {
        run_in_test_db(|| {
            use crate::db::repo::FavoritesRepo;
            let track = super::bridge::Track {
                id: "fav_unicode_🚀_日本語".to_string(),
                title: "Canción con tilde y emoji 🚀".to_string(),
                artist: "日本語の歌手".to_string(),
                album: "Álbum Especial".to_string(),
                duration_ms: 240000,
                thumbnail: "https://example.com/🚀.png".to_string(),
            };

            super::on_add_favorite_impl(track);

            let saved = FavoritesRepo::all_tracks().unwrap();
            assert_eq!(saved.len(), 1);
            assert_eq!(saved[0].id, "fav_unicode_🚀_日本語");
            assert_eq!(saved[0].title, "Canción con tilde y emoji 🚀");
            assert_eq!(saved[0].artist, "日本語の歌手");
        });
    }

    #[test]
    fn test_track_invalid_and_empty_id() {
        run_in_test_db(|| {
            use crate::db::repo::FavoritesRepo;
            // 1. Caso de ID vacío
            let track_empty_id = super::bridge::Track {
                id: "".to_string(),
                title: "Song Empty ID".to_string(),
                artist: "Artist".to_string(),
                album: "".to_string(),
                duration_ms: 0,
                thumbnail: "".to_string(),
            };
            super::on_add_favorite_impl(track_empty_id);

            // 2. Caso de título vacío
            let track_empty_title = super::bridge::Track {
                id: "some_valid_id".to_string(),
                title: "".to_string(),
                artist: "Artist".to_string(),
                album: "".to_string(),
                duration_ms: 0,
                thumbnail: "".to_string(),
            };
            super::on_add_favorite_impl(track_empty_title);

            let saved = FavoritesRepo::all_tracks().unwrap();
            assert!(
                saved.is_empty(),
                "Track con ID o título vacío no debe ser insertado"
            );
        });
    }

    #[test]
    fn test_large_list_allocation() {
        let mut tracks = Vec::new();
        for i in 0..10000 {
            tracks.push(super::bridge::Track {
                id: format!("id_{i}"),
                title: format!("Title {i}"),
                artist: format!("Artist {i}"),
                album: format!("Album {i}"),
                duration_ms: 180000,
                thumbnail: format!("https://example.com/thumb_{i}.png"),
            });
        }

        assert_eq!(tracks.len(), 10000);
        assert_eq!(tracks[9999].id, "id_9999");
    }
}

pub fn on_remove_favorite_impl(track_id: &str) {
    log::info!("Remove favorite: {track_id}");
    if let Err(e) = crate::db::repo::FavoritesRepo::remove_track(track_id) {
        log::error!("Failed to remove favorite: {e}");
    }
}

pub fn on_remove_favorite(track_id: &str) {
    let track_id = track_id.to_string();
    tokio::spawn(async move {
        let local_id = track_id.clone();
        tokio::task::spawn_blocking(move || {
            on_remove_favorite_impl(&local_id);
        })
        .await
        .ok();
        if let Err(error) =
            crate::api::innertube::rate_song(&track_id, crate::api::models::LikeStatus::Indifferent)
                .await
        {
            log::debug!("Remote unlike was not applied for {track_id}: {error}");
        }
    });
}

pub fn on_add_favorite_impl(track: bridge::Track) {
    if track.id.trim().is_empty() || track.title.trim().is_empty() {
        log::warn!("Rejected adding favorite track: empty id or title");
        return;
    }
    log::info!("Add favorite: {} — {}", track.title, track.artist);
    use crate::db::repo::FavoriteTrack;
    let fav_track = FavoriteTrack {
        id: track.id.clone(),
        title: track.title,
        artist: track.artist,
        album: track.album,
        album_id: String::new(),
        duration_ms: track.duration_ms,
        thumbnail: track.thumbnail,
        added_at: String::new(),
    };
    if let Err(e) = crate::db::repo::FavoritesRepo::add_track(&fav_track) {
        log::error!("Failed to add favorite: {e}");
    } else {
        log::info!("Added favorite: {} — {}", fav_track.title, fav_track.artist);
    }
}

pub fn on_add_favorite(track: bridge::Track) {
    let track_id = track.id.clone();
    tokio::spawn(async move {
        tokio::task::spawn_blocking(move || {
            on_add_favorite_impl(track);
        })
        .await
        .ok();
        if let Err(error) =
            crate::api::innertube::rate_song(&track_id, crate::api::models::LikeStatus::Like).await
        {
            log::debug!("Remote like was not applied for {track_id}: {error}");
        }
    });
}

pub fn on_download_requested(track: bridge::Track) {
    log::info!("Download requested: {} — {}", track.title, track.artist);
    if !track.id.is_empty() {
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

pub fn on_search_item_clicked(track: bridge::Track) {
    log::info!(
        "Search item clicked: {} — {} (id: {})",
        track.title,
        track.artist,
        track.id
    );
    with_player(|p| p.play_track_dto(track));
}

fn queue_track_from_dto(track: bridge::Track) -> crate::player::queue::TrackInfo {
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
    log::info!("Adding track {} next in queue", track.id);
    with_player(|player| player.enqueue_next(queue_track_from_dto(track)));
}

pub fn on_add_to_queue_end(track: bridge::Track) {
    log::info!("Adding track {} to end of queue", track.id);
    with_player(|player| player.enqueue(queue_track_from_dto(track)));
}

pub fn on_timer_tick() {
    with_player(|p| {
        p.poll();
        crate::bridge::bridge::set_player_volume(p.volume());
    });
    // Sync shuffle/repeat state once (lazy — only when value changes)
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

pub fn apply_settings_impl() {
    use crate::config::paths::AppDirs;
    use crate::config::settings::AppSettings;
    let dirs = AppDirs::global();
    let settings = AppSettings::load(&dirs.settings_path());
    bridge::apply_settings_to_ui();
    bridge::set_settings_theme(&settings.appearance.theme_mode);
    bridge::set_settings_accent(&settings.appearance.accent_color);
    bridge::set_settings_font_size(settings.appearance.font_size);
    bridge::set_settings_language(&settings.language);
    bridge::set_settings_normalize(settings.player.normalize_audio);
    bridge::set_settings_crossfade(settings.player.crossfade_enabled);
    bridge::set_settings_equalizer_enabled(settings.equalizer.enabled);
    bridge::set_settings_equalizer_preset(&settings.equalizer.preset_name);
    bridge::set_settings_equalizer_values(
        settings.equalizer.preamp,
        settings.equalizer.bands.clone(),
    );
    bridge::set_settings_sleep_timer(settings.player.sleep_timer_minutes);

    // Apply subtitles settings
    bridge::set_settings_subtitle_alignment(&settings.subtitles.alignment);
    bridge::set_settings_subtitle_font_size(settings.subtitles.font_size);
    bridge::set_settings_subtitle_line_spacing(settings.subtitles.line_spacing);
    bridge::set_settings_subtitle_auto_scroll(settings.subtitles.auto_scroll);
    bridge::set_settings_subtitle_glow_effect(settings.subtitles.glow_effect);

    // Apply integrations settings
    crate::services::discord::set_enabled(settings.integrations.discord_rpc_enabled);
    crate::services::lastfm::set_enabled(settings.integrations.lastfm_enabled);

    bridge::set_settings_discord_rpc(settings.integrations.discord_rpc_enabled);
    bridge::set_settings_lastfm_enabled(settings.integrations.lastfm_enabled);
    bridge::set_settings_mpris_enabled(settings.integrations.mpris_enabled);
    bridge::set_settings_stop_on_close(settings.player.stop_on_close);

    if settings.integrations.mpris_enabled {
        if let Some(player) = PLAYER.get() {
            crate::mpris::start_mpris(player.clone());
        }
    } else {
        crate::mpris::stop_mpris();
    }

    let lastfm_authenticated = !settings.integrations.lastfm_session_key.is_empty();
    bridge::set_settings_lastfm_session(
        lastfm_authenticated,
        &settings.integrations.lastfm_username,
        "",
        "",
    );

    // Apply player settings to the AudioEngine
    with_player(|p| {
        p.set_volume(settings.player.volume);
        p.apply_equalizer(
            settings.equalizer.enabled,
            settings.equalizer.preamp,
            &settings.equalizer.bands,
            &settings.equalizer.preset_name,
        );
        p.set_sleep_timer(settings.player.sleep_timer_minutes);
    });
}

pub fn on_setting_changed(key: &str, value: &str) {
    log::info!("Setting changed: {key} = {value}");
    use crate::config::paths::AppDirs;
    use crate::config::settings::AppSettings;
    let dirs = AppDirs::global();
    let mut settings = AppSettings::load(&dirs.settings_path());
    match key {
        "theme_mode" => {
            settings.appearance.theme_mode = value.to_string();
            bridge::apply_theme(value, &settings.appearance.accent_color);
        }
        "accent_color" => {
            settings.appearance.accent_color = value.to_string();
            bridge::apply_theme(&settings.appearance.theme_mode, value);
        }
        "font_size" => {
            if let Ok(v) = value.parse::<i32>() {
                settings.appearance.font_size = v;
            }
        }
        "language" => {
            settings.language = value.to_string();
            crate::utils::i18n::set_language(value);
            crate::api::endpoints::configure(value, &settings.network.region);
        }
        "normalize_audio" => settings.player.normalize_audio = value == "true",
        "crossfade_enabled" => settings.player.crossfade_enabled = value == "true",
        "equalizer_enabled" => {
            settings.equalizer.enabled = value == "true";
            let enabled = settings.equalizer.enabled;
            let preamp = settings.equalizer.preamp;
            let bands = settings.equalizer.bands.clone();
            let preset = settings.equalizer.preset_name.clone();
            with_player(|p| p.apply_equalizer(enabled, preamp, &bands, &preset));
        }
        "equalizer_preset" => {
            settings.equalizer.preset_name = value.to_string();
            if let Some((preamp, bands)) = crate::config::themes::eq_presets().get(value) {
                settings.equalizer.preamp = *preamp;
                settings.equalizer.bands = bands.to_vec();
                bridge::set_settings_equalizer_values(*preamp, bands.to_vec());
            }
            let enabled = settings.equalizer.enabled;
            let preamp = settings.equalizer.preamp;
            let bands = settings.equalizer.bands.clone();
            let preset = settings.equalizer.preset_name.clone();
            with_player(|p| p.apply_equalizer(enabled, preamp, &bands, &preset));
        }
        "equalizer_preamp" => {
            if let Ok(v) = value.parse::<f64>() {
                settings.equalizer.preamp = v.clamp(-20.0, 20.0);
                settings.equalizer.preset_name = "Custom".to_string();
                let enabled = settings.equalizer.enabled;
                let preamp = settings.equalizer.preamp;
                let bands = settings.equalizer.bands.clone();
                let preset = settings.equalizer.preset_name.clone();
                with_player(|p| p.apply_equalizer(enabled, preamp, &bands, &preset));
            }
        }
        "equalizer_bands" => {
            let parsed_bands: Vec<f64> = value
                .split(',')
                .filter_map(|s| s.parse::<f64>().ok())
                .collect();
            if !parsed_bands.is_empty() {
                settings.equalizer.bands = parsed_bands
                    .into_iter()
                    .take(10)
                    .map(|value| value.clamp(-20.0, 20.0))
                    .collect();
                settings.equalizer.bands.resize(10, 0.0);
                settings.equalizer.preset_name = "Custom".to_string();
                let enabled = settings.equalizer.enabled;
                let preamp = settings.equalizer.preamp;
                let bands = settings.equalizer.bands.clone();
                let preset = settings.equalizer.preset_name.clone();
                with_player(|p| p.apply_equalizer(enabled, preamp, &bands, &preset));
            }
        }
        "sleep_timer" => {
            if let Ok(v) = value.parse::<i32>() {
                settings.player.sleep_timer_minutes = v;
                with_player(|p| p.set_sleep_timer(v));
            }
        }
        "player_volume" => {
            if let Ok(v) = value.parse::<i32>() {
                settings.player.volume = v;
                with_player(|p| p.set_volume(v));
            }
        }
        "discord_rpc_enabled" => {
            settings.integrations.discord_rpc_enabled = value == "true";
            crate::services::discord::set_enabled(settings.integrations.discord_rpc_enabled);
        }
        "lastfm_enabled" => {
            settings.integrations.lastfm_enabled = value == "true";
            crate::services::lastfm::set_enabled(settings.integrations.lastfm_enabled);
        }
        "subtitle_alignment" => {
            settings.subtitles.alignment = value.to_string();
            bridge::set_settings_subtitle_alignment(value);
        }
        "subtitle_font_size" => {
            if let Ok(v) = value.parse::<i32>() {
                settings.subtitles.font_size = v;
                bridge::set_settings_subtitle_font_size(v);
            }
        }
        "subtitle_line_spacing" => {
            if let Ok(v) = value.parse::<f64>() {
                settings.subtitles.line_spacing = v;
                bridge::set_settings_subtitle_line_spacing(v);
            }
        }
        "subtitle_auto_scroll" => {
            settings.subtitles.auto_scroll = value == "true";
            bridge::set_settings_subtitle_auto_scroll(value == "true");
        }
        "subtitle_glow_effect" => {
            settings.subtitles.glow_effect = value == "true";
            bridge::set_settings_subtitle_glow_effect(value == "true");
        }
        "stop_on_close" => {
            let stop = value == "true";
            settings.player.stop_on_close = stop;
            bridge::set_settings_stop_on_close(stop);
        }
        "mpris_enabled" => {
            let enabled = value == "true";
            settings.integrations.mpris_enabled = enabled;
            bridge::set_settings_mpris_enabled(enabled);
            if enabled {
                if let Some(player) = PLAYER.get() {
                    crate::mpris::start_mpris(player.clone());
                }
            } else {
                crate::mpris::stop_mpris();
            }
        }
        _ => log::warn!("Unknown setting key: {key}"),
    }
    if let Err(e) = settings.save(&dirs.settings_path()) {
        log::error!("Failed to save setting '{key}': {e}");
    }
}

pub fn on_lastfm_auth_requested(api_key: &str, api_secret: &str, username: &str, password: &str) {
    use zeroize::{Zeroize, Zeroizing};

    let api_key = api_key.trim().to_string();
    let api_secret = Zeroizing::new(api_secret.trim().to_string());
    let username = username.trim().to_string();
    let mut password = Zeroizing::new(password.to_string());

    tokio::spawn(async move {
        bridge::show_notification("Conectando con Last.fm...", "info");
        let auth_result =
            crate::services::lastfm::authenticate(&api_key, &api_secret, &username, &password)
                .await;
        password.zeroize();

        match auth_result {
            Ok(session_key) => {
                log::info!("Last.fm auth successful");
                let dirs = crate::config::paths::AppDirs::global();
                let mut settings =
                    crate::config::settings::AppSettings::load(&dirs.settings_path());

                let credentials = crate::utils::secure_storage::LastFmCredentials {
                    api_key: api_key.clone(),
                    api_secret: api_secret.to_string(),
                    session_key,
                };
                if let Err(e) = crate::utils::secure_storage::save_lastfm_credentials(&credentials)
                {
                    log::error!("Failed to store Last.fm credentials securely: {e}");
                    bridge::show_notification(
                        "No se pudo acceder al llavero del sistema; la cuenta no fue guardada",
                        "error",
                    );
                    return;
                }

                settings.integrations.lastfm_username = username.clone();
                settings.integrations.lastfm_enabled = true;

                if let Err(e) = settings.save(&dirs.settings_path()) {
                    log::error!("Failed to save Last.fm settings: {e}");
                }

                crate::services::lastfm::set_enabled(true);

                bridge::set_settings_lastfm_session(true, &username, "", "");
                bridge::set_settings_lastfm_enabled(true);
                bridge::show_notification("Cuenta de Last.fm conectada con éxito", "success");
            }
            Err(e) => {
                log::warn!("Last.fm auth failed: {e}");
                bridge::show_notification(&format!("Error de autenticación: {e}"), "error");
            }
        }
    });
}

pub fn on_lastfm_disconnect_requested() {
    log::info!("Disconnecting Last.fm account");
    let dirs = crate::config::paths::AppDirs::global();
    let mut settings = crate::config::settings::AppSettings::load(&dirs.settings_path());

    settings.integrations.lastfm_api_key.clear();
    settings.integrations.lastfm_api_secret.clear();
    settings.integrations.lastfm_username.clear();
    settings.integrations.lastfm_session_key.clear();
    settings.integrations.lastfm_enabled = false;

    if let Err(e) = crate::utils::secure_storage::delete_lastfm_credentials() {
        log::warn!("Failed to delete Last.fm credentials: {e}");
    }

    if let Err(e) = settings.save(&dirs.settings_path()) {
        log::error!("Failed to save settings: {e}");
    }

    crate::services::lastfm::set_enabled(false);

    bridge::set_settings_lastfm_session(false, "", "", "");
    bridge::set_settings_lastfm_enabled(false);
    bridge::show_notification("Cuenta de Last.fm desconectada", "info");
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

pub fn on_stats_requested() {
    log::info!("Stats requested");
    tokio::spawn(async move {
        let stats_res = tokio::task::spawn_blocking(move || {
            let total_time_ms = crate::db::with_db(|conn| {
                conn.query_row("SELECT SUM(duration_ms) FROM recently_played", [], |r| {
                    let val: Option<i64> = r.get(0)?;
                    Ok(val.unwrap_or(0))
                })
            })
            .unwrap_or(0);

            let total_plays = crate::db::with_db(|conn| {
                conn.query_row("SELECT COUNT(*) FROM recently_played", [], |r| {
                    let val: i32 = r.get(0)?;
                    Ok(val)
                })
            })
            .unwrap_or(0);

            let unique_artists = crate::db::with_db(|conn| {
                conn.query_row(
                    "SELECT COUNT(DISTINCT artist) FROM recently_played",
                    [],
                    |r| {
                        let val: i32 = r.get(0)?;
                        Ok(val)
                    },
                )
            })
            .unwrap_or(0);

            // Weekly activity: last 7 days daily counts
            let mut weekly_activity = vec![0; 7];
            if let Ok(results) = crate::db::with_db(|conn| {
                let mut stmt = conn.prepare(
                    "SELECT strftime('%Y-%m-%d', played_at) as play_day, COUNT(*) as cnt
                     FROM recently_played
                     WHERE played_at >= datetime('now', '-7 days')
                     GROUP BY play_day
                     ORDER BY play_day DESC
                     LIMIT 7",
                )?;
                let rows = stmt.query_map([], |r| {
                    let day_str: String = r.get(0)?;
                    let count: i32 = r.get(1)?;
                    Ok((day_str, count))
                })?;
                let list: Result<Vec<(String, i32)>, rusqlite::Error> = rows.collect();
                list
            }) {
                let now = chrono::Local::now();
                for i in 0..7 {
                    let d = now - chrono::Duration::days(6 - i as i64);
                    let d_str = d.format("%Y-%m-%d").to_string();
                    if let Some((_, cnt)) = results.iter().find(|(day, _)| day == &d_str) {
                        weekly_activity[i] = *cnt;
                    }
                }
            }

            // Top 5 Tracks
            let mut top_tracks = Vec::new();

            if let Ok(results) = crate::db::with_db(|conn| {
                let mut stmt = conn.prepare(
                    "SELECT track_id, title, artist, duration_ms, thumbnail, COUNT(*) as cnt
                     FROM recently_played
                     GROUP BY track_id, title, artist
                     ORDER BY cnt DESC
                     LIMIT 5",
                )?;
                let rows = stmt.query_map([], |r| {
                    let id: String = r.get(0)?;
                    let title: String = r.get(1)?;
                    let artist: String = r.get(2)?;
                    let duration_ms: i64 = r.get(3)?;
                    let thumb: String = r.get(4)?;
                    Ok((id, title, artist, duration_ms, thumb))
                })?;
                let list: Result<Vec<_>, rusqlite::Error> = rows.collect();
                list
            }) {
                for row in results {
                    let (id, title, artist, duration_ms, mut thumb) = row;
                    if thumb.is_empty() {
                        thumb = crate::bridge::bridge::get_or_create_thumbnail(&title, 0);
                    }
                    top_tracks.push(crate::bridge::bridge::Track {
                        id,
                        title,
                        artist,
                        album: String::new(),
                        duration_ms,
                        thumbnail: thumb,
                    });
                }
            }

            let total_secs = total_time_ms / 1000;
            let total_mins = total_secs / 60;
            let total_hours = total_mins / 60;
            let time_str = if total_hours > 0 {
                format!("{}h {}m", total_hours, total_mins % 60)
            } else {
                format!("{}m", total_mins)
            };

            crate::bridge::bridge::StatsData {
                total_play_time: time_str,
                total_plays,
                unique_artists,
                weekly_activity,
                top_tracks,
            }
        })
        .await;

        if let Ok(stats) = stats_res {
            crate::bridge::bridge::set_stats_data(stats);
        }
    });
}

fn load_local_history() -> (Vec<bridge::Track>, Vec<String>) {
    let mut history = Vec::new();
    let mut played_at = Vec::new();
    if let Ok(results) = crate::db::with_db(|conn| {
        let mut stmt = conn.prepare(
            "SELECT track_id, title, artist, duration_ms, thumbnail, played_at
             FROM recently_played
             ORDER BY played_at DESC
             LIMIT 50",
        )?;
        let rows = stmt.query_map([], |row| {
            Ok((
                row.get::<_, String>(0)?,
                row.get::<_, String>(1)?,
                row.get::<_, String>(2)?,
                row.get::<_, i64>(3)?,
                row.get::<_, String>(4)?,
                row.get::<_, String>(5)?,
            ))
        })?;
        rows.collect::<Result<Vec<_>, rusqlite::Error>>()
    }) {
        for (id, title, artist, duration_ms, thumbnail, played) in results {
            history.push(bridge::Track {
                id,
                title,
                artist,
                album: String::new(),
                duration_ms,
                thumbnail,
            });
            played_at.push(played);
        }
    }
    (history, played_at)
}

fn remote_played_at(label: &str, index: usize) -> String {
    let label = label.to_lowercase();
    let days = if label.contains("today") || label.contains("hoy") {
        0
    } else if label.contains("yesterday") || label.contains("ayer") {
        1
    } else if label.contains("week") || label.contains("semana") {
        3
    } else if label.contains("month") || label.contains("mes") {
        14
    } else {
        30
    };
    (chrono::Local::now() - chrono::Duration::days(days) - chrono::Duration::seconds(index as i64))
        .format("%Y-%m-%d %H:%M:%S")
        .to_string()
}

pub fn on_history_requested() {
    log::info!("History requested");
    tokio::spawn(async move {
        if crate::api::auth::is_authenticated() {
            match crate::api::innertube::remote_history().await {
                Ok(items) => {
                    let mut history = Vec::with_capacity(items.len());
                    let mut played_at = Vec::with_capacity(items.len());
                    for (index, item) in items.into_iter().enumerate() {
                        played_at.push(remote_played_at(&item.played, index));
                        history.push(bridge::Track {
                            id: item.track.id,
                            title: item.track.title,
                            artist: item.track.artists.join(", "),
                            album: item.track.album.unwrap_or_default(),
                            duration_ms: item.track.duration_ms,
                            thumbnail: item.track.thumbnail,
                        });
                    }
                    crate::bridge::bridge::set_history_data(history, played_at);
                    return;
                }
                Err(error) => {
                    log::warn!("Could not load remote YouTube Music history: {error}");
                }
            }
        }

        let local = tokio::task::spawn_blocking(load_local_history).await;
        if let Ok((history, played_at)) = local {
            crate::bridge::bridge::set_history_data(history, played_at);
        }
    });
}

pub fn on_youtube_login_success(headers_json: &str, name: &str, avatar_url: &str) {
    log::info!("YouTube Music login success. Name: {name}");
    let config_dir = crate::config::paths::AppDirs::global().config_dir();

    if let Err(e) = crate::utils::secure_storage::save_youtube_headers(headers_json) {
        log::error!("Failed to store YouTube credentials securely: {e}");
        crate::bridge::bridge::show_notification(
            "No se pudo acceder al llavero del sistema; la sesión no fue guardada",
            "error",
        );
        return;
    }
    crate::api::endpoints::invalidate_cache();

    // Save profile
    let profile_path = config_dir.join("user_profile.json");
    let profile_data = serde_json::json!({
        "name": name,
        "avatar_url": avatar_url
    });
    if let Err(e) = crate::config::settings::write_private_file(
        &profile_path,
        profile_data.to_string().as_bytes(),
    ) {
        log::warn!("Failed to save YouTube profile: {e}");
    }

    // Notify UI
    crate::bridge::bridge::update_youtube_auth_state(true, name, avatar_url);

    // Reload home data
    let home = crate::services::home::HomeService::new();
    tokio::spawn(async move {
        home.load_home().await;
    });
}

pub fn on_youtube_logout() {
    log::info!("YouTube Music logout requested");
    let config_dir = crate::config::paths::AppDirs::global().config_dir();

    let profile_path = config_dir.join("user_profile.json");

    if let Err(e) = crate::utils::secure_storage::delete_youtube_headers() {
        log::warn!("Failed to delete YouTube credentials: {e}");
    }
    crate::api::endpoints::invalidate_cache();
    let _ = std::fs::remove_file(profile_path);

    crate::bridge::bridge::update_youtube_auth_state(false, "", "");

    // Reload home data
    let home = crate::services::home::HomeService::new();
    tokio::spawn(async move {
        home.load_home().await;
    });
}

pub fn is_youtube_authenticated() -> bool {
    let config_dir = crate::config::paths::AppDirs::global().config_dir();
    match crate::utils::secure_storage::migrate_legacy_youtube_headers(&config_dir) {
        Ok(true) => log::info!("Migrated YouTube credentials to Secret Service"),
        Ok(false) => {}
        Err(e) => log::warn!("Could not migrate YouTube credentials: {e}"),
    }
    crate::utils::secure_storage::load_youtube_headers()
        .ok()
        .flatten()
        .is_some()
}

pub fn on_check_for_updates_requested() {
    log::info!("Update check requested from UI");
    tokio::spawn(async {
        if let Some(release) = crate::services::updater::check_for_updates().await {
            crate::bridge::bridge::set_update_available(
                &release.version,
                &release.notes,
                &release.url,
                &release.asset_url,
                &release.asset_name,
                release.asset_size as i64,
            );
        } else {
            crate::bridge::bridge::set_no_update_available();
        }
    });
}

pub fn on_download_update_requested(asset_url: &str, asset_name: &str) {
    let url = asset_url.to_string();
    let name = asset_name.to_string();
    log::info!("Download update requested: {name}");
    tokio::spawn(async move {
        let path = crate::services::updater::download_update_package(&url, &name, |pct, msg| {
            crate::bridge::bridge::set_update_download_progress(pct, msg);
        })
        .await;

        if let Some(p) = path {
            crate::bridge::bridge::set_update_download_finished(&p.to_string_lossy());
        } else {
            crate::bridge::bridge::set_update_download_failed(
                "Error al descargar la actualización.",
            );
        }
    });
}

pub fn on_validate_sudo_password(password: &str) -> bool {
    crate::services::updater::validate_sudo_password(password)
}

pub fn on_install_update_requested(package_path: &str, password: &str) {
    let package_path = package_path.to_string();
    let pwd = if password.is_empty() {
        None
    } else {
        Some(password.to_string())
    };
    log::info!("Install update requested: {package_path}");

    tokio::spawn(async move {
        let path = std::path::Path::new(&package_path);
        let success = crate::services::updater::install_update_async(path, pwd).await;
        crate::bridge::bridge::set_update_install_finished(success);
    });
}

pub fn get_app_version() -> String {
    crate::VERSION.to_string()
}

pub fn doremi_tr(key: &str) -> String {
    crate::utils::i18n::tr(key)
}

pub fn get_storage_sizes() -> Vec<f64> {
    crate::utils::storage::get_storage_sizes()
}

pub fn clear_cache() {
    crate::utils::storage::clear_cache();
}

pub fn clear_downloads() {
    crate::utils::storage::clear_downloads();
}

pub fn export_backup(zip_path: &str) -> bool {
    let path = std::path::Path::new(zip_path);
    crate::utils::backup::export_backup(path)
}

pub fn import_backup(zip_path: &str) -> bool {
    let path = std::path::Path::new(zip_path);
    crate::utils::backup::import_backup(path)
}
