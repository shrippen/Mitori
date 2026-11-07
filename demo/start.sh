#!/bin/sh
# Mitori with the Studio Weber demo data (Emi Hartmann's practice, demo/world.json): own scratch
# folders, no videos, no network sources (course calendar and video manifest are local files).
#   demo/start.sh [de|en] [leinen|kante|shinki|breeze|material]
# With SHOT_DIR and SHOT_PLAN (demo/tools/screenshots.py in shrippen.github.io) it takes the shots
# of demo/shots.json offscreen and quits, once per style the shots name (`style`, default leinen).
# The demo build never ships: MITORI_DEMO is off by default and every release package is checked
# (scripts/check-no-demo.sh).
set -eu
cd "$(dirname "$0")/.."
build=${MITORI_DEMO_BUILD:-build-demo}
cmake -S app -B "$build" -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo -DMITORI_DEMO=ON -DMITORI_BUILD_TESTS=OFF >/dev/null
cmake --build "$build" >/dev/null

export DEMO_LANG="${1:-${DEMO_LANG:-de}}"
style="${2:-${DEMO_STYLE:-leinen}}"
scratch=$(mktemp -d)
trap 'rm -rf "$scratch"' EXIT
export XDG_CONFIG_HOME="$scratch/config" XDG_DATA_HOME="$scratch/data" XDG_CACHE_HOME="$scratch/cache"
export MITORI_DEMO_WORLD="$PWD/demo/world.json"
export MITORI_COURSES_CALENDAR="file://$scratch/lehrgaenge.ics"
export MITORI_BUNDLES_MANIFEST="file://$scratch/no-manifest.json"
mkdir -p "$scratch/no-videos"
export MITORI_VIDEOS="$scratch/no-videos"
export MITORI_DEVICE_LEVELS=memory
export LANGUAGE="$DEMO_LANG"

if [ -z "${SHOT_PLAN:-}" ]; then
    DEMO_STYLE="$style" "$build/mitori"
    exit
fi

# Shots: one run per style, offscreen, in the style's order of first appearance.
export QT_QPA_PLATFORM="${QT_QPA_PLATFORM:-offscreen}" QT_SCALE_FACTOR="${QT_SCALE_FACTOR:-2}"
styles=$(python3 -c 'import json, os; print(" ".join(dict.fromkeys(s.get("style", "leinen") for s in json.loads(os.environ["SHOT_PLAN"]))))')
for s in $styles; do
    plan=$(python3 -c 'import json, os, sys; print(json.dumps([x for x in json.loads(os.environ["SHOT_PLAN"]) if x.get("style", "leinen") == sys.argv[1]]))' "$s")
    SHOT_PLAN="$plan" DEMO_STYLE="$s" "$build/mitori"
done
