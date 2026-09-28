// Unit + socket-level tests for the debug socket v2 protocol (issue #35).
//
// Layer 1: psp_debug_handle_line() against a fake rdram buffer and fake
//          hooks — framing, ERR paths, W/R round-trip, I JSON shape.
// Layer 2: a real server on a test port with two CONCURRENT clients —
//          proves the multi-client accept loop (v1 served one at a time).
//
// Mirrors the test_vfpu harness conventions (counters + exit code).

#include "psp_debug_socket.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

static int failures = 0;
static int tests_run = 0;

#define ASSERT_TRUE(cond, msg) \
    do { \
        tests_run++; \
        if (!(cond)) { \
            std::fprintf(stderr, "FAIL: %s\n", msg); \
            failures++; \
        } \
    } while (0)

#define ASSERT_STR_EQ(actual, expected, msg) \
    do { \
        tests_run++; \
        if (std::string(actual) != std::string(expected)) { \
            std::fprintf(stderr, "FAIL: %s: got \"%s\", expected \"%s\"\n", \
                msg, std::string(actual).c_str(), \
                std::string(expected).c_str()); \
            failures++; \
        } \
    } while (0)

static constexpr size_t TEST_RDRAM_SIZE = 1 << 20;  // 1MB fake rdram
static constexpr int TEST_PORT = 38999;

// ===================================================================
// Fake hooks for I and S
// ===================================================================

static void fake_ge_stats(uint32_t* frames, int* prims, int* real_ns,
                          int* sprite, int* clears) {
    *frames = 42;
    *prims = 1234;
    *real_ns = 7;
    *sprite = 100;
    *clears = 50;
}

static void fake_miss_stats(uint32_t* unique_addrs, uint64_t* total) {
    *unique_addrs = 3;
    *total = 99;
}

static int fake_recent_funcs(uint32_t* out, int max) {
    if (max < 3) return 0;
    out[0] = 0x08123456;
    out[1] = 0x0885FE90;
    out[2] = 0x089ACCD0;
    return 3;
}

static int fake_thread_list(PspDebugThreadInfo* out, int max) {
    if (max < 2) return 0;
    out[0] = PspDebugThreadInfo{0, "user_main", "RUNNING", ""};
    // Hostile name: embedded quote + control char must be JSON-escaped.
    out[1] = PspDebugThreadInfo{1, "ev\"il\x01", "WAIT", "sema:259"};
    return 2;
}

static bool fake_capture_ok(const char* path, int timeout_ms) {
    (void)path; (void)timeout_ms;
    return true;
}

static bool fake_capture_fail(const char* path, int timeout_ms) {
    (void)path; (void)timeout_ms;
    return false;
}

// ===================================================================
// Layer 1: command handler unit tests
// ===================================================================

static void test_err_framing(uint8_t* rdram) {
    PspDebugReply r;

    r = psp_debug_handle_line("X 1 2", rdram, TEST_RDRAM_SIZE);
    ASSERT_STR_EQ(r.header, "ERR unknown-command\n", "unknown command");
    ASSERT_TRUE(r.payload.empty(), "ERR has no payload");

    r = psp_debug_handle_line("", rdram, TEST_RDRAM_SIZE);
    ASSERT_TRUE(r.header.rfind("ERR ", 0) == 0, "empty line gets ERR");

    r = psp_debug_handle_line("R", rdram, TEST_RDRAM_SIZE);
    ASSERT_TRUE(r.header.rfind("ERR ", 0) == 0, "bare R gets ERR");

    r = psp_debug_handle_line("R zz 4", rdram, TEST_RDRAM_SIZE);
    ASSERT_STR_EQ(r.header, "ERR bad-addr\n", "R with garbage addr");

    r = psp_debug_handle_line("R 1000", rdram, TEST_RDRAM_SIZE);
    ASSERT_STR_EQ(r.header, "ERR bad-size\n", "R missing size");

    r = psp_debug_handle_line("R 1000 0", rdram, TEST_RDRAM_SIZE);
    ASSERT_STR_EQ(r.header, "ERR size-out-of-range\n", "R size 0");

    r = psp_debug_handle_line("R 1000 999999", rdram, TEST_RDRAM_SIZE);
    ASSERT_STR_EQ(r.header, "ERR size-out-of-range\n", "R size too big");

    r = psp_debug_handle_line("W 1000 XYZ1", rdram, TEST_RDRAM_SIZE);
    ASSERT_STR_EQ(r.header, "ERR bad-bytes\n", "W non-hex bytes");

    r = psp_debug_handle_line("W 1000 ABC", rdram, TEST_RDRAM_SIZE);
    ASSERT_STR_EQ(r.header, "ERR bad-bytes\n", "W odd-length hex");

    r = psp_debug_handle_line("W FFFFC 0102030405060708",
                              rdram, TEST_RDRAM_SIZE);
    ASSERT_STR_EQ(r.header, "ERR out-of-range\n", "W past end of rdram");

    r = psp_debug_handle_line("B 4000", rdram, TEST_RDRAM_SIZE);
    ASSERT_STR_EQ(r.header, "ERR bad-duration\n", "B missing duration");

    r = psp_debug_handle_line("B zz 5", rdram, TEST_RDRAM_SIZE);
    ASSERT_STR_EQ(r.header, "ERR bad-mask\n", "B garbage mask");

    r = psp_debug_handle_line("S ", rdram, TEST_RDRAM_SIZE);
    ASSERT_STR_EQ(r.header, "ERR bad-path\n", "S empty path");
}

