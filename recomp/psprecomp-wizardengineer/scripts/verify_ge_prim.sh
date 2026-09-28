#!/usr/bin/env bash
# DEPRECATED — kept for backward compatibility.
#
# The old logic here grepped `PRIM.*count.*[1-9]`, which matches CLEAR prims too
# (clears carry count>0), so it reported PASS even when the runtime drew nothing
# but framebuffer clears — a false positive. Use the real harness instead:
#
#   ./scripts/verify_geometry.sh [runs] [seconds_per_run]
#
# It distinguishes real (non-clear, non-sprite type!=6) draws from clears via the
# [GE_GEOM_VERDICT] line, runs multiple times to defeat boot nondeterminism, and
# exits 0=GRAPHICS / 1=NO-GRAPHICS / 2=INCONCLUSIVE.
#
# This shim forwards the single legacy positional arg (timeout seconds) as the
# per-run window and runs once.
set -uo pipefail
TIMEOUT="${1:-35}"
DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
echo "[verify_ge_prim.sh is deprecated -> delegating to verify_geometry.sh]" >&2
exec "$DIR/verify_geometry.sh" 1 "$TIMEOUT"
