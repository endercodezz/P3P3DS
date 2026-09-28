// games/patapon/runtime/hooks_memory.cpp — Patapon allocator/CRT overrides
// and kernel-memory debug hooks (issues #46 / #47 Phase 5).
//
// Moved VERBATIM from runtime/src/hle/psp_hle_kernel_memory.cpp (relocation,
// not rewrite): the dlmalloc/CRT/sprintf overrides at Patapon newlib
// addresses, the vtable-alloc BSS fallback, the list-iterator loop breaker,
// the GE-gatekeeper/object-state-machine corruption fixes, and every
// PSPRECOMP_*-gated investigation probe they carry. The generic bump heap
// stayed in the runtime core; this file reaches it through the
// psp_kmem_* seams (psp_hle.h) instead of touching its statics.
//
// Compiled only under -DPSPRECOMP_GAME=patapon. Behavioral (non-diagnostic)
// hooks carry REMOVAL CRITERION comments per issue #46; band-aids are
// disabled wholesale by PSPRECOMP_CLEANROOM=1 (the verification gate).

#include "recomp.h"
#include "psp_memory.h"
#include "psp_scheduler.h"
#include "hle/psp_hle.h"
#include "hle/psp_hle_kernel.h"
#include "patapon_hooks.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <algorithm>
#include <pthread.h>

// Forward declarations
void psp_dlmalloc_set_arena(uint32_t base, uint32_t size);

// ---- Guest-function resolution (issue #47 Phase 1) ----
// The hooks in this file wrap specific Patapon guest functions. They used
// to name them as link-time extern declarations of generated FUN_* symbols,
// which made the runtime unlinkable against any other game's output. The
// originals are now resolved through the dispatch table instead:
// psp_hle_kernel_memory_resolve_guest_funcs() runs immediately after
// psp_init_dispatch_table() — while the table is PRISTINE — so each slot
// captures the original generated function, exactly what the extern used
// to bind to. Resolving any later would capture the hook wrappers
// registered below (self-recursion) or main.cpp's own hooks (double
// layering). A slot stays nullptr when the address is absent from this
// game's dispatch table; call_guest() then warns once and no-ops.
//
// NOTE: FUN_08827F7C is coalesced into its frame owner FUN_08827E7C (the
// emitter's Option-A coalesce pass), so it no longer exists as a standalone
// function. 0x08827F7C and its mid-entry 0x08827F9C are now mid-entries of
// FUN_08827E7C; call the owner with the appropriate entry_point instead.
static FuncPtr s_FUN_08827E7C = nullptr;  // vtable dispatch trampoline (owner)
static FuncPtr s_FUN_08827A50 = nullptr;  // vtable-dispatch allocator
static FuncPtr s_FUN_088623E0 = nullptr;  // GE gatekeeper
static FuncPtr s_FUN_0885EEE8 = nullptr;  // pool initializer
static FuncPtr s_FUN_0885FE90 = nullptr;  // per-asset object state machine
// Asset registry lookup + the next function (used as its upper code bound) — to
// scope the strncpy r17 band-aid so it does NOT fire for the lookup's strncpy.
static FuncPtr s_FUN_0896A6A4 = nullptr;
static FuncPtr s_FUN_0896AA60 = nullptr;
// Subsystem-list push (FUN_089b4cec): a0=list(+0xa864), a1=&payload. Diagnostic
// wrapper gated by PSPRECOMP_PUSH_TRACE captures construction-time node/links.
static FuncPtr s_FUN_089B4CEC = nullptr;
// Subsystem-list+pool ctor (FUN_089b4420): a0=list-obj base; sentinel=a0+0x314.
static FuncPtr s_FUN_089B4420 = nullptr;
static FuncPtr s_FUN_089B4B68 = nullptr;  // list-iterator != comparison
static FuncPtr s_FUN_08822D5C = nullptr;  // strncpy (called just before GE_GATE)
static FuncPtr s_FUN_089B4DF0 = nullptr;  // engine frame tick ([DF0_PROBE])
static FuncPtr s_FUN_089B440C = nullptr;  // std::list "set end" ([END_PROBE])
static FuncPtr s_FUN_0885EFC8 = nullptr;  // loadinggroup driver ([CTX_PROBE])
static FuncPtr s_FUN_088601C4 = nullptr;  // GE-finish completion cb ([CTX_PROBE])

void psp_hle_kernel_memory_resolve_guest_funcs() {
    struct Slot { FuncPtr* slot; uint32_t addr; };
    static constexpr size_t k_slot_count = 15;
    const Slot slots[k_slot_count] = {
        { &s_FUN_08827E7C, 0x08827E7CU },
        { &s_FUN_08827A50, 0x08827A50U },
        { &s_FUN_088623E0, 0x088623E0U },
        { &s_FUN_0885EEE8, 0x0885EEE8U },
        { &s_FUN_0885FE90, 0x0885FE90U },
        { &s_FUN_0896A6A4, 0x0896A6A4U },
        { &s_FUN_0896AA60, 0x0896AA60U },
        { &s_FUN_089B4CEC, 0x089B4CECU },
        { &s_FUN_089B4420, 0x089B4420U },
        { &s_FUN_089B4B68, 0x089B4B68U },
        { &s_FUN_08822D5C, 0x08822D5CU },
        { &s_FUN_089B4DF0, 0x089B4DF0U },
        { &s_FUN_089B440C, 0x089B440CU },
        { &s_FUN_0885EFC8, 0x0885EFC8U },
        { &s_FUN_088601C4, 0x088601C4U },
    };
    size_t missing = 0;
    for (const Slot& s : slots) {
        *s.slot = psp_dispatch_probe_lookup(s.addr);
        if (*s.slot == nullptr) {
            missing++;
            std::fprintf(stderr,
                "[HLE] kernel-memory hook target 0x%08X absent from this "
                "game's dispatch table — its hook will no-op\n", s.addr);
        }
    }
    if (missing > 0) {
        std::fprintf(stderr,
            "[HLE] kernel-memory hooks: %zu/%zu wrapped guest fns missing "
            "(these hooks are Patapon-specific; see issue #47)\n",
            missing, k_slot_count);
    }
}

/// Call a resolved guest function. When the slot is nullptr (address was
/// absent from the dispatch table at resolve time), warn once per address
/// and return without calling — the hook degrades to a transparent no-op.
static void call_guest(FuncPtr fn, uint32_t addr,
                       uint8_t* rdram, recomp_context* ctx) {
    if (fn != nullptr) {
        fn(rdram, ctx);
        return;
    }
    static std::mutex warn_mutex;
    {
        std::lock_guard<std::mutex> lock(warn_mutex);
        static std::unordered_set<uint32_t> warned;
        if (!warned.insert(addr).second) return;
    }
    std::fprintf(stderr,
        "[HLE] inert hook called: guest fn 0x%08X was never resolved\n",
        addr);
}
extern thread_local uint32_t g_prev_func_addr;  // caller of last-dispatched fn
extern thread_local uint32_t g_last_func_addr;  // last-dispatched fn addr
extern thread_local uint32_t g_func_ring[32];   // ring of recent fn-entry addrs
extern thread_local uint32_t g_func_ring_pos;   // ring write position

// [CLEANROOM] Agent F: master gate to disable the GUESSING register-mutating
// band-aids (OBJ_SM_FIX, GE_BASE_FIX, GE_GATE_FIX, STRNCPY_FIX, LOOSE_REGION_FIX,
// the list-iter loop-breaker, the vtable-alloc bump fallback) while KEEPING the
// faithful 19A/19B/19C fixes (BND parser, gatekeeper_layout_needed, the BND
// reroute). When set, the debug wrappers become pure pass-throughs that forward
// to the recompiled function with NO register/state mutation. Diagnostic infra,
// not a band-aid: default (env unset) preserves current behavior exactly.
static inline bool psp_cleanroom() {
    static const bool v = std::getenv("PSPRECOMP_CLEANROOM") != nullptr;
    return v;
}

// ---- Native dlmalloc Override ----
// The game's dlmalloc (FUN_0881E7A8) has uninitialized arena
// metadata at 0x089F69B4, causing an infinite bitmap scan loop.
// Rather than trying to initialize the complex malloc_state struct,
// we replace the allocator entry points in the dispatch table with
// a native bump allocator backed by PSP memory.
//
// Overridden functions:
//   FUN_0881E558 (memalign): r[4]=alignment, r[5]=size -> r[2]=ptr
//   FUN_0881E7A8 (dlmalloc core): r[4]=heap, r[5]=align, r[6]=size -> r[2]=ptr
//
// The allocator uses a simple bump strategy with 16-byte alignment.
// Free is a no-op (leak-tolerant for boot sequence; games rarely
// free during init). This is sufficient to break the boot stall
// and let user_main proceed to FileThread creation.

// dlmalloc arena tracks: set when the game allocates UserSbrk
// via sceKernelAllocPartitionMemory. The game's dlmalloc would
// normally sub-allocate from this block via its sbrk wrapper.
// Our native allocator does the same.
static uint32_t g_dlmalloc_pos = 0;
static uint32_t g_dlmalloc_end = 0;
static std::mutex g_dlmalloc_mtx;
static uint32_t g_dlmalloc_count = 0;
static bool g_dlmalloc_arena_ready = false;
// __attribute__((used)) prevents the compiler from optimizing these away
// as dead stores — they are only read by lldb's psp_heap formatter.
__attribute__((used)) static uint32_t g_dlmalloc_base = 0;
static constexpr int DLMALLOC_TOP_N = 8;
__attribute__((used)) static uint32_t g_dlmalloc_top_sizes[DLMALLOC_TOP_N] = {};

/// Set the dlmalloc arena to the full remaining user memory.
/// Called when sceKernelAllocPartitionMemory allocates a block
/// whose name contains "Sbrk" (the game's heap block).
/// Uses the UserSbrk base as arena start but expands the arena
/// end to g_heap_end (PSP_USER_MEM_END) for full user memory.
void psp_dlmalloc_set_arena(uint32_t base, uint32_t size) {
    g_dlmalloc_pos = base;
    g_dlmalloc_base = base;
    // Expand arena to full remaining user memory, not just
    // the UserSbrk partition size. Stacks grow downward from
    // PSP_USER_MEM_END separately (psp_hle_kernel_thread.cpp).
    // psp_kmem_reserve_remaining (#47 P5 seam) also advances the core
    // heap position to its end so future AllocPartitionMemory calls
    // cannot overlap the dlmalloc arena — same effect as the historical
    // direct g_heap_pos = g_heap_end write.
    uint32_t rem_start = 0;
    uint32_t rem_end = 0;
    psp_kmem_reserve_remaining(&rem_start, &rem_end);
    (void)rem_start;  // arena starts at the UserSbrk base, not the heap pos
    g_dlmalloc_end = rem_end;
    g_dlmalloc_arena_ready = true;

    uint32_t arena_size = g_dlmalloc_end - base;
    std::fprintf(stderr,
        "[DLMALLOC] Arena set: 0x%08X - 0x%08X (%u MB) "
        "(expanded from %u MB UserSbrk)\n",
        base, g_dlmalloc_end, arena_size / (1024 * 1024),
        size / (1024 * 1024));
}

