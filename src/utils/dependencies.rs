use crate::tr;
use std::process::Command;

pub struct DependencyStatus {
    pub ytdlp_ok: bool,
    pub ytdlp_version: String,
    pub ffmpeg_ok: bool,
    pub ffmpeg_version: String,
}

/// Check if yt-dlp and ffmpeg are available on the system
pub fn check_dependencies() -> DependencyStatus {
    let ytdlp_res = Command::new("yt-dlp").arg("--version").output();
    let (ytdlp_ok, ytdlp_version) = match ytdlp_res {
        Ok(output) => {
            let ver = String::from_utf8_lossy(&output.stdout).trim().to_string();
            (true, ver)
        }
        Err(_) => (false, tr!("ytdlp_not_found")),
    };

    let ffmpeg_res = Command::new("ffmpeg").arg("-version").output();
    let (ffmpeg_ok, ffmpeg_version) = match ffmpeg_res {
        Ok(output) => {
            let ver_out = String::from_utf8_lossy(&output.stdout);
            let first_line = ver_out.lines().next().unwrap_or("").to_string();
            let ver = first_line
                .split("version ")
                .nth(1)
                .and_then(|s| s.split_whitespace().next())
                .unwrap_or("detectado")
                .to_string();
            (true, ver)
        }
        Err(_) => (false, tr!("ffmpeg_not_found")),
    };

    DependencyStatus {
        ytdlp_ok,
        ytdlp_version,
        ffmpeg_ok,
        ffmpeg_version,
    }
}
