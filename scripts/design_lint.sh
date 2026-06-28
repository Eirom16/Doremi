#!/usr/bin/env bash
# Design lint: check for hardcoded hex colors and border-radius outside design_tokens.cpp
set -euo pipefail

cd "$(dirname "$0")/.."

errors=0

# Check 1: hardcoded hex color literals (#XXXXXX or #XXX) outside allowed files
# Exclude design_tokens.cpp, .clang-format, scripts/
echo "--- Checking hardcoded hex colors outside design_tokens.cpp ---"
while IFS=: read -r file line content; do
    # Skip allowed paths
    case "$file" in
        src/cpp/design_tokens.cpp|src/cpp/design_tokens.h|.clang-format|scripts/*|target/*)
            continue ;;
    esac
    # Skip non-source files
    case "$file" in
        *.cpp|*.h) ;;
        *) continue ;;
    esac
    # Skip #pragma, #include, Q_PROPERTY, and comments
    case "$content" in
        *\#pragma*|*\#include*|*Q_PROPERTY*|*//\ *|*\*\ *) continue ;;
    esac
    # Skip intentional artistic colors (decided in BF4.6)
    case "$file" in
        *vinyl_disc.cpp|*nebula_bg.cpp) continue ;;
    esac
    echo "ERROR: $file:$line: hardcoded hex color found: $content"
    errors=$((errors + 1))
done < <(rg -n '"#[0-9A-Fa-f]{6}"|"#[0-9A-Fa-f]{3}"' src/cpp/ 2>/dev/null || true)

# Check 2: border-radius with hardcoded pixel value outside allowed files
echo "--- Checking hardcoded border-radius values outside design_tokens.cpp ---"
while IFS=: read -r file line content; do
    case "$file" in
        src/cpp/design_tokens.cpp|target/*)
            continue ;;
    esac
    echo "ERROR: $file:$line: hardcoded border-radius found: $content"
    errors=$((errors + 1))
done < <(rg -n 'border-radius:\s*[0-9]+px' src/cpp/ 2>/dev/null | rg -v 'border-radius:\s*1px' || true)

if [ "$errors" -gt 0 ]; then
    echo "--- FAILED: $errors design lint error(s) ---"
    exit 1
fi

echo "--- PASS: no design lint issues ---"