static void test_write_read_roundtrip(uint8_t* rdram) {
    PspDebugReply r;

    r = psp_debug_handle_line("W 1000 DEADBEEF", rdram, TEST_RDRAM_SIZE);
    ASSERT_STR_EQ(r.header, "OK 0\n", "W replies OK 0");
    ASSERT_TRUE(r.payload.empty(), "W has no payload");

    r = psp_debug_handle_line("R 1000 4", rdram, TEST_RDRAM_SIZE);
    ASSERT_STR_EQ(r.header, "OK 4\n", "R replies OK 4");
    ASSERT_TRUE(r.payload.size() == 4, "R payload is 4 bytes");
    const uint8_t expect[4] = {0xDE, 0xAD, 0xBE, 0xEF};
    ASSERT_TRUE(std::memcmp(r.payload.data(), expect, 4) == 0,
                "W then R round-trips the bytes");

    // Address masking: 0x08001000 & 0x07FFFFFF == 0x1000 -- same bytes.
    r = psp_debug_handle_line("R 08001000 4", rdram, TEST_RDRAM_SIZE);
    ASSERT_TRUE(r.payload.size() == 4
                && std::memcmp(r.payload.data(), expect, 4) == 0,
                "R masks the PSP address with 0x07FFFFFF");

    // Legacy RAW: same bytes, NO framing header.
    r = psp_debug_handle_line("RAW 1000 4", rdram, TEST_RDRAM_SIZE);
    ASSERT_TRUE(r.header.empty(), "RAW reply is unframed");
    ASSERT_TRUE(r.payload.size() == 4
                && std::memcmp(r.payload.data(), expect, 4) == 0,
                "RAW returns the legacy raw byte stream");

    // Reads past the end of rdram zero-fill (legacy semantics kept).
    r = psp_debug_handle_line("R FFFFC 8", rdram, TEST_RDRAM_SIZE);
    ASSERT_STR_EQ(r.header, "OK 8\n", "partially out-of-range R still OK");
    ASSERT_TRUE(r.payload.size() == 8 && r.payload[4] == 0
                && r.payload[7] == 0,
                "out-of-range R portion is zero-filled");
}

static void test_buttons(uint8_t* rdram) {
    PspDebugReply r = psp_debug_handle_line("B 4000 250",
                                            rdram, TEST_RDRAM_SIZE);
    ASSERT_STR_EQ(r.header, "OK 0\n", "B replies OK 0");
}