/// Lazily initialize the dlmalloc override arena
/// if the UserSbrk block was not captured.
static void dlmalloc_ensure_init() {
    if (g_dlmalloc_arena_ready) return;

    // Fallback: claim all remaining partition memory via the #47 P5 seam.
    // Stacks are allocated separately from the top of
    // user memory (psp_hle_kernel_thread.cpp), so no
    // reserve is needed here.
    uint32_t arena_start = 0;
    uint32_t arena_end = 0;
    psp_kmem_reserve_remaining(&arena_start, &arena_end);
    uint32_t available = arena_end - arena_start;
    if (available == 0) {
        std::fprintf(stderr,
            "[DLMALLOC] FATAL: no memory for fallback "
            "arena (avail=0x%08X)\n", available);
        return;
    }

    g_dlmalloc_pos = arena_start;
    g_dlmalloc_base = arena_start;
    g_dlmalloc_end = arena_end;
    g_dlmalloc_arena_ready = true;

    std::fprintf(stderr,
        "[DLMALLOC] Fallback arena: 0x%08X - 0x%08X "
        "(%u MB)\n",
        arena_start, g_dlmalloc_end,
        available / (1024 * 1024));
}

/// Native memalign: bump allocator with alignment support.
/// Returns PSP address in r[2], or 0 on failure.
static uint32_t dlmalloc_memalign(uint32_t alignment,
                                   uint32_t size) {
    std::lock_guard<std::mutex> lock(g_dlmalloc_mtx);
    dlmalloc_ensure_init();

    if (g_dlmalloc_pos == 0) return 0;

    // Minimum alignment: 16 bytes (dlmalloc standard)
    if (alignment < 16) alignment = 16;

    // Round up position to alignment boundary
    uint32_t aligned_pos =
        (g_dlmalloc_pos + alignment - 1) & ~(alignment - 1);

    // Reject corrupt sizes: anything exceeding PSP physical memory
    // (128MB) is definitively a corrupt argument from the vtable
    // constructor chain, not a legitimate allocation.
    if (size > PSP_MEM_SIZE) {
        static int s_corrupt_log_count = 0;
        if (s_corrupt_log_count < 10) {
            std::fprintf(stderr,
                "[DLMALLOC] CORRUPT size rejected: %u bytes "
                "(%u MB) align=%u -- exceeds PSP physical "
                "memory\n",
                size, size / (1024 * 1024), alignment);
            ++s_corrupt_log_count;
            if (s_corrupt_log_count == 10) {
                std::fprintf(stderr,
                    "[DLMALLOC] (further corrupt size "
                    "messages suppressed)\n");
            }
        }
        return 0;
    }

    // Diagnose suspiciously large allocations (> 50MB)
    if (size > 50u * 1024 * 1024) {
        std::fprintf(stderr,
            "[DLMALLOC] LARGE alloc: %u bytes (%u MB) "
            "from 0x%08X\n",
            size, size / (1024 * 1024), aligned_pos);
    }

    // Use uint64_t to prevent unsigned overflow wrapping
    uint64_t end_pos =
        static_cast<uint64_t>(aligned_pos) + size;
    if (end_pos > g_dlmalloc_end) {
        std::fprintf(stderr,
            "[DLMALLOC] OUT OF MEMORY: need %u (%u MB) "
            "at 0x%08X, limit 0x%08X\n",
            size, size / (1024 * 1024),
            aligned_pos, g_dlmalloc_end);
        return 0;
    }

    g_dlmalloc_pos = aligned_pos + size;
    g_dlmalloc_count++;

    // Track top-N largest allocation sizes
    if (size > 0) {
        for (int i = 0; i < DLMALLOC_TOP_N; ++i) {
            if (size > g_dlmalloc_top_sizes[i]) {
                // Shift smaller entries down
                for (int j = DLMALLOC_TOP_N - 1; j > i; --j) {
                    g_dlmalloc_top_sizes[j] =
                        g_dlmalloc_top_sizes[j - 1];
                }
                g_dlmalloc_top_sizes[i] = size;
                break;
            }
        }
    }

    return aligned_pos;
}

/// HLE override for FUN_0881E558 (memalign entry point).
/// Called with: r[4]=alignment, r[5]=size
/// Returns: r[2]=pointer (or 0)
static void hle_dlmalloc_memalign(
    uint8_t* rdram, recomp_context* ctx
) {
    uint32_t alignment = static_cast<uint32_t>(ctx->r[4]);
    uint32_t size = static_cast<uint32_t>(ctx->r[5]);

    uint32_t ptr = dlmalloc_memalign(alignment, size);

    // Zero-fill the allocated memory (calloc semantics --
    // the game's calloc wrapper expects this)
    if (ptr != 0) {
        std::memset(rdram + (ptr & PSP_ADDR_MASK), 0, size);
    }

    ctx->r[2] = static_cast<int32_t>(ptr);
}

/// HLE override for FUN_0881E7A8 (dlmalloc core).
/// Called with TWO conventions:
///   From memalign: r[4]=heap, r[5]=alignment, r[6]=size
///   From malloc:   r[4]=heap, r[5]=size (no alignment)
///
/// Since we cannot distinguish calling convention at the
/// override level, we override the malloc wrapper (FUN_0881E750)
/// separately below. This override handles the 3-arg memalign
/// convention.
/// Returns: r[2]=pointer (or 0)
static void hle_dlmalloc_core(
    uint8_t* rdram, recomp_context* ctx
) {
    uint32_t alignment = static_cast<uint32_t>(ctx->r[5]);
    uint32_t size = static_cast<uint32_t>(ctx->r[6]);

    (void)ctx->r[4];  // heap_state ignored

    uint32_t ptr = dlmalloc_memalign(alignment, size);

    if (ptr != 0) {
        std::memset(rdram + (ptr & PSP_ADDR_MASK), 0, size);
    }

    ctx->r[2] = static_cast<int32_t>(ptr);
}

/// HLE override for FUN_0881E750 (malloc wrapper).
/// Called with: r[4]=size
/// Returns: r[2]=pointer (or 0)
///
/// The game's malloc calls get_heap() then dlmalloc_core(heap,
/// size). When dlmalloc_core is overridden, the 2-arg calling
/// convention (heap, size) is misinterpreted as 3-arg memalign
/// (heap, alignment, size), reading garbage from r[6] as size.
/// This override short-circuits the wrapper and calls our
/// allocator directly with the correct size.
static void hle_malloc(
    uint8_t* rdram, recomp_context* ctx
) {
    uint32_t size = static_cast<uint32_t>(ctx->r[4]);

    uint32_t ptr = dlmalloc_memalign(16, size);  // default align

    if (ptr != 0) {
        std::memset(rdram + (ptr & PSP_ADDR_MASK), 0, size);
    }

    ctx->r[2] = static_cast<int32_t>(ptr);
}

/// HLE override for FUN_08804338 (C++ operator new).
/// Called with: r[4]=size
/// Returns: r[2]=pointer (or 0)
///
/// Override prevents the retry loop and bad_alloc throw on OOM.
/// Standard PSP behavior: return NULL on OOM, let caller handle.
static void hle_operator_new(
    uint8_t* rdram, recomp_context* ctx
) {
    uint32_t size = static_cast<uint32_t>(ctx->r[4]);
    if (size == 0) size = 1;  // C++ standard: new(0) allocates

    uint32_t ptr = dlmalloc_memalign(16, size);

    if (ptr != 0) {
        std::memset(rdram + (ptr & PSP_ADDR_MASK), 0, size);
    }

    ctx->r[2] = static_cast<int32_t>(ptr);
}

/// HLE override for FUN_0880416C (C++ operator delete / free).
/// Our bump allocator does not support free. This is a no-op.
static void hle_operator_delete(
    uint8_t* rdram, recomp_context* ctx
) {
    // No-op: bump allocator, free is leak-tolerant for boot
    (void)rdram;
    (void)ctx;
}

/// Partition-alloc observer (#47 P5 seam): when the game allocates its
/// UserSbrk block, capture it as the dlmalloc-override arena. This used to
/// be an inline strstr("Sbrk") branch in the core AllocPartitionMemory.
static void patapon_partition_alloc_observer(
    const char* name, uint32_t addr, uint32_t size
) {
    if (std::strstr(name, "Sbrk") != nullptr) {
        psp_dlmalloc_set_arena(addr, size);
    }
}

/// Initialize dlmalloc dispatch overrides.
/// Must be called after psp_init_dispatch_table() and psp_hle_init().
void psp_dlmalloc_override_init() {
    // Watch for the UserSbrk partition allocation (arena capture).
    psp_kmem_set_partition_alloc_observer(patapon_partition_alloc_observer);

    // Override FUN_0881E558 (memalign entry point)
    psp_dispatch_register(0x0881E558U,
        reinterpret_cast<FuncPtr>(hle_dlmalloc_memalign));

    // Override FUN_0881E7A8 (dlmalloc core, 3-arg memalign)
    psp_dispatch_register(0x0881E7A8U,
        reinterpret_cast<FuncPtr>(hle_dlmalloc_core));

    // Override FUN_0881E750 (malloc wrapper)
    // CRITICAL: the malloc wrapper calls dlmalloc_core with a
    // 2-arg convention (heap, size) but our dlmalloc_core
    // override reads 3 args (heap, alignment, size). Without
    // this override, r[6] garbage is read as size, producing
    // corrupt 138MB / 4GB allocation requests.
    psp_dispatch_register(0x0881E750U,
        reinterpret_cast<FuncPtr>(hle_malloc));

    // Override FUN_08804338 (C++ operator new)
    // Prevents the retry loop and bad_alloc throw on OOM.
    psp_dispatch_register(0x08804338U,
        reinterpret_cast<FuncPtr>(hle_operator_new));

    // Override FUN_0880416C (C++ operator delete / free)
    // Our bump allocator does not support free.
    psp_dispatch_register(0x0880416CU,
        reinterpret_cast<FuncPtr>(hle_operator_delete));

    std::fprintf(stderr,
        "[RT] dlmalloc overrides registered "
        "(memalign=0x0881E558, core=0x0881E7A8, "
        "malloc=0x0881E750, new=0x08804338, "
        "delete=0x0880416C)\n");
}

// ---- Native CRT Memory Function Overrides ----
// The game's CRT memmove/memcpy (FUN_0881F280, FUN_0881F120)
// use LWL/LWR/SWL/SWR (unaligned MIPS load/store) instructions
// in their alignment prologues. These are emitted as no-op stubs
// by the recompiler (Phase 2 decision: deferred to Phase 7).
// The no-ops cause memmove to skip alignment byte copies, leading
// to incorrect loop state and either infinite loops or wrong data.
//
// Fix: replace with native std::memmove/memcpy/memset that operate
// directly on the rdram buffer. This is PSP-correct behavior --
// the semantics are identical, just using host instructions.
//
// Overridden functions:
//   FUN_0881F280 (memmove): r[4]=dst, r[5]=src, r[6]=size -> r[2]=dst
//   FUN_0881F120 (memcpy):  r[4]=dst, r[5]=src, r[6]=size -> r[2]=dst
//   FUN_0881F5C0 (memset):  r[4]=dst, r[5]=val, r[6]=size -> r[2]=dst

