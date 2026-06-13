use serde::{Deserialize, Serialize};
use std::path::Path;
use std::io::Write;

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct AppearanceSettings {
    #[serde(default = "default_theme_mode")]
    pub theme_mode: String,
    #[serde(default = "default_accent_color")]
    pub accent_color: String,
    #[serde(default = "default_true")]
    pub use_dynamic_color: bool,
    #[serde(default = "default_true")]
    pub show_artwork_blur_bg: bool,
    #[serde(default = "default_false")]
    pub compact_sidebar: bool,
    #[serde(default = "default_font_size")]
    pub font_size: i32,
}

fn default_theme_mode() -> String { "dark".to_string() }
fn default_accent_color() -> String { "#7C4DFF".to_string() }
fn default_true() -> bool { true }
fn default_false() -> bool { false }
fn default_font_size() -> i32 { 13 }

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct PlayerSettings {
    #[serde(default = "default_volume")]
    pub volume: i32,
    #[serde(default = "default_true")]
    pub normalize_audio: bool,
    #[serde(default = "default_false")]
    pub skip_silence: bool,
    #[serde(default = "default_true")]
    pub crossfade_enabled: bool,
    #[serde(default = "default_crossfade_duration")]
    pub crossfade_duration_sec: i32,
    #[serde(default = "default_true")]
    pub resume_on_startup: bool,
    #[serde(default = "default_true")]
    pub gapless_playback: bool,
    #[serde(default = "default_false")]
    pub stop_on_close: bool,
    #[serde(default = "default_sleep_timer")]
    pub sleep_timer_minutes: i32,
}

fn default_volume() -> i32 { 80 }
fn default_crossfade_duration() -> i32 { 5 }
fn default_sleep_timer() -> i32 { 0 }

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct EqualizerSettings {
    #[serde(default)]
    pub enabled: bool,
    #[serde(default)]
    pub preamp: f64,
    #[serde(default = "default_eq_bands")]
    pub bands: Vec<f64>,
    #[serde(default = "default_eq_preset")]
    pub preset_name: String,
}

fn default_eq_bands() -> Vec<f64> { vec![0.0; 10] }
fn default_eq_preset() -> String { "Flat".to_string() }

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct NetworkSettings {
    pub proxy_url: Option<String>,
    #[serde(default = "default_stream_quality")]
    pub stream_quality: String,
    #[serde(default = "default_true")]
    pub preload_next: bool,
}

fn default_stream_quality() -> String { "best".to_string() }

