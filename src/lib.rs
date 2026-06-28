#![allow(dead_code)]
#![deny(clippy::all)]
#![allow(clippy::collapsible_if)]
#![allow(clippy::clone_on_copy)]
#![allow(clippy::field_reassign_with_default)]
#![allow(clippy::get_first)]
#![allow(clippy::items_after_test_module)]
#![allow(clippy::let_and_return)]
#![allow(clippy::let_unit_value)]
#![allow(clippy::module_inception)]
#![allow(clippy::needless_borrow)]
#![allow(clippy::needless_borrows_for_generic_args)]
#![allow(clippy::needless_range_loop)]
#![allow(clippy::needless_return)]
#![allow(clippy::new_without_default)]
#![allow(clippy::should_implement_trait)]
#![allow(clippy::single_match)]
#![allow(clippy::unnecessary_sort_by)]
#![allow(clippy::too_many_arguments)]
#![allow(clippy::type_complexity)]
#![allow(clippy::unnecessary_cast)]
#![allow(clippy::vec_init_then_push)]
#![allow(clippy::wrong_self_convention)]

pub const VERSION: &str = "2.0.0";

pub mod api;
pub mod app;
pub mod bridge;
pub mod config;
pub mod db;
pub mod mpris;
pub mod player;
pub mod services;
pub mod system;
pub mod utils;

// Temporary: mark whole bridge to avoid unused warnings in Phase 0
#[allow(unused_imports)]
pub use bridge::bridge::*;

pub use config::paths::AppDirs;
pub use config::settings::AppSettings;
pub use config::themes;
pub use utils::i18n;
pub use utils::time;