/// HLE override for FUN_0881F280 (memmove).
/// MIPS calling convention: a0=dst, a1=src, a2=size, returns dst in v0.
static void hle_memmove(
    uint8_t* rdram, recomp_context* ctx
) {
    uint32_t dst = static_cast<uint32_t>(ctx->r[4]);
    uint32_t src = static_cast<uint32_t>(ctx->r[5]);
    uint32_t size = static_cast<uint32_t>(ctx->r[6]);

    // Bounds check: clamp to PSP address space
    uint32_t dst_off = dst & PSP_ADDR_MASK;
    uint32_t src_off = src & PSP_ADDR_MASK;
    if (size > 0 && dst_off + size <= PSP_MEM_SIZE
        && src_off + size <= PSP_MEM_SIZE) {
        std::memmove(rdram + dst_off, rdram + src_off, size);
    }

    ctx->r[2] = static_cast<int32_t>(dst);
}

/// HLE override for FUN_0881F120 (memcpy).
/// Same convention as memmove, but no overlap handling needed.
static void hle_memcpy(
    uint8_t* rdram, recomp_context* ctx
) {
    uint32_t dst = static_cast<uint32_t>(ctx->r[4]);
    uint32_t src = static_cast<uint32_t>(ctx->r[5]);
    uint32_t size = static_cast<uint32_t>(ctx->r[6]);

    uint32_t dst_off = dst & PSP_ADDR_MASK;
    uint32_t src_off = src & PSP_ADDR_MASK;
    if (size > 0 && dst_off + size <= PSP_MEM_SIZE
        && src_off + size <= PSP_MEM_SIZE) {
        std::memcpy(rdram + dst_off, rdram + src_off, size);
    }

    ctx->r[2] = static_cast<int32_t>(dst);
}

/// HLE override for FUN_0881F5C0 (memset).
/// MIPS calling convention: a0=dst, a1=value(byte), a2=size,
/// returns dst in v0.
static void hle_memset(
    uint8_t* rdram, recomp_context* ctx
) {
    uint32_t dst = static_cast<uint32_t>(ctx->r[4]);
    uint8_t val = static_cast<uint8_t>(ctx->r[5] & 0xFF);
    uint32_t size = static_cast<uint32_t>(ctx->r[6]);

    uint32_t dst_off = dst & PSP_ADDR_MASK;
    if (size > 0 && dst_off + size <= PSP_MEM_SIZE) {
        std::memset(rdram + dst_off, val, size);
    }

    ctx->r[2] = static_cast<int32_t>(dst);
}

// ---- HLE sprintf ----
// The game's recompiled newlib _svfprintf_r (at 0x0882473C) has a
// recompilation bug: each %s substitution is prepended with "0s"
// (bytes 0x30,0x73).  This corrupts all sprintf output, breaking
// path construction (e.g. "0sdisc0:0s/PSP_GAME/..." instead of
// "disc0:/PSP_GAME/..."), which prevents DVDUMD_SAMPLE callback
// creation and cascades into PRIM=0.
//
// Fix: HLE the top-level sprintf wrapper (FUN_088226BC) so that
// format processing uses the host's snprintf.
//
// Calling convention (MIPS o32):
//   r[4] = dest buffer (PSP address)
//   r[5] = format string (PSP address)
//   r[6]..r[11] = first 6 varargs (ints or PSP pointers)
//   returns r[2] = number of chars written (not including NUL)

/// Read a NUL-terminated string from PSP memory.
static const char* psp_read_cstr(
    const uint8_t* rdram, uint32_t psp_addr
) {
    uint32_t off = psp_addr & PSP_ADDR_MASK;
    if (off >= PSP_MEM_SIZE) return "";
    return reinterpret_cast<const char*>(rdram + off);
}

/// HLE override for FUN_088226BC (sprintf).
static void hle_sprintf(
    uint8_t* rdram, recomp_context* ctx
) {
    uint32_t dest_addr = static_cast<uint32_t>(ctx->r[4]);
    uint32_t fmt_addr  = static_cast<uint32_t>(ctx->r[5]);

    const char* fmt = psp_read_cstr(rdram, fmt_addr);

    // Varargs: r[6]..r[11] hold the first 6 variadic arguments.
    // The PSP sprintf wrapper (FUN_088226BC) stores r[6]-r[11]
    // contiguously and passes a pointer as the va_list to
    // _svfprintf_r.  We read them directly from registers.
    uint32_t args[6] = {
        static_cast<uint32_t>(ctx->r[6]),
        static_cast<uint32_t>(ctx->r[7]),
        static_cast<uint32_t>(ctx->r[8]),
        static_cast<uint32_t>(ctx->r[9]),
        static_cast<uint32_t>(ctx->r[10]),
        static_cast<uint32_t>(ctx->r[11]),
    };

    // Walk the format string, processing one specifier at a time.
    char result[2048];
    int rlen = 0;
    int ai = 0;            // arg index
    const char* p = fmt;

    while (*p && rlen < static_cast<int>(sizeof(result)) - 1) {
        // --- literal run ---
        if (*p != '%') {
            result[rlen++] = *p++;
            continue;
        }
        p++;  // skip '%'
        if (*p == '%') { result[rlen++] = '%'; p++; continue; }
        if (*p == '\0') break;

        // --- collect the full specifier into spec[] ---
        char spec[64];
        int si = 0;
        spec[si++] = '%';

        // flags
        while (*p == '-' || *p == '+' || *p == ' '
               || *p == '#' || *p == '0') {
            if (si < 60) spec[si++] = *p;
            p++;
        }
        // width
        if (*p == '*') {
            if (ai < 6) {
                si += std::snprintf(
                    spec + si, sizeof(spec) - si, "%d",
                    static_cast<int32_t>(args[ai++]));
            }
            p++;
        } else {
            while (*p >= '0' && *p <= '9') {
                if (si < 60) spec[si++] = *p;
                p++;
            }
        }
        // precision
        if (*p == '.') {
            if (si < 60) spec[si++] = '.';
            p++;
            if (*p == '*') {
                if (ai < 6) {
                    si += std::snprintf(
                        spec + si, sizeof(spec) - si, "%d",
                        static_cast<int32_t>(args[ai++]));
                }
                p++;
            } else {
                while (*p >= '0' && *p <= '9') {
                    if (si < 60) spec[si++] = *p;
                    p++;
                }
            }
        }
        // length modifier (h, hh, l, ll — on 32-bit MIPS
        // long==int so 'l' is effectively ignored)
        if (*p == 'h') {
            if (si < 60) spec[si++] = *p; p++;
            if (*p == 'h') { if (si < 60) spec[si++] = *p; p++; }
        } else if (*p == 'l') {
            if (si < 60) spec[si++] = *p; p++;
            if (*p == 'l') { if (si < 60) spec[si++] = *p; p++; }
        }

        char conv = *p ? *p++ : '\0';
        if (si < 62) spec[si++] = conv;
        spec[si] = '\0';
        if (ai > 6) ai = 6;  // clamp

        char buf[1024];
        buf[0] = '\0';
        switch (conv) {
        case 's': {
            uint32_t sa = (ai < 6) ? args[ai++] : 0;
            const char* s = psp_read_cstr(rdram, sa);
            std::snprintf(buf, sizeof(buf), spec, s);
            break;
        }
        case 'd': case 'i': {
            int32_t v = (ai < 6)
                ? static_cast<int32_t>(args[ai++]) : 0;
            std::snprintf(buf, sizeof(buf), spec, v);
            break;
        }
        case 'u': case 'x': case 'X': case 'o': {
            uint32_t v = (ai < 6) ? args[ai++] : 0;
            std::snprintf(buf, sizeof(buf), spec, v);
            break;
        }
        case 'c': {
            int v = (ai < 6)
                ? static_cast<int>(args[ai++] & 0xFF) : 0;
            std::snprintf(buf, sizeof(buf), spec, v);
            break;
        }
        case 'p': {
            uint32_t v = (ai < 6) ? args[ai++] : 0;
            std::snprintf(buf, sizeof(buf), "0x%08x", v);
            break;
        }
        case 'n':
            // %n: store current position — skip for safety
            if (ai < 6) ai++;
            break;
        default:
            // Unknown conversion — emit literal
            buf[0] = conv; buf[1] = '\0';
            break;
        }

        int blen = static_cast<int>(std::strlen(buf));
        if (rlen + blen < static_cast<int>(sizeof(result))) {
            std::memcpy(result + rlen, buf, blen);
            rlen += blen;
        }
    }
    result[rlen] = '\0';

    // Write result to PSP memory
    uint32_t dest_off = dest_addr & PSP_ADDR_MASK;
    if (dest_off + rlen + 1 <= PSP_MEM_SIZE) {
        std::memcpy(rdram + dest_off, result, rlen + 1);
    }

    ctx->r[2] = static_cast<int32_t>(rlen);
}

/// HLE override for FUN_08827A50 — vtable-dispatch allocator.
///
/// The game's vtable allocator reads a vtable ptr from an allocator context
/// object stored in BSS (at 0x08A7B640). This vtable is normally populated by
/// a kernel PRX module before user_main, but our runtime doesn't load PRX
/// modules, so the vtable ptr is 0 (BSS-zero). When vtable_ptr==0, the
/// dispatch reads MEM_W(0+8)==0 and calls RECOMP_LOOKUP(0)==noop, treating
/// the SIZE value as the return "allocation result". This then cascades to
/// FUN_08860348(size_as_addr) which writes object headers at wrong PSP
/// addresses, corrupting node+8 pointers and ultimately causing idx passed
/// to FUN_0896A6A4 to be 0x000379D8 instead of 0x088379D8.
///
/// Fix: intercept FUN_08827A50(size, allocator_ctx) and allocate from our
/// bump allocator when the allocator_ctx vtable is zero (uninitialized).
/// Calling convention: r4=size, r5=allocator_ctx
static void hle_vtable_alloc(uint8_t* rdram, recomp_context* ctx) {
    uint32_t size = static_cast<uint32_t>(ctx->r[4]);
    uint32_t alloc_ctx = static_cast<uint32_t>(ctx->r[5]);
    // Check if the allocator context has a valid vtable pointer at [ctx+0].
    // If vtable_ptr == 0 the allocator was never initialized by PRX modules.
    uint32_t vtable_ptr = 0;
    if (alloc_ctx != 0) {
        uint32_t ctx_off = alloc_ctx & PSP_ADDR_MASK;
        if (ctx_off + 4 <= PSP_MEM_SIZE) {
            std::memcpy(&vtable_ptr, rdram + ctx_off, sizeof(uint32_t));
        }
    }

    if (vtable_ptr == 0 && !psp_cleanroom()) {
        // Uninitialised allocator — use our bump allocator instead.
        // Align to 32 bytes (matches the alignment=32 the vtable path uses).
        uint32_t aligned = (size + 31U) & ~31U;

        if (aligned == 0) aligned = 32;  // clamp degenerate zero-size allocs

        uint32_t result = psp_kmem_bump_alloc(aligned);  // #47 P5 seam
        if (result == 0) {
            std::fprintf(stderr,
                "[HLE] hle_vtable_alloc: OUT OF MEMORY (size=%u)\n", size);
            ctx->r[2] = 0;
            return;
        }

        // Zero the allocated block so embedded vtables / counts start clean.
        uint32_t off = result & PSP_ADDR_MASK;
        std::memset(rdram + off, 0, aligned);

        std::fprintf(stderr,
            "[HLE] hle_vtable_alloc: BSS allocator ctx=0x%08X size=%u "
            "-> 0x%08X (vtable was null)\n",
            alloc_ctx, size, result);

        ctx->r[2] = static_cast<int32_t>(result);
        return;
    }

    // Allocator context has a valid vtable — fall through to the real
    // recompiled implementation.
    call_guest(s_FUN_08827A50, 0x08827A50U, rdram, ctx);

}

