#pragma once

#include <cstdint>
#include <cstddef>
#include <string>
#include <vector>

/// Debug socket v2 (issue #35) — TCP server on loopback, line-oriented
/// text protocol, multiple concurrent clients (thread per client).
///
/// Every command replies with an ACK header:
///     "OK <len>\n"  followed by exactly <len> payload bytes, or
///     "ERR <reason>\n"  with no payload.
/// Malformed or unknown input always gets an ERR reply, never silence.
///
/// Commands (one per line, human-typeable via nc):
///     I                       -- info: one JSON line as the OK payload
///     R   <hexaddr> <decsize> -- read masked PSP memory (framed)
///     RAW <hexaddr> <decsize> -- legacy v1 read: raw bytes, NO framing
///     W   <hexaddr> <hexbytes>-- write bytes into rdram (masked, bounds-checked)
///     B   <hexmask> <decms>   -- inject button mask for <ms> milliseconds
///     S   <path>              -- screenshot: render thread writes a TGA to
///                                <path>; OK once written, ERR on timeout
///
/// Backward compat: v1 clients used "R" and parsed the unframed byte
/// stream. v2 frames "R"; the old behavior is kept verbatim under the
/// "RAW" alias (success replies only -- malformed RAW still gets ERR).

/// One PSP thread entry for the I command's thread list.
/// Filled by psp_scheduler_snapshot() (see psp_scheduler.h).
struct PspDebugThreadInfo {
    int  id;
    char name[32];
    char status[16];       ///< "DORMANT"/"READY"/"RUNNING"/"WAIT"/"DEAD"/"WAIT_SLEEP"
    char wait_reason[24];  ///< e.g. "sema:259", "sleep", "" when not waiting
};

/// Dependency seams for the I and S commands. All members are optional:
/// a null member degrades gracefully (zeros / empty arrays / "ERR
/// unsupported") so the command layer is unit-testable without live game
/// state. main.cpp wires the real providers at boot.
struct PspDebugHooks {
    /// GE counters (psp_ge_draw.cpp). Any out-pointer may receive 0.
    void (*ge_stats)(uint32_t* frames, int* prims_total,
                     int* real_nonsprite, int* sprite_nonclear,
                     int* clears) = nullptr;
    /// LOOKUP_MISS counters (psp_dispatch.cpp).
    void (*lookup_miss_stats)(uint32_t* unique_addrs,
                              uint64_t* total_calls) = nullptr;
    /// Recent dispatched-function ring, oldest first. Returns count written.
    int (*recent_funcs)(uint32_t* out, int max) = nullptr;
    /// PSP thread snapshot. Returns count written.
    int (*thread_list)(PspDebugThreadInfo* out, int max) = nullptr;
    /// Blocking screenshot request (render thread services it).
    /// Returns true once the file is written, false on timeout/failure.
    bool (*capture_screenshot)(const char* path, int timeout_ms) = nullptr;
};

/// Install the providers used by the I and S commands.
/// Safe to call before or after psp_debug_socket_start.
void psp_debug_socket_set_hooks(const PspDebugHooks& hooks);

/// Reply produced for one command line.
/// For all v2 commands `header` is "OK <len>\n" or "ERR <reason>\n" and
/// `payload` holds the <len> bytes. For a successful legacy RAW command
/// `header` is empty and `payload` is the unframed byte stream.
struct PspDebugReply {
    std::string header;
    std::vector<uint8_t> payload;
};

/// Parse and execute one command line (no socket I/O) — the testable
/// command layer. `line` is NUL-terminated without the trailing newline.
PspDebugReply psp_debug_handle_line(const char* line,
                                    uint8_t* rdram,
                                    size_t rdram_size);

/// Start the TCP debug socket server on the given port (loopback only).
/// Runs a background accept thread; each client gets its own thread.
///
/// @param rdram      Pointer to the 128MB PSP memory buffer.
/// @param rdram_size Size of rdram in bytes (PSP_MEM_SIZE = 0x08000000).
/// @param port       TCP port to listen on (default 9999).
void psp_debug_socket_start(uint8_t* rdram, size_t rdram_size, int port);

/// Stop the debug socket server and join all socket threads.
/// Safe to call even if psp_debug_socket_start was never called.
void psp_debug_socket_stop();
