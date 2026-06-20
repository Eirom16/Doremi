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

/// Setup VLC environment variables
pub fn setup_vlc_env() {
    if std::env::var("PATH").is_ok() {
        let vlc_paths = vec![
            "/usr/lib/vlc",
            "/usr/lib/x86_64-linux-gnu/vlc",
            "/usr/local/lib/vlc",
        ];
        for vp in vlc_paths {
            let p = PathBuf::from(vp);
            if p.exists() {
                std::env::set_var("VLC_PLUGIN_PATH", vp);
                break;
            }
        }
    }
}

/// Error message for missing VLC
pub fn vlc_error_message() -> String {
    tr!("vlc_not_found")
}