static void test_screenshot_hooks(uint8_t* rdram) {
    PspDebugReply r;

    PspDebugHooks none;
    psp_debug_socket_set_hooks(none);
    r = psp_debug_handle_line("S /tmp/x.tga", rdram, TEST_RDRAM_SIZE);
    ASSERT_STR_EQ(r.header, "ERR unsupported\n", "S without hook");

    PspDebugHooks ok_hooks;
    ok_hooks.capture_screenshot = fake_capture_ok;
    psp_debug_socket_set_hooks(ok_hooks);
    r = psp_debug_handle_line("S /tmp/x.tga", rdram, TEST_RDRAM_SIZE);
    ASSERT_STR_EQ(r.header, "OK 0\n", "S success path");

    PspDebugHooks fail_hooks;
    fail_hooks.capture_screenshot = fake_capture_fail;
    psp_debug_socket_set_hooks(fail_hooks);
    r = psp_debug_handle_line("S /tmp/x.tga", rdram, TEST_RDRAM_SIZE);
    ASSERT_STR_EQ(r.header, "ERR timeout\n", "S timeout path");
}

/// Light JSON sanity check (no jq in-process): balanced braces/brackets,
/// even quote count, expected substrings present.
static void test_info_json(uint8_t* rdram) {
    PspDebugHooks hooks;
    hooks.ge_stats = fake_ge_stats;
    hooks.lookup_miss_stats = fake_miss_stats;
    hooks.recent_funcs = fake_recent_funcs;
    hooks.thread_list = fake_thread_list;
    psp_debug_socket_set_hooks(hooks);

    PspDebugReply r = psp_debug_handle_line("I", rdram, TEST_RDRAM_SIZE);
    ASSERT_TRUE(r.header.rfind("OK ", 0) == 0, "I replies OK");
    size_t announced = std::strtoul(r.header.c_str() + 3, nullptr, 10);
    ASSERT_TRUE(announced == r.payload.size(),
                "I header length matches payload length");

    std::string j(r.payload.begin(), r.payload.end());
    ASSERT_TRUE(!j.empty() && j.front() == '{', "JSON starts with {");
    ASSERT_TRUE(j.size() >= 2 && j[j.size() - 2] == '}'
                && j.back() == '\n',
                "JSON is one }-terminated line");

    int braces = 0, brackets = 0, quotes = 0;
    bool in_str = false;
    for (size_t i = 0; i < j.size(); i++) {
        char c = j[i];
        if (c == '"' && (i == 0 || j[i - 1] != '\\')) {
            in_str = !in_str;
            quotes++;
        }
        if (in_str) continue;
        if (c == '{') braces++;
        if (c == '}') braces--;
        if (c == '[') brackets++;
        if (c == ']') brackets--;
    }
    ASSERT_TRUE(braces == 0, "JSON braces balanced");
    ASSERT_TRUE(brackets == 0, "JSON brackets balanced");
    ASSERT_TRUE(quotes % 2 == 0, "JSON quotes balanced");

    ASSERT_TRUE(j.find("\"uptime_sec\":") != std::string::npos,
                "JSON has uptime_sec");
    ASSERT_TRUE(j.find("\"frames\":42") != std::string::npos,
                "JSON carries ge.frames from hook");
    ASSERT_TRUE(j.find("\"prims\":1234") != std::string::npos,
                "JSON carries ge.prims from hook");
    ASSERT_TRUE(j.find("\"unique\":3") != std::string::npos,
                "JSON carries lookup_miss.unique from hook");
    ASSERT_TRUE(j.find("\"total\":99") != std::string::npos,
                "JSON carries lookup_miss.total from hook");
    ASSERT_TRUE(j.find("\"0x08123456\"") != std::string::npos,
                "JSON carries recent_funcs entries");
    ASSERT_TRUE(j.find("\"name\":\"user_main\"") != std::string::npos,
                "JSON carries thread name");
    ASSERT_TRUE(j.find("\"wait\":\"sema:259\"") != std::string::npos,
                "JSON carries wait reason");
    ASSERT_TRUE(j.find("ev\\\"il\\u0001") != std::string::npos,
                "hostile thread name is JSON-escaped");

    // Null hooks degrade to zeros/empty arrays, still valid framing.
    PspDebugHooks none;
    psp_debug_socket_set_hooks(none);
    r = psp_debug_handle_line("I", rdram, TEST_RDRAM_SIZE);
    std::string j2(r.payload.begin(), r.payload.end());
    ASSERT_TRUE(r.header.rfind("OK ", 0) == 0
                && j2.find("\"recent_funcs\":[]") != std::string::npos
                && j2.find("\"threads\":[]") != std::string::npos,
                "I degrades gracefully with no hooks");
}

// ===================================================================
// Layer 2: real server, two concurrent clients
// ===================================================================

