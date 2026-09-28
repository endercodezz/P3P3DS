// ============================================================================
// runtime/src/asset_bnd.cpp — Phase 11-03 BND parser foundation
//
// Implements the public API declared in games/patapon/runtime/asset_bnd.h:
//   - psp_alloc_bnd_arena  — mutex-guarded bump allocator over
//                            [PSP_BND_ARENA_BASE..PSP_BND_ARENA_END]
//   - bnd_init             — open + header-validate + entry-table walk
//   - bnd_resolve_and_allocate — stub for Plan 11-04
//   - fallback_to_shared_stub  — stub for Plan 11-05
//
// Does NOT implement gzip decompression, names-region walking, signature
// dispatch, or descriptor population — those land in 11-04 / 11-05.
//
// References:
//   - 11-RESEARCH.md §3  (R-02 entry layout, walk-until-invalid predicate,
//                         locked BND_TRACE dump format lines 252-261)
//   - 11-RESEARCH.md §6  (arena allocator body lines 420-435)
//   - 11-RESEARCH.md §7  (bnd_init shape lines 553-571)
//   - 11-RESEARCH.md §10 (pitfalls P1 / P3 / P5 / P6 / P7)
//   - 11-CONTEXT.md  D-04 (allocator location), D-08 (missing-file warn),
//                    D-09 (malformed-file abort), D-10 (split rationale)
//   - CLAUDE.md      §3   (rdram is a separate FIRST parameter)
//                    §8 r4 (PSP_ADDR_MASK 0x07FFFFFFU on every rdram write)
// ============================================================================

#include "asset_bnd.h"
#include "hle/psp_hle.h"
#include "psp_memory.h"
#include "recomp.h"

#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>
#include <zlib.h>

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

// zlib pulled in by Plan 11-04 to support gzip-compressed BND entries
// (windowBits=31 = gzip-only wrapping). The build linkage is the
// `ZLIB::ZLIB` target wired up in runtime/CMakeLists.txt at Plan 11-02.

// ============================================================================
// File-scope state
// ============================================================================

// Arena bump-pointer + mutex.
// `g_bnd_arena_pos` advances monotonically from PSP_BND_ARENA_BASE; the arena
// can never shrink (bump-only, no free). 48 MB at 0x0C400000 — outside guest
// user memory so it can never stomp guest thread stacks (see psp_hle.h).
static std::mutex g_bnd_arena_mtx;
static uint32_t   g_bnd_arena_pos = PSP_BND_ARENA_BASE;

// mmap-backed view of DATA_CMN.BND. Populated by bnd_init on success;
// unmapped only on process exit (no explicit munmap — host page cache is
// the lifetime owner). Used by future plans (11-04) to read entry payloads.
static const uint8_t* g_bnd_mmap      = nullptr;
static size_t         g_bnd_mmap_size = 0;

// In-memory copy of the outer entry table (after walk-until-invalid).
// Built once in bnd_init; consumed by Plan 11-04's lookup path.
static std::vector<BndOuterEntry> g_outer_entries;

// Plan 11-04: virtual-path → outer entry index map. Populated by
// `build_virt_path_map()` at the tail of `bnd_init()` after the entry-table
// walk. Lookup is the first step of `bnd_resolve_and_allocate`. Keys are
// slash-joined paths exactly as the game's recompiled code requests them
// (e.g. `"loadinggroup/systemdata.bnd"` — see 11-RESEARCH.md §2.3).
static std::unordered_map<std::string, uint32_t> g_virt_to_entry;

// doc 19A — the authoritative on-disk DIRECTORY TABLE at file offset 0x28.
//
// THE DECODE (this is the transform docs 16/17/18 could not pin): the game's
// by-name resolver consults an in-memory table at `base+0x28`, where `base` is
// the directory buffer read straight off disc (FUN_08861aec: base = alloc(size);
// raw read into base — NO transform). So `base+0x28` IS the literal bytes at
// DATA_CMN.BND file offset 0x28. Layout, 16-byte stride, count = header +0x24:
//     { u32 key/content-hash @0, u32 name_rel @4, u32 file_offset @8, u32 size @0xc }
// The NAME for each slot lives at `file + name_rel + 7` (the name follows a
// 7-byte trailer `[flag][b1][b2][val:u32]`). This table is content-hash-sorted
// (the `key`), which is exactly why the prior `0x70`-based attempts failed: the
// `0x70` table (`{off,size,hash,sec}`) is the SAME bytes viewed at a +0x48
// (=4.5-entry) misaligned stride. Reading off/size from `0x28 + idx*16 + {8,0xc}`
// yields the correct {file_offset,size} for every name (verified: all 25 tips,
// colony_msg.lbnd, and the 5 loadinggroup directory containers — see 19A §3).
//
// Maps a bare asset name (e.g. "systemlocalizedata.bnd", "tips01.gxt") to a
// synthesized BndOuterEntry {file_offset, size, 0, 0}. Leaf basenames are 100%
// unique in DATA_CMN.BND (verified), so basename resolution is unambiguous.
static std::unordered_map<std::string, BndOuterEntry> g_name_to_entry;

// doc 19A — header field: directory-table slot count (header +0x24 = 0x1A7 = 423).
static constexpr uint32_t BND_HEADER_DIR_COUNT_OFFSET = 0x24U;
// doc 19A — directory table base (file offset of the {key,name_rel,off,size} table).
static constexpr uint32_t BND_DIR_TABLE_OFFSET        = 0x28U;

// Plan 11-04: idx_str → cached descriptor PSP-VA. Populated on first
// resolution per `idx_str`; subsequent calls return the cached value so
// repeated requests do not re-inflate or re-allocate (R4 acceptance).
static std::unordered_map<std::string, uint32_t> g_idx_to_desc;

// Plan 11-04: one shared 11-entry NOOP vtable per process (PSP-VA in the
// BND arena). Lazily allocated by `allocate_shared_noop_vtable()` on the
// first descriptor population; reused for every subsequent asset. The 11
// slots all hold the pre-Phase-11 wrapper's NOOP_STUB (0x089D76E8).
static uint32_t g_shared_noop_vtable_addr = 0U;

// Public externs — declared in asset_bnd.h. atomic<bool> because they may
// be read concurrently from the game thread (wrapper, Plan 11-05) and the
// host main thread (this file's bnd_init at runtime init).
std::atomic<bool> g_bnd_unavailable{false};
std::atomic<bool> g_bnd_initialized{false};

// Plan 11-05: recompiled original of FUN_0896A6A4, captured by main.cpp's
// wrapper installer. fallback_to_shared_stub chains into it when
// PSPRECOMP_ASSET_NO_STUB=1 is set. Defaults to nullptr — fallback path
// short-circuits to a no-op when the wrapper hasn't installed yet (only
// possible if fallback_to_shared_stub is called before the wrapper, which
// the runtime init sequence ensures does not happen).
BndFuncPtr g_asset_lookup_orig = nullptr;

// Plan 11-05: file-scope state for the lifted fallback path.
//
// `g_fallback_stub_desc` is the cached PSP-VA of the synthetic 60-byte
// descriptor produced on first miss; reused for every subsequent fallback
// call (matches pre-Phase-11 single-static behavior in main.cpp). It is
// allocated via `psp_alloc_kernel_memory` (NOT the BND arena) because this
// path predates Phase 11 and must keep its legacy allocation source — the
// arena was introduced specifically for the BND-resolve hot path.
//
// `g_fallback_fm_count` rate-limits the `[ASSET_STUB]` log line per the
// existing idiom (`<= 8 || % 500 == 0`); see 11-PATTERNS.md.
static uint32_t g_fallback_stub_desc = 0U;
static int      g_fallback_fm_count  = 0;