#[derive(Debug, Clone, Serialize, Deserialize, Default)]
pub struct IntegrationsSettings {
    #[serde(default)]
    pub lastfm_enabled: bool,
    #[serde(default, skip_serializing)]
    pub lastfm_session_key: String,
    #[serde(default, skip_serializing)]
    pub lastfm_api_key: String,
    #[serde(default, skip_serializing)]
    pub lastfm_api_secret: String,
    #[serde(default)]
    pub lastfm_username: String,
    #[serde(default)]
    pub discord_rpc_enabled: bool,
    #[serde(default = "default_true")]
    pub mpris_enabled: bool,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct SubtitleSettings {
    #[serde(default = "default_alignment")]
    pub alignment: String,
    #[serde(default = "default_subtitle_font_size")]
    pub font_size: i32,
    #[serde(default = "default_line_spacing")]
    pub line_spacing: f64,
    #[serde(default)]
    pub delay_ms: i32,
    #[serde(default = "default_true")]
    pub auto_scroll: bool,
    #[serde(default = "default_animation_style")]
    pub animation_style: String,
    #[serde(default = "default_true")]
    pub glow_effect: bool,
    #[serde(default = "default_text_color_active")]
    pub text_color_active: String,
    #[serde(default = "default_text_color_inactive")]
    pub text_color_inactive: String,
}

fn default_alignment() -> String { "center".to_string() }
fn default_subtitle_font_size() -> i32 { 22 }
fn default_line_spacing() -> f64 { 1.5 }
fn default_animation_style() -> String { "glow".to_string() }
fn default_text_color_active() -> String { "#FFFFFF".to_string() }
fn default_text_color_inactive() -> String { "#6E6E77".to_string() }

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct AppSettings {
    #[serde(default)]
    pub google_client_id: String,
    #[serde(default, skip_serializing)]
    pub google_client_secret: String,
    #[serde(default)]
    pub appearance: AppearanceSettings,
    #[serde(default)]
    pub player: PlayerSettings,
    #[serde(default)]
    pub equalizer: EqualizerSettings,
    #[serde(default)]
    pub network: NetworkSettings,
    #[serde(default)]
    pub integrations: IntegrationsSettings,
    #[serde(default)]
    pub subtitles: SubtitleSettings,
    #[serde(default = "default_language")]
    pub language: String,
    pub last_video_id: Option<String>,
}

fn default_language() -> String { "es".to_string() }

impl Default for AppSettings {
    fn default() -> Self {
        Self {
            google_client_id: String::new(),
            google_client_secret: String::new(),
            appearance: AppearanceSettings::default(),
            player: PlayerSettings::default(),
            equalizer: EqualizerSettings::default(),
            network: NetworkSettings::default(),
            integrations: IntegrationsSettings::default(),
            subtitles: SubtitleSettings::default(),
            language: "es".to_string(),
            last_video_id: None,
        }
    }
}

impl Default for AppearanceSettings {
    fn default() -> Self {
        Self {
            theme_mode: "dark".to_string(),
            accent_color: "#7C4DFF".to_string(),
            use_dynamic_color: true,
            show_artwork_blur_bg: true,
            compact_sidebar: false,
            font_size: 13,
        }
    }
}

impl Default for PlayerSettings {
    fn default() -> Self {
        Self {
            volume: 80,
            normalize_audio: true,
            skip_silence: false,
            crossfade_enabled: true,
            crossfade_duration_sec: 5,
            resume_on_startup: true,
            gapless_playback: true,
            stop_on_close: false,
            sleep_timer_minutes: 0,
        }
    }
}

impl Default for EqualizerSettings {
    fn default() -> Self {
        Self {
            enabled: false,
            preamp: 0.0,
            bands: vec![0.0; 10],
            preset_name: "Flat".to_string(),
        }
    }
}

impl Default for NetworkSettings {
    fn default() -> Self {
        Self {
            proxy_url: None,
            stream_quality: "best".to_string(),
            preload_next: true,
        }
    }
}

impl Default for SubtitleSettings {
    fn default() -> Self {
        Self {
            alignment: "center".to_string(),
            font_size: 22,
            line_spacing: 1.5,
            delay_ms: 0,
            auto_scroll: true,
            animation_style: "glow".to_string(),
            glow_effect: true,
            text_color_active: "#FFFFFF".to_string(),
            text_color_inactive: "#6E6E77".to_string(),
        }
    }
}

impl AppSettings {
    pub fn load(path: &Path) -> Self {
        let mut settings = Self::default();
        if path.exists() {
            match std::fs::read_to_string(path) {
                Ok(content) => {
                    match toml::from_str(&content) {
                        Ok(parsed) => settings = parsed,
                        Err(e) => log::warn!("Failed to parse settings: {e}"),
                    }
                }
                Err(e) => log::warn!("Failed to read settings file: {e}"),
            }
        }
        let legacy_credentials = crate::utils::secure_storage::LastFmCredentials {
            api_key: settings.integrations.lastfm_api_key.clone(),
            api_secret: settings.integrations.lastfm_api_secret.clone(),
            session_key: settings.integrations.lastfm_session_key.clone(),
        };

        if !legacy_credentials.api_key.is_empty()
            || !legacy_credentials.api_secret.is_empty()
            || !legacy_credentials.session_key.is_empty()
        {
            match crate::utils::secure_storage::save_lastfm_credentials(&legacy_credentials) {
                Ok(()) => {
                    settings.integrations.lastfm_api_key.clear();
                    settings.integrations.lastfm_api_secret.clear();
                    settings.integrations.lastfm_session_key.clear();
                    if let Err(e) = settings.save(path) {
                        log::error!("Failed to remove legacy Last.fm secrets from settings: {e}");
                    } else {
                        log::info!("Migrated Last.fm credentials to Secret Service");
                    }
                }
                Err(e) => log::warn!("Could not migrate Last.fm credentials: {e}"),
            }
        }

        if !settings.google_client_secret.is_empty() {
            match crate::utils::secure_storage::save_google_client_secret(&settings.google_client_secret) {
                Ok(()) => {
                    settings.google_client_secret.clear();
                    if let Err(e) = settings.save(path) {
                        log::error!("Failed to remove legacy Google secret from settings: {e}");
                    } else {
                        log::info!("Migrated Google client secret to Secret Service");
                    }
                }
                Err(e) => log::warn!("Could not migrate Google client secret: {e}"),
            }
        }

        match crate::utils::secure_storage::load_lastfm_credentials() {
            Ok(credentials) => {
                settings.integrations.lastfm_api_key = credentials.api_key;
                settings.integrations.lastfm_api_secret = credentials.api_secret;
                settings.integrations.lastfm_session_key = credentials.session_key;
            }
            Err(e) => log::debug!("Could not load Last.fm credentials: {e}"),
        }

        match crate::utils::secure_storage::load_google_client_secret() {
            Ok(Some(secret)) => settings.google_client_secret = secret,
            Ok(None) => {}
            Err(e) => log::debug!("Could not load Google client secret: {e}"),
        }

        settings
    }

