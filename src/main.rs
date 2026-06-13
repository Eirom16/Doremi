use doremi_core::app::DoremiApp;
use doremi_core::utils::i18n;

fn main() {
    env_logger::Builder::from_env(env_logger::Env::default().default_filter_or("info"))
        .format(|buf, record| {
            use std::io::Write;
            let message = record.args().to_string();
            let redacted = doremi_core::utils::security::redact_secrets(&message);
            writeln!(
                buf,
                "[{}] {} - {}",
                record.level(),
                record.target(),
                redacted
            )
        })
        .init();

    i18n::set_language("es");
    log::info!("Starting Doremi v{}", doremi_core::VERSION);

    let rt = tokio::runtime::Runtime::new().expect("Failed to create Tokio runtime");
    let _guard = rt.enter();

    // Handle Ctrl+C — forward to Qt's quit which triggers cleanup
    let (tx, rx) = std::sync::mpsc::channel();
    ctrlc::set_handler(move || {
        let _ = tx.send(());
    })
    .ok();

    let app = DoremiApp::new();
    app.run();

    // If Ctrl+C was pressed before Qt event loop started
    if rx.try_recv().is_ok() {
        log::info!("Ctrl+C received, shutting down");
    }
}
