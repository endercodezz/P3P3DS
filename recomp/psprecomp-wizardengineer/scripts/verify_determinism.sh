#!/usr/bin/env bash
# verify_determinism.sh — many-boot freeze-rate / determinism harness (#66).
#
# WHY THIS EXISTS
#   The .hack//Link loading blocker is a *race* (~43% of boots freeze in the CRI
#   streaming-ring producer/consumer convergence, see
#   .planning/research/dothack-loadprog.md and docs/SCHEDULER-DESIGN.md §6). A
#   single green boot proves nothing: under the unchanged code a "passing" boot
#   has ~57% prior probability. A race fix must be measured as a *rate* across
#   many boots, classified by an OBJECTIVE signal — not eyeballed. This harness
#   boots a game headless N times (one runtime at a time, SIGKILL between,
#   pgrep-confirmed clean), classifies each boot, and reports a freeze-rate.
#
# MODES
#   dothack   Boot the .hack disc N×; classify each boot frozen-vs-progressed by
#             the GE frame counter (debug socket `I` -> ge.frames) crossing a
#             threshold. Frozen boots in the research stalled at <=~52 frames;
#             progressed boots climbed past ~470 — a clean separation. Reports a
#             FREEZE-RATE (frozen/N).
#   patapon   Boot Patapon N× through the CLEANROOM band check (real_nonsprite in
#             13.8k–15.7k from the [GE_GEOM_HEARTBEAT] sentinel). Reports how many
#             boots landed in band — a regression / new-nondeterminism check for
#             the title that works today.
#
# This is STEP 1 of the #66 scheduler rollout: with the preemption flag OFF it
# MEASURES the current baseline freeze-rate to confirm the harness + classifier
# work; STEP 3 re-runs it with PSPRECOMP_PREEMPT=1 to validate the fix.
#
# USAGE
#   scripts/verify_determinism.sh dothack [N] [secs] [runtime] [disc]
#   scripts/verify_determinism.sh patapon [N] [secs] [runtime] [disc]
#     N        boots (default 30; >=30 for a tight CI)
#     secs     seconds per boot before the probe + kill (default 75; the .hack
#              spin window resolves ~70s in)
#     runtime  runtime binary (default: mode-appropriate — build-hack for
#              dothack, runtime/build for patapon)
#     disc     disc0 dir (default: mode-appropriate)
#
# ENV
#   PSPRECOMP_PREEMPT is passed through to the runtime untouched, so the same
#   harness measures baseline (unset) and candidate (=1).
#
# EXIT CODES
#   0  ran to completion and printed a freeze-rate / band summary
#   2  setup error (missing runtime/disc/tooling)
set -uo pipefail

MODE="${1:-}"
N="${2:-30}"
SECS="${3:-75}"
PORT=9999
DEBUG_HOST=127.0.0.1

# Classifier threshold: a boot is PROGRESSED if ge.frames > FRAME_THRESHOLD,
# else FROZEN. Research separation is ~52 (frozen) vs >470 (progressed); 100 is
# a safe midpoint that no frozen boot reaches and every progressed boot clears.
FRAME_THRESHOLD="${FRAME_THRESHOLD:-100}"
# Patapon CLEANROOM band (docs/SCHEDULER-DESIGN.md §6.2 / DEBUGGING.md §2).
BAND_LO=13800
BAND_HI=15700

usage() {
    echo "usage: $0 {dothack|patapon} [N] [secs] [runtime] [disc]" >&2
    exit 2
}

[ "$MODE" = "dothack" ] || [ "$MODE" = "patapon" ] || usage

if [ "$MODE" = "dothack" ]; then
    RUNTIME="${4:-./build-hack/psprecomp_runtime}"
    DISC="${5:-./data/dothack/disc0}"
else
    RUNTIME="${4:-./runtime/build/psprecomp_runtime}"
    DISC="${5:-./disc0}"
fi

[ -x "$RUNTIME" ] || { echo "FAIL: runtime not executable: $RUNTIME" >&2; exit 2; }
[ -d "$DISC" ] || { echo "FAIL: disc dir not found: $DISC" >&2; exit 2; }
command -v nc >/dev/null || { echo "FAIL: nc not found" >&2; exit 2; }
command -v python3 >/dev/null || { echo "FAIL: python3 not found" >&2; exit 2; }

LOGDIR="${TMPDIR:-/tmp}/psprecomp_determinism"
mkdir -p "$LOGDIR"

