#pragma once

// ============================================================================
// games/patapon/runtime/asset_bnd.h — Phase 11 BND parser + arena allocator
// API (Patapon game module; moved from runtime/include in issue #47 Phase 5
// — the BND format and DATA_CMN.BND are Patapon-specific).
//
// Public C++ API for the host-side DATA_CMN.BND parser. The game module
// opens `disc0/PSP_GAME/USRDIR/DATA_CMN.BND`, validates its 16-byte header,
// walks the entry table at offset 0x70 with the walk-until-invalid predicate
// (see 11-RESEARCH.md §3, R-02), and lazily decompresses+allocates per-asset
// descriptors into the dedicated PSP arena at 0x0C400000..0x0F400000.
//
// All public symbols here pair with definitions in asset_bnd.cpp.
// The header is declaration-only — no inline function bodies.
//
// CLAUDE.md §3 rule 3: every API that touches PSP memory takes `uint8_t* rdram`
// as its FIRST parameter — rdram is NEVER a recomp_context field.
// CLAUDE.md §3 rule 4: any rdram address arithmetic must mask with
// PSP_ADDR_MASK (0x07FFFFFFU) — never `*(uint32_t*)(rdram + addr)`.
// ============================================================================

#include <atomic>
#include <cstdint>
#include <string>

#include "hle/psp_hle.h"  // PSP_USER_MEM_END (arena alias-safety asserts)

// Forward declaration — runtime/include/recomp.h owns the full definition.
struct recomp_context;

// ---- BND parser arena placement ----
// Host-side asset staging region OUTSIDE guest user memory. Moved here from
// runtime/include/hle/psp_hle.h with the rest of the BND layer (issue #47
// Phase 5) — the arena exists only for this parser.
//
// History: the arena originally lived at 0x0B000000..0x0C000000 (carved from
// the top of user memory, Phase 11 D-02). That collided with guest thread
// stacks, which `psp_alloc_stack` carves DOWN from PSP_USER_MEM_END - 0x1000
// = 0x0BFFF000: by the time titledata.bnd (1.4 MB decompressed) resolved, the
// arena bump cursor had reached ~0x0BE3CC00 and the payload memcpy stomped
// user_main's live stack frames (saved s3 = engine base overwritten →
// frame-tick list walk never terminated → boot halted at Frame 5). The arena
// also OOMed at 16 MB.
//
// Placement: guest VA 0x0C400000..0x0F400000 (48 MB). Alias safety under
// the runtime's 0x07FFFFFFU mask into the 128 MB rdram:
//   - arena masks to rdram [0x04400000, 0x07400000)
//   - guest user RAM 0x08000000..0x0BFFFFFF masks to [0x00000000, 0x04000000)
//   - VRAM 0x04000000..0x041FFFFF (and uncached 0x44000000 alias) masks to
//     [0x04000000, 0x04200000) — arena starts 2 MB above it
//   - scratchpad 0x00010000 masks to itself (far below)
//   - the [S252] probe stack at 0x0FF00000 masks to 0x07F00000 — above the
//     arena end with 11 MB margin
// No guest region or runtime reservation aliases into [0x04400000, 0x07400000).
constexpr uint32_t PSP_BND_ARENA_BASE = 0x0C400000U;
constexpr uint32_t PSP_BND_ARENA_END  = 0x0F400000U;

// Compile-time alias-safety guards for the BND arena (mask = 0x07FFFFFFU,
// rdram = 128 MB = 0x08000000 bytes, VRAM masked end = 0x04200000).
static_assert(PSP_BND_ARENA_BASE >= PSP_USER_MEM_END,
              "BND arena must not overlap guest user memory / thread stacks");
static_assert((PSP_BND_ARENA_BASE & 0x07FFFFFFU) >= 0x04200000U,
              "BND arena (masked) must not overlap VRAM");
static_assert(((PSP_BND_ARENA_END - 1U) & 0x07FFFFFFU) < 0x08000000U
                  && (PSP_BND_ARENA_END & 0x07FFFFFFU)
                         > (PSP_BND_ARENA_BASE & 0x07FFFFFFU),
              "BND arena (masked) must fit contiguously inside 128MB rdram");

// ----------------------------------------------------------------------------
// BND Arena Allocator
// ----------------------------------------------------------------------------
//
// The arena lives at PSP-VA 0x0C400000..0x0F400000 (48 MB), ABOVE guest user
// memory (PSP_USER_MEM_END = 0x0C000000) so it can never overlap the dlmalloc
// heap or the guest thread stacks that grow down from 0x0BFFF000. Under the
// runtime's 0x07FFFFFFU mask it occupies rdram [0x04400000, 0x07400000) —
// clear of masked user RAM (< 0x04000000) and VRAM (< 0x04200000); see the
// alias analysis next to PSP_BND_ARENA_BASE in hle/psp_hle.h. Bump-only
// (no free).
//
// WHY NOT psp_alloc_kernel_memory: the kernel arena is 0x08000000..0x08400000
// (4 MB), already partially consumed by NativeModule + reent buffers. A single
// default.bnd decompress (1.78 MB) plus a handful of follow-up assets would
// OOM the kernel arena. See 11-RESEARCH.md §6 lines 461-471.

