#include "hle/psp_hle.h"
#include "hle/psp_hle_io.h"
#include "hle/psp_hle_kernel.h"
#include "psp_memory.h"
#include "psp_scheduler.h"
#include "recomp.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <set>
#include <unordered_map>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <pthread.h>
#include <cerrno>

// ---- State ----
static std::string g_disc0_path;
static std::unordered_map<int, PspFileDesc> g_fd_table;
static int g_next_fd = PSP_FD_BASE;

// ---- Pre-registration state ----
// When sceIoSetAsyncCallback is called with fd=0 (before OpenAsync is called),
// the game is pre-registering the callback for the NEXT async operation.
// On real PSP hardware the "fd" parameter is the actual async fd that will be
// opened; in our model we allocate a new fd in OpenAsync.  Store the pending
// registration here and apply it to the next sceIoOpenAsync fd.
static int g_pending_prereg_callbackId  = 0;
static int g_pending_prereg_callbackArg = 0;

// Last known cbArg per callbackId — used to inherit cbArg when
// sceIoSetAsyncCallback is called with cbArg=0 (the game omits it when
// the slot pointer hasn't changed between operations).
static std::unordered_map<int, int> g_last_cbarg_for_cbid;

extern thread_local uint32_t g_last_func_addr;

// PSP current working directory (PPSSPP MetaFileSystem.cpp:475)
// For ISO games, defaults to disc0:/PSP_GAME/USRDIR
static std::string g_cwd = "disc0:/PSP_GAME/USRDIR";

// ---- Game-module IO policy (issue #47 Phase 5 seam) ----
// Zero-initialized = generic defaults (no archive reroute, no slot staging,
// no artifact filtering, no extra diagnostics). The game module installs
// its policy from register_hooks.
static PspIoPolicy g_io_policy{};

void psp_io_set_policy(const PspIoPolicy& policy) {
    g_io_policy = policy;
}

// ---- Init ----

void psp_io_init(const char* disc0_host_path) {
    g_disc0_path = disc0_host_path ? disc0_host_path : "./disc0";
    std::fprintf(stderr, "[IO] disc0 mapped to: %s\n",
                 g_disc0_path.c_str());
}

// ---- Path Mapping ----

std::string psp_path_to_host(const char* psp_path) {
    // NULL or empty PSP path is never a valid file. Returning "" routes the
    // caller into its host_path.empty() -> ENOENT branch — without this, an
    // empty path resolved relative to the CWD ends up naming the USRDIR
    // *directory*, which macOS ::open() happily opens (the mysterious
    // 160-byte APFS dir), leaking one host fd per retry.
    if (psp_path == nullptr || *psp_path == '\0') {
        return "";
    }

    std::string p(psp_path);

    // Normalize separators
    for (auto& c : p) {
        if (c == '\\') c = '/';
    }

    // host0: -> not mounted (devkit host filesystem; absent on retail
    // hardware, same as ms0:/flash0: below). ENOENT is the faithful
    // retail answer for every game.
    if (p.substr(0, 6) == "host0:") {
        return "";
    }

    // disc0:/ -> host directory
    if (p.substr(0, 7) == "disc0:/") {
        return g_disc0_path + "/" + p.substr(7);
    }
    if (p.substr(0, 6) == "disc0:") {
        return g_disc0_path + "/" + p.substr(6);
    }

    // umd0:/ -> same as disc0
    if (p.substr(0, 6) == "umd0:/") {
        return g_disc0_path + "/" + p.substr(6);
    }

    // ms0: / flash0: -> not mounted
    if (p.substr(0, 4) == "ms0:" ||
        p.substr(0, 7) == "flash0:") {
        return "";
    }

    // No device prefix -> resolve relative to CWD
    if (p.find(':') == std::string::npos) {
        std::string resolved = g_cwd + "/" + p;
        return psp_path_to_host(resolved.c_str());
    }

    // Unknown prefix
    return "";
}

// ---- Sync HLE Functions ----

/// Route a missing async open through the game's container archive (issue
/// #47 Phase 5 seam): the policy resolves the path to a slice
/// [off, off+size) of a host container file; the slice-fd mechanics
/// (slice_base/slice_len, SEEK_END = asset size, read clamping) are
/// mechanism-generic and stay here. Returns false (no reroute) when no
/// policy is installed or the path has no archive backing.
static bool try_route_missing_async_open_to_archive(
    const char* psp_path, uint8_t* rdram, int psp_fd,
    int prereg_cbId, int prereg_cbArg, recomp_context* ctx
) {
    if (g_io_policy.resolve_archive_backing == nullptr) return false;

    uint32_t size = 0U;
    uint32_t off = 0U;
    std::string host_path;
    if (!g_io_policy.resolve_archive_backing(
            psp_path, &host_path, &size, &off)) {
        return false;
    }
    if (host_path.empty()) return false;

    int host_fd = ::open(host_path.c_str(), O_RDONLY);
    if (host_fd < 0) return false;

    // Record the asset's slice [off, off+size) within the container. The game
    // navigates the rerouted fd by ABSOLUTE container offsets, so seeks/reads
    // pass through to the host fd unchanged; the slice is used to (a) answer
    // SEEK_END with the asset's true end (so the size-probe no longer returns
    // the container size, which a prior band-aid capped to 512KB and
    // truncated the asset) and (b) clamp reads to the asset boundary.
    if (g_io_policy.prepare_async_slot != nullptr) {
        g_io_policy.prepare_async_slot(rdram, psp_path, prereg_cbArg);
    }
    PspFileDesc desc{};
    desc.host_fd = host_fd;
    desc.psp_path = std::string(psp_path);
    desc.is_dir = false;
    desc.dir_handle = nullptr;
    desc.asyncResult = static_cast<int64_t>(psp_fd);
    desc.asyncPending = true;
    desc.closePending = false;
    desc.callbackId = prereg_cbId;
    desc.callbackArg = prereg_cbArg;
    desc.is_slice = true;
    desc.slice_base = static_cast<int64_t>(off);
    desc.slice_len = static_cast<int64_t>(size);
    g_fd_table[psp_fd] = desc;
    std::fprintf(stderr,
        "[HLE] sceIoOpenAsync reroute \"%s\" -> %s fd=%d off=%u size=%u "
        "(sliced view)\n",
        psp_path, host_path.c_str(), psp_fd, off, size);
    ctx->r[2] = psp_fd;
    return true;
}


// Rate-limit repetitive IO log messages (e.g. FileThread retry loop)
static int g_io_open_log_count = 0;
static constexpr int IO_LOG_LIMIT = 100;

// ISO-extraction tools sometimes leave host files that do NOT exist on the
// retail disc (e.g. degenerate placeholder stubs for archive-packed paths).
// On real hardware the open of such a path FAILS (ENOENT), steering the
// game's asset state machine down its packed-archive path. What counts as
// an artifact is a per-game format decision — the policy decides (issue
// #47 Phase 5 seam); generic default: nothing is filtered.
static bool is_extraction_artifact(const std::string& host_path) {
    return g_io_policy.is_extraction_artifact != nullptr
        && g_io_policy.is_extraction_artifact(host_path.c_str());
}

