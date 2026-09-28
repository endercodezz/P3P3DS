// games/patapon/runtime/hooks_dispatch.cpp — Patapon LOOKUP_MISS handler
// (issues #46 / #47 Phase 5).
//
// Moved verbatim from runtime/src/psp_dispatch.cpp's noop_stub: the
// IO-slot field dump (Patapon async-slot struct offsets +292/+300/+308),
// the [BND_VTABLE_MISS] punch list (misses whose caller sits in the BND
// arena, i.e. a noop-stub-filled fake vtable slot), and the 0x438
// corrupt-vtable workaround with its [V438*] diagnostics. The generic
// dispatcher invokes this through psp_dispatch_set_miss_handler.
// Compiled only under -DPSPRECOMP_GAME=patapon.

#include "recomp.h"
#include "psp_memory.h"
#include "hle/psp_hle.h"
#include "hle/psp_hle_kernel.h"
#include "asset_bnd.h"   // PSP_BND_ARENA_BASE / PSP_BND_ARENA_END
#include "patapon_hooks.h"

#include <cstdio>
#include <cstdlib>
#include <execinfo.h>
#include <dlfcn.h>

extern thread_local uint32_t g_last_func_addr;  // last-dispatched fn addr
extern thread_local uint32_t g_func_ring[32];   // ring of recent fn entries
extern thread_local uint32_t g_func_ring_pos;   // ring write position

