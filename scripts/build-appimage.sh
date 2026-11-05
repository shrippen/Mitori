#!/usr/bin/env bash
# Build Mitori (app/) as an AppImage that runs without Qt/KF6 installed: Qt via aqtinstall,
# KF6 (ECM, Kirigami) from source, bundled with linuxdeploy and its Qt plugin. The same
# versions as the Android build. Build on an old distro (CI: ubuntu-22.04) for an old glibc.
#
# Usage: ./scripts/build-appimage.sh   → dist/appimage/Mitori-<version>-x86_64.AppImage
# Needs: cmake, ninja, a C++20 compiler, git, python3 with PyYAML, pip, the GL/EGL and xkbcommon
# development files, libspeechd (pronunciation), libpulse (Qt Multimedia links it), curl, and network
# access.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
VERSION="$(sed -n 's/^project(mitori VERSION \([0-9.]*\).*/\1/p' "$ROOT/app/CMakeLists.txt")"
[ -n "$VERSION" ] || { echo "error: no version in app/CMakeLists.txt" >&2; exit 1; }

QT_VER="6.11.3"
KF6_VERSION="6.30.0"
QT="$HOME/Qt/$QT_VER/gcc_64"
PREFIX="${KF6_LINUX:-$HOME/kf6-linux}"
ARCH="$(uname -m)"

GREEN='\033[0;32m'; NC='\033[0m'
info() { echo -e "${GREEN}[INFO]${NC} $*"; }
# Build output is trimmed to its last lines; MITORI_VERBOSE=1 (CI) shows all of it.
show() { if [ -n "${MITORI_VERBOSE:-}" ]; then cat; else tail -"$1"; fi; }

WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT

# ── 1. Qt (with ShaderTools for Kirigami, Multimedia for the player, TextToSpeech for the glossary) ──
if [ ! -d "$QT/lib/cmake/Qt6ShaderTools" ] || [ ! -d "$QT/lib/cmake/Qt6Multimedia" ] \
   || [ ! -d "$QT/lib/cmake/Qt6TextToSpeech" ]; then
    info "Installing Qt $QT_VER (aqtinstall)..."
    pip install --user --break-system-packages aqtinstall 2>/dev/null || pip install --user aqtinstall
    export PATH="$HOME/.local/bin:$PATH"
    aqt install-qt linux desktop "$QT_VER" linux_gcc_64 -m qtshadertools qtmultimedia qtspeech -O "$HOME/Qt" 2>&1 | show 3
fi

# ── 2. KF6 into $PREFIX ──
build() { # name source-dir [cmake args...]
    local name="$1" src="$2"; shift 2
    info "Building $name..."
    cmake -S "$src" -B "$src/build" -G Ninja -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_PREFIX_PATH="$QT;$PREFIX" -DCMAKE_INSTALL_PREFIX="$PREFIX" -DCMAKE_INSTALL_LIBDIR=lib \
        -DBUILD_TESTING=OFF -DBUILD_QCH=OFF -DBUILD_PYTHON_BINDINGS=OFF "$@" 2>&1 | show 3
    cmake --build "$src/build" 2>&1 | show 3
    cmake --install "$src/build" > /dev/null
}
# Always lib/ (Debian/Ubuntu would pick lib/x86_64-linux-gnu, where linuxdeploy does not look).
if ! grep -qs "\"$KF6_VERSION\"" "$PREFIX"/lib/cmake/KF6Kirigami/KF6KirigamiConfigVersion.cmake; then
    rm -rf "${PREFIX:?}"   # another KF6 version (built against another Qt) must not mix with this one
    mkdir -p "$PREFIX"
    for repo in extra-cmake-modules kirigami; do
        git clone --quiet --depth 1 --branch "v$KF6_VERSION" "https://invent.kde.org/frameworks/$repo.git" "$WORK/$repo" &
    done
    wait
    build extra-cmake-modules "$WORK/extra-cmake-modules" -DBUILD_HTML_DOCS=OFF -DBUILD_MAN_DOCS=OFF -DBUILD_QTHELP_DOCS=OFF
    build Kirigami "$WORK/kirigami" -DBUILD_EXAMPLES=OFF
fi
LIBDIR="$PREFIX/lib"

# ── 3. The app, installed into an AppDir ──
info "Building Mitori $VERSION..."
APPDIR="$WORK/AppDir"
cmake -S "$ROOT/app" -B "$WORK/app" -G Ninja -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_PREFIX_PATH="$QT;$PREFIX" -DCMAKE_INSTALL_PREFIX=/usr 2>&1 | show 5
cmake --build "$WORK/app" 2>&1 | show 10
DESTDIR="$APPDIR" cmake --install "$WORK/app" > /dev/null
install -Dm644 "$ROOT/packaging/linux/io.github.shrippen.Mitori.desktop" "$APPDIR/usr/share/applications/io.github.shrippen.Mitori.desktop"
install -Dm644 "$ROOT/packaging/linux/io.github.shrippen.Mitori.metainfo.xml" "$APPDIR/usr/share/metainfo/io.github.shrippen.Mitori.metainfo.xml"
install -Dm644 "$ROOT/packaging/linux/io.github.shrippen.Mitori.png" "$APPDIR/usr/share/icons/hicolor/256x256/apps/io.github.shrippen.Mitori.png"

