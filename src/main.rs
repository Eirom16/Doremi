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

    // 1. Single Instance Check & Port Forwarding
    let args: Vec<String> = std::env::args().skip(1).collect();
    let single_instance_disabled = std::env::var("DOREMI_DISABLE_SINGLE_INSTANCE")
        .map(|value| value == "1" || value.eq_ignore_ascii_case("true"))
        .unwrap_or(false);
    if !single_instance_disabled {
        let instance_port = std::env::var("DOREMI_INSTANCE_PORT")
            .ok()
            .and_then(|value| value.parse::<u16>().ok())
            .unwrap_or(18420);
        let instance_addr = format!("127.0.0.1:{instance_port}");
        match std::net::TcpListener::bind(&instance_addr) {
            Ok(listener) => {
                // Primary instance. Spawn thread to listen for forwarded arguments.
                std::thread::spawn(move || {
                    use std::io::Read;
                    for mut stream in listener.incoming().flatten() {
                        let mut buffer = Vec::new();
                        stream
                            .set_read_timeout(Some(std::time::Duration::from_secs(2)))
                            .ok();
                        if stream.read_to_end(&mut buffer).is_ok() {
                            if let Ok(text) = String::from_utf8(buffer) {
                                let received_args: Vec<String> = text
                                    .split('\n')
                                    .map(|s| s.to_string())
                                    .filter(|s| !s.is_empty())
                                    .collect();
                                doremi_core::bridge::handle_forwarded_args(received_args);
                            }
                        }
                    }
                });
            }
            Err(error) => {
                // Secondary instance. Forward arguments and exit.
                use std::io::Write;
                if let Ok(mut stream) = std::net::TcpStream::connect(&instance_addr) {
                    let payload = args.join("\n");
                    let _ = stream.write_all(payload.as_bytes());
                }
                log::info!(
                    "Another instance is running or single-instance bind failed ({error}). Forwarded arguments to primary instance. Exiting."
                );
                return;
            }
        }
    } else {
        log::warn!("Single-instance guard disabled by DOREMI_DISABLE_SINGLE_INSTANCE");
    }

    // Global Panic Hook for Clean Shutdown
    std::panic::set_hook(Box::new(|panic_info| {
        let panic_msg = if let Some(s) = panic_info.payload().downcast_ref::<&str>() {
            *s
        } else if let Some(s) = panic_info.payload().downcast_ref::<String>() {
            s.as_str()
        } else {
            "Unknown panic payload"
        };
        let redacted_msg = doremi_core::utils::security::redact_secrets(panic_msg);
        let location = panic_info
            .location()
            .map(|l| format!("at {}:{}", l.file(), l.line()))
            .unwrap_or_default();
        log::error!("CRITICAL PANIC: {redacted_msg} {location}");

        log::info!("Executing emergency shutdown...");

        // 1. Disconnect Discord RPC
        doremi_core::services::discord::disconnect();

        // 2. Stop VLC playback, save volume and release database connection
        doremi_core::bridge::on_app_quit();
    }));

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