// Header layout constants (R-02, 11-RESEARCH.md §3 lines 188-261).
//
// Magic stored on-disk as ASCII bytes `B`, `N`, `D`, `\0` at offset 0.
// Little-endian uint32 read of those four bytes yields 0x00444E42.
//   byte[0]='B'=0x42  → LSB
//   byte[1]='N'=0x4E
//   byte[2]='D'=0x44
//   byte[3]='\0'=0x00 → MSB
// Per the empirical R-02 dump (`head=424e4400`), the on-disk byte order is
// `42 4E 44 00`, so a host-little-endian uint32 load yields 0x00444E42.
static constexpr uint32_t BND_MAGIC_LE       = 0x00444E42U;
static constexpr uint32_t BND_HEADER_VERSION = 1U;
static constexpr uint32_t BND_HEADER_VERSION_OFFSET   = 0x04U;
// Header field offsets (verified against disc0/PSP_GAME/USRDIR/DATA_CMN.BND):
//   +0x08 = entry-table offset (0x800 in DATA_CMN.BND)   [LAYER A: was unused]
//   +0x10 = names-region offset (0x1A98)
//   +0x14 = data-section offset (0x4800)
//   +0x20 = primary count   (0x197 = 407 = file-leaf count)
//   +0x24 = secondary count (0x1A7 = 423)
static constexpr uint32_t BND_HEADER_ENTRY_TABLE_OFFSET = 0x08U;
static constexpr uint32_t BND_HEADER_NAMES_SEC_OFFSET    = 0x10U;
static constexpr uint32_t BND_HEADER_DATA_SEC_OFFSET     = 0x14U;
static constexpr uint32_t BND_DATA_SECTION_FLOOR         = 0x4800U;

// LAYER A (doc 15 — BND parser foundation; verified against disc0 DATA_CMN.BND).
//
// The entry table is ONE contiguous, HASH-SORTED block of 16-byte entries:
//   [0x70 .. names_off)  == [0x70 .. 0x1A98)  ==  418 entries
// The zero-valued slots interspersed in it (entry[0], plus ~11 more) are NOT
// list terminators — they are holes inside a sorted table, and the existing
// `is_valid_outer_entry` sentinel allowance already keeps the walk going past
// them (it does NOT truncate early — empirically it reaches index ~418/419).
// Doc 14's claim that the walk stopped at the first sentinel was incorrect;
// the entries read ARE the real ascending-hash table (BND/GZ/PSMF payloads).
//
// The header's +0x08 (=0x800) marks an interior split of this same block (the
// upper 297-entry sub-range); the FULL table base is 0x70 and the authoritative
// UPPER BOUND is names_off (0x10), used below so the walk cannot spill the
// first 16 bytes of the names region in as a spurious 419th entry.
//
// NOTE: the substantive bug is NOT here — it is `build_virt_path_map`'s
// POSITIONAL name->entry pairing (Layer B), which is unrelated to the table
// parse. See doc 15 for the decode that remains uncracked.
static constexpr uint32_t BND_ENTRY_TABLE_OFFSET      = 0x70U;

// Hard cap on entry-table walk iterations (11-SPEC.md R1; 11-RESEARCH.md §10
// pitfall P1). Empirically only ~419 entries exist in DATA_CMN.BND, but the
// cap protects against pathologically malformed files whose first sentinel
// happens to look valid for a huge run of bytes.
static constexpr uint32_t MAX_BND_ENTRIES = 65536U;

// Payload-signature magics (first 4 bytes of each entry payload).
// R-02 §3 lines 226-231:  383 BND, 33 GZIP, 3 PSMF, 1 unknown out of 419.
static constexpr uint32_t SIG_BND_LE  = 0x00444E42U;  // 'B','N','D','\0'
static constexpr uint32_t SIG_GZIP_LE = 0x08088B1FU;  // 0x1F,0x8B,0x08,0x08
static constexpr uint32_t SIG_PSMF_LE = 0x464D5350U;  // 'P','S','M','F'

// ============================================================================
// === psp_alloc_bnd_arena ===
// ============================================================================
//
// WHY NOT psp_alloc_kernel_memory: the kernel-memory allocator in
// psp_hle_utility.cpp:21-36 is bounded to 0x08000000..0x08400000 (4 MB),
// already partially consumed by NativeModule (0xC4 bytes) + reent buffers.
// A handful of BND decompresses (default.bnd alone is 1.78 MB decompressed)
// would OOM the kernel arena. The BND parser uses a dedicated 48 MB arena
// (PSP_BND_ARENA_BASE..PSP_BND_ARENA_END, 0x0C400000..0x0F400000) placed
// ABOVE guest user memory so it cannot collide with the dlmalloc heap or
// guest thread stacks. See psp_hle.h for the alias-safety analysis.
//
// Alignment policy:
//   - alignment < 16  → bumped to 16 (matches dlmalloc minimum, descriptors)
//   - alignment >= 256 → 256-byte alignment (payload buffers)
//
// Shape copied from runtime/src/hle/psp_hle_kernel_memory.cpp:248-322
// (dlmalloc_memalign) — mutex + uint64 overflow guard + OOM log.
// Tag substituted from [DLMALLOC] to [BND_ERR] (Phase 11 D-09).

uint32_t psp_alloc_bnd_arena(uint32_t size, uint32_t alignment) {
    std::lock_guard<std::mutex> lock(g_bnd_arena_mtx);

    // Snap alignment to one of the two locked policies (16 or 256). Any
    // request below 16 collapses to 16; any request >= 256 collapses to
    // 256. Caller doesn't need to know about non-power-of-two alignment.
    if (alignment < 16U) alignment = 16U;
    else if (alignment >= 256U) alignment = 256U;

    // Round bump pointer up to alignment boundary.
    const uint32_t aligned_pos =
        (g_bnd_arena_pos + alignment - 1U) & ~(alignment - 1U);

    // Use uint64_t to prevent unsigned overflow wrapping past
    // PSP_BND_ARENA_END (pitfall on 4 GB+ inputs; cheap insurance).
    const uint64_t end_pos =
        static_cast<uint64_t>(aligned_pos) + size;

    if (end_pos > PSP_BND_ARENA_END) {
        std::fprintf(stderr,
            "[BND_ERR] arena OOM: need %u (align=%u) at 0x%08X, "
            "limit 0x%08X\n",
            size, alignment, aligned_pos, PSP_BND_ARENA_END);
        return 0U;  // caller decides whether to abort
    }

    g_bnd_arena_pos = static_cast<uint32_t>(end_pos);
    return aligned_pos;
}

// ============================================================================
// === Static helpers ===
// ============================================================================

// Validation predicate (11-RESEARCH.md §3 lines 240-247).
//
// Sentinel rule: entry[0] is allowed to be {file_offset=0, size=0, ...};
// hash/sec carry non-zero bookkeeping values whose meaning is currently
// undocumented (possibly total-entry-count and total-archive-size — out of
// scope for 11-03).
//
// All other entries MUST satisfy:
//   - file_offset >= 0x4800       (must point at or past the data section)
//   - size > 0                    (zero-size payloads are reserved)
//   - file_offset + size <= file_size  (in-bounds; uint64 math to dodge wrap)
//
// First failure terminates the walk and locks the entry count.
static bool is_valid_outer_entry(const BndOuterEntry& e, size_t file_size) {
    // Sentinel allowance — entry[0] commonly has file_offset==size==0.
    if (e.file_offset == 0U && e.size == 0U) return true;

    if (e.file_offset < BND_DATA_SECTION_FLOOR) return false;
    if (e.size == 0U) return false;

    const uint64_t end =
        static_cast<uint64_t>(e.file_offset) +
        static_cast<uint64_t>(e.size);
    if (end > static_cast<uint64_t>(file_size)) return false;

    return true;
}

