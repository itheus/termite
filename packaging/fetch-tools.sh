#!/usr/bin/env bash
#
# Download the AppImage packaging tools into packaging/tools/.
# These are build-time helpers only; nothing is installed system-wide.
#
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TOOLS_DIR="$SCRIPT_DIR/tools"
mkdir -p "$TOOLS_DIR"

fetch() {
    local name="$1" url="$2"
    if [ -x "$TOOLS_DIR/$name" ]; then
        echo "present: $name"
        return
    fi
    echo "fetching: $name"
    curl -fsSL -o "$TOOLS_DIR/$name" "$url"
    chmod +x "$TOOLS_DIR/$name"
}

BASE_LD="https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous"
BASE_QT="https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous"
BASE_AT="https://github.com/AppImage/appimagetool/releases/download/continuous"

fetch linuxdeploy-x86_64.AppImage          "$BASE_LD/linuxdeploy-x86_64.AppImage"
fetch linuxdeploy-plugin-qt-x86_64.AppImage "$BASE_QT/linuxdeploy-plugin-qt-x86_64.AppImage"
fetch appimagetool-x86_64.AppImage          "$BASE_AT/appimagetool-x86_64.AppImage"

echo "Tools ready in $TOOLS_DIR"