static int connect_client() {
    int fd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) return -1;
    struct sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(TEST_PORT);
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    for (int attempt = 0; attempt < 50; attempt++) {
        if (::connect(fd, reinterpret_cast<sockaddr*>(&addr),
                      sizeof(addr)) == 0) {
            return fd;
        }
        ::usleep(20000);
    }
    ::close(fd);
    return -1;
}

static bool send_str(int fd, const char* s) {
    size_t len = std::strlen(s);
    return ::send(fd, s, len, 0) == static_cast<ssize_t>(len);
}

static std::string recv_line(int fd) {
    std::string line;
    char ch;
    while (::recv(fd, &ch, 1, 0) == 1) {
        line += ch;
        if (ch == '\n') break;
    }
    return line;
}

static std::vector<uint8_t> recv_exact(int fd, size_t n) {
    std::vector<uint8_t> buf(n);
    size_t got = 0;
    while (got < n) {
        ssize_t r = ::recv(fd, buf.data() + got, n - got, 0);
        if (r <= 0) break;
        got += static_cast<size_t>(r);
    }
    buf.resize(got);
    return buf;
}

static void test_concurrent_clients(uint8_t* rdram) {
    psp_debug_socket_set_hooks(PspDebugHooks{});
    psp_debug_socket_start(rdram, TEST_RDRAM_SIZE, TEST_PORT);

    int a = connect_client();
    int b = connect_client();
    ASSERT_TRUE(a >= 0 && b >= 0, "two clients connect");
    if (a < 0 || b < 0) {
        psp_debug_socket_stop();
        return;
    }

    // Client B works while A stays connected and idle -- the v1 server
    // (one client at a time) would leave B unanswered here.
    ASSERT_TRUE(send_str(b, "W 3000 0102\n"), "B sends W");
    ASSERT_STR_EQ(recv_line(b), "OK 0\n", "client B write while A idle");

    // Now interleave both clients.
    ASSERT_TRUE(send_str(a, "R 3000 2\n"), "A sends R");
    std::string ha = recv_line(a);
    ASSERT_STR_EQ(ha, "OK 2\n", "client A read header");
    std::vector<uint8_t> pa = recv_exact(a, 2);
    ASSERT_TRUE(pa.size() == 2 && pa[0] == 0x01 && pa[1] == 0x02,
                "client A sees client B's write");

    ASSERT_TRUE(send_str(b, "I\n"), "B sends I");
    std::string hb = recv_line(b);
    ASSERT_TRUE(hb.rfind("OK ", 0) == 0, "client B info header");
    size_t blen = std::strtoul(hb.c_str() + 3, nullptr, 10);
    std::vector<uint8_t> pb = recv_exact(b, blen);
    ASSERT_TRUE(pb.size() == blen && !pb.empty() && pb[0] == '{',
                "client B info payload");

    // Oversized line gets ERR, connection stays usable.
    std::string longline(5000, 'A');
    longline += '\n';
    ASSERT_TRUE(send_str(a, longline.c_str()), "A sends oversized line");
    ASSERT_STR_EQ(recv_line(a), "ERR line-too-long\n",
                  "oversized line gets ERR");
    ASSERT_TRUE(send_str(a, "R 3000 2\n"), "A sends R after ERR");
    ASSERT_STR_EQ(recv_line(a), "OK 2\n",
                  "connection survives an oversized line");
    recv_exact(a, 2);

    ::close(a);
    ::close(b);
    psp_debug_socket_stop();
    tests_run++;  // reaching here without hanging is the assertion
}

// ===================================================================
// main
// ===================================================================

int main() {
    std::printf("Running debug socket v2 tests...\n\n");

    std::vector<uint8_t> rdram(TEST_RDRAM_SIZE, 0);

    test_err_framing(rdram.data());
    test_write_read_roundtrip(rdram.data());
    test_buttons(rdram.data());
    test_screenshot_hooks(rdram.data());
    test_info_json(rdram.data());
    test_concurrent_clients(rdram.data());

    std::printf("\n%d tests run, %d failures\n", tests_run, failures);

    if (failures == 0) {
        std::printf("ALL TESTS PASSED\n");
        return 0;
    }
    std::printf("SOME TESTS FAILED\n");
    return 1;
}
