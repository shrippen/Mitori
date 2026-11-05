# For --run of qa/session.sh: source "$MITORI_QA_LIB", then tap, type, key, shot, warnings.
tap() { xdotool mousemove "$1" "$2" click 1; sleep "${3:-0.6}"; }
swipe() { xdotool mousemove "$1" "$2" mousedown 1; sleep 0.1; xdotool mousemove "$3" "$4"; sleep 0.1; xdotool mouseup 1; sleep 0.6; }
type_text() { xdotool type --delay 40 "$1"; sleep 0.4; }
key() { xdotool key "$@"; sleep 0.5; }
# shot FILE: the screen as a picture (it may show a video: keep it on this machine).
shot() { import -window root "$1"; }
# What the app said (warnings, errors) so far.
warnings() { grep -vE 'propertyCache|portal|QML debugging' "$MITORI_QA_LOG_FILE" | sort | uniq -c | sort -rn | head -"${1:-20}"; }
