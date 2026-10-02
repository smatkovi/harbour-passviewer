#!/bin/sh
# Builds and runs the QML checker over meego/qml. See meego/tests/qml_check.cpp.
set -e
HERE=$(cd "$(dirname "$0")/../.." && pwd)
LOG=$HERE/build/meego/check-build.log
mkdir -p "$(dirname "$LOG")"
if ! sh "$HERE/meego/build.sh" check > "$LOG" 2>&1; then
    grep -E "error:|Error [0-9]" "$LOG" | head -30
    echo "== build failed, full log in $LOG" >&2
    exit 1
fi
cd "$HERE"
exec build/meego/check/harbour-passviewer meego/qml meego/tests/stubs
