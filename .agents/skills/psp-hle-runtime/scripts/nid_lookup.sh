#!/usr/bin/env bash
# Name a PSP NID and find every local source that describes it.
#   nid_lookup.sh <NID hex, e.g. 0x682A619B> [function name]
# Run from the repository root.
set -uo pipefail
nid=${1:?NID}
hex=$(printf '%08X' "$((nid))")
echo "== PSPSDK import stubs"
grep -rhi "0x$hex" psp/pspsdk/src --include=*.S | head -5
name=${2:-$(grep -rhi "0x$hex" psp/pspsdk/src --include=*.S | head -1 | sed -E 's/.*,[[:space:]]*([A-Za-z0-9_]+)[[:space:]]*$/\1/')}
echo "name: ${name:-<unknown>}"
echo "== uOFW exports / sources"
grep -rni "0x$hex" references/uofw/src --include=*.exp | head -5
[ -n "$name" ] && grep -rnl "$name" references/uofw/src references/uofw/include 2>/dev/null | head -8
echo "== pspautotests (tests and hardware .expected)"
[ -n "$name" ] && grep -rl "$name" references/pspautotests/tests 2>/dev/null | grep -E '\.(c|cpp|expected)$' | head -12
echo "== already registered in core/"
grep -rni "0x$hex" core/src | head -5
