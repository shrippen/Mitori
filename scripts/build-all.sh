#!/usr/bin/env bash
# Everything the CI builds, locally and in one go (faster than waiting for GitHub):
#   checks (scripts/check.sh), debug APK, AppImage, Flatpak bundle → dist/
# Usage: scripts/build-all.sh [check|apk|appimage|flatpak]...   (no argument: all of them)
# Temporary files go to ~/.cache/claude/mitori-*; the CI keeps running on every push as well.
set -euo pipefail
cd "$(dirname "$0")/.."
export TMPDIR="${TMPDIR:-$HOME/.cache/claude/mitori-tmp}"
mkdir -p "$TMPDIR"

steps=("$@")
[ ${#steps[@]} -gt 0 ] || steps=(check apk appimage flatpak)
for step in "${steps[@]}"; do
    echo "── $step ──"
    case "$step" in
        check) scripts/check.sh ;;
        apk) scripts/build-android.sh debug ;;
        appimage) scripts/build-appimage.sh ;;
        flatpak) scripts/build-flatpak.sh ;;
        *) echo "unknown step: $step" >&2; exit 2 ;;
    esac
done
if [ -d dist ]; then ls -la dist/*/; fi
