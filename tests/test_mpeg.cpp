// sceMpeg against PSP hardware output.
// 1. Replays references/pspautotests/tests/video/mpeg/basic.c (createTestMpeg,
//    registMpegStreams, loadMpegFile, 180 frames of testDecodeVideo on
//    test.pmf) and compares the transcript with basic.expected line by line.
//    The guest read callback is a host function at a fake guest address that
//    is reached through ThreadManager::call_guest like real guest code.
// 2. ringbuffer/{memsize,construct,avail,destruct}.expected value tables.
#include "p3p3ds/hle/hle_modules.hpp"
#include "p3p3ds/hle/mpeg.hpp"
#include "p3p3ds/kernel_state.hpp"
#include "psprecomp/runtime.hpp"

#include <cstdio>
#include <fstream>
#include <iostream>
#include <iterator>
#include <sstream>
#include <string>
#include <vector>

#ifndef P3P_SOURCE_DIR
#define P3P_SOURCE_DIR "."
#endif

namespace {
int failures = 0;
#define CHECK(x) do { if (!(x)) { std::cerr << __LINE__ << ": " << #x << "\n"; ++failures; } } while (0)

constexpr std::uint32_t kStub = 0x08A00000u, kRet = 0x08A00100u, kCallback = 0x08A00200u;
constexpr std::uint32_t kMpegPtr = 0x08B00000u, kRingbuffer = 0x08B00100u, kAvcAu = 0x08B00200u, kAtracAu = 0x08B00240u;
constexpr std::uint32_t kScratch = 0x08B00300u, kHeader = 0x08B01000u, kMpegData = 0x08C00000u;
constexpr std::uint32_t kAtracData = 0x08C20000u, kVideo = 0x08D00000u, kRingData = 0x09000000u;

struct Env {
    psprecomp::Runtime rt;
    p3p3ds::KernelState k;
    Env() {
        p3p3ds::hle::register_all_hle_modules(rt, k);
        k.threads().set_import_stubs({kStub});
        k.threads().init_root_thread("root", 0x08804000u, 0x09FFFF00u, 0u);
        rt.cpu().pc = 0x08804000u;
    }
    // One syscall; guest calls it starts are dispatched until the caller's
    // return address is reached (as the runtime dispatcher would).
    std::uint32_t call(std::uint32_t nid, std::initializer_list<std::uint32_t> args) {
        auto &ctx = rt.cpu();
        std::uint32_t reg = 4u;
        for (auto v : args) ctx.gpr[reg++] = v;
        ctx.gpr[31] = kRet;
        ctx.pc = kStub;
        rt.invoke_import("sceMpeg", nid, ctx);
        if (ctx.pc == kStub) ctx.pc = kRet;
        for (int guard = 0; ctx.pc != kRet && guard < 64 && !rt.stopped(); ++guard)
            if (!rt.invoke_isolated_aot(ctx.pc, ctx)) { std::cerr << "no function at " << std::hex << ctx.pc << "\n"; ++failures; break; }
        return ctx.gpr[2];
    }
};

std::string fmt(const char *f, auto... a) {
    char buf[256];
    std::snprintf(buf, sizeof buf, f, a...);
    return buf;
}

// pspautotests/tests/video/mpeg/basic.c
std::vector<std::string> run_basic(const std::vector<std::uint8_t> &pmf) {
    Env env;
    auto &m = env.rt.memory();
    std::vector<std::string> out;
    std::size_t file_pos = 0;
    auto file_read = [&](std::uint32_t dest, std::size_t bytes) {
        const auto n = std::min(bytes, pmf.size() - file_pos);
        for (std::size_t i = 0; i < n; ++i) m.store8(dest + static_cast<std::uint32_t>(i), pmf[file_pos + i]);
        file_pos += n;
        return static_cast<std::uint32_t>(n);
    };
    static std::vector<std::string> *log = nullptr;
    static decltype(file_read) *reader = nullptr;
    log = &out;
    reader = &file_read;
    env.rt.register_function(kCallback, [](psprecomp::Runtime &, psprecomp::AllegrexContext &ctx) {
        const std::uint32_t data = ctx.gpr[4];
        const auto packets = static_cast<std::int32_t>(ctx.gpr[5]);
        log->push_back(fmt("  - mpeg_callback called: %08x, %d, %08x", data - kRingData, packets, ctx.gpr[6]));
        std::uint32_t result = 0;
        if (packets > 0) {
            const auto read = (*reader)(data, 2048u * static_cast<std::uint32_t>(packets));
            log->push_back(fmt("    sceIoRead: %08x, returning %d", read, read / 2048));
            result = read / 2048u;
        } else {
            log->push_back("    asked for negative packets, ignoring");
        }
        ctx.gpr[2] = result;
        ctx.pc = ctx.gpr[31];
    }, "test_mpeg_callback");
    auto au_line = [&](std::uint32_t au, const char *es) {
        const auto ph = m.load32(au), pl = m.load32(au + 4), dh = m.load32(au + 8), dl = m.load32(au + 12);
        const bool plain = ph == 0xFFFFFFFFu && pl == 0xFFFFFFFFu && dh == 0xFFFFFFFFu && dl == 0xFFFFFFFFu;
        out.push_back(fmt(plain ? "    Au: presented=%d/%d, decoded=%d/%d, es=%s, size=%d"
                                : "    Au (interesting): presented=%d/%d, decoded=%d/%d, es=%s, size=%d",
                          ph, pl, dh, dl, es, m.load32(au + 20)));
    };

    env.call(0x682A619Bu, {}); // sceMpegInit
    out.push_back("createTestMpeg:");
    const auto rb_size = env.call(0xD7A29F46u, {512u});
    out.push_back(fmt("  sceMpegRingbufferQueryMemSize: %08x", rb_size));
    const auto mem_size = env.call(0xC132E22Fu, {0u});
    out.push_back(fmt("  sceMpegQueryMemSize: %08x", mem_size));
    out.push_back(fmt("  sceMpegRingbufferConstruct: %08x", env.call(0x37295ED8u, {kRingbuffer, 512u, kRingData, rb_size, kCallback, 0x1234u})));
    out.push_back(fmt("  sceMpegCreate: %08x", env.call(0xD8C5F121u, {kMpegPtr, kMpegData, mem_size, kRingbuffer, 512u, 0u, 0u})));
    std::string magic;
    for (std::uint32_t i = 0; i < 8u && m.load8(m.load32(kMpegPtr) + i); ++i) magic += static_cast<char>(m.load8(m.load32(kMpegPtr) + i));
    out.push_back("  MPEG: " + magic);
    out.push_back("");

    out.push_back("registMpegStreams:");
    const auto avc_stream = env.call(0x42560F23u, {kMpegPtr, 0u, 0u});
    out.push_back(fmt("  sceMpegRegistStream: %08x", avc_stream - m.load32(kMpegPtr)));
    const auto atrac_stream = env.call(0x42560F23u, {kMpegPtr, 1u, 0u});
    out.push_back(fmt("  sceMpegRegistStream: %08x", atrac_stream - m.load32(kMpegPtr)));
    const auto avc_buf = env.call(0xA780CF7Eu, {kMpegPtr});
    out.push_back(fmt("  sceMpegMallocAvcEsBuf: %08x", avc_buf));
    out.push_back(fmt("  sceMpegInitAu AVC: %08x", env.call(0x167AFD9Eu, {kMpegPtr, avc_buf, kAvcAu})));
    const auto r = env.call(0xF8DCB679u, {kMpegPtr, kScratch, kScratch + 4});
    out.push_back(fmt("  sceMpegQueryAtracEsSize: %08x (%08x, %08x)", r, m.load32(kScratch), m.load32(kScratch + 4)));
    out.push_back(fmt("  sceMpegInitAu ATRAC: %08x", env.call(0x167AFD9Eu, {kMpegPtr, kAtracData, kAtracAu})));
    out.push_back("");

    out.push_back("loadMpegFile:");
    out.push_back("  sceIoOpen: OK");
    out.push_back(fmt("  sceIoRead: %08x", file_read(kHeader, 2048)));
    const auto ro = env.call(0x21FF80E4u, {kMpegPtr, kHeader, kScratch});
    out.push_back(fmt("  sceMpegQueryStreamOffset: %08x (%08x)", ro, m.load32(kScratch)));
    const auto rs = env.call(0x611E9E11u, {kHeader, kScratch + 4});
    out.push_back(fmt("  sceMpegQueryStreamSize: %08x (%08x)", rs, m.load32(kScratch + 4)));
    file_pos = m.load32(kScratch);
    out.push_back("");

    out.push_back("testDecodeVideo:");
    out.push_back("");
    constexpr std::uint32_t kAttr = kScratch + 16, kVbufPtr = kScratch + 20, kInit = kScratch + 24, kAbuf = kScratch + 28;
    m.store32(kVbufPtr, kVideo);
    for (int frame = 0; frame < 180; ++frame) {
        out.push_back("Next frame");
        auto free_packets = env.call(0xB5F6DC87u, {kRingbuffer});
        out.push_back(fmt("  sceMpegRingbufferAvailableSize: %d", free_packets));
        out.push_back(fmt("  sceMpegRingbufferPut: %08x", env.call(0xB240A59Eu, {kRingbuffer, 24u, free_packets})));
        free_packets = env.call(0xB5F6DC87u, {kRingbuffer});
        out.push_back(fmt("  sceMpegRingbufferAvailableSize: %d", free_packets));
        auto res = env.call(0xE1CE83A7u, {kMpegPtr, atrac_stream, kAtracAu, kAbuf});
        out.push_back(fmt("  sceMpegGetAtracAu: %08x (at %08x)", res, m.load32(kAbuf)));
        au_line(kAtracAu, "ATRAC");
        out.push_back(fmt("  sceMpegAtracDecode: %08x", env.call(0x800C44DFu, {kMpegPtr, kAtracAu, m.load32(kAbuf), 1u})));
        au_line(kAtracAu, "ATRAC");
        m.store32(kAttr, 6u);
        res = env.call(0xFE246728u, {kMpegPtr, avc_stream, kAvcAu, kAttr});
        out.push_back(fmt("  sceMpegGetAvcAu: %08x (%08x)", res, m.load32(kAttr)));
        au_line(kAvcAu, "AVC");
        res = env.call(0x0E3C2E9Du, {kMpegPtr, kAvcAu, 512u, kVbufPtr, kInit});
        out.push_back(fmt("  sceMpegAvcDecode: %08x (%08x, %08x)", res, 512u, m.load32(kInit)));
        au_line(kAvcAu, "AVC");
        out.push_back(fmt(" *** Frame: %d ***", frame));
        out.push_back("");
    }
    out.back() = fmt("  sceMpegAvcDecodeFlush: %08x", env.call(0x4571CC64u, {kMpegPtr}));
    return out;
}

std::vector<std::string> expected_lines(const std::string &path) {
    std::ifstream in(path);
    std::vector<std::string> lines;
    for (std::string line; std::getline(in, line);) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.size() >= 4 && line[0] == '[' && line[2] == ']') line = line.substr(4);
        // The ATRAC buffer address is specific to the original test's heap.
        if (const auto at = line.find(" from "); at != std::string::npos && line.find("GetAtracAu") != std::string::npos)
            line = line.substr(0, at) + ")";
        lines.push_back(line);
    }
    return lines;
}

