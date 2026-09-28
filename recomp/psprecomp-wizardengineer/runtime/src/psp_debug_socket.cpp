#include "psp_debug_socket.h"
#include "psp_runtime.h"

#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <thread>
#include <vector>

#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

static constexpr uint32_t PSP_DBG_ADDR_MASK = 0x07FFFFFFU;
static constexpr size_t   MAX_READ_BYTES    = 65536;
static constexpr size_t   MAX_LINE          = 4096;
static constexpr int      SCREENSHOT_WAIT_MS = 10000;
static constexpr int      MAX_RING_ENTRIES  = 64;
static constexpr int      MAX_THREAD_ENTRIES = 64;

static std::atomic<bool>  g_debug_running{false};
static std::thread        g_debug_thread;
static int                g_server_fd = -1;

// Per-client threads + fds, so psp_debug_socket_stop can unblock and join
// every client handler (recv() returns once its fd is shut down).
static std::mutex               g_clients_mutex;
static std::vector<int>         g_client_fds;
static std::vector<std::thread> g_client_threads;

// Static rdram pointer and size set by psp_debug_socket_start.
static uint8_t* g_rdram      = nullptr;
static size_t   g_rdram_size = 0;

// Providers for the I and S commands (see psp_debug_socket.h).
static PspDebugHooks g_hooks;

// Process start reference for uptime. Set by psp_debug_socket_start; falls
// back to first-use so the unit-testable handler works standalone.
static std::chrono::steady_clock::time_point g_start_time{};
static bool g_start_time_set = false;

// Injected button overlay (declared extern in psp_runtime.h).
// Set by the B command below; consumed by the sceCtrl HLE.
std::atomic<uint32_t> g_injected_buttons{0};
std::atomic<int64_t>  g_injected_buttons_deadline_ms{0};

void psp_debug_socket_set_hooks(const PspDebugHooks& hooks) {
    g_hooks = hooks;
}

static double debug_uptime_sec() {
    if (!g_start_time_set) {
        g_start_time = std::chrono::steady_clock::now();
        g_start_time_set = true;
    }
    return std::chrono::duration<double>(
        std::chrono::steady_clock::now() - g_start_time).count();
}

// ---------------------------------------------------------------------------
// Reply helpers
// ---------------------------------------------------------------------------

static PspDebugReply make_err(const char* reason) {
    PspDebugReply r;
    r.header = std::string("ERR ") + reason + "\n";
    return r;
}

static PspDebugReply make_ok(std::vector<uint8_t> payload) {
    PspDebugReply r;
    char hdr[32];
    std::snprintf(hdr, sizeof(hdr), "OK %zu\n", payload.size());
    r.header = hdr;
    r.payload = std::move(payload);
    return r;
}

static PspDebugReply make_ok_empty() {
    return make_ok({});
}

// ---------------------------------------------------------------------------
// Parsing helpers
// ---------------------------------------------------------------------------

/// strtoul wrapper that fails on empty/garbage input (strtoul itself
/// returns 0 for both, which silently masked malformed commands in v1).
static bool parse_ulong(const char* s, int base, unsigned long* out,
                        const char** end_out) {
    if (!s || *s == '\0') return false;
    char* end = nullptr;
    unsigned long v = std::strtoul(s, &end, base);
    if (end == s) return false;
    *out = v;
    if (end_out) *end_out = end;
    return true;
}

static const char* skip_spaces(const char* s) {
    while (*s == ' ') s++;
    return s;
}

