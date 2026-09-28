#pragma once
#include <cstdint>
#include <string>

// PSP I/O open flags
constexpr int PSP_O_RDONLY   = 0x0001;
constexpr int PSP_O_WRONLY   = 0x0002;
constexpr int PSP_O_RDWR     = 0x0003;
constexpr int PSP_O_APPEND   = 0x0100;
constexpr int PSP_O_CREAT    = 0x0200;
constexpr int PSP_O_TRUNC    = 0x0400;

// PSP I/O seek modes
constexpr int PSP_SEEK_SET = 0;
constexpr int PSP_SEEK_CUR = 1;
constexpr int PSP_SEEK_END = 2;

// PSP fd allocation
constexpr int PSP_FD_BASE = 3;
// Real firmware has PSP_COUNT_FDS = 64 fd slots, 3 of them reserved for
// stdin/stdout/stderr (PPSSPP sceIo.cpp:127-132, 364-374), leaving 61 usable.
// Exhaustion returns SCE_KERNEL_ERROR_MFILE (0x80020320).
constexpr int PSP_FD_MAX  = 61;

struct PspFileDesc {
    int host_fd;
    std::string psp_path;
    bool is_dir;
    void* dir_handle;      // DIR* for directory operations
    int64_t asyncResult = 0;   // Result stored by async ops, read by WaitAsync
    bool asyncPending = false;  // Whether an async op is in flight
    bool closePending = false;  // Whether sceIoCloseAsync was called
    int callbackId = 0;        // Callback UID from sceIoSetAsyncCallback
    int callbackArg = 0;       // Callback argument from sceIoSetAsyncCallback
    // BND slice view: when this fd is a packed-archive asset rerouted into
    // DATA_CMN.BND, the logical "file" is the byte range
    // [slice_base, slice_base+slice_len). All lseek/read operate relative to
    // slice_base, and SEEK_END returns slice_len — so the rerouted fd behaves
    // exactly like a standalone file of the asset's true size. is_slice gates
    // this so plain files are unaffected.
    bool is_slice = false;
    int64_t slice_base = 0;    // host-file offset of the asset's first byte
    int64_t slice_len = 0;     // asset size in bytes
};

// Initialize I/O subsystem
void psp_io_init(const char* disc0_host_path);

// Map PSP path to host path
std::string psp_path_to_host(const char* psp_path);

// ---- Game-module IO policy (issue #47 Phase 5 seam) ----
// Everything title-specific the IO layer used to hardcode (Patapon's
// DATA_CMN.BND reroute, BND IO-slot staging at game struct offsets,
// extraction-artifact stub rejection, address-keyed diagnostics) installs
// through this struct from the game module's register_hooks. Every pointer
// is optional (nullptr = generic default: no reroute, no staging, no
// artifact filtering, no extra diagnostics).
struct PspIoPolicy {
    /// Resolve a path whose host file is missing to a slice of a container
    /// archive: fill the container's HOST path plus the slice's byte size
    /// and absolute file offset. Return false when the path has no archive
    /// backing (the open then fails with ENOENT as usual).
    bool (*resolve_archive_backing)(const char* psp_path,
                                    std::string* host_container,
                                    uint32_t* size, uint32_t* off);
    /// Stage the game's async IO slot for an archive-backed read (game
    /// struct offsets live game-side). Called from the archive reroute and
    /// from sceIoSetAsyncCallback for fds in the fd table.
    void (*prepare_async_slot)(uint8_t* rdram, const char* psp_path,
                               int callback_arg);
    /// True when an existing host file is an ISO-extraction artifact that
    /// does not exist on the retail disc (open must fail with ENOENT).
    bool (*is_extraction_artifact)(const char* host_path);
    /// Diagnostic: sceIoSetAsyncCallback observer. psp_path is nullptr
    /// when the fd is not in the fd table.
    void (*on_set_async_callback)(uint8_t* rdram, int fd, int callback_arg,
                                  const char* psp_path);
    /// Diagnostic: NULL-path sceIoOpen observer (bad-descriptor races).
    void (*on_null_path_open)(uint8_t* rdram);
};
void psp_io_set_policy(const PspIoPolicy& policy);

// Registration
void psp_hle_register_io();
