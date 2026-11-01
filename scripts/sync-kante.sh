#!/usr/bin/env bash
# Copies the Kante QML module from the design system repo into the app (app/qml/Kante).
# Source: KANTE_DS, default ../Kante (https://github.com/shrippen/Kante).
# The copy is not edited here; change the design system and sync again.
set -euo pipefail
cd "$(dirname "$0")/.."

DS="${KANTE_DS:-../Kante}"
if [ ! -f "$DS/qml/Kante/qmldir" ]; then
    echo "sync-kante: no Kante module in $DS/qml (set KANTE_DS)" >&2
    exit 1
fi

rm -rf app/qml/Kante
cp -r "$DS/qml/Kante" app/qml/Kante

echo "sync-kante: $(git -C "$DS" describe --always --dirty 2>/dev/null || echo unknown) -> app/qml/Kante"
