# Doremi - Project Knowledge for Claude Code

## Architecture
- **Rust backend** (tokio async) in `src/` — networking, YTM API, VLC audio, D-Bus/MPRIS, SQLite DB, config
- **C++/Qt6 UI** in `src/cpp/` — Qt6 Widgets with DesignTokens
- **Bridge**: typed FFI via `cxx` crate in `src/bridge.rs` (~2900 lines)
- **Modules**: `api/`, `config/`, `db/`, `player/`, `services/`, `system/`, `utils/`

## Build
- Release: `cargo build --release`
- Check: `cargo check --all-targets`
- Test: `cargo test`
- Visual test: `cargo run -- --ui-test <view> --screenshot /path/to.png`
- `build.rs` runs: cxx-build + cc (C++17) + moc + rcc + pkg-config

## Bridge (`src/bridge.rs`)
- Contract: `CONTRACT_MAJOR=1`, `CONTRACT_MINOR=3`
- C++→Rust (`extern "Rust"`): ~50 `on_*` callbacks
- Rust→C++ (`unsafe extern "C++"`): ~50 `set_*` functions
- Shared structs: `Track`, `Playlist`, `Album`, `Artist`, `TopResult`, `Show`, `Episode`, `HomeCard`, `StatsData`, `DownloadItem`
- UTF-8 text: use `Ffi::to_qstring` / `Ffi::to_std_string` / `Ffi::to_rust_string`

## Database (SQLite, schema v11)
Tables: `favorite_tracks`, `favorite_albums`, `favorite_artists`, `favorite_shows`, `playlists`, `playlist_tracks`, `recently_played`, `search_history`, `response_cache`, `downloads`, `lyrics_cache`, `cached_shows`, `cached_show_episodes`.
PRAGMA: `journal_mode=WAL`, `foreign_keys=ON`.
Connection: `Mutex<Option<Connection>>`, access via `with_db(f)` / `with_db_mut(f)`.

## Conventions
- Rust: snake_case, PascalCase types, `DoremiError` via thiserror
- Bridge: `on_*` (C++→Rust), `set_*` (Rust→C++)
- Async: tokio runtime, `spawn_blocking` for DB/blocking IO
- GUI thread owns QObject widgets. Tokio for HTTP/streams/downloads
- Error handling: `log::error!` for non-critical, `Result` for DB
- Settings: TOML at `XDG_CONFIG_HOME/doremi/settings.toml`
- Paths: `directories` crate for XDG paths

## UI Components (C++)
Views: `welcome_view`, `home_view`, `search_view`, `library_view`, `trending_view`, `now_playing_view`, `settings_view`, `stats_view`, `history_view`, `downloads_view`, `album_detail_view`, `artist_detail_view`, `playlist_detail_view`, `show_detail_view`.
Controllers: `navigation_controller`, `theme_controller`, `tray_controller`, `shortcut_manager`, `session_cookie_manager`.
