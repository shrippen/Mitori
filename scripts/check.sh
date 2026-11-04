#!/usr/bin/env bash
# The local checks before a commit, the same as .github/workflows/checks.yml:
# data (from the mitori-daten submodul), release notes, the app with its C++ tests,
# qmllint (reports only). Build directory: MITORI_BUILD, default ~/.cache/claude/mitori-build.
# The Python tests of the data live in the motodaten-repo itself; run there too.
set -euo pipefail
cd "$(dirname "$0")/.."
BUILD="${MITORI_BUILD:-$HOME/.cache/claude/mitori-build}"
DATEN="${MITORI_DATA_DIR:-$PWD/third_party/mitori-daten}"

python3 "$DATEN/tools/daten.py"
scripts/release-notes.sh --check
# Generated YAML in the data repo must match its tools too — skip if the submodul is missing.
if [ -d "$DATEN" ]; then
    (cd "$DATEN" && python3 kommentare.py > /dev/null && python3 lexikon.py > /dev/null && \
        git diff --exit-code -- daten/shinki-toho-kommentare.yaml daten/lexikon.yaml)
fi
cmake -S app -B "$BUILD" -G Ninja -DCMAKE_BUILD_TYPE=Debug -DMITORI_BUILD_TESTS=ON > /dev/null
cmake --build "$BUILD"
QT_QPA_PLATFORM=offscreen ctest --test-dir "$BUILD" --output-on-failure
# QML-Lint deckt jetzt nur app/qml; Shinki liegt im Submodul und wird dort geprüft.
scripts/qmllint.sh || echo "qmllint: findings (reported, not blocking)"
