// Settings — extracted from bridge.rs
use super::bridge;
use super::with_player;
use std::sync::atomic::Ordering;

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
    bridge::set_settings_region(&settings.network.region);
    bridge::set_settings_normalize(settings.player.normalize_audio);
    bridge::set_settings_crossfade(settings.player.crossfade_enabled);
    bridge::set_settings_equalizer_enabled(settings.equalizer.enabled);
    bridge::set_settings_equalizer_preset(&settings.equalizer.preset_name);
    bridge::set_settings_equalizer_values(
        settings.equalizer.preamp,
        settings.equalizer.bands.clone(),
    );
    bridge::set_settings_sleep_timer(settings.player.sleep_timer_minutes);

    bridge::set_settings_subtitle_alignment(&settings.subtitles.alignment);
    bridge::set_settings_subtitle_font_size(settings.subtitles.font_size);
    bridge::set_settings_subtitle_line_spacing(settings.subtitles.line_spacing);
    bridge::set_settings_subtitle_auto_scroll(settings.subtitles.auto_scroll);
    bridge::set_settings_subtitle_glow_effect(settings.subtitles.glow_effect);

    bridge::set_settings_download_location(&settings.downloads.location);
    bridge::set_settings_download_format(&settings.downloads.format);
    bridge::set_settings_download_quality(&settings.downloads.quality);

    crate::services::discord::set_enabled(settings.integrations.discord_rpc_enabled);
    crate::services::lastfm::set_enabled(settings.integrations.lastfm_enabled);

    bridge::set_settings_discord_rpc(settings.integrations.discord_rpc_enabled);
    bridge::set_settings_lastfm_enabled(settings.integrations.lastfm_enabled);
    bridge::set_settings_mpris_enabled(settings.integrations.mpris_enabled);
    bridge::set_settings_stop_on_close(settings.player.stop_on_close);

    if settings.integrations.mpris_enabled {
        if let Some(player) = super::PLAYER.get() {
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
            reload_discovery_feeds();
        }
        "region" => {
            settings.network.region = value.trim().to_uppercase();
            crate::api::endpoints::configure(&settings.language, &settings.network.region);
            reload_discovery_feeds();
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
                if let Some(player) = super::PLAYER.get() {
                    crate::mpris::start_mpris(player.clone());
                }
            } else {
                crate::mpris::stop_mpris();
            }
        }
        "download_location" => {
            settings.downloads.location = value.to_string();
        }
        "download_format" => {
            settings.downloads.format = value.to_string();
        }
        "download_quality" => {
            settings.downloads.quality = value.to_string();
        }
        _ => log::warn!("Unknown setting key: {key}"),
    }
    if let Err(e) = settings.save(&dirs.settings_path()) {
        log::error!("Failed to save setting '{key}': {e}");
    }
}

pub(super) fn reload_discovery_feeds() {
    tokio::spawn(async {
        let home = crate::services::home::HomeService::new();
        let trending = crate::services::trending::TrendingService::new();
        tokio::join!(home.load_home(), trending.load());
    });
}

pub fn set_connectivity_online(is_online: bool) {
    let was_online = super::IS_ONLINE.swap(is_online, Ordering::SeqCst);
    bridge::set_online_status(is_online);

    if is_online && !was_online {
        log::info!("Connectivity restored; retrying pending online loads");
        reload_discovery_feeds();
        crate::services::download::DownloadManager::refresh_downloads_ui();
        crate::services::download::DownloadManager::get_instance().resume_unfinished_downloads();
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