// Human-readable tag for the first-4-byte payload signature, used by the
// PSPRECOMP_BND_TRACE=1 one-shot dump and the histogram. Anything that
// isn't a known signature falls back to "???".
static const char* signature_label(uint32_t sig) {
    if (sig == SIG_BND_LE)  return "BND";
    if (sig == SIG_GZIP_LE) return "GZ";
    if (sig == SIG_PSMF_LE) return "PSMF";
    return "???";
}

// Read a 4-byte little-endian uint32 from a possibly-misaligned byte cursor.
// All BND fields are little-endian on disk per R-02 dump.
static uint32_t read_u32_le(const uint8_t* p) {
    uint32_t v;
    std::memcpy(&v, p, sizeof(v));
    return v;
}

// Plan 11-04 forward declarations — bodies live below `bnd_init` so the
// reader gets entry-table walk first, then names map, then resolve. The
// `bnd_init` tail calls `build_virt_path_map`, hence the forward decl.
static bool gzip_inflate(const uint8_t* src, size_t src_len,
                         std::vector<uint8_t>& out);
static void build_virt_path_map();
static uint32_t allocate_shared_noop_vtable(uint8_t* rdram);
static void bnd_dump_content_check();
static std::string bnd_basename(const char* virt_path);  // doc 19A

// LAYER B correctness gate (doc 15). Returns a short, human-readable
// "identity" of an outer entry's PAYLOAD by peeking the on-disk bytes WITHOUT
// inflating — used to prove that a name->entry mapping points at the asset the
// name MEANS (e.g. titledata -> title/model data, NOT a helmet actor BND).
//
//   - gzip member (1F 8B 08 ..) with FNAME flag  -> the embedded filename
//     (e.g. "default.bnd", "tips01_US.gxt") — the strongest ground truth.
//   - inner BND (BND\0)                          -> its first child names
//     (e.g. "missiondata.bnd,predata.bnd,..." vs "equipparam.dat,model.amdl").
//   - PSMF / other                               -> a short signature tag.
//
// This is diagnostic-only; it never feeds the resolver. It exists so any
// future decode (the still-uncracked name->entry mechanism) can be validated
// against content before it is trusted.
static std::string bnd_entry_identity(const BndOuterEntry& e) {
    if (e.file_offset == 0U && e.size == 0U) return "[SENTINEL]";
    if (g_bnd_mmap == nullptr) return "[no-mmap]";
    const uint64_t end =
        static_cast<uint64_t>(e.file_offset) + static_cast<uint64_t>(e.size);
    if (e.size < 16U || end > static_cast<uint64_t>(g_bnd_mmap_size)) {
        return "[oob]";
    }
    const uint8_t* p = g_bnd_mmap + e.file_offset;

    // gzip with FNAME flag → embedded filename (ground truth).
    if (p[0] == 0x1FU && p[1] == 0x8BU && p[2] == 0x08U && (p[3] & 0x08U)) {
        size_t q = 10;                 // fixed gzip header
        if (p[3] & 0x04U) {            // FEXTRA present → skip XLEN bytes
            if (q + 2 > e.size) return "[gz]";
            const uint32_t xlen = static_cast<uint32_t>(p[q]) |
                                  (static_cast<uint32_t>(p[q + 1]) << 8);
            q += 2U + xlen;
        }
        std::string fn = "gz:";
        for (size_t i = q; i < e.size && i < q + 64; ++i) {
            if (p[i] == 0) break;
            fn.push_back(static_cast<char>(p[i]));
        }
        return fn;
    }

    // Inner BND → list its first few child names from its own names region.
    if (read_u32_le(p) == SIG_BND_LE && e.size >= 0x18U) {
        const uint32_t in_names = read_u32_le(p + 0x10);
        if (in_names + 7U < e.size) {
            std::string out = "bnd:";
            size_t c = in_names + 7U;  // skip the 7-byte names-region header
            int found = 0;
            while (c < e.size && found < 3) {
                size_t s = c;
                while (c < e.size && p[c] != 0) ++c;
                if (c > s) {
                    if (found) out.push_back(',');
                    out.append(reinterpret_cast<const char*>(p + s), c - s);
                    ++found;
                }
                c += 1U + 7U;          // NUL + 7-byte trailer
            }
            return out;
        }
        return "bnd:?";
    }
    if (read_u32_le(p) == SIG_PSMF_LE) return "psmf";
    char tag[24];
    std::snprintf(tag, sizeof(tag), "raw:%02x%02x%02x%02x",
                  p[0], p[1], p[2], p[3]);
    return tag;
}

// ============================================================================
// === bnd_init ===
// ============================================================================
//
// Lazy one-shot initialiser. Opens DATA_CMN.BND, validates the 16-byte
// header, walks the entry table at offset 0x70 until walk-until-invalid
// fires, builds g_outer_entries, and (when PSPRECOMP_BND_TRACE=1 is set)
// emits the locked one-shot dump from 11-RESEARCH.md §3 lines 252-261.
//
// CLAUDE.md rule 3: rdram is the FIRST parameter. Plan 11-03 does NOT write
// to rdram from bnd_init — the (void)rdram cast documents the intent. Plan
// 11-04 starts using it for descriptor + payload writes.
//
// Re-entrancy / pitfall P6: this function is strictly pure — no HLE calls,
// no logging that could trigger an HLE re-entry. The static int warn_count
// rate-gate emits the [BND_WARN] line at most once per process.

