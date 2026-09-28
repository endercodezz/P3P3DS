#!/usr/bin/env bash
# ============================================================================
# runtime/tools/purity_gate.sh — generic-runtime purity gate
# (issues #46 / #47 Phase 5 close-out check)
#
# Proves the runtime core is free of game-specific code two ways:
#
#   1. SOURCE RESIDUE: every game-range hex literal (0x08xxxxxx/0x09xxxxxx)
#      under runtime/src + runtime/include must be one of the documented
#      PSP-universal class-(b) constants (memory-map bases, kernel range,
#      pointer-validity guards). Any other literal is a quarantine escape.
#
#   2. OBJECT SYMBOLS: nm over the runtime-core object files of a build dir
#      (every object EXCEPT those compiled from games/) must contain no
#      defined OR undefined symbol that belongs to a game module: generated
#      guest functions (FUN_0xxxxxxx), the Patapon module installers, or the
#      BND asset layer. Undefined references would mean game code leaked
#      INTO core (core cannot link without the game module); defined ones
#      would mean game code was compiled into core.
#
#      NOTE: the check runs on OBJECT files, not the linked executable —
#      the executable always links the per-game psp::recomp library, whose
#      thousands of FUN_* symbols are the recompiled game by construction.
#
# Usage:
#   runtime/tools/purity_gate.sh [build-dir]
#
#   build-dir defaults to runtime/build-none. Configure one with:
#     cmake -B runtime/build-none -S runtime -DPSPRECOMP_GAME=none
#     cmake --build runtime/build-none -j$(sysctl -n hw.ncpu)
#   A PSPRECOMP_GAME=patapon build dir is also valid input: game objects
#   (paths containing /games/) are excluded, so the same invariant is
#   checked continuously on the everyday build.
#
# Exit: 0 = pure; 1 = violations found (each printed with file:line / symbol).
# ============================================================================
set -u

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
BUILD_DIR="${1:-$ROOT/runtime/build-none}"
FAILURES=0

note() { printf '%s\n' "$*"; }

# ---------------------------------------------------------------------------
# 1. Source residue: only documented class-(b) PSP-universal literals allowed.
#    Allowlist (see DEBUGGING.md "purity gate"):
#      0x08000000  PSP_KERNEL_MEM_BASE / PSP_MEM_SIZE / pointer-validity guards
#      0x08000100  BOOT_MODULE_ADDR (NativeModule placement in kernel arena)
#      0x083FFFFF  kernel memory range end (doc comment)
#      0x08400000  KERNEL_MEM_END (kernel arena bound)
#      0x08800000  PSP_USER_MEM_BASE (user-memory base / module base)
# ---------------------------------------------------------------------------
ALLOWED_RE='^0x(08000000|08000100|083FFFFF|08400000|08800000)$'

residue=$(grep -rnoE '0x0[89][0-9A-Fa-f]{6}' \
              "$ROOT/runtime/src" "$ROOT/runtime/include" 2>/dev/null \
          | awk -F: -v allowed="$ALLOWED_RE" '$3 !~ allowed { print }')
if [ -n "$residue" ]; then
    note "FAIL [source residue] non-allowlisted game-range literals in runtime core:"
    note "$residue"
    FAILURES=$((FAILURES + 1))
else
    note "PASS [source residue] runtime core literals are allowlisted class-(b) only"
fi

# ---------------------------------------------------------------------------
# 2. Object symbols: core objects must not define or reference game symbols.
# ---------------------------------------------------------------------------
if [ ! -d "$BUILD_DIR" ]; then
    note "FAIL [objects] build dir not found: $BUILD_DIR"
    note "  configure one: cmake -B runtime/build-none -S runtime -DPSPRECOMP_GAME=none && cmake --build runtime/build-none"
    exit 1
fi

OBJ_ROOT="$BUILD_DIR/CMakeFiles/psprecomp_runtime.dir"
core_objs=$(find "$OBJ_ROOT" -name '*.o' 2>/dev/null | grep -v '/games/')
if [ -z "$core_objs" ]; then
    note "FAIL [objects] no core object files under $OBJ_ROOT — build first"
    exit 1
fi

# Game-symbol pattern: generated guest functions, Patapon installers/override
# entry points, and the BND asset layer. Matched against both defined and
# undefined nm entries (mangled names contain these as substrings).
GAME_SYM_RE='FUN_0[0-9A-Fa-f]{7}|[Pp]atapon|bnd_init|bnd_resolve_and_allocate|bnd_find_outer|bnd_find_virt|bnd_get_outer|bnd_get_entry|bnd_virt_path|psp_alloc_bnd_arena|asset_bnd|g_bnd_|fallback_to_shared_stub|psp_dlmalloc_override_init|psp_crt_override_init|psp_crt_assertion_override_init|hle_debug_088623E0|psp_hle_kernel_memory_resolve_guest_funcs|PSP_BND_ARENA'

sym_fail=0
for obj in $core_objs; do
    hits=$(nm "$obj" 2>/dev/null | grep -E "$GAME_SYM_RE")
    if [ -n "$hits" ]; then
        note "FAIL [objects] game symbols in core object ${obj#$BUILD_DIR/}:"
        printf '%s\n' "$hits" | sed 's/^/    /'
        sym_fail=1
    fi
done
if [ "$sym_fail" -eq 0 ]; then
    note "PASS [objects] $(printf '%s\n' "$core_objs" | wc -l | tr -d ' ') core objects free of game symbols"
else
    FAILURES=$((FAILURES + 1))
fi

# ---------------------------------------------------------------------------
# 3. Generic-build sanity: a PSPRECOMP_GAME=none build must contain no
#    games/ objects at all (the glob must not have leaked in).
# ---------------------------------------------------------------------------
cache_game=$(grep -E '^PSPRECOMP_GAME:' "$BUILD_DIR/CMakeCache.txt" 2>/dev/null | cut -d= -f2)
if [ "$cache_game" = "none" ] || [ -z "$cache_game" ]; then
    game_objs=$(find "$OBJ_ROOT" -name '*.o' 2>/dev/null | grep '/games/')
    if [ -n "$game_objs" ]; then
        note "FAIL [generic build] games/ objects present in a PSPRECOMP_GAME=none build:"
        printf '%s\n' "$game_objs" | sed 's/^/    /'
        FAILURES=$((FAILURES + 1))
    else
        note "PASS [generic build] no games/ objects in the generic build"
    fi
else
    note "INFO [generic build] build dir is PSPRECOMP_GAME=$cache_game — games/ objects expected and excluded from the symbol scan"
fi

if [ "$FAILURES" -gt 0 ]; then
    note "PURITY GATE: FAIL ($FAILURES check(s) failed)"
    exit 1
fi
note "PURITY GATE: PASS"
exit 0
