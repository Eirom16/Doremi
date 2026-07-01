# 🎵 Doremi

**Doremi** es un cliente de escritorio elegante para **YouTube Music** en **Linux**. Diseñado para ofrecer una experiencia de reproducción fluida y moderna, combina una interfaz nativa con potentes funciones de búsqueda, navegación y control de audio.

## ✨ Características principales

- 🎧 Reproducción de música desde YouTube Music con VLC (ecualizador, crossfade, gapless)
- 🔍 Búsqueda integrada con filtros, sugerencias e historial
- 📚 Biblioteca completa: canciones, álbumes, artistas, playlists y podcasts
- 📥 Descargas y modo offline con yt-dlp
- 🎤 Letras sincronizadas con auto-scroll y seek por línea (LRCLIB)
- 📊 Estadísticas de escucha e historial con rangos de tiempo
- 🔗 Integración con Last.fm, Discord RPC, MPRIS y bandeja del sistema
- 🔒 Almacenamiento seguro de credenciales (Secret Service / KWallet)
- 🎨 Temas claro/oscuro con colores de acento personalizables
- 🌐 Localización en español e inglés
- 🎙️ Soporte para podcasts y shows

## 🧩 Arquitectura

Doremi está construido con:

- **Rust** (~38k LOC) — lógica central, API, audio, estado, servicios y concurrencia
- **C++/Qt 6** (~18k LOC) — interfaz nativa, animaciones y accesibilidad
- **cxx** — contratos tipados entre Rust y C++ sin protocolos de strings ambiguos
- **VLC** — reproducción y control de audio
- **SQLite** — persistencia local (caché, historial, biblioteca, descargas)

## 📁 Organización del proyecto

```
src/
├── api/          # Cliente YouTube Music (Innertube)
├── bridge/       # Contratos cxx Rust↔C++ por dominio
├── config/       # Settings, temas, paths
├── cpp/          # Interfaz Qt6 (vistas, componentes, design system)
│   └── components/  # Widgets reutilizables
├── db/           # SQLite: migraciones, repo, caché
├── player/       # Reproductor VLC, cola, resolver, estado
├── services/     # Discord, Last.fm, descargas, letras, updater
├── system/       # Conectividad
├── ui/           # Design system Rust-side
└── utils/        # Seguridad, i18n, backup, migración, artwork
```

## 📦 Instalación

### Dependencias de runtime

- Qt 6 (Widgets + WebEngine)
- VLC (libvlc)
- D-Bus
- **Recomendado:** yt-dlp, ffmpeg (para descargas)
- **Opcional:** gnome-keyring o kwallet (almacenamiento seguro)

### Desde paquete (recomendado)

**Debian/Ubuntu:**
```bash
sudo dpkg -i doremi_2.0.0_amd64.deb
sudo apt-get install -f  # Instalar dependencias faltantes
```

**Arch Linux:**
```bash
# Desde PKGBUILD (en packaging/arch/)
makepkg -si
```

### Desde código fuente

```bash
# Dependencias de compilación (Ubuntu/Debian)
sudo apt-get install pkg-config qt6-base-dev qt6-webengine-dev \
  libqt6webenginewidgets6 libvlc-dev libdbus-1-dev

# Compilar
cargo build --release

# Ejecutar
./target/release/doremi
```

### Instalar manualmente

```bash
# Binario
sudo install -Dm755 target/release/doremi /usr/bin/doremi

# Icono
sudo install -Dm644 assets/icons/io.github.eirom16.Doremi.svg \
  /usr/share/icons/hicolor/scalable/apps/io.github.eirom16.Doremi.svg

# Entrada de escritorio
sudo install -Dm644 assets/io.github.eirom16.Doremi.desktop \
  /usr/share/applications/io.github.eirom16.Doremi.desktop

# AppStream metadata
sudo install -Dm644 assets/io.github.eirom16.Doremi.metainfo.xml \
  /usr/share/metainfo/io.github.eirom16.Doremi.metainfo.xml
```

## 🔧 Desarrollo

```bash
# Compilar en modo debug
cargo build

# Ejecutar tests
cargo test --all-targets

# Verificar formato y lints
cargo fmt --check
cargo clippy --all-targets -- -D warnings

# Crear paquete .deb
scripts/build-deb.sh
```

## 📄 Licencia

MIT — ver [CHANGELOG.md](CHANGELOG.md) para el historial de cambios.