void bnd_init(uint8_t* rdram) {
    (void)rdram;  // Plan 11-04 fills this in.

    // ---- Path resolution ------------------------------------------------
    // 11-CONTEXT.md D-08 / 11-RESEARCH.md §9: DATA_CMN_BND_PATH env override
    // exists so the corrupted-fixture test (Plan 11-06) can swap files
    // without clobbering the real disc image.
    const char* env_path = std::getenv("DATA_CMN_BND_PATH");
    const bool  env_path_valid = (env_path != nullptr) && (env_path[0] != '\0');
    const char* path = env_path_valid
        ? env_path
        : "disc0/PSP_GAME/USRDIR/DATA_CMN.BND";

    // ---- Open + fstat (D-08 missing-file path) --------------------------
    int fd = ::open(path, O_RDONLY);
    if (fd < 0) {
        // D-08: missing file = warn ONCE + fallback. Process continues.
        // Pitfall P6: emit via std::fprintf only, no HLE traffic.
        static int warn_count = 0;
        if (warn_count++ == 0) {
            std::fprintf(stderr,
                "[BND_WARN] DATA_CMN.BND missing — falling back to "
                "shared stub (path=%s)\n", path);
        }
        g_bnd_unavailable.store(true, std::memory_order_release);
        return;
    }

    struct stat st{};
    if (::fstat(fd, &st) != 0) {
        static int warn_count = 0;
        if (warn_count++ == 0) {
            std::fprintf(stderr,
                "[BND_WARN] DATA_CMN.BND missing — falling back to "
                "shared stub (fstat failed on %s)\n", path);
        }
        ::close(fd);
        g_bnd_unavailable.store(true, std::memory_order_release);
        return;
    }

    const size_t file_size = static_cast<size_t>(st.st_size);

    // ---- mmap (read-only, MAP_PRIVATE) ----------------------------------
    // 294 MB is too large to pull into host RAM; mmap defers paging to the
    // OS page cache. MAP_PRIVATE is correct because we never write back.
    void* mapped = ::mmap(nullptr, file_size, PROT_READ,
                          MAP_PRIVATE | MAP_FILE, fd, 0);
    ::close(fd);  // fd no longer needed; mapping survives.

    if (mapped == MAP_FAILED) {
        // Treat mmap failure as missing-file: it's almost certainly a host
        // I/O problem (permissions, exhausted address space), not a parser
        // bug. D-10: fail loudly but recoverable.
        std::fprintf(stderr,
            "[BND_WARN] DATA_CMN.BND missing — mmap failed on %s\n", path);
        g_bnd_unavailable.store(true, std::memory_order_release);
        return;
    }

    g_bnd_mmap      = static_cast<const uint8_t*>(mapped);
    g_bnd_mmap_size = file_size;

    // ---- Header validation (D-09 malformed-file path) -------------------
    // Must have at least: header (0x10) + entry table base (0x70) + 1 entry.
    if (file_size < BND_ENTRY_TABLE_OFFSET + sizeof(BndOuterEntry)) {
        std::fprintf(stderr,
            "[BND_ERR] %s:0x0: file too small for header+entry "
            "(size=%zu, need at least %u)\n",
            path, file_size,
            static_cast<unsigned>(BND_ENTRY_TABLE_OFFSET +
                                  sizeof(BndOuterEntry)));
        std::abort();
    }

    const uint32_t magic = read_u32_le(g_bnd_mmap + 0);
    if (magic != BND_MAGIC_LE) {
        std::fprintf(stderr,
            "[BND_ERR] %s:0x0: bad magic (got 0x%08X expected 0x%08X)\n",
            path, magic, BND_MAGIC_LE);
        std::abort();
    }

    const uint32_t version =
        read_u32_le(g_bnd_mmap + BND_HEADER_VERSION_OFFSET);
    if (version != BND_HEADER_VERSION) {
        std::fprintf(stderr,
            "[BND_ERR] %s:0x%X: unsupported version (got %u expected %u)\n",
            path, BND_HEADER_VERSION_OFFSET, version, BND_HEADER_VERSION);
        std::abort();
    }

    const uint32_t data_sec_off =
        read_u32_le(g_bnd_mmap + BND_HEADER_DATA_SEC_OFFSET);
    if (data_sec_off == 0U || data_sec_off > file_size) {
        std::fprintf(stderr,
            "[BND_ERR] %s:0x%X: data-section offset out of range "
            "(got 0x%08X file_size=0x%08zX)\n",
            path, BND_HEADER_DATA_SEC_OFFSET, data_sec_off, file_size);
        std::abort();
    }

    // LAYER A: authoritative upper bound for the entry-table walk. The table
    // ends exactly where the names region begins (header +0x10). Bounding the
    // walk here prevents the first 16 bytes of the names region from being
    // mis-read as a spurious extra entry. Validated: (names_off-0x70)/16 = 418.
    const uint32_t names_sec_off =
        read_u32_le(g_bnd_mmap + BND_HEADER_NAMES_SEC_OFFSET);
    const uint32_t entry_table_end =
        (names_sec_off > BND_ENTRY_TABLE_OFFSET && names_sec_off <= file_size)
            ? names_sec_off
            : static_cast<uint32_t>(file_size);

    // Cross-check (diagnostic): header +0x08 is an interior split (0x800) of
    // the SAME [0x70 .. names_off) table — it must fall inside the table range
    // if present. We do not parse from it (the table base is 0x70), but a wild
    // value would signal an unexpected file layout worth tracing.
    if (std::getenv("PSPRECOMP_BND_TRACE") != nullptr) {
        const uint32_t split_off =
            read_u32_le(g_bnd_mmap + BND_HEADER_ENTRY_TABLE_OFFSET);
        std::fprintf(stderr,
            "[BND_TRACE] entry-table [0x%X..0x%X) interior-split(+0x08)=0x%X\n",
            BND_ENTRY_TABLE_OFFSET, entry_table_end, split_off);
    }

    // ---- Entry table walk (R1, walk-until-invalid) ----------------------
    g_outer_entries.clear();
    g_outer_entries.reserve(512);  // pre-size for the expected ~419 count.

    // Histogram tallies for the BND_TRACE dump.
    uint32_t sig_bnd_count  = 0;
    uint32_t sig_gz_count   = 0;
    uint32_t sig_psmf_count = 0;
    uint32_t sig_other_count = 0;
    uint32_t sentinel_count = 0;

    uint32_t idx = 0;
    for (; idx < MAX_BND_ENTRIES; ++idx) {
        const size_t off =
            static_cast<size_t>(BND_ENTRY_TABLE_OFFSET) +
            static_cast<size_t>(idx) * sizeof(BndOuterEntry);
        if (off + sizeof(BndOuterEntry) > file_size) break;
        // LAYER A: stop at the names-region boundary (header +0x10) so the
        // walk reads only the 418 real entries, never the names region.
        if (off + sizeof(BndOuterEntry) > entry_table_end) break;

        BndOuterEntry e{};
        std::memcpy(&e, g_bnd_mmap + off, sizeof(e));

        if (!is_valid_outer_entry(e, file_size)) break;

        g_outer_entries.push_back(e);

        // Sentinel entry has file_offset==0 && size==0 → no payload to peek.
        if (e.file_offset == 0U && e.size == 0U) {
            ++sentinel_count;
            continue;
        }

        // Histogram: read first 4 bytes of the entry's payload. Each entry's
        // payload byte range was already bounds-checked above by is_valid.
        const uint32_t head = read_u32_le(g_bnd_mmap + e.file_offset);
        if      (head == SIG_BND_LE)  ++sig_bnd_count;
        else if (head == SIG_GZIP_LE) ++sig_gz_count;
        else if (head == SIG_PSMF_LE) ++sig_psmf_count;
        else                          ++sig_other_count;
    }

    // R5 cap enforcement — walk hit the MAX without terminating.
    if (idx >= MAX_BND_ENTRIES) {
        std::fprintf(stderr,
            "[BND_ERR] %s:0x%X: entry walk exceeded MAX_BND_ENTRIES "
            "(%u entries with no terminator — file likely corrupt)\n",
            path, BND_ENTRY_TABLE_OFFSET, MAX_BND_ENTRIES);
        std::abort();
    }

    // ---- PSPRECOMP_BND_TRACE=1 one-shot dump (locked format §3) ---------
    const char* trace_env = std::getenv("PSPRECOMP_BND_TRACE");
    const bool trace = (trace_env != nullptr) && (trace_env[0] == '1');
    if (trace) {
        std::fprintf(stderr,
            "[BND_TRACE] DATA_CMN.BND header: magic=BND\\0 version=%u "
            "+0x14=0x%08X file_size=0x%08zX\n",
            version, data_sec_off, file_size);

        const uint32_t dump_count =
            (g_outer_entries.size() < 9U)
                ? static_cast<uint32_t>(g_outer_entries.size())
                : 9U;
        for (uint32_t i = 0; i < dump_count; ++i) {
            const BndOuterEntry& e = g_outer_entries[i];
            if (e.file_offset == 0U && e.size == 0U) {
                std::fprintf(stderr,
                    "[BND_TRACE] entry[%u]: file_off=0x%08X "
                    "size=0x%08X hash=0x%08X sec=0x%08X [SENTINEL]\n",
                    i, e.file_offset, e.size, e.hash, e.secondary);
            } else {
                const uint32_t head =
                    read_u32_le(g_bnd_mmap + e.file_offset);
                std::fprintf(stderr,
                    "[BND_TRACE] entry[%u]: file_off=0x%08X "
                    "size=0x%08X hash=0x%08X sec=0x%08X "
                    "head=%08x [%s]\n",
                    i, e.file_offset, e.size, e.hash, e.secondary,
                    head, signature_label(head));
            }
        }

        std::fprintf(stderr,
            "[BND_TRACE] entry table terminated at index %u\n",
            static_cast<unsigned>(g_outer_entries.size()));

        std::fprintf(stderr,
            "[BND_TRACE] payload signatures: %u BND, %u GZIP, %u PSMF, "
            "%u unknown (sentinels=%u)\n",
            sig_bnd_count, sig_gz_count, sig_psmf_count, sig_other_count,
            sentinel_count);
    }

    // Plan 11-04: build the virtual-path → entry-index map by walking
    // the names region at [0x1A9F .. 0x4800). Must run AFTER the entry
    // table is populated (the map walk validates `val` against
    // `g_outer_entries.size()`).
    build_virt_path_map();

    // LAYER B correctness gate (doc 15). When PSPRECOMP_BND_CONTENT_CHECK=1,
    // print the PAYLOAD IDENTITY of the entry each loadinggroup asset currently
    // resolves to. With the positional map this prints MISMATCHES (e.g.
    // titledata -> a helmet actor BND), proving the decode is wrong. Any future
    // name->entry decode must make these identities MATCH the asset name.
    bnd_dump_content_check();

    g_bnd_initialized.store(true, std::memory_order_release);
}