void test_ringbuffer_tables() {
    Env env;
    auto &m = env.rt.memory();
    // memsize.expected
    const std::pair<std::uint32_t, std::uint32_t> memsize[] = {
        {0xFFFFFFFFu, 0xFFFFF798u}, {0u, 0u}, {1u, 0x868u}, {10u, 0x5410u}, {512u, 0x10D000u}, {4097u, 0x868868u},
        {0x7FFFFFFFu, 0xFFFFF798u}, {0x80000000u, 0u}};
    for (auto [n, size] : memsize) CHECK(env.call(0xD7A29F46u, {n}) == size);
    // construct.expected
    const auto big = env.call(0xD7A29F46u, {4096u});
    CHECK(env.call(0x37295ED8u, {kRingbuffer, 4097u, kRingData, big, kCallback, 0xDEADBEEFu}) == 0x80610022u);
    CHECK(env.call(0x37295ED8u, {kRingbuffer, 0xFFFFFFFFu, kRingData, big, kCallback, 0xDEADBEEFu}) == 0u);
    CHECK(m.load32(kRingbuffer + 32) - m.load32(kRingbuffer + 20) == static_cast<std::uint32_t>(-2048));
    CHECK(env.call(0x37295ED8u, {kRingbuffer, 0u, kRingData, 0xFFFFFFFFu, kCallback, 0u}) == 0x80610022u);
    CHECK(env.call(0x37295ED8u, {kRingbuffer, 0u, kRingData, 0x80000000u, kCallback, 0u}) == 0x80610022u);
    CHECK(env.call(0x37295ED8u, {kRingbuffer, 0u, kRingData, 0x7FFFFFFFu, kCallback, 0u}) == 0u);
    CHECK(env.call(0x37295ED8u, {kRingbuffer, 512u, kRingData, env.call(0xD7A29F46u, {512u}), kCallback, 0xDEADC0DEu}) == 0u);
    CHECK(m.load32(kRingbuffer) == 512u && m.load32(kRingbuffer + 16) == 2048u && m.load32(kRingbuffer + 28) == 0xDEADC0DEu);
    CHECK(m.load32(kRingbuffer + 32) - m.load32(kRingbuffer + 20) == 1048576u && m.load32(kRingbuffer + 40) == 0u);
    // destruct.expected: always 0, fields kept.
    m.store32(kRingbuffer + 12, 90u);
    CHECK(env.call(0x13407F13u, {kRingbuffer}) == 0u && env.call(0x13407F13u, {0u}) == 0u && m.load32(kRingbuffer + 12) == 90u);
    // avail.expected
    env.call(0x37295ED8u, {kRingbuffer, 512u, kRingData, env.call(0xD7A29F46u, {512u}), kCallback, 0x1234u});
    env.call(0xD8C5F121u, {kMpegPtr, kMpegData, 0x10000u, kRingbuffer, 512u, 0u, 0u});
    CHECK(env.call(0xB5F6DC87u, {kRingbuffer}) == 0x200u);
    env.call(0x606A4649u, {kMpegPtr});
    CHECK(env.call(0xB5F6DC87u, {kRingbuffer}) == 0x80618009u);
    env.call(0xD8C5F121u, {kMpegPtr, kMpegData, 0x10000u, kRingbuffer, 512u, 0u, 0u});
    m.store32(kRingbuffer + 12, 512u);
    CHECK(env.call(0xB5F6DC87u, {kRingbuffer}) == 0u);
    m.store32(kRingbuffer + 12, 1u);
    CHECK(env.call(0xB5F6DC87u, {kRingbuffer}) == 0x1FFu);
}

} // namespace

