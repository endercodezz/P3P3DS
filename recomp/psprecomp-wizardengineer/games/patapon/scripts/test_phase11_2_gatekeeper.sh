#!/usr/bin/env bash
# Phase 11.2 acceptance harness — wraps the A1-A12 commands from
# .planning/phases/11.2-.../11.2-RESEARCH.md §5.1 into a single deterministic
# bash harness. Each step prints one PASS/FAIL line and exits non-zero on the
# first failure. Designed to be run AFTER the gatekeeper consolidation
# (Plan 11.2-01 / Wave 0) + the H1/H2/H3 fix (Plan 11.2-03 / Wave 2) land.
#
# Sibling to scripts/test_phase11_1_no_retry.sh (Phase 11.1 harness). The 7-step
# structure is verbatim from 11.2-PATTERNS.md Pattern G; the only differences
# are log-file suffixes (p111 -> p112) and per-A-ID assertion thresholds.
#
# Usage: ./scripts/test_phase11_2_gatekeeper.sh
# Exit:  0 = all of A1..A12 passed; 1 = first failed assertion.
set -euo pipefail

RUNTIME="./runtime/build/psprecomp_runtime"
BASELINE_TGA=".planning/research/artifacts/frame_post_phase11_proof.tga"
LOG_NORMAL="/tmp/p112.log"
LOG_GETEST="/tmp/p112_getest.log"
LOG_DIS="/tmp/p112_dis.log"
LOG_CORRUPT="/tmp/p112_corrupt.log"
CORRUPT_BND="/tmp/p112_corrupt.bnd"

# -----------------------------------------------------------------------------
# Step 0: preflight — runtime binary present, baseline TGA present + SHA-256
# -----------------------------------------------------------------------------
if [ ! -x "$RUNTIME" ]; then
    echo "FAIL preflight: runtime missing at $RUNTIME"
    exit 1
fi
if [ ! -r "$BASELINE_TGA" ]; then
    echo "FAIL preflight: baseline TGA missing or unreadable at $BASELINE_TGA"
    exit 1
fi
if ! shasum -a 256 "$BASELINE_TGA" | grep -q '^832a7a99'; then
    echo "FAIL preflight: baseline TGA SHA-256 mismatch (expected prefix 832a7a99)"
    shasum -a 256 "$BASELINE_TGA"
    exit 1
fi
echo "PASS preflight: runtime + baseline TGA verified"

# -----------------------------------------------------------------------------
# Step 1: A1 (build green)
# -----------------------------------------------------------------------------
BUILD_OUT=$(cmake --build runtime/build -j"$(sysctl -n hw.ncpu)" 2>&1 | tail -20)
if ! echo "$BUILD_OUT" | grep -q 'Built target psprecomp_runtime'; then
    echo "FAIL A1: build did not finish with 'Built target psprecomp_runtime'"
    echo "$BUILD_OUT" | tail -10
    exit 1
fi
if echo "$BUILD_OUT" | grep -qE 'error:'; then
    echo "FAIL A1: build emitted 'error:' lines"
    echo "$BUILD_OUT" | grep -E 'error:' | head -5
    exit 1
fi
echo "PASS A1: build green"

# -----------------------------------------------------------------------------
# Step 2: A2/A3/A5/A6/A7/A12 — normal 12s run, write /tmp/p112.log + frame.tga
# -----------------------------------------------------------------------------
rm -f frame.tga
timeout 12 "$RUNTIME" 2>&1 | tee "$LOG_NORMAL" || true

# A2: gatekeeper re-entries ≥ 5 (consolidated wrapper emits [GE_GATE#N] or [GK_GATE])
COUNT=$(grep -cE '\[GE_GATE#|\[GK_GATE\]' "$LOG_NORMAL" || true)
if [ "${COUNT:-0}" -lt 5 ]; then
    echo "FAIL A2: gatekeeper re-entries count=$COUNT (need ≥ 5)"
    grep -E '\[GE_GATE#|\[GK_GATE\]' "$LOG_NORMAL" | head -5 || true
    exit 1
fi
echo "PASS A2: gatekeeper re-entries count=$COUNT (≥ 5)"

# A3: at least one [DRAW_PRIM] ... clear=0 line (R7 carry-forward — headline unblock)
COUNT=$(grep -c '\[DRAW_PRIM\].*clear=0' "$LOG_NORMAL" || true)
if [ "${COUNT:-0}" -lt 1 ]; then
    echo "FAIL A3: [DRAW_PRIM].*clear=0 count=$COUNT (need ≥ 1)"
    exit 1
fi
echo "PASS A3: [DRAW_PRIM] clear=0 count=$COUNT (≥ 1)"