// ---- Async Callback Notification ----
// After each async op completes, notify the fd's registered callback.
static void notify_fd_callback(int fd) {
    auto it = g_fd_table.find(fd);
    if (it != g_fd_table.end() && it->second.callbackId != 0) {
        psp_kernel_notify_callback(
            it->second.callbackId,
            it->second.callbackArg);
    }
}

static void hle_sceIoOpen(
    uint8_t* rdram, recomp_context* ctx
) {
    uint32_t path_ptr = static_cast<uint32_t>(ctx->r[4]);
    int32_t flags = ctx->r[5];
    int32_t mode = ctx->r[6];

    // Faithful: real firmware / PPSSPP (sceIo.cpp:1574ff) rejects a NULL
    // path pointer immediately — no delay, no fd allocation.
    if (path_ptr == 0) {
        if (g_io_open_log_count < IO_LOG_LIMIT) {
            std::fprintf(stderr,
                "[HLE] sceIoOpen NULL path -> 0x80010002 caller=0x%08X\n",
                g_last_func_addr);
            g_io_open_log_count++;
        }
        // Game-module diagnostic (#47 P5 seam): e.g. Patapon's issue-#15
        // SGXD bad-descriptor tripwire (the addresses live game-side).
        if (g_io_policy.on_null_path_open != nullptr) {
            g_io_policy.on_null_path_open(rdram);
        }
        ctx->r[2] = SCE_ERROR_ERRNO_ENOENT;
        return;
    }

    const char* psp_path = reinterpret_cast<const char*>(
        rdram + (path_ptr & PSP_ADDR_MASK));

    std::string host_path = psp_path_to_host(psp_path);

    if (g_io_open_log_count < IO_LOG_LIMIT) {
        std::fprintf(stderr,
                     "[HLE] sceIoOpen(\"%s\", 0x%X, 0x%X) caller=0x%08X path_ptr=0x%08X\n",
                     psp_path, flags, mode, g_last_func_addr, path_ptr);
        g_io_open_log_count++;
        if (g_io_open_log_count == IO_LOG_LIMIT) {
            std::fprintf(stderr,
                "[HLE] (further sceIoOpen logs suppressed)\n");
        }
    }
    if (host_path.empty()) {
        ctx->r[2] = SCE_ERROR_ERRNO_ENOENT;
        sched_yield_point();
        return;
    }

    // Reject extraction-artifact stubs (absent on the retail disc) so the game
    // takes the packed-archive asset path, exactly like real hardware/PPSSPP.
    if (!(flags & PSP_O_CREAT) && is_extraction_artifact(host_path)) {
        ctx->r[2] = SCE_ERROR_ERRNO_ENOENT;
        sched_yield_point();
        return;
    }

    // Convert PSP flags to POSIX
    int posix_flags = 0;
    if ((flags & PSP_O_RDWR) == PSP_O_RDWR) {
        posix_flags = O_RDWR;
    } else if (flags & PSP_O_WRONLY) {
        posix_flags = O_WRONLY;
    } else {
        posix_flags = O_RDONLY;
    }
    if (flags & PSP_O_CREAT) posix_flags |= O_CREAT;
    if (flags & PSP_O_TRUNC) posix_flags |= O_TRUNC;
    if (flags & PSP_O_APPEND) posix_flags |= O_APPEND;

    int host_fd = ::open(host_path.c_str(), posix_flags, 0644);

    // Always log LOADINGGROUP opens (bypass IO_LOG_LIMIT) for debugging
    if (std::string(psp_path).find("LOADINGGROUP") != std::string::npos ||
        std::string(psp_path).find("loadinggroup") != std::string::npos) {
        static int lg_count = 0;
        lg_count++;
        if (lg_count <= 20) {
            std::fprintf(stderr,
                "[LOADINGGROUP] #%d open('%s') host='%s' fd=%d errno=%d\n",
                lg_count, psp_path, host_path.c_str(), host_fd,
                host_fd < 0 ? errno : 0);
        }
    }

    if (host_fd < 0) {
        ctx->r[2] = SCE_ERROR_ERRNO_ENOENT;
        sched_yield_point();
        return;
    }

    // Directory-as-file hardening: macOS ::open() succeeds on directories,
    // but sceIoOpen on real firmware does not open directories (that's
    // sceIoDopen). Reject so the game sees ENOENT, not a 160-byte APFS dir.
    struct stat open_st;
    if (::fstat(host_fd, &open_st) == 0 && S_ISDIR(open_st.st_mode)) {
        ::close(host_fd);
        if (g_io_open_log_count < IO_LOG_LIMIT) {
            std::fprintf(stderr,
                "[HLE] sceIoOpen(\"%s\") is a directory -> 0x80010002\n",
                psp_path);
            g_io_open_log_count++;
        }
        ctx->r[2] = SCE_ERROR_ERRNO_ENOENT;
        sched_yield_point();
        return;
    }

    // Faithful fd-table cap: 64 slots minus 3 reserved on real firmware.
    if (g_fd_table.size() >= static_cast<size_t>(PSP_FD_MAX)) {
        ::close(host_fd);
        std::fprintf(stderr,
            "[HLE] sceIoOpen: fd table full -> SCE_KERNEL_ERROR_MFILE\n");
        ctx->r[2] = SCE_KERNEL_ERROR_MFILE;
        sched_yield_point();
        return;
    }

    int psp_fd = g_next_fd++;
    g_fd_table[psp_fd] = {
        host_fd, std::string(psp_path), false, nullptr,
        0, false, false, 0, 0
    };

    ctx->r[2] = psp_fd;
    sched_yield_point();
}

static void hle_sceIoClose(
    uint8_t* rdram, recomp_context* ctx
) {
    int fd = ctx->r[4];
    std::fprintf(stderr, "[HLE] sceIoClose(fd=%d) caller=0x%08X\n",
        fd, g_last_func_addr);
    auto it = g_fd_table.find(fd);
    if (it != g_fd_table.end()) {
        if (it->second.host_fd >= 0) {
            ::close(it->second.host_fd);
        }
        g_fd_table.erase(it);
        ctx->r[2] = SCE_OK;
    } else {
        ctx->r[2] = SCE_ERROR_ERRNO_ENOENT;
    }
    (void)rdram;
    sched_yield_point();
}

