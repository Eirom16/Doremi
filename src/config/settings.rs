use serde::{Deserialize, Serialize};
use std::path::Path;

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
    #[serde(default)]
    pub lastfm_session_key: String,
    #[serde(default)]
    pub lastfm_api_key: String,
    #[serde(default)]
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
    #[serde(default)]
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
        if path.exists() {
            match std::fs::read_to_string(path) {
                Ok(content) => {
                    match toml::from_str(&content) {
                        Ok(settings) => return settings,
                        Err(e) => log::warn!("Failed to parse settings: {e}"),
                    }
                }
                Err(e) => log::warn!("Failed to read settings file: {e}"),
            }
        }
        Self::default()
    }

    pub fn save(&self, path: &Path) -> Result<(), Box<dyn std::error::Error>> {
        if let Some(parent) = path.parent() {
            std::fs::create_dir_all(parent)?;
        }
        let data = toml::to_string_pretty(self)?;
        std::fs::write(path, data)?;
        Ok(())
    }
}