// LAYER B correctness gate body (doc 15). See bnd_entry_identity above.
static void bnd_dump_content_check() {
    const char* env = std::getenv("PSPRECOMP_BND_CONTENT_CHECK");
    if (env == nullptr || env[0] != '1') return;

    static const char* const kProbes[] = {
        "loadinggroup/titledata.bnd",
        "loadinggroup/systemdata.bnd",
        "loadinggroup/systemlocalizedata.bnd",
        "loadinggroup/logodata.bnd",
        "loadinggroup/basesdata.bnd",
        "loadinggroup/gamedata.bnd",
        "colony_msg.lbnd",
        "us/tips01.gxt",
        "us/tips13.gxt",
        "us/tips25.gxt",
    };
    std::fprintf(stderr,
        "[BND_CHECK] name -> resolved entry payload identity "
        "(should MATCH the name's meaning):\n");
    for (const char* name : kProbes) {
        // doc 19A — resolve by basename against the on-disk directory table.
        auto it = g_name_to_entry.find(bnd_basename(name));
        if (it == g_name_to_entry.end()) {
            std::fprintf(stderr, "[BND_CHECK]   %-38s -> <unmapped>\n", name);
            continue;
        }
        const BndOuterEntry& e = it->second;
        std::fprintf(stderr,
            "[BND_CHECK]   %-38s -> off=0x%X size=%u :: %s\n",
            name, e.file_offset, e.size,
            bnd_entry_identity(e).c_str());
    }
}

// ============================================================================
// === gzip_inflate (Plan 11-04) ===
// ============================================================================
//
// Clean-room zlib glue (the PPSSPP analog in Core/HLE/sceKernelModule.cpp::
// gzipDecompress is GPL-2.0 — read-only behavioral oracle, NO code copied).
//
// Strategy: read the gzip member's ISIZE trailer (last 4 bytes,
// uncompressed-size mod 2^32) to size the output buffer up front; add a
// 64 KB slack to absorb the mod-2^32 wrap on the unlikely >4 GB asset.
// On `Z_BUF_ERROR` (i.e. ISIZE was wrong or multi-member input), grow
// `out` by 2× and retry up to 4 times — caps total expansion at 16× the
// ISIZE estimate, well below any sane asset's compression ratio. The
// `inflateInit2(stream, 31)` call selects gzip-only wrapping per
// 11-RESEARCH.md §5 (windowBits = 16 + MAX_WBITS = 16 + 15 = 31).
//
// On success returns true and resizes `out` exactly to `stream.total_out`
// (the caller does NOT need to compute the actual size). On any zlib
// failure returns false and the caller is expected to emit `[BND_ERR]`
// + abort per Phase 11 D-09.

static bool gzip_inflate(const uint8_t* src, size_t src_len,
                         std::vector<uint8_t>& out) {
    if (src_len < 4U) return false;  // too small for ISIZE trailer

    // ISIZE — last 4 bytes of the gzip member, little-endian.
    const uint32_t isize = read_u32_le(src + src_len - 4U);

    // Initial buffer: ISIZE + 64 KB slack. 64 KB is the conservative
    // upper bound for gzip metadata overhead (FNAME + FCOMMENT + extra).
    // Use uint64_t to dodge wrap when isize is near UINT32_MAX.
    const uint64_t initial_cap =
        static_cast<uint64_t>(isize) + 65536ULL;
    out.resize(static_cast<size_t>(initial_cap));

    z_stream stream{};
    stream.next_in   = const_cast<Bytef*>(src);
    stream.avail_in  = static_cast<uInt>(src_len);
    stream.next_out  = out.data();
    stream.avail_out = static_cast<uInt>(out.size());

    if (inflateInit2(&stream, 31) != Z_OK) {
        return false;  // 31 = gzip-only wrapping
    }

    for (int attempt = 0; attempt < 4; ++attempt) {
        const int rc = inflate(&stream, Z_FINISH);
        if (rc == Z_STREAM_END) {
            out.resize(static_cast<size_t>(stream.total_out));
            // Pitfall P2: multi-member gzip — log if we did not consume the
            // entire input member (the caller may want to know there are
            // more members on disk that we silently ignored).
            if (stream.total_in < src_len &&
                std::getenv("PSPRECOMP_BND_TRACE") != nullptr) {
                std::fprintf(stderr,
                    "[BND_TRACE] multi-member gzip: total_in=%lu of %zu "
                    "after Z_STREAM_END\n",
                    static_cast<unsigned long>(stream.total_in), src_len);
            }
            inflateEnd(&stream);
            return true;
        }
        if (rc == Z_BUF_ERROR) {
            // Grow the output buffer 2× and retry. avail_out exhausted —
            // re-anchor next_out at the new tail so the stream continues
            // writing where it stopped.
            const size_t old_size  = out.size();
            const size_t new_size  = old_size * 2U;
            out.resize(new_size);
            stream.next_out  = out.data() + stream.total_out;
            stream.avail_out =
                static_cast<uInt>(new_size -
                                  static_cast<size_t>(stream.total_out));
            continue;
        }
        // Any other rc is a hard error.
        inflateEnd(&stream);
        return false;
    }

    // Hit the 4-attempt retry cap — the caller will [BND_ERR] + abort.
    inflateEnd(&stream);
    return false;
}