// lseek for an archive-rerouted fd, which represents the asset slice
// [slice_base, slice_base+slice_len) inside a container file (installed by
// the game module's PspIoPolicy). Two caller idioms are observed against
// the SAME fd kind and both must work:
//   * size-probe path: treats the fd as a STANDALONE FILE —
//     `SEEK_SET 0` = asset start, `SEEK_END` = asset size, then reads
//     from offset 0.
//   * directory path: issues `SEEK_SET <file_offset>`,
//     i.e. an ABSOLUTE container position equal to slice_base.
// Unified rule: SEEK_SET/CUR offsets below slice_base are slice-relative
// (rebased to slice_base); offsets >= slice_base are already absolute. Since
// slice_base is hundreds of MB and any intra-asset offset is < slice_len
// (a few MB), the two ranges never overlap, so the rule is unambiguous.
// SEEK_END returns the asset SIZE (slice_len) — the value the size-probe uses
// to allocate its read buffer — replacing the old container-size answer
// that a band-aid then capped to 512KB (truncating the asset).
static off_t slice_aware_lseek(PspFileDesc& d, int64_t offset, int posix_whence) {
    if (!d.is_slice) {
        return ::lseek(d.host_fd, static_cast<off_t>(offset), posix_whence);
    }
    const int64_t base = d.slice_base;
    const int64_t len = d.slice_len;
    int64_t rel;  // resulting position relative to the slice start
    switch (posix_whence) {
        case SEEK_END:
            rel = len + offset;
            break;
        case SEEK_CUR: {
            off_t cur = ::lseek(d.host_fd, 0, SEEK_CUR);
            rel = (static_cast<int64_t>(cur) - base) + offset;
            break;
        }
        case SEEK_SET:
        default:
            rel = (offset >= base) ? (offset - base) : offset;
            break;
    }
    if (rel < 0) rel = 0;
    if (rel > len) rel = len;
    ::lseek(d.host_fd, static_cast<off_t>(base + rel), SEEK_SET);
    return static_cast<off_t>(rel);
}

// Clamp a read on an archive-rerouted fd so it cannot run past the asset end
// into adjacent container bytes. Uses the host fd's current position relative
// to the slice. Non-slice fds pass through unchanged.
static uint32_t clamp_read_to_slice(PspFileDesc& d, uint32_t size) {
    if (!d.is_slice || d.host_fd < 0) return size;
    off_t cur = ::lseek(d.host_fd, 0, SEEK_CUR);
    int64_t rel = static_cast<int64_t>(cur) - d.slice_base;
    int64_t remaining = d.slice_len - rel;
    if (remaining < 0) remaining = 0;
    if (static_cast<int64_t>(size) > remaining) {
        return static_cast<uint32_t>(remaining);
    }
    return size;
}

static void hle_sceIoRead(
    uint8_t* rdram, recomp_context* ctx
) {
    int fd = ctx->r[4];
    uint32_t buf_ptr = static_cast<uint32_t>(ctx->r[5]);
    uint32_t size = static_cast<uint32_t>(ctx->r[6]);

    auto it = g_fd_table.find(fd);
    if (it == g_fd_table.end()) {
        ctx->r[2] = SCE_ERROR_ERRNO_ENOENT;
        sched_yield_point();
        return;
    }

    uint8_t* dest = rdram + (buf_ptr & PSP_ADDR_MASK);
    size = clamp_read_to_slice(it->second, size);
    ssize_t bytes_read = ::read(it->second.host_fd, dest, size);
    if (bytes_read < 0) {
        ctx->r[2] = SCE_ERROR_ERRNO_EIO;
        sched_yield_point();
        return;
    }

    ctx->r[2] = static_cast<int32_t>(bytes_read);
    sched_yield_point();
}

static void hle_sceIoWrite(
    uint8_t* rdram, recomp_context* ctx
) {
    int fd = ctx->r[4];
    uint32_t buf_ptr = static_cast<uint32_t>(ctx->r[5]);
    uint32_t size = static_cast<uint32_t>(ctx->r[6]);

    // stdout/stderr: redirect to host stderr
    if (fd == 1 || fd == 2) {
        const char* src = reinterpret_cast<const char*>(
            rdram + (buf_ptr & PSP_ADDR_MASK));
        std::fprintf(stderr, "[GAME] %.*s", (int)size, src);
        ctx->r[2] = static_cast<int32_t>(size);
        sched_yield_point();
        return;
    }

    auto it = g_fd_table.find(fd);
    if (it == g_fd_table.end()) {
        ctx->r[2] = SCE_ERROR_ERRNO_ENOENT;
        sched_yield_point();
        return;
    }

    const uint8_t* src = rdram + (buf_ptr & PSP_ADDR_MASK);
    ssize_t written = ::write(it->second.host_fd, src, size);
    ctx->r[2] = static_cast<int32_t>(written);
    sched_yield_point();
}

static void hle_sceIoLseek(
    uint8_t* rdram, recomp_context* ctx
) {
    int fd = ctx->r[4];
    std::fprintf(stderr,
        "[IO_DIAG] sceIoLseek(fd=%d, r4=%d, r5=%d, offset=0x%08X:%08X, whence=%d) "
        "caller=0x%08X sp=0x%08X\n",
        fd, (int)ctx->r[4], (int)ctx->r[5],
        (uint32_t)ctx->r[7], (uint32_t)ctx->r[6],
        (int32_t)ctx->r[8], g_last_func_addr,
        (uint32_t)ctx->r[29]);
    // PSP calling convention for sceIoLseek (64-bit offset):
    //   a0 (r4) = fd
    //   a1 (r5) = NOT used (no 64-bit alignment padding in PSP ABI for this call)
    //   a2:a3 (r6:r7) = offset (64-bit, lo:hi)
    //   t0 (r8) = whence  [NOT on stack — placed in delay slot by compiler]
    //
    // Confirmed from batch_0052.cpp disassembly (state=101, L_08863054):
    //   ctx->r[6] = 0; ctx->r[7] = 0; ctx->r[8] = 0; JAL sceIoLseek  -> SEEK_SET
    //   ctx->r[6] = 0; ctx->r[7] = 0; ctx->r[8] = 2; JAL sceIoLseek  -> SEEK_END
    // No sw instruction stores r8 to sp+16, so sp+16 has the prologue-saved s1
    // (slot pointer) which is a large PSP address — not a valid whence value.
    uint32_t offset_lo = static_cast<uint32_t>(ctx->r[6]);
    uint32_t offset_hi = static_cast<uint32_t>(ctx->r[7]);
    int32_t whence = static_cast<int32_t>(ctx->r[8]);

    auto it = g_fd_table.find(fd);
    if (it == g_fd_table.end()) {
        // Faithful: bad descriptor -> SCE_KERNEL_ERROR_BADF (PPSSPP
        // sceIo.cpp:355-362, 1452-1455), sign-extended into v0:v1
        // since sceIoLseek returns a 64-bit value.
        int64_t err = static_cast<int64_t>(SCE_KERNEL_ERROR_BADF);
        ctx->r[2] = static_cast<int32_t>(err & 0xFFFFFFFF);
        ctx->r[3] = static_cast<int32_t>(
            (static_cast<uint64_t>(err) >> 32) & 0xFFFFFFFF);
        sched_yield_point();
        return;
    }

    int64_t offset = static_cast<int64_t>(
        (static_cast<uint64_t>(offset_hi) << 32) | offset_lo);

    int posix_whence = SEEK_SET;
    if (whence == PSP_SEEK_CUR) posix_whence = SEEK_CUR;
    if (whence == PSP_SEEK_END) posix_whence = SEEK_END;

    // Slice-relative seek for archive-rerouted fds: the logical file is
    // [slice_base, slice_base+len). SEEK_END returns exactly the asset size —
    // the size the game uses to allocate its read buffer — with NO arbitrary
    // cap. This makes the rerouted fd behave like a real standalone file,
    // delivering the correct asset bytes instead of the container header.
    off_t pos = slice_aware_lseek(it->second, offset, posix_whence);

    std::fprintf(stderr,
        "[IO_DIAG] sceIoLseek(fd=%d, whence=%d) -> pos=%lld (0x%llX)\n",
        fd, whence, (long long)pos, (unsigned long long)pos);

    // Return 64-bit position in v0:v1
    ctx->r[2] = static_cast<int32_t>(pos & 0xFFFFFFFF);
    ctx->r[3] = static_cast<int32_t>(
        (static_cast<uint64_t>(pos) >> 32) & 0xFFFFFFFF);
    // NOTE: no sched_yield_point() here — sceIoLseek is called in a tight
    // state-machine loop (state=101) and yielding interrupts the loop before
    // state reaches 200 (LseekAsync). The state machine is single-threaded
    // within the FileThread so no cooperative switch is needed here.
}

