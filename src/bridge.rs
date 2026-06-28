// Bridge Rust/C++ — punto de entrada del contrato de interoperabilidad.
//
// NOTA ARQUITECTURAL: El macro `#[cxx::bridge]` no puede dividirse entre
// múltiples archivos (limitación técnica de cxx). Ver ADR-001 en
// docs/architecture/ADR-001-bridge-design.md para la justificación completa.
//
// ÍNDICE DE SECCIONES (orden aproximado de aparición):
//   1. Statics globales y utilidades (PLAYER, SEARCH, IS_ONLINE)
//   2. #[cxx::bridge] — tipos compartidos (Track, Album, Artist, …) y firmas
//   3. Player controls      → on_play_pause_triggered, on_next, on_seek, …
//   4. Search               → on_search_submitted, suggestions, history
//   5. Home / Trending      → on_home_retry_requested, load_more
//   6. Browse / Detail      → on_album_requested, on_artist_requested, …
//   7. System               → on_app_quit, on_window_close_requested
//   8. Library              → favorites, playlists, tabs, filter, search
//   9. Downloads            → on_download_requested, batch, cancel, delete
//  10. Playback actions     → on_play_all, on_queue_item_*, on_add_to_queue_*
//  11. Auth / Session       → on_youtube_login_success, logout, refresh
//  12. Settings             → on_setting_changed, apply_settings_impl
//  13. Stats / History      → on_stats_requested, on_history_requested
//  14. Last.fm / Discord    → on_lastfm_auth_requested, scrobble
//  15. Updater              → on_check_for_updates_requested, download, install
//  16. UI Test              → setup_ui_test_impl, should_run_online_startup_work

use crate::player::PlayerService;
use crate::services::search::SearchService;
use once_cell::sync::OnceCell;
use std::sync::atomic::{AtomicBool, AtomicI32, Ordering};
use std::sync::Arc;

static PLAYER: OnceCell<Arc<PlayerService>> = OnceCell::new();
static SEARCH: OnceCell<SearchService> = OnceCell::new();
static IS_ONLINE: AtomicBool = AtomicBool::new(true);
static FILTER_SOURCE: AtomicI32 = AtomicI32::new(0); // Default: 0 (All)

pub const CONTRACT_MAJOR: u16 = 1;
pub const CONTRACT_MINOR: u16 = 3;

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
    match PLAYER.get() {
        Some(p) => f(p.as_ref()),
        None => {
            log::error!("Player service is not initialized!");
            R::default()
        }
    }
}

#[cxx::bridge]
pub mod bridge {
    #[derive(Clone)]
    struct Track {
        id: String,
        title: String,
        artist: String,
        album: String,
        duration_ms: i64,
        thumbnail: String,
    }

    #[derive(Clone)]
    struct Playlist {
        id: String,
        name: String,
        description: String,
        thumbnail: String,
        track_count: i32,
        owner: String,
        privacy: String,
        editable: bool,
    }

    #[derive(Clone)]
    struct Album {
        id: String,
        title: String,
        artist: String,
        year: String,
        thumbnail: String,
        track_count: i32,
        artist_id: String,
    }

    #[derive(Clone)]
    struct Artist {
        id: String,
        name: String,
        thumbnail: String,
        description: String,
        subscribers: String,
    }

    #[derive(Clone)]
    struct TopResult {
        id: String,
        title: String,
        subtitle: String,
        thumbnail: String,
        item_type: String,
    }

    #[derive(Clone)]
    struct Show {
        id: String,
        title: String,
        author: String,
        description: String,
        thumbnail: String,
        episode_count: i32,
    }

    #[derive(Clone)]
    struct Episode {
        id: String,
        title: String,
        show: String,
        show_id: String,
        description: String,
        thumbnail: String,
        duration_ms: i64,
    }

    #[derive(Clone)]
    struct HomeCard {
        id: String,
        title: String,
        subtitle: String,
        thumbnail: String,
        item_type: String,
    }

    #[derive(Clone)]
    struct StatsData {
        total_play_time: String,
        total_plays: i32,
        unique_artists: i32,
        weekly_activity: Vec<i32>,
        top_tracks: Vec<Track>,
        top_tracks_plays: Vec<i32>,
    }