int main() {
    const std::string dir = std::string(P3P_SOURCE_DIR) + "/references/pspautotests/tests/video/mpeg/";
    std::ifstream pmf_file(dir + "test.pmf", std::ios::binary);
    if (!pmf_file) { std::cout << "SKIP: test.pmf not found\n"; return 77; }
    const std::vector<std::uint8_t> pmf((std::istreambuf_iterator<char>(pmf_file)), std::istreambuf_iterator<char>());
    const auto got = run_basic(pmf);
    const auto want = expected_lines(dir + "basic.expected");
    std::size_t mismatches = 0;
    for (std::size_t i = 0; i < std::max(got.size(), want.size()); ++i) {
        const auto &g = i < got.size() ? got[i] : std::string("<missing>");
        const auto &w = i < want.size() ? want[i] : std::string("<missing>");
        if (g != w && ++mismatches <= 10) std::cerr << "line " << i + 1 << ":\n  got:  " << g << "\n  want: " << w << "\n";
    }
    CHECK(mismatches == 0);
    std::cout << "basic.expected lines compared=" << want.size() << " mismatches=" << mismatches << "\n";
    test_ringbuffer_tables();
    std::cout << (failures ? "FAILED" : "PASS") << " (" << failures << " failures)\n";
    return failures ? 1 : 0;
}
