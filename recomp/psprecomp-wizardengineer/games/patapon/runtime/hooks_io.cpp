// games/patapon/runtime/hooks_io.cpp — Patapon IO policy (issues #46 / #47
// Phase 5).
//
// Moved verbatim from runtime/src/hle/psp_hle_io.cpp: the DATA_CMN.BND
// archive-backing resolution for missing async opens, the BND IO-slot
// staging (Patapon async-slot struct offsets +20/+292/+300/+308), the
// degenerate "BND\0" extraction-artifact rejection, the SetAsyncCallback
// slot-field diagnostics, and the SGXD NULL-path tripwire. The generic IO
// layer reaches all of it through the PspIoPolicy seam (psp_hle_io.h).
// Compiled only under -DPSPRECOMP_GAME=patapon.

#include "recomp.h"
#include "psp_memory.h"
#include "hle/psp_hle.h"
#include "hle/psp_hle_io.h"
#include "asset_bnd.h"
#include "patapon_hooks.h"

#include <cstdio>
#include <cstring>
#include <cctype>
#include <string>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <pthread.h>

extern thread_local uint32_t g_last_func_addr;

static constexpr const char* k_data_cmn_bnd_path =
    "disc0:/PSP_GAME/USRDIR/DATA_CMN.BND";

static bool lookup_bnd_backing_for_psp_path(
    const char* psp_path, uint32_t* size_out, uint32_t* off_out
) {
    if (psp_path == nullptr || *psp_path == '\0') return false;

    const char* candidates[4] = {psp_path, nullptr, nullptr, nullptr};
    const char* disc0 = std::strstr(psp_path, "disc0:");
    if (disc0 == psp_path) candidates[1] = psp_path + 6;
    const char* usrdir = std::strstr(psp_path, "USRDIR/");
    if (usrdir != nullptr) candidates[2] = usrdir + 7;
    const char* loadinggroup = std::strstr(psp_path, "loadinggroup/");
    if (loadinggroup != nullptr) candidates[3] = loadinggroup;

    for (const char* candidate : candidates) {
        if (candidate == nullptr || *candidate == '\0') continue;
        std::string normalized(candidate);
        for (char& ch : normalized) {
            ch = static_cast<char>(
                std::tolower(static_cast<unsigned char>(ch)));
        }
        if (const BndOuterEntry* outer =
                bnd_find_outer_entry_for_virt_path(normalized.c_str())) {
            *size_out = outer->size;
            *off_out = outer->file_offset;
            return true;
        }
    }
    return false;
}

static bool detect_bnd_backed_async_slot(
    uint8_t* rdram, int callback_arg, uint32_t* slot_ptr_out,
    uint32_t* size_out, uint32_t* off_out
) {
    if (callback_arg == 0) return false;
    uint32_t slot_ptr = static_cast<uint32_t>(callback_arg);
    if (slot_ptr < 0x08000000U || slot_ptr >= 0x0A000000U) return false;

    int32_t slot_idx = static_cast<int32_t>(
        psp_mem_read<uint32_t>(rdram, slot_ptr + 20));
    uint32_t size = psp_mem_read<uint32_t>(rdram, slot_ptr + 300);
    uint32_t off = psp_mem_read<uint32_t>(rdram, slot_ptr + 308);
    if (slot_idx >= 0 || size == 0U || off == 0U) return false;

    *slot_ptr_out = slot_ptr;
    *size_out = size;
    *off_out = off;
    return true;
}

static uint32_t ensure_bnd_slot_buffer(
    uint8_t* rdram, uint32_t slot_ptr, uint32_t size
) {
    uint32_t buf = psp_mem_read<uint32_t>(rdram, slot_ptr + 292);
    if (buf != 0U) return buf;

    buf = psp_alloc_bnd_arena(size, 256U);
    if (buf == 0U) return 0U;

    psp_mem_write<uint32_t>(rdram, slot_ptr + 292, buf);
    return buf;
}

