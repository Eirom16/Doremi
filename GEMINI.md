# Doremi - Conocimiento del Proyecto para Antigravity

## Arquitectura
- **Backend Rust** (tokio async) en `src/` — red, API YTM, audio VLC, D-Bus/MPRIS, SQLite, configuración
- **UI C++/Qt6** en `src/cpp/` — Qt6 Widgets con DesignTokens
- **Bridge**: FFI tipado via `cxx` en `src/bridge.rs` (~2900 líneas)
- **Módulos**: `api/`, `config/`, `db/`, `player/`, `services/`, `system/`, `utils/`

## Build
- Release: `cargo build --release`
- Check: `cargo check --all-targets`
- Tests: `cargo test`
- Test visual: `cargo run -- --ui-test <view> --screenshot /path/to.png`

## Bridge (`src/bridge.rs`)
- Contrato: `CONTRACT_MAJOR=1`, `CONTRACT_MINOR=3`
- C++→Rust (`extern "Rust"`): ~50 callbacks `on_*`
- Rust→C++ (`unsafe extern "C++"`): ~50 funciones `set_*`
- Structs compartidos: `Track`, `Playlist`, `Album`, `Artist`, `StatsData`, `DownloadItem`

## Base de Datos (SQLite, schema v11)
Tablas: `favorite_tracks`, `favorite_albums`, `favorite_artists`, `playlists`, `playlist_tracks`, `recently_played`, `search_history`, `response_cache`, `downloads`, `lyrics_cache`.
Conexión: `Mutex<Option<Connection>>`, acceso via `with_db(f)` / `with_db_mut(f)`.

## Convenciones
- Rust: snake_case, PascalCase, `DoremiError` via thiserror
- Bridge: `on_*` (C++→Rust), `set_*` (Rust→C++)
- Tokio para async, `spawn_blocking` para DB
- GUI thread para widgets Qt. Tokio para HTTP/streams
- Logs con `log::error!`, errores no críticos con `unwrap_or_default()`
- Settings: TOML en `XDG_CONFIG_HOME/doremi/settings.toml`

## UI Componentes (C++)
Vistas: `welcome_view`, `home_view`, `search_view`, `library_view`, `trending_view`, `now_playing_view`, `settings_view`, `stats_view`, `history_view`, `downloads_view`, `album_detail_view`, `artist_detail_view`, `playlist_detail_view`, `show_detail_view`.
Controladores: `navigation_controller`, `theme_controller`, `tray_controller`, `shortcut_manager`, `session_cookie_manager`.
