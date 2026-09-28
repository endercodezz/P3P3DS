// psp_game_default.cpp — the no-op game module (issues #46 / #47 Phase 4).
//
// Compiled in when the runtime is configured with -DPSPRECOMP_GAME=none:
// a pure generic build with ZERO game hooks registered. A well-behaved
// game should boot this way; games/<id>/runtime/ modules exist for
// curated workarounds, not for bring-up table stakes.

#include "psp_game_module.h"

namespace {

void noop_register_hooks(uint8_t* /*rdram*/) {}

void noop_on_boot_context(uint8_t* /*rdram*/, recomp_context* /*ctx*/) {
    // Generic boot leaves the k0 area exactly as main() built it
    // (k0+4 heap descriptor stays 0 — an unconfigured PPSSPP-style k0
    // area, plan decision P12).
}

void noop_on_thread_start(uint8_t* /*rdram*/, uint32_t /*k0_addr*/) {}

const PspGameModule g_default_module = {
    /* id             */ "",
    /* register_hooks */ noop_register_hooks,
    /* on_boot_context*/ noop_on_boot_context,
    /* on_thread_start*/ noop_on_thread_start,
};

}  // namespace

const PspGameModule* psp_game_module() {
    return &g_default_module;
}