/// Allocate `size` bytes from the BND parser arena, aligned to `alignment`.
///
/// Returns a PSP virtual address (NOT masked — the caller is expected to
/// mask before any direct rdram pointer arithmetic). Returns 0 on OOM; the
/// caller is responsible for emitting a `[BND_ERR]` line and calling
/// `std::abort()` per Phase 11 D-09 if the failure is unrecoverable.
///
/// Alignment policy:
///   - `alignment < 16`  → bumped to 16 (descriptor minimum, matches dlmalloc)
///   - `alignment >= 256` → 256-byte alignment (payload buffers; matches
///                         dlmalloc's payload alignment so PSP-side reads
///                         from FUN_088623E0 see naturally-aligned data)
///
/// Thread-safe: serialised by an internal `std::mutex`. The mutex exists so
/// the rare bnd-init thread and the future renderer thread cannot race on
/// the bump pointer.
uint32_t psp_alloc_bnd_arena(uint32_t size, uint32_t alignment = 16);

// ----------------------------------------------------------------------------
// BND Parser API
// ----------------------------------------------------------------------------

/// Locked outer-entry layout (11-RESEARCH.md §3 R-02 — empirically verified
/// against all 419 entries of DATA_CMN.BND). The SPEC's tentative ordering
/// `{hash, offset_in_data, size, padding}` was WRONG — the first field is
/// the ABSOLUTE file offset (not relative to the data section).
///
/// Validation predicate (see `is_valid_outer_entry` in asset_bnd.cpp):
///   - Sentinel allowance: entry[0] has file_offset==0 && size==0.
///   - All other entries: file_offset >= 0x4800, size > 0,
///                        file_offset + size <= file_size.
struct BndOuterEntry {
    uint32_t file_offset;   ///< Absolute file offset of payload bytes.
    uint32_t size;          ///< Payload size in bytes.
    uint32_t hash;          ///< Asset hash (FUN_0893D6F8 input; out of scope).
    uint32_t secondary;     ///< Possibly inner-asset count / secondary key.
};
static_assert(sizeof(BndOuterEntry) == 16, "BND entry layout drift");

/// Per-asset reference returned by `bnd_resolve_and_allocate` (populated by
/// Plan 11-04 once gzip decompression + cache exist). For this plan
/// (11-03) only the type is declared — the body that fills `cached_desc_addr`
/// lands in 11-04.
struct BndAssetRef {
    uint32_t entry_index;       ///< Index into the outer entry table.
    uint32_t cached_desc_addr;  ///< PSP-VA of the cached 16-byte descriptor.
};

/// One-shot lazy initialiser for the BND parser.
///
/// Opens `DATA_CMN_BND_PATH` (env override) or
/// `"disc0/PSP_GAME/USRDIR/DATA_CMN.BND"` (default), validates the 16-byte
/// header, walks the entry table at offset 0x70 with the walk-until-invalid
/// predicate (cap MAX_BND_ENTRIES = 65536; expected count ~419 in practice
/// per R-02), and builds the in-memory entry list.
///
/// Failure modes (locked by Phase 11 D-08 / D-09):
///   - Missing file (open/fstat fails) → log `[BND_WARN] DATA_CMN.BND
///     missing — falling back to shared stub` EXACTLY ONCE, set
///     `g_bnd_unavailable = true`, return without aborting. The wrapper
///     in Plan 11-05 falls back to the pre-Phase-11 shared stub.
///   - Malformed file (bad magic / over-cap / out-of-bounds entry) → log
///     `[BND_ERR] <file>:<offset>: <reason>` and call `std::abort()`.
///
/// `bnd_init` is strictly pure: no HLE calls, no logging that could re-enter
/// the wrapper (pitfall P6). Safe to call from inside `std::call_once`.
///
/// `rdram` is the FIRST parameter per CLAUDE.md rule 3. The body does not
/// (yet) write to rdram in this plan — descriptor population is 11-04.
void bnd_init(uint8_t* rdram);

/// Resolve `idx_str` to a 16-byte descriptor at a per-asset PSP-VA.
///
/// Stub for Plan 11-03: always returns 0. Plan 11-04 implements:
///   1. Look up `idx_str` in the virtual-path map.
///   2. mmap-read the entry payload.
///   3. gzip-decompress (if 1F 8B magic) or copy raw (if BND\0 magic).
///   4. Allocate decompressed buffer + descriptor via `psp_alloc_bnd_arena`.
///   5. Write descriptor fields (vtable, size, data_offset, 0) into rdram.
///   6. Cache the descriptor PSP-VA in `g_idx_to_desc`.
///
/// Returns 0 on miss (idx_str not in outer index) or stub.
uint32_t bnd_resolve_and_allocate(uint8_t* rdram, const char* idx_str);

// ----------------------------------------------------------------------------
// Wrapper Helpers
// ----------------------------------------------------------------------------

