#!/usr/bin/env bash
# Phase 11.5 Plan 01 / Wave 0 — sequence-anchored ordered diff driver.
#
# Reads /tmp/trace_ppsspp.raw.jsonl (PPSSPP `log` push events) and
# /tmp/trace_runtime.raw.log (runtime [HLE-TRACE] stderr capture), normalizes
# both via runtime/tools/normalize_trace.py, applies the RESEARCH §4.4
# verbatim denylist of timing/cache-maintenance calls, runs `diff -u`, and
# emits the first hunk (head -30) wrapped in `=== FIRST DIVERGENCE ===`
# banner lines. Exit 0 is the expected outcome regardless of whether the
# traces differ — this is a diagnostic, not a regression test.
#
# Usage:
#   scripts/diff_traces.sh             # normal mode, reads /tmp inputs
#   scripts/diff_traces.sh --self-test # synthetic fixture; exits 0 on PASS
set -euo pipefail

# ─── Constants ───────────────────────────────────────────────────────────────
TRACE_DIR="${TRACE_DIR:-/tmp}"
PPSSPP_RAW="${TRACE_DIR}/trace_ppsspp.raw.jsonl"
RUNTIME_RAW="${TRACE_DIR}/trace_runtime.raw.log"
PPSSPP_CANON="${TRACE_DIR}/trace_ppsspp.txt"
RUNTIME_CANON="${TRACE_DIR}/trace_runtime.txt"
DIFF_OUT="${TRACE_DIR}/trace_diff.unified"

# Verbatim from RESEARCH §4.4 — drop timing/cache-maintenance calls whose
# frequency is not behaviorally meaningful for the gatekeeper chain.
DENYLIST='sceKernelGetSystemTimeLow|sceKernelLibcGettimeofday|sceKernelLibcClock|sceKernelLibcTime|sceDisplayGetVcount|sceKernelCheckCallback|sceKernelDcacheWritebackAll|sceKernelDcacheWritebackRange|sceKernelDcacheWritebackInvalidateAll'

# Resolve repo root from the script's own location so paths to
# runtime/tools/normalize_trace.py work regardless of cwd.
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
NORMALIZE="${REPO_ROOT}/runtime/tools/normalize_trace.py"

# ─── Self-test mode ──────────────────────────────────────────────────────────
self_test() {
    local tdir
    tdir="$(mktemp -d)"
    local p="${tdir}/ppsspp.selftest.txt"
    local r="${tdir}/runtime.selftest.txt"
    local d="${tdir}/diff.unified"

    # Synthetic canonical-form lines — 4 each. Line 3 differs between sides
    # so the first hunk contains a clear `-` for the PPSSPP side. Line 1 is
    # a denylisted call (sceKernelGetSystemTimeLow) that MUST be filtered out.
    cat > "$p" <<'EOF'
000001 boot sceKernelGetSystemTimeLow a0=0x00000000 a1=0x00000000 a2=0x00000000 a3=0x00000000 v0=0x00000001
000002 boot sceKernelLoadModule a0=0x08DEAD00 a1=0x00000000 a2=0x00000000 a3=0x00000000 v0=0x0000002A
000003 boot sceKernelCreateThread a0=0x08BBB000 a1=0x00000000 a2=0x00000000 a3=0x00000000 v0=0x00000043
000004 boot sceGeListEnQueue a0=0x09000000 a1=0x09000400 a2=0x00000000 a3=0x00000000 v0=0x00000001
EOF
    cat > "$r" <<'EOF'
000001 boot sceKernelGetSystemTimeLow a0=0x00000000 a1=0x00000000 a2=0x00000000 a3=0x00000000 v0=0x00000002
000002 boot sceKernelLoadModule a0=0x08DEAD00 a1=0x00000000 a2=0x00000000 a3=0x00000000 v0=0x0000002A
000003 boot sceKernelCreateThread a0=0x08BBB000 a1=0x00000000 a2=0x00000000 a3=0x00000000 v0=0xDEADBEEF
000004 boot sceGeListEnQueue a0=0x09000000 a1=0x09000400 a2=0x00000000 a3=0x00000000 v0=0x00000001
EOF

    # Apply denylist + diff
    local p_f="${p}.filtered"
    local r_f="${r}.filtered"
    grep -vE "$DENYLIST" "$p" > "$p_f" || true
    grep -vE "$DENYLIST" "$r" > "$r_f" || true

    # Verify denylist actually removed the line with sceKernelGetSystemTimeLow.
    if grep -q 'sceKernelGetSystemTimeLow' "$p_f" || grep -q 'sceKernelGetSystemTimeLow' "$r_f"; then
        echo "FAIL diff_traces.sh self-test: denylist did not drop sceKernelGetSystemTimeLow" >&2
        exit 1
    fi

    diff -u "$p_f" "$r_f" > "$d" || true

    # Pull the first hunk
    local first_hunk
    first_hunk=$(awk '/^@@/{n++} n==1' "$d" | head -30 || true)
    if [ -z "$first_hunk" ]; then
        echo "FAIL diff_traces.sh self-test: no first hunk extracted from synthetic divergence" >&2
        cat "$d" >&2
        exit 1
    fi
    # The synthetic divergence is on the v0= field of the sceKernelCreateThread line.
    if ! echo "$first_hunk" | grep -q 'sceKernelCreateThread'; then
        echo "FAIL diff_traces.sh self-test: first hunk does not reference the divergent call" >&2
        echo "$first_hunk" >&2
        exit 1
    fi
    if ! echo "$first_hunk" | grep -qE '^-.*v0=0x00000043'; then
        echo "FAIL diff_traces.sh self-test: PPSSPP-side v0 not present as '-' line" >&2
        echo "$first_hunk" >&2
        exit 1
    fi

    # Clean up temp dir on success only
    rm -rf "$tdir"
    echo "PASS diff_traces.sh self-test"
    exit 0
}