static void patapon_lookup_miss_handler(
    uint8_t* rdram, recomp_context* ctx, uint32_t addr, int count
) {
    // If r17 looks like a valid PSP IO-slot ptr, dump slot fields (same
    // per-address quota as the generic [LOOKUP_MISS_CTX] line).
    if (count <= 5) {
        uint32_t r17 = static_cast<uint32_t>(ctx->r[17]);
        if (r17 >= 0x08000000U && r17 < 0x0A000000U) {
            uint32_t base = r17 & 0x07FFFFFFU;
            auto rd32 = [&](uint32_t off) {
                return *reinterpret_cast<uint32_t*>(rdram + base + off);
            };
            std::fprintf(stderr,
                "[LOOKUP_MISS_CTX]   slot: state=%u ap=%u fd=%u [292]=%u [300]=%u [308]=%u\n",
                rd32(0), rd32(12), rd32(16), rd32(292), rd32(300), rd32(308));
        }
    }

    // [BND_VTABLE_MISS] — if g_last_func_addr is in the BND arena, the caller
    // was a noop_stub-filled vtable slot allocated by Phase 11's
    // bnd_resolve_and_allocate (allocate_shared_noop_vtable). Punch-list
    // output for Phase 11.2 R11.2-09 + A12 acceptance — fires regardless of
    // the per-address `count <= 5` quota above so we always see new vtable
    // sources as the consumer advances. See Pattern E in 11.2-PATTERNS.md.
    if (g_last_func_addr >= PSP_BND_ARENA_BASE
            && g_last_func_addr < PSP_BND_ARENA_END) {
        static int vtable_miss_count = 0;
        vtable_miss_count++;
        if (vtable_miss_count <= 8 || vtable_miss_count % 500 == 0) {
            std::fprintf(stderr,
                "[BND_VTABLE_MISS] from=0x%08X (BND arena vtable entry) "
                "target=0x%08X (#%d)\n",
                g_last_func_addr, addr, vtable_miss_count);
        }
    }

    // WORKAROUND: 0x438 is a corrupt vtable dispatch that should call
    // FUN_0895b798 (sceKernelSignalSema thunk for uid=259).
    // The original function loads the UID from the object, but the object
    // pointer is corrupt (0x0003796C — invalid PSP address). Signal uid=259
    // directly to keep the render pipeline flowing.
    // REMOVAL CRITERION: retire when the corrupt-vtable source is root-fixed
    // (the [V438] probes below exist to find it) — the BND fake-vtable layer
    // and this workaround go together.
    if (addr == 0x438) {
        // [V438] Native-path root-cause probe (Phase 12): capture the REAL
        // indirect-call site. In recompiled MIPS, jalr sets r31=return addr,
        // so ctx->r[31] points just past the bad call. a0..a3 carry the
        // object/args. Dump the object's first words to see where the
        // corrupt pointer (0x0003796C = asset size) originates.
        static int v438_count = 0;
        if (++v438_count <= 8) {
            // Host return address → the generated FUN_ that made this call.
            void* host_ra = __builtin_return_address(0);
            Dl_info dli; const char* sym = "?";
            if (dladdr(host_ra, &dli) && dli.dli_sname) sym = dli.dli_sname;
            std::fprintf(stderr, "[V438] host_ra=%p sym=%s r25=0x%08X\n",
                host_ra, sym, static_cast<uint32_t>(ctx->r[25]));
            uint32_t a0 = static_cast<uint32_t>(ctx->r[4]);
            std::fprintf(stderr,
                "[V438] #%d ra=0x%08X a0=0x%08X a1=0x%08X a2=0x%08X "
                "s0=0x%08X s1=0x%08X s2=0x%08X s3=0x%08X gp=0x%08X\n",
                v438_count,
                static_cast<uint32_t>(ctx->r[31]),
                a0, static_cast<uint32_t>(ctx->r[5]),
                static_cast<uint32_t>(ctx->r[6]),
                static_cast<uint32_t>(ctx->r[16]),
                static_cast<uint32_t>(ctx->r[17]),
                static_cast<uint32_t>(ctx->r[18]),
                static_cast<uint32_t>(ctx->r[19]),
                static_cast<uint32_t>(ctx->r[28]));
            if (a0 >= 0x08000000U && a0 < 0x0A000000U) {
                uint32_t b = a0 & 0x07FFFFFFU;
                std::fprintf(stderr,
                    "[V438]   *a0[0..3]=0x%08X 0x%08X 0x%08X 0x%08X\n",
                    *reinterpret_cast<uint32_t*>(rdram + b + 0),
                    *reinterpret_cast<uint32_t*>(rdram + b + 4),
                    *reinterpret_cast<uint32_t*>(rdram + b + 8),
                    *reinterpret_cast<uint32_t*>(rdram + b + 12));
            }
            // Recent function-entry chain (most recent last).
            std::fprintf(stderr, "[V438]   ring:");
            for (int i = 0; i < 32; ++i) {
                uint32_t e = g_func_ring[(g_func_ring_pos + i) & 31u];
                if (e) std::fprintf(stderr, " %08X", e);
            }
            std::fprintf(stderr, "\n");
            // Dump outer object s0 fields 0..96 to find the NULL sub-ptr.
            uint32_t s0 = static_cast<uint32_t>(ctx->r[16]);
            if (s0 >= 0x08000000U && s0 < 0x0A000000U) {
                uint32_t b = s0 & 0x07FFFFFFU;
                std::fprintf(stderr, "[V438]   s0obj@0x%08X:", s0);
                for (uint32_t o = 0; o <= 96; o += 4) {
                    std::fprintf(stderr, " +%u=%08X", o,
                        *reinterpret_cast<uint32_t*>(rdram + b + o));
                }
                std::fprintf(stderr, "\n");
            }
            // [V438FULL] full register file + dump *(reg) for any reg that looks
            // like a live PSP object pointer, to find the bogus object register
            // (same-snapshot; addresses captured at OTHER times are unreliable).
            // [V438BT] real host backtrace — names the recompiled FUN_ call chain
            // at the crash (lldb can't reach this crash point in batch time).
            {
                void* bt[24];
                int n = backtrace(bt, 24);
                char** syms = backtrace_symbols(bt, n);
                if (syms) {
                    std::fprintf(stderr, "[V438BT] host stack (%d frames):\n", n);
                    for (int bi = 0; bi < n; ++bi)
                        std::fprintf(stderr, "[V438BT]   %s\n", syms[bi]);
                    free(syms);
                }
            }
            std::fprintf(stderr, "[V438FULL] regs:");
            for (int ri = 0; ri < 32; ++ri) {
                std::fprintf(stderr, " r%d=%08X", ri,
                    static_cast<uint32_t>(ctx->r[ri]));
            }
            std::fprintf(stderr, "\n");
            for (int ri = 0; ri < 32; ++ri) {
                uint32_t rv = static_cast<uint32_t>(ctx->r[ri]);
                if (rv >= 0x08000000U && rv < 0x0A000000U) {
                    uint32_t rb = rv & 0x07FFFFFFU;
                    std::fprintf(stderr,
                        "[V438FULL]   *r%d@%08X: +0=%08X +4=%08X +8=%08X +12=%08X\n",
                        ri, rv,
                        *reinterpret_cast<uint32_t*>(rdram + rb + 0),
                        *reinterpret_cast<uint32_t*>(rdram + rb + 4),
                        *reinterpret_cast<uint32_t*>(rdram + rb + 8),
                        *reinterpret_cast<uint32_t*>(rdram + rb + 12));
                }
            }
        }
        psp_hle_signal_sema_by_uid(259, 1);
    }
}

void patapon_install_miss_handler() {
    psp_dispatch_set_miss_handler(patapon_lookup_miss_handler);
}
