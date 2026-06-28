use super::bridge;

pub fn on_youtube_login_success(headers_json: &str, name: &str, avatar_url: &str) {
    log::info!("YouTube Music login success. Name: {name}");
    let config_dir = crate::config::paths::AppDirs::global().config_dir();

    if let Err(e) = crate::utils::secure_storage::save_youtube_headers(headers_json) {
        log::error!("Failed to store YouTube credentials securely: {e}");
        bridge::show_notification(
            "No se pudo acceder al llavero del sistema; la sesión no fue guardada",
            "error",
        );
        return;
    }
    crate::api::endpoints::invalidate_cache();
    crate::api::auth::reset_session_revoked();
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
    bridge::update_youtube_auth_state(true, name, avatar_url);

    super::reload_discovery_feeds();
}

pub fn on_youtube_session_refresh(headers_json: &str) {
    let headers_json = headers_json.to_string();
    tokio::spawn(async move {
        let saved = tokio::task::spawn_blocking(move || {
            if !crate::api::auth::is_authenticated() {
                return Ok(false);
            }
            crate::utils::secure_storage::save_youtube_headers(&headers_json).map(|_| true)
        })
        .await;
        match saved {
            Ok(Ok(true)) => {
                crate::api::auth::reset_session_revoked();
                crate::api::endpoints::invalidate_cache();
                log::debug!("Refreshed YouTube Music session from WebEngine cookies");
            }
            Ok(Ok(false)) => {}
            Ok(Err(error)) => log::warn!("Could not refresh YouTube Music session: {error}"),
            Err(error) => log::warn!("YouTube Music session refresh task failed: {error}"),
        }
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
    crate::api::auth::reset_session_revoked();
    if let Err(e) = std::fs::remove_file(&profile_path) {
        if e.kind() != std::io::ErrorKind::NotFound {
            log::warn!("Failed to remove user profile file: {e}");
        }
    }

    bridge::update_youtube_auth_state(false, "", "");

    super::reload_discovery_feeds();
}

/// Invoked from the API transport layer when an authenticated request is
/// rejected with 401/403. Credentials have already been cleared by
/// `auth::handle_session_revoked`; here we tear down the rest of the session
/// state, inform the user and reload the now-anonymous home feed.
pub fn on_session_revoked() {
    let config_dir = crate::config::paths::AppDirs::global().config_dir();
    if let Err(e) = std::fs::remove_file(config_dir.join("user_profile.json")) {
        if e.kind() != std::io::ErrorKind::NotFound {
            log::warn!("Failed to remove user profile file: {e}");
        }
    }

    bridge::update_youtube_auth_state(false, "", "");
    bridge::show_notification(
        "Tu sesión de YouTube Music expiró. Continuando en modo anónimo.",
        "warning",
    );

    super::reload_discovery_feeds();
}

pub fn is_youtube_authenticated() -> bool {
    let config_dir = crate::config::paths::AppDirs::global().config_dir();
    match crate::utils::secure_storage::migrate_legacy_youtube_headers(&config_dir) {
        Ok(true) => log::info!("Migrated YouTube credentials to Secret Service"),
        Ok(false) => {}
        Err(e) => log::warn!("Could not migrate YouTube credentials: {e}"),
    }
    crate::api::auth::is_authenticated()
}
