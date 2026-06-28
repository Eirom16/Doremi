# Doremi - Project Knowledge

## Architecture

- **Rust backend** (tokio async runtime) in `src/` — networking, YTM API, VLC audio, D-Bus/MPRIS, SQLite DB, config
- **C++/Qt6 UI** in `src/cpp/` — Qt6 Widgets with DesignTokens, glass panels, nebula backgrounds
- **Bridge**: typed FFI via `cxx` crate in `src/bridge.rs` (~2900 lines, single bridge block)
- **Module structure**: `api/`, `config/`, `db/`, `player/`, `services/`, `system/`, `utils/`

## Build

| Command | Description |
|---------|-------------|
| `cargo build --release` | Release build with LTO, codegen-units=1, stripped |
| `cargo check --all-targets` | Fast compilation check |
| `cargo test` | Run all Rust unit tests |
| `cargo run -- --ui-test <view> --screenshot /path/to.png` | Visual test mode |

`build.rs` orchestrates: `cxx-build` (bridge) + `cc` (C++17 sources) + `moc` (Qt meta-object) + `rcc` (resources) + `pkg-config` (Qt6 libs).

## Bridge Contract (`src/bridge.rs`)

- **Versioned**: `CONTRACT_MAJOR=1`, `CONTRACT_MINOR=3` — verify at startup
- **C++ → Rust** (`extern "Rust"`): ~50 `on_*` callbacks (player controls, search, browse, library, downloads, auth, settings, stats)
- **Rust → C++** (`unsafe extern "C++"`): ~50 `set_*` functions (UI data push, theme, player state, search results, browse details)
- **Shared structs**: `Track`, `Playlist`, `Album`, `Artist`, `TopResult`, `Show`, `Episode`, `HomeCard`, `StatsData`, `DownloadItem`
- All bridge text is UTF-8. Use `Ffi::to_qstring` / `Ffi::to_std_string` / `Ffi::to_rust_string` in C++ side.

## Database (SQLite, schema v11)

Tables: `favorite_tracks`, `favorite_albums`, `favorite_artists`, `favorite_shows`, `playlists`, `playlist_tracks`, `recently_played`, `search_history`, `response_cache`, `downloads`, `lyrics_cache`, `cached_shows`, `cached_show_episodes`.

PRAGMA: `journal_mode=WAL`, `foreign_keys=ON`.
Connection: global `Mutex<Option<Connection>>`, access via `with_db(f)` / `with_db_mut(f)`.

## Conventions

- **Rust**: snake_case, PascalCase types, `DoremiError` via thiserror, logging via `log` crate
- **Bridge**: `on_*` (C++→Rust callbacks), `set_*` (Rust→C++ data push)
- **Async**: tokio runtime, `spawn_blocking` for DB/blocking IO
- **Threading**: GUI thread owns all QObject widgets. Tokio runtime for HTTP/streams/downloads.
- **Error handling**: `Result` propagation for DB, `log::error!` / `unwrap_or_default()` for non-critical failures
- **Spanish**: log messages and UI strings in Spanish

## Tests

- `#[cfg(test)] mod tests` at bottom of each file (26 test modules across codebase)
- In-memory SQLite (`Connection::open_in_memory()`) for DB tests
- `TEST_MUTEX` serialization for global connection tests
- No integration tests directory

## Key Config

- Settings: TOML at `XDG_CONFIG_HOME/doremi/settings.toml`
- Paths: `directories` crate for XDG paths
- Secure storage: keyring/libsecret for YouTube headers, Last.fm credentials

## UI Components (C++)

Views: `welcome_view`, `home_view`, `search_view`, `library_view`, `trending_view`, `now_playing_view`, `settings_view`, `stats_view`, `history_view`, `downloads_view`, `album_detail_view`, `artist_detail_view`, `playlist_detail_view`, `show_detail_view`.

Controllers: `navigation_controller`, `theme_controller`, `tray_controller`, `shortcut_manager`, `session_cookie_manager`.

Components: `album_card`, `artist_card`, `song_card`, `stat_card`, `horizontal_carousel`, `glass_panel`, `fade_stack`, `nebula_bg`, `vinyl_disc`, `waveform_bars`, `skeleton_loader`, `toast_notification`, etc.