static void hle_sceIoLseek32(
    uint8_t* rdram, recomp_context* ctx
) {
    int fd = ctx->r[4];
    int32_t offset = static_cast<int32_t>(ctx->r[5]);
    int32_t whence = static_cast<int32_t>(ctx->r[6]);

    auto it = g_fd_table.find(fd);
    if (it == g_fd_table.end()) {
        // Faithful: bad descriptor -> SCE_KERNEL_ERROR_BADF
        // (PPSSPP sceIo.cpp:355-362, 1452-1455).
        ctx->r[2] = SCE_KERNEL_ERROR_BADF;
        sched_yield_point();
        return;
    }

    int posix_whence = SEEK_SET;
    if (whence == PSP_SEEK_CUR) posix_whence = SEEK_CUR;
    if (whence == PSP_SEEK_END) posix_whence = SEEK_END;

    off_t pos = ::lseek(it->second.host_fd, offset, posix_whence);
    ctx->r[2] = static_cast<int32_t>(pos);
    (void)rdram;
    sched_yield_point();
}

static void hle_sceIoGetstat(
    uint8_t* rdram, recomp_context* ctx
) {
    uint32_t path_ptr = static_cast<uint32_t>(ctx->r[4]);
    uint32_t stat_ptr = static_cast<uint32_t>(ctx->r[5]);

    const char* psp_path = reinterpret_cast<const char*>(
        rdram + (path_ptr & PSP_ADDR_MASK));
    std::string host_path = psp_path_to_host(psp_path);

    if (host_path.empty()) {
        ctx->r[2] = SCE_ERROR_ERRNO_ENOENT;
        sched_yield_point();
        return;
    }

    struct stat st;
    if (::stat(host_path.c_str(), &st) != 0) {
        ctx->r[2] = SCE_ERROR_ERRNO_ENOENT;
        sched_yield_point();
        return;
    }

    if (stat_ptr != 0) {
        // Zero the stat struct first (SceIoStat = 64 bytes)
        for (uint32_t i = 0; i < 64; i++) {
            psp_mem_write<uint8_t>(rdram, stat_ptr + i, 0);
        }

        // Write SceIoStat: mode, attr, size
        if (S_ISDIR(st.st_mode)) {
            // Directory: mode=0x11FF, attr=0x0010
            psp_mem_write<int32_t>(
                rdram, stat_ptr, 0x11FF);
            psp_mem_write<uint32_t>(
                rdram, stat_ptr + 4, 0x0010);
        } else {
            // File: mode=0x21FF, attr=0x0020
            psp_mem_write<int32_t>(
                rdram, stat_ptr, 0x21FF);
            psp_mem_write<uint32_t>(
                rdram, stat_ptr + 4, 0x0020);
        }
        psp_mem_write<int64_t>(rdram, stat_ptr + 8,
                               st.st_size);
    }

    ctx->r[2] = SCE_OK;
    sched_yield_point();
}

static void hle_sceIoDopen(
    uint8_t* rdram, recomp_context* ctx
) {
    uint32_t path_ptr = static_cast<uint32_t>(ctx->r[4]);
    const char* psp_path = reinterpret_cast<const char*>(
        rdram + (path_ptr & PSP_ADDR_MASK));

    std::string host_path = psp_path_to_host(psp_path);
    if (host_path.empty()) {
        ctx->r[2] = SCE_ERROR_ERRNO_ENOENT;
        sched_yield_point();
        return;
    }

    DIR* dir = opendir(host_path.c_str());
    if (!dir) {
        ctx->r[2] = SCE_ERROR_ERRNO_ENOENT;
        sched_yield_point();
        return;
    }

    // Note: deliberately NOT counted against the PSP_FD_MAX cap — PPSSPP's
    // sceIoDopen allocates a kernel-object UID outside the fds[] table, so
    // directory listings are not subject to the 61-fd limit.
    int psp_fd = g_next_fd++;
    g_fd_table[psp_fd] = {
        -1, std::string(psp_path), true,
        static_cast<void*>(dir), 0, false, false, 0, 0
    };

    ctx->r[2] = psp_fd;
    sched_yield_point();
}

static void hle_sceIoDread(
    uint8_t* rdram, recomp_context* ctx
) {
    int fd = ctx->r[4];
    uint32_t dirent_ptr = static_cast<uint32_t>(ctx->r[5]);

    auto it = g_fd_table.find(fd);
    if (it == g_fd_table.end() || !it->second.is_dir) {
        ctx->r[2] = SCE_ERROR_ERRNO_ENOENT;
        sched_yield_point();
        return;
    }

    DIR* dir = static_cast<DIR*>(it->second.dir_handle);
    struct dirent* entry = readdir(dir);
    if (!entry) {
        ctx->r[2] = 0;  // End of directory
        sched_yield_point();
        return;
    }

    if (dirent_ptr != 0) {
        // Zero the struct first (SceIoDirent = 324 bytes)
        for (uint32_t i = 0; i < 324; i++) {
            psp_mem_write<uint8_t>(rdram, dirent_ptr + i, 0);
        }

        // Populate SceIoStat (first 64 bytes of SceIoDirent)
        // Layout: offset 0: int32_t st_mode
        //         offset 4: uint32_t st_attr
        //         offset 8: int64_t st_size

        // Determine if entry is a directory or file
        bool is_directory = false;
        std::string full_path = psp_path_to_host(
            it->second.psp_path.c_str());
        full_path += "/";
        full_path += entry->d_name;

#ifdef _DIRENT_HAVE_D_TYPE
        if (entry->d_type == DT_DIR) {
            is_directory = true;
        } else if (entry->d_type == DT_UNKNOWN) {
            // d_type not supported on this fs, use stat
            struct stat st;
            if (::stat(full_path.c_str(), &st) == 0) {
                is_directory = S_ISDIR(st.st_mode);
            }
        }
#else
        struct stat st_check;
        if (::stat(full_path.c_str(), &st_check) == 0) {
            is_directory = S_ISDIR(st_check.st_mode);
        }
#endif

        if (is_directory) {
            // Directory: mode=0x11FF, attr=0x0010
            psp_mem_write<int32_t>(
                rdram, dirent_ptr + 0, 0x11FF);
            psp_mem_write<uint32_t>(
                rdram, dirent_ptr + 4, 0x0010);
            psp_mem_write<int64_t>(
                rdram, dirent_ptr + 8, 0);
        } else {
            // File: mode=0x21FF, attr=0x0020
            psp_mem_write<int32_t>(
                rdram, dirent_ptr + 0, 0x21FF);
            psp_mem_write<uint32_t>(
                rdram, dirent_ptr + 4, 0x0020);
            // Get actual file size via stat()
            struct stat st;
            int64_t file_size = 0;
            if (::stat(full_path.c_str(), &st) == 0) {
                file_size = static_cast<int64_t>(st.st_size);
            }
            psp_mem_write<int64_t>(
                rdram, dirent_ptr + 8, file_size);
        }

        // Name at offset 68 (after SceIoStat)
        const char* name = entry->d_name;
        for (int i = 0; name[i] && i < 255; i++) {
            psp_mem_write<uint8_t>(
                rdram, dirent_ptr + 68 + i,
                static_cast<uint8_t>(name[i]));
        }
    }

    ctx->r[2] = 1;  // Entry read successfully
    sched_yield_point();
}