# ── 4. Bundle with linuxdeploy ──
TOOLS="$WORK/tools"; mkdir -p "$TOOLS"
for tool in linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage \
            linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-x86_64.AppImage; do
    curl -sSfL "https://github.com/$tool" -o "$TOOLS/$(basename "$tool")"
done
chmod +x "$TOOLS"/*.AppImage
# Unpacked and started directly: no FUSE needed (CI containers), and AppImageLauncher's binfmt
# hook on a desktop cannot get between linuxdeploy and its Qt plugin.
for tool in linuxdeploy linuxdeploy-plugin-qt; do
    (cd "$TOOLS" && "./$tool-x86_64.AppImage" --appimage-extract > /dev/null && mv squashfs-root "$tool.d")
    ln -s "$tool.d/AppRun" "$TOOLS/$tool"
    # linuxdeploy also looks for plugins named *.AppImage on PATH; only the unpacked one may answer.
    rm "$TOOLS/$tool-x86_64.AppImage"
done
info "Bundling the AppImage..."
# linuxdeploy-plugin-qt deploys Wayland's EGL client integration only for the pre-6.10 plugin
# name; without it Qt Quick can't create an OpenGL context on Wayland and shows nothing.
mkdir -p "$APPDIR/usr/plugins/wayland-graphics-integration-client"
cp "$QT/plugins/wayland-graphics-integration-client/libqt-plugin-wayland-egl.so" \
    "$APPDIR/usr/plugins/wayland-graphics-integration-client/"
# linuxdeploy-plugin-qt bundles every SQL driver of the Qt installation, and the MySQL, PostgreSQL
# … drivers need client libraries that are not there. Mitori uses SQLite only: a qmake that names
# a plugin folder with just that driver (the other plugin folders linked through).
PLUGINS="$WORK/qt-plugins"
rm -rf "$PLUGINS"; mkdir -p "$PLUGINS/sqldrivers"
for d in "$QT/plugins"/*; do
    [ "$(basename "$d")" = sqldrivers ] || ln -s "$d" "$PLUGINS/"
done
ln -s "$QT/plugins/sqldrivers/libqsqlite.so" "$PLUGINS/sqldrivers/"
cat > "$TOOLS/qmake" <<QMAKE
#!/usr/bin/env bash
set -o pipefail     # a failing qmake fails the wrapper too
"$QT/bin/qmake" "\$@" | sed 's|^QT_INSTALL_PLUGINS:.*|QT_INSTALL_PLUGINS:$PLUGINS|'
QMAKE
chmod +x "$TOOLS/qmake"
(
    cd "$WORK"
    export PATH="$TOOLS:$QT/bin:$PATH"
    export QMAKE="$TOOLS/qmake"
    export LD_LIBRARY_PATH="$LIBDIR:$QT/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
    export QML_SOURCES_PATHS="$ROOT/app/qml"
    export NO_STRIP=1   # linuxdeploy's strip is too old for current distros' libraries (.relr.dyn)
    export QML_MODULES_PATHS="$LIBDIR/qml"
    export EXTRA_PLATFORM_PLUGINS="libqwayland.so"   # Qt 6.10+: one Wayland plugin
    export EXTRA_QT_MODULES="svg"
    export LINUXDEPLOY_OUTPUT_VERSION="$VERSION"
    "$TOOLS/linuxdeploy" --appdir "$APPDIR" --plugin qt \
        -d "$APPDIR/usr/share/applications/io.github.shrippen.Mitori.desktop" \
        -i "$APPDIR/usr/share/icons/hicolor/256x256/apps/io.github.shrippen.Mitori.png" 2>&1 | show 15
    # Qt's own translations: the app ships its own catalogs.
    rm -rf "$APPDIR/usr/translations"
    # Pronunciation (glossary) through the desktop's speech-dispatcher; the mock engine is for tests.
    rm -f "$APPDIR"/usr/plugins/texttospeech/libqtexttospeech_mock.so
    [ -f "$APPDIR/usr/plugins/texttospeech/libqtexttospeech_speechd.so" ] \
        || { echo "error: no speech-dispatcher plugin in the AppImage (qtspeech, libspeechd)" >&2; exit 1; }
    "$TOOLS/linuxdeploy" --appdir "$APPDIR" --output appimage 2>&1 | show 15
)
mkdir -p "$ROOT/dist/appimage"
OUT="$ROOT/dist/appimage/Mitori-$VERSION-$ARCH.AppImage"
mv "$WORK"/Mitori-*.AppImage "$OUT"
info "AppImage ready: $OUT"
