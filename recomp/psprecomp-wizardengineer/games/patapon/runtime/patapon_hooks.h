// games/patapon/runtime/patapon_hooks.h — internal cross-TU declarations of
// the Patapon game module (issues #46 / #47 Phase 5).
//
// Everything declared here is PATAPON-PRIVATE: defined in one of the
// games/patapon/runtime/*.cpp translation units and consumed only by the
// others (hooks_main.cpp drives registration through the PspGameModule
// seam). Nothing in runtime/src or runtime/include may include this header —
// the purity gate (runtime/tools/purity_gate.sh) enforces that no generic
// object file references these symbols.
#pragma once

#include <cstdint>

struct recomp_context;

// ---- hooks_memory.cpp (formerly runtime/src/hle/psp_hle_kernel_memory.cpp) --

/// Resolve the guest functions wrapped by the kernel-memory hooks against
/// the PRISTINE dispatch table. MUST be called immediately after
/// psp_init_dispatch_table(), before any psp_dispatch_register override —
/// later resolution would capture hook wrappers instead of the original
/// generated functions (#47 P1; replaces link-time extern FUN_* refs).
void psp_hle_kernel_memory_resolve_guest_funcs();

/// Override the game's dlmalloc allocator (FUN_0881E558, FUN_0881E7A8)
/// with a native bump allocator. Must be called after psp_hle_init().
void psp_dlmalloc_override_init();

/// Override the game's CRT memory functions (memmove, memcpy, memset)
/// with native implementations. The recompiled MIPS versions use
/// LWL/LWR/SWL/SWR (unaligned access) which are emitted as no-op stubs,
/// causing infinite loops or data corruption in copy prologues.
void psp_crt_override_init();

/// GE-gatekeeper fixup body (GE_BASE_FIX + GE_GATE_FIX) — called by the
/// consolidated gatekeeper wrapper in hooks_main.cpp (Phase 11.2 Pattern F
/// FORM 2) and forwards to the resolved FUN_088623E0 original.
void hle_debug_088623E0(uint8_t* rdram, recomp_context* ctx);

// ---- hooks_thread.cpp (formerly psp_hle_kernel_thread.cpp overrides) -------

/// Override the game's CRT libc assertion handler (FUN_088133EC) that
/// prints "no reent structure found" and calls sceKernelExitThread(1).
/// The override returns gracefully instead of killing the thread.
void psp_crt_assertion_override_init();

/// Install the Patapon callback-dispatch observer (IoAsyncCallback
/// FUN_088629CC arg-struct dump) on the generic kernel callback seam.
void patapon_install_callback_observer();

// ---- hooks_dispatch.cpp (formerly psp_dispatch.cpp probes) -----------------

/// Install the Patapon LOOKUP_MISS handler: IO-slot field dump,
/// [BND_VTABLE_MISS] punch list, and the 0x438 corrupt-vtable workaround
/// (signals sema uid=259 to keep the render pipeline flowing).
void patapon_install_miss_handler();

// ---- hooks_io.cpp (formerly psp_hle_io.cpp BND reroute / diagnostics) ------

/// Install the Patapon IO policy: DATA_CMN.BND archive backing for missing
/// async opens, BND IO-slot staging, extraction-artifact stub rejection,
/// and the SGXD NULL-path tripwire dump.
void patapon_install_io_policy();

// ---- hooks_ge.cpp (formerly psp_ge_vertex.cpp NDC-direct fallback) ---------

/// Install the Patapon degenerate-matrix vertex fallback (NDC-direct ortho
/// mapping, issue #27) on the generic GE vertex seam.
void patapon_install_ge_vertex_fallback();
