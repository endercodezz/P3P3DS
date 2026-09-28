#!/usr/bin/env bash
# Produce a deliberately-corrupted DATA_CMN.BND fixture by truncating it to
# 16 bytes (just the header bytes — no entry table). Fed to the runtime via
# `DATA_CMN_BND_PATH=$DST` to exercise the R5 abort-on-malformed-input path.
set -euo pipefail
SRC="${1:-disc0/PSP_GAME/USRDIR/DATA_CMN.BND}"
DST="${2:-/tmp/corrupt_truncated.bnd}"
[ -f "$SRC" ] || { echo "missing source $SRC" >&2; exit 1; }
cp "$SRC" "$DST"
truncate -s 16 "$DST"
echo "wrote $DST (truncated to 16 bytes)"