/// PspIoPolicy::prepare_async_slot — stage the game's async IO slot for an
/// archive-backed read (allocate the BND-arena buffer into slot+292).
static void patapon_prepare_async_slot(
    uint8_t* rdram, const char* psp_path, int callback_arg
) {
    uint32_t expected_size = 0U;
    uint32_t expected_off = 0U;
    if (!lookup_bnd_backing_for_psp_path(
            psp_path, &expected_size, &expected_off)) {
        return;
    }

    uint32_t slot_ptr = 0U;
    uint32_t size = 0U;
    uint32_t off = 0U;
    if (!detect_bnd_backed_async_slot(
            rdram, callback_arg, &slot_ptr, &size, &off)) {
        return;
    }

    if (size != expected_size || off != expected_off) return;
    uint32_t buf = ensure_bnd_slot_buffer(rdram, slot_ptr, size);
    if (buf == 0U) return;

    static int prep_count = 0;
    if (++prep_count <= 12) {
        std::fprintf(stderr,
            "[HLE] prepared BND slot=0x%08X off=%u size=%u buf=0x%08X\n",
            slot_ptr, off, size, buf);
    }
}

/// PspIoPolicy::resolve_archive_backing — resolve a missing async-open path
/// to its slice [off, off+size) inside DATA_CMN.BND.
static bool patapon_resolve_archive_backing(
    const char* psp_path, std::string* host_container,
    uint32_t* size_out, uint32_t* off_out
) {
    if (!lookup_bnd_backing_for_psp_path(psp_path, size_out, off_out)) {
        return false;
    }
    *host_container = psp_path_to_host(k_data_cmn_bnd_path);
    return !host_container->empty();
}

// Some loose-file group directories (e.g. Patapon's LOADINGGROUP/*.BND) do NOT
// exist on the retail ISO at all -- the game streams those assets from a packed
// archive (DATA_CMN.BND) instead. Tools that unpack an ISO to a host directory
// sometimes leave behind degenerate 4-byte "BND\0" placeholder files for these
// entries. On real hardware / PPSSPP the open of such a path FAILS (ENOENT),
// which steers the game's asset state machine down the packed-archive path.
//
// If we let the open succeed against the 4-byte stub, sceIoLseek(SEEK_END)
// returns 4, which poisons the asset object's region-size field (obj+176) and
// makes the loader skip the real region read forever. So we treat a degenerate
// "BND\0" stub as non-existent, matching the retail disc.
static bool patapon_is_extraction_artifact(const char* host_path) {
    struct stat st;
    if (::stat(host_path, &st) != 0) return false;
    if (st.st_size != 4) return false;
    int fd = ::open(host_path, O_RDONLY);
    if (fd < 0) return false;
    char hdr[4] = {0};
    ssize_t n = ::read(fd, hdr, 4);
    ::close(fd);
    if (n != 4) return false;
    return hdr[0] == 'B' && hdr[1] == 'N' && hdr[2] == 'D' && hdr[3] == '\0';
}

