#!/usr/bin/env bash
# Build the Flatpak bundle from packaging/flatpak/io.github.shrippen.Mitori.yml, as the release
# workflow does: → dist/flatpak/Mitori-<version>-<arch>.flatpak (arch of this machine, e.g. aarch64)
# Work directory: MITORI_FLATPAK_WORK, default ~/.cache/claude/mitori-flatpak (kept for faster rebuilds).
# Needs flatpak-builder; runtime and SDK (org.kde.Platform/Sdk) come from Flathub.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
VERSION="${1:-$(sed -n 's/^project(mitori VERSION \([0-9.]*\).*/\1/p' "$ROOT/app/CMakeLists.txt")}"
WORK="${MITORI_FLATPAK_WORK:-$HOME/.cache/claude/mitori-flatpak}"
APP_ID=io.github.shrippen.Mitori

flatpak remote-add --user --if-not-exists flathub https://dl.flathub.org/repo/flathub.flatpakrepo
flatpak-builder --user --install-deps-from=flathub --disable-rofiles-fuse --force-clean \
    --state-dir="$WORK/state" --repo="$WORK/repo" "$WORK/build" "$ROOT/packaging/flatpak/$APP_ID.yml"
"$ROOT/scripts/check-no-demo.sh" "$WORK/build/files"
mkdir -p "$ROOT/dist/flatpak"
OUT="$ROOT/dist/flatpak/Mitori-$VERSION-$(flatpak --default-arch).flatpak"
flatpak build-bundle "$WORK/repo" "$OUT" "$APP_ID" --runtime-repo=https://dl.flathub.org/repo/flathub.flatpakrepo
echo "Flatpak ready: $OUT (flatpak install --user $OUT)"
