#include "hle/psp_hle.h"
#include "hle/psp_hle_kernel.h"
#include "psp_memory.h"
#include "psp_scheduler.h"
#include "recomp.h"
#include "recomp_module.h"  // generated module facts (issue #47 Phase 2)

// Generated per-game choices (issues #46/#47 Phase 4): RECOMP_HEAP_OVERRIDE
// pins the heap base when a game's manifest sets [module] heap_base.
// Guarded so output dirs generated before Phase 4 still build.
#if __has_include("recomp_game_config.h")
#include "recomp_game_config.h"
#endif
#ifndef RECOMP_HEAP_OVERRIDE
#define RECOMP_HEAP_OVERRIDE 0x0U
#endif

#include <cstdio>
#include <cstring>
#include <mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <algorithm>
#include <pthread.h>


// ---- Heap Allocator ----
// Bump allocator for PSP user memory. Grows upward from the heap start.
// Heap-base policy (#47 P2, decision P11): the generated fact
// RECOMP_HEAP_BASE (analysis.json heap_base — first 64K-aligned address past
// the loaded image) aligned UP to 16 MB. Game-agnostic rule, no manifest pin;
// for Patapon the alignment lands exactly on the historical hardcoded heap
// base, keeping the fragile boot layout bit-identical (plan risk R4).
// Revisit (drop the alignment) only when a game actually needs the headroom
// below the next 16 MB boundary.
// A manifest [module] heap_base pin (RECOMP_HEAP_OVERRIDE, issue #46)
// bypasses the align policy when a game needs tighter packing.
static constexpr uint32_t k_heap_start =
    (RECOMP_HEAP_OVERRIDE != 0x0U)
        ? RECOMP_HEAP_OVERRIDE
        : ((RECOMP_HEAP_BASE + 0x00FFFFFFU) & ~0x00FFFFFFU);
static uint32_t g_heap_pos = k_heap_start;
// Cap dlmalloc at 0x0B000000 so it cannot grow into the guest thread-stack
// region (psp_alloc_stack carves DOWN from PSP_USER_MEM_END - 0x1000 =
// 0x0BFFF000). Historically this was PSP_BND_ARENA_BASE, but the BND arena
// has moved above PSP_USER_MEM_END (see psp_hle.h) — keep the heap cap at
// the same address so heap layout is unchanged and stacks stay safe.
static constexpr uint32_t g_heap_end = 0x0B000000U;
static_assert(g_heap_end <= PSP_USER_MEM_END - 0x1000U,
              "heap must stay below the guest thread-stack region");
static_assert(k_heap_start < g_heap_end,
              "16MB-aligned RECOMP_HEAP_BASE must leave room below the "
              "stack-region floor — image too large for the P11 alignment "
              "policy; revisit #47 P11");

static std::unordered_map<int, uint32_t> g_mem_blocks;   // uid -> addr
static std::unordered_map<int, uint32_t> g_mem_sizes;    // uid -> size

// Game-module partition-alloc observer slot (#47 P5 seam, installed via
// psp_kmem_set_partition_alloc_observer below).
static PspPartitionAllocObserver g_partition_alloc_observer = nullptr;

// ---- HLE Functions ----

static void hle_sceKernelAllocPartitionMemory(
    uint8_t* rdram, recomp_context* ctx
) {
    int32_t partition = ctx->r[4];
    uint32_t name_ptr = static_cast<uint32_t>(ctx->r[5]);
    int32_t type = ctx->r[6];
    uint32_t size = static_cast<uint32_t>(ctx->r[7]);

    (void)partition;
    (void)type;

    const char* name = "";
    if (name_ptr != 0) {
        name = reinterpret_cast<const char*>(
            rdram + (name_ptr & PSP_ADDR_MASK));
    }

    // Align to 256 bytes
    uint32_t aligned = (size + 0xFF) & ~0xFFU;

    if (g_heap_pos + aligned > g_heap_end) {
        std::fprintf(stderr,
            "[HLE] sceKernelAllocPartitionMemory(\"%s\", %u) "
            "OUT OF MEMORY\n", name, size);
        ctx->r[2] = SCE_KERNEL_ERROR_NO_MEMORY;
        return;
    }

    uint32_t block_addr = g_heap_pos;
    g_heap_pos += aligned;

    int uid = psp_next_uid();
    g_mem_blocks[uid] = block_addr;
    g_mem_sizes[uid] = aligned;

    std::fprintf(stderr,
        "[HLE] sceKernelAllocPartitionMemory(\"%s\", %u) "
        "-> uid=%d addr=0x%08X\n",
        name, size, uid, block_addr);

    // Game-module seam (issue #47 Phase 5): the observer lets a game module
    // watch partition allocations (e.g. Patapon captures the UserSbrk block
    // as its dlmalloc-override arena) without a title branch in core.
    if (g_partition_alloc_observer != nullptr) {
        g_partition_alloc_observer(name, block_addr, aligned);
    }

    ctx->r[2] = uid;
}