static void hle_sceIoDclose(
    uint8_t* rdram, recomp_context* ctx
) {
    int fd = ctx->r[4];
    auto it = g_fd_table.find(fd);
    if (it != g_fd_table.end()) {
        if (it->second.is_dir && it->second.dir_handle) {
            closedir(static_cast<DIR*>(
                it->second.dir_handle));
        }
        g_fd_table.erase(it);
    }
    ctx->r[2] = SCE_OK;
    (void)rdram;
    sched_yield_point();
}

// ---- Async IO: fd-based result model (matching PPSSPP) ----
// Async ops perform work synchronously but store results in
// PspFileDesc.asyncResult. The caller retrieves results via
// sceIoWaitAsync / sceIoPollAsync / sceIoGetAsyncStat.

static void hle_sceIoOpenAsync(
    uint8_t* rdram, recomp_context* ctx
) {
    uint32_t path_ptr = static_cast<uint32_t>(ctx->r[4]);
    int32_t flags = ctx->r[5];

    // Consume any pending pre-registration from sceIoSetAsyncCallback(fd=0)
    // up front: it was aimed at THIS open, so even an immediately-failing
    // open must not leave it dangling for the next unrelated OpenAsync.
    int prereg_cbId  = g_pending_prereg_callbackId;
    int prereg_cbArg = g_pending_prereg_callbackArg;
    g_pending_prereg_callbackId  = 0;
    g_pending_prereg_callbackArg = 0;

    // Faithful: PPSSPP (sceIo.cpp:2195-2200) returns the same immediate
    // error for a NULL path here — no fd is allocated.
    if (path_ptr == 0) {
        if (g_io_open_log_count < IO_LOG_LIMIT) {
            std::fprintf(stderr,
                "[HLE] sceIoOpenAsync NULL path -> 0x80010002 caller=0x%08X\n",
                g_last_func_addr);
            g_io_open_log_count++;
        }
        ctx->r[2] = SCE_ERROR_ERRNO_ENOENT;
        return;
    }

    const char* psp_path = reinterpret_cast<const char*>(
        rdram + (path_ptr & PSP_ADDR_MASK));

    // Faithful fd-table cap: 64 slots minus 3 reserved on real firmware.
    if (g_fd_table.size() >= static_cast<size_t>(PSP_FD_MAX)) {
        std::fprintf(stderr,
            "[HLE] sceIoOpenAsync: fd table full -> SCE_KERNEL_ERROR_MFILE\n");
        ctx->r[2] = SCE_KERNEL_ERROR_MFILE;
        return;
    }

    // Always allocate an fd first (PPSSPP behavior). The consumed
    // pre-registration (if any) is transferred to this fd so
    // notify_fd_callback can reach the IO completion callback.
    int psp_fd = g_next_fd++;

    std::string host_path = psp_path_to_host(psp_path);
    if (host_path.empty()) {
        if (try_route_missing_async_open_to_archive(
                psp_path, rdram, psp_fd,
                prereg_cbId, prereg_cbArg, ctx)) {
            return;
        }
        // File not found -- still return fd, store error
        g_fd_table[psp_fd] = {
            -1, std::string(psp_path), false, nullptr,
            static_cast<int64_t>(SCE_ERROR_ERRNO_ENOENT),
            true, false,
            prereg_cbId, prereg_cbArg
        };
        // Do NOT notify callback here — sceIoSetAsyncCallback fires it
        // when it sees asyncPending=true. Yielding here races with the
        // state machine (another thread clears asyncPending before
        // SetAsyncCallback is reached).
        ctx->r[2] = psp_fd;
        return;
    }

    // Reject extraction-artifact stubs (absent on the retail disc) -> ENOENT,
    // so the async asset state machine takes the packed-archive path.
    if (!(flags & PSP_O_CREAT) && is_extraction_artifact(host_path)) {
        if (try_route_missing_async_open_to_archive(
                psp_path, rdram, psp_fd,
                prereg_cbId, prereg_cbArg, ctx)) {
            return;
        }
        g_fd_table[psp_fd] = {
            -1, std::string(psp_path), false, nullptr,
            static_cast<int64_t>(SCE_ERROR_ERRNO_ENOENT),
            true, false,
            prereg_cbId, prereg_cbArg
        };
        ctx->r[2] = psp_fd;
        return;
    }

    // Convert PSP flags to POSIX
    int posix_flags = 0;
    if ((flags & PSP_O_RDWR) == PSP_O_RDWR) {
        posix_flags = O_RDWR;
    } else if (flags & PSP_O_WRONLY) {
        posix_flags = O_WRONLY;
    } else {
        posix_flags = O_RDONLY;
    }
    if (flags & PSP_O_CREAT) posix_flags |= O_CREAT;
    if (flags & PSP_O_TRUNC) posix_flags |= O_TRUNC;
    if (flags & PSP_O_APPEND) posix_flags |= O_APPEND;

    int host_fd = ::open(host_path.c_str(), posix_flags, 0644);
    // Faithful: a directory is not openable as a file (macOS ::open allows
    // it; PSP firmware does not) — treat as not-found, same as sync open.
    if (host_fd >= 0) {
        struct stat async_open_st;
        if (::fstat(host_fd, &async_open_st) == 0 &&
            S_ISDIR(async_open_st.st_mode)) {
            ::close(host_fd);
            host_fd = -1;
        }
    }
    if (host_fd < 0) {
        if (try_route_missing_async_open_to_archive(
                psp_path, rdram, psp_fd,
                prereg_cbId, prereg_cbArg, ctx)) {
            return;
        }
        g_fd_table[psp_fd] = {
            -1, std::string(psp_path), false, nullptr,
            static_cast<int64_t>(SCE_ERROR_ERRNO_ENOENT),
            true, false,
            prereg_cbId, prereg_cbArg
        };
        // Do NOT notify callback here — sceIoSetAsyncCallback fires it.
        ctx->r[2] = psp_fd;
        return;
    }

    // Success: asyncResult = fd (positive value)
    g_fd_table[psp_fd] = {
        host_fd, std::string(psp_path), false, nullptr,
        static_cast<int64_t>(psp_fd), true, false,
        prereg_cbId, prereg_cbArg
    };

    if (g_io_open_log_count < IO_LOG_LIMIT) {
        std::fprintf(stderr,
                     "[HLE] sceIoOpenAsync(\"%s\", 0x%X) -> fd %d\n",
                     psp_path, flags, psp_fd);
        g_io_open_log_count++;
        if (g_io_open_log_count == IO_LOG_LIMIT) {
            std::fprintf(stderr,
                "[HLE] (further sceIoOpen logs suppressed)\n");
        }
    }

    // Do NOT call notify_fd_callback or sched_yield_point here.
    // sceIoSetAsyncCallback fires the callback when it sees asyncPending=true.
    // Yielding here races: another thread may call psp_kernel_check_callbacks,
    // fire the callback prematurely and clear asyncPending before SetAsyncCallback.
    ctx->r[2] = psp_fd;
}

