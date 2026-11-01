#!/usr/bin/env bash
# Fails when a built package carries the internal demo (agent.md, "Demo"): a demo build
# (-DMITORI_DEMO=ON) has the mark MITORI-DEMO-BUILD in its binary.
# Usage: scripts/check-no-demo.sh <unpacked package dir>...
set -euo pipefail
[ $# -gt 0 ] || { echo "usage: $0 <dir>..." >&2; exit 2; }
if grep -rlaF "MITORI-DEMO-BUILD" "$@"; then
    echo "check-no-demo: demo code in the package (files above)" >&2
    exit 1
fi
echo "check-no-demo: no demo in $*"
