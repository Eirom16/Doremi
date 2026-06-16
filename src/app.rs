use std::sync::Arc;

use crate::bridge;
use crate::bridge::bridge::*;
use crate::config::paths::AppDirs;
use crate::config::settings::AppSettings;
use crate::db::repo::{FavoritesRepo, PlaylistRepo, SearchHistoryRepo};
use crate::db::Database;
use crate::player::vlc_check;
use crate::player::PlayerService;
use crate::services::home::HomeService;
use crate::services::search::SearchService;
use crate::services::trending::TrendingService;

pub struct DoremiApp {
    settings: AppSettings,
}

impl DoremiApp {
    pub fn new() -> Self {
        AppDirs::setup();
        let dirs = AppDirs::global();

        vlc_check::setup_vlc_env();
        let vlc_ok = vlc_check::check_vlc_available();
        if !vlc_ok {
            log::warn!("VLC not found. Audio playback requires VLC.");
        } else {
            log::info!("VLC detected");
        }

        // Initialize database
        if let Err(e) = Database::init() {
            log::error!("Failed to initialize database: {e}");
        } else {
            log::info!("Database initialized");
        }

        // Run Pyrolist -> Doremi migration
        match crate::utils::migration::run_migration() {
            Ok(Some(summary)) => {
                log::info!(
                    "Migration from Pyrolist completed successfully: {:?}",
                    summary
                );
            }
            Ok(None) => {
                log::info!("No legacy Pyrolist data found for migration");
            }
            Err(e) => {
                log::error!("Migration from Pyrolist failed: {}", e);
            }
        }

        let settings = AppSettings::load(&dirs.settings_path());
        log::info!("Settings loaded from: {:?}", dirs.settings_path());

        Self { settings }
    }

    pub fn run(self) {
        log::info!("Doremi v{} initialized", crate::VERSION);
        log::info!("Theme: {}", self.settings.appearance.theme_mode);
        log::info!("Language: {}", self.settings.language);
        crate::api::endpoints::configure(&self.settings.language, &self.settings.network.region);

        if let Err(error) = bridge::verify_contract() {
            log::error!("Cannot start Doremi: {error}");
            return;
        }

        // Initialize player
        let player = Arc::new(PlayerService::new_with_preload(
            self.settings.network.preload_next,
        ));
        let restored_queue =
            self.settings.player.resume_on_startup && player.restore_queue_session();
        bridge::init_player(player.clone());
        log::info!("Player service initialized");

        // Start MPRIS D-Bus server
        crate::mpris::spawn_mpris(player.clone());

        // Initialize services
        bridge::init_search(SearchService::new());
        log::info!("Search service initialized");

        let accent = &self.settings.appearance.accent_color;
        let theme = &self.settings.appearance.theme_mode;
        let font_size = self.settings.appearance.font_size;

        let args: Vec<String> = std::env::args().skip(1).collect();
        let mut startup_url = None;
        for arg in &args {
            if arg.starts_with("http://") || arg.starts_with("https://") {
                startup_url = Some(arg.clone());
                break;
            }
        }

        create_main_window("Doremi", theme, accent, font_size);
        apply_theme(theme, accent);
        set_window_title("Doremi");
        show_main_window();
        if restored_queue {
            player.refresh_queue_ui();
        }
        if let Some(url) = startup_url {
            player.clear_queue();
            player.play_url(&url);
        }

        // Check auth on startup
        if crate::bridge::is_youtube_authenticated() {
            let config_dir = AppDirs::global().config_dir();
            let profile_path = config_dir.join("user_profile.json");
            let mut name = "YouTube Music".to_string();
            let mut avatar = "".to_string();
            if profile_path.exists() {
                if let Ok(content) = std::fs::read_to_string(profile_path) {
                    if let Ok(val) = serde_json::from_str::<serde_json::Value>(&content) {
                        if let Some(n) = val.get("name").and_then(|v| v.as_str()) {
                            name = n.to_string();
                        }
                        if let Some(a) = val.get("avatar_url").and_then(|v| v.as_str()) {
                            avatar = a.to_string();
                        }
                    }
                }
            }
            update_youtube_auth_state(true, &name, &avatar);
            navigate_to("home");
        } else {
            update_youtube_auth_state(false, "", "");
            navigate_to("welcome");
        }

        // Load settings into UI
        crate::bridge::apply_settings_impl();

        // Load home, library and history data in background task
        tokio::spawn(async move {
            let home = HomeService::new();
            home.load_home().await;
            TrendingService::new().load().await;

            tokio::task::spawn_blocking(move || {
                // Load recently played from DB → add "Seguir escuchando" section
                if let Ok(recent) = crate::db::repo::RecentlyPlayedRepo::recent(12) {
                    if !recent.is_empty() {
                        let items = recent
                            .iter()
                            .map(|track| crate::bridge::bridge::HomeCard {
                                id: track.track_id.clone(),
                                title: track.title.clone(),
                                subtitle: track.artist.clone(),
                                thumbnail: track.thumbnail.clone(),
                                item_type: "song".to_string(),
                            })
                            .collect::<Vec<_>>();
                        add_home_section("Seguir escuchando", items);
                    }
                }

                // Load library data from DB
                // Songs (favorites)
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
                    set_library_songs(songs);
                }
                // Playlists
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
                    crate::bridge::bridge::set_context_playlists(
                        p_list.iter().map(|p| crate::bridge::bridge::Playlist {
                            id: p.id.clone(),
                            name: p.name.clone(),
                            description: p.description.clone(),
                            thumbnail: p.thumbnail.clone(),
                            track_count: p.track_count,
                        }).collect()
                    );
                    set_library_playlists(p_list);
                }
                // Albums
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
                    set_library_albums(a_list);
                }
                // Artists
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
                    set_library_artists(art_list);
                }

                // Load search history
                if let Ok(history) = SearchHistoryRepo::recent(10) {
                    let queries: Vec<String> = history.iter().map(|h| h.query.clone()).collect();
                    set_search_history(queries);
                }
            })
            .await
            .ok();
        });

        // Load downloads data from database
        crate::services::download::DownloadManager::refresh_downloads_ui();

        // Trigger update check after a short delay (3 seconds)
        tokio::spawn(async {
            tokio::time::sleep(std::time::Duration::from_secs(3)).await;
            crate::bridge::on_check_for_updates_requested();
        });

        run_event_loop();

        // Event loop ended — final cleanup
        crate::bridge::on_app_quit();
        log::info!("Doremi shutdown complete");
    }
}

impl Default for DoremiApp {
    fn default() -> Self {
        Self::new()
    }
}
