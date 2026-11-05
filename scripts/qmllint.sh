#!/usr/bin/env bash
# qmllint over the app's QML and Shinki's,
# as the CI runs it. Exit 1 on any finding.
# Every finding as JSON: scripts/qmllint.sh --json -
#
# Off, on purpose:
#   block-scope-var-declaration  Qt 6.12 reports every `var` in a block; switch file by file with a
#                                check (agent.md, "QML lint").
# The Mitori module (C++ types) comes from a build: MITORI_BUILD, default ~/.cache/claude/mitori-build.
# Findings in app/qml/Kante are fixed in Kante (scripts/sync-kante.sh); Shinki findings in
# shinki/qml/Shinki here.
set -uo pipefail
cd "$(dirname "$0")/.."
QMLLINT=/usr/lib/qt6/bin/qmllint
[ -x "$QMLLINT" ] || QMLLINT=$(command -v qmllint6 || command -v qmllint)
BUILD="${MITORI_BUILD:-$HOME/.cache/claude/mitori-build}"
mapfile -t files < <(find app/qml shinki/qml -name '*.qml' -not -path 'app/qml/Kante/*' | sort)
"$QMLLINT" --block-scope-var-declaration disable --max-warnings 0 -I app/qml -I "$BUILD/qml" "$@" "${files[@]}" \
    && echo "qmllint: no findings"
