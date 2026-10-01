#!/usr/bin/env bash
# Two identical runner invocations; prints SHA-256 of both event dumps (and
# WAVs) and fails when they differ. Extra arguments go to the runner.
#   replay_check.sh <max_dispatches> [runner args...]
# Run from the repository root. Outputs land in .tmp/replay/.
set -euo pipefail
dispatches=${1:?max dispatches}
shift
export TEMP="$PWD/.tmp" TMP="$PWD/.tmp" TMPDIR="$PWD/.tmp"
mkdir -p .tmp/replay
for i in 1 2; do
    build/p3p_pc_bootstrap.exe --run-until-blocker --max-dispatches "$dispatches" --ms0 .tmp/ms0 \
        --wav ".tmp/replay/run$i.wav" --dump-events ".tmp/replay/run$i.json" "$@" > ".tmp/replay/run$i.log" 2>&1 || true
done
grep -a "Stop Reason" .tmp/replay/run1.log || true
sha256sum .tmp/replay/run1.json .tmp/replay/run2.json .tmp/replay/run1.wav .tmp/replay/run2.wav
a=$(sha256sum < .tmp/replay/run1.json); b=$(sha256sum < .tmp/replay/run2.json)
c=$(sha256sum < .tmp/replay/run1.wav); d=$(sha256sum < .tmp/replay/run2.wav)
if [ "$a" = "$b" ] && [ "$c" = "$d" ]; then echo "REPLAY IDENTICAL"; else echo "REPLAY MISMATCH"; exit 1; fi
