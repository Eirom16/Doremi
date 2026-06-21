pub const VERSION: &str = "2.0.0";

pub mod api;
pub mod app;
pub mod bridge;
pub mod config;
pub mod db;
pub mod mpris;
pub mod player;
pub mod services;
pub mod utils;
pub mod system;

// Temporary: mark whole bridge to avoid unused warnings in Phase 0
#[allow(unused_imports)]
pub use bridge::bridge::*;

pub use config::paths::AppDirs;
pub use config::settings::AppSettings;
pub use config::themes;
pub use utils::i18n;
pub use utils::time;