# ─── CLI ────────────────────────────────────────────────────────────────────
if [ "${1:-}" = "--self-test" ]; then
    self_test
fi

# ─── Normal mode ────────────────────────────────────────────────────────────
if [ ! -r "$PPSSPP_RAW" ]; then
    echo "FAIL diff_traces.sh: missing $PPSSPP_RAW" >&2
    echo "  (Plan 02 / capture.ts produces this; ensure capture has run)" >&2
    exit 1
fi
if [ ! -r "$RUNTIME_RAW" ]; then
    echo "FAIL diff_traces.sh: missing $RUNTIME_RAW" >&2
    echo "  (PSPRECOMP_HLE_TRACE=1 ./runtime/build/psprecomp_runtime 2>$RUNTIME_RAW)" >&2
    exit 1
fi
if [ ! -r "$NORMALIZE" ]; then
    echo "FAIL diff_traces.sh: normalize_trace.py not found at $NORMALIZE" >&2
    exit 1
fi

# Normalize both sides
python3 "$NORMALIZE" --format ppsspp --input "$PPSSPP_RAW" --out "$PPSSPP_CANON"
python3 "$NORMALIZE" --format runtime --input "$RUNTIME_RAW" --out "$RUNTIME_CANON"

# Apply denylist filter
PPSSPP_FILTERED="${PPSSPP_CANON}.filtered"
RUNTIME_FILTERED="${RUNTIME_CANON}.filtered"
grep -vE "$DENYLIST" "$PPSSPP_CANON" > "$PPSSPP_FILTERED" || true
grep -vE "$DENYLIST" "$RUNTIME_CANON" > "$RUNTIME_FILTERED" || true

# diff -u — the existence of differences is the desired outcome; swallow the
# non-zero exit so `set -e` doesn't trip.
diff -u "$PPSSPP_FILTERED" "$RUNTIME_FILTERED" > "$DIFF_OUT" || true

# Extract the first hunk
FIRST_HUNK_OUT="${DIFF_OUT}.first_hunk"
awk '/^@@/{n++} n==1' "$DIFF_OUT" | head -30 > "$FIRST_HUNK_OUT" || true

# Print the banner-wrapped first hunk + summary
echo "=== FIRST DIVERGENCE ==="
cat "$FIRST_HUNK_OUT"
echo "=== END DIVERGENCE ==="

PPSSPP_LINES=$(wc -l < "$PPSSPP_FILTERED" | tr -d ' ')
RUNTIME_LINES=$(wc -l < "$RUNTIME_FILTERED" | tr -d ' ')
DIV_AT=$(awk '/^@@/{print; exit}' "$DIFF_OUT" || echo "(no hunk header)")
echo "[DIFF] runtime_lines=${RUNTIME_LINES}, ppsspp_lines=${PPSSPP_LINES}, divergence_at=${DIV_AT}"

exit 0
