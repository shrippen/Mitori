#!/usr/bin/env bash
# Cold start and memory of the app on this machine: qa/perf.sh [runs]. MITORI_BIN as in session.sh.
# Starts until the window is there, then reads RSS after 3 s (with five years of data if the
# tst_perf run left them: MITORI_PERF_DATA=<build>/perf-data/data).
set -euo pipefail
app="${MITORI_BIN:-$HOME/.cache/claude/mitori-qa-build/mitori}"
if [ ! -x "$app" ]; then
    echo "perf: $app not found; qa/build.sh first" >&2
    exit 2
fi
runs="${1:-5}"
base="$HOME/.cache/claude/mitori-qa"
mkdir -p "$base"
work="$(mktemp -d -p "$base" perf.XXXXXX)"
trap 'rm -rf "$work"' EXIT
mkdir -p "$work/runtime" "$work/home" "$work/config/shrippen.github.io"
chmod 700 "$work/runtime"
printf '[look]\nstyle=4\nwelcomed=true\n' > "$work/config/shrippen.github.io/Mitori.conf"
data="${MITORI_PERF_DATA:-$work/data}"
mkfifo "$work/display"
Xvfb -displayfd 3 -screen 0 1280x800x24 -nolisten tcp 3> "$work/display" > /dev/null 2>&1 &
xvfb="$!"
read -r display < "$work/display"
trap 'kill "$xvfb" 2>/dev/null || true; rm -rf "$work"' EXIT
for i in $(seq "$runs"); do
    started="$(date +%s%N)"
    env -i PATH="$PATH" QT_QPA_PLATFORM=xcb DISPLAY=":$display" HOME="$work/home" \
        XDG_RUNTIME_DIR="$work/runtime" XDG_CONFIG_HOME="$work/config" XDG_DATA_HOME="$data" \
        XDG_CACHE_HOME="$work/cache" QT_QUICK_BACKEND=software MITORI_DEVICE_LEVELS=memory \
        MITORI_SPEECH_ENGINE=mock "$app" > /dev/null 2>&1 &
    pid="$!"
    until DISPLAY=":$display" xdotool search --onlyvisible --pid "$pid" > /dev/null 2>&1; do
        if ! kill -0 "$pid" 2>/dev/null; then
            echo "perf: the app ended before its window" >&2
            exit 1
        fi
        sleep 0.02
    done
    ms="$(( ($(date +%s%N) - started) / 1000000 ))"
    sleep 3
    rss="$(awk '/VmRSS/ {print int($2 / 1024)}' "/proc/$pid/status")"
    kill "$pid"; wait "$pid" 2>/dev/null || true
    echo "run $i: window after $ms ms, RSS $rss MiB"
done
