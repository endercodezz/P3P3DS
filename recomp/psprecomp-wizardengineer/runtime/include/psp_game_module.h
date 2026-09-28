// psp_game_module.h — per-game module seam (issues #46 / #47 Phase 4).
//
// The runtime core is game-agnostic; everything address-keyed or
// title-specific lives in games/<id>/runtime/*.cpp, compiled in via the
// CMake cache var PSPRECOMP_GAME ("none" = generic build, zero game hooks).
// Exactly ONE translation unit provides the strong psp_game_module()
// definition: the selected game module, or runtime/src/psp_game_default.cpp
// when PSPRECOMP_GAME=none.
#pragma once

#include <cstdint>

struct recomp_context;

/// Registration hooks a game module exposes to the generic boot path.
/// Every function pointer is non-null (use no-op bodies, not nullptr) so
/// call sites stay unconditional.
struct PspGameModule {
    /// Game id (e.g. "patapon"; "" for the generic default module). Compared
    /// at boot against RECOMP_GAME_ID from the output dir's generated
    /// recomp_game_config.h — a mismatch prints a loud warning.
    const char* id;
    /// After psp_hle_init(), before data sections / boot: every
    /// psp_dispatch_register wrapper, allocator/CRT override init,
    /// asset-layer init, io-policy setter.
    void (*register_hooks)(uint8_t* rdram);
    /// Immediately before entry() (module_start): boot-context tweaks
    /// (e.g. Patapon's k0+4 dlmalloc heap descriptor, plan decision P12).
    void (*on_boot_context)(uint8_t* rdram, recomp_context* ctx);
    /// After each PSP thread's k0 block is built
    /// (psp_hle_kernel_thread.cpp); k0_addr is the thread's k0 base.
    void (*on_thread_start)(uint8_t* rdram, uint32_t k0_addr);
};

/// The compiled-in game module (exactly one strong definition per build).
const PspGameModule* psp_game_module();
