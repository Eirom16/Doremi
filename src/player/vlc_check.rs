use crate::tr;
use std::path::PathBuf;
use std::process::Command;

/// Check if VLC is available on the system
pub fn check_vlc_available() -> bool {
    // Try `vlc --version`
    if Command::new("vlc").arg("--version").output().is_ok() {
        return true;
    }
    // Try `cvlc --version`
    if Command::new("cvlc").arg("--version").output().is_ok() {
        return true;
    }
    // Check common VLC paths
    for path in &["/usr/bin/vlc", "/usr/local/bin/vlc", "/snap/bin/vlc"] {
        if std::path::Path::new(path).exists() {
            return true;
        }
    }
    false
}

/// Setup VLC environment variables.
///
/// # Safety note
/// `std::env::set_var` is not thread-safe in Rust edition 2024 when other threads
/// are running. This function should be called only during early initialization,
/// before any threads that read env vars are spawned. The `VLC_PLUGIN_PATH` set here
/// is read by libVLC once at `Instance::new()` time, so calling this before creating
/// the `AudioEngine` is sufficient.
pub fn setup_vlc_env() {
    // BF2.9: removed `if std::env::var("PATH").is_ok()` which is always true.
    // Set VLC_PLUGIN_PATH only if it isn't already set, to respect the user's config.
    if std::env::var("VLC_PLUGIN_PATH").is_ok() {
        // Already configured — don't override.
        return;
    }
    let vlc_paths = [
        "/usr/lib/vlc",
        "/usr/lib/x86_64-linux-gnu/vlc",
        "/usr/local/lib/vlc",
    ];
    for vp in vlc_paths {
        let p = PathBuf::from(vp);
        if p.exists() {
            // SAFETY: called once before audio threads are created.
            #[allow(unsafe_code)]
            unsafe {
                std::env::set_var("VLC_PLUGIN_PATH", vp);
            }
            break;
        }
    }
}

/// Error message for missing VLC
pub fn vlc_error_message() -> String {
    tr!("vlc_not_found")
}
