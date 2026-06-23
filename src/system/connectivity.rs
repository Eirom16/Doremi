use std::sync::atomic::{AtomicBool, Ordering};
use std::sync::Arc;
use std::time::Duration;
use tokio::time::sleep;

pub struct ConnectivityMonitor;

impl ConnectivityMonitor {
    pub fn start() -> Arc<AtomicBool> {
        let is_online = Arc::new(AtomicBool::new(true)); // Assume online by default
        let is_online_clone = is_online.clone();

        tokio::spawn(async move {
            let mut consecutive_failures = 0;
            let mut consecutive_successes = 0;
            let check_interval = Duration::from_secs(5);
            let timeout_duration = Duration::from_secs(2);

            if !Self::check_connection(timeout_duration).await {
                log::warn!("Initial connectivity check failed. Starting offline.");
                consecutive_failures = 2;
                is_online_clone.store(false, Ordering::SeqCst);
                crate::bridge::set_connectivity_online(false);
            }

            loop {
                let check_result = Self::check_connection(timeout_duration).await;
                let current_state = is_online_clone.load(Ordering::SeqCst);

                if check_result {
                    consecutive_successes += 1;
                    consecutive_failures = 0;

                    // Require 1 success to go online (fast recovery)
                    if !current_state && consecutive_successes >= 1 {
                        log::info!("Connectivity restored. Going online.");
                        is_online_clone.store(true, Ordering::SeqCst);
                        crate::bridge::set_connectivity_online(true);
                    }
                } else {
                    consecutive_failures += 1;
                    consecutive_successes = 0;

                    // Require 2 consecutive failures (10 seconds) to go offline (debounce)
                    if current_state && consecutive_failures >= 2 {
                        log::warn!("Connectivity lost. Going offline.");
                        is_online_clone.store(false, Ordering::SeqCst);
                        crate::bridge::set_connectivity_online(false);
                    }
                }

                sleep(check_interval).await;
            }
        });

        is_online
    }

    async fn check_connection(timeout: Duration) -> bool {
        // Try connecting to reliable targets
        let targets = vec!["music.youtube.com:443", "1.1.1.1:53", "8.8.8.8:53"];

        for target in targets {
            if let Ok(Ok(_)) =
                tokio::time::timeout(timeout, tokio::net::TcpStream::connect(target)).await
            {
                return true;
            }
        }
        false
    }
}