static void hle_sceKernelFreePartitionMemory(
    uint8_t* rdram, recomp_context* ctx
) {
    int uid = ctx->r[4];
    g_mem_blocks.erase(uid);
    g_mem_sizes.erase(uid);
    ctx->r[2] = SCE_OK;
    (void)rdram;
}

static void hle_sceKernelGetBlockHeadAddr(
    uint8_t* rdram, recomp_context* ctx
) {
    int uid = ctx->r[4];
    auto it = g_mem_blocks.find(uid);
    if (it != g_mem_blocks.end()) {
        ctx->r[2] = static_cast<int32_t>(it->second);
    } else {
        std::fprintf(stderr,
            "[HLE] sceKernelGetBlockHeadAddr(uid=%d) NOT FOUND\n",
            uid);
        ctx->r[2] = 0;
    }
    (void)rdram;
}

static void hle_sceKernelMaxFreeMemSize(
    uint8_t* rdram, recomp_context* ctx
) {
    ctx->r[2] = static_cast<int32_t>(g_heap_end - g_heap_pos);
    (void)rdram;
}

static void hle_sceKernelTotalFreeMemSize(
    uint8_t* rdram, recomp_context* ctx
) {
    ctx->r[2] = static_cast<int32_t>(g_heap_end - g_heap_pos);
    (void)rdram;
}

static void hle_sceKernelDevkitVersion(
    uint8_t* rdram, recomp_context* ctx
) {
    ctx->r[2] = 0x06060010;  // PSP FW 6.60
    (void)rdram;
}

static void hle_sceKernelPrintf(
    uint8_t* rdram, recomp_context* ctx
) {
    uint32_t fmt_ptr = static_cast<uint32_t>(ctx->r[4]);
    if (fmt_ptr != 0) {
        const char* fmt = reinterpret_cast<const char*>(
            rdram + (fmt_ptr & PSP_ADDR_MASK));
        std::fprintf(stderr, "[GAME] %s", fmt);
    }
    ctx->r[2] = SCE_OK;
}

// Compiled SDK version (mirrors PPSSPP sceKernelMemory.cpp sdkVersion_):
// recorded by the sceKernelSetCompiledSdkVersion* family, read by
// sceKernelReferThreadStatus to pick the 104- vs 108-byte
// SceKernelThreadInfo protocol (gate: version > 0x02060010). 0 = never set.
static uint32_t g_compiled_sdk_version = 0;

uint32_t psp_kernel_compiled_sdk_version() {
    return g_compiled_sdk_version;
}

static void hle_sceKernelSetCompiledSdkVersion(
    uint8_t* rdram, recomp_context* ctx
) {
    g_compiled_sdk_version = static_cast<uint32_t>(ctx->r[4]);
    ctx->r[2] = SCE_OK;
    (void)rdram;
}

static void hle_sceKernelSetCompilerVersion(
    uint8_t* rdram, recomp_context* ctx
) {
    ctx->r[2] = SCE_OK;
    (void)rdram;
}

// ---- Game-module seams (issue #47 Phase 5) ----
// The per-game module (games/<id>/runtime/, e.g. Patapon's allocator/CRT
// overrides) interacts with the bump heap ONLY through these functions —
// the heap statics stay private and the core carries no game knowledge.

void psp_kmem_set_partition_alloc_observer(PspPartitionAllocObserver fn) {
    g_partition_alloc_observer = fn;
}

uint32_t psp_kmem_bump_alloc(uint32_t size) {
    if (size == 0 || g_heap_pos + size > g_heap_end) return 0;
    uint32_t addr = g_heap_pos;
    g_heap_pos += size;
    return addr;
}

void psp_kmem_reserve_remaining(uint32_t* start, uint32_t* end) {
    if (start) *start = g_heap_pos;
    if (end) *end = g_heap_end;
    g_heap_pos = g_heap_end;
}

// ---- Registration ----

void psp_hle_register_kernel_memory() {
    psp_hle_register("sceKernelAllocPartitionMemory",
                      hle_sceKernelAllocPartitionMemory);
    psp_hle_register("sceKernelFreePartitionMemory",
                      hle_sceKernelFreePartitionMemory);
    psp_hle_register("sceKernelGetBlockHeadAddr",
                      hle_sceKernelGetBlockHeadAddr);
    psp_hle_register("sceKernelMaxFreeMemSize",
                      hle_sceKernelMaxFreeMemSize);
    psp_hle_register("sceKernelTotalFreeMemSize",
                      hle_sceKernelTotalFreeMemSize);
    psp_hle_register("sceKernelDevkitVersion",
                      hle_sceKernelDevkitVersion);
    psp_hle_register("sceKernelPrintf",
                      hle_sceKernelPrintf);
    psp_hle_register("sceKernelSetCompiledSdkVersion370",
                      hle_sceKernelSetCompiledSdkVersion);
    psp_hle_register("sceKernelSetCompilerVersion",
                      hle_sceKernelSetCompilerVersion);
}
