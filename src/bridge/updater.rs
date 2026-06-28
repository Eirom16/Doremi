use super::bridge;

pub fn on_check_for_updates_requested() {
    log::info!("Update check requested from UI");
    if !super::is_connectivity_online() {
        bridge::show_notification(
            "Sin conexión: no se puede buscar actualizaciones.",
            "warning",
        );
        return;
    }

    tokio::spawn(async {
        if let Some(release) = crate::services::updater::check_for_updates().await {
            bridge::set_update_available(
                &release.version,
                &release.notes,
                &release.url,
                &release.asset_url,
                &release.asset_name,
                release.asset_size as i64,
            );
        } else {
            bridge::set_no_update_available();
        }
    });
}

pub fn on_download_update_requested(asset_url: &str, asset_name: &str) {
    let url = asset_url.to_string();
    let name = asset_name.to_string();
    log::info!("Download update requested: {name}");
    tokio::spawn(async move {
        let path = crate::services::updater::download_update_package(&url, &name, |pct, msg| {
            bridge::set_update_download_progress(pct, msg);
        })
        .await;

        if let Some(p) = path {
            bridge::set_update_download_finished(&p.to_string_lossy());
        } else {
            bridge::set_update_download_failed(
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
        bridge::set_update_install_finished(success);
    });
}
