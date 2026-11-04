#!/bin/sh
# Schreibt die Programmdaten für die Clickdummies als daten.js (file:// erlaubt kein fetch).
set -e
cd "$(dirname "$0")/../.."
DATEN="${MITORI_DATA_DIR:-$PWD/third_party/mitori-daten}"
python3 "$DATEN/tools/daten.py" >/dev/null
{ printf 'window.MITORI = '; cat build/daten/mitori-daten.json; printf ';\n'; } > entwuerfe/clickdummy/daten.js
# Lehrgänge aus dem öffentlichen Kalender (ohne Netz bleibt die alte lehrgaenge.js)
python3 entwuerfe/clickdummy/lehrgaenge.py || echo "lehrgaenge: Kalender nicht erreichbar, alte Daten bleiben" >&2
