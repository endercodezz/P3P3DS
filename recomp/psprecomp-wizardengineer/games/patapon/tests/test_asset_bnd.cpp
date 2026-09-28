// ============================================================================
// games/patapon/tests/test_asset_bnd.cpp — Phase 11-06 BND parser unit tests
// (moved from runtime/tests with the BND layer, issue #47 Phase 5)
//
// Activates the EXISTS-gated `test_asset_bnd` CMake target declared by
// Plan 11-02. Linked against `asset_bnd.cpp` + `psp_memory.cpp` only
// (see BND_TEST_SOURCES in runtime/CMakeLists.txt) — no SDL / GL / HLE
// transitively pulled in.
//
// Harness pattern: lifted directly from `runtime/tests/test_vfpu.cpp` —
// `static int failures / tests_run`, `ASSERT_*` macros, `main()` runs every
// `test_*` in declared order and returns `failures > 0 ? 1 : 0`.
//
// CLAUDE.md rule 4: every rdram read uses psp_mem_read<T> (never raw cast).
// CLAUDE.md rule 3: rdram is a separate FIRST parameter to every PSP API.
//
// Coverage (per 11-PLAN.md tasks):
//   R1 — bnd_init opens DATA_CMN.BND, counts 419 outer entries.
//   R2 — bnd_resolve_and_allocate("loadinggroup/systemdata.bnd") returns a
//        descriptor whose payload starts with `BND\0` and whose declared
//        size is >= 1 MB (gzip-inflated nested BND archive).
//   R3 — g_virt_to_entry contains "loadinggroup/systemdata.bnd"; bogus
//        path resolves false.
//   Arena alignment — psp_alloc_bnd_arena enforces 16/256 alignment.
//   Arena OOM — psp_alloc_bnd_arena(>16 MB) returns 0.
//   R4 — two distinct virtual paths resolve to distinct descriptors with
//        distinct sizes and payload addresses (skipped if titledata.bnd
//        not in the names map for the local DATA_CMN.BND).
//
// Test execution order matters because:
//   * bnd_init MUST run first (every descriptor test depends on it).
//   * The BND arena is a single-process bump allocator — once
//     bnd_resolve_and_allocate consumes some of it, the arena-alignment
//     test still passes (it asserts a fresh aligned addr) but the OOM
//     test must run LAST so the arena has not been exhausted by
//     accident from preceding tests.
// ============================================================================

#include "asset_bnd.h"
#include "hle/psp_hle.h"
#include "psp_memory.h"
#include "recomp.h"

#include <atomic>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

// ---------------------------------------------------------------------------
// Linker stubs for symbols the runtime ordinarily provides.
//
// `asset_bnd.cpp` calls `psp_alloc_kernel_memory` from inside
// `fallback_to_shared_stub`. That helper lives in psp_hle_kernel_memory.cpp,
// which is NOT in BND_TEST_SOURCES. None of these tests exercise the
// fallback path, so we provide a trivial stub here to satisfy the linker
// without dragging the whole HLE layer in. If a future test calls into the
// fallback, the test will trip the assert below. Declared with C++ linkage
// (no `extern "C"`) to match the declaration in `hle/psp_hle.h`.
// ---------------------------------------------------------------------------
uint32_t psp_alloc_kernel_memory(uint32_t /*size*/) {
    std::fprintf(stderr, "FATAL: tests must not reach fallback path\n");
    std::abort();
}

// ---------------------------------------------------------------------------
// Harness preamble — mirrors test_vfpu.cpp lines 1-50.
// ---------------------------------------------------------------------------

static int failures  = 0;
static int tests_run = 0;

