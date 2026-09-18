#!/usr/bin/env bash
#
# Build a self-contained Termite AppImage.
#
#   packaging/build-appimage.sh
#
# Produces:  dist/Termite-<version>-x86_64.AppImage
#
# This script only *builds* the AppImage. It does not install it, copy it into
# any system directory, register it, or integrate it with the desktop — that is
# left to the user.
#
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
cd "$PROJECT_ROOT"

TOOLS_DIR="$SCRIPT_DIR/tools"
APPDIR="$SCRIPT_DIR/AppDir"
BUILD_DIR="$PROJECT_ROOT/build"
DIST_DIR="$PROJECT_ROOT/dist"

VERSION="$(sed -n 's/^[[:space:]]*VERSION[[:space:]]*\([0-9][0-9.]*\).*/\1/p' "$PROJECT_ROOT/CMakeLists.txt" | head -1)"
VERSION="${VERSION:-0.0.0}"
OUTPUT="$DIST_DIR/Termite-${VERSION}-x86_64.AppImage"

export APPIMAGE_EXTRACT_AND_RUN=1   # tools are AppImages; avoid needing FUSE
export ARCH=x86_64
# linuxdeploy's bundled tools mishandle the RELR relative relocations
# (.relr.dyn) that Fedora 44 emits: strip cannot parse them and patchelf
# corrupts binaries when it rewrites RPATH (repeatedly, on the same file).
# Termite's AppRun sets LD_LIBRARY_PATH/QT_PLUGIN_PATH, so RPATH rewriting is
# unnecessary: disable both by pointing linuxdeploy at a no-op patchelf.
export NO_STRIP=1
export PATCHELF=/bin/true
# Termite forces QT_QPA_PLATFORM=wayland, but linuxdeploy-plugin-qt only
# deploys libqxcb.so by default. Ask for the Wayland platform plugin (offscreen
# and minimal are included so the bundle can be smoke-tested headlessly).
export EXTRA_PLATFORM_PLUGINS="libqwayland.so;libqoffscreen.so;libqminimal.so"

LINUXDEPLOY="$TOOLS_DIR/linuxdeploy-x86_64.AppImage"
QT_PLUGIN="$TOOLS_DIR/linuxdeploy-plugin-qt-x86_64.AppImage"
APPIMAGETOOL="$TOOLS_DIR/appimagetool-x86_64.AppImage"

for tool in "$LINUXDEPLOY" "$QT_PLUGIN" "$APPIMAGETOOL"; do
    if [ ! -x "$tool" ]; then
        echo "error: missing packaging tool: $tool" >&2
        echo "       run packaging/fetch-tools.sh first" >&2
        exit 1
    fi
done

# linuxdeploy discovers plugins by name on PATH.
ln -sf "$QT_PLUGIN" "$TOOLS_DIR/linuxdeploy-plugin-qt"
export PATH="$TOOLS_DIR:$PATH"

echo "==> Building Termite $VERSION"
cmake -S "$PROJECT_ROOT" -B "$BUILD_DIR" -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build "$BUILD_DIR"

echo "==> Assembling AppDir"
rm -rf "$APPDIR"
DESTDIR="$APPDIR" cmake --install "$BUILD_DIR" --prefix /usr

# AppImage icon (256x256) for the AppDir root and the hicolor theme.
mkdir -p "$APPDIR/usr/share/icons/hicolor/256x256/apps"
if command -v convert >/dev/null 2>&1; then
    convert "$PROJECT_ROOT/assets/icon.png" -resize 256x256 \
        "$APPDIR/usr/share/icons/hicolor/256x256/apps/termite.png"
else
    cp "$PROJECT_ROOT/assets/icon.png" \
        "$APPDIR/usr/share/icons/hicolor/256x256/apps/termite.png"
fi
cp "$APPDIR/usr/share/icons/hicolor/256x256/apps/termite.png" "$APPDIR/termite.png"

echo "==> Bundling dependencies (Qt + libraries)"

# The Wayland shell/graphics integration plugins are not pulled in
# automatically; copy them before linuxdeploy so their dependencies are
# resolved and their rpaths are fixed along with everything else.
HOST_QT_PLUGINS="$(qmake -query QT_INSTALL_PLUGINS 2>/dev/null || echo /usr/lib64/qt6/plugins)"
for rel in wayland-graphics-integration-client wayland-shell-integration wayland-decoration-client; do
    if [ -d "$HOST_QT_PLUGINS/$rel" ]; then
        echo "==> Bundling Qt plugin dir: $rel"
        mkdir -p "$APPDIR/usr/plugins/$rel"
        cp -a "$HOST_QT_PLUGINS/$rel/." "$APPDIR/usr/plugins/$rel/"
    fi
done

"$LINUXDEPLOY" --appdir "$APPDIR" \
    --executable "$APPDIR/usr/bin/termite" \
    --desktop-file "$APPDIR/usr/share/applications/termite.desktop" \
    --icon-file "$APPDIR/usr/share/icons/hicolor/256x256/apps/termite.png" \
    --plugin qt \
    --custom-apprun "$SCRIPT_DIR/AppRun"

# qtermwidget ships color schemes and translations as data files (not ELF), so
# linuxdeploy cannot pick them up; copy them explicitly.
if [ -d /usr/share/qtermwidget6 ]; then
    echo "==> Bundling qtermwidget6 data"
    mkdir -p "$APPDIR/usr/share/qtermwidget6"
    cp -a /usr/share/qtermwidget6/. "$APPDIR/usr/share/qtermwidget6/"
fi

if [ "${TERMITE_SKIP_PACKAGE:-0}" = "1" ]; then
    echo "AppDir ready at $APPDIR (packaging skipped)"
    exit 0
fi

echo "==> Packaging AppImage"
mkdir -p "$DIST_DIR"
rm -f "$OUTPUT"
"$APPIMAGETOOL" "$APPDIR" "$OUTPUT"

echo
echo "Built: $OUTPUT"