# A5: [BND_TRACE] distinct idx_str count ≥ 2 (proves consumer advances)
COUNT=$(grep '\[BND_TRACE\] resolved' "$LOG_NORMAL" | awk -F'"' '{print $2}' | sort -u | wc -l || true)
if [ "${COUNT:-0}" -lt 2 ]; then
    echo "FAIL A5: [BND_TRACE] distinct idx_str count=$COUNT (need ≥ 2)"
    grep '\[BND_TRACE\] resolved' "$LOG_NORMAL" | head -5 || true
    exit 1
fi
echo "PASS A5: [BND_TRACE] distinct idx_str count=$COUNT (≥ 2)"

# A6: [SEMA271] ≤ 100 (down from 6,848 post-11.1)
COUNT=$(grep -c '\[SEMA271\]' "$LOG_NORMAL" || true)
if [ "${COUNT:-0}" -gt 100 ]; then
    echo "FAIL A6: [SEMA271] count=$COUNT (need ≤ 100)"
    exit 1
fi
echo "PASS A6: [SEMA271] count=$COUNT (≤ 100)"

# A7: [BND_SLOT_SHORT_TOTAL] firings ≤ 50 (uses NEW atexit total dump from Task 3)
# atexit emits exactly one line "[BND_SLOT_SHORT_TOTAL] %d firings ..."; if it
# never fires (counter == 0) we substitute 0 via :- default and that passes ≤ 50.
COUNT=$(grep '\[BND_SLOT_SHORT_TOTAL\]' "$LOG_NORMAL" | awk '{print $2}' | head -1 || true)
if [ "${COUNT:-0}" -gt 50 ]; then
    echo "FAIL A7: [BND_SLOT_SHORT_TOTAL] firings=$COUNT (need ≤ 50)"
    grep '\[BND_SLOT_SHORT_TOTAL\]' "$LOG_NORMAL" | head -3 || true
    exit 1
fi
echo "PASS A7: [BND_SLOT_SHORT_TOTAL] firings=${COUNT:-0} (≤ 50)"

# A12: [LOOKUP_MISS] gate ≤ 10 excluding sentinel 0xDEADBEEF
# Also dumps any [BND_VTABLE_MISS] punch-list for the next phase.
COUNT=$(grep '\[LOOKUP_MISS\]' "$LOG_NORMAL" | grep -v '0xDEADBEEF' | wc -l | tr -d ' ' || true)
if [ "${COUNT:-0}" -gt 10 ]; then
    echo "FAIL A12: [LOOKUP_MISS] non-sentinel count=$COUNT (need ≤ 10)"
    grep '\[LOOKUP_MISS\]' "$LOG_NORMAL" | grep -v '0xDEADBEEF' | head -10 || true
    exit 1
fi
echo "PASS A12: [LOOKUP_MISS] non-sentinel count=$COUNT (≤ 10)"
# Soft: list [BND_VTABLE_MISS] punch-list addresses if any fired
VTBL_MISS=$(grep -c '\[BND_VTABLE_MISS\]' "$LOG_NORMAL" || true)
if [ "${VTBL_MISS:-0}" -gt 0 ]; then
    echo "INFO A12: [BND_VTABLE_MISS] punch-list (first 5):"
    grep '\[BND_VTABLE_MISS\]' "$LOG_NORMAL" | head -5 || true
fi

# -----------------------------------------------------------------------------
# Step 3: A4 — pixel diff vs baseline TGA (ImageMagick `compare`)
# -----------------------------------------------------------------------------
if [ ! -r frame.tga ]; then
    echo "FAIL A4 prereq: frame.tga missing after normal run"
    exit 1
fi
DIFF=$(compare -metric AE "$BASELINE_TGA" frame.tga null: 2>&1 || true)
# `compare` may print "<N>" or "<N> (<m>%)" or "<N> @ <x>,<y>"; take first integer
DIFF_NUM=$(echo "$DIFF" | grep -oE '^[0-9]+' | head -1 || true)
if [ -z "${DIFF_NUM:-}" ] || [ "${DIFF_NUM:-0}" -lt 100 ]; then
    echo "FAIL A4: pixel diff=$DIFF (need ≥ 100; raw='$DIFF')"
    exit 1
fi
echo "PASS A4: pixel diff=$DIFF_NUM (≥ 100)"

# -----------------------------------------------------------------------------
# Step 4: A8 — PSPRECOMP_GE_TEST_ONLY=1 — center pixel teal + zero [BND_*] family
# -----------------------------------------------------------------------------
rm -f frame.tga
PSPRECOMP_GE_TEST_ONLY=1 timeout 8 "$RUNTIME" 2>&1 | tee "$LOG_GETEST" || true
if [ ! -r frame.tga ]; then
    echo "FAIL A8 prereq: frame.tga missing after GE_TEST_ONLY run"
    exit 1