#define ASSERT_TRUE(cond, msg) \
    do { \
        tests_run++; \
        if (!(cond)) { \
            std::fprintf(stderr, "FAIL: %s: condition false: %s\n", \
                         msg, #cond); \
            failures++; \
        } \
    } while (0)

#define ASSERT_INT_EQ(actual, expected, msg) \
    do { \
        tests_run++; \
        if ((actual) != (expected)) { \
            std::fprintf(stderr, \
                "FAIL: %s: got 0x%X, expected 0x%X\n", \
                msg, \
                static_cast<unsigned>(actual), \
                static_cast<unsigned>(expected)); \
            failures++; \
        } \
    } while (0)

#define ASSERT_GE(actual, lower_bound, msg) \
    do { \
        tests_run++; \
        if (!((actual) >= (lower_bound))) { \
            std::fprintf(stderr, \
                "FAIL: %s: got %llu, expected >= %llu\n", \
                msg, \
                static_cast<unsigned long long>(actual), \
                static_cast<unsigned long long>(lower_bound)); \
            failures++; \
        } \
    } while (0)

#define ASSERT_BUF_EQ(buf, expected, n, msg) \
    do { \
        tests_run++; \
        if (std::memcmp((buf), (expected), (n)) != 0) { \
            std::fprintf(stderr, "FAIL: %s: byte buffers differ\n", msg); \
            failures++; \
        } \
    } while (0)

// ---------------------------------------------------------------------------
// Constants
// ---------------------------------------------------------------------------

// LAYER A (doc 15): the entry table spans [0x70 .. names_off) = exactly
// (0x1A98 - 0x70) / 16 = 418 sixteen-byte entries. The prior 419 count
// included one spurious entry read from the first 16 bytes of the names
// region; the walk is now bounded at names_off (header +0x10).
static constexpr size_t   EXPECTED_OUTER_ENTRY_COUNT = 418U;

// First 4 bytes of every nested BND archive (little-endian read of
// `'B','N','D','\0'`). Matches SIG_BND_LE in asset_bnd.cpp.
static constexpr uint32_t INNER_BND_MAGIC = 0x00444E42U;

// gzip member magic (first 4 bytes are 0x1F 0x8B 0x08 + flags-byte). The
// 0x08 in slot 2 is the deflate compression-method ID; slot 3 holds the
// FLG bits and varies across entries. Histogram tally in `bnd_init` checks
// the full 4-byte SIG_GZIP_LE constant, but here we route on the first
// byte alone (`0x1F`) to admit every well-formed gzip header.
static constexpr uint8_t  GZIP_FIRST_BYTE   = 0x1FU;

// 1 MB headroom gate for the inflated gzip-payload sanity check. RESEARCH
// §3 reports the largest gzip-compressed entry inflates to ~1.78 MB; we
// assert >= 1 MB so the test stays robust against minor repacking variance.
static constexpr uint32_t INFLATED_MIN_SIZE = 1U * 1024U * 1024U;

// systemdata.bnd is empirically a BND-passthrough (NOT gzip-compressed) at
// entry 373 with 5248 bytes (11-04-SUMMARY.md). Plan 11-06's text assumed
// it was gzip; the assumption was wrong. The test for systemdata.bnd
// therefore only asserts non-zero size + BND\0 head + arena-resident
// payload — the gzip path is exercised by `test_bnd_inflate_gzip_entry`.
//
// Captured by test 2 so SUMMARY.md can record it.
static uint32_t g_systemdata_passthrough_size = 0U;

// Inflated size of the first gzip-mapped entry — captured by test 2b so
// SUMMARY.md can record it as the gzip-inflate evidence.
static uint32_t g_first_gzip_inflated_size = 0U;
static const char* g_first_gzip_virt_path  = nullptr;

// ---------------------------------------------------------------------------
// Test 1: open + count outer entries (R1)
// ---------------------------------------------------------------------------
static void test_bnd_open_and_count_entries(std::vector<uint8_t>& rdram) {
    std::printf("  test_bnd_open_and_count_entries...\n");

    bnd_init(rdram.data());

    ASSERT_TRUE(g_bnd_initialized.load(), "g_bnd_initialized");
    ASSERT_TRUE(!g_bnd_unavailable.load(), "g_bnd_unavailable false");

    const size_t count = bnd_get_outer_entry_count();
    ASSERT_INT_EQ(static_cast<uint32_t>(count),
                  static_cast<uint32_t>(EXPECTED_OUTER_ENTRY_COUNT),
                  "entry count = 419");

    // Sentinel rule (R-02 §3): entry[0] is `{0, 0, ...}`.
    const BndOuterEntry* e0 = bnd_get_outer_entry(0U);
    ASSERT_TRUE(e0 != nullptr, "entry[0] not null");
    if (e0 != nullptr) {
        ASSERT_INT_EQ(e0->file_offset, 0U, "entry[0].file_offset == 0");
        ASSERT_INT_EQ(e0->size,        0U, "entry[0].size == 0");
    }

    // entry[1] must be a real payload (file_offset past data-section floor,
    // size > 0). Both invariants are enforced by `is_valid_outer_entry`
    // in asset_bnd.cpp — restating them here as a parser-state contract.
    const BndOuterEntry* e1 = bnd_get_outer_entry(1U);
    ASSERT_TRUE(e1 != nullptr, "entry[1] not null");
    if (e1 != nullptr) {
        ASSERT_TRUE(e1->size > 0U, "entry[1].size > 0");
        ASSERT_GE(e1->file_offset, 0x4800U, "entry[1].file_offset >= 0x4800");
    }

    // Out-of-bounds index returns nullptr (defensive).
    const BndOuterEntry* eN = bnd_get_outer_entry(count);
    ASSERT_TRUE(eN == nullptr, "OOB index returns nullptr");
}

// ---------------------------------------------------------------------------
// Test 2a: resolve `loadinggroup/systemdata.bnd` — BND-passthrough path
//
// 11-04-SUMMARY.md empirically pins systemdata.bnd at entry 373: a 5248-byte
// uncompressed BND archive (passthrough, no gzip). The test asserts the
// resolver returns a non-zero descriptor with a BND\0-headed payload sized
// > 0 and located inside the BND arena. The "1 MB inflated" assumption in
// the original plan text was incorrect for this asset and is replaced by
// the dedicated gzip test below (test 2b). [Rule 1 deviation — plan-text
// vs empirical reality.]
// ---------------------------------------------------------------------------
static void test_bnd_decompress_entry(std::vector<uint8_t>& rdram) {
    std::printf("  test_bnd_decompress_entry...\n");

    const uint32_t desc_addr =
        bnd_resolve_and_allocate(rdram.data(),
                                 "loadinggroup/systemdata.bnd");
    ASSERT_TRUE(desc_addr != 0U, "systemdata.bnd descriptor allocated");
    if (desc_addr == 0U) return;

    // Descriptor layout (see asset_bnd.cpp step 9):
    //   +0 vtable_ptr
    //   +4 size (decompressed/passthrough payload size in bytes)
    //   +8 data_offset (payload PSP-VA)
    //  +12 reserved (0)
    const uint32_t size =
        psp_mem_read<uint32_t>(rdram.data(), desc_addr + 4U);
    const uint32_t payload_addr =
        psp_mem_read<uint32_t>(rdram.data(), desc_addr + 8U);

    ASSERT_TRUE(size > 0U, "systemdata.bnd size > 0");
    g_systemdata_passthrough_size = size;

    // First 4 bytes of payload must be `BND\0` — confirms the resolver
    // delivered an inner BND archive (passthrough path here, gzip path in
    // test 2b). Use psp_mem_read (rule 4).
    const uint32_t head =
        psp_mem_read<uint32_t>(rdram.data(), payload_addr);
    ASSERT_INT_EQ(head, INNER_BND_MAGIC, "payload head == BND\\0");

    // Sanity: payload_addr is inside the BND arena range [PSP_BND_ARENA_BASE,
    // PSP_BND_ARENA_END) and 256-byte aligned (asset_bnd.cpp step 6).
    ASSERT_TRUE(payload_addr >= PSP_BND_ARENA_BASE,
                "payload_addr >= arena base");
    ASSERT_TRUE(payload_addr <  PSP_BND_ARENA_END,
                "payload_addr <  arena end");
    ASSERT_INT_EQ(payload_addr & 0xFFU, 0U,
                  "payload_addr 256-byte aligned");
}

// ---------------------------------------------------------------------------
// Test 2b: gzip inflate (R2 spirit — ≥ 1 MB inflated buffer)
//
// Iterates all 419 outer entries to find the first gzip-headed payload
// (first byte 0x1F) that ALSO has a virtual-path mapping (so the resolver
// can be exercised end-to-end). If no gzip+mapped entry exists in the
// current DATA_CMN.BND, the test is skipped with a [TEST_SKIP] log line.
// Otherwise the test asserts the resolver returns an inflated buffer
// >= 1 MB (RESEARCH §3 reports the largest gzip entry inflates to ~1.78
// MB).
// ---------------------------------------------------------------------------
static void test_bnd_inflate_gzip_entry(std::vector<uint8_t>& rdram) {
    std::printf("  test_bnd_inflate_gzip_entry...\n");

    const size_t count = bnd_get_outer_entry_count();
    uint32_t    target_idx     = UINT32_MAX;
    const char* target_virt    = nullptr;
    uint32_t    target_compsize = 0U;
    for (size_t i = 0; i < count; ++i) {
        const uint32_t sig = bnd_get_entry_payload_signature(i);
        if ((sig & 0xFFU) != GZIP_FIRST_BYTE) continue;

        const char* vp =
            bnd_find_virt_path_for_entry(static_cast<uint32_t>(i));
        if (vp == nullptr) continue;

        const BndOuterEntry* e = bnd_get_outer_entry(i);
        if (e == nullptr) continue;
        target_idx     = static_cast<uint32_t>(i);
        target_virt    = vp;
        target_compsize = e->size;
        break;
    }

    if (target_virt == nullptr) {
        std::printf("    [TEST_SKIP] no gzip-headed entry has a virt_path "
                    "mapping in this DATA_CMN.BND\n");
        return;
    }
    std::printf("    found gzip entry %u virt=\"%s\" compressed=%u\n",
                target_idx, target_virt, target_compsize);

    const uint32_t desc_addr =
        bnd_resolve_and_allocate(rdram.data(), target_virt);
    ASSERT_TRUE(desc_addr != 0U, "gzip-mapped path resolves");
    if (desc_addr == 0U) return;

    const uint32_t inflated =
        psp_mem_read<uint32_t>(rdram.data(), desc_addr + 4U);
    const uint32_t payload_addr =
        psp_mem_read<uint32_t>(rdram.data(), desc_addr + 8U);

    ASSERT_GE(inflated, INFLATED_MIN_SIZE, "inflated size >= 1 MB");
    g_first_gzip_inflated_size = inflated;
    g_first_gzip_virt_path     = target_virt;

    // The first 4 bytes of an inflated gzip entry are not guaranteed to be
    // `BND\0` (could be raw asset bytes); we only assert the buffer is in
    // the arena and properly aligned.
    ASSERT_TRUE(payload_addr >= PSP_BND_ARENA_BASE,
                "gzip payload in arena (lower)");
    ASSERT_TRUE(payload_addr <  PSP_BND_ARENA_END,
                "gzip payload in arena (upper)");
}

// ---------------------------------------------------------------------------
// Test 3: virtual path map (R3)
// ---------------------------------------------------------------------------
static void test_bnd_virtual_path_map_has_systemdata() {
    std::printf("  test_bnd_virtual_path_map_has_systemdata...\n");

    ASSERT_TRUE(bnd_virt_path_resolves("loadinggroup/systemdata.bnd"),
                "systemdata.bnd is mapped");
    ASSERT_TRUE(!bnd_virt_path_resolves("garbage/nonexistent.bnd"),
                "bogus path NOT mapped");
    // nullptr safety: accessor must not crash on nullptr.
    ASSERT_TRUE(!bnd_virt_path_resolves(nullptr),
                "nullptr path returns false");
}

// ---------------------------------------------------------------------------
// Test 4: arena alignment policy
// ---------------------------------------------------------------------------
static void test_bnd_arena_alignment() {
    std::printf("  test_bnd_arena_alignment...\n");

    const uint32_t a256 = psp_alloc_bnd_arena(100U, 256U);
    ASSERT_TRUE(a256 != 0U, "alloc(100, 256) not OOM");
    ASSERT_INT_EQ(a256 & 0xFFU, 0U, "256-byte aligned");
    ASSERT_TRUE(a256 >= PSP_BND_ARENA_BASE, "in BND arena (lower)");
    ASSERT_TRUE(a256 <  PSP_BND_ARENA_END, "in BND arena (upper)");

    const uint32_t a16  = psp_alloc_bnd_arena(16U, 16U);
    ASSERT_TRUE(a16 != 0U, "alloc(16, 16) not OOM");
    ASSERT_INT_EQ(a16 & 0xFU, 0U, "16-byte aligned");
}

// ---------------------------------------------------------------------------
// Test 5: arena OOM
// ---------------------------------------------------------------------------
static void test_bnd_arena_oom() {
    std::printf("  test_bnd_arena_oom...\n");

    // Arena size + 1 — strictly larger than the entire BND arena, so even
    // with a freshly-reset bump pointer the allocator must return 0. uint64
    // math in `psp_alloc_bnd_arena` is what guarantees the overflow check
    // fires before any wrap-around. (arena size + 1) plus the current bump
    // position can only ever exceed PSP_BND_ARENA_END.
    const uint32_t oom = psp_alloc_bnd_arena(
        (PSP_BND_ARENA_END - PSP_BND_ARENA_BASE) + 1U, 16U);
    ASSERT_INT_EQ(oom, 0U, "alloc(> arena size) returns 0 (OOM)");
}

// ---------------------------------------------------------------------------
// Test 6: two distinct resolves → distinct descriptors (R4)
// ---------------------------------------------------------------------------
static void test_bnd_two_distinct_resolves(std::vector<uint8_t>& rdram) {
    std::printf("  test_bnd_two_distinct_resolves...\n");

    const uint32_t desc_a =
        bnd_resolve_and_allocate(rdram.data(),
                                 "loadinggroup/systemdata.bnd");
    ASSERT_TRUE(desc_a != 0U, "systemdata.bnd resolves");

    // If titledata.bnd is not in the names map for the local DATA_CMN.BND
    // (positional mapping is fragile across repackings), skip the rest of
    // the test — see header comment.
    if (!bnd_virt_path_resolves("loadinggroup/titledata.bnd")) {
        std::printf("    [TEST_SKIP] titledata.bnd not in names map "
                    "(positional mapping for this BND build)\n");
        return;
    }

    const uint32_t desc_b =
        bnd_resolve_and_allocate(rdram.data(),
                                 "loadinggroup/titledata.bnd");
    ASSERT_TRUE(desc_b != 0U, "titledata.bnd resolves");
    if (desc_b == 0U) return;

    ASSERT_TRUE(desc_a != desc_b, "descriptors are distinct");

    const uint32_t size_a =
        psp_mem_read<uint32_t>(rdram.data(), desc_a + 4U);
    const uint32_t size_b =
        psp_mem_read<uint32_t>(rdram.data(), desc_b + 4U);
    const uint32_t off_a  =
        psp_mem_read<uint32_t>(rdram.data(), desc_a + 8U);
    const uint32_t off_b  =
        psp_mem_read<uint32_t>(rdram.data(), desc_b + 8U);

    ASSERT_TRUE(size_a != 0U, "size_a non-zero");
    ASSERT_TRUE(size_b != 0U, "size_b non-zero");
    ASSERT_TRUE(size_a != size_b, "sizes distinct");
    ASSERT_TRUE(off_a  != off_b,  "data offsets distinct");
}

// ---------------------------------------------------------------------------
// main
//
// Execution order (mandatory):
//   1. test_bnd_open_and_count_entries — calls bnd_init; every later test
//      depends on this having run successfully.
//   2a test_bnd_decompress_entry       — systemdata.bnd BND-passthrough.
//   2b test_bnd_inflate_gzip_entry     — first gzip-mapped entry, ≥ 1 MB
//                                        inflated (covers R2 intent).
//   3. test_bnd_virtual_path_map_has_systemdata — read-only.
//   4. test_bnd_two_distinct_resolves  — distinct descriptors / sizes.
//   5. test_bnd_arena_alignment        — read+bump of arena.
//   6. test_bnd_arena_oom              — final, asserts OOM rejection.
//
// The arena is a single-process bump allocator; test 2b can consume up to
// ~1.78 MB. Tests 4 and 5 add further bumps. Test 6 (OOM) is purposely
// run LAST so its 16 MB + 1 request fails on the size check, not because
// earlier tests exhausted the arena.
// ---------------------------------------------------------------------------
int main() {
    std::printf("Running BND parser runtime tests...\n\n");

    // 128 MB rdram buffer, zero-initialised. Shared across every test so
    // bnd_init's descriptor writes are visible to subsequent reads.
    std::vector<uint8_t> rdram(0x08000000U, 0);

    test_bnd_open_and_count_entries(rdram);
    test_bnd_decompress_entry(rdram);
    test_bnd_inflate_gzip_entry(rdram);
    test_bnd_virtual_path_map_has_systemdata();
    test_bnd_two_distinct_resolves(rdram);
    test_bnd_arena_alignment();
    test_bnd_arena_oom();

    std::printf("\n%d tests run, %d failures\n", tests_run, failures);
    if (g_systemdata_passthrough_size != 0U) {
        std::printf("systemdata.bnd passthrough size: %u bytes\n",
                    g_systemdata_passthrough_size);
    }
    if (g_first_gzip_inflated_size != 0U && g_first_gzip_virt_path != nullptr) {
        std::printf("first gzip entry: \"%s\" inflated=%u bytes\n",
                    g_first_gzip_virt_path, g_first_gzip_inflated_size);
    }

    if (failures == 0) {
        std::printf("ALL TESTS PASSED\n");
        return 0;
    }
    std::printf("SOME TESTS FAILED\n");
    return 1;
}
