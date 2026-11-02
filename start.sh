#!/usr/bin/env bash
# Mitori bauen (falls nötig) und starten: ./start.sh [Argumente für die App]
# Baut nach build-app/ (RelWithDebInfo; nur geänderte Dateien). Liegt neben dem Repo der Ordner
# "Mitori Videos" und ist MITORI_VIDEOS nicht gesetzt, nimmt die App die Videos von dort
# (solange in den Einstellungen kein anderer Ordner gewählt ist).
set -euo pipefail
cd "$(dirname "$0")"
BUILD=build-app

if [ ! -f "$BUILD/CMakeCache.txt" ]; then
    cmake -S app -B "$BUILD" -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo > /dev/null
fi
cmake --build "$BUILD"

if [ -z "${MITORI_VIDEOS:-}" ] && [ -d "../Mitori Videos" ]; then
    MITORI_VIDEOS="$(cd "../Mitori Videos" && pwd)"
    export MITORI_VIDEOS
fi
exec "$BUILD/mitori" "$@"