static void hle_sceIoReadAsync(
    uint8_t* rdram, recomp_context* ctx
) {
    int fd = ctx->r[4];
    uint32_t buf_ptr = static_cast<uint32_t>(ctx->r[5]);
    uint32_t size = static_cast<uint32_t>(ctx->r[6]);

    std::fprintf(stderr,
        "[HLE] sceIoReadAsync(fd=%d, buf=0x%08X, size=%u)\n",
        fd, buf_ptr, size);

    auto it = g_fd_table.find(fd);
    if (it == g_fd_table.end()) {
        ctx->r[2] = SCE_ERROR_ERRNO_ENOENT;
        sched_yield_point();
        return;
    }

    // Perform read synchronously, store result in asyncResult
    uint8_t* dest = rdram + (buf_ptr & PSP_ADDR_MASK);
    size = clamp_read_to_slice(it->second, size);
    ssize_t bytes_read = ::read(it->second.host_fd, dest, size);
    if (bytes_read < 0) {
        it->second.asyncResult = static_cast<int64_t>(
            SCE_ERROR_ERRNO_EIO);
    } else {
        it->second.asyncResult = static_cast<int64_t>(
            bytes_read);
    }
    it->second.asyncPending = true;
    // Do NOT call notify_fd_callback here. The state machine sets *(slot+12)=1
    // AFTER this function returns, then calls sceIoSetAsyncCallback which fires
    // the callback when it sees asyncPending=true. Firing here (via yield) races
    // with the state machine and clears asyncPending before SetAsyncCallback sees it.

    // Return SCE_OK (not the bytes-read count)
    ctx->r[2] = SCE_OK;
}

static void hle_sceIoCloseAsync(
    uint8_t* rdram, recomp_context* ctx
) {
    int fd = ctx->r[4];
    std::fprintf(stderr, "[HLE] sceIoCloseAsync(fd=%d)\n", fd);
    auto it = g_fd_table.find(fd);
    if (it != g_fd_table.end()) {
        if (it->second.host_fd >= 0) {
            ::close(it->second.host_fd);
            it->second.host_fd = -1;
        }
        it->second.asyncResult = 0;   // close success
        it->second.asyncPending = true;
        it->second.closePending = true;
        // Do NOT call notify_fd_callback here — same race as LseekAsync/ReadAsync.
        // sceIoSetAsyncCallback fires the callback when it sees asyncPending=true.
    }
    ctx->r[2] = SCE_OK;
    (void)rdram;
}

static void hle_sceIoLseekAsync(
    uint8_t* rdram, recomp_context* ctx
) {
    int fd = ctx->r[4];
    // Same PSP calling convention as sceIoLseek: whence is in r8 (t0), not sp+16.
    // See hle_sceIoLseek comment for full explanation.
    uint32_t offset_lo = static_cast<uint32_t>(ctx->r[6]);
    uint32_t offset_hi = static_cast<uint32_t>(ctx->r[7]);
    int32_t whence = static_cast<int32_t>(ctx->r[8]);

    std::fprintf(stderr,
        "[HLE] sceIoLseekAsync(fd=%d, offset=0x%08X%08X, whence=%d)\n",
        fd, offset_hi, offset_lo, whence);

    auto it = g_fd_table.find(fd);
    if (it == g_fd_table.end()) {
        // Faithful: bad descriptor -> SCE_KERNEL_ERROR_BADF (PPSSPP __IoGetFd)
        std::fprintf(stderr,
            "[HLE] sceIoLseekAsync: fd=%d NOT IN TABLE -> BADF\n", fd);
        ctx->r[2] = SCE_KERNEL_ERROR_BADF;
        sched_yield_point();
        return;
    }

    int64_t offset = static_cast<int64_t>(
        (static_cast<uint64_t>(offset_hi) << 32) | offset_lo);

    int posix_whence = SEEK_SET;
    if (whence == PSP_SEEK_CUR) posix_whence = SEEK_CUR;
    if (whence == PSP_SEEK_END) posix_whence = SEEK_END;

    off_t pos = slice_aware_lseek(it->second, offset, posix_whence);
    std::fprintf(stderr,
        "[HLE] sceIoLseekAsync: fd=%d host_fd=%d offset=%lld whence=%d -> pos=%lld asyncPending_before=%d\n",
        fd, it->second.host_fd, (long long)offset, posix_whence,
        (long long)pos, (int)it->second.asyncPending);
    it->second.asyncResult = static_cast<int64_t>(pos);
    it->second.asyncPending = true;
    // Do NOT call notify_fd_callback or sched_yield_point here.
    // The state machine sets *(slot+12)=1 AFTER this returns, then calls
    // sceIoSetAsyncCallback which fires the callback upon seeing asyncPending=true.
    // Yielding here races with the state machine: another thread may run
    // psp_kernel_check_callbacks, consume asyncPending, then SetAsyncCallback
    // sees asyncPending=false and never fires — leaving *(slot+12)=1 permanently.

    ctx->r[2] = SCE_OK;
}

static void hle_sceIoLseek32Async(
    uint8_t* rdram, recomp_context* ctx
) {
    int fd = ctx->r[4];
    int32_t offset = static_cast<int32_t>(ctx->r[5]);
    int32_t whence = static_cast<int32_t>(ctx->r[6]);

    std::fprintf(stderr,
        "[HLE] sceIoLseek32Async(fd=%d, offset=0x%08X, whence=%d)\n",
        fd, offset, whence);

    auto it = g_fd_table.find(fd);
    if (it == g_fd_table.end()) {
        // Faithful: bad descriptor -> SCE_KERNEL_ERROR_BADF (PPSSPP __IoGetFd)
        ctx->r[2] = SCE_KERNEL_ERROR_BADF;
        (void)rdram;
        sched_yield_point();
        return;
    }

    int posix_whence = SEEK_SET;
    if (whence == PSP_SEEK_CUR) posix_whence = SEEK_CUR;
    if (whence == PSP_SEEK_END) posix_whence = SEEK_END;

    off_t pos = ::lseek(it->second.host_fd, offset, posix_whence);
    it->second.asyncResult = static_cast<int64_t>(pos);
    it->second.asyncPending = true;
    notify_fd_callback(fd);

    ctx->r[2] = SCE_OK;
    (void)rdram;
    sched_yield_point();
}