/// Mid-entry wrapper for 0x08827EA0 in FUN_08827E7C.
/// Sets entry_point so the parent dispatches to L_08827EA0.
static void hle_mid_08827EA0(uint8_t* rdram, recomp_context* ctx) {
    ctx->entry_point = 0x08827EA0U;
    call_guest(s_FUN_08827E7C, 0x08827E7CU, rdram, ctx);
    ctx->entry_point = 0;
}

/// Mid-entry wrapper for 0x08827EB0 in FUN_08827E7C.
/// Sets entry_point so the parent dispatches to L_08827EB0 (function body).
static void hle_mid_08827EB0(uint8_t* rdram, recomp_context* ctx) {
    ctx->entry_point = 0x08827EB0U;
    call_guest(s_FUN_08827E7C, 0x08827E7CU, rdram, ctx);
    ctx->entry_point = 0;
}

/// Mid-entry wrapper for 0x08827F9C — now inside the coalesce owner FUN_08827E7C
/// (FUN_08827F7C was merged into it). Dispatch to the owner via entry_point.
static void hle_mid_08827F9C(uint8_t* rdram, recomp_context* ctx) {
    ctx->entry_point = 0x08827F9CU;
    call_guest(s_FUN_08827E7C, 0x08827E7CU, rdram, ctx);
    ctx->entry_point = 0;
}

/// Initialize CRT memory function dispatch overrides.
/// Must be called after psp_init_dispatch_table().
// ---- Infinite loop breaker for intrusive list iteration ----
// FUN_089B4B68 is an iterator != comparison in the game engine.
// The list at obj+0xa864 should have 4 items. If the comparison
// is called >100 times without returning "equal", the list is
// corrupt (likely circular). Force "equal" to break the loop.
// (Original FUN_089b4b68 resolved into s_FUN_089B4B68 at boot — #47 P1.)

static void hle_list_iter_compare(
    uint8_t* rdram, recomp_context* ctx
) {
    static thread_local int iter_count = 0;

    // Save iter_a and iter_b BEFORE calling real function
    // r[4] = &iter_a (stack slot holding current node ptr)
    // r[5] = &iter_b (stack slot holding end sentinel ptr)
    uint32_t iter_a_ptr = static_cast<uint32_t>(ctx->r[4]);
    uint32_t iter_b_ptr = static_cast<uint32_t>(ctx->r[5]);
    uint32_t node_a = psp_mem_read<uint32_t>(rdram, iter_a_ptr);  // current node
    uint32_t node_b = psp_mem_read<uint32_t>(rdram, iter_b_ptr);  // end sentinel

    // [PRISTINE] One-shot caller+sentinel dump BEFORE any repair mutation, so we
    // see the list's TRUE first-iteration state and who iterates it. Fires
    // regardless of the band-aid gate below. Diagnostic.
    if (std::getenv("PSPRECOMP_PUSH_TRACE") != nullptr) {
        static std::unordered_set<uint32_t> seen_sent;
        if (seen_sent.find(node_b) == seen_sent.end()
                && node_b >= 0x08000000u && node_b < 0x0A000000u) {
            seen_sent.insert(node_b);
            uint32_t sn = psp_mem_read<uint32_t>(rdram, node_b + 0);
            uint32_t sp = psp_mem_read<uint32_t>(rdram, node_b + 4);
            std::fprintf(stderr,
                "[PRISTINE] sentinel=0x%08X (engine~0x%08X) node_a=0x%08X "
                "sent.next=0x%08X sent.prev=0x%08X caller(prev)=0x%08X\n",
                node_b, node_b - 0xa864u, node_a, sn, sp, g_prev_func_addr);
            // Walk the forward (next @ +0) chain from sentinel.next; the loop
            // terminates when a node == sentinel.  Detect non-termination /
            // cycles that don't pass through the sentinel.
            uint32_t cur = sn;
            std::unordered_set<uint32_t> vis;
            for (int i = 0; i < 64; i++) {
                if (cur == node_b) { std::fprintf(stderr,
                    "[CHAIN] step %d -> SENTINEL (clean end)\n", i); break; }
                if (cur < 0x08000000u || cur >= 0x0A000000u) {
                    std::fprintf(stderr,
                        "[CHAIN] step %d node=0x%08X OUT-OF-RANGE\n", i, cur);
                    break;
                }
                if (vis.count(cur)) { std::fprintf(stderr,
                    "[CHAIN] step %d node=0x%08X CYCLE (revisited, never hit "
                    "sentinel)\n", i, cur); break; }
                vis.insert(cur);
                uint32_t nx = psp_mem_read<uint32_t>(rdram, cur + 0);
                uint32_t pv = psp_mem_read<uint32_t>(rdram, cur + 4);
                uint32_t obj = psp_mem_read<uint32_t>(rdram, cur + 8);
                std::fprintf(stderr,
                    "[CHAIN] step %d node=0x%08X next=0x%08X prev=0x%08X "
                    "obj(+8)=0x%08X\n", i, cur, nx, pv, obj);
                cur = nx;
            }
        }
    }

    // Diagnostic gate: PSPRECOMP_LISTLOOP_OFF=1 (or PSPRECOMP_CLEANROOM=1)
    // disables the loop-breaker/repair band-aid so the true unmasked iteration
    // behavior can be observed.
    if (psp_cleanroom() || std::getenv("PSPRECOMP_LISTLOOP_OFF") != nullptr) {
        // [ITER_PROBE] log the current node ptr (*iter_a) each comparison so we
        // can see whether the iterator stays inside the 4-node chain or wanders
        // off (stack/iterator corruption) -> non-termination.
        if (std::getenv("PSPRECOMP_ITER_PROBE") != nullptr) {
            static thread_local int probe_n = 0;
            // Flag the first BAD comparison: end value lost its high bits
            // (0x0000A864 = sentinel offset w/o engine base) -> corruption.
            bool bad = (node_b < 0x08000000u);
            static thread_local bool fired_bad = false;
            if (++probe_n <= 80 || (bad && !fired_bad))
                std::fprintf(stderr,
                    "[ITER_PROBE] #%d cur(*a)=0x%08X end(*b)=0x%08X "
                    "iter_a_ptr=0x%08X last_fn=0x%08X prev_fn=0x%08X%s\n",
                    probe_n, node_a, node_b, iter_a_ptr,
                    g_last_func_addr, g_prev_func_addr,
                    bad ? " <<< BAD (end hi-bits stripped)" : "");
            if (bad) fired_bad = true;
        }
        call_guest(s_FUN_089B4B68, 0x089B4B68U, rdram, ctx);
        return;
    }

    // Call the real function
    call_guest(s_FUN_089B4B68, 0x089B4B68U, rdram, ctx);

    if (ctx->r[2] == 0) {
        // Equal (loop would end) — reset counter
        iter_count = 0;
    } else if (node_a == 0u) {
        // NULL current node — the chain has a broken forward link.
        // The sentinel's backward chain (prev pointers) may still be
        // intact. Attempt to repair by walking backward from sentinel.prev
        // to find the node whose prev points to the broken head, then
        // link that node's next chain properly.
        //
        // Only attempt repair once per sentinel to avoid repeated work.
        static std::unordered_set<uint32_t> repaired_sentinels;
        bool just_repaired = false;
        uint32_t sentinel = node_b;
        if (repaired_sentinels.find(sentinel) == repaired_sentinels.end()
                && sentinel >= 0x08000000u && sentinel < 0x0A000000u) {
            repaired_sentinels.insert(sentinel);
            uint32_t first_node = psp_mem_read<uint32_t>(rdram, sentinel + 0);
            // Walk backward from sentinel.prev using a visited set to detect cycles
            std::vector<uint32_t> backward_nodes;
            std::unordered_set<uint32_t> visited;
            visited.insert(sentinel);
            visited.insert(first_node);  // don't revisit first_node in backward walk
            uint32_t walk = psp_mem_read<uint32_t>(rdram, sentinel + 4); // sentinel.prev
            while (walk != sentinel && walk != 0u
                    && walk >= 0x08000000u && walk < 0x0A000000u
                    && visited.find(walk) == visited.end()) {
                backward_nodes.push_back(walk);
                visited.insert(walk);
                walk = psp_mem_read<uint32_t>(rdram, walk + 4); // node.prev
            }
            if (!backward_nodes.empty() && first_node != 0u
                    && first_node >= 0x08000000u) {
                // backward_nodes[] = [last, second-to-last, ..., second] (tail-first)
                // Reverse to get forward order: [second, ..., last]
                std::reverse(backward_nodes.begin(), backward_nodes.end());
                // Full chain: sentinel -> first_node -> second -> ... -> last -> sentinel
                std::vector<uint32_t> full_chain;
                full_chain.push_back(first_node);
                full_chain.insert(full_chain.end(),
                    backward_nodes.begin(), backward_nodes.end());
                // Rebuild ALL next/prev pointers
                for (size_t k = 0; k < full_chain.size(); k++) {
                    uint32_t cur  = full_chain[k];
                    uint32_t next_ptr = (k + 1 < full_chain.size())
                        ? full_chain[k + 1] : sentinel;
                    uint32_t prev_ptr = (k > 0)
                        ? full_chain[k - 1] : sentinel;
                    if (cur >= 0x08000000u && cur < 0x0A000000u) {
                        psp_mem_write<uint32_t>(rdram, cur + 0, next_ptr);
                        psp_mem_write<uint32_t>(rdram, cur + 4, prev_ptr);
                    }
                }
                psp_mem_write<uint32_t>(rdram, sentinel + 0, first_node);
                psp_mem_write<uint32_t>(rdram, sentinel + 4, full_chain.back());
                std::fprintf(stderr,
                    "[LISTLOOP] Rebuilt list sentinel=0x%08X: "
                    "%zu nodes (first=0x%08X last=0x%08X)\n",
                    sentinel, full_chain.size(),
                    full_chain.front(), full_chain.back());
                just_repaired = true;
            } else {
                std::fprintf(stderr,
                    "[LISTLOOP] NULL node sentinel=0x%08X: "
                    "could not repair (backward=%zu first=0x%08X)\n",
                    sentinel, backward_nodes.size(), first_node);
            }
        }
        if (!just_repaired) {
            // Could not repair or already tried — terminate this iteration
            ctx->r[2] = 0;
        } else {
            // Re-run the comparison now that the list is repaired
            call_guest(s_FUN_089B4B68, 0x089B4B68U, rdram, ctx);
        }
        iter_count = 0;
    } else {
        iter_count++;
        // Log on first detection and at milestones
        if (iter_count == 1) {
            // On first bad iteration: dump sentinel node content
            // node_b is the end sentinel address; its next field is at
            // the same address (sentinel.next == &sentinel when empty)
            uint32_t sentinel = node_b;
            uint32_t s_next = (sentinel >= 0x08000000u && sentinel < 0x0A000000u)
                ? psp_mem_read<uint32_t>(rdram, sentinel + 0) : 0xBADu;
            uint32_t s_prev = (sentinel >= 0x08000000u && sentinel < 0x0A000000u)
                ? psp_mem_read<uint32_t>(rdram, sentinel + 4) : 0xBADu;
            std::fprintf(stderr,
                "[LISTLOOP] BEGIN: node_a=0x%08X sentinel=0x%08X "
                "sentinel.next=0x%08X sentinel.prev=0x%08X\n",
                node_a, sentinel, s_next, s_prev);

            // [A864_DUMP] one-shot full forward-walk for ANY broken sentinel,
            // to compare list structure vs PPSSPP ground truth (4-node clean).
            {
                static std::unordered_set<uint32_t> dumped;
                if (dumped.find(sentinel) == dumped.end()
                        && sentinel >= 0x08000000u && sentinel < 0x0A000000u) {
                    dumped.insert(sentinel);
                    uint32_t w = s_next; int st = 0;
                    std::unordered_set<uint32_t> vis;
                    std::fprintf(stderr,
                        "[A864_DUMP] walk sentinel=0x%08X (engine~0x%08X):\n",
                        sentinel, sentinel - 0xa864u);
                    while (w != sentinel && w >= 0x08000000u && w < 0x0A000000u
                            && st < 16 && vis.find(w) == vis.end()) {
                        vis.insert(w);
                        uint32_t nx = psp_mem_read<uint32_t>(rdram, w + 0);
                        uint32_t pv = psp_mem_read<uint32_t>(rdram, w + 4);
                        uint32_t pl = psp_mem_read<uint32_t>(rdram, w + 8);
                        std::fprintf(stderr,
                            "[A864_DUMP]   node[%d]=0x%08X next=0x%08X "
                            "prev=0x%08X payload=0x%08X\n", st, w, nx, pv, pl);
                        w = nx; st++;
                    }
                    std::fprintf(stderr,
                        "[A864_DUMP]   -> %s after %d nodes (last=0x%08X)\n",
                        (w == sentinel ? "TERMINATES" : "BROKEN"), st, w);
                }
            }

            // Only trace for the problematic sentinel 0x09149164
            if (sentinel == 0x09149164u && node_a != 0u) {
                // Walk the chain manually to find the broken link
                uint32_t walk = node_a;
                int walk_steps = 0;
                std::fprintf(stderr,
                    "[LISTLOOP] Tracing chain from 0x%08X (sentinel=0x%08X):\n",
                    walk, sentinel);
                while (walk != 0u && walk != sentinel && walk_steps < 20) {
                    uint32_t next_node = (walk >= 0x08000000u && walk < 0x0A000000u)
                        ? psp_mem_read<uint32_t>(rdram, walk + 0) : 0xBADu;
                    uint32_t prev_node = (walk >= 0x08000000u && walk < 0x0A000000u)
                        ? psp_mem_read<uint32_t>(rdram, walk + 4) : 0xBADu;
                    std::fprintf(stderr,
                        "[LISTLOOP]   [%d] node=0x%08X next=0x%08X prev=0x%08X\n",
                        walk_steps, walk, next_node, prev_node);
                    walk = next_node;
                    walk_steps++;
                }
                if (walk == 0u) {
                    std::fprintf(stderr,
                        "[LISTLOOP]   -> chain ends with NULL at step %d!\n",
                        walk_steps);
                } else if (walk == sentinel) {
                    std::fprintf(stderr,
                        "[LISTLOOP]   -> chain properly terminates at step %d\n",
                        walk_steps);
                }
            }
        }
        if (iter_count == 10 || iter_count == 100 || iter_count == 1000
                || iter_count == 10000) {
            std::fprintf(stderr,
                "[LISTLOOP] iter=%d node_a=0x%08X end_b=0x%08X "
                "node_a->next=0x%08X\n",
                iter_count, node_a, node_b,
                (node_a >= 0x08000000u && node_a < 0x0A000000u)
                    ? psp_mem_read<uint32_t>(rdram, node_a) : 0xDEADBEEFu);
        }
        if (iter_count > 2000) {
            // Cap at 2000 — enough to identify a cycle, not enough to
            // stall the render thread for seconds.
            // Log final state on first hit.
            static bool warned = false;
            if (!warned) {
                std::fprintf(stderr,
                    "[LISTLOOP] Circular list detected: "
                    "iter=%d node_a=0x%08X end_b=0x%08X breaking\n",
                    iter_count, node_a, node_b);
                warned = true;
            }
            ctx->r[2] = 0;  // Force "equal" to break loop
            iter_count = 0;
        }
    }
}

