#!/usr/bin/env bash
# One iteration of finding an in-game route on the PC runner: run an input
# script (optionally loading a save), dump frames from a vblank on, and write
# a contact sheet to look at.
#
# usage: try_route.sh <name> <input script> <max dispatches> <from vblank> <frame every> [ms0 dir] [savedata policy]
#   ms0 dir: memory stick with saves (default .tmp/route/ms0; a copy, the run may write saves)
#   savedata policy: latest | cancel | <slot index> (runner --savedata)
# Output: .tmp/route/<name>/ (frames, events json, log) and .tmp/route/<name>.png
# Rasterization starts 50 vblanks before <from vblank> (--render-from) to keep runs short.
set -euo pipefail
cd "$(dirname "$0")/../../../.."
name=$1 input=$2 dispatches=$3 from=$4 every=$5
ms0=${6:-.tmp/route/ms0} policy=${7:-latest}
python=${PYTHON:-python}
out=.tmp/route/$name
rm -rf "$out"; mkdir -p "$out" "$ms0"
export TEMP=$PWD/.tmp TMP=$PWD/.tmp
./build/p3p_pc_bootstrap.exe --run-until-blocker --max-dispatches "$dispatches" --ms0 "$ms0" --savedata "$policy" \
    --input "$input" --render-from $((from > 50 ? from - 50 : 0)) --frames-dir "$out/frames" --frame-every "$every" \
    --dump-events "$out/events.json" > "$out/run.log" 2>&1 || true
grep -a "Stop Reason" "$out/run.log" || true
"$python" .claude/skills/p3p-run-triage/scripts/contact_sheet.py "$out/frames" "$out/events.json" ".tmp/route/$name.png" "$from"
