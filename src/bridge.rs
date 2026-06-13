use once_cell::sync::OnceCell;
use std::sync::Arc;
use crate::player::PlayerService;
use crate::services::search::SearchService;

static PLAYER: OnceCell<Arc<PlayerService>> = OnceCell::new();
static SEARCH: OnceCell<SearchService> = OnceCell::new();

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
        fn on_search_submitted(query: &str);
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
        fn on_lastfm_auth_requested(api_key: &str, api_secret: &str, username: &str, password: &str);
        fn on_lastfm_disconnect_requested();
        fn on_queue_item_clicked(index: i32);
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
        fn create_main_window(app_name: &str, theme_mode: &str,
                              accent_color: &str, font_size: i32);
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
        fn apply_settings_to_ui();
        fn set_settings_theme(mode: &str);
        fn set_settings_accent(color: &str);
        fn set_settings_font_size(size: i32);
        fn set_settings_language(lang: &str);
        fn set_settings_normalize(on: bool);
        fn set_settings_crossfade(on: bool);
        fn set_settings_equalizer_enabled(on: bool);
        fn set_settings_equalizer_preset(preset: &str);
        fn set_settings_sleep_timer(minutes: i32);
        fn set_settings_discord_rpc(on: bool);
        fn set_settings_lastfm_enabled(on: bool);
        fn set_settings_lastfm_session(authenticated: bool, username: &str, api_key: &str, api_secret: &str);
        fn set_track_lyrics(plain: &str, synced: &str);
        fn set_settings_subtitle_alignment(align: &str);
        fn set_settings_subtitle_font_size(size: i32);
        fn set_settings_subtitle_line_spacing(spacing: f64);
        fn set_settings_subtitle_auto_scroll(on: bool);
        fn set_settings_subtitle_glow_effect(on: bool);

        // Data service functions
        fn set_search_results(songs: Vec<Track>, artists: Vec<Artist>, albums: Vec<Album>);
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
        fn set_update_available(version: &str, notes: &str, url: &str, asset_url: &str, asset_name: &str, asset_size: i64);
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

pub fn on_search_submitted(query: &str) {
    log::info!("Search: {query}");
    // Record search history
    let _ = crate::db::repo::SearchHistoryRepo::record(query, "all");
    if let Some(search) = SEARCH.get() {
        let results = search.search(query);
        search.push_to_ui(&results);
    }
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
    use crate::db::repo::*;
    let Some(tab) = LibraryTab::from_key(tab_key) else {
        log::warn!("Ignoring unknown library tab key: {tab_key}");
        return;
    };

    match tab {
        LibraryTab::Songs => {
            if let Ok(tracks) = FavoritesRepo::all_tracks() {
                let songs: Vec<crate::bridge::bridge::Track> = tracks.iter()
                    .map(|t| crate::bridge::bridge::Track {
                        id: t.id.clone(),
                        title: t.title.clone(),
                        artist: t.artist.clone(),
                        album: t.album.clone(),
                        duration_ms: t.duration_ms,
                        thumbnail: t.thumbnail.clone(),
                    }).collect();
                crate::bridge::bridge::set_library_songs(songs);
            }
        }
        LibraryTab::Albums => {
            if let Ok(albums) = FavoritesRepo::all_albums() {
                let a_list: Vec<crate::bridge::bridge::Album> = albums.iter()
                    .map(|a| crate::bridge::bridge::Album {
                        id: a.id.clone(),
                        title: a.title.clone(),
                        artist: a.artist.clone(),
                        year: a.year.map(|y| y.to_string()).unwrap_or_default(),
                        thumbnail: a.thumbnail.clone(),
                        track_count: 0,
                    }).collect();
                crate::bridge::bridge::set_library_albums(a_list);
            }
        }
        LibraryTab::Artists => {
            if let Ok(artists) = FavoritesRepo::all_artists() {
                let art_list: Vec<crate::bridge::bridge::Artist> = artists.iter()
                    .map(|a| crate::bridge::bridge::Artist {
                        id: a.id.clone(),
                        name: a.name.clone(),
                        thumbnail: a.thumbnail.clone(),
                        description: String::new(),
                        subscribers: String::new(),
                    }).collect();
                crate::bridge::bridge::set_library_artists(art_list);
            }
        }
        LibraryTab::Playlists => {
            if let Ok(playlists) = PlaylistRepo::all() {
                let p_list: Vec<crate::bridge::bridge::Playlist> = playlists.iter()
                    .map(|p| {
                        let count = PlaylistRepo::tracks(&p.id).ok().map(|t| t.len() as i32).unwrap_or(0);
                        crate::bridge::bridge::Playlist {
                            id: p.id.clone(),
                            name: p.name.clone(),
                            description: p.description.clone(),
                            thumbnail: p.artwork.clone(),
                            track_count: count,
                        }
                    }).collect();
                crate::bridge::bridge::set_library_playlists(p_list);
            }
        }
    }
}

