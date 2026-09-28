// games/patapon/runtime/hooks_main.cpp — the Patapon game module
// (issues #46 / #47 Phase 4).
//
// Everything address-keyed or title-specific that used to live inline in
// runtime/src/main.cpp: the ~44 psp_dispatch_register hooks (corruption
// detectors, BND/asset hooks, GK/GE-gate patches, GE callback faithful
// stubs, allocator probes), the dlmalloc/CRT/assertion override init calls,
// and the boot/thread k0+4 heap-descriptor writes. Compiled in only when
// the runtime is configured with -DPSPRECOMP_GAME=patapon (the default);
// -DPSPRECOMP_GAME=none builds a generic runtime containing none of this.
//
// Hook bodies are moved VERBATIM from main.cpp (relocation, not rewrite);
// the original block numbering (4a..4z) is preserved so existing planning
// docs and handoffs still cross-reference. Behavioral (non-diagnostic)
// hooks carry REMOVAL CRITERION comments per issue #46 acceptance.

#include "recomp.h"
#include "psp_memory.h"
#include "psp_game_module.h"
#include "hle/psp_hle.h"
#include "asset_bnd.h"
#include "patapon_hooks.h"

#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <map>
#include <mutex>
#include <set>
#include <string>

// From psp_dispatch.cpp
extern thread_local uint32_t g_last_func_addr;

// From emitter output (init_array.cpp) — used by the opt-in
// PSPRECOMP_RUN_CTORS boot experiment in patapon_on_boot_context.
extern void psp_call_constructors(uint8_t* rdram, recomp_context* ctx);

// ─────────────────────────────────────────────────────────────────────────────
// [BND_SLOT_SHORT_TOTAL] atexit handler (Phase 11.2 / Pattern D synthesis from
// psp_dispatch.cpp:67-118 + psp_ge.cpp:47-54).
//
// Phase 11.1's [BND_SLOT_SHORT] wrapper uses a rate-limited log gate
// (count <= 8 || count % 500 == 0) that HIDES the true firing total — the
// post-Phase-11.1 picture showed 1,000+ firings reported as "10" by the
// visible-line counter. Phase 11.2 A7 acceptance requires the actual lifetime
// total ≤ 50; this atexit handler emits the canonical total line exactly
// once at process exit (only when the counter is non-zero, matching Source 1's
// "if empty return" convention so PSPRECOMP_BND_DISABLE / GE_TEST_ONLY runs
// stay silent).
//
// The counter g_bnd_short_total is incremented inside the FUN_08862C14 SM
// wrapper closure below (block 4e; replacing the previous local static
// bnd_short_count). The on-first-use atexit registration block lives inside
// the same closure — see Pattern D §"Canonical shape".
// ─────────────────────────────────────────────────────────────────────────────
static int g_bnd_short_total = 0;
static bool g_bnd_short_atexit_registered = false;
static void dump_bnd_short_total() {
    if (g_bnd_short_total == 0) return;
    std::fprintf(stderr,
        "[BND_SLOT_SHORT_TOTAL] %d firings over process lifetime\n",
        g_bnd_short_total);
}