// ============================================================================
// === build_virt_path_map (Plan 11-04) ===
// ============================================================================
//
// Walks the names region at file offsets [0x1A9F .. 0x4800), building
// `g_virt_to_entry`. Per 11-RESEARCH.md §2.3, the region holds a flat
// sequence of NUL-terminated names; each name is followed by a 5-byte
// record `{flag:u8, val:u32_LE}` plus 2 bytes of `00 00` separator
// padding. When a name ends with `/` and its flag is a directory marker
// (0x02 or 0xfe), it becomes the current directory prefix; otherwise
// the entry's virtual path is `current_dir + name`.
//
// Empirical confirmation: the runtime requests
// `"loadinggroup/systemdata.bnd"`. This string does NOT exist as a single
// NUL-terminated string anywhere on disk — it is assembled by adjacency
// of `loadinggroup/\0` at 0x3CB2 and `systemdata.bnd\0` at 0x3CC7.
//
// --- Entry-index mapping (Rule 1 deviation from plan spec) ---
// The plan claimed `val` (the u32 LE field after `flag`) is the outer
// entry index. Empirical inspection of all 407 file records confirmed
// that NO `val` is < 419 — the encoded values are all in [10^6, 10^9].
// The exact `val → entry_index` semantic is documented as deferred
// research (11-RESEARCH.md §2.3 line 179). Pragmatic substitute used
// here: POSITIONAL mapping. We walk the entry table and the names region
// in lockstep:
//   - Names walker emits file records in disk order (skipping flag=0x02
//     and 0xfe directory headers).
//   - Entry-table walker emits non-sentinel entries in disk order.
//   - The i-th file record maps to the i-th non-sentinel entry.
// Verified locally: 407 file records ↔ 407 non-sentinel entries. The
// resolved entry index for `"loadinggroup/systemdata.bnd"` is 373, whose
// payload starts with the BND\0 magic (correct passthrough target).
// Plan 11-06 will install unit tests that pin this mapping; if a future
// empirical analysis reveals the true `val → entry_index` decoder, the
// positional path is the obvious fallback.
//
// Out-of-range `val` (informational only): skipped silently — the plan
// instructed us to log out-of-range vals, but the positional mapping
// renders this universal (every `val` is "out of range" of `[0..419]`),
// so the log would fire 400+ times. We retain the structural read of
// `flag` and `val` (cursor must advance regardless) but no longer treat
// `val` as authoritative.
//
// Missing NUL within bounds: [BND_ERR] + abort (D-09 malformed file).

static void build_virt_path_map() {
    // doc 19A — REPLACES the old positional (i-th record → i-th entry) map,
    // which was the root cause of every asset mis-resolving (systemlocalizedata
    // → a 2.97 MB mission container that stalled the loader).
    //
    // The authoritative by-name directory is the on-disk table at file offset
    // 0x28 (= the in-memory `base+0x28` table, read raw off disc with no mount
    // transform — see FUN_08861aec). Layout, 16-byte stride, count = header+0x24:
    //     { u32 key @0, u32 name_rel @4, u32 file_offset @8, u32 size @0xc }
    // NAME = NUL-terminated string at `file + name_rel + 7` (after the
    // `[flag][b1][b2][val:u32]` 7-byte trailer). We build `g_name_to_entry`
    // keyed by the bare asset name → {file_offset, size}.
    if (g_bnd_mmap == nullptr) return;

    const uint32_t names_off =
        read_u32_le(g_bnd_mmap + BND_HEADER_NAMES_SEC_OFFSET);   // 0x1A98
    const uint32_t dir_count =
        read_u32_le(g_bnd_mmap + BND_HEADER_DIR_COUNT_OFFSET);   // 0x1A7 = 423

    g_name_to_entry.clear();
    g_name_to_entry.reserve(dir_count);

    for (uint32_t idx = 0; idx < dir_count; ++idx) {
        const uint32_t ep = BND_DIR_TABLE_OFFSET + idx * 16U;    // slot offset
        // The directory table ends exactly where the names region begins.
        if (ep + 16U > names_off) break;
        if (static_cast<uint64_t>(ep) + 16U > g_bnd_mmap_size) break;

        const uint32_t name_rel = read_u32_le(g_bnd_mmap + ep + 4U);
        const uint32_t off      = read_u32_le(g_bnd_mmap + ep + 8U);
        const uint32_t size     = read_u32_le(g_bnd_mmap + ep + 0xCU);

        // name_rel == 0 → unused/sentinel slot; skip.
        if (name_rel == 0U) continue;
        // Name lives at file + name_rel + 7 (7-byte trailer precedes it).
        const uint64_t name_start = static_cast<uint64_t>(name_rel) + 7U;
        if (name_start >= g_bnd_mmap_size) continue;

        const char* const np =
            reinterpret_cast<const char*>(g_bnd_mmap + name_start);
        const size_t remaining =
            static_cast<size_t>(g_bnd_mmap_size - name_start);
        const size_t name_len = ::strnlen(np, remaining);
        if (name_len == 0U || name_len == remaining) continue;

        std::string name(np, name_len);
        // Directory markers (names ending in '/') carry off==size==0; they are
        // not openable assets — skip so they never shadow a real leaf.
        if (name.back() == '/') continue;

        BndOuterEntry e{};
        e.file_offset = off;
        e.size        = size;
        e.hash        = read_u32_le(g_bnd_mmap + ep + 0U);  // content key
        e.secondary   = 0U;
        g_name_to_entry[name] = e;
    }
}

// doc 19A — extract the last path component (basename) from a virtual path,
// splitting on BOTH '/' and '\\' (the engine treats both as separators —
// FUN_0893D8DC). Lowercases ASCII for case-insensitive matching (the runtime
// already lowercases, but be defensive).
static std::string bnd_basename(const char* virt_path) {
    std::string s(virt_path);
    size_t cut = s.find_last_of("/\\");
    std::string base = (cut == std::string::npos) ? s : s.substr(cut + 1U);
    for (char& c : base) {
        if (c >= 'A' && c <= 'Z') c = static_cast<char>(c - 'A' + 'a');
    }
    return base;
}

// doc 19A — process-lifetime storage for entries returned by the name resolver
// (the public API returns `const BndOuterEntry*`, so the pointee must outlive
// the call). One slot per distinct resolved name; written once, read-only after.
static std::unordered_map<std::string, BndOuterEntry> g_resolved_entry_store;

// ============================================================================
// === allocate_shared_noop_vtable (Plan 11-04) ===
// ============================================================================
//
// Returns the PSP-VA of a shared 44-byte (11 × uint32) NOOP vtable in the
// BND arena. Lazily allocated on first call; cached for the process
// lifetime so every descriptor's `desc+0` pointer aliases the same table.
//
// Each slot holds the pre-Phase-11 wrapper's empty-SDK-stub address
// `0x089D76E8` (NOOP_STUB) — same literal as `main.cpp:492` from the
// shared-stub fallback path. This makes any vtable method call from
// recompiled code dispatch to a guaranteed-empty function instead of
// crashing. Reusing one vtable per process (rather than per descriptor)
// is sufficient per 11-RESEARCH.md §7 line 502.

static uint32_t allocate_shared_noop_vtable(uint8_t* rdram) {
    if (g_shared_noop_vtable_addr != 0U) {
        return g_shared_noop_vtable_addr;
    }

    // PATAPON(P6) TODO(#47): per-game stub-address baking -- this whole BND
    // asset layer moves to games/patapon/ in portability Phase 5; the filler
    // address must then come from per-game data, not a literal.
    constexpr uint32_t NOOP_STUB     = 0x089D76E8U;
    constexpr uint32_t VTABLE_SIZE   = 44U;   // 11 × uint32
    constexpr uint32_t VTABLE_ALIGN  = 16U;
    constexpr uint32_t VTABLE_SLOTS  = 11U;

    const uint32_t addr =
        psp_alloc_bnd_arena(VTABLE_SIZE, VTABLE_ALIGN);
    if (addr == 0U) {
        std::fprintf(stderr,
            "[BND_ERR] arena OOM allocating shared NOOP vtable "
            "(size=%u align=%u)\n", VTABLE_SIZE, VTABLE_ALIGN);
        std::abort();
    }

    for (uint32_t slot = 0; slot < VTABLE_SLOTS; ++slot) {
        psp_mem_write<uint32_t>(rdram, addr + slot * 4U, NOOP_STUB);
    }

    g_shared_noop_vtable_addr = addr;
    return addr;
}

