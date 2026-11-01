#!/usr/bin/env bash
# The local checks before a commit, the same as .github/workflows/checks.yml:
# data, release notes, Python tests, generated data, the app with its C++ tests, qmllint (reports only).
# Build directory: MITORI_BUILD, default ~/.cache/claude/mitori-build.
set -euo pipefail
cd "$(dirname "$0")/.."
BUILD="${MITORI_BUILD:-$HOME/.cache/claude/mitori-build}"

python3 tools/daten.py
scripts/release-notes.sh --check
python3 -m pytest -q tests
# The generated YAML (from the PDFs in quellen/) must match its tools: a hand edit shows as a diff.
python3 tools/kommentare.py > /dev/null
python3 tools/lexikon.py > /dev/null
git diff --exit-code -- daten/shinki-toho-kommentare.yaml daten/lexikon.yaml
cmake -S app -B "$BUILD" -G Ninja -DCMAKE_BUILD_TYPE=Debug -DMITORI_BUILD_TESTS=ON > /dev/null
cmake --build "$BUILD"
QT_QPA_PLATFORM=offscreen ctest --test-dir "$BUILD" --output-on-failure
scripts/qmllint.sh || echo "qmllint: findings (reported, not blocking)"