// Diagnostic wrapper for FUN_089b4cec (subsystem-list push). Gated by
// PSPRECOMP_PUSH_TRACE. Logs, for the +0xa864 list, the list state BEFORE and
// AFTER the push: free-pool head ptr (list+8), the *(list+8) first-free node,
// the node that gets popped, and the payload (*a1). This captures the list at
// CONSTRUCTION TIME (before any per-frame iteration / band-aid hook runs), so
// we can compare against PPSSPP's clean 4-node build. Pure diagnostic.
// Diagnostic wrapper for FUN_089b4420 (subsystem-list+pool ctor). Logs the
// obj base, the resulting sentinel (obj+0x314), the node-block base (obj[0]),
// the count, and the free-pool head. Gated by PSPRECOMP_PUSH_TRACE.
static void hle_listctor_trace(uint8_t* rdram, recomp_context* ctx) {
    if (std::getenv("PSPRECOMP_PUSH_TRACE") == nullptr) {
        call_guest(s_FUN_089B4420, 0x089B4420U, rdram, ctx);
        return;
    }
    uint32_t obj = static_cast<uint32_t>(ctx->r[4]);
    call_guest(s_FUN_089B4420, 0x089B4420U, rdram, ctx);
    uint32_t sentinel = obj + 0x314u;
    uint32_t node_block = psp_mem_read<uint32_t>(rdram, obj + 0);
    uint32_t s_next = psp_mem_read<uint32_t>(rdram, sentinel + 0);
    uint32_t s_prev = psp_mem_read<uint32_t>(rdram, sentinel + 4);
    uint32_t pool_head = psp_mem_read<uint32_t>(rdram, sentinel + 8);
    uint32_t last_node = psp_mem_read<uint32_t>(rdram, obj + 4);
    std::fprintf(stderr,
        "[LISTCTOR] obj=0x%08X sentinel=0x%08X nodeBlock=0x%08X lastNode=0x%08X "
        "sent.next=0x%08X sent.prev=0x%08X poolHead(*sent+8)=0x%08X\n",
        obj, sentinel, node_block, last_node, s_next, s_prev, pool_head);
}

static void hle_push_trace(uint8_t* rdram, recomp_context* ctx) {
    if (std::getenv("PSPRECOMP_PUSH_TRACE") == nullptr) {
        call_guest(s_FUN_089B4CEC, 0x089B4CECU, rdram, ctx);
        return;
    }
    uint32_t list = static_cast<uint32_t>(ctx->r[4]);
    uint32_t payload_ptr = static_cast<uint32_t>(ctx->r[5]);
    uint32_t payload = (payload_ptr >= 0x08000000u && payload_ptr < 0x0A000000u)
        ? psp_mem_read<uint32_t>(rdram, payload_ptr) : 0xBADu;
    uint32_t pool_head_ptr = psp_mem_read<uint32_t>(rdram, list + 8);  // *(list+8)
    uint32_t first_free = (pool_head_ptr >= 0x08000000u && pool_head_ptr < 0x0A000000u)
        ? psp_mem_read<uint32_t>(rdram, pool_head_ptr) : 0xBADu;
    uint32_t pre_next = psp_mem_read<uint32_t>(rdram, list + 0);
    uint32_t pre_prev = psp_mem_read<uint32_t>(rdram, list + 4);
    uint32_t pre_count = psp_mem_read<uint32_t>(rdram, list + 0xc);
    std::fprintf(stderr,
        "[PUSH_TRACE] list=0x%08X PRE next=0x%08X prev=0x%08X count=%u "
        "poolHeadPtr=0x%08X firstFree=0x%08X payload=0x%08X\n",
        list, pre_next, pre_prev, pre_count, pool_head_ptr, first_free, payload);
    call_guest(s_FUN_089B4CEC, 0x089B4CECU, rdram, ctx);
    uint32_t post_next = psp_mem_read<uint32_t>(rdram, list + 0);
    uint32_t post_prev = psp_mem_read<uint32_t>(rdram, list + 4);
    uint32_t post_count = psp_mem_read<uint32_t>(rdram, list + 0xc);
    uint32_t new_node = post_prev;  // pushed at tail (list+4)
    uint32_t nn_next = (new_node >= 0x08000000u && new_node < 0x0A000000u)
        ? psp_mem_read<uint32_t>(rdram, new_node + 0) : 0xBADu;
    uint32_t nn_pl = (new_node >= 0x08000000u && new_node < 0x0A000000u)
        ? psp_mem_read<uint32_t>(rdram, new_node + 8) : 0xBADu;
    std::fprintf(stderr,
        "[PUSH_TRACE] list=0x%08X POST next=0x%08X prev=0x%08X count=%u "
        "newNode=0x%08X newNode.next=0x%08X newNode.payload=0x%08X\n",
        list, post_next, post_prev, post_count, new_node, nn_next, nn_pl);
}

// ---- Debug hook + fix for FUN_088623E0 (GE gatekeeper) ----
// Logs r[4] (base), r[6] (idx/key), r[5] (secondary), caller, then forwards.
//
// FIX: When the idx (r6) and object-area args (r8, r10) are corrupt (from
// r17 being clobbered by the vtable callee at L_0885FFB4), reconstruct the
// correct values from g_current_obj_sm — the render object address saved
// by hle_debug_0885FE90 before it called FUN_0885fe90.
//
// FUN_0885FE90 sets r17=a0 at entry, then the vtable call corrupts r17.
// GE_GATE args: r6=r17+108, r8=r17+176, r10=r17+228.
// Correct values: g_current_obj_sm + those offsets.
//
// We detect corruption via r6: if r6 < 0x08000000 but
// g_current_obj_sm >= 0x08000000, r17 was clobbered — use the saved object.
extern thread_local uint32_t g_last_func_addr;
extern thread_local uint32_t g_prev_func_addr;

// Thread-local render object saved by hle_debug_0885FE90 before dispatching
// into FUN_0885fe90. Used to reconstruct correct GE_GATE args when the
// vtable callee corrupts r17 (callee-saved register).
thread_local uint32_t g_current_obj_sm = 0;

