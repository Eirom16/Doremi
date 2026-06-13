# 🎵 Doremi

**Doremi** es un cliente de escritorio elegante para **YouTube Music** en **Linux**. Diseñado para ofrecer una experiencia de reproducción fluida y moderna, combina una interfaz nativa con potentes funciones de búsqueda, navegación y control de audio.

## ✨ Características principales

- 🎧 Reproducción de música desde YouTube Music
- 🔍 Búsqueda integrada con resultados rápidos
- 📚 Biblioteca y listas de reproducción organizadas
- 📥 Descargas y caché local para un uso más eficiente
- 🎹 Integración con VLC para reproducción de audio
- 🧭 Navegación clara con vistas de inicio, tendencias, biblioteca y más
- 🖥️ Interfaz nativa con componentes de C++ y Rust
- 🛠️ Gestión de configuraciones y temas personalizados

## 🧩 Arquitectura

Doremi está construido con:

- **Rust** para la lógica central, audio, estado y servicios
- **C++** para la capa de interfaz nativa y componentes visuales
- **VLC** para la reproducción y control de audio
- **SQLite** para almacenar caché y datos locales

## 📁 Organización del proyecto

- `src/` — Código fuente principal de Rust
- `assets/` — Fuentes y recursos visuales
- `cpp/` — Componentes de interfaz nativa y ventanas
- `src/api/` — Cliente e interacciones con servicios web
- `src/native/player/` — Reproducción y control de audio
- `src/services/` — Integración con Discord, descargas, lastfm, y más

## 🚀 Uso

Compila el proyecto con `cargo build --release` y ejecuta el binario `doremi`.

## 💡 En resumen

Doremi es una aplicación pensada para los usuarios de Linux que desean un cliente de escritorio completo para YouTube Music, con una mezcla de rendimiento, diseño nativo y funcionalidades avanzadas.