    #[derive(Clone)]
    struct DownloadItem {
        video_id: String,
        title: String,
        artist: String,
        album: String,
        thumbnail_url: String,
        parent_playlist_id: String,
        parent_playlist_title: String,
        parent_playlist_thumbnail_url: String,
        status: String,
        progress: f64,
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
        fn on_search_history_delete(query: &str);
        fn on_search_history_clear();
        fn on_home_retry_requested();
        fn on_home_load_more_requested();
        fn on_trending_retry_requested();
        fn on_album_requested(browse_id: &str);
        fn on_artist_requested(browse_id: &str);
        fn on_playlist_requested(playlist_id: &str);
        fn on_playlist_requested_with_context(
            playlist_id: &str,
            title: &str,
            subtitle: &str,
            thumbnail: &str,
        );
        fn on_show_requested(browse_id: &str);
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
        fn on_add_favorite_album(album: Album);
        fn on_remove_favorite_album(album_id: &str);
        fn on_add_favorite_artist(artist: Artist);
        fn on_remove_favorite_artist(artist_id: &str);
        fn on_add_favorite_show(show: Show);
        fn on_remove_favorite_show(show_id: &str);
        fn on_add_to_playlist(track: Track, playlist_id: &str);
        fn on_create_playlist(name: &str, description: &str, privacy: &str);
        fn on_rename_playlist(playlist_id: &str, name: &str);
        fn on_delete_playlist(playlist_id: &str);
        fn on_remove_playlist_track(playlist_id: &str, track_id: &str);
        fn on_move_playlist_track(playlist_id: &str, from: i32, to: i32);
        fn on_download_requested(track: Track);
        fn on_download_requested_with_parent(
            track: Track,
            parent_id: &str,
            parent_title: &str,
            parent_thumbnail: &str,
        );
        fn on_downloads_requested();
        fn on_batch_download_requested(
            tracks: Vec<Track>,
            parent_id: &str,
            parent_title: &str,
            parent_thumbnail: &str,
        );
        fn on_download_cancel_requested(video_id: &str);
        fn on_delete_download(video_id: &str, delete_file: bool);
        fn on_add_to_queue_next(track: Track);
        fn on_add_to_queue_end(track: Track);
        fn on_play_all(tracks: Vec<Track>, shuffle: bool);
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
        fn on_stats_requested(days: i32);
        fn on_history_requested();
        fn on_clear_history();
        fn on_delete_history_item(track_id: &str, feedback_token: &str);
        fn on_youtube_login_success(headers_json: &str, name: &str, avatar_url: &str);
        fn on_youtube_session_refresh(headers_json: &str);
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
        fn on_library_search(tab: &str, query: &str, sort_by: &str);
        fn on_library_invalidate_cache(tab: &str);
        fn on_library_set_filter_source(source: i32);
        fn get_album_favorite_state(album_id: &str) -> bool;
        fn get_artist_favorite_state(artist_id: &str) -> bool;
        fn get_track_favorite_state(track_id: &str) -> bool;
        fn get_show_favorite_state(show_id: &str) -> bool;
        fn on_update_playlist_privacy(playlist_id: &str, privacy: &str);
        fn on_playlist_load_continuations(playlist_id: &str);
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
        fn set_current_track(track: Track);
        fn get_search_bar_text() -> String;
        fn set_search_bar_text(text: &str);
        fn set_window_title(title: &str);
        fn set_playing(playing: bool);
        fn run_event_loop();
        fn set_player_volume(volume: i32);
        fn set_library_songs(songs: Vec<Track>);
        fn set_library_shows(shows: Vec<Show>);
        fn set_library_playlists(playlists: Vec<Playlist>);
        fn set_context_playlists(playlists: Vec<Playlist>);
        fn set_library_albums(albums: Vec<Album>);
        fn set_library_artists(artists: Vec<Artist>);
        fn set_library_state(state: &str, message: &str);
        fn set_search_history(queries: Vec<String>);
        fn set_search_suggestions(query: &str, suggestions: Vec<String>);
        fn apply_settings_to_ui();
        fn set_prefetch_status(video_id: &str, status: &str);
        fn set_settings_theme(mode: &str);
        fn set_settings_accent(color: &str);
        fn set_settings_font_size(size: i32);
        fn set_settings_language(lang: &str);
        fn set_settings_region(region: &str);
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
        fn set_settings_download_location(location: &str);
        fn set_settings_download_format(format: &str);
        fn set_settings_download_quality(quality: &str);