static int g_ge_gate_call_count = 0;
// NOTE: external linkage (was `static`) since Phase 11.2 — the consolidated
// gatekeeper wrapper at runtime/src/main.cpp:165-207 calls this function
// directly (FORM 2 consolidation per 11.2-PATTERNS.md Pattern F). The
// forward declaration lives in runtime/include/hle/psp_hle.h. Do NOT
// re-register this function in psp_crt_override_init below — main.cpp owns
// the single registration site for 0x088623E0 as of Phase 11.2.
void hle_debug_088623E0(uint8_t* rdram, recomp_context* ctx) {
    uint32_t base = static_cast<uint32_t>(ctx->r[4]);
    uint32_t idx  = static_cast<uint32_t>(ctx->r[6]);
    uint32_t r5   = static_cast<uint32_t>(ctx->r[5]);
    uint32_t r7   = static_cast<uint32_t>(ctx->r[7]);
    uint32_t r17  = static_cast<uint32_t>(ctx->r[17]);
    int call_no   = ++g_ge_gate_call_count;

    // Fix args derived from corrupt r17 and r16.
    // The vtable callee at L_0885FFB4 corrupts both callee-saved registers:
    //   r17 (s1) = render object   -> used to compute r6/r8/r10
    //   r16 (s0) = render_queue_obj -> used as r4 (base for GE_GATE)
    //
    // Correct values:
    //   r4 (base) = render_queue_obj = MEM_W(0x08A87DD4) set by FUN_088608F0
    //   r6 = render_obj + 108  (from g_current_obj_sm)
    //   r8 = render_obj + 176
    //   r10 = render_obj + 228
    //
    // We detect corruption in r6 (idx): if idx < 0x08000000 but
    // g_current_obj_sm >= 0x08800000, r17 was clobbered.
    // We detect corruption in r4 (base): if base < 0x08800000 (too low
    // for a heap render_queue_obj) but the global says otherwise, r16 was clobbered.
    //
    // FUN_088608F0 stores the render queue object at [0x08A80000 + 32212]:
    static constexpr uint32_t RQ_GLOBAL_ADDR = 0x08A80000U + 32212U;  // 0x08A87DD4

    bool suspicious_idx  = (idx  > 0 && idx  < 0x08000000U);

    // Always reconstruct r4 (render_queue_obj) from the authoritative global.
    // FUN_088608F0 initializes this global, and FUN_0886095C reads it.
    // The vtable callee at L_0885FFB4 corrupts r16 (s0), which is the
    // register holding the render_queue_obj by the time GE_GATE is called.
    // The corrupted value may still be >= 0x08000000 (e.g. points to .rodata),
    // so range-checking doesn't reliably detect it.
    // Instead, always use the global value which is always set by FUN_088608F0.
    if (!psp_cleanroom()) {
        uint32_t correct_base = psp_mem_read<uint32_t>(rdram, RQ_GLOBAL_ADDR);
        if (correct_base >= 0x08800000U && correct_base != base) {
            static int rq_fix_count = 0;
            rq_fix_count++;
            if (rq_fix_count <= 10) {
                std::fprintf(stderr,
                    "[GE_BASE_FIX#%d] base=0x%08X->0x%08X (from global 0x%08X)\n",
                    rq_fix_count, base, correct_base, RQ_GLOBAL_ADDR);
            }
            ctx->r[4] = static_cast<int32_t>(correct_base);
            base = correct_base;
        }
    }

    // Reconstruct r6/r8/r10 from saved render object if corrupt
    if (!psp_cleanroom() && suspicious_idx && g_current_obj_sm >= 0x08800000U) {
        static int ggate_fix_count = 0;
        ggate_fix_count++;
        uint32_t obj = g_current_obj_sm;
        uint32_t fixed_r6  = obj + 108U;
        uint32_t fixed_r8  = obj + 176U;
        uint32_t fixed_r10 = obj + 228U;
        if (ggate_fix_count <= 20) {
            std::fprintf(stderr,
                "[GE_GATE_FIX#%d] obj=0x%08X r6=0x%08X->0x%08X "
                "r8=0x%08X->0x%08X r10=0x%08X->0x%08X caller=0x%08X\n",
                ggate_fix_count, obj,
                idx, fixed_r6,
                static_cast<uint32_t>(ctx->r[8]), fixed_r8,
                static_cast<uint32_t>(ctx->r[10]), fixed_r10,
                g_last_func_addr);
        }
        ctx->r[6]  = static_cast<int32_t>(fixed_r6);
        ctx->r[8]  = static_cast<int32_t>(fixed_r8);
        ctx->r[10] = static_cast<int32_t>(fixed_r10);
        idx = fixed_r6;
        suspicious_idx = false;
    }

    bool suspicious = suspicious_idx;
    if (call_no <= 5 || suspicious) {
        std::fprintf(stderr,
            "[GE_GATE#%d] base=0x%08X idx=0x%08X r17=0x%08X r5=0x%08X r7=0x%08X "
            "obj_sm=0x%08X caller=0x%08X prev=0x%08X%s\n",
            call_no, base, idx, r17, r5, r7,
            g_current_obj_sm, g_last_func_addr, g_prev_func_addr,
            suspicious ? " *** STILL SUSPICIOUS ***" : "");
    }

    // Forward to real function
    call_guest(s_FUN_088623E0, 0x088623E0U, rdram, ctx);
}

/// Debug hook for FUN_0885EEE8 — pool initializer.
/// Logs the base address passed in r4. The corrupt pointer 0x0003794C
/// (missing 0x08800000) should appear here when the bug manifests.
static int g_eee8_call_count = 0;
static void hle_debug_0885EEE8(uint8_t* rdram, recomp_context* ctx) {
    uint32_t base = static_cast<uint32_t>(ctx->r[4]);
    int call_no   = ++g_eee8_call_count;
    bool suspicious = (base != 0 && base < 0x08000000U);
    std::fprintf(stderr,
        "[POOL_INIT#%d] base=0x%08X caller=0x%08X%s\n",
        call_no, base, g_last_func_addr,
        suspicious ? " *** SUSPICIOUS BASE (missing 0x08800000?) ***" : "");
    call_guest(s_FUN_0885EEE8, 0x0885EEE8U, rdram, ctx);
}

/// Debug hook + module-base fix for FUN_0885FE90 — object state machine.
/// Logs the object pointer passed in r4 (stored as r17 inside).
///
/// ROOT CAUSE: the linked list nodes at (outer_obj + 0x9824) store object
/// pointers as rdram offsets (PSP_VA & 0x07FFFFFF) instead of full PSP VAs.
/// This happens because game init code stores these pointers without the
/// 0x08800000 module base, likely because the kernel PRX loader that would
/// perform the base-address fixup was never run.
///
/// FIX: if a0 is in the range 0x00004000..0x007FFFFF (plausible .text/.data
/// offset missing the module base), restore it by adding 0x08800000.
/// This lets FUN_0885FE90 operate on a valid PSP object address and the
/// downstream vtable dispatch in FUN_0895B598 reaches the real render
/// function instead of 0x0438.
// ── LOOSE-ABSENT REGION FIX ────────────────────────────────────────────────
// Why this exists (oracle-verified, see handoff 09 + this session's [FE90_OBJ]):
// FUN_0885fe90 is the per-asset state machine. The asset descriptor at obj+108
// holds the asset name; the object advances state 1→2→3 as it streams a region
// out of DATA_CMN.BND. The state-2 body (batch_0050.cpp:L_0885FF34) calls the
// loose-open router FUN_08861E28, then reads *(obj+176). For a "loose idx=-1"
// asset (SYSTEMDATA) the router returns the region end-offset, *(obj+176) is set
// to the region size (227692), and the machine proceeds to enqueue the region
// read (FUN_088623E0 @ L_08860018) and sets state=3.
//
// For an asset whose loose-group file is ABSENT on the retail disc
// (SYSTEMLOCALIZEDATA: idx=0, *(desc+48)=1 → opens disc0:LOADINGGROUP/
// SYSTEMLOCALIZEDATA.BND → ENOENT), the router spins/returns without populating
// *(obj+176). The machine then loops at state 2 forever (~97×, observed via
// [FE90_OBJ]: st=2, +176=0, +104=-1 every re-dispatch) and the region read is
// never enqueued → no geometry.
//
// The asset registry (manager+4680 == 0x08A89050, looked up by FUN_0896A6A4)
// DOES resolve the name → entry with +4 = region size, +8 = region offset
// (verified: systemlocalizedata → 0x08AC8420, +4=1091291, +8=0x50800). So the
// region size IS available. FIX: when an object is stuck at state 2 with
// *(obj+176)==0, resolve its name through the registry and seed *(obj+176) with
// the registry's region size, exactly as the working (idx=-1) path does. This
// is general: any asset whose loose-group file is absent but which resolves to a
// region in the registry falls through to its region read.
//
// Heap addresses are stable across runs (handoff 09); 0x08A89050 is the manager
// lookup object (manager 0x08A87E08 + 0x1248). We replay FUN_0896A6A4 on an
// isolated scratch stack (BND-arena gap) so its stack writes can't corrupt live
// state — the same proven technique as the [E28_TRACE] probe.
// NOTE: default OFF. Seeding *(obj+176) alone is INSUFFICIENT and HARMFUL:
// verified ([FE90_OBJ]) that with +176 seeded the state-2 body routes into the
// FUN_0885DD94 region-read enqueue, which RETURNS 0 → early return at
// L_0885FF88 → object stays at state 2 → next re-dispatch re-enqueues the same
// region read (observed: 64× re-reads of the 1091291 region, leaking one fd
// each). The working asset reaches state 3 in a SINGLE state-2 visit via a
// different sub-path; +176 is a derived field, not the gate. Kept env-gated
// (PSPRECOMP_LOOSE_REGION_SEED=1) for further investigation; OFF by default so
// it does not introduce the re-read loop. See the report for the true root.
static bool g_loose_region_fix_enabled = false;  // opt-in only — see note

// Resolve a registry region size for the asset named at obj+108. Returns the
// region size (>0) if the name resolves to a valid region entry, else 0.
static uint32_t loose_region_size_from_registry(uint8_t* rdram, uint32_t obj) {
    uint32_t name_ptr = obj + 108;
    // Sanity: a printable, NUL-terminated name must exist at obj+108.
    bool ok = false;
    for (int i = 0; i < 95; i++) {
        char c = (char)psp_mem_read<uint8_t>(rdram, (name_ptr + i) & 0x07FFFFFFU);
        if (c == 0) { ok = (i > 0); break; }
        if (c < 0x20 || c > 0x7E) { ok = false; break; }
    }
    if (!ok) return 0;

    static constexpr uint32_t MANAGER_LOOKUP = 0x08A89050U;  // manager+4680
    // Build an isolated probe ctx (do NOT touch the live ctx).
    recomp_context p{};
    p.r[29] = 0x09F00000;            // isolated scratch sp (BND-arena gap)
    p.r[28] = MANAGER_LOOKUP;        // gp-ish; harmless for this lookup
    p.r[4]  = (int32_t)MANAGER_LOOKUP;
    p.r[5]  = (int32_t)name_ptr;
    FuncPtr lk = RECOMP_LOOKUP(0x0896A6A4);
    if (!lk) return 0;
    lk(rdram, &p);
    uint32_t entry = (uint32_t)p.r[2];
    if (entry < 0x08000000U || entry >= 0x0A000000U) return 0;
    int32_t size = (int32_t)psp_mem_read<uint32_t>(rdram, (entry + 4) & 0x07FFFFFFU);
    if (size <= 0) return 0;          // not a valid region entry
    return (uint32_t)size;
}