// ============================================================================
// === bnd_resolve_and_allocate (Plan 11-04) ===
// ============================================================================
//
// Real lookup body. Returns 0 on miss (idx_str not in `g_virt_to_entry`)
// — the wrapper in Plan 11-05 falls back to `fallback_to_shared_stub`.
//
// Steps:
//   1. Bail-out checks (init failed, init not yet run).
//   2. Cache lookup (`g_idx_to_desc`).
//   3. Virtual-path lookup (`g_virt_to_entry`).
//   4. Bounds check the entry's payload range.
//   5. Signature dispatch on first 4 bytes:
//        BND\0  → passthrough
//        gzip   → gzip_inflate
//        PSMF   → passthrough (P7)
//        other  → [BND_ERR] + abort
//   6. Allocate payload buffer (256-byte aligned).
//   7. memcpy payload into rdram using PSP_ADDR_MASK.
//   8. Allocate 16-byte descriptor (16-byte aligned).
//   9. Write descriptor fields: {vtable_ptr, size, payload_addr, 0}.
//  10. Cache and return.

uint32_t bnd_resolve_and_allocate(uint8_t* rdram, const char* idx_str) {
    // Step 1: bail-out guards.
    if (g_bnd_unavailable.load(std::memory_order_acquire)) return 0U;
    if (!g_bnd_initialized.load(std::memory_order_acquire)) return 0U;
    if (idx_str == nullptr) return 0U;

    // Step 2: cache lookup. R4 — repeated resolutions return the same
    // descriptor without re-inflating.
    auto cache_it = g_idx_to_desc.find(idx_str);
    if (cache_it != g_idx_to_desc.end()) {
        return cache_it->second;
    }

    // Step 3: by-NAME lookup against the on-disk directory table (doc 19A).
    // Miss → caller falls back to shared stub.
    const std::string base = bnd_basename(idx_str);
    auto it = g_name_to_entry.find(base);
    if (it == g_name_to_entry.end()) {
        return 0U;
    }
    const uint32_t entry_idx = 0U;  // diagnostic only (name-keyed now)
    const BndOuterEntry& e = it->second;

    // Step 4: bounds check (uint64 to dodge wrap on near-4 GB values).
    const uint64_t end_off =
        static_cast<uint64_t>(e.file_offset) +
        static_cast<uint64_t>(e.size);
    if (end_off > static_cast<uint64_t>(g_bnd_mmap_size)) {
        std::fprintf(stderr,
            "[BND_ERR] entry %u out-of-bounds: file_off=0x%X size=%u "
            "file_size=0x%zX (\"%s\")\n",
            entry_idx, e.file_offset, e.size, g_bnd_mmap_size, idx_str);
        std::abort();
    }

    // Step 5: signature dispatch on first 4 bytes.
    const uint32_t sig = read_u32_le(g_bnd_mmap + e.file_offset);
    const uint8_t  first_byte = static_cast<uint8_t>(sig & 0xFFU);

    std::vector<uint8_t> decompressed;

    switch (sig) {
        case SIG_BND_LE:  // 0x00444E42 — uncompressed nested BND
            decompressed.assign(g_bnd_mmap + e.file_offset,
                                g_bnd_mmap + e.file_offset + e.size);
            break;

        case SIG_PSMF_LE:  // 0x464D5350 — PlayStation Movie Format
            decompressed.assign(g_bnd_mmap + e.file_offset,
                                g_bnd_mmap + e.file_offset + e.size);
            if (std::getenv("PSPRECOMP_BND_TRACE") != nullptr) {
                std::fprintf(stderr,
                    "[BND_TRACE] PSMF passthrough for \"%s\" size=%u\n",
                    idx_str, e.size);
            }
            break;

        default:
            if (first_byte == 0x1FU) {
                // gzip member (0x1F 0x8B 0x08 ...) — full 4-byte magic
                // varies (FNAME flag, FHCRC flag, etc.) so we route on
                // the first byte alone to admit any well-formed header.
                if (!gzip_inflate(g_bnd_mmap + e.file_offset, e.size,
                                  decompressed)) {
                    std::fprintf(stderr,
                        "[BND_ERR] gzip inflate failed for \"%s\" at "
                        "entry %u (file_off=0x%X size=%u)\n",
                        idx_str, entry_idx, e.file_offset, e.size);
                    std::abort();
                }
            } else {
                std::fprintf(stderr,
                    "[BND_ERR] \"%s\": unknown payload signature 0x%08X "
                    "at entry %u (offset 0x%X)\n",
                    idx_str, sig, entry_idx, e.file_offset);
                std::abort();
            }
            break;
    }

    if (decompressed.empty()) {
        std::fprintf(stderr,
            "[BND_ERR] empty payload for \"%s\" at entry %u\n",
            idx_str, entry_idx);
        std::abort();
    }

    // Step 6: allocate payload buffer (256-byte aligned per D-04).
    const uint32_t payload_addr =
        psp_alloc_bnd_arena(static_cast<uint32_t>(decompressed.size()),
                            256U);
    if (payload_addr == 0U) {
        std::fprintf(stderr,
            "[BND_ERR] arena OOM allocating payload for \"%s\" "
            "(size=%zu)\n", idx_str, decompressed.size());
        std::abort();
    }

    // Step 7: memcpy into rdram. CLAUDE.md rule 4: mask the PSP-VA.
    std::memcpy(rdram + (payload_addr & PSP_ADDR_MASK),
                decompressed.data(), decompressed.size());

    // Step 8: allocate 16-byte descriptor (16-byte aligned).
    const uint32_t desc_addr = psp_alloc_bnd_arena(16U, 16U);
    if (desc_addr == 0U) {
        std::fprintf(stderr,
            "[BND_ERR] arena OOM allocating descriptor for \"%s\"\n",
            idx_str);
        std::abort();
    }

    // Step 9: shared NOOP vtable, then descriptor fields. Layout:
    //   +0: vtable PSP-VA  (shared 11-entry NOOP table)
    //   +4: size           (decompressed payload size in bytes)
    //   +8: data_offset    (payload PSP-VA)
    //  +12: 0              (reserved / inner-asset count placeholder)
    const uint32_t vtable = allocate_shared_noop_vtable(rdram);
    psp_mem_write<uint32_t>(rdram, desc_addr + 0U,  vtable);
    psp_mem_write<uint32_t>(rdram, desc_addr + 4U,
                            static_cast<uint32_t>(decompressed.size()));
    psp_mem_write<uint32_t>(rdram, desc_addr + 8U,  payload_addr);
    psp_mem_write<uint32_t>(rdram, desc_addr + 12U, 0U);

    // Step 10: cache and return.
    g_idx_to_desc[idx_str] = desc_addr;

    if (std::getenv("PSPRECOMP_BND_TRACE") != nullptr) {
        std::fprintf(stderr,
            "[BND_TRACE] idx=\"%s\" entry=%u file_off=0x%X size=%u "
            "payload=0x%X desc=0x%X\n",
            idx_str, entry_idx, e.file_offset,
            static_cast<uint32_t>(decompressed.size()),
            payload_addr, desc_addr);
    }

    return desc_addr;
}