    pub fn save(&self, path: &Path) -> Result<(), Box<dyn std::error::Error>> {
        let credentials = crate::utils::secure_storage::LastFmCredentials {
            api_key: self.integrations.lastfm_api_key.clone(),
            api_secret: self.integrations.lastfm_api_secret.clone(),
            session_key: self.integrations.lastfm_session_key.clone(),
        };
        if !credentials.api_key.is_empty()
            || !credentials.api_secret.is_empty()
            || !credentials.session_key.is_empty()
        {
            crate::utils::secure_storage::save_lastfm_credentials(&credentials)
                .map_err(std::io::Error::other)?;
        }
        if !self.google_client_secret.is_empty() {
            crate::utils::secure_storage::save_google_client_secret(&self.google_client_secret)
                .map_err(std::io::Error::other)?;
        }
        if let Some(parent) = path.parent() {
            std::fs::create_dir_all(parent)?;
        }
        let data = toml::to_string_pretty(self)?;
        write_private_file(path, data.as_bytes())?;
        Ok(())
    }

    pub fn sanitized_toml(&self) -> Result<String, toml::ser::Error> {
        toml::to_string_pretty(self)
    }
}

pub(crate) fn write_private_file(path: &Path, data: &[u8]) -> std::io::Result<()> {
    let mut options = std::fs::OpenOptions::new();
    options.create(true).truncate(true).write(true);
    #[cfg(unix)]
    {
        use std::os::unix::fs::OpenOptionsExt;
        options.mode(0o600);
    }
    let mut file = options.open(path)?;
    #[cfg(unix)]
    {
        use std::os::unix::fs::PermissionsExt;
        file.set_permissions(std::fs::Permissions::from_mode(0o600))?;
    }
    file.write_all(data)
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn serialized_settings_exclude_secrets() {
        let mut settings = AppSettings::default();
        settings.google_client_secret = "client-secret".to_string();
        settings.integrations.lastfm_api_key = "api-key".to_string();
        settings.integrations.lastfm_api_secret = "api-secret".to_string();
        settings.integrations.lastfm_session_key = "session-key".to_string();

        let serialized = toml::to_string(&settings).unwrap();
        assert!(!serialized.contains("api-key"));
        assert!(!serialized.contains("api-secret"));
        assert!(!serialized.contains("session-key"));
        assert!(!serialized.contains("client-secret"));
    }

    #[cfg(unix)]
    #[test]
    fn saved_settings_are_private() {
        use std::os::unix::fs::PermissionsExt;

        let path = std::env::temp_dir().join(format!(
            "doremi-settings-permissions-{}-{}.toml",
            std::process::id(),
            std::thread::current().name().unwrap_or("test")
        ));
        AppSettings::default().save(&path).unwrap();
        let mode = std::fs::metadata(&path).unwrap().permissions().mode() & 0o777;
        let _ = std::fs::remove_file(path);
        assert_eq!(mode, 0o600);
    }
}