static void hle_sceIoPollAsync(
    uint8_t* rdram, recomp_context* ctx
) {
    // fd in a0, result pointer in a1
    int fd = ctx->r[4];
    uint32_t res_ptr = static_cast<uint32_t>(ctx->r[5]);

    static int poll_count = 0;
    if (poll_count < 20) {
        std::fprintf(stderr,
            "[HLE] sceIoPollAsync(fd=%d, res_ptr=0x%08X)\n",
            fd, res_ptr);
        poll_count++;
    }

    auto it = g_fd_table.find(fd);
    if (it != g_fd_table.end() && res_ptr != 0) {
        std::fprintf(stderr,
            "[IO_DIAG] PollAsync fd=%d asyncResult=%lld asyncPending=%d\n",
            fd, (long long)it->second.asyncResult,
            (int)it->second.asyncPending);
        psp_mem_write<int64_t>(rdram, res_ptr,
                               it->second.asyncResult);
        it->second.asyncPending = false;

        // If close was pending, free the fd now
        if (it->second.closePending) {
            std::fprintf(stderr,
                "[HLE] sceIoPollAsync: erasing fd=%d (closePending)\n", fd);
            g_fd_table.erase(it);
        }
    }
    // Return 0 = complete (since we do synchronous IO)
    ctx->r[2] = SCE_OK;
    sched_yield_point();
}

static void hle_sceIoWaitAsync(
    uint8_t* rdram, recomp_context* ctx
) {
    // fd in a0, result pointer in a1
    int fd = ctx->r[4];
    uint32_t res_ptr = static_cast<uint32_t>(ctx->r[5]);

    int64_t result = 0;
    auto it = g_fd_table.find(fd);
    if (it != g_fd_table.end() && res_ptr != 0) {
        result = it->second.asyncResult;
        psp_mem_write<int64_t>(rdram, res_ptr, result);
        it->second.asyncPending = false;

        // If close was pending, free the fd now
        if (it->second.closePending) {
            g_fd_table.erase(it);
        }
    }
    std::fprintf(stderr,
        "[HLE] sceIoWaitAsync(fd=%d, res_ptr=0x%08X) -> "
        "result=0x%llX\n",
        fd, res_ptr, (unsigned long long)result);
    ctx->r[2] = SCE_OK;
    sched_yield_point();
}

static void hle_sceIoWaitAsyncCB(
    uint8_t* rdram, recomp_context* ctx
) {
    psp_kernel_check_callbacks(rdram, ctx);
    hle_sceIoWaitAsync(rdram, ctx);
}

static void hle_sceIoGetAsyncStat(
    uint8_t* rdram, recomp_context* ctx
) {
    // fd in a0, poll in a1, result pointer in a2
    int fd = ctx->r[4];
    uint32_t res_ptr = static_cast<uint32_t>(ctx->r[6]);

    auto it = g_fd_table.find(fd);
    if (it != g_fd_table.end() && res_ptr != 0) {
        psp_mem_write<int64_t>(rdram, res_ptr,
                               it->second.asyncResult);
        it->second.asyncPending = false;

        // If close was pending, free the fd now
        if (it->second.closePending) {
            g_fd_table.erase(it);
        }
    }
    // Return 0 = complete (since we do synchronous IO)
    ctx->r[2] = SCE_OK;
    sched_yield_point();
}

static void hle_sceIoSetAsyncCallback(
    uint8_t* rdram, recomp_context* ctx
) {
    int fd = ctx->r[4];
    int callbackId = ctx->r[5];
    int callbackArg = ctx->r[6];

    // Game-module diagnostic (#47 P5 seam): slot-field dumps keyed to the
    // game's async-slot struct layout live game-side.
    if (g_io_policy.on_set_async_callback != nullptr) {
        auto path_it = g_fd_table.find(fd);
        g_io_policy.on_set_async_callback(
            rdram, fd, callbackArg,
            path_it != g_fd_table.end()
                ? path_it->second.psp_path.c_str() : nullptr);
    }
    // This is a pre-registration: the game calls SetAsyncCallback(fd=0)
    // BEFORE calling OpenAsync, to register the callback for the upcoming
    // async operation.  On real PSP hardware, fd=0 is the actual fd that
    // will be used; in our model we allocate a new fd inside OpenAsync.
    //
    // Strategy: store as a pending pre-registration.  When the next
    // sceIoOpenAsync is called it will transfer cbId/cbArg to the new fd
    // so that notify_fd_callback fires with busy=1 already set.
    if (fd < PSP_FD_BASE && callbackId != 0) {
        // Inherit cbArg if the game passes 0 — it means "same slot as before".
        if (callbackArg == 0) {
            auto it2 = g_last_cbarg_for_cbid.find(callbackId);
            if (it2 != g_last_cbarg_for_cbid.end() &&
                it2->second != 0) {
                callbackArg = it2->second;
            }
        }
        std::fprintf(stderr,
            "[HLE] sceIoSetAsyncCallback: fd=%d invalid, "
            "storing as pre-registration (cbId=%d cbArg=0x%X "
            "caller=0x%08X)\n",
            fd, callbackId, callbackArg, g_last_func_addr);
        g_pending_prereg_callbackId  = callbackId;
        g_pending_prereg_callbackArg = callbackArg;
        ctx->r[2] = SCE_OK;
        (void)rdram;
        sched_yield_point();
        return;
    }

    auto it = g_fd_table.find(fd);
    if (it != g_fd_table.end()) {
        it->second.callbackId  = callbackId;
        it->second.callbackArg = callbackArg;
        if (g_io_policy.prepare_async_slot != nullptr) {
            g_io_policy.prepare_async_slot(
                rdram, it->second.psp_path.c_str(), callbackArg);
        }

        // Track last known cbArg for this cbId (for inheritance when cbArg=0)
        if (callbackId != 0 && callbackArg != 0) {
            g_last_cbarg_for_cbid[callbackId] = callbackArg;
        }

        // If async result is already available (synchronous IO
        // completed before callback was registered), fire now.
        // Clear asyncPending after notification so that
        // subsequent sceIoSetAsyncCallback calls (to prepare
        // for the next async operation) don't re-fire for
        // the same completed operation.
        std::fprintf(stderr,
            "[HLE] sceIoSetAsyncCallback(fd=%d) asyncPending=%d "
            "callbackId=%d callbackArg=0x%X -> notifying=%d\n",
            fd, it->second.asyncPending ? 1 : 0,
            callbackId, callbackArg,
            (it->second.asyncPending && callbackId != 0) ? 1 : 0);
        if (it->second.asyncPending && callbackId != 0) {
            it->second.asyncPending = false;
            psp_kernel_notify_callback(callbackId, callbackArg);
            // Immediately dispatch the callback in-place (PSP behavior).
            // On the real PSP, callbacks fire synchronously when the calling
            // thread is in callback-check mode (e.g. after sceKernelWaitSemaCB).
            // This allows the IO state machine (FUN_08862C14) to loop through
            // all states in a single FileThread invocation: each async op sets
            // *(slot+12)=1, calls sceIoSetAsyncCallback here, the callback
            // (FUN_088629CC → FUN_08862760) runs synchronously and clears
            // *(slot+12)=0, then L_08863944 loops back to process the next state.
            uint32_t pre_state = 0;
            if (callbackArg != 0) {
                pre_state = psp_mem_read<uint32_t>(
                    rdram, static_cast<uint32_t>(callbackArg));
            }
            psp_kernel_check_callbacks(rdram, ctx);
            if (callbackArg != 0) {
                uint32_t post_state = psp_mem_read<uint32_t>(
                    rdram, static_cast<uint32_t>(callbackArg));
                if (post_state != pre_state) {
                    std::fprintf(stderr,
                        "[IO_CB_STATE] fd=%d slot=0x%08X: "
                        "state %u -> %u (changed during callback)\n",
                        fd, callbackArg, pre_state, post_state);
                }
            }
        }
    }

    std::fprintf(stderr,
        "[HLE] sceIoSetAsyncCallback(fd=%d, cbId=%d, "
        "cbArg=0x%X) caller=0x%08X\n",
        fd, callbackId, callbackArg, g_last_func_addr);

    ctx->r[2] = SCE_OK;
    (void)rdram;
    sched_yield_point();
}

