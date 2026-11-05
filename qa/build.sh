#!/usr/bin/env bash
# The binary qa/session.sh starts: as the user gets it, with the style Shinki.
set -euo pipefail
repo="$(cd "$(dirname "$0")/.." && pwd)"
build="${MITORI_QA_BUILD:-$HOME/.cache/claude/mitori-qa-build}"
cmake -S "$repo/app" -B "$build" -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo -DMITORI_DEMO=OFF > /dev/null
cmake --build "$build" --target mitori
echo "built: $build/mitori"
