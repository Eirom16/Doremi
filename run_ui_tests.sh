#!/usr/bin/env bash
# run_ui_tests.sh — Ejecutar todas las pruebas visuales de Doremi
# Uso: ./run_ui_tests.sh [output_dir]
# Ejemplo: ./run_ui_tests.sh /tmp/doremi_screenshots
set -euo pipefail

BINARY="${BINARY:-./target/debug/doremi}"
OUT_DIR="${1:-/tmp/doremi_ui_tests}"
TIMESTAMP="$(date +%Y%m%d_%H%M%S)"
RUN_DIR="$OUT_DIR/$TIMESTAMP"
XDG_ROOT="${XDG_ROOT:-/tmp/doremi_ui_xdg}"

mkdir -p "$RUN_DIR"
mkdir -p "$XDG_ROOT/config" "$XDG_ROOT/data" "$XDG_ROOT/cache"

export DOREMI_DISABLE_SINGLE_INSTANCE="${DOREMI_DISABLE_SINGLE_INSTANCE:-1}"
export DOREMI_UI_TEST="${DOREMI_UI_TEST:-1}"
export QT_QPA_PLATFORM="${QT_QPA_PLATFORM:-offscreen}"
export QTWEBENGINE_DISABLE_SANDBOX="${QTWEBENGINE_DISABLE_SANDBOX:-1}"
export QTWEBENGINE_CHROMIUM_FLAGS="${QTWEBENGINE_CHROMIUM_FLAGS:---no-sandbox --disable-gpu}"
export XDG_CONFIG_HOME="${XDG_CONFIG_HOME:-$XDG_ROOT/config}"
export XDG_DATA_HOME="${XDG_DATA_HOME:-$XDG_ROOT/data}"
export XDG_CACHE_HOME="${XDG_CACHE_HOME:-$XDG_ROOT/cache}"

VIEWS=(
    "home"
    "search"
    "library"
    "trending"
    "downloads"
    "history"
    "stats"
    "settings"
    "now-playing"
    "album_detail"
    "artist_detail"
    "playlist_detail"
    "show_detail"
    "welcome"
)

if [[ ! -f "$BINARY" ]]; then
    echo "❌ Binary not found at $BINARY"
    echo "   Run: cargo build"
    exit 1
fi

PASS=0
FAIL=0

for view in "${VIEWS[@]}"; do
    OUT_FILE="$RUN_DIR/${view//-/_}.png"
    echo -n "📸 Testing view: $view → $OUT_FILE ... "

    # Run with a 10-second timeout. The app should quit on its own after the screenshot.
    if timeout 12 "$BINARY" --ui-test "$view" --screenshot "$OUT_FILE" 2>/dev/null; then
        if [[ -f "$OUT_FILE" ]]; then
            SIZE=$(stat -c%s "$OUT_FILE" 2>/dev/null || stat -f%z "$OUT_FILE" 2>/dev/null)
            echo "✅ ($SIZE bytes)"
            PASS=$((PASS + 1))
        else
            echo "⚠️  Binary exited OK but no screenshot found"
            FAIL=$((FAIL + 1))
        fi
    else
        echo "❌ Timed out or crashed"
        FAIL=$((FAIL + 1))
    fi
done

echo ""
echo "═══════════════════════════════════"
echo "  Results: $PASS passed / $FAIL failed"
echo "  Output:  $RUN_DIR"
echo "═══════════════════════════════════"

if [[ $FAIL -gt 0 ]]; then
    exit 1
fi
