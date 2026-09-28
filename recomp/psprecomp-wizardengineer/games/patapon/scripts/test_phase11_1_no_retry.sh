#!/usr/bin/env bash
# Phase 11.1 acceptance harness — wraps the A1-A12 commands from
# .planning/phases/11.1-.../11.1-RESEARCH.md §5.1 into a single deterministic
# bash harness. Each step prints one PASS/FAIL line and exits non-zero on the
# first failure. Designed to be run AFTER plan 11.1-02 installs the
# [BND_SLOT_SHORT] branch in runtime/src/main.cpp.
#
# Usage: ./scripts/test_phase11_1_no_retry.sh
# Exit:  0 = all of A1..A12 passed; 1 = first failed assertion.
set -euo pipefail

RUNTIME="./runtime/build/psprecomp_runtime"
BASELINE_TGA=".planning/research/artifacts/frame_post_phase11_proof.tga"
LOG_NORMAL="/tmp/p111.log"
LOG_GETEST="/tmp/p111_getest.log"
LOG_DIS="/tmp/p111_dis.log"
LOG_CORRUPT="/tmp/p111_corrupt.log"
CORRUPT_BND="/tmp/p111_corrupt.bnd"

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
# Step 1: A1 (build green) + sets up A12 (test_asset_bnd target)
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
# Step 2: A2/A3/A4/A5/A7/A11 — normal 12s run, write /tmp/p111.log + frame.tga
# -----------------------------------------------------------------------------
rm -f frame.tga
timeout 12 "$RUNTIME" 2>&1 | tee "$LOG_NORMAL" || true

# A2: retry loop ≤ 5
COUNT=$(grep -c 'asyncResult=-2147418110' "$LOG_NORMAL" || true)
if [ "${COUNT:-0}" -gt 5 ]; then
    echo "FAIL A2: asyncResult=-2147418110 count=$COUNT (need ≤ 5)"
    grep 'asyncResult=-2147418110' "$LOG_NORMAL" | head -5
    exit 1
fi
echo "PASS A2: asyncResult=-2147418110 count=$COUNT (≤ 5)"

# A3: SEMA271 ≤ 50
COUNT=$(grep -c '\[SEMA271\]' "$LOG_NORMAL" || true)
if [ "${COUNT:-0}" -gt 50 ]; then
    echo "FAIL A3: [SEMA271] count=$COUNT (need ≤ 50)"
    exit 1
fi
echo "PASS A3: [SEMA271] count=$COUNT (≤ 50)"

# A4: [BND_SLOT_SHORT] ∈ [1, 20]
COUNT=$(grep -c '\[BND_SLOT_SHORT\]' "$LOG_NORMAL" || true)
if [ "${COUNT:-0}" -lt 1 ] || [ "${COUNT:-0}" -gt 20 ]; then
    echo "FAIL A4: [BND_SLOT_SHORT] count=$COUNT (need 1..20)"
    grep '\[BND_SLOT_SHORT\]' "$LOG_NORMAL" | head -5
    exit 1
fi
echo "PASS A4: [BND_SLOT_SHORT] count=$COUNT (1..20)"

# A5: at least one [DRAW_PRIM] ... clear=0
COUNT=$(grep -c '\[DRAW_PRIM\].*clear=0' "$LOG_NORMAL" || true)
if [ "${COUNT:-0}" -lt 1 ]; then
    echo "FAIL A5: [DRAW_PRIM].*clear=0 count=$COUNT (need ≥ 1)"
    exit 1
fi
echo "PASS A5: [DRAW_PRIM] clear=0 count=$COUNT (≥ 1)"

# A7: [BND_TRACE] resolved ≥ 3
COUNT=$(grep -c '\[BND_TRACE\] resolved' "$LOG_NORMAL" || true)
if [ "${COUNT:-0}" -lt 3 ]; then
    echo "FAIL A7: [BND_TRACE] resolved count=$COUNT (need ≥ 3)"
    exit 1
fi
echo "PASS A7: [BND_TRACE] resolved count=$COUNT (≥ 3)"

# A11: max sfd ≤ 50
MAX_SFD=$(grep -Eo 'sfd=[0-9]+' "$LOG_NORMAL" | sed 's/sfd=//' | sort -n | tail -1 || true)
if [ "${MAX_SFD:-0}" -gt 50 ]; then
    echo "FAIL A11: max sfd=$MAX_SFD (need ≤ 50)"
    exit 1
fi
echo "PASS A11: max sfd=${MAX_SFD:-0} (≤ 50)"

# -----------------------------------------------------------------------------
# Step 3: A6 — pixel diff vs baseline TGA (ImageMagick `compare`)
# -----------------------------------------------------------------------------
if [ ! -r frame.tga ]; then
    echo "FAIL A6 prereq: frame.tga missing after normal run"
    exit 1
fi
DIFF=$(compare -metric AE "$BASELINE_TGA" frame.tga null: 2>&1 || true)
# `compare` may print "<N>" or "<N> (<m>%)" or "<N> @ <x>,<y>"; take first integer
DIFF_NUM=$(echo "$DIFF" | grep -oE '^[0-9]+' | head -1 || true)
if [ -z "${DIFF_NUM:-}" ] || [ "${DIFF_NUM:-0}" -lt 100 ]; then
    echo "FAIL A6: pixel diff=$DIFF (need ≥ 100; raw='$DIFF')"
    exit 1
fi
echo "PASS A6: pixel diff=$DIFF_NUM (≥ 100)"

# -----------------------------------------------------------------------------
# Step 4: A8 — PSPRECOMP_GE_TEST_ONLY=1 — center pixel teal + zero short-fires
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
BSC=$(grep -c '\[BND_SLOT_SHORT\]' "$LOG_GETEST" || true)
if [ "${BSC:-0}" -ne 0 ]; then
    echo "FAIL A8: [BND_SLOT_SHORT] fired without game thread (count=$BSC)"
    grep '\[BND_SLOT_SHORT\]' "$LOG_GETEST" | head -5
    exit 1
fi
echo "PASS A8: GE_TEST_ONLY teal + [BND_SLOT_SHORT] count=0"

# -----------------------------------------------------------------------------
# Step 5: A9 — PSPRECOMP_BND_DISABLE=1 — zero short-circuit fires
# -----------------------------------------------------------------------------
rm -f frame.tga
PSPRECOMP_BND_DISABLE=1 timeout 10 "$RUNTIME" 2>&1 | tee "$LOG_DIS" || true
BSC=$(grep -c '\[BND_SLOT_SHORT\]' "$LOG_DIS" || true)
if [ "${BSC:-0}" -ne 0 ]; then
    echo "FAIL A9: [BND_SLOT_SHORT] fired under BND_DISABLE (count=$BSC)"
    grep '\[BND_SLOT_SHORT\]' "$LOG_DIS" | head -5
    exit 1
fi
echo "PASS A9: BND_DISABLE preserved (count=0)"

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
# Step 7: A12 — test_asset_bnd unit suite (37/37 pass)
# -----------------------------------------------------------------------------
./runtime/build/test_asset_bnd 2>&1 | tail -5 | tee /tmp/p111_unit.log || true
if ! grep -qE '\[  PASSED  \] 37 tests' /tmp/p111_unit.log; then
    echo "FAIL A12: unit tests not 37/37 (see /tmp/p111_unit.log)"
    tail -5 /tmp/p111_unit.log || true
    exit 1
fi
echo "PASS A12: test_asset_bnd 37/37"

echo "PASS Phase 11.1 (A1-A12)"
exit 0