/// PspIoPolicy::on_set_async_callback — IO-slot field diagnostics keyed to
/// Patapon's async-slot struct layout. psp_path is nullptr when the fd is
/// not in the fd table.
static void patapon_on_set_async_callback(
    uint8_t* rdram, int fd, int callback_arg, const char* psp_path
) {
    if (fd >= 3 && callback_arg != 0) {
        uint32_t slot_ptr = static_cast<uint32_t>(callback_arg);
        uint32_t state    = psp_mem_read<uint32_t>(rdram, slot_ptr + 0);
        uint32_t ap       = psp_mem_read<uint32_t>(rdram, slot_ptr + 12);
        uint32_t slot_fd  = psp_mem_read<uint32_t>(rdram, slot_ptr + 16);
        uint32_t saved_st = psp_mem_read<uint32_t>(rdram, slot_ptr + 324);
        uint32_t file_sz  = psp_mem_read<uint32_t>(rdram, slot_ptr + 300);
        uint32_t off_292  = psp_mem_read<uint32_t>(rdram, slot_ptr + 292);
        uint32_t off_296  = psp_mem_read<uint32_t>(rdram, slot_ptr + 296);
        uint32_t off_304  = psp_mem_read<uint32_t>(rdram, slot_ptr + 304);
        uint32_t off_308  = psp_mem_read<uint32_t>(rdram, slot_ptr + 308);
        uint32_t off_312  = psp_mem_read<uint32_t>(rdram, slot_ptr + 312);
        uint32_t off_316  = psp_mem_read<uint32_t>(rdram, slot_ptr + 316);
        std::fprintf(stderr,
            "[IO_DIAG] SetAsyncCallback fd=%d cbArg=0x%08X "
            "state=%u ap=%u sfd=%u saved=%u "
            "fs=%u [292]=%u [296]=%u [304]=%u [308]=%u [312]=%u [316]=%u "
            "caller=0x%08X\n",
            fd, callback_arg, state, ap, slot_fd, saved_st,
            file_sz, off_292, off_296, off_304, off_308, off_312, off_316,
            g_last_func_addr);
        if (psp_path != nullptr && off_308 != 0U && off_292 == 0U) {
            static int bnd_slot_path_logs = 0;
            if (++bnd_slot_path_logs <= 12) {
                std::fprintf(stderr,
                    "[IO_DIAG_PATH] fd=%d psp_path=\"%s\"\n",
                    fd, psp_path);
            }
        }
    }
}

/// PspIoPolicy::on_null_path_open — issue-#15 tripwire: a NULL-path open
/// means the rare bad-descriptor race fired (a stream voice started in file
/// mode with no path). Dump the SGXD bank descriptors and stream-object
/// state once, so the race is fully captured without a debugger.
static void patapon_on_null_path_open(uint8_t* rdram) {
    static bool tripwire_fired = false;
    if (tripwire_fired) return;
    tripwire_fired = true;
    auto rd32 = [&](uint32_t a) -> uint32_t {
        uint32_t v;
        std::memcpy(&v, rdram + (a & PSP_ADDR_MASK), 4);
        return v;
    };
    uint32_t banks = rd32(0x08A51FC8);
    uint32_t strms = rd32(0x08A538B8);
    std::fprintf(stderr,
        "[IO_NULL_TRIPWIRE] tid=%p banks=0x%08X streams=0x%08X\n",
        (void*)pthread_self(), banks, strms);
    for (int i = 0; banks && i < 4; i++) {
        uint32_t b = banks + i * 0x1A8;
        std::fprintf(stderr,
            "[IO_NULL_TRIPWIRE] bank[%d]@0x%08X +0x0c=0x%08X "
            "desc{type=0x%08X off=0x%08X path=0x%08X +0x98=0x%08X}\n",
            i, b, rd32(b + 0x0C), rd32(b + 0x8C), rd32(b + 0x90),
            rd32(b + 0x94), rd32(b + 0x98));
    }
    for (int i = 0; strms && i < 4; i++) {
        uint32_t s = strms + i * 0x60B0;
        uint32_t state = rd32(s + 0x34);
        std::fprintf(stderr,
            "[IO_NULL_TRIPWIRE] stream[%d]@0x%08X path(+0x20)=0x%08X "
            "+0x34..0x3b=0x%08X 0x%08X\n",
            i, s, rd32(s + 0x20), state, rd32(s + 0x38));
    }
}

void patapon_install_io_policy() {
    PspIoPolicy policy{};
    policy.resolve_archive_backing = patapon_resolve_archive_backing;
    policy.prepare_async_slot = patapon_prepare_async_slot;
    policy.is_extraction_artifact = patapon_is_extraction_artifact;
    policy.on_set_async_callback = patapon_on_set_async_callback;
    policy.on_null_path_open = patapon_on_null_path_open;
    psp_io_set_policy(policy);
}
