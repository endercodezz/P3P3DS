// games/TEMPLATE/runtime/hooks_main.cpp — per-game module skeleton (issue #68).
//
// This is the minimal game module: it registers ZERO hooks, so it behaves
// exactly like the generic build (runtime/src/psp_game_default.cpp) while still
// providing a strong psp_game_module() keyed to this title's id. Copy
// games/TEMPLATE/ to games/<id>/, rename TEMPLATE -> <id> below and in
// game.toml, and add hooks only as bring-up demands them.
//
// THE SEAM (runtime/include/psp_game_module.h):
//   struct PspGameModule {
//       const char* id;                                   // must equal RECOMP_GAME_ID
//       void (*register_hooks)(uint8_t* rdram);           // after psp_hle_init(), before data sections
//       void (*on_boot_context)(uint8_t* rdram, recomp_context* ctx);  // just before entry()
//       void (*on_thread_start)(uint8_t* rdram, uint32_t k0_addr);     // per PSP thread
//   };
//   const PspGameModule* psp_game_module();               // EXACTLY ONE strong def per build
//
// Every function pointer MUST be non-null (use the no-op bodies below, never
// nullptr) so the generic call sites in runtime/src stay unconditional. The
// runtime selects this module at build time via -DPSPRECOMP_GAME=<id>, which
// makes CMake glob every *.cpp under this directory into the executable.
//
// CMake adds games/<id>/runtime/ to the include path, so a per-game private
// header (e.g. games/<id>/runtime/<id>_hooks.h, mirroring patapon_hooks.h) can
// declare cross-TU symbols once you split hooks across files. The purity gate
// (runtime/tools/purity_gate.sh) forbids any generic object file from
// referencing those symbols.

#include "psp_game_module.h"

namespace {

/// Register address-keyed / title-specific hooks. Called once at boot, after
/// psp_hle_init() bound the import stubs and before the data sections load —
/// the dispatch table is pristine here, so this is where you would:
///   - psp_dispatch_register(<guest addr>, <wrapper>)  to wrap a guest function
///   - install an asset-layer / io-policy / GE-fallback hook through a runtime
///     seam (see games/patapon/runtime/hooks_main.cpp for the real examples)
/// The skeleton registers nothing -> identical to the generic build.
void template_register_hooks(uint8_t* /*rdram*/) {}

/// Tweak the boot context immediately before entry() (module_start). The
/// generic build leaves k0 exactly as main() built it; a game can poke its
/// CRT heap descriptor or other boot-time guest state here. No-op skeleton.
void template_on_boot_context(uint8_t* /*rdram*/, recomp_context* /*ctx*/) {}

/// Run after each PSP thread's k0 block is built (k0_addr = that thread's k0
/// base). Use for per-thread guest state a title's CRT expects. No-op skeleton.
void template_on_thread_start(uint8_t* /*rdram*/, uint32_t /*k0_addr*/) {}

const PspGameModule g_template_module = {
    /* id              */ "TEMPLATE",  // MUST match [game] id and -DPSPRECOMP_GAME=<id>
    /* register_hooks  */ template_register_hooks,
    /* on_boot_context */ template_on_boot_context,
    /* on_thread_start */ template_on_thread_start,
};

}  // namespace

const PspGameModule* psp_game_module() {
    return &g_template_module;
}