# Ensure no stray runtime holds the SDL window / debug socket / disc.
clean_runtimes() {
    pkill -KILL -f psprecomp_runtime 2>/dev/null || true
    sleep 1
    while pgrep -f psprecomp_runtime >/dev/null 2>&1; do
        pkill -KILL -f psprecomp_runtime 2>/dev/null || true
        sleep 1
    done
}

# Query ge.frames over the debug socket, stripping the "OK <len>\n" framing.
query_frames() {
    printf 'I\n' | nc -w 5 "$DEBUG_HOST" "$PORT" 2>/dev/null \
        | python3 -c '
import sys, json
raw = sys.stdin.read()
# Reply is "OK <len>\n<json>" (or "ERR ..."); take the JSON body.
nl = raw.find("\n")
body = raw[nl + 1:] if nl >= 0 and raw[:2] == "OK" else raw
try:
    print(json.loads(body)["ge"]["frames"])
except Exception:
    print(-1)
' 2>/dev/null
}

echo "=== verify_determinism: mode=$MODE N=$N secs=${SECS}s runtime=$RUNTIME ==="
echo "    disc=$DISC PSPRECOMP_PREEMPT=${PSPRECOMP_PREEMPT:-<unset>} threshold=${FRAME_THRESHOLD}fr"
clean_runtimes

frozen=0
progressed=0
inconclusive=0
in_band=0
out_band=0

for i in $(seq 1 "$N"); do
    log="$LOGDIR/${MODE}_boot_$i.log"
    PSPRECOMP_DISC0="$DISC" "$RUNTIME" >"$log" 2>&1 &
    rtpid=$!

    # Let it boot through the spin/render window, then probe the live socket.
    sleep "$SECS"

    if [ "$MODE" = "dothack" ]; then
        frames=$(query_frames)
        kill "$rtpid" 2>/dev/null || true
        if [ -z "$frames" ] || [ "$frames" -lt 0 ] 2>/dev/null; then
            inconclusive=$((inconclusive + 1))
            cls="INCONCLUSIVE(no-socket)"
        elif [ "$frames" -gt "$FRAME_THRESHOLD" ]; then
            progressed=$((progressed + 1))
            cls="PROGRESSED"
        else
            frozen=$((frozen + 1))
            cls="FROZEN"
        fi
        printf "  boot %2d: frames=%-6s -> %s\n" "$i" "${frames:-?}" "$cls"
    else
        # Patapon: read the last [GE_GEOM_HEARTBEAT] real_nonsprite from the log.
        kill "$rtpid" 2>/dev/null || true
        sleep 1
        rns=$(grep "GE_GEOM_HEARTBEAT" "$log" | tail -1 \
              | sed -n 's/.*real_nonsprite=\([0-9]*\).*/\1/p')
        if [ -z "$rns" ]; then
            inconclusive=$((inconclusive + 1))
            cls="INCONCLUSIVE(no-heartbeat)"
        elif [ "$rns" -ge "$BAND_LO" ] && [ "$rns" -le "$BAND_HI" ]; then
            in_band=$((in_band + 1))
            cls="IN-BAND"
        else
            out_band=$((out_band + 1))
            cls="OUT-OF-BAND"
        fi
        printf "  boot %2d: real_nonsprite=%-6s -> %s\n" "$i" "${rns:-?}" "$cls"
    fi

    clean_runtimes
    : > "$log"   # delete the multi-MB boot log; verdict already extracted
done

echo "----------------------------------------------------------------"
if [ "$MODE" = "dothack" ]; then
    total=$((frozen + progressed))
    rate="n/a"
    if [ "$total" -gt 0 ]; then
        rate=$(python3 -c "print(f'{100.0*$frozen/$total:.1f}%')")
    fi
    echo "VERDICT (dothack): FREEZE-RATE = $frozen/$total = $rate"
    echo "    progressed=$progressed frozen=$frozen inconclusive=$inconclusive"
    echo "    classifier: ge.frames > $FRAME_THRESHOLD == progressed (#66 §6.1)"
else
    echo "VERDICT (patapon): IN-BAND $in_band/$N (band ${BAND_LO}-${BAND_HI})"
    echo "    in_band=$in_band out_of_band=$out_band inconclusive=$inconclusive"
fi
echo "    logs (truncated each boot): $LOGDIR/${MODE}_boot_*.log"
exit 0