static int hex_nibble(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

// ---------------------------------------------------------------------------
// Memory read (shared by R and RAW): mask, clamp, zero-fill out-of-range
// ---------------------------------------------------------------------------

static std::vector<uint8_t> read_psp_memory(uint8_t* rdram, size_t rdram_size,
                                            uint32_t addr, size_t size) {
    std::vector<uint8_t> buf(size, 0);
    uint32_t masked = addr & PSP_DBG_ADDR_MASK;
    if (static_cast<size_t>(masked) < rdram_size) {
        size_t valid = rdram_size - static_cast<size_t>(masked);
        if (valid > size) valid = size;
        std::memcpy(buf.data(), rdram + masked, valid);
    }
    return buf;
}

// ---------------------------------------------------------------------------
// I command: one JSON line, designed to be jq-friendly
// ---------------------------------------------------------------------------

/// Escape a (possibly garbage) C string for JSON. Thread names come from
/// game memory; never trust them to be printable.
static void json_append_escaped(std::string& out, const char* s) {
    for (; *s; s++) {
        unsigned char c = static_cast<unsigned char>(*s);
        if (c == '"' || c == '\\') {
            out += '\\';
            out += static_cast<char>(c);
        } else if (c < 0x20 || c >= 0x7F) {
            char buf[8];
            std::snprintf(buf, sizeof(buf), "\\u%04x", c);
            out += buf;
        } else {
            out += static_cast<char>(c);
        }
    }
}

static std::string build_info_json() {
    char buf[128];
    std::string j = "{";

    std::snprintf(buf, sizeof(buf), "\"uptime_sec\":%.1f,", debug_uptime_sec());
    j += buf;

    uint32_t frames = 0;
    int prims = 0, real_ns = 0, sprite = 0, clears = 0;
    if (g_hooks.ge_stats) {
        g_hooks.ge_stats(&frames, &prims, &real_ns, &sprite, &clears);
    }
    std::snprintf(buf, sizeof(buf),
        "\"ge\":{\"frames\":%u,\"prims\":%d,\"real_nonsprite\":%d,"
        "\"sprite_nonclear\":%d,\"clears\":%d},",
        frames, prims, real_ns, sprite, clears);
    j += buf;

    uint32_t miss_unique = 0;
    uint64_t miss_total = 0;
    if (g_hooks.lookup_miss_stats) {
        g_hooks.lookup_miss_stats(&miss_unique, &miss_total);
    }
    std::snprintf(buf, sizeof(buf),
        "\"lookup_miss\":{\"unique\":%u,\"total\":%llu},",
        miss_unique, static_cast<unsigned long long>(miss_total));
    j += buf;

    j += "\"recent_funcs\":[";
    if (g_hooks.recent_funcs) {
        uint32_t ring[MAX_RING_ENTRIES];
        int n = g_hooks.recent_funcs(ring, MAX_RING_ENTRIES);
        for (int i = 0; i < n; i++) {
            std::snprintf(buf, sizeof(buf), "%s\"0x%08X\"",
                          i ? "," : "", ring[i]);
            j += buf;
        }
    }
    j += "],";

    j += "\"threads\":[";
    if (g_hooks.thread_list) {
        PspDebugThreadInfo threads[MAX_THREAD_ENTRIES];
        int n = g_hooks.thread_list(threads, MAX_THREAD_ENTRIES);
        for (int i = 0; i < n; i++) {
            std::snprintf(buf, sizeof(buf), "%s{\"id\":%d,\"name\":\"",
                          i ? "," : "", threads[i].id);
            j += buf;
            json_append_escaped(j, threads[i].name);
            j += "\",\"status\":\"";
            json_append_escaped(j, threads[i].status);
            j += "\",\"wait\":\"";
            json_append_escaped(j, threads[i].wait_reason);
            j += "\"}";
        }
    }
    j += "]}\n";
    return j;
}

// ---------------------------------------------------------------------------
// Command dispatch (testable, no socket I/O)
// ---------------------------------------------------------------------------

/// R / RAW argument parsing + read. `framed` selects v2 framing vs the
/// legacy unframed v1 byte stream.
static PspDebugReply handle_read(const char* args, uint8_t* rdram,
                                 size_t rdram_size, bool framed) {
    unsigned long addr = 0, size = 0;
    const char* end = nullptr;
    if (!parse_ulong(skip_spaces(args), 16, &addr, &end)) {
        return make_err("bad-addr");
    }
    if (*end != ' ' || !parse_ulong(skip_spaces(end), 10, &size, nullptr)) {
        return make_err("bad-size");
    }
    if (size == 0 || size > MAX_READ_BYTES) {
        return make_err("size-out-of-range");
    }
    std::vector<uint8_t> data = read_psp_memory(
        rdram, rdram_size, static_cast<uint32_t>(addr), size);
    if (framed) {
        return make_ok(std::move(data));
    }
    PspDebugReply r;
    r.payload = std::move(data);  // empty header == legacy raw stream
    return r;
}

static PspDebugReply handle_write(const char* args, uint8_t* rdram,
                                  size_t rdram_size) {
    unsigned long addr = 0;
    const char* end = nullptr;
    if (!parse_ulong(skip_spaces(args), 16, &addr, &end)) {
        return make_err("bad-addr");
    }
    if (*end != ' ') return make_err("bad-bytes");
    const char* hex = skip_spaces(end);
    size_t hex_len = std::strlen(hex);
    if (hex_len == 0 || (hex_len % 2) != 0) {
        return make_err("bad-bytes");
    }
    size_t nbytes = hex_len / 2;
    std::vector<uint8_t> bytes(nbytes);
    for (size_t i = 0; i < nbytes; i++) {
        int hi = hex_nibble(hex[i * 2]);
        int lo = hex_nibble(hex[i * 2 + 1]);
        if (hi < 0 || lo < 0) return make_err("bad-bytes");
        bytes[i] = static_cast<uint8_t>((hi << 4) | lo);
    }
    uint32_t masked = static_cast<uint32_t>(addr) & PSP_DBG_ADDR_MASK;
    // Unlike R (which zero-fills), a partial write would silently corrupt
    // intent -- refuse anything not fully in range.
    if (static_cast<size_t>(masked) + nbytes > rdram_size) {
        return make_err("out-of-range");
    }
    std::memcpy(rdram + masked, bytes.data(), nbytes);
    return make_ok_empty();
}

static PspDebugReply handle_buttons(const char* args) {
    unsigned long mask = 0, hold_ms = 0;
    const char* end = nullptr;
    if (!parse_ulong(skip_spaces(args), 16, &mask, &end)) {
        return make_err("bad-mask");
    }
    if (*end != ' ' || !parse_ulong(skip_spaces(end), 10, &hold_ms, nullptr)) {
        return make_err("bad-duration");
    }
    // Clamp: bounds how long an injected press can stick
    // and keeps the deadline math overflow-free.
    if (hold_ms > 60000) {
        hold_ms = 60000;
    }
    if (hold_ms > 0) {
        auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();
        g_injected_buttons.store(
            static_cast<uint32_t>(mask), std::memory_order_relaxed);
        g_injected_buttons_deadline_ms.store(
            now_ms + static_cast<int64_t>(hold_ms),
            std::memory_order_relaxed);
    }
    return make_ok_empty();
}

static PspDebugReply handle_screenshot(const char* args) {
    const char* path = skip_spaces(args);
    if (*path == '\0') return make_err("bad-path");
    if (!g_hooks.capture_screenshot) return make_err("unsupported");
    // Blocks this client's thread only (one thread per client), so other
    // clients stay responsive while the render thread services the capture.
    if (!g_hooks.capture_screenshot(path, SCREENSHOT_WAIT_MS)) {
        return make_err("timeout");
    }
    return make_ok_empty();
}

PspDebugReply psp_debug_handle_line(const char* line,
                                    uint8_t* rdram,
                                    size_t rdram_size) {
    if (!line || line[0] == '\0') return make_err("empty");

    if (line[0] == 'I' && (line[1] == '\0' || line[1] == ' ')) {
        std::string json = build_info_json();
        return make_ok(std::vector<uint8_t>(json.begin(), json.end()));
    }
    if (std::strncmp(line, "RAW ", 4) == 0) {
        return handle_read(line + 4, rdram, rdram_size, /*framed=*/false);
    }
    if (line[0] == 'R' && line[1] == ' ') {
        return handle_read(line + 2, rdram, rdram_size, /*framed=*/true);
    }
    if (line[0] == 'W' && line[1] == ' ') {
        return handle_write(line + 2, rdram, rdram_size);
    }
    if (line[0] == 'B' && line[1] == ' ') {
        return handle_buttons(line + 2);
    }
    if (line[0] == 'S' && line[1] == ' ') {
        return handle_screenshot(line + 2);
    }
    return make_err("unknown-command");
}

// ---------------------------------------------------------------------------
// Socket plumbing
// ---------------------------------------------------------------------------

static bool send_all(int fd, const void* data, size_t len) {
    const uint8_t* p = static_cast<const uint8_t*>(data);
    size_t sent = 0;
    while (sent < len) {
        ssize_t w = ::send(fd, p + sent, len - sent, 0);
        if (w <= 0) return false;
        sent += static_cast<size_t>(w);
    }
    return true;
}

/// Handle one connected client: read lines, dispatch commands, send the
/// framed reply. Returns when the client disconnects or the server stops.
static void handle_client(int client_fd) {
    std::vector<char> line(MAX_LINE);
    size_t pos = 0;
    bool overflow = false;
    bool last_was_cr = false;

    while (g_debug_running.load(std::memory_order_relaxed)) {
        // Read one byte at a time until '\n' (commands are tiny; the
        // bottleneck is the human/agent on the other end, not syscalls)
        char ch;
        ssize_t n = ::recv(client_fd, &ch, 1, 0);
        if (n <= 0) {
            break;
        }
        if (ch != '\n' && ch != '\r') {
            if (pos < MAX_LINE - 1) {
                line[pos++] = ch;
            } else {
                overflow = true;
            }
            last_was_cr = false;
            continue;
        }
        // Swallow only the LF of a CRLF pair; a genuine empty line falls
        // through to the handler and gets "ERR empty" (never silence).
        const bool crlf_lf = (ch == '\n' && last_was_cr);
        last_was_cr = (ch == '\r');
        if (pos == 0 && !overflow && crlf_lf) continue;
        line[pos] = '\0';
        pos = 0;

        PspDebugReply reply;
        if (overflow) {
            overflow = false;
            reply.header = "ERR line-too-long\n";
        } else {
            reply = psp_debug_handle_line(line.data(), g_rdram, g_rdram_size);
        }

        if (!reply.header.empty()
            && !send_all(client_fd, reply.header.data(),
                         reply.header.size())) {
            break;
        }
        if (!reply.payload.empty()
            && !send_all(client_fd, reply.payload.data(),
                         reply.payload.size())) {
            break;
        }
    }
    {
        std::lock_guard<std::mutex> lock(g_clients_mutex);
        for (auto it = g_client_fds.begin(); it != g_client_fds.end(); ++it) {
            if (*it == client_fd) { g_client_fds.erase(it); break; }
        }
    }
    ::close(client_fd);
}

/// Main loop for the background accept thread.
static void server_loop(int port) {
    int fd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        std::fprintf(stderr,
            "[DEBUG] Failed to create debug socket\n");
        return;
    }
    g_server_fd = fd;

    int opt = 1;
    ::setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr{};
    addr.sin_family      = AF_INET;
    addr.sin_port        = htons(static_cast<uint16_t>(port));
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    if (::bind(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        std::fprintf(stderr,
            "[DEBUG] Failed to bind debug socket on port %d\n", port);
        ::close(fd);
        g_server_fd = -1;
        return;
    }

    if (::listen(fd, 4) < 0) {
        std::fprintf(stderr, "[DEBUG] Failed to listen on debug socket\n");
        ::close(fd);
        g_server_fd = -1;
        return;
    }

    std::fprintf(stderr,
        "[DEBUG] Debug socket listening on port %d\n", port);

    while (g_debug_running.load(std::memory_order_relaxed)) {
        // Use select() with 1-second timeout so we can check g_debug_running
        fd_set rfds;
        FD_ZERO(&rfds);
        FD_SET(fd, &rfds);
        struct timeval tv{1, 0};

        int ready = ::select(fd + 1, &rfds, nullptr, nullptr, &tv);
        if (ready <= 0) continue;

        struct sockaddr_in client_addr{};
        socklen_t client_len = sizeof(client_addr);
        int client_fd = ::accept(
            fd, reinterpret_cast<sockaddr*>(&client_addr), &client_len);
        if (client_fd < 0) continue;

        // One thread per client -- concurrent sessions are independent.
        std::lock_guard<std::mutex> lock(g_clients_mutex);
        g_client_fds.push_back(client_fd);
        g_client_threads.emplace_back(handle_client, client_fd);
    }

    ::close(fd);
    g_server_fd = -1;
}

void psp_debug_socket_start(uint8_t* rdram, size_t rdram_size, int port) {
    g_rdram      = rdram;
    g_rdram_size = rdram_size;
    g_start_time = std::chrono::steady_clock::now();
    g_start_time_set = true;
    g_debug_running.store(true, std::memory_order_relaxed);
    g_debug_thread = std::thread(server_loop, port);
}

void psp_debug_socket_stop() {
    if (!g_debug_running.load(std::memory_order_relaxed)) return;
    g_debug_running.store(false, std::memory_order_relaxed);
    // Wake the accept() select() by shutting down the server socket
    if (g_server_fd >= 0) {
        ::shutdown(g_server_fd, SHUT_RDWR);
    }
    if (g_debug_thread.joinable()) {
        g_debug_thread.join();
    }
    // Unblock every client handler stuck in recv(), then join them.
    {
        std::lock_guard<std::mutex> lock(g_clients_mutex);
        for (int fd : g_client_fds) {
            ::shutdown(fd, SHUT_RDWR);
        }
    }
    for (auto& t : g_client_threads) {
        if (t.joinable()) t.join();
    }
    g_client_threads.clear();
}
