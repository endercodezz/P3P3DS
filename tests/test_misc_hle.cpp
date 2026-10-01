// Small services: sceDmacMemcpy/TryMemcpy (pspautotests dmac/dmactest.expected),
// sceKernelPowerTick (uOFW sysmem/suspend.c), sceUmdGetDriveStat
// (pspautotests umd/wait/wait.expected).
#include "p3p3ds/hle/hle_modules.hpp"
#include "p3p3ds/kernel_state.hpp"
#include "psprecomp/runtime.hpp"

#include <iostream>
#include <string>

namespace {
int failures = 0;
#define CHECK(x) do { if (!(x)) { std::cerr << __LINE__ << ": " << #x << "\n"; ++failures; } } while (0)

struct Env {
    psprecomp::Runtime rt;
    p3p3ds::KernelState k;
    Env() {
        p3p3ds::hle::register_all_hle_modules(rt, k);
        k.threads().init_root_thread("root", 0x08804000u, 0x09FFFF00u, 0u);
    }
    std::uint32_t call(const char *lib, std::uint32_t nid, std::initializer_list<std::uint32_t> args) {
        auto &ctx = rt.cpu();
        std::uint32_t reg = 4u;
        for (auto v : args) ctx.gpr[reg++] = v;
        ctx.gpr[31] = 0x08A00100u;
        ctx.pc = 0x08A00000u;
        rt.invoke_import(lib, nid, ctx);
        return ctx.gpr[2];
    }
};
} // namespace

int main() {
    Env env;
    auto &m = env.rt.memory();
    const std::string text = "Hello World. This is a test to check if sceDmacMemcpy works.";
    constexpr std::uint32_t kSrc = 0x08900000u, kDst = 0x08910000u;
    for (std::uint32_t i = 0; i <= text.size(); ++i) m.store8(kSrc + i, i < text.size() ? static_cast<std::uint8_t>(text[i]) : 0u);
    for (const auto nid : {0x617F3FE6u, 0xD97F94D8u}) {
        m.zero(kDst, 128);
        CHECK(env.call("sceDmac", nid, {kDst, kSrc, static_cast<std::uint32_t>(text.size() + 1)}) == 0u);
        bool same = true;
        for (std::uint32_t i = 0; i <= text.size(); ++i) same &= m.load8(kDst + i) == m.load8(kSrc + i);
        CHECK(same);
        CHECK(env.call("sceDmac", nid, {0u, 0u, 0u}) == 0x80000104u);
        CHECK(env.call("sceDmac", nid, {0u, 0u, 4u}) == 0x80000103u);
        CHECK(env.call("sceDmac", nid, {kDst, kSrc, 0u}) == 0x80000104u);
        CHECK(env.call("sceDmac", nid, {0x04000000u, 0x04100000u, 0x00100000u}) == 0u); // 1 MB VRAM copy
    }
    CHECK(env.call("sceSuspendForUser", 0x090CCB3Fu, {0u}) == 0u);
    // Drive status: no disc, disc present, activated.
    CHECK(env.call("sceUmdUser", 0x6B4A146Cu, {}) == 0x01u);
    env.k.umd().set_medium_present(true);
    CHECK(env.call("sceUmdUser", 0x6B4A146Cu, {}) == 0x12u);
    constexpr std::uint32_t kAlias = 0x08920000u;
    const std::string alias = "disc0:";
    for (std::uint32_t i = 0; i <= alias.size(); ++i) m.store8(kAlias + i, i < alias.size() ? static_cast<std::uint8_t>(alias[i]) : 0u);
    CHECK(env.call("sceUmdUser", 0xC6183D47u, {1u, kAlias}) == 0u);
    CHECK(env.call("sceUmdUser", 0x6B4A146Cu, {}) == 0x32u);
    std::cout << (failures ? "FAILED" : "PASS") << " (" << failures << " failures)\n";
    return failures ? 1 : 0;
}