namespace {

/// All Patapon dispatch hooks + allocator/CRT overrides. Called by main()
/// through the PspGameModule seam after psp_hle_init(), exactly where the
/// hook block used to start inline.
void patapon_register_hooks(uint8_t* rdram) {
    // 0. Resolve the kernel-memory hooks' wrapped guest functions against
    //    the still-pristine dispatch slots (issue #47 Phase 1) BEFORE any
    //    psp_dispatch_register below or inside psp_*_override_init replaces
    //    them with wrappers. psp_hle_init() has already bound the HLE import
    //    stubs, but those live at the import-stub addresses (0x089D75xx),
    //    disjoint from every wrapped guest function, so the slots this
    //    resolves are still the original generated functions.
    psp_hle_kernel_memory_resolve_guest_funcs();

    // 0b. LOOKUP_MISS handler (#47 P5 seam): IO-slot dump, BND vtable-miss
    //     punch list, and the 0x438 corrupt-vtable workaround — formerly
    //     inline in psp_dispatch.cpp's noop_stub.
    patapon_install_miss_handler();

    // 0c. IO policy (#47 P5 seam): DATA_CMN.BND archive backing for missing
    //     async opens, BND IO-slot staging, "BND\0" extraction-artifact
    //     rejection, SGXD NULL-path tripwire — formerly inline in
    //     psp_hle_io.cpp.
    patapon_install_io_policy();

    // 0d. GE degenerate-matrix vertex fallback (#47 P5 seam, #27 family):
    //     the Patapon-ortho NDC-direct mapping — formerly inline in
    //     psp_ge_vertex.cpp.
    patapon_install_ge_vertex_fallback();

    // 4a. Install corruption detector wrapper for FUN_0885fe90
    // FUN_0885fe90 receives the render object as a0 (r[4]).
    // When the linked list has a corrupt pointer (< 0x08000000),
    // this wrapper logs the caller and the corrupt address.
    {
        FuncPtr orig_0885fe90 = RECOMP_LOOKUP(0x0885FE90);
        static FuncPtr s_orig = nullptr;
        s_orig = orig_0885fe90;
        static auto wrapper = [](uint8_t* rdram, recomp_context* ctx) {
            uint32_t a0 = static_cast<uint32_t>(ctx->r[4]);
            if (a0 != 0 && a0 < 0x08000000U) {
                static int corrupt_count = 0;
                corrupt_count++;
                if (corrupt_count <= 5) {
                    // Read the linked list node that provided this pointer
                    // FUN_0885efc8 loaded it from node+8 where node=r[19]
                    // By this point r[19] may have been overwritten, but
                    // we can scan for the value in heap
                    std::fprintf(stderr,
                        "[CORRUPT] FUN_0885fe90 called with invalid a0=0x%08X "
                        "(caller=0x%08X, #%d)\n",
                        a0, g_last_func_addr, corrupt_count);
                }
            }
            s_orig(rdram, ctx);
        };
        // Can't use lambda with captures as FuncPtr directly.
        // Use a static function instead.
        psp_dispatch_register(0x0885FE90,
            [](uint8_t* rdram, recomp_context* ctx) {
                uint32_t a0 = static_cast<uint32_t>(ctx->r[4]);
                static int total_calls = 0;
                total_calls++;
                if (total_calls <= 30 || total_calls % 100 == 0) {
                    // Trace BOTH vtable dispatch chains
                    uint32_t state = psp_mem_read<uint32_t>(rdram, a0 + 172);
                    // Chain 1: *(this+0) → sub+24 → vtbl → vtbl+12
                    uint32_t sub1 = psp_mem_read<uint32_t>(rdram, a0);
                    uint32_t vtbl1 = 0, vf1 = 0;
                    if (sub1 >= 0x08000000U) {
                        vtbl1 = psp_mem_read<uint32_t>(rdram, sub1 + 24);
                        if (vtbl1 != 0) vf1 = psp_mem_read<uint32_t>(rdram, vtbl1 + 12);
                    }
                    // Chain 2: *(this+220) → obj → obj+16 → vtbl → vtbl+12
                    uint32_t ref220 = psp_mem_read<uint32_t>(rdram, a0 + 220);
                    uint32_t obj2 = 0, vtbl2 = 0, vf2 = 0;
                    if (ref220 >= 0x08000000U) {
                        obj2 = psp_mem_read<uint32_t>(rdram, ref220 + 16);
                        if (obj2 >= 0x08000000U) {
                            vtbl2 = psp_mem_read<uint32_t>(rdram, obj2 + 12);
                        }
                    }
                    std::fprintf(stderr,
                        "[WRAP] #%d a0=0x%08X st=%d vf1=0x%08X ref220=0x%08X obj2=0x%08X vf2=0x%08X\n",
                        total_calls, a0, state, vf1, ref220, obj2, vtbl2);
                }
                // [GK_CALLER] instrumentation — Phase 11.4 / Plan 01 (R11.4-01).
                // Per RESEARCH §2.1, the state field at obj+172 determines whether
                // the gate path is taken: state==2 → gate, state==3 → bypass.
                // We log state on every entry so Plan 02 can detect the
                // self-terminating pattern (H4-A) without re-instrumenting.
                static int gk_caller_0885fe90_count = 0;
                gk_caller_0885fe90_count++;
                if (gk_caller_0885fe90_count <= 8 || gk_caller_0885fe90_count % 500 == 0) {
                    uint32_t gkc_state = 0xFFFFFFFFU;
                    if (a0 >= 0x08000000U) {
                        gkc_state = psp_mem_read<uint32_t>(rdram, a0 + 172);
                    }
                    std::fprintf(stderr,
                        "[GK_CALLER] caller=0x0885FE90 site=1 a0=0x%08X state=%u #%d\n",
                        a0, gkc_state, gk_caller_0885fe90_count);
                }
                if (a0 != 0 && a0 < 0x08000000U) {
                    // Module-base fix is applied in hle_debug_0885FE90 (memory.cpp).
                    // By the time we reach s_orig here the ctx->r[4] should
                    // already have been patched. Log if it still looks bad.
                    static int cc = 0;
                    cc++;
                    if (cc <= 5) {
                        std::fprintf(stderr,
                            "[WRAP_CORRUPT] FUN_0885fe90 a0=0x%08X "
                            "caller=0x%08X #%d (should have been fixed already)\n",
                            a0, g_last_func_addr, cc);
                    }
                }
                s_orig(rdram, ctx);
            });
        std::fprintf(stderr, "[RT] Corruption detector installed for FUN_0885fe90\n");
    }

    // 4a2. Diagnostic wrapper for FUN_0885efc8 (render-object counted loop).
    // Confirms the count-vs-actual-list-length OVERRUN behind the 0x438 crash:
    // loop runs count=*(container+0x98A0) times walking the list head=
    // *(container+0x9824) (next at node+4). If count > real nodes, the walk reads
    // *(0+8)=rdram[8] as a bogus object -> FUN_0885fe90 -> 0x438.
    {
        FuncPtr orig_efc8 = RECOMP_LOOKUP(0x0885EFC8);
        static FuncPtr s_efc8 = orig_efc8;
        psp_dispatch_register(0x0885EFC8,
            [](uint8_t* rdram, recomp_context* ctx) {
                uint32_t a0 = static_cast<uint32_t>(ctx->r[4]);
                if (a0 >= 0x08000000U && a0 < 0x0A000000U) {
                    uint32_t count = psp_mem_read<uint32_t>(rdram, a0 + 0x98A0U);
                    uint32_t head  = psp_mem_read<uint32_t>(rdram, a0 + 0x9824U);
                    uint32_t node = head, nodes = 0;
                    while (node >= 0x08000000U && node < 0x0A000000U && nodes < 100000U) {
                        nodes++;
                        node = psp_mem_read<uint32_t>(rdram, node + 4U);
                    }
                    bool overrun = (count > nodes);
                    static int n = 0, ov = 0;
                    // log first few, plus EVERY overrun (capped), plus any count>=5
                    if ((overrun && ++ov <= 20) || (count >= 5U && ++n <= 30) || (++n <= 8)) {
                        uint32_t obj = 0, n0=0,n4=0,n8=0,n12=0;
                        if (head >= 0x08000000U && head < 0x0A000000U) {
                            n0 = psp_mem_read<uint32_t>(rdram, head + 0U);
                            n4 = psp_mem_read<uint32_t>(rdram, head + 4U);
                            n8 = psp_mem_read<uint32_t>(rdram, head + 8U);
                            n12 = psp_mem_read<uint32_t>(rdram, head + 12U);
                            obj = n8; // object = *(node+8)
                        }
                        std::fprintf(stderr,
                            "[EFC8] container=0x%08X count=%u head=0x%08X nodes=%u  node[+0]=%08X [+4]=%08X [+8/obj]=%08X [+12]=%08X%s\n",
                            a0, count, head, nodes, n0, n4, n8, n12,
                            (obj != 0 && obj < 0x08000000U) ? "  *** OBJ NOT A POINTER ***" : "");
                    }
                }
                s_efc8(rdram, ctx);
            });
        std::fprintf(stderr, "[RT] EFC8 render-loop diagnostic installed\n");
    }

    // 4b. CONSOLIDATED wrapper for FUN_088623E0 (GE list gatekeeper) — Phase 11.2.
    //
    // Consolidates two pre-existing wrappers per 11.2-PATTERNS.md Pattern F:
    //   - hle_debug_088623E0 (runtime/src/hle/psp_hle_kernel_memory.cpp:1005-1093)
    //     — added in Phase 06.2 to fix register corruption from the L_0885FFB4
    //     vtable callee. Performs GE_BASE_FIX + GE_GATE_FIX then tail-calls
    //     FUN_088623e0. Forward-declared in runtime/include/hle/psp_hle.h
    //     since Phase 11.2 (linkage changed from `static` to external).
    //   - [GK_GATE] lambda (this block, was main.cpp:165-207) — logs
    //     sema_uid at obj+56 and force-overrides v0=-1 → 0.
    //
    // FORM 2 chosen (per Pattern F decision): we KEEP hle_debug_088623E0's
    // function body in psp_hle_kernel_memory.cpp because it accesses the
    // file-scope thread_local g_current_obj_sm (set by hle_debug_0885FE90).
    // We call hle_debug_088623E0() from here for sections (a)+(b)+(e); it
    // forwards to FUN_088623e0 internally so we do NOT separately call
    // s_gk_orig (would invoke twice). Section (e) is therefore satisfied
    // INSIDE hle_debug_088623E0's tail call.
    //
    // Section ordering inside the lambda:
    //   (c) [GK_GATE] entry log — sema_uid at obj+56 (rate-limited)
    //   (d) PHASE 11.2 H1/H2/H3 FIX INSERTION POINT — empty placeholder
    //       reserved for Plan 03 (Wave 2). Wave 0 (this plan) deliberately
    //       leaves it empty so the consolidation refactor is independently
    //       verifiable.
    //   (a)+(b)+(e) — call hle_debug_088623E0(rdram, ctx). Performs:
    //                 GE_BASE_FIX (r4 from RQ_GLOBAL_ADDR=0x08A87DD4),
    //                 GE_GATE_FIX (r6/r8/r10 from g_current_obj_sm),
    //                 then forwards to FUN_088623e0 at its tail.
    //   (f) [GK] post-return v0 log (rate-limited)
    //   (g) [GK_FORCE] v0=-1 → 0 override (kept per Pitfall 3 acknowledgement)
    //
    // REMOVAL CRITERION (behavioral — delegates to hle_debug_088623E0's
    // GE_BASE_FIX/GE_GATE_FIX register reconstruction): delete when the
    // L_0885FFB4 vtable-callee register corruption is fixed at the emitter
    // level (the CLEANROOM gate passing with PSPRECOMP_CLEANROOM=1 pure
    // pass-through proves the fix).
    {
        FuncPtr orig_gk = RECOMP_LOOKUP(0x088623E0);
        static FuncPtr s_gk_orig = nullptr;
        s_gk_orig = orig_gk;  // captured but unused — see FORM 2 note above.
        (void)s_gk_orig;
        psp_dispatch_register(0x088623E0,
            [](uint8_t* rdram, recomp_context* ctx) {
                uint32_t a0 = static_cast<uint32_t>(ctx->r[4]);
                static int gk_count = 0;
                gk_count++;

                // (c) [GK_GATE] entry log — rate-limited per Pattern C
                // (bumped from <= 5 to <= 8 to match canonical).
                if (gk_count <= 8 || gk_count % 500 == 0) {
                    uint32_t sema_uid = 0;
                    if (a0 >= 0x08000000U) {
                        sema_uid = psp_mem_read<uint32_t>(rdram, a0 + 56);
                    }
                    std::fprintf(stderr,
                        "[GK_GATE] #%d a0=0x%08X obj+56=0x%08X (sema_uid=%d)\n",
                        gk_count, a0, sema_uid,
                        static_cast<int32_t>(sema_uid));
                }

                // [GK_BUF] Asset-region buffer divergence probe (default OFF).
                // Oracle: at the asset-load call (caller FUN_0885F7E4 -> 0x0886001C)
                // PPSSPP passes a3(r7)=0x08f477c0 (real region buffer) + t0(r8)=size.
                // Our NO_STUB run passes a3=0 -> slot+292=0 -> whole-file fallback.
                // a3 = *(*(r17+0)+0) in the caller; capture a1..a3,t0,t1 here.
                if (std::getenv("PSPRECOMP_GK_BUF")) {
                    std::fprintf(stderr,
                        "[GK_BUF] #%d a0=0x%08X a1=0x%08X a2=0x%08X a3=0x%08X "
                        "t0=0x%08X t1=0x%08X caller=0x%08X\n",
                        gk_count, a0,
                        (uint32_t)ctx->r[5], (uint32_t)ctx->r[6],
                        (uint32_t)ctx->r[7], (uint32_t)ctx->r[8],
                        (uint32_t)ctx->r[9], g_last_func_addr);
                }

                // ─────────────────────────────────────────────────────────
                // (d) PHASE 11.2 H1/H2/H3 FIX INSERTION POINT — Plan 03 / Wave 2
                // Plan 02 picks H1, H2, or H3 from the diagnostic decompile
                // + lldb session and writes the fix here. THIS PLAN (Wave 0)
                // deliberately leaves this section empty so the consolidation
                // refactor is independently verifiable.
                // ─────────────────────────────────────────────────────────

                // (a)+(b)+(e) Delegate to hle_debug_088623E0 — performs the
                // GE_BASE_FIX (r4 reconstruction) + GE_GATE_FIX (r6/r8/r10
                // reconstruction from g_current_obj_sm) then forwards to
                // FUN_088623e0 internally. Do NOT also call s_gk_orig here
                // (FORM 2: hle_debug_088623E0 already forwarded).
                hle_debug_088623E0(rdram, ctx);

                // (f) [GK] post-return v0 log — rate-limited per Pattern C.
                int32_t v0 = static_cast<int32_t>(ctx->r[2]);
                if (gk_count <= 8 || gk_count % 500 == 0) {
                    std::fprintf(stderr,
                        "[GK] v0=%d #%d\n", v0, gk_count);
                }

                // (g) [GK_FORCE] REMOVED in Plan 11.3-03 — Pitfall 4 hygiene; real fix lives in H2-b-wrap per 11.3-DIAGNOSTIC.md.
            });
        std::fprintf(stderr,
            "[RT] CONSOLIDATED gatekeeper wrapper installed for FUN_088623E0 (merges main.cpp [GK_GATE] + hle_debug_088623E0)\n");
    }

    // 4b6. [GK_CALLER] instrumentation for FUN_08860490 — Phase 11.4 / Plan 01.
    //
    // Per RESEARCH §2.2, FUN_08860490 has TWO gate sites (batch_0051.cpp:202
    // = site 1; batch_0051.cpp:222 = site 2) discriminated at line 162 by
    // r[17] == 0 (selects site 2 / L_088605D0) vs r[17] != 0 (selects site
    // 1 / line 202 site). Both sites are gated upstream by r[22] != 0 at
    // line 84. We log site (predicted from r[17] at entry), r17, r22 so
    // Plan 02 can identify which site fires and whether the outer guard
    // ever holds.
    //
    // NOTE: r[17] and r[22] are PROLOGUE-OVERWRITTEN at the start of
    // FUN_08860490's body (RESEARCH §2.2 lines 47-83). Our wrapper runs
    // BEFORE the prologue — so we must read the args from the stack:
    //   r17 = MEM_W(sp + 24)  (a7 stack slot)
    //   r22 = MEM_W(sp + 40)  (a10 stack slot)
    {
        FuncPtr orig_08860490 = RECOMP_LOOKUP(0x08860490);
        static FuncPtr s_orig_08860490 = nullptr;
        s_orig_08860490 = orig_08860490;
        psp_dispatch_register(0x08860490,
            [](uint8_t* rdram, recomp_context* ctx) {
                static int gk_caller_08860490_count = 0;
                gk_caller_08860490_count++;
                if (gk_caller_08860490_count <= 8 || gk_caller_08860490_count % 500 == 0) {
                    uint32_t sp = static_cast<uint32_t>(ctx->r[29]);
                    uint32_t r17 = 0xDEADBEEFU;
                    uint32_t r22 = 0xDEADBEEFU;
                    if (sp >= 0x08000000U && (sp & PSP_ADDR_MASK) + 44U < PSP_MEM_SIZE) {
                        r17 = psp_mem_read<uint32_t>(rdram, sp + 24U);
                        r22 = psp_mem_read<uint32_t>(rdram, sp + 40U);
                    }
                    int site = (r17 == 0U) ? 2 : 1;
                    std::fprintf(stderr,
                        "[GK_CALLER] caller=0x08860490 site=%d r17=0x%08X r22=0x%08X #%d\n",
                        site, r17, r22, gk_caller_08860490_count);
                }
                s_orig_08860490(rdram, ctx);
            });
        std::fprintf(stderr, "[RT] GK_CALLER instrumentation installed for FUN_08860490\n");
    }

    // 4b7. [GK_CALLER] instrumentation for FUN_088623D4 — Phase 11.4 / Plan 01.
    //
    // Per RESEARCH §2.3, FUN_088623D4 is a pure no-guard adapter — 12 bytes
    // before FUN_088623E0 in the binary, shifts r9->r10 and zeroes r9 then
    // tail-calls the gate. The interesting question is WHO calls 0x088623D4,
    // which we capture via g_last_func_addr (declared extern thread_local
    // at main.cpp:29).
    {
        FuncPtr orig_088623D4 = RECOMP_LOOKUP(0x088623D4);
        static FuncPtr s_orig_088623D4 = nullptr;
        s_orig_088623D4 = orig_088623D4;
        psp_dispatch_register(0x088623D4,
            [](uint8_t* rdram, recomp_context* ctx) {
                static int gk_caller_088623d4_count = 0;
                gk_caller_088623d4_count++;
                if (gk_caller_088623d4_count <= 8 || gk_caller_088623d4_count % 500 == 0) {
                    std::fprintf(stderr,
                        "[GK_CALLER] caller=0x088623D4 site=1 invoked_by=0x%08X #%d\n",
                        g_last_func_addr, gk_caller_088623d4_count);
                }
                s_orig_088623D4(rdram, ctx);
            });
        std::fprintf(stderr, "[RT] GK_CALLER instrumentation installed for FUN_088623D4\n");
    }

    // 4b8. [GK_CALLER_TIER2] instrumentation for FUN_08862AD4 — Phase 11.4 / Plan 02.
    //
    // Per 11.4-DIAGNOSTIC.md (classified_hypothesis=H4-B, SEMA271 caller
    // breakdown) FUN_08862AD4 is the DOMINANT SEMA271 spinner with 720
    // [SEMA271] hits in a 12s normal run — RESEARCH §3.4 names it as one
    // of two prime H4-D candidates. Static analysis of FUN_08862ad4
    // (batch_0052.cpp) shows it calls 0x089D7720 / 0x088626A4 / 0x089D7670
    // / 0x089D76E8 plus an indirect via ctx->r[25] — NONE of which are
    // the three direct gatekeeper callers (FUN_0885FE90 / FUN_08860490
    // / FUN_088623D4). So this wrapper tests whether 08862AD4 actually
    // fires in the runtime (vs being a phantom SEMA271 attribution) and
    // logs g_last_func_addr to identify ITS upstream caller — pushing
    // the H4-B chain analysis one tier higher.
    {
        FuncPtr orig_08862AD4 = RECOMP_LOOKUP(0x08862AD4);
        static FuncPtr s_orig_08862AD4 = nullptr;
        s_orig_08862AD4 = orig_08862AD4;
        psp_dispatch_register(0x08862AD4,
            [](uint8_t* rdram, recomp_context* ctx) {
                static int gk_caller_tier2_08862ad4_count = 0;
                gk_caller_tier2_08862ad4_count++;
                uint32_t container = static_cast<uint32_t>(ctx->r[4]);
                uint32_t active_slot = 0;
                uint32_t slot_node_obj = 0;
                uint32_t slot_node_0 = 0;
                uint32_t slot_node_vt = 0;
                uint32_t slot_node_method = 0;
                uint32_t slot_node_16 = 0;
                uint32_t slot_172_obj = 0;
                uint32_t slot_52_obj = 0;
                uint32_t slot_332 = 0;
                uint32_t cb_table = 0;
                uint32_t cb_fn = 0;
                uint32_t slot_buf = 0;
                uint32_t slot_size = 0;
                uint32_t slot_off = 0;
                if (container >= 0x08000000U && container < 0x0C000000U) {
                    active_slot = psp_mem_read<uint32_t>(rdram, container + 40);
                    if (active_slot >= 0x08000000U && active_slot < 0x0C000000U) {
                        slot_node_obj = psp_mem_read<uint32_t>(rdram, active_slot + 328);
                        if (slot_node_obj >= 0x08000000U
                                && slot_node_obj < 0x0C000000U) {
                            slot_node_0 = psp_mem_read<uint32_t>(rdram, slot_node_obj + 0);
                            slot_node_16 = psp_mem_read<uint32_t>(rdram, slot_node_obj + 16);
                            if (slot_node_0 >= 0x08000000U
                                    && slot_node_0 < 0x0C000000U) {
                                slot_node_vt = psp_mem_read<uint32_t>(rdram, slot_node_0 + 0);
                                if (slot_node_vt >= 0x08000000U
                                        && slot_node_vt < 0x0C000000U) {
                                    slot_node_method = psp_mem_read<uint32_t>(
                                        rdram, slot_node_vt + 8);
                                }
                            }
                        }
                        slot_172_obj = psp_mem_read<uint32_t>(rdram, active_slot + 172);
                        slot_52_obj = psp_mem_read<uint32_t>(rdram, active_slot + 52);
                        slot_332 = psp_mem_read<uint32_t>(rdram, active_slot + 332);
                        cb_table = psp_mem_read<uint32_t>(rdram, active_slot + 356);
                        if (cb_table >= 0x08000000U && cb_table < 0x0C000000U) {
                            cb_fn = psp_mem_read<uint32_t>(rdram, cb_table + 8);
                        }
                        slot_buf = psp_mem_read<uint32_t>(rdram, active_slot + 292);
                        slot_size = psp_mem_read<uint32_t>(rdram, active_slot + 300);
                        slot_off = psp_mem_read<uint32_t>(rdram, active_slot + 308);
                    }
                }
                if (gk_caller_tier2_08862ad4_count <= 8
                        || gk_caller_tier2_08862ad4_count % 500 == 0) {
                    std::fprintf(stderr,
                        "[GK_CALLER_TIER2] addr=0x08862AD4 invoked_by=0x%08X #%d "
                        "slot=0x%08X node=0x%08X node0=0x%08X vt=0x%08X "
                        "meth=0x%08X node16=0x%08X obj172=0x%08X obj52=0x%08X "
                        "slot332=0x%08X cbtbl=0x%08X cbfn=0x%08X buf=0x%08X "
                        "size=%u off=%u\n",
                        g_last_func_addr, gk_caller_tier2_08862ad4_count,
                        active_slot, slot_node_obj, slot_node_0, slot_node_vt,
                        slot_node_method, slot_node_16, slot_172_obj, slot_52_obj,
                        slot_332, cb_table, cb_fn, slot_buf, slot_size, slot_off);
                }
                s_orig_08862AD4(rdram, ctx);
                if (active_slot >= 0x08000000U && active_slot < 0x0C000000U
                        && (gk_caller_tier2_08862ad4_count <= 8

                            || gk_caller_tier2_08862ad4_count % 500 == 0)) {
                    std::fprintf(stderr,
                        "[GK62_POST] slot=0x%08X state=%u s8=%u pending=%u "
                        "obj40=0x%08X qhead=0x%08X tail=%d active=%d\n",
                        active_slot,
                        psp_mem_read<uint32_t>(rdram, active_slot + 0),
                        psp_mem_read<uint32_t>(rdram, active_slot + 8),
                        psp_mem_read<uint32_t>(rdram, active_slot + 12),
                        psp_mem_read<uint32_t>(rdram, container + 40),
                        psp_mem_read<uint32_t>(rdram, container + 1808),
                        psp_mem_read<uint32_t>(rdram, container + 1824),
                        psp_mem_read<uint32_t>(rdram, container + 1828));
                }
            });
        std::fprintf(stderr, "[RT] GK_CALLER_TIER2 instrumentation installed for FUN_08862AD4\n");
    }

    // Diagnostic wrapper for the region-completion dispatcher used by slot
    // callbacks. We log the wrapper/node chain that FUN_08860314 resolves
    // through, but otherwise preserve the native control flow.
    {
        FuncPtr orig_08860314 = RECOMP_LOOKUP(0x08860314);
        static FuncPtr s_orig_08860314 = nullptr;
        s_orig_08860314 = orig_08860314;
        psp_dispatch_register(0x08860314,
            [](uint8_t* rdram, recomp_context* ctx) {
                uint32_t wrapper_obj = static_cast<uint32_t>(ctx->r[4]);
                uint32_t node0 = 0;
                uint32_t node0_word0 = 0;
                uint32_t node0_word8 = 0;
                uint32_t node1_word8 = 0;
                if (wrapper_obj >= 0x08000000U && wrapper_obj < 0x0C000000U) {
                    node0 = psp_mem_read<uint32_t>(rdram, wrapper_obj + 0U);
                }
                if (node0 >= 0x08000000U && node0 < 0x0C000000U) {
                    node0_word0 = psp_mem_read<uint32_t>(rdram, node0 + 0U);
                    node0_word8 = psp_mem_read<uint32_t>(rdram, node0 + 8U);
                }
                if (node0_word0 >= 0x08000000U && node0_word0 < 0x0C000000U) {
                    node1_word8 = psp_mem_read<uint32_t>(rdram, node0_word0 + 8U);
                }
                static int region_cb_wrap_n = 0;
                if (++region_cb_wrap_n <= 12) {
                    std::fprintf(stderr,
                        "[REGION_CB_WRAP] wrapper=0x%08X node0=0x%08X "
                        "node0[0]=0x%08X node0[8]=0x%08X node1[8]=0x%08X\n",
                        wrapper_obj, node0, node0_word0, node0_word8, node1_word8);
                }
                s_orig_08860314(rdram, ctx);
            });
        std::fprintf(stderr,
            "[RT] REGION callback diagnostic wrapper installed for FUN_08860314\n");
    }

    // Trace the callback thunk decoder used by FUN_088602FC / FUN_08860308.
    {
        FuncPtr orig_08804728 = RECOMP_LOOKUP(0x08804728);
        static FuncPtr s_orig_08804728 = nullptr;
        s_orig_08804728 = orig_08804728;
        psp_dispatch_register(0x08804728,
            [](uint8_t* rdram, recomp_context* ctx) {
                uint32_t thunk = static_cast<uint32_t>(ctx->r[25]);
                uint32_t base = static_cast<uint32_t>(ctx->r[4]);
                uint32_t add0 = 0;
                int32_t off_selector = 0;
                uint32_t thunk_next = 0;
                uint32_t adjusted_base = 0;
                uint32_t table_ptr = 0;
                uint32_t table_entry = 0;
                uint32_t final_callee = 0;
                if (thunk >= 0x08000000U && thunk < 0x0C000000U) {
                    add0 = psp_mem_read<uint32_t>(rdram, thunk + 0U);
                    off_selector = static_cast<int32_t>(
                        psp_mem_read<uint32_t>(rdram, thunk + 4U));
                    thunk_next = psp_mem_read<uint32_t>(rdram, thunk + 8U);
                    adjusted_base = base + add0;
                    if (off_selector >= 0) {
                        table_ptr = adjusted_base + thunk_next;
                        if (table_ptr >= 0x08000000U && table_ptr < 0x0C000000U) {
                            table_entry = psp_mem_read<uint32_t>(rdram, table_ptr);
                            uint32_t slot_ptr = table_entry + static_cast<uint32_t>(off_selector);
                            if (slot_ptr >= 0x08000000U && slot_ptr < 0x0C000000U) {
                                final_callee = psp_mem_read<uint32_t>(rdram, slot_ptr);
                            }
                        }
                    } else {
                        final_callee = thunk_next;
                    }
                }
                static int thunk_trace_n = 0;
                if (++thunk_trace_n <= 12) {
                    std::fprintf(stderr,
                        "[THUNK_04728] thunk=0x%08X base=0x%08X add=%u sel=%d "
                        "next=0x%08X adj=0x%08X table=0x%08X entry=0x%08X "
                        "target=0x%08X\n",
                        thunk, base, add0, off_selector, thunk_next,
                        adjusted_base, table_ptr, table_entry, final_callee);
                }
                s_orig_08804728(rdram, ctx);
            });
        std::fprintf(stderr, "[RT] THUNK_04728 diagnostic wrapper installed\n");
    }

    // 4b9. [GK_CALLER_TIER2] instrumentation for FUN_08863AA0 — Phase 11.4 / Plan 02.
    //
    // Per 11.4-DIAGNOSTIC.md FUN_08863AA0 is the SECOND DOMINANT SEMA271
    // spinner with 694 hits; also named in RESEARCH §3.4. Static analysis
    // of FUN_08863aa0 (batch_0052.cpp) shows it calls 0x089D7720 and
    // 0x089D76E8 — same WaitSema-family pattern as FUN_08862AD4. This
    // wrapper symmetrically captures who invokes it; if g_last_func_addr
    // reveals a single dominant caller, that caller becomes the Tier-3
    // investigation target for Phase 11.5.
    // CR-01 fix: ONE psp_dispatch_register call for FUN_08863AA0 that
    // composes both the [GK_CALLER_TIER2] tag instrumentation and the
    // [SEL_DIAG] priority-queue/slot dump in a single lambda. Previously
    // block 4f registered 0x08863AA0 a second time and silently overwrote
    // this lambda; [GK_CALLER_TIER2] only fired via pass-through chaining.
    {
        FuncPtr orig_08863AA0 = RECOMP_LOOKUP(0x08863AA0);
        static FuncPtr s_orig_08863AA0 = nullptr;
        s_orig_08863AA0 = orig_08863AA0;
        psp_dispatch_register(0x08863AA0,
            [](uint8_t* rdram, recomp_context* ctx) {
                static int gk_caller_tier2_08863aa0_count = 0;
                gk_caller_tier2_08863aa0_count++;
                if (gk_caller_tier2_08863aa0_count <= 8 || gk_caller_tier2_08863aa0_count % 500 == 0) {
                    std::fprintf(stderr,
                        "[GK_CALLER_TIER2] addr=0x08863AA0 invoked_by=0x%08X #%d\n",
                        g_last_func_addr, gk_caller_tier2_08863aa0_count);
                }
                // [SEL_DIAG] body (was block 4f). Capture obj BEFORE the
                // original call; read pq state AFTER (slot index in r[2]).
                uint32_t obj = static_cast<uint32_t>(ctx->r[4]);
                s_orig_08863AA0(rdram, ctx);
                static int sel_count = 0;
                sel_count++;
                if (sel_count <= 30) {
                    uint32_t qhead = 0, qtail = 0, qactive = 0;
                    uint32_t node_root = 0, n0 = 0, n8 = 0, n12 = 0;
                    uint32_t slot_arr = 0;
                    if (obj >= 0x08000000U && obj < 0x0A000000U) {
                        qhead = psp_mem_read<uint32_t>(rdram, obj + 1808);
                        qtail = psp_mem_read<uint32_t>(rdram, obj + 1824);
                        qactive = psp_mem_read<uint32_t>(rdram, obj + 1828);
                        node_root = psp_mem_read<uint32_t>(rdram, obj + 1836);
                        slot_arr  = psp_mem_read<uint32_t>(rdram, obj + 220);
                        if (node_root >= 0x08000000U && node_root < 0x0A000000U) {
                            n0  = psp_mem_read<uint32_t>(rdram, node_root + 0);
                            n8  = psp_mem_read<uint32_t>(rdram, node_root + 8);
                            n12 = psp_mem_read<uint32_t>(rdram, node_root + 12);
                        }
                    }
                    std::fprintf(stderr,
                        "[SEL_DIAG] FUN_08863AA0 #%d obj=0x%08X -> slot_idx=%d "
                        "qhead=%d qtail=%d qactive=%d node=0x%08X "
                        "n[0]=%u n[8]=%u n[12]=%u slot_arr=0x%08X\n",
                        sel_count, obj,
                        static_cast<int32_t>(ctx->r[2]),
                        static_cast<int32_t>(qhead),
                        static_cast<int32_t>(qtail),
                        static_cast<int32_t>(qactive),
                        node_root, n0, n8, n12, slot_arr);
                    if (slot_arr >= 0x08000000U && slot_arr < 0x0A000000U) {
                        for (int si = 0; si < 3; si++) {
                            uint32_t item = slot_arr + si * 360;
                            uint32_t s0   = psp_mem_read<uint32_t>(rdram, item + 0);
                            uint32_t s8   = psp_mem_read<uint32_t>(rdram, item + 8);
                            uint32_t s12  = psp_mem_read<uint32_t>(rdram, item + 12);
                            std::fprintf(stderr,
                                "[SEL_DIAG]   slot[%d]=0x%08X state=%u s8=%u asyncP=%u\n",
                                si, item, s0, s8, s12);
                        }
                    }
                }
            });
        std::fprintf(stderr, "[RT] GK_CALLER_TIER2 + SEL_DIAG composite wrapper installed for FUN_08863AA0\n");
    }

    // 4b2. Hook FUN_088616AC (render channel start — creates sema at obj+56)
    // Logs when called and what UID is stored at obj+56
    {
        FuncPtr orig_start = RECOMP_LOOKUP(0x088616AC);
        static FuncPtr s_start_orig = nullptr;
        s_start_orig = orig_start;
        psp_dispatch_register(0x088616AC,
            [](uint8_t* rdram, recomp_context* ctx) {
                uint32_t a0 = static_cast<uint32_t>(ctx->r[4]);
                uint32_t pre_44 = 0, pre_56 = 0;
                if (a0 >= 0x08000000U) {
                    pre_44 = psp_mem_read<uint32_t>(rdram, a0 + 44);
                    pre_56 = psp_mem_read<uint32_t>(rdram, a0 + 56);
                }
                std::fprintf(stderr,
                    "[SEMA_INIT] FUN_088616AC called: a0=0x%08X pre_44=%u pre_56=0x%08X\n",
                    a0, pre_44, pre_56);
                s_start_orig(rdram, ctx);
                uint32_t post_44 = 0, post_56 = 0;
                if (a0 >= 0x08000000U) {
                    post_44 = psp_mem_read<uint32_t>(rdram, a0 + 44);
                    post_56 = psp_mem_read<uint32_t>(rdram, a0 + 56);
                }
                std::fprintf(stderr,
                    "[SEMA_INIT] FUN_088616AC returned: post_44=%u post_56=0x%08X (sema_uid=%d)\n",
                    post_44, post_56, static_cast<int32_t>(post_56));
            });
        std::fprintf(stderr, "[RT] SEMA init hook installed for FUN_088616AC\n");
    }

    // 4b3. Hook FUN_0886095C (returns render queue object global)
    // Logs what address it returns
    {
        FuncPtr orig_gqobj = RECOMP_LOOKUP(0x0886095C);
        static FuncPtr s_gqobj_orig = nullptr;
        s_gqobj_orig = orig_gqobj;
        psp_dispatch_register(0x0886095C,
            [](uint8_t* rdram, recomp_context* ctx) {
                s_gqobj_orig(rdram, ctx);
                static int qobj_count = 0;
                qobj_count++;
                if (qobj_count <= 3) {
                    std::fprintf(stderr,
                        "[RQ_OBJ] FUN_0886095C -> 0x%08X (call #%d)\n",
                        static_cast<uint32_t>(ctx->r[2]), qobj_count);
                }
            });
        std::fprintf(stderr, "[RT] RQ object hook installed for FUN_0886095C\n");
    }

    // 4b4. Hook FUN_0886207C (render sync: WaitSemaCB + work + SignalSema on obj+56)
    // Logs the object address and what UID is at obj+56
    {
        FuncPtr orig_rsync = RECOMP_LOOKUP(0x0886207C);
        static FuncPtr s_rsync_orig = nullptr;
        s_rsync_orig = orig_rsync;
        psp_dispatch_register(0x0886207C,
            [](uint8_t* rdram, recomp_context* ctx) {
                uint32_t a0 = static_cast<uint32_t>(ctx->r[4]);
                int32_t a1 = static_cast<int32_t>(ctx->r[5]);
                int32_t a2 = static_cast<int32_t>(ctx->r[6]);
                uint32_t sema_uid = 0;
                int32_t qhead_before = 0;
                int32_t qtail_before = 0;
                int32_t qactive_before = 0;
                if (a0 >= 0x08000000U) {
                    sema_uid = psp_mem_read<uint32_t>(rdram, a0 + 56);
                    qhead_before = static_cast<int32_t>(
                        psp_mem_read<uint32_t>(rdram, a0 + 1808));
                    qtail_before = static_cast<int32_t>(
                        psp_mem_read<uint32_t>(rdram, a0 + 1824));
                    qactive_before = static_cast<int32_t>(
                        psp_mem_read<uint32_t>(rdram, a0 + 1828));
                }
                static int rsync_count = 0;
                rsync_count++;
                uint32_t rsync_ra = static_cast<uint32_t>(ctx->r[31]);
                if (rsync_count <= 8 || rsync_count % 500 == 0) {
                    std::fprintf(stderr,
                        "[RSYNC] #%d ra=0x%08X a0=0x%08X a1=%d a2=%d obj+56=0x%08X "
                        "(sema_uid=%d) qhead=%d qtail=%d qactive=%d\n",
                        rsync_count, rsync_ra, a0, a1, a2, sema_uid,
                        static_cast<int32_t>(sema_uid),
                        qhead_before, qtail_before, qactive_before);
                }
                s_rsync_orig(rdram, ctx);
                if (a0 >= 0x08000000U && (rsync_count <= 8 || rsync_count % 500 == 0)) {
                    std::fprintf(stderr,
                        "[RSYNC_POST] #%d qhead=%d qtail=%d qactive=%d\n",
                        rsync_count,
                        static_cast<int32_t>(psp_mem_read<uint32_t>(rdram, a0 + 1808)),
                        static_cast<int32_t>(psp_mem_read<uint32_t>(rdram, a0 + 1824)),
                        static_cast<int32_t>(psp_mem_read<uint32_t>(rdram, a0 + 1828)));
                }
                return;
            });
        std::fprintf(stderr, "[RT] RSYNC hook installed for FUN_0886207C\n");
    }

    // 4b4b. [DEQ_PROBE] Hook FUN_088620E8 (the dequeue/ack that sets slot+8=0).
    // Captures the caller ra + slot_idx + slot+8 BEFORE the clear, so we can
    // tell WHO acks systemdata's slot: the 0x1a5 auto-ack on the FileThread
    // (ra inside FUN_08862C14 ~0x08863xxx) vs the FE90 consumer path
    // (ra ~0x0886006c) vs the state-500 direct call. Read-only diagnostic.
    if (std::getenv("PSPRECOMP_GATE_PROBE") != nullptr) {
        FuncPtr orig_deq = RECOMP_LOOKUP(0x088620E8);
        static FuncPtr s_deq_orig = nullptr;
        s_deq_orig = orig_deq;
        psp_dispatch_register(0x088620E8,
            [](uint8_t* rdram, recomp_context* ctx) {
                uint32_t mgr = static_cast<uint32_t>(ctx->r[4]);
                int32_t  sidx = static_cast<int32_t>(ctx->r[5]);
                uint32_t ra = static_cast<uint32_t>(ctx->r[31]);
                uint32_t s8 = 0;
                if (mgr >= 0x08000000U && sidx >= 0 && sidx < 64) {
                    uint32_t arr = psp_mem_read<uint32_t>(rdram, mgr + 0xdc);
                    if (arr >= 0x08000000U)
                        s8 = psp_mem_read<uint32_t>(
                            rdram, arr + static_cast<uint32_t>(sidx) * 0x168U + 8);
                }
                std::fprintf(stderr,
                    "[DEQ_PROBE] FUN_088620E8 ra=0x%08X mgr=0x%08X slot_idx=%d "
                    "slot+8(before clear)=%u\n", ra, mgr, sidx, s8);
                s_deq_orig(rdram, ctx);
            });
        std::fprintf(stderr, "[RT] DEQ_PROBE installed for FUN_088620E8\n");

        // [POLL_PROBE] Hook FUN_08861FFC (consumer poll: returns slot+8).
        // This is what FE90 state-3 reads to detect completion (==6).
        FuncPtr orig_poll = RECOMP_LOOKUP(0x08861FFC);
        static FuncPtr s_poll_orig = nullptr;
        s_poll_orig = orig_poll;
        psp_dispatch_register(0x08861FFC,
            [](uint8_t* rdram, recomp_context* ctx) {
                uint32_t mgr = static_cast<uint32_t>(ctx->r[4]);
                int32_t  sidx = static_cast<int32_t>(ctx->r[5]);
                uint32_t ra = static_cast<uint32_t>(ctx->r[31]);
                s_poll_orig(rdram, ctx);
                uint32_t ret = static_cast<uint32_t>(ctx->r[2]);
                static int poll_n = 0;
                if (++poll_n <= 60)
                    std::fprintf(stderr,
                        "[POLL_PROBE] FUN_08861FFC ra=0x%08X mgr=0x%08X "
                        "slot_idx=%d -> slot+8=%u\n", ra, mgr, sidx, ret);
            });
        std::fprintf(stderr, "[RT] POLL_PROBE installed for FUN_08861FFC\n");
    }

    // 4b5b. Hook FUN_0895B788 (WaitSemaCB wrapper — reveals the render-slot object)
    {
        FuncPtr orig_wsema = RECOMP_LOOKUP(0x0895B788);
        static FuncPtr s_wsema_orig = nullptr;
        s_wsema_orig = orig_wsema;
        psp_dispatch_register(0x0895B788,
            [](uint8_t* rdram, recomp_context* ctx) {
                // r4 = obj+4 (points to sema uid field)
                // MEM_W(r4) = sema uid
                uint32_t a0 = static_cast<uint32_t>(ctx->r[4]);
                uint32_t sema_uid = 0;
                if (a0 >= 0x08000000U && a0 < 0x0C000000U) {
                    sema_uid = psp_mem_read<uint32_t>(rdram, a0);
                }
                static int wsema_count = 0;
                wsema_count++;
                if (wsema_count <= 10 || wsema_count % 500 == 0) {
                    std::fprintf(stderr,
                        "[WSEMA788] #%d a0=0x%08X sema_uid=%u "
                        "caller=0x%08X\n",
                        wsema_count, a0, sema_uid, g_last_func_addr);
                }
                s_wsema_orig(rdram, ctx);
            });
        std::fprintf(stderr, "[RT] WaitSema788 hook installed\n");
    }

    // 4b5c. Hook FUN_0895B598 (render slot dispatch — vtable+28 -> Signal(259))
    {
        FuncPtr orig_rslot = RECOMP_LOOKUP(0x0895B598);
        static FuncPtr s_rslot_orig = nullptr;
        s_rslot_orig = orig_rslot;
        psp_dispatch_register(0x0895B598,
            [](uint8_t* rdram, recomp_context* ctx) {
                uint32_t a0 = static_cast<uint32_t>(ctx->r[4]);
                uint32_t sema_uid = 0, obj_ptr = 0, vtable = 0, vfn = 0;
                if (a0 >= 0x08000000U && a0 < 0x0C000000U) {
                    sema_uid = psp_mem_read<uint32_t>(rdram, a0 + 4);
                    obj_ptr = psp_mem_read<uint32_t>(rdram, a0 + 8);
                    if (obj_ptr >= 0x08000000U && obj_ptr < 0x0C000000U) {
                        vtable = psp_mem_read<uint32_t>(rdram, obj_ptr + 0);
                        if (vtable >= 0x08000000U && vtable < 0x0C000000U) {
                            vfn = psp_mem_read<uint32_t>(rdram, vtable + 28);
                        }
                    }
                }
                static int rslot_count = 0;
                rslot_count++;
                if (rslot_count <= 10 || rslot_count % 1000 == 0) {
                    std::fprintf(stderr,
                        "[RSLOT] #%d a0=0x%08X sema=%u obj=0x%08X "
                        "vtable=0x%08X vfn=0x%08X\n",
                        rslot_count, a0, sema_uid, obj_ptr, vtable, vfn);
                }
                s_rslot_orig(rdram, ctx);
            });
        std::fprintf(stderr, "[RT] Render slot hook installed for FUN_0895B598\n");
    }

    // 4b5. Hook FUN_08816F20 (GE signal callback -- signals sema 259)
    // Logs when called and from which caller function
    {
        FuncPtr orig_sigcb = RECOMP_LOOKUP(0x08816F20);
        static FuncPtr s_sigcb_orig = nullptr;
        s_sigcb_orig = orig_sigcb;
        psp_dispatch_register(0x08816F20,
            [](uint8_t* rdram, recomp_context* ctx) {
                static int sigcb_count = 0;
                sigcb_count++;
                if (sigcb_count <= 20) {
                    std::fprintf(stderr,
                        "[SIGCB] FUN_08816F20 called #%d: "
                        "caller=0x%08X a0=0x%08X a1=0x%08X\n",
                        sigcb_count,
                        g_last_func_addr,
                        static_cast<uint32_t>(ctx->r[4]),
                        static_cast<uint32_t>(ctx->r[5]));
                }
                s_sigcb_orig(rdram, ctx);
            });
        std::fprintf(stderr, "[RT] Signal callback hook installed for FUN_08816F20\n");
    }

    // 4b6. Register missing dispatch entries for addresses Ghidra analysis missed
    // REMOVAL CRITERION (behavioral — faithful stand-ins for entries absent
    // from analysis.json): delete each stub once the address is recovered as
    // a real function/mid-entry by analyze or the manifest force lists
    // (games/patapon/game.toml [recompile]) and the dispatch table serves it.
    // (a) 0x08816F9C — GE finish handler (label inside FUN_08816F20, not
    //     emitted). Faithful to the real MIPS (issue #10):
    //         lw v0, 4(a1); andi a0, a0, 0xffff; if (v0) jalr v0
    //     i.e. read the finish sub-callback the game registered at
    //     *(arg+4) (GuSettings 0x08A49308 +4, set via sceGuSetCallback)
    //     and CALL it with a0 = id & 0xffff. Fired from psp_ge.cpp's
    //     GE_CMD_FINISH path with a carved guest sp in ctx->r[29].
    psp_dispatch_register(0x08816F9CU,
        [](uint8_t* rdram, recomp_context* ctx) {
            uint32_t fin = psp_mem_read<uint32_t>(rdram,
                static_cast<uint32_t>(ctx->r[5]) + 4);
            ctx->r[4] = static_cast<int64_t>(
                static_cast<uint32_t>(ctx->r[4]) & 0xFFFFU);
            ctx->r[2] = static_cast<int32_t>(fin);
            if (fin != 0) {
                static bool subcb_logged = false;
                if (!subcb_logged) {
                    std::fprintf(stderr,
                        "[GE] finish sub-cb dispatch fin=0x%08X "
                        "id=0x%04X\n",
                        fin, static_cast<uint32_t>(ctx->r[4]));
                    subcb_logged = true;
                }
                FuncPtr sub = RECOMP_LOOKUP(fin);
                if (sub) {
                    sub(rdram, ctx);
                } else {
                    static bool subcb_miss_logged = false;
                    if (!subcb_miss_logged) {
                        std::fprintf(stderr,
                            "[STUB-08816F9C] sub-cb RECOMP_LOOKUP"
                            "(0x%08X) MISS\n", fin);
                        subcb_miss_logged = true;
                    }
                }
            }
        });

    // (b) 0x08828080 — Epilogue mid-entry (label inside FUN_08827FA8).
    //     Restores saved regs from the caller's -48 byte PSP stack frame
    //     and deallocates it. Called via tail-jump from FUN_08827EB0.
    psp_dispatch_register(0x08828080U,
        [](uint8_t* rdram, recomp_context* ctx) {
            uint32_t sp = static_cast<uint32_t>(ctx->r[29]);
            ctx->r[31] = static_cast<int32_t>(
                psp_mem_read<uint32_t>(rdram, sp + 28));
            ctx->r[20] = static_cast<int32_t>(
                psp_mem_read<uint32_t>(rdram, sp + 24));
            ctx->r[19] = static_cast<int32_t>(
                psp_mem_read<uint32_t>(rdram, sp + 20));
            ctx->r[18] = static_cast<int32_t>(
                psp_mem_read<uint32_t>(rdram, sp + 16));
            ctx->r[17] = static_cast<int32_t>(
                psp_mem_read<uint32_t>(rdram, sp + 12));
            ctx->r[16] = static_cast<int32_t>(
                psp_mem_read<uint32_t>(rdram, sp +  8));
            ctx->r[29] = static_cast<int32_t>(sp + 48);
        });

    // (c) 0x08A44664 — Vtable virtual method (slot 4/offset 16) called from
    //     FUN_089b4df0's render list loop. Falls in Ghidra gap 0x08A4357C–
    //     0x08A49880. Return value NOT used by caller. No-op is safe.
    psp_dispatch_register(0x08A44664U,
        [](uint8_t* rdram, recomp_context* ctx) {
            static int call_count = 0;
            call_count++;
            if (call_count <= 5) {
                std::fprintf(stderr,
                    "[STUB-08A44664] vtable call #%d a0=0x%08X a1=0x%08X\n",
                    call_count,
                    static_cast<uint32_t>(ctx->r[4]),
                    static_cast<uint32_t>(ctx->r[5]));
            }
            ctx->r[2] = 0;
            (void)rdram;
        });

    std::fprintf(stderr,
        "[RT] Registered 3 missing dispatch stubs "
        "(0x08816F9C, 0x08828080, 0x08A44664)\n");

    // [REGION_BUF_FIX] Allocate the per-region destination buffer the native
    // asset state machine needs. Oracle (PPSSPP, FUN_088623E0/FUN_08863B6C):
    // each region request carries a real buffer pointer in t0(r8) (e.g.
    // 0x08f477c0 for titledata, size 1412264), which the state machine writes
    // to slot+292 (batch_0052.cpp:L_08863CF4). With slot+292 != 0 the machine
    // takes the region-read path (L_08863180: SEEK_SET + read into
    // slot+292+offset from DATA_CMN.BND). Our native (ASSET_NO_STUB) path
    // computes the correct sizes/offsets (227692@0x18800, 1091291@0x50800,
    // 81139@0x4800, 1412264@0x15b000 — byte-identical to PPSSPP) but t0(r8)=0,
    // so it falls into the whole-file SEEK_END fallback and reads the wrong
    // bytes -> no geometry. We allocate the destination buffer here (from the
    // dedicated BND arena, separate from the game pools so it never OOMs) and
    // inject it as r8 when a region request arrives with a NULL buffer.
    // request flows through. Active only on the native path (ASSET_NO_STUB) or
    // when PSPRECOMP_REGION_BUF_FIX is set; the default shim path is untouched.
    // so it falls into the whole-file SEEK_END fallback and reads the wrong
    // bytes -> no geometry. We allocate the destination buffer here (from the
    // dedicated BND arena, separate from the game pools so it never OOMs) and
    // inject it as r8 when a region request arrives with a NULL buffer.
    // request flows through. Active only on the native path (ASSET_NO_STUB) or
    // when PSPRECOMP_REGION_BUF_FIX is set; the default shim path is untouched.
    {
        bool region_fix_on = (std::getenv("PSPRECOMP_ASSET_NO_STUB") != nullptr)
                          || (std::getenv("PSPRECOMP_REGION_BUF_FIX") != nullptr);
        bool region_fix_off = (std::getenv("PSPRECOMP_REGION_BUF_FIX_OFF") != nullptr);
        if (region_fix_on && !region_fix_off) {
            FuncPtr orig_rb = RECOMP_LOOKUP(0x08863B6C);
            static FuncPtr s_rb_orig = nullptr;
            s_rb_orig = orig_rb;
            psp_dispatch_register(0x08863B6C,
                [](uint8_t* rdram, recomp_context* ctx) {
                    uint32_t r8   = static_cast<uint32_t>(ctx->r[8]);   // dest buffer
                    int32_t  size = static_cast<int32_t>(ctx->r[9]);    // region size
                    int32_t  off  = static_cast<int32_t>(ctx->r[10]);   // region offset
                    bool is_region = (size > 0 && off > 0);
                    bool bad_buf   = (r8 < 0x08000000U);
                    uint32_t arg12 = 0;
                    uint32_t arg12_f0 = 0;
                    uint32_t arg12_f4 = 0;
                    uint32_t arg12_f8 = 0;
                    uint32_t arg12_fc = 0;
                    uint32_t arg12_f10 = 0;
                    uint32_t arg12_f14 = 0;
                    uint32_t arg12_src0 = 0;
                    uint32_t arg12_src4 = 0;
                    uint32_t arg12_src18_0 = 0;
                    uint32_t arg12_m89f0c = 0;
                    uint32_t arg12_m89f18 = 0;
                    if (is_region
                            && static_cast<uint32_t>(ctx->r[29]) >= 0x08000004U) {
                        arg12 = psp_mem_read<uint32_t>(
                            rdram, static_cast<uint32_t>(ctx->r[29]) + 4U);
                        if (arg12 >= 0x08000000U && arg12 < 0x0C000000U) {
                            arg12_f0 = psp_mem_read<uint32_t>(rdram, arg12 + 0U);
                            arg12_f4 = psp_mem_read<uint32_t>(rdram, arg12 + 4U);
                            arg12_f8 = psp_mem_read<uint32_t>(rdram, arg12 + 8U);
                            arg12_fc = psp_mem_read<uint32_t>(rdram, arg12 + 12U);
                            arg12_f10 = psp_mem_read<uint32_t>(rdram, arg12 + 16U);
                            arg12_f14 = psp_mem_read<uint32_t>(rdram, arg12 + 20U);
                            if (arg12_f8 >= 0x08000000U && arg12_f8 < 0x0C000000U) {
                                arg12_src0 = psp_mem_read<uint32_t>(rdram, arg12_f8 + 0U);
                                arg12_src4 = psp_mem_read<uint32_t>(rdram, arg12_f8 + 4U);
                            }
                            arg12_src18_0 = psp_mem_read<uint32_t>(rdram, 0x08A44718U);
                            arg12_m89f0c = psp_mem_read<uint32_t>(rdram, 0x089F470CU);
                            arg12_m89f18 = psp_mem_read<uint32_t>(rdram, 0x089F4718U);
                        }
                    }
                    if (is_region) {
                        uint32_t static_4470c =
                            psp_mem_read<uint32_t>(rdram, 0x08A4470CU);
                        uint32_t static_44718 =
                            psp_mem_read<uint32_t>(rdram, 0x08A44718U);
                        if (static_4470c == 0U && arg12_m89f0c != 0U) {
                            psp_mem_write<uint32_t>(
                                rdram, 0x08A4470CU, arg12_m89f0c);
                            static int region_cb_static_seed_0c_n = 0;
                            if (++region_cb_static_seed_0c_n <= 12) {
                                std::fprintf(stderr,
                                    "[REGION_CB_STATIC] 0x08A4470C <- 0x%08X\n",
                                    arg12_m89f0c);
                            }
                        }
                        if (static_44718 == 0U && arg12_m89f18 != 0U) {
                            psp_mem_write<uint32_t>(
                                rdram, 0x08A44718U, arg12_m89f18);
                            static int region_cb_static_seed_18_n = 0;
                            if (++region_cb_static_seed_18_n <= 12) {
                                std::fprintf(stderr,
                                    "[REGION_CB_STATIC] 0x08A44718 <- 0x%08X\n",
                                    arg12_m89f18);
                            }
                        }
                    }
                    if (is_region && bad_buf) {
                        static std::map<uint64_t, uint32_t> s_region_buf_cache;
                        uint64_t key = (static_cast<uint64_t>(
                                            static_cast<uint32_t>(off)) << 32)
                                     | static_cast<uint32_t>(size);
                        auto it = s_region_buf_cache.find(key);
                        uint32_t buf = 0U;
                        bool cached = false;
                        if (it != s_region_buf_cache.end()) {
                            buf = it->second;
                            cached = true;
                        } else {
                            buf = psp_alloc_bnd_arena(
                                static_cast<uint32_t>(size), 256U);
                            if (buf != 0U) s_region_buf_cache[key] = buf;
                        }
                        if (buf != 0U) {
                            ctx->r[8] = static_cast<int32_t>(buf);
                            static int rb_n = 0;
                            if (++rb_n <= 24) {
                                std::fprintf(stderr,
                                    "[REGION_BUF_FIX] #%d size=%d off=%d "
                                    "buf 0x%08X (was 0x%08X)%s\n",
                                    rb_n, size, off, buf, r8,
                                    cached ? " [CACHED]" : "");
                                std::fprintf(stderr,
                                    "[REGION_SRC] arg12=0x%08X f0=0x%08X f4=0x%08X "
                                    "f8=0x%08X fc=0x%08X f10=0x%08X f14=0x%08X "
                                    "src0=0x%08X src4=0x%08X g18=0x%08X "
                                    "m89f0c=0x%08X m89f18=0x%08X "
                                    "a1=0x%08X a2=0x%08X a3=0x%08X\n",
                                    arg12, arg12_f0, arg12_f4, arg12_f8,
                                    arg12_fc, arg12_f10, arg12_f14,
                                    arg12_src0, arg12_src4, arg12_src18_0,
                                    arg12_m89f0c, arg12_m89f18,
                                    static_cast<uint32_t>(ctx->r[5]),
                                    static_cast<uint32_t>(ctx->r[6]),
                                    static_cast<uint32_t>(ctx->r[7]));
                            }
                            // [OBJ4_FIX] Bind the buffer onto the asset object's
                            // stream sub-object so the state machine (FUN_0885fe90)
                            // sees the region as loaded and ADVANCES instead of
                            // re-requesting it forever. Oracle (PPSSPP same-snapshot):
                            // FUN_0885DEC8 allocates this buffer via FUN_0885E06C and
                            // stores it at *(obj+0)+0; our runtime skips that alloc
                            // (its gate *(desc+12)>=9 is unmet), leaving it NULL ->
                            // FUN_08863B6C gets t0=NULL -> wrong branch -> fd-leak loop.
                            // Asset obj = a1 - 108 (a1 = obj+108, the region_obj arg).
                            if (std::getenv("PSPRECOMP_OBJ4_FIX_OFF") == nullptr) {
                                uint32_t a1 = static_cast<uint32_t>(ctx->r[5]);
                                if (a1 >= 0x08000000U + 108U) {
                                    uint32_t obj = a1 - 108U;
                                    uint32_t sub =
                                        psp_mem_read<uint32_t>(rdram, obj + 0);
                                    if (sub >= 0x08000000U && sub < 0x0A000000U) {
                                        uint32_t cur = psp_mem_read<uint32_t>(
                                            rdram, sub + 0);
                                        if (cur < 0x08000000U) {  // slot empty
                                            psp_mem_write<uint32_t>(
                                                rdram, sub + 0, buf);
                                            psp_mem_write<uint8_t>(
                                                rdram, sub + 8, 1);
                                            static int o4n = 0;
                                            if (++o4n <= 8) {
                                                std::fprintf(stderr,
                                                    "[OBJ4_FIX] obj=0x%08X "
                                                    "sub=0x%08X bound buf 0x%08X\n",
                                                    obj, sub, buf);
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                    s_rb_orig(rdram, ctx);
                });
            std::fprintf(stderr,
                "[RT] REGION_BUF_FIX installed for FUN_08863B6C "
                "(allocates region dest buffer when r8==NULL)\n");
        }
    }

    // [LOOSE_GROUP_ABSENT] FUN_08863980 opens disc0:LOADINGGROUP/<name>.BND to
    // check whether an asset has a loose-file override. On the RETAIL disc the
    // LOADINGGROUP directory does not exist, so this open fails (ENOENT) and the
    // game's loader then streams the asset from its DATA_CMN.BND region. Our
    // extracted disc only has degenerate 4-byte BND stubs (rejected as ENOENT by
    // is_degenerate_bnd_stub), so the open also fails -- but the caller's retry
    // loop (FUN_08861E28 L_08861EE4..EF8) has NO give-up branch and spins on the
    // single FileThread, STARVING the region reads for every asset after #1
    // (e.g. SYSTEMLOCALIZEDATA, region #2 = 1091291@0x50800 never enqueues).
    //
    // Routing field (verified live): *(manager+24216) = the per-asset loose-group
    // index `idx`. idx==-1 (SYSTEMDATA) -> FUN_08863980 returns early WITHOUT
    // opening -> region path. idx>=0 (SYSTEMLOCALIZEDATA) -> opens loose file ->
    // ENOENT -> spin. On retail the absent LOADINGGROUP must collapse to the
    // region path for idx>=0 assets too.
    //
    // FAITHFUL + GENERAL FIX: when FUN_08863980 returns a NEGATIVE value (the
    // loose-group open genuinely failed -- file absent on the retail disc), force
    // its return to 0 (the same "no loose override, use region" result the idx==-1
    // path returns). This collapses the loose-group probe to the archive-region
    // path for ANY asset whose loose-group file is absent -- exactly the retail
    // behavior -- and breaks the FileThread-starving infinite retry. Active only
    // on the native asset path (ASSET_NO_STUB) or when REGION_BUF_FIX is set; the
    // default shim path is untouched.
    {
        bool lga_on = (std::getenv("PSPRECOMP_ASSET_NO_STUB") != nullptr)
                   || (std::getenv("PSPRECOMP_REGION_BUF_FIX") != nullptr);
        bool lga_off = (std::getenv("PSPRECOMP_LOOSE_GROUP_ABSENT_OFF") != nullptr);
        if (lga_on && !lga_off) {
            FuncPtr orig_lga = RECOMP_LOOKUP(0x08863980);
            static FuncPtr s_lga_orig = nullptr;
            s_lga_orig = orig_lga;
            psp_dispatch_register(0x08863980,
                [](uint8_t* rdram, recomp_context* ctx) {
                    s_lga_orig(rdram, ctx);
                    int32_t ret = static_cast<int32_t>(ctx->r[2]);
                    if (ret < 0) {
                        // Loose-group BND open failed (absent on retail disc).
                        // Collapse to the "use region" result (0), matching the
                        // idx==-1 / no-loose-override path. Prevents the caller's
                        // give-up-less retry loop from starving the FileThread.
                        ctx->r[2] = 0;
                        static int lga_n = 0;
                        if (++lga_n <= 12) {
                            std::fprintf(stderr,
                                "[LOOSE_GROUP_ABSENT] #%d FUN_08863980 ret %d -> 0 "
                                "(loose group absent; route to region)\n",
                                lga_n, ret);
                        }
                    }
                });
            std::fprintf(stderr,
                "[RT] LOOSE_GROUP_ABSENT installed for FUN_08863980 "
                "(absent loose group -> region path)\n");
        }
    }

    // [B6C_TRACE] Diagnostic wrapper for FUN_08863B6C (region read-request
    // enqueue). PPSSPP oracle: first call passes a0=container, r9(size)=227692,
    // r10(offset)=first region offset -> slot+300=227692. Our runtime falls into
    // the whole-file SEEK_END path because slot+300==0. Capture r4..r11 + sp+84
    // to see the size/offset args our runtime passes. Env-gated, default OFF.
    if (std::getenv("PSPRECOMP_B6C_TRACE")) {
        FuncPtr orig_b6c = RECOMP_LOOKUP(0x08863B6C);
        static FuncPtr s_b6c_orig = nullptr;
        s_b6c_orig = orig_b6c;
        psp_dispatch_register(0x08863B6C,
            [](uint8_t* rdram, recomp_context* ctx) {
                static int b6c_n = 0;
                b6c_n++;
                uint32_t sp = static_cast<uint32_t>(ctx->r[29]);
                uint32_t sp84 = (sp >= 0x08000000U)
                    ? psp_mem_read<uint32_t>(rdram, sp + 84) : 0;
                if (b6c_n <= 12) {
                    std::fprintf(stderr,
                        "[B6C_TRACE] #%d a0=0x%08X a1=0x%08X a2=0x%08X a3=0x%08X "
                        "t0(r8)=0x%08X r9(size)=%d r10(off)=%d r11=0x%08X sp84=%d\n",
                        b6c_n,
                        (uint32_t)ctx->r[4], (uint32_t)ctx->r[5],
                        (uint32_t)ctx->r[6], (uint32_t)ctx->r[7],
                        (uint32_t)ctx->r[8], (int32_t)ctx->r[9],
                        (int32_t)ctx->r[10], (uint32_t)ctx->r[11], (int32_t)sp84);
                }
                s_b6c_orig(rdram, ctx);
            });
        std::fprintf(stderr, "[RT] B6C_TRACE wrapper installed for FUN_08863B6C\n");
    }

    // [AEC_TRACE] FUN_08861AEC computes the read buffer (r19) via a vtable
    // method dispatch on object r18 (= a0), gated by a 4-byte compare of
    // *(r18+72) against 0x089F8868. PPSSPP passes a non-zero buffer to
    // FUN_08862270/FUN_08863B6C (t0 != 0 -> slot+292 != 0 -> region read).
    // Our runtime passes t0=0 -> SEEK_END whole-file fallback. Capture the
    // object, *(obj+72) (device/type tag), *(obj+4) (vtable), and the buffer
    // returned, to see why the region buffer is NULL. Env-gated, default OFF.
    if (std::getenv("PSPRECOMP_AEC_TRACE")) {
        FuncPtr orig_aec = RECOMP_LOOKUP(0x08861AEC);
        static FuncPtr s_aec_orig = nullptr;
        s_aec_orig = orig_aec;
        psp_dispatch_register(0x08861AEC,
            [](uint8_t* rdram, recomp_context* ctx) {
                static int aec_n = 0;
                aec_n++;
                uint32_t obj = static_cast<uint32_t>(ctx->r[4]);
                uint32_t o72 = 0, o4 = 0, vt0 = 0, tag = 0;
                if (obj >= 0x08000000U && obj < 0x0A000000U) {
                    o72 = psp_mem_read<uint32_t>(rdram, obj + 72);
                    o4  = psp_mem_read<uint32_t>(rdram, obj + 4);
                    if (o72 >= 0x08000000U && o72 < 0x0A000000U)
                        tag = psp_mem_read<uint32_t>(rdram, o72);
                    if (o4 >= 0x08000000U && o4 < 0x0A000000U)
                        vt0 = psp_mem_read<uint32_t>(rdram, o4);
                }
                if (aec_n <= 12) {
                    std::fprintf(stderr,
                        "[AEC_TRACE] #%d obj=0x%08X a1=0x%08X a2=0x%08X a3=0x%08X "
                        "*(obj+72)=0x%08X tag=0x%08X *(obj+4)=0x%08X vt[0]=0x%08X\n",
                        aec_n, obj, (uint32_t)ctx->r[5], (uint32_t)ctx->r[6],
                        (uint32_t)ctx->r[7], o72, tag, o4, vt0);
                }
                s_aec_orig(rdram, ctx);
            });
        std::fprintf(stderr, "[RT] AEC_TRACE wrapper installed for FUN_08861AEC\n");
    }

    // [E28_TRACE] FUN_08861E28 is the open/route decision: it looks the asset
    // name (a1=r5, the obj+108 string) up in the manager's registry via
    // FUN_0896A6A4(manager+4680, name). If FOUND (r2!=0) -> region path (r18 =
    // *(r2+4) region index). If NOT FOUND (r2==0) -> r18=-2 -> loose-file open
    // of disc0:LOADINGGROUP/<name>.BND (the retry-spin). Capture the name string
    // and the lookup result to prove which asset routes which way. Env-gated.
    if (std::getenv("PSPRECOMP_E28_TRACE")) {
        FuncPtr orig_e28 = RECOMP_LOOKUP(0x08861E28);
        static FuncPtr s_e28_orig = nullptr;
        s_e28_orig = orig_e28;
        psp_dispatch_register(0x08861E28,
            [](uint8_t* rdram, recomp_context* ctx) {
                static int e28_n = 0;
                e28_n++;
                uint32_t slot = static_cast<uint32_t>(ctx->r[4]);   // a0
                uint32_t name_ptr = static_cast<uint32_t>(ctx->r[5]); // a1
                char name[96]; name[0] = 0;
                if (name_ptr >= 0x08000000U && name_ptr < 0x0A000000U) {
                    for (int i = 0; i < 95; i++) {
                        char c = (char)psp_mem_read<uint8_t>(rdram, name_ptr + i);
                        name[i] = c; if (!c) break; name[i+1] = 0;
                    }
                }
                // Peek the registry lookup result by replaying its first steps:
                // FUN_0896A6A4(manager=slot+4680, name). We can't cheaply re-run
                // it; instead just record the name + a1, and rely on whether a
                // loose open follows. Print name string for both assets.
                // Read the routing field *(entry+4) WITHOUT side effects: only
                // replay the lookup once per distinct asset (first time we see
                // the name), using an isolated scratch stack so FUN_0896A6A4's
                // stack writes don't corrupt the live state.
                static std::set<std::string> seen;
                if (seen.insert(name).second) {
                    recomp_context probe = *ctx;
                    probe.r[29] = 0x09F00000;  // isolated scratch sp (BND arena gap)
                    probe.r[4] = (int32_t)(slot + 4680);
                    probe.r[5] = (int32_t)name_ptr;
                    FuncPtr lk = RECOMP_LOOKUP(0x0896A6A4);
                    lk(rdram, &probe);
                    uint32_t entry = (uint32_t)probe.r[2];
                    int32_t f0 = 0, f4 = 0, f8 = 0;
                    if (entry >= 0x08000000U && entry < 0x0A000000U) {
                        f0 = (int32_t)psp_mem_read<uint32_t>(rdram, entry + 0);
                        f4 = (int32_t)psp_mem_read<uint32_t>(rdram, entry + 4);
                        f8 = (int32_t)psp_mem_read<uint32_t>(rdram, entry + 8);
                    }
                    std::fprintf(stderr,
                        "[E28_TRACE] name=\"%s\" entry=0x%08X *(entry+0)=%d "
                        "*(entry+4)=%d *(entry+8)=%d -> %s\n",
                        name, entry, f0, f4, f8,
                        (entry != 0 && f4 >= 0) ? "REGION" : "LOOSE/NOTFOUND");
                }
                // [E28_RET] capture the LIVE return r2 + obj+176 before/after,
                // and *(*(obj+0)) (the L_0885FF60 routing gate) for both assets.
                uint32_t pre176 = (slot >= 0x08000000U && slot < 0x0A000000U)
                    ? psp_mem_read<uint32_t>(rdram, (slot + 176) & 0x07FFFFFFU) : 0;
                uint32_t p180 = (slot >= 0x08000000U && slot < 0x0A000000U)
                    ? psp_mem_read<uint32_t>(rdram, (slot + 180) & 0x07FFFFFFU) : 0;
                s_e28_orig(rdram, ctx);
                int32_t ret = (int32_t)ctx->r[2];
                uint32_t sub = (slot >= 0x08000000U && slot < 0x0A000000U)
                    ? psp_mem_read<uint32_t>(rdram, slot & 0x07FFFFFFU) : 0;
                uint32_t subv0 = (sub >= 0x08000000U && sub < 0x0A000000U)
                    ? psp_mem_read<uint32_t>(rdram, sub & 0x07FFFFFFU) : 0xDEAD;
                static int e28r = 0;
                if (++e28r <= 30) {
                    std::fprintf(stderr,
                        "[E28_RET] #%d name=\"%s\" ret=%d pre176=%u +180=%u "
                        "*(obj+0)=0x%08X *(*(obj+0))=0x%08X\n",
                        e28r, name, ret, pre176, p180, sub, subv0);
                }
            });
        std::fprintf(stderr, "[RT] E28_TRACE wrapper installed for FUN_08861E28\n");
    }

    // [LK_TRACE] Wrap FUN_0896A6A4 (the manager registry lookup) DIRECTLY to
    // capture, for each asset-name lookup, the LIVE stack pointer, args, and
    // return value. The E28_TRACE replay (isolated scratch stack) FINDS
    // systemlocalizedata (entry 0x08AC8420, size 1091291) but the live E28
    // returns 0. This probe discriminates: does the live FUN_0896A6A4 itself
    // miss (lookup/stack bug), or does it find the entry but E28 routes wrong?
    // Run WITHOUT E28_TRACE (their replay calls RECOMP_LOOKUP(0x0896A6A4)).
    if (std::getenv("PSPRECOMP_LK_TRACE")) {
        FuncPtr orig_lk = RECOMP_LOOKUP(0x0896A6A4);
        static FuncPtr s_lk_orig = nullptr;
        s_lk_orig = orig_lk;
        psp_dispatch_register(0x0896A6A4,
            [](uint8_t* rdram, recomp_context* ctx) {
                uint32_t a0 = static_cast<uint32_t>(ctx->r[4]);
                uint32_t a1 = static_cast<uint32_t>(ctx->r[5]);
                uint32_t sp = static_cast<uint32_t>(ctx->r[29]);
                char name[96]; name[0] = 0;
                if (a1 >= 0x08000000U && a1 < 0x0A000000U) {
                    for (int i = 0; i < 95; i++) {
                        char c = (char)psp_mem_read<uint8_t>(rdram, (a1 + i) & 0x07FFFFFFU);
                        name[i] = c; if (!c) break; name[i+1] = 0;
                    }
                }
                bool interesting = (std::strstr(name, "system") != nullptr);
                // [LK_FIX] Clean, non-destructive correctness fix: FUN_0896A6A4 is a
                // pure registry search, so run it on an ISOLATED scratch stack (free
                // upper rdram) where there is no stale garbage, and return its correct
                // result + preserve callee-saved regs. The live stack is untouched, so
                // unlike frame-zeroing this introduces NO side-effects on other code.
                // Applies to ALL lookups (short names already correct; long names fixed).
                if (std::getenv("PSPRECOMP_LK_FIX")) {
                    recomp_context probe = *ctx;
                    probe.r[29] = 0x0FF00000;  // masks to 0x07F00000 — free upper rdram
                    s_lk_orig(rdram, &probe);
                    ctx->r[2]  = probe.r[2];
                    ctx->r[16] = probe.r[16];
                    ctx->r[17] = probe.r[17];
                    ctx->r[31] = probe.r[31];
                    if (interesting) {
                        static int lkfix_n = 0;
                        if (++lkfix_n <= 6)
                            std::fprintf(stderr, "[LK_FIX] name=\"%s\" -> ret=0x%08X\n",
                                name, (uint32_t)ctx->r[2]);
                    }
                    return;  // skip the buggy live-stack execution
                }
                // [LK_ZERO] Test the stack-garbage hypothesis: zero the region
                // the callee's frame + its sub-calls will use ([sp-4096, sp)),
                // mimicking the clean scratch stack that makes the replay FIND
                // the long name. If the long name now resolves -> the live miss
                // is stale-stack contamination of the stack-built string key.
                if (interesting && std::getenv("PSPRECOMP_LK_ZERO")) {
                    // Bisectable: zero [sp - ZHI, sp - ZLO). Defaults to the full
                    // 4096B window below sp. Narrow ZLO/ZHI to find the exact
                    // bytes whose garbage causes the long-name miss.
                    const char* zlo = std::getenv("PSPRECOMP_LK_ZLO");
                    const char* zhi = std::getenv("PSPRECOMP_LK_ZHI");
                    uint32_t off_lo = zlo ? (uint32_t)strtoul(zlo, nullptr, 0) : 0u;
                    uint32_t off_hi = zhi ? (uint32_t)strtoul(zhi, nullptr, 0) : 4096u;
                    uint32_t hi = sp - off_lo;
                    uint32_t lo = sp - off_hi;
                    for (uint32_t a = lo; a < hi; a += 4)
                        psp_mem_write<uint32_t>(rdram, a & 0x07FFFFFFU, 0);
                }
                s_lk_orig(rdram, ctx);
                int32_t ret = (int32_t)ctx->r[2];
                if (interesting) {
                    static int lk_n = 0;
                    if (++lk_n <= 40) {
                        std::fprintf(stderr,
                            "[LK_TRACE] a0=0x%08X sp=0x%08X name=\"%s\" "
                            "ret=0x%08X (%d)\n",
                            a0, sp, name, (uint32_t)ret, ret);
                    }
                    // [LK_KEY] After the lookup returns, its 528-byte frame still
                    // holds the stack-built key (sp+16) and copy (sp+272). Dump
                    // them: the byte where they diverge from the real name is the
                    // dropped-store / contamination point.
                    static int lk_key_n = 0;
                    if (std::getenv("PSPRECOMP_LK_KEY") && ++lk_key_n <= 8) {
                        uint32_t fsp = static_cast<uint32_t>(ctx->r[29]) - 528u;
                        for (uint32_t off : {16u, 272u}) {
                            char hex[200]; int p = 0; char asc[64];
                            for (int i = 0; i < 44; i++) {
                                uint8_t b = psp_mem_read<uint8_t>(rdram, (fsp + off + i) & 0x07FFFFFFU);
                                p += std::snprintf(hex + p, sizeof(hex) - p, "%02X ", b);
                                asc[i] = (b >= 0x20 && b < 0x7F) ? (char)b : '.';
                            }
                            asc[44] = 0;
                            std::fprintf(stderr,
                                "[LK_KEY] name=\"%s\" sp+%u ret=%d\n   hex: %s\n   asc: %s\n",
                                name, off, ret, hex, asc);
                        }
                    }
                }
            });
        std::fprintf(stderr, "[RT] LK_TRACE wrapper installed for FUN_0896A6A4\n");
    }

    // [GK_ZERO] Contrarian A/B: is FUN_088623E0's -1 (region-enqueue) the SAME
    // uninitialized-stack class as the lookup miss? Zero its frame + sub-call
    // frames before delegating. If it now returns >=0 (asset reaches state 3,
    // no re-read loop) -> the whole cascade is ONE translator bug class. If it
    // still returns -1 on a clean stack -> a DIFFERENT bug (do NOT assume same
    // root). Registers AFTER the consolidated gatekeeper wrapper, so it wraps
    // outermost. Env-gated; default-off.
    if (std::getenv("PSPRECOMP_GK_ZERO")) {
        FuncPtr orig_gkz = RECOMP_LOOKUP(0x088623E0);
        static FuncPtr s_gkz_orig = nullptr;
        s_gkz_orig = orig_gkz;
        psp_dispatch_register(0x088623E0,
            [](uint8_t* rdram, recomp_context* ctx) {
                uint32_t sp = static_cast<uint32_t>(ctx->r[29]);
                uint32_t lo = (sp >= 0x08000000U + 8192U) ? (sp - 8192U) : 0x08000000U;
                for (uint32_t a = lo; a < sp; a += 4)
                    psp_mem_write<uint32_t>(rdram, a & 0x07FFFFFFU, 0);
                s_gkz_orig(rdram, ctx);
                static int gkz_n = 0;
                if (++gkz_n <= 20)
                    std::fprintf(stderr, "[GK_ZERO] #%d sp=0x%08X ret=%d\n",
                        gkz_n, sp, (int32_t)ctx->r[2]);
            });
        std::fprintf(stderr, "[RT] GK_ZERO wrapper installed for FUN_088623E0\n");
    }

    // [FFC_ZERO] Layer-3 test: frame-clean FUN_08861FFC (async-status poll whose
    // return gates state-3 -> re-create/re-read). Same uninitialized-frame class.
    if (std::getenv("PSPRECOMP_FFC_ZERO")) {
        FuncPtr orig_ffc = RECOMP_LOOKUP(0x08861FFC);
        static FuncPtr s_ffc_orig = nullptr; s_ffc_orig = orig_ffc;
        psp_dispatch_register(0x08861FFC,
            [](uint8_t* rdram, recomp_context* ctx) {
                uint32_t sp = static_cast<uint32_t>(ctx->r[29]);
                uint32_t lo = (sp >= 0x08000000U + 4096U) ? (sp - 4096U) : 0x08000000U;
                for (uint32_t a = lo; a < sp; a += 4)
                    psp_mem_write<uint32_t>(rdram, a & 0x07FFFFFFU, 0);
                s_ffc_orig(rdram, ctx);
                static int n=0; if (++n<=12)
                    std::fprintf(stderr, "[FFC_ZERO] #%d ret=%d\n", n, (int32_t)ctx->r[2]);
            });
        std::fprintf(stderr, "[RT] FFC_ZERO wrapper installed for FUN_08861FFC\n");
    }

    // [ASSETPROC_ZERO] Frame-clean the asset-PROCESSING chain (infrequent, unlike
    // the render loop) to test if the uninit-frame class is what loops the asset.
    if (std::getenv("PSPRECOMP_ASSETPROC_ZERO")) {
        #define INSTALL_PZ(NAME, ADDR) \
            static FuncPtr s_pz_##NAME = RECOMP_LOOKUP(ADDR); \
            psp_dispatch_register((ADDR), [](uint8_t* rdram, recomp_context* ctx){ \
                uint32_t sp=static_cast<uint32_t>(ctx->r[29]); \
                uint32_t lo=(sp>=0x08000000U+4096U)?(sp-4096U):0x08000000U; \
                for(uint32_t a=lo;a<sp;a+=4) psp_mem_write<uint32_t>(rdram,a&0x07FFFFFFU,0); \
                s_pz_##NAME(rdram,ctx); })
        INSTALL_PZ(g23e0,0x088623E0);
        INSTALL_PZ(ffc,  0x08861FFC);
        INSTALL_PZ(p207c,0x0886207C);
        INSTALL_PZ(b6c,  0x08863B6C);
        #undef INSTALL_PZ
        std::fprintf(stderr, "[RT] ASSETPROC_ZERO installed (088623E0/08861FFC/0886207C/08863B6C)\n");
    }

    // [LOOSE_REGION_FIX] FUN_08861E28 is the asset-open/route decision called
    // from FUN_0885fe90:L_0885FF3C. Its RETURN VALUE (r2) is the gate (verified
    // [E28_RET]):
    //   WORKING (systemdata):       returns 227692 (the registry region size).
    //   STUCK   (systemlocalizedata): returns 0 (the loose disc0:LOADINGGROUP
    //                                 /<name>.BND ENOENTs on the retail disc).
    // The caller body (batch_0050.cpp L_0885FF40..) then computes, when
    // *(obj+176)==0: obj+176 = ret - *(obj+180). The working asset gets
    // obj+176 = 227692 (-> dispatch path -> state 3 in ONE state-2 visit).
    // The stuck asset gets obj+176 = 0 -> never advances -> re-dispatched ~97x.
    //
    // FIX (general, ANY loose-absent asset): after the original runs, if it
    // returned a dead-end value (<=0) but the asset name resolves to a valid
    // registry region, override r2 with (region_size + *(obj+180)). Then the
    // body computes obj+176 = (size + p180) - p180 = size = the SAME value the
    // working asset's region path produces -> dispatch -> state 3. We do NOT
    // seed obj+176 directly (proven harmful: the body must derive it from ret).
    // STATUS (2026-06-01): the override is VERIFIED to make the caller compute
    // obj+176=1091291 correctly and take the dispatch path, BUT the asset still
    // does NOT reach state 3: FUN_088623E0 (region read enqueue, L_08860018)
    // returns -1 for systemlocalizedata, so *(obj+104) stays -1, the state-3
    // write at L_0886002C is skipped, and the object re-dispatches in an fd-leak
    // re-read loop (#2..#19+, fd 6->19). This is the SAME harmful loop the
    // handoff warned about. The true blocker is one level deeper (FUN_088623E0
    // returning -1), NOT the FUN_08861E28 return value. Therefore this fix is
    // OPT-IN ONLY (PSPRECOMP_LOOSE_REGION_FIX=1) and NOT auto-enabled under
    // ASSET_NO_STUB, so it never introduces the fd-leak loop by default.
    {
        bool lrf_on = (std::getenv("PSPRECOMP_LOOSE_REGION_FIX") != nullptr);
        bool lrf_off = (std::getenv("PSPRECOMP_LOOSE_REGION_FIX_OFF") != nullptr);
        if (lrf_on && !lrf_off) {
            FuncPtr orig_lrf = RECOMP_LOOKUP(0x08861E28);
            static FuncPtr s_lrf_orig = nullptr;
            s_lrf_orig = orig_lrf;
            psp_dispatch_register(0x08861E28,
                [](uint8_t* rdram, recomp_context* ctx) {
                    uint32_t obj = static_cast<uint32_t>(ctx->r[4]);   // a0
                    uint32_t name_ptr = static_cast<uint32_t>(ctx->r[5]); // a1
                    s_lrf_orig(rdram, ctx);
                    int32_t ret = (int32_t)ctx->r[2];
                    // Only correct dead-end returns for a real asset object.
                    if (ret > 0 || obj < 0x08000000U || obj >= 0x0A000000U) return;
                    // Resolve the registry region size by replaying the manager
                    // lookup on an isolated scratch stack (BND-arena gap), the
                    // proven [E28_TRACE] technique. manager+4680 == 0x08A89050.
                    char nm[8] = {0};
                    bool printable = true;
                    for (int i = 0; i < 1; i++) {
                        char c = (char)psp_mem_read<uint8_t>(rdram,
                            (name_ptr) & 0x07FFFFFFU);
                        if (c < 0x20 || c > 0x7E) printable = false;
                        nm[0] = c;
                    }
                    if (!printable) return;
                    recomp_context probe = *ctx;
                    probe.r[29] = 0x09F00000;
                    probe.r[4]  = (int32_t)0x08A89050U;   // manager+4680
                    probe.r[5]  = (int32_t)name_ptr;
                    FuncPtr lk = RECOMP_LOOKUP(0x0896A6A4);
                    if (!lk) return;
                    lk(rdram, &probe);
                    uint32_t entry = (uint32_t)probe.r[2];
                    if (entry < 0x08000000U || entry >= 0x0A000000U) return;
                    int32_t size = (int32_t)psp_mem_read<uint32_t>(rdram,
                        (entry + 4) & 0x07FFFFFFU);
                    if (size <= 0) return;
                    uint32_t p180 = psp_mem_read<uint32_t>(rdram,
                        (obj + 180) & 0x07FFFFFFU);
                    // Override r2 so caller computes obj+176 = (size+p180)-p180.
                    ctx->r[2] = (int32_t)((uint32_t)size + p180);
                    static int lrf_n = 0;
                    if (++lrf_n <= 12) {
                        std::fprintf(stderr,
                            "[LOOSE_REGION_FIX] obj=0x%08X loose-absent ret=%d "
                            "-> r2=%u (size=%d + *(obj+180)=%u) entry=0x%08X\n",
                            obj, ret, (uint32_t)ctx->r[2], size, p180, entry);
                    }
                });
            std::fprintf(stderr,
                "[RT] LOOSE_REGION_FIX installed for FUN_08861E28 "
                "(loose-absent -> registry region)\n");
        }
    }

    // [O980_TRACE] FUN_08863980 = the loose-open+validate. *(a0+61) is an
    // "already-resolved" flag: if non-zero it returns success WITHOUT opening.
    // Capture a0 and *(a0+61) to see why systemdata never opens loose but
    // systemlocalizedata spins. Env-gated.
    if (std::getenv("PSPRECOMP_O980_TRACE")) {
        FuncPtr orig_980 = RECOMP_LOOKUP(0x08863980);
        static FuncPtr s_980_orig = nullptr;
        s_980_orig = orig_980;
        psp_dispatch_register(0x08863980,
            [](uint8_t* rdram, recomp_context* ctx) {
                static int n980 = 0; n980++;
                uint32_t a0 = (uint32_t)ctx->r[4];
                uint32_t f61 = 0, f60 = 0;
                if (a0 >= 0x08000000U && a0 < 0x0C000000U) {
                    f61 = psp_mem_read<uint8_t>(rdram, a0 + 61);
                    f60 = psp_mem_read<uint8_t>(rdram, a0 + 60);
                }
                // Replicate FUN_08863980's descriptor lookup to read *(desc+48),
                // the field that gates loose-open vs region-skip:
                //   idx  = *(obj + 0x50000 - 25784)   (signed)
                //   desc = *(obj+220) + (idx*168)      (168 = ((idx<<4)-idx)*... )
                //   gate = *(desc+48)
                int32_t idx = (a0 >= 0x08000000U)
                    ? (int32_t)psp_mem_read<uint32_t>(rdram, a0 + (0x50000U - 25784U)) : -1;
                uint32_t base220 = (a0 >= 0x08000000U)
                    ? psp_mem_read<uint32_t>(rdram, a0 + 220) : 0;
                int32_t gate48 = -999;
                if (idx >= 0 && base220 >= 0x08000000U && base220 < 0x0C000000U) {
                    // 088639B8: r2=idx<<4; r3=r2-idx (=idx*15); r2=r3<<2; r2=r2-r3
                    // (=idx*45); r3=r2<<3 (=idx*360). desc = *(obj+220)+r3.
                    int32_t r3 = (idx << 4) - idx;       // idx*15
                    int32_t r2v = (r3 << 2) - r3;        // idx*45
                    int32_t off = (r2v << 3);            // idx*360
                    uint32_t descp = base220 + (uint32_t)off;
                    if (descp >= 0x08000000U && descp < 0x0C000000U)
                        gate48 = (int32_t)psp_mem_read<uint32_t>(rdram, descp + 48);
                }
                s_980_orig(rdram, ctx);
                if (n980 <= 8 || (n980 % 40) == 0) {
                    std::fprintf(stderr,
                        "[O980] #%d a0=0x%08X *(a0+61)=%u idx=%d *(desc+48)=%d ret=%d\n",
                        n980, a0, f61, idx, gate48, (int32_t)ctx->r[2]);
                }
            });
        std::fprintf(stderr, "[RT] O980_TRACE wrapper installed for FUN_08863980\n");
    }

    // 4c. Wrapper for FUN_0896A6A4 (asset lookup called by gatekeeper)
    //
    // Phase 11-05: wrapper now routes through asset_bnd.cpp's BND parser.
    // Control flow (per 11-RESEARCH.md §7 "New wrapper control flow"):
    //   1. PSPRECOMP_BND_DISABLE=1 → fallback_to_shared_stub (A/B aid).
    //   2. PSPRECOMP_ASSET_NO_STUB=1 → chain to recompiled original.
    //   3. std::call_once gates bnd_init (D-05 lazy-init, GE_TEST_ONLY-safe).
    //   4. g_bnd_unavailable → fallback_to_shared_stub (D-08).
    //   5. bucket_count != 0 → chain to recompiled original (game's own
    //      hashtable is populated; we trust the original).
    //   6. BND parser resolve → desc_addr.
    //   7. desc_addr == 0 → fallback_to_shared_stub.
    //   8. PSPRECOMP_ASSET_FORCE_MISS=1 → override desc to 0.
    //   9. Set ctx->r[2] = desc_addr, rate-limited [BND_TRACE] log.
    //
    // Outer envelope (orig_al capture + g_asset_lookup_orig set + install)
    // unchanged in shape; g_asset_lookup_orig wiring is new for Plan 11-05
    // so fallback_to_shared_stub can honor PSPRECOMP_ASSET_NO_STUB.
    //
    // REMOVAL CRITERION (behavioral — replaces the game's registry lookup
    // with the host BND parser): delete when the game's own asset hash
    // table populates at boot (constructor walk fixed — see
    // PSPRECOMP_RUN_CTORS in patapon_on_boot_context) and the native lookup
    // resolves all loadinggroup assets (CLEANROOM real_nonsprite band held
    // with PSPRECOMP_BND_DISABLE=1).
    {
        FuncPtr orig_al = RECOMP_LOOKUP(0x0896A6A4);
        static FuncPtr s_al_orig = nullptr;
        s_al_orig = orig_al;
        g_asset_lookup_orig = orig_al;
        psp_dispatch_register(0x0896A6A4,
            [](uint8_t* rdram, recomp_context* ctx) {
                uint32_t a0 = static_cast<uint32_t>(ctx->r[4]);
                uint32_t a1 = static_cast<uint32_t>(ctx->r[5]);

                // WORKAROUND: If idx looks like a truncated .text address
                // (< 0x08000000 but within plausible range), add module base
                if (a1 != 0 && a1 < 0x00800000U && a1 >= 0x00004000U) {
                    uint32_t fixed = a1 + 0x08800000U;
                    static int fix_count = 0;
                    fix_count++;
                    if (fix_count <= 5) {
                        std::fprintf(stderr,
                            "[ASSET_FIX] idx 0x%08X -> 0x%08X (added module base)\n",
                            a1, fixed);
                    }
                    ctx->r[5] = static_cast<int32_t>(fixed);
                }

                // Dump bucket count before call (at base + 0x40000 + 25124)
                uint32_t bucket_count = 0;
                if ((a0 & PSP_ADDR_MASK) + 0x46224U + 4U <= PSP_MEM_SIZE) {
                    bucket_count = MEM_W(rdram, a0 + 0x40000U + 25124U);
                }
                // Read the string at idx (key for the lookup)
                char idx_str[64] = "<invalid>";
                if (a1 >= 0x08000000U) {
                    uint32_t off = a1 & PSP_ADDR_MASK;
                    if (off + 63U < PSP_MEM_SIZE) {
                        std::strncpy(idx_str,
                            reinterpret_cast<const char*>(rdram + off),
                            63);
                        idx_str[63] = '\0';
                    }
                }

                // Phase 11-05 control flow.
                // (1) PSPRECOMP_BND_DISABLE=1 → revert to the lifted shared
                //     stub path. A/B comparison aid; bypasses BND entirely.
                if (std::getenv("PSPRECOMP_BND_DISABLE")) {
                    fallback_to_shared_stub(rdram, ctx, idx_str, bucket_count);
                    return;
                }

                // (2) PSPRECOMP_ASSET_NO_STUB=1 → chain straight to the
                //     recompiled original; no synthetic descriptor.
                if (std::getenv("PSPRECOMP_ASSET_NO_STUB")) {
                    s_al_orig(rdram, ctx);
                    return;
                }

                // (3) Lazy init the BND parser exactly once. Inside the
                //     wrapper so PSPRECOMP_GE_TEST_ONLY=1 (game thread never
                //     starts) → wrapper never fires → bnd_init never runs
                //     → D-06 structural invariant preserved.
                static std::once_flag g_bnd_once;
                std::call_once(g_bnd_once, [rdram]() { bnd_init(rdram); });

                // [V438FIX] The game's asset name→entry hash table at
                // managerObj+0x40000+0x6220 is empty (bucket_count=0) because the
                // game CRT never reaches the asset-table constructor FUN_08A21CD4
                // (it IS in init_array.cpp but the CRT's .init_array walk stops
                // before it). Run it once here, with a fresh stack via a ctx copy
                // (it takes no args — uses constants — so propagating caller regs
                // is safe; it just needs a valid r29/r28). If it populates the
                // table, the native lookup below resolves and assigns real
                // handlers → assets stream → geometry.
                // Re-read bucket_count: the global constructors (run before
                // module_start, block 9b) may have populated the table.
                bucket_count = MEM_W(rdram, a0 + 0x40000U + 25124U);

                // [DIAG07] One-shot: is the lookup's manager object the static
                // one FUN_08A21CD4 populates (0x089A0000)? Compares the table
                // FUN_08A21CD4 wrote (absolute 0x089E6224) vs the wrapper's
                // a0-relative read. Pinpoints "wrong object" vs "zeroed".
                if (std::getenv("PSPRECOMP_DIAG07")) {
                    static int diag07 = 0;
                    if (diag07++ < 3) {
                        uint32_t abs_bc =
                            MEM_W(rdram, 0x089E6224U & PSP_ADDR_MASK);
                        uint32_t abs_arr =
                            MEM_W(rdram, 0x089E6220U & PSP_ADDR_MASK);
                        std::fprintf(stderr,
                            "[DIAG07] a0=0x%08X a0_rel_bucket=%u | "
                            "abs@0x089E6224 bucket=%u arr=0x%08X "
                            "(a0==0x089A0000? %d)\n",
                            a0, bucket_count, abs_bc, abs_arr,
                            (a0 == 0x089A0000U));
                    }
                }

                // (4) Missing/malformed DATA_CMN.BND falls back to the
                //     pre-Phase-11 shared stub (D-08).
                if (g_bnd_unavailable.load()) {
                    fallback_to_shared_stub(rdram, ctx, idx_str, bucket_count);
                    return;
                }

                // (5) If the game's own hashtable is populated, defer to the
                //     recompiled original — it knows how to resolve the
                //     entry from its own state. This is the post-Issue-#1
                //     path; for the current build it is rare to hit here.
                if (bucket_count != 0) {
                    s_al_orig(rdram, ctx);
                    return;
                }

                // (6) Resolve via the BND parser.
                uint32_t desc_addr = bnd_resolve_and_allocate(rdram, idx_str);

                // (7) Path not in outer index → fall back to shared stub.
                if (desc_addr == 0) {
                    fallback_to_shared_stub(rdram, ctx, idx_str, bucket_count);
                    return;
                }

                // (8) FORCE_MISS diagnostic override applies post-resolve.
                if (std::getenv("PSPRECOMP_ASSET_FORCE_MISS")) {
                    desc_addr = 0;
                }

                // (8b) Phase 11.3 H2-b-wrap: the Phase 11 BND parser writes a
                //      layout {vtable_ptr, size, payload_addr, 0} at desc_addr,
                //      but the gatekeeper FUN_088623E0 reads
                //      {uint16 id, int16 idx, uint32 max, uint32 offset}
                //      per batch_0052.cpp:481-700 (RESEARCH §2.3, DIAGNOSTIC §1).
                //      Translate in-place BEFORE the descriptor is returned to
                //      the gatekeeper. Guard with [PSP_BND_ARENA_BASE,
                //      PSP_BND_ARENA_END) so the D-08 fallback descriptor at
                //      0x08002864 (outside arena) is never touched (R11.2-07).
                //      Speculative field values — cycle 1 of 3 per RESEARCH §7
                //      Pitfall 5 (P-UNAVAILABLE fallback from DIAGNOSTIC §4).
                //      Plan 04 verifies via [BND_LAYOUT_TRANSLATE] canary log
                //      + A3 ([DRAW_PRIM] clear=0) count.
                // 19B: the gatekeeper-style reinterpretation reports the
                // on-disk {size, offset} (compressed size + DATA_CMN.BND file
                // offset) for the descriptor. The real game's open-by-name
                // resolver (FUN_088623E0, PPSSPP-confirmed doc 20) returns the
                // COMPRESSED {off, size} for EVERY loadinggroup/*.bnd asset, so
                // the IO state machine reads the compressed stream and inflates
                // downstream — it never streams the inflated payload through the
                // IO slot. The descriptor's {max, offset} flow to the slot's
                // {s300, s308}; our sceIoOpenAsync reroute keys
                // detect_bnd_backed_async_slot off those exact fields to prepare
                // a BND-arena read buffer (slot+292), which lets the slot skip
                // the state-103 IO-pool alloc (the ~1 MB block the fixed IO pool
                // cannot serve -> FUN_0895B3AC returns sentinel 1 -> spin).
                //
                // Was gated to ONLY "loadinggroup/systemdata.bnd" (the V438 A/B
                // band-aid), which is why systemdata reached state=300 but
                // systemlocalizedata (untranslated -> s300=inflated 4307840 !=
                // expected compressed) failed detect, was never prepared, and
                // stalled at state-103. Generalize to ANY path that resolves to
                // a BND outer entry: every BND-backed asset must report its
                // on-disk compressed {size, offset} so the reroute can back it.
                bool gatekeeper_layout_needed =
                    (idx_str != nullptr
                     && bnd_find_outer_entry_for_virt_path(idx_str) != nullptr);
                if (gatekeeper_layout_needed
                        && desc_addr >= PSP_BND_ARENA_BASE
                        && desc_addr < PSP_BND_ARENA_END
                        && !std::getenv("PSPRECOMP_NO_DESC_XLATE")) {
                    const uint16_t desc_id  = 1u;   // small id_or_count
                    const int16_t  desc_idx = -1;   // skip table lookup
                    uint32_t desc_max =
                        psp_mem_read<uint32_t>(rdram, desc_addr + 4U);
                    uint32_t desc_offset =
                        psp_mem_read<uint32_t>(rdram, desc_addr + 8U);
                    if (const BndOuterEntry* outer =
                            bnd_find_outer_entry_for_virt_path(idx_str)) {
                        desc_max = outer->size;
                        desc_offset = outer->file_offset;
                    }
                    psp_mem_write<uint16_t>(rdram, desc_addr + 0U, desc_id);
                    psp_mem_write<int16_t> (rdram, desc_addr + 2U, desc_idx);
                    psp_mem_write<uint32_t>(rdram, desc_addr + 4U, desc_max);
                    psp_mem_write<uint32_t>(rdram, desc_addr + 8U, desc_offset);
                    psp_mem_write<uint32_t>(rdram, desc_addr + 12U, 1U);
                    static int desc_xlate_count = 0;
                    desc_xlate_count++;
                    if (desc_xlate_count <= 8 || desc_xlate_count % 500 == 0) {
                        std::fprintf(stderr,
                            "[BND_LAYOUT_TRANSLATE] desc=0x%08X f0=%u f2=%d f4=%u f8=%u (speculative cycle 1) (#%d)\n",
                            desc_addr, static_cast<unsigned>(desc_id),
                            static_cast<int>(desc_idx),
                            desc_max, desc_offset, desc_xlate_count);
                    }
                }

                // (9) Return the descriptor PSP-VA in v0 and rate-limit a
                //     [BND_TRACE] success line (first 8 + every 500th).
                ctx->r[2] = static_cast<int32_t>(desc_addr);
                static int bnd_count = 0;
                bnd_count++;
                if (bnd_count <= 8 || bnd_count % 500 == 0) {
                    std::fprintf(stderr,
                        "[BND_TRACE] resolved \"%s\" -> 0x%08X (#%d)\n",
                        idx_str, desc_addr, bnd_count);
                }
            });
        std::fprintf(stderr, "[RT] Asset lookup wrapper installed for FUN_0896A6A4\n");
    }

    // 4d. Wrapper for FUN_0885FBA8 (writes to this+180 = the asset index)
    // Catches when a corrupt value (< 0x08000000, non-zero) is stored
    {
        FuncPtr orig_wr = RECOMP_LOOKUP(0x0885FBA8);
        static FuncPtr s_wr_orig = nullptr;
        s_wr_orig = orig_wr;
        psp_dispatch_register(0x0885FBA8,
            [](uint8_t* rdram, recomp_context* ctx) {
                // r[8] = the value that will be stored at this+180
                uint32_t val = static_cast<uint32_t>(ctx->r[8]);
                s_wr_orig(rdram, ctx);
                if (val != 0 && val < 0x08000000U) {
                    static int wc = 0;
                    wc++;
                    if (wc <= 5) {
                        std::fprintf(stderr,
                            "[CORRUPT_WRITE] FUN_0885FBA8 storing 0x%08X at this+180 "
                            "(caller=0x%08X) #%d\n",
                            val, g_last_func_addr, wc);
                    }
                }
            });
    }

    // 4d2. [V438b] Probe FUN_0895b410 entry — capture object O (=r4) whose +8
    // sub-object pointer is NULL on the native asset path (Phase 12 root cause).
    {
        FuncPtr orig_b410 = RECOMP_LOOKUP(0x0895B410);
        static FuncPtr s_b410_orig = nullptr;
        s_b410_orig = orig_b410;
        psp_dispatch_register(0x0895B410,
            [](uint8_t* rdram, recomp_context* ctx) {
                uint32_t O = static_cast<uint32_t>(ctx->r[4]);
                // [POOL_FREE] — FUN_0895B410 is the pool free wrapper: it calls
                // pool_vtable[+12] (pool = *(O+8)). Args: a1..a4 = r5,r6,r7,r8.
                // a1 (r5) is the block pointer being returned. Count every free,
                // and flag frees of blocks that look like 512KB heap pointers.
                {
                    static int g_free_total = 0;
                    g_free_total++;
                    uint32_t fa1 = static_cast<uint32_t>(ctx->r[5]);
                    uint32_t fa2 = static_cast<uint32_t>(ctx->r[6]);
                    uint32_t fa3 = static_cast<uint32_t>(ctx->r[7]);
                    uint32_t fa4 = static_cast<uint32_t>(ctx->r[8]);
                    // pool of THIS free wrapper's object = *(O+8). Also resolve the
                    // IO 512KB pool (singleton 0x08A7B634) for cross-reference.
                    uint32_t poolf = (O >= 0x08000000U && O < 0x0A000000U)
                        ? psp_mem_read<uint32_t>(rdram, O + 8) : 0;
                    uint32_t io_pool = psp_mem_read<uint32_t>(rdram, 0x08A7B634U + 8);
                    bool io_obj = (O == 0x08A7B634U);
                    bool blk_512ish = (fa2 == 0x80000U || fa3 == 0x80000U
                        || fa1 == 0x09096FE0U);
                    std::fprintf(stderr,
                        "[POOL_FREE] #%d O=0x%08X obj_pool=0x%08X io_pool=0x%08X "
                        "a1(block)=0x%08X a2=0x%08X a3=0x%08X a4=0x%08X caller=0x%08X%s%s\n",
                        g_free_total, O, poolf, io_pool, fa1, fa2, fa3, fa4,
                        g_last_func_addr,
                        io_obj ? " [IO_OBJ]" : "",
                        blk_512ish ? " [512KB?]" : "");
                    std::fflush(stderr);
                }
                static int n = 0;
                if (++n <= 6) {
                    auto rd = [&](uint32_t off) -> uint32_t {
                        uint32_t b = (O & 0x07FFFFFFU) + off;
                        if (b + 4 > PSP_MEM_SIZE) return 0xBADBAD;
                        return *reinterpret_cast<uint32_t*>(rdram + b);
                    };
                    std::fprintf(stderr,
                        "[V438b] FUN_0895b410 #%d O=0x%08X caller=0x%08X "
                        "O+0=%08X O+4=%08X O+8=%08X O+12=%08X O+16=%08X\n",
                        n, O, g_last_func_addr,
                        rd(0), rd(4), rd(8), rd(12), rd(16));
                    // Follow the chain: subobj=*(O+8); subvt=*(subobj+0);
                    // method=*(subvt+12)  (the indirect-call target).
                    uint32_t subobj = rd(8);
                    auto rdat = [&](uint32_t addr, uint32_t off) -> uint32_t {
                        if (addr < 0x08000000U || addr >= 0x0A000000U) {
                            // allow the 0x0Axxxxxx heap too
                            if (!(addr >= 0x08000000U && addr < 0x0B000000U))
                                return 0xBADBAD;
                        }
                        uint32_t b = (addr & 0x07FFFFFFU) + off;
                        if (b + 4 > PSP_MEM_SIZE) return 0xBADBAD;
                        return *reinterpret_cast<uint32_t*>(rdram + b);
                    };
                    uint32_t subvt = rdat(subobj, 0);
                    std::fprintf(stderr,
                        "[V438b]   subobj=0x%08X *subobj+0(vt)=%08X vt+12(method)=%08X "
                        "subobj+4=%08X subobj+8=%08X\n",
                        subobj, subvt, rdat(subvt, 12),
                        rdat(subobj, 4), rdat(subobj, 8));
                }
                s_b410_orig(rdram, ctx);
            });
        std::fprintf(stderr, "[RT] V438b probe installed for FUN_0895B410\n");
    }

    // 4d3. [V438c] Probe FUN_0885e020 (virtual-dispatch trampoline) — find the
    // object whose vtable slot +12 is garbage (= the 0x438 source, Phase 12).
    {
        FuncPtr orig_e020 = RECOMP_LOOKUP(0x0885E020);
        static FuncPtr s_e020_orig = nullptr;
        s_e020_orig = orig_e020;
        psp_dispatch_register(0x0885E020,
            [](uint8_t* rdram, recomp_context* ctx) {
                uint32_t obj = static_cast<uint32_t>(ctx->r[4]);
                if (obj >= 0x08000000U && obj < 0x0B000000U) {
                    uint32_t vb = obj & 0x07FFFFFFU;
                    uint32_t vt = (vb + 4 <= PSP_MEM_SIZE)
                        ? *reinterpret_cast<uint32_t*>(rdram + vb) : 0;
                    uint32_t method = 0;
                    if (vt >= 0x08000000U && vt < 0x0B000000U) {
                        uint32_t mb = (vt & 0x07FFFFFFU) + 12;
                        if (mb + 4 <= PSP_MEM_SIZE)
                            method = *reinterpret_cast<uint32_t*>(rdram + mb);
                    }
                    // Flag the garbage case: method not a valid code addr.
                    bool bad = !(method >= 0x08000000U && method < 0x0A000000U);
                    static int good = 0, badc = 0;
                    if (bad && ++badc <= 10) {
                        std::fprintf(stderr,
                            "[V438c] BAD obj=0x%08X vt=0x%08X *(vt+12)=0x%08X "
                            "caller=0x%08X (#%d)\n",
                            obj, vt, method, g_last_func_addr, badc);
                    } else if (!bad && ++good <= 4) {
                        std::fprintf(stderr,
                            "[V438c] ok  obj=0x%08X vt=0x%08X *(vt+12)=0x%08X\n",
                            obj, vt, method);
                    }
                }
                s_e020_orig(rdram, ctx);
            });
        std::fprintf(stderr, "[RT] V438c probe installed for FUN_0885E020\n");
    }

    // 4e. Wrapper for FUN_08862C14 (IO async state machine)
    // Fixes state=1 routing bug: when slot+312=0 AND slot+316=0, the state
    // machine incorrectly writes slot_index (e.g. 2) to slot+0 as the "state",
    // corrupting the state machine. On real PSP, slot+316 is set to 1 (or 2)
    // before state=1 is processed, so this path is never taken.
    //
    // Root cause: FUN_0886455C (job setup) writes slot+316 after FUN_08863B6C
    // enqueues the slot. If FileThread picks up slot 2 (fd=5) between enqueueing
    // and slot+316 being written, state=1 sees slot+316=0 and falls into the bug.
    //
    // Fix: before calling the original function, if state=1 and slot+316=0,
    // pre-set slot+316=1 so the correct L_08863040 branch is taken.
    //
    // REMOVAL CRITERION (behavioral — [BND_SLOT_SHORT] short-circuit +
    // [SM_FIX] state reset + [SM_DRIVE] forced re-calls): delete when the
    // IO slot state machine runs the real path end-to-end, i.e. the
    // CLEANROOM gate (PSPRECOMP_CLEANROOM=1 forces this wrapper to pure
    // pass-through) holds the real_nonsprite band with zero
    // [BND_SLOT_SHORT_TOTAL] firings.
    {
        FuncPtr orig_sm = RECOMP_LOOKUP(0x08862C14);
        static FuncPtr s_sm_orig = nullptr;
        s_sm_orig = orig_sm;
        psp_dispatch_register(0x08862C14,
            [](uint8_t* rdram, recomp_context* ctx) {
                // [CLEANROOM] Agent F: gate the entire FUN_08862C14 SM wrapper
                // (the [SM_FIX] state-correction, the [SM_DRIVE] forced re-call
                // loop, the slot_index<16 gate, the r5-direct slot decode) to a
                // pure passthrough so the per-slot state machine runs exactly as
                // the recompiled game code drives it. Diagnostic, not a band-aid:
                // default behavior is unchanged. NOTE: BND_SLOT_SHORT lives below
                // and has its own PSPRECOMP_BND_SHORT_OFF gate; CLEANROOM implies
                // SM-wrapper off but does NOT force BND_SHORT off (keep that
                // toggle independent so we can A/B them).
                static const bool s_cleanroom =
                    std::getenv("PSPRECOMP_CLEANROOM") != nullptr;
                if (s_cleanroom) {
                    // [CR_SM] diagnostic: log entry/exit state + slot+8 for the
                    // first calls so we can see whether the REAL SM drives slots
                    // through the 0xcc/0x1a5 terminal (which sets slot+8=6).
                    static int cr_sm = 0;
                    uint32_t cont = static_cast<uint32_t>(ctx->r[4]);
                    uint32_t arr = (cont >= 0x08000000U && cont < 0x0A000000U)
                        ? psp_mem_read<uint32_t>(rdram, cont + 220) : 0;
                    uint32_t idx = static_cast<uint32_t>(ctx->r[5]);
                    uint32_t item = (arr >= 0x08000000U && idx < 64U)
                        ? arr + idx * 360U : 0;
                    uint32_t st0 = item ? psp_mem_read<uint32_t>(rdram, item + 0) : 0;
                    uint32_t s80 = item ? psp_mem_read<uint32_t>(rdram, item + 8) : 0;
                    // [GATE_PROBE] Read the three inputs that decide the
                    // 0x1a5 (auto-ack, FileThread) vs 0xcc (consumer) terminal
                    // in FUN_08862C14:  slot+0xc (piVar7[3]), slot+0x13c
                    // (piVar7[0x4f]), and the entry state (*piVar7).  We mirror
                    // the decompiled slot decode (param_1[0x37] + slot*0x168)
                    // to get the SAME slot the SM operates on, not the r5 guess.
                    static const bool s_gate_probe =
                        std::getenv("PSPRECOMP_GATE_PROBE") != nullptr;
                    if (s_gate_probe && cont >= 0x08000000U
                            && cont < 0x0A000000U) {
                        // slot index stored at mgr + 0x126d2 (word index).
                        uint32_t slot_idx = psp_mem_read<uint32_t>(
                            rdram, cont + 0x126d2U * 4U);
                        uint32_t slot_base = psp_mem_read<uint32_t>(
                            rdram, cont + 0x37U * 4U);
                        if (slot_base >= 0x08000000U && slot_base < 0x0A000000U
                                && slot_idx < 64U) {
                            uint32_t slot = slot_base + slot_idx * 0x168U;
                            uint32_t st   = psp_mem_read<uint32_t>(rdram, slot + 0);
                            uint32_t f0xc = psp_mem_read<uint32_t>(rdram, slot + 0xc);
                            uint32_t f13c = psp_mem_read<uint32_t>(rdram, slot + 0x13c);
                            uint32_t s8   = psp_mem_read<uint32_t>(rdram, slot + 8);
                            uint32_t mgr50 = psp_mem_read<uint32_t>(rdram, cont + 0x50);
                            uint32_t pathp = psp_mem_read<uint32_t>(rdram, slot + 292);
                            std::fprintf(stderr,
                                "[GATE_PROBE] slot_idx=%u slot=0x%08X state=%u "
                                "slot+0xc=%u slot+0x13c=%u slot+8=%u mgr+0x50=%u "
                                "path=0x%08X\n",
                                slot_idx, slot, st, f0xc, f13c, s8, mgr50, pathp);
                        }
                    }
                    s_sm_orig(rdram, ctx);  // pure passthrough to the real SM
                    if (item && ++cr_sm <= 40) {
                        uint32_t st1 = psp_mem_read<uint32_t>(rdram, item + 0);
                        uint32_t s81 = psp_mem_read<uint32_t>(rdram, item + 8);
                        std::fprintf(stderr,
                            "[CR_SM] #%d idx=%u item=0x%08X state %u->%u s8 %u->%u\n",
                            cr_sm, idx, item, st0, st1, s80, s81);
                    }
                    return;
                }
                static int sm_count = 0;
                sm_count++;
                uint32_t container = static_cast<uint32_t>(ctx->r[4]);
                if (container >= 0x08000000U && container < 0x0A000000U) {
                    // Decode slot_item via FUN_08862C14's actual entry logic:
                    //   node_ptr   = *(container + 1792)
                    //   if node_ptr == container+1792: slot_index = r[23] = r[5] = second arg
                    //   else: slot_index = *(node_ptr + 8)
                    //   slot_array = *(container + 220)
                    //   slot_item  = slot_array + slot_index * 360
                    uint32_t node_ptr  = psp_mem_read<uint32_t>(rdram, container + 1792);
                    uint32_t slot_array = psp_mem_read<uint32_t>(rdram, container + 220);
                    uint32_t slot_index;
                    uint32_t second_arg = static_cast<uint32_t>(ctx->r[5]);
                    if (node_ptr == container + 1792) {
                        // Special "self-pointer" case: use r[5] directly
                        slot_index = second_arg;
                    } else if (node_ptr >= 0x08000000U && node_ptr < 0x0A000000U) {
                        slot_index = psp_mem_read<uint32_t>(rdram, node_ptr + 8);
                    } else {
                        // node_ptr is small (e.g. 0) — same logic as MIPS: *(node_ptr+8)
                        // In rdram, small addresses map to near-zero which is 0
                        slot_index = 0;
                    }
                    if (slot_array >= 0x08000000U && slot_array < 0x0A000000U
                            && slot_index < 16U) {
                        uint32_t slot_item = slot_array + slot_index * 360U;
                        uint32_t state     = psp_mem_read<uint32_t>(rdram, slot_item + 0);
                        uint32_t s292      = psp_mem_read<uint32_t>(rdram, slot_item + 292);
                        uint32_t s300      = psp_mem_read<uint32_t>(rdram, slot_item + 300);
                        uint32_t s308      = psp_mem_read<uint32_t>(rdram, slot_item + 308);
                        uint32_t s312      = psp_mem_read<uint32_t>(rdram, slot_item + 312);
                        uint32_t s316      = psp_mem_read<uint32_t>(rdram, slot_item + 316);
                        if (sm_count <= 30) {
                            std::fprintf(stderr,
                                "[SM_DIAG] FUN_08862C14 #%d container=0x%08X "
                                "node=0x%08X r5=%u slot_idx=%u item=0x%08X "
                                "state=%u s292=%u s300=%u s312=%u s316=%u\n",
                                sm_count, container, node_ptr, second_arg,
                                slot_index, slot_item, state, s292, s300, s312, s316);
                        }
                        // [BND_SLOT_SHORT] short-circuit (Phase 11.1):
                        // When the IO state machine is asked to process a slot whose
                        // path pointer is NULL (slot+292 == 0) AND whose pre-resolved
                        // descriptor (slot+308) lives in the BND parser arena, there
                        // is no compiled state-machine branch that can consume it
                        // (the constructor that would have wired the path string is
                        // missing — Phase 11 deferred Issue #1). Calling s_sm_orig
                        // would re-enter sceIoOpenAsync(NULL) and spin the slot[2]
                        // retry loop (1,235x in 12s pre-fix, leaking PSP fds and
                        // hammering [SEMA271]). Short-circuit to the terminal
                        // state (300, 0, 0) — the same triple healthy slots 0/1
                        // settle at — and skip s_sm_orig entirely. The AND-
                        // conjunction is tight: slot[0]/[1] have non-NULL s292 and
                        // BND_DISABLE descriptors (0x08002864) live outside the
                        // arena range, so neither path can over-match.
                        // Precedent: [SM_FIX] post-call write at main.cpp:649-655
                        // uses the same psp_mem_write<uint32_t>(slot_item + N)
                        // convention; [BND_TRACE] at main.cpp:540-546 sets the
                        // rate-limit gate this branch mirrors.
                        if (s292 == 0U
                                && s308 >= PSP_BND_ARENA_BASE
                                && s308 <  PSP_BND_ARENA_END
                                && std::getenv("PSPRECOMP_BND_SHORT_OFF")
                                       == nullptr) {
                            // Lift counter to file-scope (g_bnd_short_total) so
                            // the [BND_SLOT_SHORT_TOTAL] atexit handler can read
                            // the true lifetime total even when the rate-limit
                            // gate hides intermediate firings (Phase 11.1
                            // finding — see Pattern D in 11.2-PATTERNS.md).
                            g_bnd_short_total++;
                            if (!g_bnd_short_atexit_registered) {
                                g_bnd_short_atexit_registered = true;
                                std::atexit(dump_bnd_short_total);
                            }
                            if (g_bnd_short_total <= 8
                                    || g_bnd_short_total % 500 == 0) {
                                std::fprintf(stderr,
                                    "[BND_SLOT_SHORT] slot_idx=%u item=0x%08X "
                                    "descriptor=0x%08X size=%u (#%d)\n",
                                    slot_index, slot_item, s308, s300,
                                    g_bnd_short_total);
                            }
                            psp_mem_write<uint32_t>(rdram, slot_item + 0,  300U);
                            psp_mem_write<uint32_t>(rdram, slot_item + 8,    0U);
                            psp_mem_write<uint32_t>(rdram, slot_item + 12,   0U);
                            return;
                        }
                        // Save sm_count before orig (re-entrant calls increment it)
                        int this_sm = sm_count;
                        s_sm_orig(rdram, ctx);
                        // POST-CALL FIX: detect the state=1 routing bug.
                        uint32_t new_state = psp_mem_read<uint32_t>(rdram, slot_item + 0);
                        uint32_t ns292 = psp_mem_read<uint32_t>(rdram, slot_item + 292);
                        uint32_t ns312 = psp_mem_read<uint32_t>(rdram, slot_item + 312);
                        uint32_t ns316 = psp_mem_read<uint32_t>(rdram, slot_item + 316);
                        // Always log post-call state for the first 6 calls
                        if (this_sm <= 6) {
                            std::fprintf(stderr,
                                "[SM_POST] #%d slot_idx=%u entry_state=%u -> "
                                "new_state=%u ns292=%u ns312=%u ns316=%u\n",
                                this_sm, slot_index, state, new_state,
                                ns292, ns312, ns316);
                        }
                        if (new_state == slot_index && slot_index < 10U) {
                            psp_mem_write<uint32_t>(rdram, slot_item + 0, 110U);
                            std::fprintf(stderr,
                                "[SM_FIX] slot_idx=%u slot_item=0x%08X "
                                "entry_state=%u corrupted to %u (=slot_idx), reset to 110\n",
                                slot_index, slot_item, state, new_state);
                        }
                        // Drive states that need a follow-up call:
                        // State=103 (buffer alloc needed) and state=110 (lseek setup)
                        // are set when there is no async op in flight — the state machine
                        // is not in the queue and won't be driven again via the callback.
                        // Call FUN_08862C14 again immediately to process them.
                        uint32_t drive_state = psp_mem_read<uint32_t>(rdram, slot_item + 0);
                        static int drive_count = 0;
                        while ((drive_state == 103U || drive_state == 110U)
                                && drive_count < 5) {
                            drive_count++;
                            uint32_t pre_s292 = psp_mem_read<uint32_t>(rdram, slot_item + 292);
                            uint32_t pre_s12  = psp_mem_read<uint32_t>(rdram, slot_item + 12);
                            uint32_t vptr = psp_mem_read<uint32_t>(rdram, container + 0);
                            // Trace into FUN_089b3d98: alloc_obj = *(0x08A7B634)
                            uint32_t alloc_vtbl = psp_mem_read<uint32_t>(rdram, 0x08A7B634U); // vtable ptr
                            uint32_t alloc_fn = (alloc_vtbl >= 0x08000000U && alloc_vtbl < 0x0A000000U)
                                ? psp_mem_read<uint32_t>(rdram, alloc_vtbl + 8) : 0;
                            uint32_t pool_ptr = psp_mem_read<uint32_t>(rdram, 0x08A7B63CU); // *(alloc_singleton+8)
                            uint32_t cont0 = psp_mem_read<uint32_t>(rdram, container + 0);
                            uint32_t s300 = psp_mem_read<uint32_t>(rdram, slot_item + 300);
                            uint32_t s304 = psp_mem_read<uint32_t>(rdram, slot_item + 304);
                            uint32_t s308 = psp_mem_read<uint32_t>(rdram, slot_item + 308);
                            std::fprintf(stderr,
                                "[SM_DRIVE] #%d slot_idx=%u driving state=%u "
                                "pre_s292=%u pre_s12=%u vptr=0x%08X "
                                "alloc_vtbl=0x%08X alloc_fn=0x%08X "
                                "pool_ptr=0x%08X cont0=0x%08X "
                                "s300=%u s304=%u s308=%u\n",
                                drive_count, slot_index, drive_state,
                                pre_s292, pre_s12, vptr,
                                alloc_vtbl, alloc_fn,
                                pool_ptr, cont0,
                                s300, s304, s308);
                            // [SM_Q] work-queue / enqueue state when a slot is
                            // stuck pre-open (state 103/110). Tests hypothesis
                            // (A): was this slot ever enqueued? FUN_08862C14
                            // re-derives WHICH slot to process from the manager's
                            // node queue (node=*(container+1792), idx=*(node+8)),
                            // NOT from its argument. If the queue head
                            // (container+0x710) is -1/empty or node_idx != this
                            // slot, the natural dispatch can never advance THIS
                            // slot's state-103 — explaining the spin. mgr80 is
                            // the FUN_08862A10 top gate ([mgr+80]==1 required);
                            // pathfn is the state-103 path-builder fn ptr.
                            {
                                uint32_t q_head = psp_mem_read<uint32_t>(rdram, container + 0x710);
                                uint32_t q_tail = psp_mem_read<uint32_t>(rdram, container + 0x718);
                                uint32_t q_work = psp_mem_read<uint32_t>(rdram, container + 0x720);
                                uint32_t nodep  = psp_mem_read<uint32_t>(rdram, container + 1792);
                                uint32_t nidx   = (nodep >= 0x08000000U && nodep < 0x0A000000U)
                                    ? psp_mem_read<uint32_t>(rdram, nodep + 8) : 0xFFFFFFFFU;
                                uint32_t mgr80  = psp_mem_read<uint32_t>(rdram, container + 80);
                                uint32_t pathfn = psp_mem_read<uint32_t>(rdram, container + 0);
                                uint32_t s316   = psp_mem_read<uint32_t>(rdram, slot_item + 316);
                                std::fprintf(stderr,
                                    "[SM_Q] slot_idx=%u q_head=0x%08X q_tail=0x%08X "
                                    "q_work=%u nodeptr=0x%08X node_idx=%u s316=%u "
                                    "mgr80=%u pathfn=0x%08X\n",
                                    slot_index, q_head, q_tail, q_work, nodep, nidx,
                                    s316, mgr80, pathfn);
                            }
                            // Re-use the same container/slot args
                            ctx->r[4] = static_cast<int32_t>(container);
                            ctx->r[5] = static_cast<int32_t>(slot_index);
                            s_sm_orig(rdram, ctx);
                            drive_state = psp_mem_read<uint32_t>(rdram, slot_item + 0);
                            uint32_t post_s292 = psp_mem_read<uint32_t>(rdram, slot_item + 292);
                            uint32_t post_s12  = psp_mem_read<uint32_t>(rdram, slot_item + 12);
                            uint32_t post_s304 = psp_mem_read<uint32_t>(rdram, slot_item + 304);
                            uint32_t post_s308 = psp_mem_read<uint32_t>(rdram, slot_item + 308);
                            uint32_t post_s324 = psp_mem_read<uint32_t>(rdram, slot_item + 324);
                            std::fprintf(stderr,
                                "[SM_DRIVE_POST] #%d -> new_state=%u "
                                "post_s292=%u post_s12=%u "
                                "s304=%u s308=%u s324=%u\n",
                                drive_count, drive_state, post_s292, post_s12,
                                post_s304, post_s308, post_s324);
                        }
                        return;
                    }
                }
                s_sm_orig(rdram, ctx);
            });
        std::fprintf(stderr, "[RT] IO state machine wrapper installed for FUN_08862C14\n");
    }

    // 4e1b. Path-builder probe — FUN_089B3D98 = [container+0], called by the
    // state-103 handler (batch_0052.cpp:2323) to build slot+292 (the asset path
    // string). Decisive test for the stuck-at-103 bug: if ENTER fires but RETURN
    // never does, the path-builder blocks/yields and never lets the handler set
    // state=110. If both fire with v0=0, it returns null. If neither fires, the
    // handler never reaches the call. Pure diagnostic.
    {
        FuncPtr orig_pb = RECOMP_LOOKUP(0x089B3D98);
        static FuncPtr s_pb = orig_pb;
        psp_dispatch_register(0x089B3D98,
            [](uint8_t* rdram, recomp_context* ctx) {
                static int pb_n = 0;
                pb_n++;
                uint32_t a0 = static_cast<uint32_t>(ctx->r[4]);
                uint32_t a1 = static_cast<uint32_t>(ctx->r[5]);
                uint32_t ra = static_cast<uint32_t>(ctx->r[31]);
                if (pb_n <= 20) {
                    std::fprintf(stderr,
                        "[PATHFN] #%d ENTER a0=0x%08X a1=0x%08X ra=0x%08X\n",
                        pb_n, a0, a1, ra);
                    std::fflush(stderr);
                }
                int this_n = pb_n;
                s_pb(rdram, ctx);
                if (this_n <= 20) {
                    std::fprintf(stderr,
                        "[PATHFN] #%d RETURN v0=0x%08X\n",
                        this_n, static_cast<uint32_t>(ctx->r[2]));
                    std::fflush(stderr);
                }
            });
        std::fprintf(stderr, "[RT] Path-builder probe installed for FUN_089B3D98\n");
    }

    // 4e2. Wrapper for FUN_0895B3AC (IO read-buffer allocator, called by FUN_089b3d98)
    // Logs what allocator pool ptr is at *(0x08A7B63C) and what function it dispatches.
    // Called with: a0=0x08A7B634 (alloc obj ptr), a1=file_size, a2=16 (alignment).
    // Key read: r[4] = *(a0+8) = *(0x08A7B63C) — should be pool obj ptr.
    {
        FuncPtr orig_alloc = RECOMP_LOOKUP(0x0895B3AC);
        static FuncPtr s_alloc_orig = nullptr;
        s_alloc_orig = orig_alloc;
        psp_dispatch_register(0x0895B3ACU,
            [](uint8_t* rdram, recomp_context* ctx) {
                static int g_alloc_total = 0;
                g_alloc_total++;
                uint32_t a0 = static_cast<uint32_t>(ctx->r[4]);
                uint32_t a1 = static_cast<uint32_t>(ctx->r[5]);
                uint32_t a2 = static_cast<uint32_t>(ctx->r[6]);
                uint32_t pool_ptr = 0, pool_vtbl = 0, alloc_fn = 0;
                if (a0 >= 0x08000000U && a0 < 0x0A000000U) {
                    pool_ptr  = psp_mem_read<uint32_t>(rdram, a0 + 8);
                    if (pool_ptr >= 0x08000000U && pool_ptr < 0x0A000000U) {
                        pool_vtbl = psp_mem_read<uint32_t>(rdram, pool_ptr + 0);
                        if (pool_vtbl >= 0x08000000U && pool_vtbl < 0x0A000000U)
                            alloc_fn = psp_mem_read<uint32_t>(rdram, pool_vtbl + 8);
                    }
                }
                // [VT_DUMP] one-shot: print the IO pool's full vtable so we can
                // identify the FREE function (vtable+12) vs alloc (vtable+8).
                {
                    static bool dumped = false;
                    if (!dumped && pool_vtbl >= 0x08000000U
                            && pool_vtbl < 0x0A000000U) {
                        dumped = true;
                        std::fprintf(stderr,
                            "[VT_DUMP] pool=0x%08X vtbl=0x%08X "
                            "+0=%08X +4=%08X +8=%08X +12=%08X +16=%08X +20=%08X\n",
                            pool_ptr, pool_vtbl,
                            psp_mem_read<uint32_t>(rdram, pool_vtbl + 0),
                            psp_mem_read<uint32_t>(rdram, pool_vtbl + 4),
                            psp_mem_read<uint32_t>(rdram, pool_vtbl + 8),
                            psp_mem_read<uint32_t>(rdram, pool_vtbl + 12),
                            psp_mem_read<uint32_t>(rdram, pool_vtbl + 16),
                            psp_mem_read<uint32_t>(rdram, pool_vtbl + 20));
                        std::fflush(stderr);
                    }
                }
                static int alloc_count = 0;
                alloc_count++;
                // Always log calls with a0=0x08A7B634 (the global IO alloc obj)
                // and always log when pool_ptr is out-of-range (broken cases).
                bool log_this = (alloc_count <= 10)
                    || (a0 == 0x08A7B634U)
                    || (pool_ptr < 0x08000000U || pool_ptr >= 0x0A000000U);
                if (log_this) {
                    std::fprintf(stderr,
                        "[ALLOC_DIAG] FUN_0895B3AC #%d a0=0x%08X a1=%u a2=%u "
                        "pool_ptr=0x%08X pool_vtbl=0x%08X alloc_fn=0x%08X\n",
                        alloc_count, a0, a1, a2,
                        pool_ptr, pool_vtbl, alloc_fn);
                    // Also show full object area
                    if (a0 >= 0x08000000U && a0 < 0x0A000000U) {
                        for (int i = 0; i < 4; i++) {
                            std::fprintf(stderr,
                                "[ALLOC_DIAG]   *(0x%08X+%d) = 0x%08X\n",
                                a0, i*4, psp_mem_read<uint32_t>(rdram, a0 + i*4));
                        }
                    }
                }
                // [POOL_ALLOC] — resolve the pool via the FIXED singleton data
                // address 0x08A7B634: singleton = *(0x08A7B634); pool = *(singleton+8).
                // Dump pool header pool+0..pool+0x100 BEFORE calling orig. Tag the
                // 512KB (0x80000) requests specially so we can count distinct blocks.
                static int g_pa_512 = 0;          // count of 512KB alloc calls
                bool is_512 = (a1 == 0x80000U);
                // The singleton OBJECT *is* the fixed data address 0x08A7B634
                // (its +0 is the vtable 0x08A47610, its +8 is the pool pointer).
                uint32_t sgl = 0x08A7B634U;
                uint32_t pool2 = psp_mem_read<uint32_t>(rdram, sgl + 8);
                bool pool_ok = (pool2 >= 0x08000000U && pool2 < 0x0A000000U);
                bool dump_pool = is_512 || (a0 == 0x08A7B634U && alloc_count <= 6);
                if (dump_pool) {
                    if (is_512) g_pa_512++;
                    std::fprintf(stderr,
                        "[POOL_ALLOC] #%d call512=%d size=%u(0x%X) align=%u "
                        "singleton=0x%08X pool=0x%08X%s\n",
                        alloc_count, g_pa_512, a1, a1, a2, sgl, pool2,
                        pool_ok ? "" : " [POOL_BAD]");
                    if (pool_ok) {
                        for (int w = 0; w <= 0x100/4; w += 4) {
                            std::fprintf(stderr,
                                "[POOL_ALLOC]   PRE  pool+0x%02X: %08X %08X %08X %08X\n",
                                w*4,
                                psp_mem_read<uint32_t>(rdram, pool2 + (w+0)*4),
                                psp_mem_read<uint32_t>(rdram, pool2 + (w+1)*4),
                                psp_mem_read<uint32_t>(rdram, pool2 + (w+2)*4),
                                psp_mem_read<uint32_t>(rdram, pool2 + (w+3)*4));
                        }
                    }
                    std::fflush(stderr);
                }
                s_alloc_orig(rdram, ctx);
                uint32_t retval = static_cast<uint32_t>(ctx->r[2]);
                if (dump_pool) {
                    bool sentinel = (retval == 1U);
                    bool heap = (retval >= 0x08000000U && retval < 0x0A000000U);
                    std::fprintf(stderr,
                        "[POOL_ALLOC] -> v0=0x%08X size=%u(0x%X)%s%s\n",
                        retval, a1, a1,
                        sentinel ? " [SENTINEL_1]" : (heap ? " [HEAP_PTR]" : ""),
                        is_512 ? " (512KB)" : "");
                    if (pool_ok) {
                        for (int w = 0; w <= 0x100/4; w += 4) {
                            std::fprintf(stderr,
                                "[POOL_ALLOC]   POST pool+0x%02X: %08X %08X %08X %08X\n",
                                w*4,
                                psp_mem_read<uint32_t>(rdram, pool2 + (w+0)*4),
                                psp_mem_read<uint32_t>(rdram, pool2 + (w+1)*4),
                                psp_mem_read<uint32_t>(rdram, pool2 + (w+2)*4),
                                psp_mem_read<uint32_t>(rdram, pool2 + (w+3)*4));
                        }
                    }
                    std::fflush(stderr);
                }
                // Always log when alloc returns 0 with a valid pool, or for all logged calls
                bool ret_fail = (retval == 0 && pool_ptr >= 0x08000000U && pool_ptr < 0x0A000000U);
                if (log_this || ret_fail) {
                    std::fprintf(stderr,
                        "[ALLOC_DIAG] -> v0=0x%08X%s\n",
                        retval, ret_fail ? " [ALLOC_FAILED!]" : "");
                }
            });
        std::fprintf(stderr, "[RT] Alloc diag wrapper installed for FUN_0895B3AC\n");
    }

    // 4e3. [IOFREE] Probe FUN_08827EA0 — the IO pool's deallocate (pool
    // vtable 0x08A44480 +12). Resolved via the [VT_DUMP] one-shot. The IO
    // streaming buffers (incl. the 512KB block 0x09096FE0) are returned
    // through THIS function, not FUN_0895B410 (that frees the object pool).
    // Logs every free: a0=pool-owner object, a1=block being freed, caller.
    {
        FuncPtr orig_iof = RECOMP_LOOKUP(0x08827EA0);
        static FuncPtr s_iof_orig = nullptr;
        s_iof_orig = orig_iof;
        psp_dispatch_register(0x08827EA0,
            [](uint8_t* rdram, recomp_context* ctx) {
                static int n = 0;
                n++;
                uint32_t a0 = static_cast<uint32_t>(ctx->r[4]);
                uint32_t a1 = static_cast<uint32_t>(ctx->r[5]);
                bool is_io = (a0 == 0x0913E6D0U);
                bool is_512blk = (a1 == 0x09096FE0U);
                if (n <= 40 || is_512blk) {
                    std::fprintf(stderr,
                        "[IOFREE] #%d a0(pool)=0x%08X a1(block)=0x%08X "
                        "a2=0x%08X caller=0x%08X%s%s\n",
                        n, a0, a1, static_cast<uint32_t>(ctx->r[6]),
                        g_last_func_addr,
                        is_io ? " [IO_POOL]" : "",
                        is_512blk ? " [512KB_BLOCK!]" : "");
                    std::fflush(stderr);
                }
                s_iof_orig(rdram, ctx);
            });
        std::fprintf(stderr, "[RT] IO free probe installed for FUN_08827EA0\n");
    }

    // 4f. (REMOVED — CR-01 fix.) Block 4f used to call psp_dispatch_register
    // for 0x08863AA0 a second time, silently overwriting block 4b9's
    // [GK_CALLER_TIER2] wrapper above. The [SEL_DIAG] body is now composed
    // into block 4b9's single lambda. See CR-01 in
    // .planning/phases/11.5-…/11.5-REVIEW.md.

    // 4z. [GE_INIT_PATH] diagnostic wrappers — Phase 11.5 / Plan 03.
    //
    // Per 11.5-DIAGNOSTIC.md (Revision 2): runtime makes ZERO calls to
    // sceKernelRegisterSubIntrHandler / sceKernelEnableSubIntr where PPSSPP
    // makes 2 each. Root cause traced to FUN_0895DBE8 (batch_0166.cpp ~440-454):
    // a virtual call at L_0895DC4C returns 0 in our runtime but non-zero in
    // PPSSPP, causing us to SKIP FUN_0895CD18 (the GE init function that
    // calls FUN_0881762C → sceKernelRegisterSubIntrHandler).
    //
    // These wrappers verify the hypothesis by logging:
    //   (a) FUN_0895DBE8 entry/exit + r[2] after each vtable call
    //   (b) FUN_0895CD18 entry — proves whether the GE init function runs
    //
    // If FUN_0895CD18 entry log NEVER fires, the hypothesis is confirmed and
    // the fix must force the vtable to return non-zero (either by overriding
    // the vtable callee or by patching the result in a wrapper).
    {
        FuncPtr orig_0895dbe8 = RECOMP_LOOKUP(0x0895DBE8);
        static FuncPtr s_orig_0895dbe8 = nullptr;
        s_orig_0895dbe8 = orig_0895dbe8;
        psp_dispatch_register(0x0895DBE8,
            [](uint8_t* rdram, recomp_context* ctx) {
                static int ge_init_dbe8_count = 0;
                ge_init_dbe8_count++;
                uint32_t a0_entry = static_cast<uint32_t>(ctx->r[4]);
                if (ge_init_dbe8_count <= 8) {
                    std::fprintf(stderr,
                        "[GE_INIT_PATH] FUN_0895DBE8 enter #%d a0=0x%08X\n",
                        ge_init_dbe8_count, a0_entry);
                }
                s_orig_0895dbe8(rdram, ctx);
                if (ge_init_dbe8_count <= 8) {
                    // After return, ctx->r[16] is restored; we need to inspect
                    // the result fields at the original this+340 (first vtable
                    // result) and this+348 (second vtable result) which were
                    // written by the original. But ctx->r[16] is restored from
                    // stack post-return, so we can't easily re-read. The v0
                    // (ctx->r[2]) of FUN_0895DBE8 itself indicates whether
                    // FUN_0895CD18 was called (v0=1 means the full GE init
                    // path ran; v0=0 means an early-skip).
                    int32_t v0 = static_cast<int32_t>(ctx->r[2]);
                    std::fprintf(stderr,
                        "[GE_INIT_PATH] FUN_0895DBE8 exit  #%d v0=%d (1=GE init ran, 0=skipped)\n",
                        ge_init_dbe8_count, v0);
                }
            });
        std::fprintf(stderr, "[RT] GE_INIT_PATH wrapper installed for FUN_0895DBE8\n");
    }
    {
        FuncPtr orig_0895cd18 = RECOMP_LOOKUP(0x0895CD18);
        static FuncPtr s_orig_0895cd18 = nullptr;
        s_orig_0895cd18 = orig_0895cd18;
        psp_dispatch_register(0x0895CD18,
            [](uint8_t* rdram, recomp_context* ctx) {
                static int ge_init_cd18_count = 0;
                ge_init_cd18_count++;
                if (ge_init_cd18_count <= 8) {
                    std::fprintf(stderr,
                        "[GE_INIT_PATH] FUN_0895CD18 enter #%d a0=0x%08X a1=0x%08X a2=0x%08X a3=0x%08X (GE init w/ sub-intr)\n",
                        ge_init_cd18_count,
                        static_cast<uint32_t>(ctx->r[4]),
                        static_cast<uint32_t>(ctx->r[5]),
                        static_cast<uint32_t>(ctx->r[6]),
                        static_cast<uint32_t>(ctx->r[7]));
                }
                s_orig_0895cd18(rdram, ctx);
            });
        std::fprintf(stderr, "[RT] GE_INIT_PATH wrapper installed for FUN_0895CD18\n");
    }

    // (Plan 11-05 removed the prior smoke-test direct calls to bnd_init and
    //  the resolver — the FUN_0896A6A4 wrapper above is now the canonical
    //  call site via std::call_once.)

    // 4-fix. [REMOVED band-aid] A prior session overwrote 0x089F8758 from
    //        0x089F8738 ("host0:") to 0x089F8740 ("disc0:"), believing it was
    //        "a pointer to the active device prefix string." It is NOT: it is
    //        table[0] of the device-type lookup array {host0:,disc0:,ms0:,net0:}
    //        consumed by FUN_0886186c (the device-mount). FUN_0886186c does:
    //          for i in 0..3: if strncmp(path, table[i], 4)==0:
    //                              mgr+0x50 = type[i] (= i); break
    //        With table[0] stomped to "disc0:", a "disc0:" mount matched at
    //        index 0 and wrote mgr+0x50 = type[0] = 0 (the UMD-present flag),
    //        instead of advancing to index 1 (type[1] = 1). mgr+0x50 = 0 makes
    //        FUN_08862A10 (the IO-slot retry gate) always return 1, which
    //        permanently stalls titledata's slot SM at state 511 (doc 41).
    //        disc0: asset loading does NOT depend on this stomp: the game's own
    //        FUN_089b4178 mounts the literal "disc0:" (0x08A085A0) and the IO
    //        layer reroutes host0:->ENOENT / disc0:->g_disc0_path. Leaving the
    //        table pristine lets FUN_0886186c(mgr,"disc0:") match index 1 and
    //        write mgr+0x50 = 1 faithfully (the disc IS present). See doc 43.

    // 4a. Override game's dlmalloc with native HLE allocator.
    //     The game's dlmalloc (FUN_0881E7A8) has uninitialized arena
    //     metadata at 0x089F69B4, causing infinite bitmap scan loops.
    //     Instead of trying to initialize the complex malloc_state
    //     struct, we replace the allocator entry points in the
    //     dispatch table with a native bump allocator backed by
    //     PSP memory. See psp_hle_kernel_memory.cpp.
    psp_dlmalloc_override_init();

    // 4a2. Override game's CRT memory functions (memmove, memcpy, memset)
    //      with native implementations. The MIPS versions use LWL/LWR/SWL/SWR
    //      (unaligned load/store) which are emitted as no-op stubs, causing
    //      memmove to loop forever on misaligned copies.
    psp_crt_override_init();

    // 4a3. Override CRT libc assertion handler (FUN_088133EC) that prints
    //      "no reent structure found" and calls sceKernelExitThread(1).
    //      The game's newlib CRT requires a per-thread _reent structure
    //      that the PSP kernel normally provides. Without it, any failed
    //      allocation triggers a fatal assertion. The override returns
    //      gracefully, letting the game handle OOM without thread death.
    psp_crt_assertion_override_init();

    // 4a4. Callback-dispatch observer (#47 P5 seam): IoAsyncCallback
    //      (FUN_088629CC) arg-struct dump, formerly inline in the generic
    //      sceKernelCheckCallback dispatcher.
    patapon_install_callback_observer();
}

/// Boot-context tweaks, invoked immediately before entry() (module_start).
void patapon_on_boot_context(uint8_t* rdram, recomp_context* ctx) {
    // Boot k0+4 heap descriptor (plan decision P12): 0x089F0000 is Patapon's
    // dlmalloc default-heap descriptor — a .bss global inside the game image
    // (FUN_0881E1F4 checks k0+4: if non-zero it uses this heap; if zero it
    // reads a fallback pointer from 0x089F65F0). NOT derivable from
    // analysis.json facts; the generic boot leaves k0+4 = 0.
    // REMOVAL CRITERION: retire together with psp_dlmalloc_override_init —
    // the descriptor only matters to the game allocator's heap selection,
    // which the override currently bypasses.
    uint32_t boot_k0 = static_cast<uint32_t>(ctx->r[26]);
    psp_mem_write<uint32_t>(rdram, boot_k0 + 0x04, 0x089F0000U);

    // [V438FIX] Run C++ static constructors (.init_array) BEFORE module_start
    // (moved verbatim from main.cpp step 9b). The game CRT never walks
    // .init_array on our boot path (PC-trace showed ZERO of the 4600 ctors
    // run), leaving the asset name->entry hash table empty. Opt-in because
    // it currently hangs at FUN_08804778 from the boot ctx (WIP); the asset
    // bucket count read below is a Patapon asset-manager global.
    if (std::getenv("PSPRECOMP_RUN_CTORS")) {
        std::fprintf(stderr, "[V438FIX] running psp_call_constructors()...\n");
        psp_call_constructors(rdram, ctx);
        uint32_t bc = MEM_W(rdram, 0x089E0000U + 25124U);
        std::fprintf(stderr,
            "[V438FIX] constructors done; asset bucket_count=%u\n", bc);
    }
}

/// Per-thread k0 block hook (psp_hle_kernel_thread.cpp, after the standard
/// PPSSPP-shape writes): same P12 heap descriptor for every created thread.
void patapon_on_thread_start(uint8_t* rdram, uint32_t k0_addr) {
    // REMOVAL CRITERION: retire together with psp_dlmalloc_override_init
    // (see patapon_on_boot_context).
    psp_mem_write<uint32_t>(rdram, k0_addr + 0x04, 0x089F0000U);
}

const PspGameModule g_patapon_module = {
    /* id             */ "patapon",
    /* register_hooks */ patapon_register_hooks,
    /* on_boot_context*/ patapon_on_boot_context,
    /* on_thread_start*/ patapon_on_thread_start,
};

}  // namespace

/// The one strong definition for -DPSPRECOMP_GAME=patapon builds
/// (psp_game_default.cpp provides the no-op module otherwise).
const PspGameModule* psp_game_module() {
    return &g_patapon_module;
}