static void hle_sceIoDevctl(
    uint8_t* rdram, recomp_context* ctx
) {
    uint32_t dev_ptr = static_cast<uint32_t>(ctx->r[4]);
    uint32_t cmd = static_cast<uint32_t>(ctx->r[5]);
    uint32_t arg_ptr = static_cast<uint32_t>(ctx->r[6]);
    uint32_t arg_len = static_cast<uint32_t>(ctx->r[7]);

    const char* dev_name = reinterpret_cast<const char*>(
        rdram + (dev_ptr & PSP_ADDR_MASK));

    // fatms0: MScmRegisterMSInsertEjectCallback (0x02415821)
    // PPSSPP sceIo.cpp:1894-1926 -- registers callback and fires
    // immediately with MemoryStick_FatState() = 1 (ASSIGNED)
    if (std::strcmp(dev_name, "fatms0:") == 0 &&
        cmd == 0x02415821) {
        if (arg_ptr != 0 && arg_len >= 4) {
            uint32_t cbId = psp_mem_read<uint32_t>(
                rdram, arg_ptr);
            std::fprintf(stderr,
                "[HLE] sceIoDevctl(fatms0:, 0x%08X): "
                "MS FAT callback %u registered, notifying\n",
                cmd, cbId);
            // Memstick always inserted; FAT state = ASSIGNED (1)
            // Mark pending AND dispatch immediately -- the game
            // checks the flag set by this callback before any *CB
            // wait function would naturally dispatch it.
            psp_kernel_notify_callback(
                static_cast<int>(cbId), 1);
            psp_kernel_check_callbacks(rdram, ctx);
        }
        ctx->r[2] = SCE_OK;
        sched_yield_point();
        return;
    }

    // Default: stub (log once for unknown commands)
    static std::set<uint32_t> logged_cmds;
    if (logged_cmds.insert(cmd).second) {
        std::fprintf(stderr,
            "[HLE] sceIoDevctl(%s, 0x%08X): stub\n",
            dev_name, cmd);
    }
    ctx->r[2] = SCE_OK;
    sched_yield_point();
}

static void hle_sceIoIoctlAsync(
    uint8_t* rdram, recomp_context* ctx
) {
    // sceIoIoctlAsync submits an async device-control op. Like every other
    // async submit (Open/Read/Lseek/Close), it must register a pending async
    // completion on the fd: the game's IO state machine (FUN_08862C14) arms
    // sceIoSetAsyncCallback immediately after this returns, and that HLE only
    // fires the registered IoAsyncCallback (FUN_088629CC -> FUN_08862760, which
    // clears the slot's retry flag) when it sees asyncPending=true. Without
    // marking the fd pending here, the completion never fires and the slot's
    // memory-resident load (titledata, state 510->511) stalls forever.
    // The device-control commands the game issues on this path are status
    // queries (e.g. cmd 0x01020006); we have no real device, so report
    // success (asyncResult=0) the same way sceIoCloseAsync does.
    int fd = ctx->r[4];
    auto it = g_fd_table.find(fd);
    if (it != g_fd_table.end()) {
        it->second.asyncResult = 0;       // device-control success
        it->second.asyncPending = true;   // arm the completion callback
    }
    std::fprintf(stderr,
        "[HLE] sceIoIoctlAsync(fd=%d, cmd=0x%08X) -> asyncPending=1\n",
        fd, static_cast<uint32_t>(ctx->r[5]));
    ctx->r[2] = SCE_OK;
    (void)rdram;
    // Do NOT notify here — sceIoSetAsyncCallback fires the callback when it
    // sees asyncPending=true (same race-avoidance as the sibling async ops).
}

static void hle_sceIoRename(
    uint8_t* rdram, recomp_context* ctx
) {
    ctx->r[2] = SCE_OK;
    (void)rdram;
    sched_yield_point();
}

static void hle_sceIoChdir(
    uint8_t* rdram, recomp_context* ctx
) {
    uint32_t path_ptr = static_cast<uint32_t>(ctx->r[4]);
    const char* psp_path = reinterpret_cast<const char*>(
        rdram + (path_ptr & PSP_ADDR_MASK));

    std::string p(psp_path);
    for (auto& c : p) {
        if (c == '\\') c = '/';
    }
    while (p.size() > 1 && p.back() == '/') {
        p.pop_back();
    }

    g_cwd = p;
    std::fprintf(stderr, "[HLE] sceIoChdir(\"%s\")\n", p.c_str());
    ctx->r[2] = SCE_OK;
    sched_yield_point();
}

// ---- Registration ----

void psp_hle_register_io() {
    psp_hle_register("sceIoOpen", hle_sceIoOpen);
    psp_hle_register("sceIoClose", hle_sceIoClose);
    psp_hle_register("sceIoRead", hle_sceIoRead);
    psp_hle_register("sceIoWrite", hle_sceIoWrite);
    psp_hle_register("sceIoLseek", hle_sceIoLseek);
    psp_hle_register("sceIoLseek32", hle_sceIoLseek32);
    psp_hle_register("sceIoLseekAsync", hle_sceIoLseekAsync);
    psp_hle_register("sceIoLseek32Async", hle_sceIoLseek32Async);
    psp_hle_register("sceIoGetstat", hle_sceIoGetstat);
    psp_hle_register("sceIoDopen", hle_sceIoDopen);
    psp_hle_register("sceIoDread", hle_sceIoDread);
    psp_hle_register("sceIoDclose", hle_sceIoDclose);
    psp_hle_register("sceIoOpenAsync", hle_sceIoOpenAsync);
    psp_hle_register("sceIoReadAsync", hle_sceIoReadAsync);
    psp_hle_register("sceIoCloseAsync", hle_sceIoCloseAsync);
    psp_hle_register("sceIoPollAsync", hle_sceIoPollAsync);
    psp_hle_register("sceIoWaitAsync", hle_sceIoWaitAsync);
    psp_hle_register("sceIoWaitAsyncCB", hle_sceIoWaitAsyncCB);
    psp_hle_register("sceIoGetAsyncStat", hle_sceIoGetAsyncStat);
    psp_hle_register("sceIoSetAsyncCallback",
                      hle_sceIoSetAsyncCallback);
    psp_hle_register("sceIoDevctl", hle_sceIoDevctl);
    psp_hle_register("sceIoIoctlAsync", hle_sceIoIoctlAsync);
    psp_hle_register("sceIoRename", hle_sceIoRename);
    psp_hle_register("sceIoChdir", hle_sceIoChdir);
}
