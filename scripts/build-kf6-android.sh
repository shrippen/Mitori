#!/usr/bin/env bash
# Build the KF6 prefix the Android app needs: extra-cmake-modules, KCoreAddons (a Kirigami
# dependency) and Kirigami, cross-compiled for android_arm64_v8a, and QtKeychain (static: nothing
# more to package).
#
# Usage: ./scripts/build-kf6-android.sh [-f|--force] [prefix]   (prefix default $HOME/kf6-android)
# Skips what the prefix already has in this version. Needs the NDK and Qt for Android + host tools
# (scripts/build-android.sh installs both) and network access to invent.kde.org and github.com.
set -euo pipefail

FORCE=0
if [ "${1:-}" = "-f" ] || [ "${1:-}" = "--force" ]; then FORCE=1; shift; fi
PREFIX="${1:-$HOME/kf6-android}"

KF6_VERSION="6.30.0"
QTKEYCHAIN_VERSION="0.17.0"
SDK_ROOT="${ANDROID_SDK_ROOT:-$HOME/android-sdk}"
QT_VER="6.11.3"
QT_ANDROID="$HOME/Qt/$QT_VER/android_arm64_v8a"
QT_HOST="$HOME/Qt/$QT_VER/gcc_64"
ANDROID_PLATFORM_LEVEL="android-28"   # QT_ANDROID_MIN_SDK_VERSION in app/CMakeLists.txt

RED='\033[0;31m'; GREEN='\033[0;32m'; NC='\033[0m'
info()  { echo -e "${GREEN}[INFO]${NC} $*"; }
error() { echo -e "${RED}[ERROR]${NC} $*"; exit 1; }

NDK_PATH=$(ls -d "$SDK_ROOT"/ndk/27.2.* 2>/dev/null | head -1)
[ -d "$NDK_PATH" ] || error "Android NDK not found under $SDK_ROOT/ndk (scripts/build-android.sh installs it)"
[ -d "$QT_ANDROID" ] && [ -d "$QT_HOST" ] || error "Qt $QT_VER not found under \$HOME/Qt (scripts/build-android.sh installs it)"

WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT

CROSS_ARGS=(
    -DCMAKE_TOOLCHAIN_FILE="$NDK_PATH/build/cmake/android.toolchain.cmake"
    -DANDROID_ABI=arm64-v8a -DANDROID_PLATFORM="$ANDROID_PLATFORM_LEVEL"
    -DCMAKE_BUILD_TYPE=Release
    -DCMAKE_PREFIX_PATH="$QT_ANDROID;$PREFIX"
    -DCMAKE_INSTALL_PREFIX="$PREFIX"
    -DCMAKE_FIND_ROOT_PATH_MODE_PACKAGE=BOTH
    -DQT_HOST_PATH="$QT_HOST"
    -DECM_DIR="$PREFIX/share/ECM/cmake"
    -DBUILD_TESTING=OFF -DBUILD_QCH=OFF
)

# The keystore's access (store/secretstore), added to a prefix that lacks it.
keychain() {
    if grep -qs "\"$QTKEYCHAIN_VERSION\"" "$PREFIX/lib/cmake/Qt6Keychain/Qt6KeychainConfigVersion.cmake"; then
        return
    fi
    info "Building QtKeychain $QTKEYCHAIN_VERSION..."
    git clone --quiet --depth 1 --branch "$QTKEYCHAIN_VERSION" https://github.com/frankosterfeld/qtkeychain.git "$WORK/qtkeychain"
    cmake -B "$WORK/qtkeychain/build" -S "$WORK/qtkeychain" "${CROSS_ARGS[@]}" \
        -DBUILD_SHARED_LIBS=OFF -DBUILD_TRANSLATIONS=OFF
    cmake --build "$WORK/qtkeychain/build" -j"$(nproc)"
    cmake --install "$WORK/qtkeychain/build"
}

if [ "$FORCE" -ne 1 ] && grep -qs "\"$KF6_VERSION\"" "$PREFIX/lib/cmake/KF6Kirigami/KF6KirigamiConfigVersion.cmake"; then
    info "$PREFIX already has KF6Kirigami $KF6_VERSION, skipping (-f rebuilds)"
    keychain
    exit 0
fi
# Another KF6 version (built against another Qt) must not mix with this one.
rm -rf "${PREFIX:?}"
mkdir -p "$PREFIX"

info "Cloning extra-cmake-modules, kcoreaddons, kirigami @ v$KF6_VERSION..."
for repo in extra-cmake-modules kcoreaddons kirigami; do
    git clone --quiet --depth 1 --branch "v$KF6_VERSION" "https://invent.kde.org/frameworks/$repo.git" "$WORK/$repo" &
done
wait

info "Building extra-cmake-modules..."
cmake -B "$WORK/extra-cmake-modules/build" -S "$WORK/extra-cmake-modules" \
    -DCMAKE_INSTALL_PREFIX="$PREFIX" -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF \
    -DBUILD_HTML_DOCS=OFF -DBUILD_MAN_DOCS=OFF -DBUILD_QTHELP_DOCS=OFF
cmake --build "$WORK/extra-cmake-modules/build" -j"$(nproc)"
cmake --install "$WORK/extra-cmake-modules/build"

info "Building KCoreAddons..."
cmake -B "$WORK/kcoreaddons/build" -S "$WORK/kcoreaddons" "${CROSS_ARGS[@]}"
cmake --build "$WORK/kcoreaddons/build" -j"$(nproc)"
cmake --install "$WORK/kcoreaddons/build"

info "Building Kirigami..."
cmake -B "$WORK/kirigami/build" -S "$WORK/kirigami" "${CROSS_ARGS[@]}" \
    -DBUILD_EXAMPLES=OFF -DDESKTOP_ENABLED=OFF
cmake --build "$WORK/kirigami/build" -j"$(nproc)"
cmake --install "$WORK/kirigami/build"

keychain

info "KF6 for Android ready at $PREFIX"
