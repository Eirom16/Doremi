# ADR-001: Diseño del Módulo Bridge Rust/C++ (cxx)

## Estado

Aceptado

## Contexto

El módulo `src/bridge.rs` actúa como frontera entre Rust y C++. Con 2700+ líneas, contiene:

1. El macro `#[cxx::bridge]` con todos los tipos y firmas de funciones compartidas.
2. Las implementaciones Rust de los callbacks `on_*` (llamados desde C++).
3. Las funciones auxiliares del dominio (player, search, library, downloads, auth, settings).

A medida que la app crece, surge la pregunta: ¿debería dividirse este módulo?

## Decisión

**Mantener un único `#[cxx::bridge]` en `bridge.rs`, pero extraer las implementaciones de dominio a submódulos.**

La macro `#[cxx::bridge]` tiene la limitación técnica de que **no puede dividirse entre múltiples archivos** — debe estar en un único módulo Rust. Intentar moverla generaría errores de compilación en `cxx_build`.

Sin embargo, las funciones de implementación (las `pub fn on_*`) **sí pueden moverse** a submódulos dedicados. El bridge entonces simplemente reexporta o delega:

```rust
// bridge.rs — solo el contrato cxx y delegaciones
pub fn on_play_pause_triggered() {
    crate::services::player_actions::on_play_pause();
}
```

## Estructura de implementación actual (aceptada)

El bridge está organizado por secciones con comentarios marcadores:

- **Player controls** (líneas ~386-553): play/pause/next/prev/seek/volume
- **Search** (~411-492): submit, suggestions, history  
- **Home/Trending** (~495-516): retries, more
- **Browse/Detail** (~518-537): album, artist, playlist, show
- **System** (~539-590): quit, close, forwarded args
- **Library** (~600-1100): favorites, playlists, tabs, search
- **Downloads** (~1100-1200): queue, cancel, batch, delete
- **Playback** (~1368-1480): play_all, queue actions
- **Auth/Session** (~1500-1600): login, logout, session
- **Settings** (~1600-1740): appearance, player, equalizer
- **Stats/History** (~1740-1800): stats, history

## Consecuencias

### Positivas
- No se requiere reestructurar el macro cxx (sin riesgo de regresión de build).
- Las implementaciones dentro del bridge son delegaciones cortas y legibles.
- La separación de intereses se logra en los módulos `services/`, `db/`, `player/`, etc.

### Negativas
- `bridge.rs` seguirá siendo largo (~2700 líneas).
- Los desarrolladores nuevos necesitan conocer la convención de secciones.

### Mitigación
- Añadir un índice de secciones al inicio de bridge.rs.
- Documentar en `FFI_CONVENTIONS.md` que la macro cxx no puede dividirse.
- Considerar en el futuro si cxx evoluciona para permitir múltiples archivos de bridge.

## Alternativas consideradas

1. **Dividir en múltiples archivos bridge**: No es posible con la versión actual de `cxx`. El `#[cxx::bridge]` genera código C++ específico del módulo Rust donde se define.

2. **Usar cbindgen en lugar de cxx**: Permitiría más flexibilidad en la organización, pero perdería los tipos seguros de cxx (rust::Vec, rust::Str) y la verificación de contratos en compilación.

3. **Microservicios o IPC**: Excesiva complejidad para una aplicación desktop single-process.

## Referencias

- [cxx book: Multiple bridges](https://cxx.rs/extern-rust.html) — actualmente no soportado entre archivos
- `docs/architecture/BRIDGE_CONTRACT.md`
- `docs/architecture/FFI_CONVENTIONS.md`
