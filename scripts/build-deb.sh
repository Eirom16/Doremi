#!/usr/bin/env bash
# Build a Debian package for Doremi.
# Usage: scripts/build-deb.sh [VERSION]
#   VERSION defaults to the value in Cargo.toml if not provided.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"

# Extract version from Cargo.toml if not provided
VERSION="${1:-$(grep '^version' "$PROJECT_DIR/Cargo.toml" | head -1 | sed 's/.*"\(.*\)"/\1/')}"
ARCH="amd64"
PKG_NAME="doremi"
PKG_DIR="$PROJECT_DIR/dist/${PKG_NAME}_${VERSION}_${ARCH}"
BINARY="$PROJECT_DIR/target/release/doremi"

echo "==> Building Debian package: ${PKG_NAME} v${VERSION}"

# Verify the binary exists
if [ ! -f "$BINARY" ]; then
    echo "ERROR: Release binary not found at $BINARY"
    echo "       Run 'cargo build --release' first."
    exit 1
fi

# Clean and create package directory structure
rm -rf "$PKG_DIR"
mkdir -p "$PKG_DIR/DEBIAN"
mkdir -p "$PKG_DIR/usr/bin"
mkdir -p "$PKG_DIR/usr/share/applications"
mkdir -p "$PKG_DIR/usr/share/metainfo"
mkdir -p "$PKG_DIR/usr/share/icons/hicolor/scalable/apps"
mkdir -p "$PKG_DIR/usr/share/doc/${PKG_NAME}"

# Copy binary
install -Dm755 "$BINARY" "$PKG_DIR/usr/bin/doremi"

# Copy desktop integration files
install -Dm644 "$PROJECT_DIR/assets/io.github.eirom16.Doremi.desktop" \
    "$PKG_DIR/usr/share/applications/io.github.eirom16.Doremi.desktop"
install -Dm644 "$PROJECT_DIR/assets/io.github.eirom16.Doremi.metainfo.xml" \
    "$PKG_DIR/usr/share/metainfo/io.github.eirom16.Doremi.metainfo.xml"
install -Dm644 "$PROJECT_DIR/assets/icons/io.github.eirom16.Doremi.svg" \
    "$PKG_DIR/usr/share/icons/hicolor/scalable/apps/io.github.eirom16.Doremi.svg"

# Copy documentation
install -Dm644 "$PROJECT_DIR/README.md" "$PKG_DIR/usr/share/doc/${PKG_NAME}/README.md"

# Calculate installed size (in KiB)
INSTALLED_SIZE=$(du -sk "$PKG_DIR" | cut -f1)

# Generate control file
cat > "$PKG_DIR/DEBIAN/control" << EOF
Package: ${PKG_NAME}
Version: ${VERSION}
Section: sound
Priority: optional
Architecture: ${ARCH}
Depends: libqt6widgets6 (>= 6.2), libqt6webenginewidgets6 (>= 6.2), libvlc5, libdbus-1-3
Recommends: yt-dlp, ffmpeg
Suggests: secret-tool
Installed-Size: ${INSTALLED_SIZE}
Maintainer: Eirom <eirom@users.noreply.github.com>
Homepage: https://github.com/Eirom16/Doremi
Description: Desktop client for YouTube Music
 Doremi is an elegant desktop client for YouTube Music on Linux.
 It combines a native Qt 6 interface with a high-performance Rust backend
 to deliver a smooth, feature-rich music experience.
 .
 Features include full YouTube Music integration, VLC-based audio playback
 with equalizer and crossfade, synchronized lyrics, offline downloads,
 Last.fm scrobbling, Discord RPC, MPRIS integration, and more.
EOF

# Build the .deb
mkdir -p "$PROJECT_DIR/dist"
dpkg-deb --build --root-owner-group "$PKG_DIR"

DEB_FILE="$PROJECT_DIR/dist/${PKG_NAME}_${VERSION}_${ARCH}.deb"
echo "==> Package built: $DEB_FILE"
echo "    Size: $(du -sh "$DEB_FILE" | cut -f1)"

# Optional: verify with lintian if available
if command -v lintian &>/dev/null; then
    echo "==> Running lintian..."
    lintian "$DEB_FILE" --no-tag-display-limit || true
fi

# Clean up build directory
rm -rf "$PKG_DIR"

echo "==> Done! Install with: sudo dpkg -i $DEB_FILE"
