use super::bridge;
use super::with_player;

pub fn handle_forwarded_args(args: Vec<String>) {
    log::info!("Received forwarded arguments: {:?}", args);
    bridge::show_main_window();
    for arg in args {
        if arg.starts_with("http://") || arg.starts_with("https://") {
            log::info!("Playing forwarded URL: {}", arg);
            if let Some(player) = super::PLAYER.get() {
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