#[cfg(test)]
mod contract_tests {
    use super::LibraryTab;

    #[test]
    fn library_tabs_use_stable_non_localized_keys() {
        assert_eq!(LibraryTab::from_key("songs"), Some(LibraryTab::Songs));
        assert_eq!(LibraryTab::from_key("albums"), Some(LibraryTab::Albums));
        assert_eq!(LibraryTab::from_key("artists"), Some(LibraryTab::Artists));
        assert_eq!(LibraryTab::from_key("playlists"), Some(LibraryTab::Playlists));
        assert_eq!(LibraryTab::from_key("Canciones"), None);
        assert_eq!(LibraryTab::from_key("Songs"), None);
        assert_eq!(LibraryTab::from_key(""), None);
    }
}

pub fn on_remove_favorite(track_id: &str) {
    log::info!("Remove favorite: {track_id}");
    if let Err(e) = crate::db::repo::FavoritesRepo::remove_track(track_id) {
        log::error!("Failed to remove favorite: {e}");
    }
}

pub fn on_add_favorite(track: bridge::Track) {
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

pub fn on_download_requested(track: bridge::Track) {
    log::info!("Download requested: {} — {}", track.title, track.artist);
    if !track.id.is_empty() {
        crate::services::download::DownloadManager::get_instance().add_download(
            &track.id,
            &track.title,
            &track.artist,
            ""
        );
    } else {
        log::warn!("Cannot download: no track id provided for {} — {}", track.title, track.artist);
    }
}

pub fn on_search_item_clicked(track: bridge::Track) {
    log::info!("Search item clicked: {} — {} (id: {})", track.title, track.artist, track.id);
    with_player(|p| p.play_track_dto(track));
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
            let enabled = settings.equalizer.enabled;
            let preamp = settings.equalizer.preamp;
            let bands = settings.equalizer.bands.clone();
            let preset = settings.equalizer.preset_name.clone();
            with_player(|p| p.apply_equalizer(enabled, preamp, &bands, &preset));
        }
        "equalizer_preamp" => {
            if let Ok(v) = value.parse::<f64>() {
                settings.equalizer.preamp = v;
                let enabled = settings.equalizer.enabled;
                let preamp = settings.equalizer.preamp;
                let bands = settings.equalizer.bands.clone();
                let preset = settings.equalizer.preset_name.clone();
                with_player(|p| p.apply_equalizer(enabled, preamp, &bands, &preset));
            }
        }
        "equalizer_bands" => {
            let parsed_bands: Vec<f64> = value.split(',')
                .filter_map(|s| s.parse::<f64>().ok())
                .collect();
            if !parsed_bands.is_empty() {
                settings.equalizer.bands = parsed_bands;
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
        let auth_result = crate::services::lastfm::authenticate(
            &api_key,
            &api_secret,
            &username,
            &password,
        ).await;
        password.zeroize();

        match auth_result {
            Ok(session_key) => {
                log::info!("Last.fm auth successful");
                let dirs = crate::config::paths::AppDirs::global();
                let mut settings = crate::config::settings::AppSettings::load(&dirs.settings_path());

                let credentials = crate::utils::secure_storage::LastFmCredentials {
                    api_key: api_key.clone(),
                    api_secret: api_secret.to_string(),
                    session_key,
                };
                if let Err(e) = crate::utils::secure_storage::save_lastfm_credentials(&credentials) {
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
    log::info!("Queue item clicked: jump to index {index}");
    with_player(|p| p.play_index(index as usize));
}

pub fn on_stats_requested() {
    log::info!("Stats requested");

    let total_time_ms = crate::db::with_db(|conn| {
        conn.query_row("SELECT SUM(duration_ms) FROM recently_played", [], |r| {
            let val: Option<i64> = r.get(0)?;
            Ok(val.unwrap_or(0))
        })
    }).unwrap_or(0);

    let total_plays = crate::db::with_db(|conn| {
        conn.query_row("SELECT COUNT(*) FROM recently_played", [], |r| {
            let val: i32 = r.get(0)?;
            Ok(val)
        })
    }).unwrap_or(0);

    let unique_artists = crate::db::with_db(|conn| {
        conn.query_row("SELECT COUNT(DISTINCT artist) FROM recently_played", [], |r| {
            let val: i32 = r.get(0)?;
            Ok(val)
        })
    }).unwrap_or(0);

    // Weekly activity: last 7 days daily counts
    let mut weekly_activity = vec![0; 7];
    if let Ok(results) = crate::db::with_db(|conn| {
        let mut stmt = conn.prepare(
            "SELECT strftime('%Y-%m-%d', played_at) as play_day, COUNT(*) as cnt
             FROM recently_played
             WHERE played_at >= datetime('now', '-7 days')
             GROUP BY play_day
             ORDER BY play_day DESC
             LIMIT 7"
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
             LIMIT 5"
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

    let stats = crate::bridge::bridge::StatsData {
        total_play_time: time_str,
        total_plays,
        unique_artists,
        weekly_activity,
        top_tracks,
    };

    crate::bridge::bridge::set_stats_data(stats);
}

pub fn on_history_requested() {
    log::info!("History requested");

    let mut history = Vec::new();
    let mut played_at = Vec::new();

    if let Ok(results) = crate::db::with_db(|conn| {
        let mut stmt = conn.prepare(
            "SELECT track_id, title, artist, duration_ms, thumbnail, played_at
             FROM recently_played
             ORDER BY played_at DESC
             LIMIT 50"
        )?;
        let rows = stmt.query_map([], |r| {
            let track_id: String = r.get(0)?;
            let title: String = r.get(1)?;
            let artist: String = r.get(2)?;
            let duration_ms: i64 = r.get(3)?;
            let thumbnail: String = r.get(4)?;
            let played_at_str: String = r.get(5)?;
            Ok((track_id, title, artist, duration_ms, thumbnail, played_at_str))
        })?;
        let list: Result<Vec<_>, rusqlite::Error> = rows.collect();
        list
    }) {
        for row in results {
            let (track_id, title, artist, duration_ms, mut thumbnail, played_at_str) = row;
            if thumbnail.is_empty() {
                thumbnail = crate::bridge::bridge::get_or_create_thumbnail(&title, 0);
            }
            history.push(crate::bridge::bridge::Track {
                id: track_id,
                title,
                artist,
                album: String::new(),
                duration_ms,
                thumbnail,
            });
            played_at.push(played_at_str);
        }
    }

    crate::bridge::bridge::set_history_data(history, played_at);
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
    home.load_home();
}

pub fn on_youtube_logout() {
    log::info!("YouTube Music logout requested");
    let config_dir = crate::config::paths::AppDirs::global().config_dir();
    
    let profile_path = config_dir.join("user_profile.json");

    if let Err(e) = crate::utils::secure_storage::delete_youtube_headers() {
        log::warn!("Failed to delete YouTube credentials: {e}");
    }
    let _ = std::fs::remove_file(profile_path);

    crate::bridge::bridge::update_youtube_auth_state(false, "", "");

    // Reload home data
    let home = crate::services::home::HomeService::new();
    home.load_home();
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
        }).await;

        if let Some(p) = path {
            crate::bridge::bridge::set_update_download_finished(&p.to_string_lossy());
        } else {
            crate::bridge::bridge::set_update_download_failed("Error al descargar la actualización.");
        }
    });
}

pub fn on_validate_sudo_password(password: &str) -> bool {
    crate::services::updater::validate_sudo_password(password)
}

pub fn on_install_update_requested(package_path: &str, password: &str) {
    let package_path = package_path.to_string();
    let pwd = if password.is_empty() { None } else { Some(password.to_string()) };
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
