---
trigger: always_on
description: Reglas de proyecto para Doremi
---

# Doremi Project Rules

## Rust/C++ Bridge
- Bridge en `src/bridge.rs` via `cxx`. Structs compartidos dentro de `#[cxx::bridge]`.
- `on_*` son callbacks de C++ a Rust. `set_*` empujan datos de Rust a C++.
- No modifiques archivos `.h`/`.cpp` desde un agente Rust.

## Build & Test
- `cargo build --release` para release con LTO.
- `cargo check --all-targets` para verificar compilación.
- `cargo test` para tests unitarios.

## Base de Datos
- SQLite con WAL journal. Migraciones v1 a v11 en `src/db/mod.rs`.
- Acceso via `with_db(f)` / `with_db_mut(f)`.

## UI Qt6
- Usa DesignTokens (colores/fonts/márgenes) — nunca valores hardcodeados.
- Mutaciones de widgets SOLO en el GUI thread.
- Prefiere CMake moderno (`qt_add_executable`, `qt_standard_project_setup()`).