        // Data service functions
        fn set_search_results(
            top_result: TopResult,
            has_top_result: bool,
            songs: Vec<Track>,
            videos: Vec<Track>,
            artists: Vec<Artist>,
            albums: Vec<Album>,
            playlists: Vec<Playlist>,
            shows: Vec<Show>,
            episodes: Vec<Episode>,
        );
        fn set_show_detail(show: Show, episodes: Vec<Episode>);
        fn add_home_section(title: &str, items: Vec<HomeCard>);
        fn clear_home_sections();
        fn set_home_state(state: &str, message: &str);
        fn set_trending_items(items: Vec<HomeCard>);
        fn set_trending_state(state: &str, message: &str);
        fn set_downloads_list(items: Vec<DownloadItem>);
        fn set_download_progress(video_id: &str, percent: f64, status: &str);
        fn set_batch_download_progress(parent_id: &str, total: i32, completed: i32, percent: f64);
        fn set_player_shuffle(on: bool);
        fn set_player_repeat(mode: i32);
        fn get_or_create_thumbnail(title: &str, variant: i32) -> String;
        fn set_dominant_colors(colors: Vec<String>);
        fn set_playback_queue(queue: Vec<Track>, current_index: i32);
        fn set_related_tracks(tracks: Vec<Track>);
        fn set_stats_data(stats: StatsData);

        fn set_history_data(
            history: Vec<Track>,
            played_at: Vec<String>,
            feedback_tokens: Vec<String>,
        );

        fn set_album_detail(album: Album, tracks: Vec<Track>);

        fn set_artist_detail(artist: Artist, tracks: Vec<Track>, albums: Vec<Album>, singles: Vec<Album>);

        fn set_playlist_detail(playlist: Playlist, tracks: Vec<Track>);
        fn set_online_status(is_online: bool);
        fn setup_ui_test(view: &str, screenshot_path: &str);
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

pub mod player;
pub use player::*;
pub mod library;
pub use library::*;
pub mod settings;
pub use settings::*;
pub mod auth;
pub use auth::*;
pub mod stats;
pub use stats::*;
pub mod search;
pub use search::*;
pub mod browse;
pub use browse::*;
pub mod downloads;
pub use downloads::*;
pub mod system;
pub use system::*;
pub mod updater;
pub use updater::*;

fn is_online() -> bool {
    IS_ONLINE.load(Ordering::SeqCst)
}

pub fn is_connectivity_online() -> bool {
    is_online()
}

pub(crate) fn should_run_online_startup_work() -> bool {
    is_connectivity_online()
}

#[cfg(test)]
mod contract_tests {
    use super::{
        is_contract_compatible, should_run_online_startup_work, versions_are_compatible,
        LibraryTab, CONTRACT_MAJOR, CONTRACT_MINOR, IS_ONLINE,
    };
    use std::sync::atomic::Ordering;
    #[test]
    fn bridge_contract_rejects_incompatible_versions() {
        assert!(is_contract_compatible(CONTRACT_MAJOR, CONTRACT_MINOR));
        assert!(is_contract_compatible(CONTRACT_MAJOR, CONTRACT_MINOR + 1));
        assert!(!is_contract_compatible(CONTRACT_MAJOR + 1, CONTRACT_MINOR));
        assert!(!versions_are_compatible(1, 2, 1, 1));
    }

    #[test]
    fn offline_startup_skips_online_work() {
        let previous = IS_ONLINE.swap(false, Ordering::SeqCst);
        assert!(!should_run_online_startup_work());
        IS_ONLINE.store(previous, Ordering::SeqCst);
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
        assert_eq!(LibraryTab::from_key("shows"), Some(LibraryTab::Shows));
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

        let _guard = crate::db::TEST_MUTEX.lock().unwrap();
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