static int g_fe90_call_count = 0;
static void hle_debug_0885FE90(uint8_t* rdram, recomp_context* ctx) {
    uint32_t obj  = static_cast<uint32_t>(ctx->r[4]);
    int call_no   = ++g_fe90_call_count;

    // [D1_TRACE] cleanroom-safe diagnostic: the per-asset SM. Logs obj, state
    // (+172), the asset name (+108), and result-of-last-resolve (+104) on
    // ENTRY, plus state/+104 on EXIT. Read-only — no behavior change.
    const bool d1_trace = std::getenv("PSPRECOMP_D1_TRACE") != nullptr;
    auto d1_name = [&](uint32_t o) -> std::string {
        std::string s;
        if (o < 0x08000000U || o >= 0x0A000000U) return s;
        for (int i = 0; i < 80; i++) {
            char c = (char)psp_mem_read<uint8_t>(rdram, (o + 108 + i) & 0x07FFFFFFU);
            if (c == 0) break;
            if (c < 0x20 || c > 0x7E) { s += '?'; } else { s += c; }
        }
        return s;
    };
    if (d1_trace && obj >= 0x08000000U && obj < 0x0A000000U) {
        uint32_t st  = psp_mem_read<uint32_t>(rdram, (obj + 172) & 0x07FFFFFFU);
        int32_t  r104 = (int32_t)psp_mem_read<uint32_t>(rdram, (obj + 104) & 0x07FFFFFFU);
        uint32_t f176 = psp_mem_read<uint32_t>(rdram, (obj + 176) & 0x07FFFFFFU);
        std::fprintf(stderr,
            "[D1_TRACE>] #%d obj=0x%08X state=%u +104=%d +176=%u name=\"%s\" caller=0x%08X\n",
            call_no, obj, st, r104, f176, d1_name(obj).c_str(), g_last_func_addr);
    }

    // Detect and fix truncated module-base pointer:
    // Range 0x00004000..0x007FFFFF covers the game's .text/.data
    // sections when the 0x08800000 module base is stripped.
    // Range 0x08000000+ is already a valid PSP VA — no fix needed.
    bool was_fixed = false;
    if (!psp_cleanroom() && obj >= 0x00004000U && obj < 0x00800000U) {
        uint32_t fixed = obj + 0x08800000U;
        static int fix_count = 0;
        fix_count++;
        if (fix_count <= 20) {
            std::fprintf(stderr,
                "[OBJ_SM_FIX#%d] a0=0x%08X -> 0x%08X "
                "(restored module base) caller=0x%08X\n",
                fix_count, obj, fixed, g_last_func_addr);
        }
        ctx->r[4] = static_cast<int32_t>(fixed);
        obj = fixed;
        was_fixed = true;
    }

    bool suspicious = (obj != 0 && obj < 0x08000000U);
    if (call_no <= 3 || suspicious || was_fixed) {
        std::fprintf(stderr,
            "[OBJ_SM#%d] obj=0x%08X caller=0x%08X%s\n",
            call_no, obj, g_last_func_addr,
            suspicious ? " *** STILL SUSPICIOUS AFTER FIX ***" : "");
    }
    // Save the render object so hle_debug_088623E0 can reconstruct correct
    // GE_GATE args if r17 gets clobbered by the vtable callee at L_0885FFB4.
    g_current_obj_sm = obj;

    // [FE90_OBJ] (default-OFF) — dump the asset object's sub-object chain that
    // FUN_0885fe90 dispatches through at L_0885FFB4 (the 0x438 crash site):
    //   this = *(obj+0); vtable = *(this+24); method = *(vtable+12); call.
    // The sharp target is *(obj+0) == obj+4 with vtable 0x08A44748 at sub+24.
    if (std::getenv("PSPRECOMP_FE90_OBJ") && obj >= 0x08000000U
            && obj < 0x0A000000U) {
        uint32_t b = obj & 0x07FFFFFFU;
        auto rd = [&](uint32_t o) {
            return *reinterpret_cast<uint32_t*>(rdram + ((obj + o) & 0x07FFFFFFU));
        };
        uint32_t state = rd(172);
        uint32_t sub   = rd(0);
        uint32_t vt = 0, meth = 0, subv24 = 0;
        if (sub >= 0x08000000U && sub < 0x0A000000U) {
            subv24 = *reinterpret_cast<uint32_t*>(
                rdram + ((sub + 24) & 0x07FFFFFFU));
            vt = subv24;
            if (vt >= 0x08000000U && vt < 0x0A000000U) {
                meth = *reinterpret_cast<uint32_t*>(
                    rdram + ((vt + 12) & 0x07FFFFFFU));
            }
        }
        static int fe90_obj_count = 0;
        if (++fe90_obj_count <= 120) {
            auto rdf = [&](uint32_t o) {
                return *reinterpret_cast<uint32_t*>(
                    rdram + ((obj + o) & 0x07FFFFFFU));
            };
            // Sub-object chain that gates L_0885FF60 / L_0885FF7C / state 2→3:
            //   sub = *(obj+0); s0 = *(sub+0); s16 = *(sub+16); s24 = *(sub+24)
            uint32_t s0 = 0, s16 = 0, s24 = 0;
            if (sub >= 0x08000000U && sub < 0x0A000000U) {
                s0  = *reinterpret_cast<uint32_t*>(rdram + ((sub + 0)  & 0x07FFFFFFU));
                s16 = *reinterpret_cast<uint32_t*>(rdram + ((sub + 16) & 0x07FFFFFFU));
                s24 = *reinterpret_cast<uint32_t*>(rdram + ((sub + 24) & 0x07FFFFFFU));
            }
            std::fprintf(stderr,
                "[FE90_OBJ] #%d obj=0x%08X st=%u +176=%d +180=%d +104=%d "
                "+184=%u meth=0x%08X *(obj+0)=0x%08X sub:+0=0x%08X +16=%d +24=0x%08X\n",
                fe90_obj_count, obj, state,
                (int32_t)rdf(176), (int32_t)rdf(180), (int32_t)rdf(104),
                rdf(184) & 0xFFu, meth, sub, s0, (int32_t)s16, s24);
        }
        (void)b; (void)subv24;
    }

    // ── LOOSE-ABSENT REGION FIX (default ON) ───────────────────────────────
    // If this object is at state 2 with *(obj+176)==0, it is the stuck loose-
    // absent asset. Seed *(obj+176) with the registry's region size so the
    // state-2 body proceeds to the region read exactly like the working asset.
    // Re-dispatch counting per object avoids touching the first benign visit.
    if (!psp_cleanroom()
            && (g_loose_region_fix_enabled
                || std::getenv("PSPRECOMP_LOOSE_REGION_SEED") != nullptr)
            && obj >= 0x08000000U && obj < 0x0A000000U) {
        uint32_t state = psp_mem_read<uint32_t>(rdram, (obj + 172) & 0x07FFFFFFU);
        uint32_t cur176 = psp_mem_read<uint32_t>(rdram, (obj + 176) & 0x07FFFFFFU);
        if (state == 2 && cur176 == 0) {
            static std::unordered_map<uint32_t,int> s_stuck;
            int n = ++s_stuck[obj];
            // Fire after a couple of re-dispatches so a normally-advancing
            // object (which sets +176 itself on its first state-2 visit) is
            // never disturbed; only a genuinely STUCK object reaches n>=2.
            if (n >= 2) {
                uint32_t rsize = loose_region_size_from_registry(rdram, obj);
                if (rsize != 0) {
                    psp_mem_write<uint32_t>(rdram, (obj + 176) & 0x07FFFFFFU, rsize);
                    static int seeded = 0;
                    if (++seeded <= 12) {
                        std::fprintf(stderr,
                            "[LOOSE_REGION_FIX] obj=0x%08X stuck@state2 (n=%d) "
                            "seeded *(obj+176)=%u from registry\n",
                            obj, n, rsize);
                    }
                }
            }
        }
    }

    uint32_t sp_before_fe90 = static_cast<uint32_t>(ctx->r[29]);
    call_guest(s_FUN_0885FE90, 0x0885FE90U, rdram, ctx);
    if (std::getenv("PSPRECOMP_CTX_PROBE") != nullptr) {
        uint32_t sp_after_fe90 = static_cast<uint32_t>(ctx->r[29]);
        if (sp_after_fe90 != sp_before_fe90) {
            std::fprintf(stderr,
                "[CTX_PROBE] FE90 SP IMBALANCE: pre=0x%08X post=0x%08X "
                "delta=%d obj=0x%08X\n",
                sp_before_fe90, sp_after_fe90,
                (int32_t)(sp_after_fe90 - sp_before_fe90), obj);
            std::fprintf(stderr, "[CTX_PROBE]   func-ring (oldest->newest):");
            for (int i = 0; i < 32; i++) {
                uint32_t e = g_func_ring[(g_func_ring_pos + i) & 31u];
                if (e) std::fprintf(stderr, " %08X", e);
            }
            std::fprintf(stderr, "\n");
        }
    }
    if (d1_trace && obj >= 0x08000000U && obj < 0x0A000000U) {
        uint32_t st  = psp_mem_read<uint32_t>(rdram, (obj + 172) & 0x07FFFFFFU);
        int32_t  r104 = (int32_t)psp_mem_read<uint32_t>(rdram, (obj + 104) & 0x07FFFFFFU);
        uint32_t f176 = psp_mem_read<uint32_t>(rdram, (obj + 176) & 0x07FFFFFFU);
        std::fprintf(stderr,
            "[D1_TRACE<] #%d obj=0x%08X state=%u +104=%d +176=%u name=\"%s\"\n",
            call_no, obj, st, r104, f176, d1_name(obj).c_str());
    }
    g_current_obj_sm = 0;
}

/// Debug hook + r17 restoration for FUN_08822D5C (strncpy).
/// Called just before GE_GATE in FUN_0885FE90. If r17 is corrupt (missing
/// module base), patch it here so GE_GATE receives a valid 'this' pointer.
///
/// The corruption of r17 happens inside FUN_0885FE90 at the vtable dispatch
/// on L_0885FFB4 (RECOMP_LOOKUP(ctx->r[25])). The callee corrupts r17
/// (callee-saved), most likely by using it as a scratch register without
/// saving it. Until the true culprit is patched, we fix r17 here as a
/// second defensive layer right before the GE_GATE call.
static int g_strncpy_call_count = 0;
static void hle_debug_08822D5C(uint8_t* rdram, recomp_context* ctx) {
    // [ROOT-FIX 2026-06-01] The "defensive r17 restoration" below is a band-aid
    // for the RENDER path (FUN_0885FE90 chain), where a vtable callee clobbers
    // callee-saved r17 before this strncpy. But FUN_08822D5C is GENERAL: the asset
    // registry lookup FUN_0896A6A4 calls strncpy with r17 = the path-separator
    // index (12, a legitimate small value). The old unconditional band-aid
    // clobbered THAT r17 to the render object, so the lookup's null-terminator
    // store (sp + r17 + 273) wrote to garbage instead of sp+285, leaving the
    // "loadinggroup/" directory key UN-terminated -> CRC32 (FUN_0888AFAC,
    // hash-until-null) over-read into stack garbage -> wrong bucket ->
    // systemlocalizedata MISS -> asset routed to dead loose path -> no geometry.
    // (Proven via headless lldb: r17=0xc before the call, 0x0913EA10 after.)
    // FIX: scope the band-aid to NOT fire when the caller is the registry lookup.
    void* caller = __builtin_return_address(0);
    uintptr_t cp  = reinterpret_cast<uintptr_t>(caller);
    // Host code-range of the registry lookup: [FUN_0896A6A4, FUN_0896AA60).
    // Uses the boot-resolved originals (#47 P1) — same host addresses the
    // former extern symbols had. nullptr (foreign game) disables the scope.
    uintptr_t lk0 = reinterpret_cast<uintptr_t>(s_FUN_0896A6A4);
    uintptr_t lk1 = reinterpret_cast<uintptr_t>(s_FUN_0896AA60);
    bool from_lookup = (lk0 != 0) && (lk1 > lk0) && (cp >= lk0) && (cp < lk1);

    uint32_t r17 = static_cast<uint32_t>(ctx->r[17]);
    int call_no  = ++g_strncpy_call_count;
    bool suspicious = (r17 != 0 && r17 < 0x08000000U);

    if (!from_lookup && !psp_cleanroom()) {
        if (suspicious && g_current_obj_sm >= 0x08800000U) {
            uint32_t fixed = g_current_obj_sm;
            static int sfix_count = 0;
            if (++sfix_count <= 20)
                std::fprintf(stderr,
                    "[STRNCPY_FIX#%d] r17=0x%08X -> 0x%08X (restored from obj_sm)\n",
                    sfix_count, r17, fixed);
            ctx->r[17] = static_cast<int32_t>(fixed); r17 = fixed; suspicious = false;
        } else if (r17 >= 0x00004000U && r17 < 0x00800000U) {
            uint32_t fixed = r17 + 0x08800000U;
            static int sfix2_count = 0;
            if (++sfix2_count <= 10)
                std::fprintf(stderr,
                    "[STRNCPY_FIX2#%d] r17=0x%08X -> 0x%08X (fallback: +module_base)\n",
                    sfix2_count, r17, fixed);
            ctx->r[17] = static_cast<int32_t>(fixed); r17 = fixed; suspicious = false;
        }
    }

    if (call_no <= 5 || (suspicious && !from_lookup)) {
        std::fprintf(stderr,
            "[STRNCPY#%d] r17=0x%08X r4=0x%08X r5=0x%08X r6=0x%08X from_lookup=%d\n",
            call_no, r17, static_cast<uint32_t>(ctx->r[4]),
            static_cast<uint32_t>(ctx->r[5]), static_cast<uint32_t>(ctx->r[6]),
            from_lookup ? 1 : 0);
    }
    call_guest(s_FUN_08822D5C, 0x08822D5CU, rdram, ctx);
}