fi
if ! python3 runtime/tests/check_center_pixel.py frame.tga 68 136 255 2 | grep -q PASS; then
    echo "FAIL A8: center pixel not teal after PSPRECOMP_GE_TEST_ONLY=1"
    python3 runtime/tests/check_center_pixel.py frame.tga 68 136 255 2 || true
    exit 1
fi
# Stricter than Phase 11.1's [BND_SLOT_SHORT] check: NO [BND_*] family tag should
# fire under GE_TEST_ONLY (game thread never starts). [GK_GATE] from the
# consolidated wrapper MAY fire at wrapper install time and is OUT OF SCOPE
# for this assertion (we filter only [BND_* prefix).
BSC=$(grep -c '\[BND_' "$LOG_GETEST" || true)
if [ "${BSC:-0}" -ne 0 ]; then
    echo "FAIL A8: [BND_*] family fired without game thread (count=$BSC)"
    grep '\[BND_' "$LOG_GETEST" | head -5 || true
    exit 1
fi
echo "PASS A8: GE_TEST_ONLY teal + [BND_*] family count=0"

# -----------------------------------------------------------------------------
# Step 5: A9 — PSPRECOMP_BND_DISABLE=1 — zero [BND_SLOT_SHORT] firings
# (per RESEARCH §5.1: [GK_GATE] MAY fire under BND_DISABLE because the
# consolidated wrapper installs regardless of arena state — what must be 0
# is [BND_SLOT_SHORT] because the fallback descriptor at 0x08002864 is
# outside the BND arena range)
# -----------------------------------------------------------------------------
rm -f frame.tga
PSPRECOMP_BND_DISABLE=1 timeout 10 "$RUNTIME" 2>&1 | tee "$LOG_DIS" || true
BSC=$(grep -c '\[BND_SLOT_SHORT\]' "$LOG_DIS" || true)
if [ "${BSC:-0}" -ne 0 ]; then
    echo "FAIL A9: [BND_SLOT_SHORT] fired under BND_DISABLE (count=$BSC)"
    grep '\[BND_SLOT_SHORT\]' "$LOG_DIS" | head -5 || true
    exit 1
fi
echo "PASS A9: BND_DISABLE preserved ([BND_SLOT_SHORT] count=0)"

# -----------------------------------------------------------------------------
# Step 6: A10 — corrupt BND aborts with exit 134 + [BND_ERR] line
# -----------------------------------------------------------------------------
./games/patapon/tests/make_corrupt_bnd.sh disc0/PSP_GAME/USRDIR/DATA_CMN.BND "$CORRUPT_BND"
set +e
DATA_CMN_BND_PATH="$CORRUPT_BND" timeout 12 "$RUNTIME" > "$LOG_CORRUPT" 2>&1
RC=$?
set -e
BND_ERR_COUNT=$(grep -c '\[BND_ERR\]' "$LOG_CORRUPT" || true)
if [ "$RC" -ne 134 ]; then
    echo "FAIL A10: corrupt exit=$RC (need 134 = SIGABRT); BND_ERR count=$BND_ERR_COUNT"
    tail -20 "$LOG_CORRUPT" || true
    exit 1
fi
if [ "${BND_ERR_COUNT:-0}" -lt 1 ]; then
    echo "FAIL A10: [BND_ERR] count=$BND_ERR_COUNT (need ≥ 1) despite exit 134"
    exit 1
fi
echo "PASS A10: corrupt exit=134, [BND_ERR] count=$BND_ERR_COUNT"

# -----------------------------------------------------------------------------
# Step 7: A11 — test_asset_bnd unit suite (37/37 pass — custom minimalist runner)
# Per Phase 11.1 final verdict the test runner is a custom runner, NOT GoogleTest;
# the format is "37 tests run, 0 failures" + "ALL TESTS PASSED".
# -----------------------------------------------------------------------------
./runtime/build/test_asset_bnd 2>&1 | tail -5 | tee /tmp/p112_unit.log || true
if ! grep -qE '37 tests run, 0 failures' /tmp/p112_unit.log; then
    echo "FAIL A11: unit tests not 37/37 (see /tmp/p112_unit.log)"
    tail -5 /tmp/p112_unit.log || true
    exit 1
fi
if ! grep -qE 'ALL TESTS PASSED' /tmp/p112_unit.log; then
    echo "FAIL A11: 'ALL TESTS PASSED' marker missing (see /tmp/p112_unit.log)"
    tail -5 /tmp/p112_unit.log || true
    exit 1
fi
echo "PASS A11: test_asset_bnd 37/37 (custom runner)"

echo "PASS Phase 11.2 (A1-A12)"
exit 0