// ============================================================================
// === fallback_to_shared_stub (Plan 11-05) ===
// ============================================================================
//
// Body lifted verbatim from runtime/src/main.cpp lines 466-560 (pre-Phase-11
// wrapper). All semantics preserved:
//
//   1. PSPRECOMP_ASSET_NO_STUB=1 → chain through to recompiled original
//      (via `g_asset_lookup_orig`, populated by main.cpp). Returns
//      immediately; no descriptor allocation.
//
//   2. First call (g_fallback_stub_desc == 0): allocate a 16-byte
//      AssetDescriptor + 44-byte noop_vtable via psp_alloc_kernel_memory
//      (NOT the BND arena — legacy path keeps its legacy allocator). The
//      vtable has 11 entries, each pointing at NOOP_STUB (0x089D76E8).
//      The descriptor layout is:
//        +0  vtable_addr
//        +4  size_hint (from PSPRECOMP_ASSET_SIZE_HINT or 0)
//        +8  0
//        +12 0
//      Cached in g_fallback_stub_desc for the process lifetime.
//
//   3. PSPRECOMP_ASSET_FORCE_MISS=1 → ctx->r[2] = 0 (diagnostic A/B path).
//      Otherwise ctx->r[2] = g_fallback_stub_desc.
//
//   4. Rate-limited `[ASSET_STUB]` log: first 8 calls + every 500th.
//
// Pre-condition: `idx_str` points at a NUL-terminated key string (the
// recompiled function's a1 argument, mapped through rdram). The pointer
// targets host memory (already copied out of rdram by the caller), so we
// do not apply PSP_ADDR_MASK here.
//
// `bucket_count` is provided for log context but unused by the allocation
// logic — the wrapper handled the `bucket_count != 0 → chain to original`
// path BEFORE reaching here.

void fallback_to_shared_stub(uint8_t* rdram, recomp_context* ctx,
                             const char* idx_str, uint32_t bucket_count) {
    // PSPRECOMP_ASSET_NO_STUB=1 short-circuit: defer to the recompiled
    // original. The wrapper in main.cpp normally screens this flag itself
    // BEFORE calling us, but we re-honor it here so the BND-DISABLE path
    // (which routes here unconditionally) also respects the override.
    const char* no_stub_env = std::getenv("PSPRECOMP_ASSET_NO_STUB");
    if (no_stub_env != nullptr && no_stub_env[0] == '1') {
        if (g_asset_lookup_orig != nullptr) {
            g_asset_lookup_orig(rdram, ctx);
        }
        return;
    }

    // Lazy allocate the shared 60-byte stub on first call.
    if (g_fallback_stub_desc == 0U) {
        // PATAPON(P6) TODO(#47): same per-game literal as
        // allocate_shared_noop_vtable -- leaves with the file in Phase 5.
        constexpr uint32_t NOOP_STUB = 0x089D76E8U;
        uint32_t vtable_addr = psp_alloc_kernel_memory(44);
        uint32_t desc_addr   = psp_alloc_kernel_memory(16);
        if (vtable_addr != 0U && desc_addr != 0U) {
            for (int i = 0; i < 11; ++i) {
                psp_mem_write<uint32_t>(rdram,
                    vtable_addr + static_cast<uint32_t>(i * 4),
                    NOOP_STUB);
            }
            psp_mem_write<uint32_t>(rdram, desc_addr + 0U,  vtable_addr);

            // PSPRECOMP_ASSET_SIZE_HINT=N sets desc+4 to N so the gatekeeper
            // FUN_088623E0's zero-size fast-exit branch is skipped and the
            // game attempts a real data read. Pre-Phase-11 default = 0.
            const char* size_env = std::getenv("PSPRECOMP_ASSET_SIZE_HINT");
            uint32_t size_hint = 0U;
            if (size_env != nullptr) {
                size_hint = static_cast<uint32_t>(
                    std::strtoul(size_env, nullptr, 0));
            }
            psp_mem_write<uint32_t>(rdram, desc_addr + 4U,  size_hint);
            psp_mem_write<uint32_t>(rdram, desc_addr + 8U,  0U);
            psp_mem_write<uint32_t>(rdram, desc_addr + 12U, 0U);

            g_fallback_stub_desc = desc_addr;
            std::fprintf(stderr,
                "[ASSET_STUB] Allocated stub AssetDescriptor at 0x%08X "
                "(vtable 0x%08X -> %d x 0x%08X)\n",
                desc_addr, vtable_addr, 11, NOOP_STUB);
        }
    }

    // PSPRECOMP_ASSET_FORCE_MISS=1 returns 0 even after allocation; useful
    // for comparing game behavior under "all assets return 0" vs
    // "all assets return synthetic stub".
    const char* force_miss_env = std::getenv("PSPRECOMP_ASSET_FORCE_MISS");
    bool force_miss = force_miss_env != nullptr && force_miss_env[0] == '1';
    uint32_t result = force_miss ? 0U : g_fallback_stub_desc;
    ctx->r[2] = static_cast<int32_t>(result);

    g_fallback_fm_count++;
    if (g_fallback_fm_count <= 8 || g_fallback_fm_count % 500 == 0) {
        std::fprintf(stderr,
            "[ASSET_STUB] \"%s\" bucket=%u -> 0x%08X (#%d)\n",
            idx_str, bucket_count, result, g_fallback_fm_count);
    }
}

// ============================================================================
// === Test Accessors (Plan 11-06) ===
// ============================================================================
//
// Read-only views over the file-scope statics, exposed exclusively for the
// `test_asset_bnd` unit-test executable. The runtime proper never calls
// these — it uses `bnd_resolve_and_allocate`. Keeping the statics file-scope
// (rather than promoting them to public globals) preserves encapsulation.
//
// Thread safety: `g_outer_entries` and `g_virt_to_entry` are written exactly
// once during `bnd_init` and read-only afterward. The accessors are
// therefore lock-free as long as the caller respects the `bnd_init`-then-
// query ordering.

size_t bnd_get_outer_entry_count() {
    return g_outer_entries.size();
}

const BndOuterEntry* bnd_get_outer_entry(size_t idx) {
    if (idx >= g_outer_entries.size()) {
        return nullptr;
    }
    return &g_outer_entries[idx];
}

bool bnd_virt_path_resolves(const char* virt_path) {
    if (virt_path == nullptr) return false;
    // doc 19A — resolve by BASENAME against the on-disk directory table.
    return g_name_to_entry.count(bnd_basename(virt_path)) != 0U;
}
const BndOuterEntry* bnd_find_outer_entry_for_virt_path(const char* virt_path) {
    if (virt_path == nullptr) return nullptr;
    // doc 19A — resolve by BASENAME (leaf names are unique in DATA_CMN.BND)
    // against the authoritative on-disk directory table at file offset 0x28.
    const std::string base = bnd_basename(virt_path);
    auto it = g_name_to_entry.find(base);
    if (it == g_name_to_entry.end()) return nullptr;
    // Cache the entry in process-lifetime storage so the returned pointer
    // outlives this call (the map value would dangle if returned by ref to a
    // temporary). Keyed by basename so repeated lookups return the same slot.
    auto stored = g_resolved_entry_store.emplace(base, it->second);
    return &stored.first->second;
}


uint32_t bnd_get_entry_payload_signature(size_t idx) {
    if (idx >= g_outer_entries.size()) return 0U;
    if (g_bnd_mmap == nullptr) return 0U;
    const BndOuterEntry& e = g_outer_entries[idx];
    if (e.file_offset == 0U && e.size == 0U) return 0U;  // sentinel
    if (e.size < 4U) return 0U;
    if (static_cast<uint64_t>(e.file_offset) + 4ULL >
        static_cast<uint64_t>(g_bnd_mmap_size)) {
        return 0U;
    }
    return read_u32_le(g_bnd_mmap + e.file_offset);
}

const char* bnd_find_virt_path_for_entry(uint32_t entry_idx) {
    for (const auto& kv : g_virt_to_entry) {
        if (kv.second == entry_idx) {
            return kv.first.c_str();
        }
    }
    return nullptr;
}