// [DF0_PROBE] env PSPRECOMP_DF0_PROBE: log every entry into the engine frame-tick
// FUN_089b4df0 — capture a0 (engine ptr arg) and *a0 (engine base, -> r19).
// Detects the re-entrant call whose engine ptr is truncated to 0 (the gate).
static void hle_df0_probe(uint8_t* rdram, recomp_context* ctx) {
    if (std::getenv("PSPRECOMP_DF0_PROBE") != nullptr) {
        static thread_local int df0_n = 0;
        uint32_t a0 = static_cast<uint32_t>(ctx->r[4]);
        uint32_t glob = psp_mem_read<uint32_t>(rdram, 0x08A5B5A4u & 0x07FFFFFFu);
        uint32_t star_a0 = (a0 >= 0x08000000u && a0 < 0x0A000000u)
            ? psp_mem_read<uint32_t>(rdram, a0) : 0xBADBADu;
        std::fprintf(stderr,
            "[DF0_PROBE] #%d a0(engine_ptr)=0x%08X *a0(engine_base->r19)=0x%08X "
            "glob[0x08A5B5A4]=0x%08X sp=0x%08X ra(r31)=0x%08X prev_fn=0x%08X\n",
            ++df0_n, a0, star_a0, glob,
            static_cast<uint32_t>(ctx->r[29]),
            static_cast<uint32_t>(ctx->r[31]), g_prev_func_addr);
    }
    call_guest(s_FUN_089B4DF0, 0x089B4DF0U, rdram, ctx);
}

// [END_PROBE] env PSPRECOMP_END_PROBE: wrap FUN_089b440c (*r4 = r5, the std::list
// "set end" op). When r5 (the end value) has its high bits stripped (< 0x08000000),
// dump the caller (g_prev_func_addr) — that is the function that computed a
// truncated engine_base+0xa864 (the gate). Logs the first few hits then is quiet.
static void hle_end_probe(uint8_t* rdram, recomp_context* ctx) {
    if (std::getenv("PSPRECOMP_END_PROBE") != nullptr) {
        uint32_t v = static_cast<uint32_t>(ctx->r[5]);
        uint32_t dst = static_cast<uint32_t>(ctx->r[4]);
        static thread_local int bad_hits = 0;
        if (v != 0u && v < 0x08000000u && ++bad_hits <= 4) {
            std::fprintf(stderr,
                "[END_PROBE] TRUNCATED end value=0x%08X dst=0x%08X "
                "caller(prev_fn)=0x%08X last_fn=0x%08X sp=0x%08X\n",
                v, dst, g_prev_func_addr, g_last_func_addr,
                static_cast<uint32_t>(ctx->r[29]));
            // Dump the recent function-entry ring (oldest -> newest) for a backtrace.
            std::fprintf(stderr, "[END_PROBE]   ring:");
            for (int i = 0; i < 32; i++) {
                uint32_t e = g_func_ring[(g_func_ring_pos + i) & 31u];
                if (e) std::fprintf(stderr, " %08X", e);
            }
            std::fprintf(stderr, "\n");
        }
    }
    call_guest(s_FUN_089B440C, 0x089B440CU, rdram, ctx);
}

// [CTX_PROBE] env PSPRECOMP_CTX_PROBE: confirm the thread/ctx model on the
// GE-completion chain. Logs, per entry to EFC8 / FUN_088601C4 / FE90, WHICH OS
// thread runs it, WHICH PspThread (id/name) g_current points at, and WHICH
// recomp_context pointer the function was handed — and whether that ctx is the
// current thread's OWN ctx (&g_current->ctx). A "ctx != own" hit is the
// per-thread-ctx-model violation (a callback running on the wrong thread/ctx).
static void ctx_probe_log(const char* tag, uint8_t* rdram, recomp_context* ctx) {
    if (std::getenv("PSPRECOMP_CTX_PROBE") == nullptr) return;
    PspThread* t = psp_get_current_thread();
    void* own = t ? static_cast<void*>(&t->ctx) : nullptr;
    bool mismatch = (own != nullptr) && (own != static_cast<void*>(ctx));
    std::fprintf(stderr,
        "[CTX_PROBE] %s os_thr=%p psp_thr=%d(%s) prio=%d ctx=%p own_ctx=%p "
        "sp=0x%08X ra=0x%08X%s\n",
        tag,
        reinterpret_cast<void*>(pthread_self()),
        t ? t->id : -1,
        t ? t->name : "?",
        t ? t->priority : -1,
        static_cast<void*>(ctx),
        own,
        static_cast<uint32_t>(ctx->r[29]),
        static_cast<uint32_t>(ctx->r[31]),
        mismatch ? "   <<< CTX MISMATCH (wrong-thread callback)" : "");
    (void)rdram;
}

static void hle_ctx_probe_efc8(uint8_t* rdram, recomp_context* ctx) {
    ctx_probe_log("EFC8", rdram, ctx);
    call_guest(s_FUN_0885EFC8, 0x0885EFC8U, rdram, ctx);
}
static void hle_ctx_probe_601c4(uint8_t* rdram, recomp_context* ctx) {
    ctx_probe_log("601C4", rdram, ctx);
    call_guest(s_FUN_088601C4, 0x088601C4U, rdram, ctx);
}

void psp_crt_override_init() {
    // Override FUN_0881F280 (memmove)
    psp_dispatch_register(0x0881F280U,
        reinterpret_cast<FuncPtr>(hle_memmove));

    // Override FUN_0881F120 (memcpy)
    psp_dispatch_register(0x0881F120U,
        reinterpret_cast<FuncPtr>(hle_memcpy));

    // Override FUN_0881F5C0 (memset)
    psp_dispatch_register(0x0881F5C0U,
        reinterpret_cast<FuncPtr>(hle_memset));

    // Override FUN_088226BC (sprintf) — recompiled vfprintf
    // has a bug that prepends "0s" to each %s substitution
    psp_dispatch_register(0x088226BCU,
        reinterpret_cast<FuncPtr>(hle_sprintf));

    // Mid-entries in FUN_08827E7C — vtable dispatch trampoline.
    // 0x08827EA0: vtable dispatch without arg shuffle
    // 0x08827EB0: actual function body
    psp_dispatch_register(0x08827EA0U,
        reinterpret_cast<FuncPtr>(hle_mid_08827EA0));
    psp_dispatch_register(0x08827EB0U,
        reinterpret_cast<FuncPtr>(hle_mid_08827EB0));
    psp_dispatch_register(0x08827F9CU,
        reinterpret_cast<FuncPtr>(hle_mid_08827F9C));

    // Break infinite list iteration in game engine init
    psp_dispatch_register(0x089B4B68U,
        reinterpret_cast<FuncPtr>(hle_list_iter_compare));

    // Diagnostic-only: trace engine frame-tick entries (PSPRECOMP_DF0_PROBE=1).
    psp_dispatch_register(0x089B4DF0U,
        reinterpret_cast<FuncPtr>(hle_df0_probe));

    // Diagnostic-only: catch truncated std::list "end" writes (PSPRECOMP_END_PROBE=1).
    psp_dispatch_register(0x089B440CU,
        reinterpret_cast<FuncPtr>(hle_end_probe));

    // Diagnostic-only: confirm the thread/ctx model on the GE-completion chain
    // (PSPRECOMP_CTX_PROBE=1). Wrap EFC8 (loadinggroup driver) and FUN_088601C4
    // (GE-finish completion cb). Flags any entry whose ctx != g_current's own ctx.
    psp_dispatch_register(0x0885EFC8U,
        reinterpret_cast<FuncPtr>(hle_ctx_probe_efc8));
    psp_dispatch_register(0x088601C4U,
        reinterpret_cast<FuncPtr>(hle_ctx_probe_601c4));

    // Diagnostic-only: trace subsystem-list pushes (PSPRECOMP_PUSH_TRACE=1).
    psp_dispatch_register(0x089B4CECU,
        reinterpret_cast<FuncPtr>(hle_push_trace));
    psp_dispatch_register(0x089B4420U,
        reinterpret_cast<FuncPtr>(hle_listctor_trace));

    // Override FUN_08827A50: vtable-dispatch allocator.
    // When the BSS allocator context at 0x08A7B640 has vtable_ptr==0
    // (kernel PRX never loaded), fall back to our bump allocator so
    // FUN_089B4C88 gets a valid PSP address instead of the size value.
    psp_dispatch_register(0x08827A50U,
        reinterpret_cast<FuncPtr>(hle_vtable_alloc));

    // MOVED TO runtime/src/main.cpp:165-207 (Phase 11.2 consolidation, FORM 2).
    // The two pre-existing wrappers for FUN_088623E0 (this one + the [GK_GATE]
    // lambda in main.cpp) were unified into a single registration site at
    // main.cpp. Do NOT re-register here — the consolidated wrapper invokes
    // hle_debug_088623E0() directly via the forward decl in psp_hle.h, so
    // GE_BASE_FIX / GE_GATE_FIX behavior is preserved with single source of
    // truth (Pitfall 4 mitigated: exactly one psp_dispatch_register call for
    // 0x088623E0 exists in the runtime tree). See 11.2-PATTERNS.md Pattern F.

    // Debug hook for FUN_0885EEE8 (pool initializer) — traces base ptr.
    psp_dispatch_register(0x0885EEE8U,
        reinterpret_cast<FuncPtr>(hle_debug_0885EEE8));

    // Debug hook for FUN_0885FE90 (object state machine) — traces obj ptr.
    psp_dispatch_register(0x0885FE90U,
        reinterpret_cast<FuncPtr>(hle_debug_0885FE90));

    // Debug hook for FUN_08822D5C (strncpy called just before GE_GATE) — traces r17.
    psp_dispatch_register(0x08822D5CU,
        reinterpret_cast<FuncPtr>(hle_debug_08822D5C));

    std::fprintf(stderr,
        "[RT] CRT overrides registered "
        "(memmove, memcpy, memset, sprintf, "
        "mid_08827EA0, mid_08827EB0, list_iter_break, vtable_alloc, "
        "debug_ge_gate, debug_pool_init, debug_obj_sm)\n");
}
