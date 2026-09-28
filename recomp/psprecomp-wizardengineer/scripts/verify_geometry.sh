#!/usr/bin/env bash
# verify_geometry.sh — definitive, reproducible answer to:
#   "Is the runtime actually setting up graphics, or only clearing the screen?"
#
# WHY THIS EXISTS
#   The prior check (verify_ge_prim.sh) grepped `PRIM.*count.*[1-9]`, which
#   matches CLEAR prims too (clears have count>0) — a false PASS. And boot is
#   nondeterministic, so a single run is not trustworthy. This harness:
#     1. Runs the runtime N times with a bounded, SIGTERM-first window so the
#        atexit [GE_GEOM_VERDICT] line prints (clean exit), with --kill-after as
#        a hang backstop.
#     2. ALSO reads the live sentinels [GE_GEOM_REAL_DRAW] / [GE_GEOM_HEARTBEAT]
#        (psp_ge_draw.cpp), which survive even SIGKILL/hang.
#     3. Aggregates across runs and prints ONE verdict.
#
# SUCCESS BAR (handoff 11-START-HERE §SUCCESS BAR, half b):
#   one real draw with type!=6 (non-sprite) AND clear==0 AND normal>0.
#   i.e. real_nonsprite > 0  ==  graphics are being set up.
#
# EXIT CODES
#   0  GRAPHICS DETECTED   (real_nonsprite>0 in >=1 run) — success bar (b) met
#   1  NO GRAPHICS         (every run: real_nonsprite==0) — confirmed starvation
#   2  INCONCLUSIVE        (no run produced a trustworthy verdict line)
#
# USAGE
#   ./scripts/verify_geometry.sh [runs] [seconds_per_run]
#   RUNS (default 3), SECONDS (default 35). Title load can take 60-110s; bump
#   SECONDS for a deeper check, e.g. `./scripts/verify_geometry.sh 3 120`.
set -uo pipefail

RUNS="${1:-3}"
SECS="${2:-35}"
RUNTIME="./runtime/build/psprecomp_runtime"
LOGDIR="${TMPDIR:-/tmp}/psprecomp_geom_verify"
mkdir -p "$LOGDIR"

if [ ! -x "$RUNTIME" ]; then
    echo "FAIL: runtime not found at $RUNTIME — build it first:" >&2
    echo "  cmake --build runtime/build -j\$(sysctl -n hw.ncpu)" >&2
    exit 2
fi

# Make sure no stray runtime is holding the SDL window / debug socket.
pkill -TERM -f psprecomp_runtime 2>/dev/null || true
sleep 1

any_graphics=0
any_verdict=0
echo "=== verify_geometry: $RUNS run(s) x ${SECS}s, binary=$RUNTIME ==="

for i in $(seq 1 "$RUNS"); do
    log="$LOGDIR/run_$i.log"
    # SIGTERM at $SECS (clean shutdown -> atexit verdict fires); SIGKILL 10s
    # later only if shutdown itself hangs (then the live sentinels still apply).
    timeout --kill-after=10 --signal=TERM "$SECS" "$RUNTIME" >"$log" 2>&1
    rc=$?

    clean=$(grep -c "Shutdown complete" "$log")
    real_draw_line=$(grep -m1 "GE_GEOM_REAL_DRAW" "$log" || true)
    verdict_line=$(grep -m1 "GE_GEOM_VERDICT" "$log" || true)
    last_hb=$(grep "GE_GEOM_HEARTBEAT" "$log" | tail -1 || true)

    # Derive this run's real_nonsprite count from the strongest evidence
    # available (verdict line > heartbeat > live sentinel presence).
    real_ns="?"
    if [ -n "$verdict_line" ]; then
        real_ns=$(printf '%s\n' "$verdict_line" | sed -n 's/.*real_nonsprite=\([0-9]*\).*/\1/p')
        any_verdict=1
    elif [ -n "$last_hb" ]; then
        real_ns=$(printf '%s\n' "$last_hb" | sed -n 's/.*real_nonsprite=\([0-9]*\).*/\1/p')
        any_verdict=1
    fi
    # The live sentinel is conclusive proof of graphics regardless of exit path.
    if [ -n "$real_draw_line" ]; then real_ns="${real_ns/#?/1}"; fi

    run_state="NO-GRAPHICS"
    if [ -n "$real_draw_line" ] || { [ "$real_ns" != "?" ] && [ "${real_ns:-0}" -gt 0 ] 2>/dev/null; }; then
        run_state="GRAPHICS"; any_graphics=1
    fi
    [ "$real_ns" = "?" ] && run_state="INCONCLUSIVE"

    printf "  run %d: rc=%-3s clean_exit=%s real_nonsprite=%-3s -> %s\n" \
        "$i" "$rc" "$clean" "$real_ns" "$run_state"
    [ -n "$verdict_line" ] && echo "         $verdict_line"
    [ -n "$real_draw_line" ] && echo "         $real_draw_line"
done

echo "----------------------------------------------------------------"
if [ "$any_graphics" -eq 1 ]; then
    echo "VERDICT: GRAPHICS DETECTED — at least one run produced a real"
    echo "         (non-clear, non-sprite type!=6) draw. Success bar (b) MET."
    echo "         logs: $LOGDIR/run_*.log"
    exit 0
elif [ "$any_verdict" -eq 1 ]; then
    echo "VERDICT: NO GRAPHICS — every run set up only framebuffer clears"
    echo "         (real_nonsprite=0). Graphics are NOT being set up."
    echo "         logs: $LOGDIR/run_*.log"
    exit 1
else
    echo "VERDICT: INCONCLUSIVE — no run produced a trustworthy verdict line."
    echo "         (No clean exit AND no live sentinel/heartbeat.) Check that"
    echo "         the runtime started and the GE ran at all:"
    echo "         grep -E 'GE_SUMMARY|GE_GEOM' $LOGDIR/run_1.log"
    exit 2
fi
