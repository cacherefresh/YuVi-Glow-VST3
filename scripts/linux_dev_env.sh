#!/usr/bin/env bash
# Build and (re)launch the Standalone build for local Linux development.
# Encodes the gotchas hit repeatedly during development on this platform:
#   - the app needs a real DISPLAY, which isn't always inherited by scripted/
#     background shells, so it's set explicitly;
#   - `pkill -f "YuVi Glow"` can match its own wrapping shell's command line
#     and kill itself instead of (or as well as) the app, if the pattern text
#     appears verbatim in that shell's own invocation — a bracket-obfuscated
#     pattern ("[Y]uVi Glow") avoids this while still matching the real app.
#
# Usage: scripts/linux_dev_env.sh [Debug|Release]
set -euo pipefail

CONFIG="${1:-Debug}"
REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="$REPO_ROOT/build"
APP_PATH="$BUILD_DIR/YuViGlow_artefacts/$CONFIG/Standalone/YuVi Glow"
LOG_FILE="/tmp/yuviglow_dev.log"

cd "$REPO_ROOT"

if [ ! -d "$BUILD_DIR" ]; then
    echo "==> No build directory yet, configuring..."
    cmake -S . -B "$BUILD_DIR"
fi

echo "==> Building ($CONFIG)..."
cmake --build "$BUILD_DIR" --config "$CONFIG" -j"$(nproc)"

echo "==> Stopping any previous instance..."
pkill -f "[Y]uVi Glow" 2>/dev/null || true
sleep 1

echo "==> Launching Standalone build..."
DISPLAY="${DISPLAY:-:0}" nohup "$APP_PATH" > "$LOG_FILE" 2>&1 &
disown
sleep 2

if pgrep -f "[Y]uVi Glow" > /dev/null; then
    echo "==> Running. Process(es):"
    pgrep -af "[Y]uVi Glow"
    echo "==> Log: $LOG_FILE"
else
    echo "!! App did not stay running — check $LOG_FILE"
    exit 1
fi
