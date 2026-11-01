#!/usr/bin/env bash
# Baut das Prüfungsprogramm-Heft.
#   ./build.sh           build/pruefungsprogramm.pdf
#   ./build.sh entwurf   build/pruefungsprogramm-entwurf.pdf (mit Marken für offene Fragen)
# Vorher prüft tools/daten.py die Daten in ../daten/ und schreibt ../build/daten/.
set -euo pipefail
cd "$(dirname "$0")"

art="${1:-heft}"
name=pruefungsprogramm
optionen=""
if [[ "$art" == entwurf ]]; then name=pruefungsprogramm-entwurf; optionen="\\def\\mitorientwurf{}"; fi

python3 ../tools/daten.py
mkdir -p build
latexmk -lualatex -interaction=nonstopmode -halt-on-error -outdir=build -jobname="$name" \
  -usepretex="$optionen" pruefungsprogramm.tex >/dev/null \
  || { grep -n -A8 '^!' "build/$name.log" | head -40; exit 1; }
echo "build/$name.pdf"
