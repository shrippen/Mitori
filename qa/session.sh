#!/usr/bin/env bash
# One isolated run of Mitori in its own X server, as a persona (qa/QA.md). It never sees the desktop,
# the real profile or the real screen brightness:
#
#   Xvfb :N ─► mitori (xcb, own XDG folders, levels in memory) ─► --run CMD ─► clean up
#
#   qa/session.sh [--size 412x915] [--scale 2] [--style shinki|kante|leinen|material|breeze]
#                 [--lang de|en] [--new] [--no-videos] [--keep DIR] [--run CMD] [-- <app arguments>]
#
#   --size       screen (phone 412x915, tablet 1280x800)
#   --new        first start: the wizard; without it the wizard counts as done
#   --no-videos  a device without videos
#   --keep DIR   profile in DIR, kept: the next session goes on with it
#   --run CMD    runs with DISPLAY set once the window is there, then stops the app; see qa/lib.sh
#
# Binary: $MITORI_BIN, else ~/.cache/claude/mitori-qa-build/mitori. The videos stay on this machine
# and in the session's screen: screenshots with a frame of a video never leave it (agent.md).
set -euo pipefail
repo="$(cd "$(dirname "$0")/.." && pwd)"

size="412x915" scale=1 style="shinki" lang="de" new=0 videos=1 keep="" run=""
while [ $# -gt 0 ]; do
    case "$1" in
        --size) size="$2"; shift 2 ;;
        --scale) scale="$2"; shift 2 ;;
        --style) style="$2"; shift 2 ;;
        --lang) lang="$2"; shift 2 ;;
        --new) new=1; shift ;;
        --no-videos) videos=0; shift ;;
        --keep) keep="$2"; shift 2 ;;
        --run) run="$2"; shift 2 ;;
        --) shift; break ;;
        *) echo "session: unknown option $1 (see the head of this script)" >&2; exit 2 ;;
    esac
done

app="${MITORI_BIN:-$HOME/.cache/claude/mitori-qa-build/mitori}"
if [ ! -x "$app" ]; then
    echo "session: $app not found; qa/build.sh first" >&2
    exit 2
fi

base="$HOME/.cache/claude/mitori-qa"
mkdir -p "$base"
work="$(mktemp -d -p "$base" run.XXXXXX)"
profile="${keep:-$work/profile}"
# Its own HOME too: file dialogs must not show the real folders.
mkdir -p "$work/runtime" "$profile/data" "$profile/config/shrippen.github.io" "$profile/cache" "$profile/home/Documents"
chmod 700 "$work/runtime"

# The look's number and the wizard, as the first start would leave them.
declare -A styles=([breeze]=0 [material]=1 [kante]=2 [leinen]=3 [shinki]=4)
conf="$profile/config/shrippen.github.io/Mitori.conf"
if [ ! -e "$conf" ] && [ "$new" = 0 ]; then
    printf '[look]\nstyle=%s\nwelcomed=true\n' "${styles[$style]}" > "$conf"
fi

xvfb_pid="" app_pid=""
cleanup() {
    if [ -n "$app_pid" ]; then kill -- "-$app_pid" 2>/dev/null || true; fi
    if [ -n "$xvfb_pid" ]; then kill "$xvfb_pid" 2>/dev/null || true; fi
    wait 2>/dev/null || true
    rm -rf "$work"
}
trap cleanup EXIT INT TERM

# Xvfb picks a free display itself: two sessions started together cannot meet on one.
mkfifo "$work/display"
Xvfb -displayfd 3 -screen 0 "${size}x24" -nolisten tcp 3> "$work/display" > /dev/null 2>&1 &
xvfb_pid=$!
read -r display < "$work/display"

clean=(env -u WAYLAND_DISPLAY -u XDG_SESSION_TYPE -u DBUS_SESSION_BUS_ADDRESS -u MITORI_VIDEOS
    QT_QPA_PLATFORM=xcb DISPLAY=":$display" XDG_RUNTIME_DIR="$work/runtime" HOME="$profile/home"
    XDG_DATA_HOME="$profile/data" XDG_CONFIG_HOME="$profile/config" XDG_CACHE_HOME="$profile/cache"
    QT_SCALE_FACTOR="$scale" LANGUAGE="$lang" LANG="${lang}_DE.UTF-8" LC_ALL="" QT_QUICK_BACKEND=software
    MITORI_DEVICE_LEVELS=memory MITORI_SPEECH_ENGINE=mock)
if [ "$videos" = 1 ]; then
    clean+=(MITORI_VIDEOS="${MITORI_QA_VIDEOS:-$repo/../Mitori Videos}")
fi

log="${MITORI_QA_LOG:-$profile/session.log}"
setsid "${clean[@]}" "$app" "$@" > "$log" 2>&1 &
app_pid=$!

# The window fills the screen: no window manager here.
export DISPLAY=":$display"
win=""
for _ in $(seq 100); do
    win="$(xdotool search --onlyvisible --pid "$app_pid" 2>/dev/null | head -1 || true)"
    [ -n "$win" ] && break
    sleep 0.1
done
if [ -z "$win" ]; then
    echo "session: no window; log: $log" >&2
    tail -5 "$log" >&2
    exit 1
fi
w="${size%x*}"; h="${size#*x}"
xdotool windowmove "$win" 0 0 windowsize "$win" "$w" "$h" windowfocus "$win"
sleep 2     # the new size has reached the layout

if [ -z "$run" ]; then
    wait "$app_pid"
    exit $?
fi
MITORI_QA_WIN="$win" MITORI_QA_LIB="$repo/qa/lib.sh" MITORI_QA_LOG_FILE="$log" bash -c "$run"