/// Function-pointer signature for recompiled dispatched functions. Matches
/// the `FuncPtr` typedef in runtime/include/hle/psp_hle.h:10 so the wrapper
/// in main.cpp can assign the recompiled-original via:
///     g_asset_lookup_orig = RECOMP_LOOKUP(0x0896A6A4);
/// and `fallback_to_shared_stub` can chain into it under
/// `PSPRECOMP_ASSET_NO_STUB=1`. We re-declare the alias here rather than
/// pulling psp_hle.h into the asset_bnd public surface to keep the header
/// dependency footprint minimal.
using BndFuncPtr = void(*)(uint8_t*, recomp_context*);

/// Recompiled FUN_0896A6A4 captured by the wrapper in main.cpp and consumed
/// by `fallback_to_shared_stub` to honor `PSPRECOMP_ASSET_NO_STUB=1`. main.cpp
/// assigns this exactly once at wrapper installation time; reads from the
/// helper are well-ordered because the wrapper lambda fires only AFTER the
/// install path has run. nullptr until the wrapper installs (no-op chain).
extern BndFuncPtr g_asset_lookup_orig;

/// Pre-Phase-11 shared 60-byte stub fallback path.
///
/// Stub for Plan 11-03: empty body. Plan 11-05 lifts the existing logic from
/// `runtime/src/main.cpp:424-563` into this helper so the BND-disabled and
/// BND-miss paths can both reach it without duplication.
///
/// When called: writes the synthetic 60-byte descriptor into rdram and
/// stores its PSP-VA in `ctx->r[2]` (return value).
void fallback_to_shared_stub(uint8_t* rdram, recomp_context* ctx,
                             const char* idx_str, uint32_t bucket_count);

/// D-08 fallback flag — set true when `bnd_init` detects a missing
/// DATA_CMN.BND. The wrapper checks this before BND lookup and falls back
/// to the shared stub. Atomic because the wrapper runs on the game thread
/// and `bnd_init` is invoked from the host main thread during runtime init.
extern std::atomic<bool> g_bnd_unavailable;

/// Set true at the tail of `bnd_init` once header validation, entry walk,
/// and in-memory list construction all succeed. Read by tests + the wrapper
/// guard in Plan 11-05.
extern std::atomic<bool> g_bnd_initialized;

// ----------------------------------------------------------------------------
// Test Accessors
// ----------------------------------------------------------------------------
//
// Read-only views over the file-scope statics in asset_bnd.cpp
// (`g_outer_entries`, `g_virt_to_entry`). Provided so Plan 11-06's
// `runtime/tests/test_asset_bnd.cpp` can verify parser state without
// exposing the statics globally. NOT part of the production API surface —
// callers in the runtime proper must continue to use
// `bnd_resolve_and_allocate` instead.
//
// Thread safety: safe to call after `bnd_init` returns (the underlying
// containers are written exactly once during init and read-only afterward).

/// Test accessor; safe to call after bnd_init succeeds.
///
/// Returns `g_outer_entries.size()`. Before `bnd_init` runs (or when it
/// failed and set `g_bnd_unavailable`), returns 0 because the vector is
/// default-constructed empty.
size_t bnd_get_outer_entry_count();

/// Test accessor; safe to call after bnd_init succeeds.
///
/// Returns `&g_outer_entries[idx]` when `idx < bnd_get_outer_entry_count()`,
/// otherwise `nullptr`. The pointer is read-only; the caller MUST NOT cast
/// away const and modify the entry.
const BndOuterEntry* bnd_get_outer_entry(size_t idx);

/// Test accessor; safe to call after bnd_init succeeds.
///
/// Returns true if `virt_path` is a key in `g_virt_to_entry` (i.e. the
/// names-region walk produced a positional mapping for this path). Returns
/// false if `virt_path == nullptr` or the key is absent.
bool bnd_virt_path_resolves(const char* virt_path);
/// Runtime helper; safe to call after bnd_init succeeds.
///
/// Returns the outer entry mapped from `virt_path`, or nullptr when the
/// path is absent from `g_virt_to_entry`.
const BndOuterEntry* bnd_find_outer_entry_for_virt_path(const char* virt_path);

/// Test accessor; safe to call after bnd_init succeeds.
///
/// Returns the first 4 bytes of entry `idx`'s on-disk payload as a
/// little-endian uint32. Returns 0 when `idx >= bnd_get_outer_entry_count()`
/// OR the entry is the sentinel (file_offset == 0 && size == 0) — both
/// cases convey "no payload to read". Used by tests to find a gzip-headed
/// (0x08088B1F) entry without re-mmap'ing DATA_CMN.BND on the test side.
uint32_t bnd_get_entry_payload_signature(size_t idx);

/// Test accessor; safe to call after bnd_init succeeds.
///
/// Reverse-lookup over `g_virt_to_entry`: returns the FIRST virt_path that
/// maps to `entry_idx`, or nullptr if no path maps. Output pointer is
/// borrowed from the map's keys (stable for the lifetime of the parser).
/// O(n) over g_virt_to_entry — only suitable for diagnostic / test use.
const char* bnd_find_virt_path_for_entry(uint32_t entry_idx);
