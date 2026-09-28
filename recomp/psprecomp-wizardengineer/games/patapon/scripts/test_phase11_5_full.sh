#!/usr/bin/env bash
# Phase 11.5 full acceptance — REUSES scripts/test_phase11_2_gatekeeper.sh
# UNCHANGED (A1-A12 carry-forward) then appends:
#   - A18 (non-clear geometry) via runtime/tests/check_a18_non_clear_geometry.py
#   - A19 (11.5-DIAGNOSTIC.md frontmatter schema validation)
#   - six-tag carry-forward self-check from 11.5-RESEARCH.md §5
#   - output/generated regression check (CLAUDE.md rule 1)
#   - Phase 11.3 H2-b-wrap preservation
#
# Exit: 0 = all gates pass; 1 = first failed assertion.
# Sibling to scripts/test_phase11_2_gatekeeper.sh (Phase 11.2 harness) — does
# NOT modify the existing harness; this is an ADDITIVE driver.
set -euo pipefail

DIAG=".planning/phases/11.5-identify-the-first-divergent-hle-call-between-ppsspp-and-our/11.5-DIAGNOSTIC.md"

# -----------------------------------------------------------------------------
# Step 1: Invoke the REUSED Phase 11.2 harness (A1-A12)
# -----------------------------------------------------------------------------
echo "=== A1-A12 (REUSED via scripts/test_phase11_2_gatekeeper.sh) ==="
bash scripts/test_phase11_2_gatekeeper.sh || {
    echo "FAIL: Phase 11.2 harness exited non-zero (A1-A12)"
    exit 1
}

# -----------------------------------------------------------------------------
# Step 2: A18 (non-clear geometry) — runs the Python helper on /tmp/p112.log
# (produced by the Phase 11.2 harness's normal 12s run in Step 2).
# -----------------------------------------------------------------------------
echo ""
echo "=== A18 (Phase 11.5 NEW — non-clear geometry) ==="
python3 runtime/tests/check_a18_non_clear_geometry.py /tmp/p112.log || {
    echo "FAIL: A18 — no non-clear geometry [GE_PRIM_DETAIL] line in /tmp/p112.log"
    exit 1
}
echo "PASS A18: non-clear geometry produced"

# -----------------------------------------------------------------------------
# Step 3: A19 — 11.5-DIAGNOSTIC.md frontmatter schema validation
# -----------------------------------------------------------------------------
echo ""
echo "=== A19 (Phase 11.5 NEW — DIAGNOSTIC.md schema) ==="
test -r "$DIAG" || { echo "FAIL: A19 — $DIAG missing"; exit 1; }
# Pick a python3 that has pyyaml available (env's default may lack it).
PY_BIN=""
for cand in /usr/bin/python3 /opt/homebrew/bin/python3.11 \
            /opt/homebrew/bin/python3.12 python3; do
    if command -v "$cand" >/dev/null 2>&1 && \
       "$cand" -c "import yaml" >/dev/null 2>&1; then
        PY_BIN="$cand"
        break
    fi
done
if [ -z "$PY_BIN" ]; then
    # No pyyaml available — fall back to plain-text grep for the required keys.
    if grep -qE '^first_divergent_call:[[:space:]]+[^[:space:]]' "$DIAG" && \
       grep -qE '^classified_divergence:[[:space:]]+(DIV-[ABCD]|unknown)\b' "$DIAG"; then
        CLS=$(grep -E '^classified_divergence:' "$DIAG" \
              | awk '{print $2}' | head -1)
        FDC=$(grep -E '^first_divergent_call:' "$DIAG" \
              | awk '{print $2}' | head -1)
        echo "PASS A19 (plain-text fallback — classification=$CLS; first_divergent_call=$FDC)"
    else
        echo "FAIL: A19 — DIAGNOSTIC.md frontmatter schema mismatch (plain-text fallback)"
        exit 1
    fi
else
    "$PY_BIN" - "$DIAG" <<'PY' || { echo "FAIL: A19 — DIAGNOSTIC.md frontmatter schema mismatch"; exit 1; }
import sys, yaml
text = open(sys.argv[1]).read()
fm = yaml.safe_load(text.split("---")[1])
assert "first_divergent_call" in fm, "missing first_divergent_call"
assert fm["first_divergent_call"], "empty first_divergent_call"
cls = fm.get("classified_divergence")
assert cls in ("DIV-A", "DIV-B", "DIV-C", "DIV-D", "unknown"), (
    f"bad classified_divergence: {cls}"
)
if cls == "unknown":
    print(
        "PASS A19 (structural — classification=unknown, A18 expected to "
        "fail; phase_complete handled at gate-aggregation layer)"
    )
else:
    print(f"PASS A19 (classification={cls}; "
          f"first_divergent_call={fm['first_divergent_call']})")
PY
fi

# -----------------------------------------------------------------------------
# Step 4: Six-tag carry-forward self-check (RESEARCH §5)
# -----------------------------------------------------------------------------
echo ""
echo "=== Six-tag carry-forward self-check (RESEARCH §5) ==="
for tag in BND_SLOT_SHORT_TOTAL BND_VTABLE_MISS BND_LAYOUT_TRANSLATE \
           GK_CALLER GK_CALLER_TIER2 GE_PRIM_DETAIL; do
    COUNT=$(grep -ac "\[$tag" /tmp/p112.log || true)
    if [ "${COUNT:-0}" -eq 0 ]; then
        echo "FAIL: instrumentation tag [$tag] never fired — Plan 03 regressed carry-forward (RESEARCH §6 Pitfall 1)"
        exit 1
    fi
    echo "PASS: [$tag] count=$COUNT"
done

# -----------------------------------------------------------------------------
# Step 5: output/generated regression check (CLAUDE.md rule 1)
# -----------------------------------------------------------------------------
echo ""
echo "=== output/generated regression check (CLAUDE.md rule 1) ==="
GEN_DIFF=$(git diff --name-only HEAD~3..HEAD -- output/generated/ 2>/dev/null || true)
if [ -n "$GEN_DIFF" ]; then
    echo "FAIL: Plan 03 modified output/generated/* in last 3 commits"
    echo "$GEN_DIFF"
    exit 1
fi
echo "PASS: no output/generated modifications in last 3 commits"

# -----------------------------------------------------------------------------
# Step 6: Phase 11.3 H2-b-wrap preservation
# -----------------------------------------------------------------------------
echo ""
echo "=== Phase 11.3 H2-b-wrap preservation (RESEARCH §1 Deferred Ideas) ==="
H2B=$(grep -c 'BND_LAYOUT_TRANSLATE' runtime/src/main.cpp || true)
if [ "${H2B:-0}" -lt 2 ]; then
    echo "FAIL: Phase 11.3 H2-b-wrap appears modified ([BND_LAYOUT_TRANSLATE] count=$H2B, expected ≥ 2)"
    exit 1
fi
echo "PASS: [BND_LAYOUT_TRANSLATE] count=$H2B (Phase 11.3 H2-b-wrap intact)"

# -----------------------------------------------------------------------------
# Final report
# -----------------------------------------------------------------------------
echo ""
echo "================================================================"
echo " ALL GATES PASSED: Phase 11.5 (A1-A12 + A18 + A19 + 6-tag carry-forward)"
echo "================================================================"
exit 0
