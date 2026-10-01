// sceSasCore against pspautotests hardware expectations.
// The call sequences of references/pspautotests/tests/audio/sascore/adsrcurve.cpp
// (testAttackCurve / testDecayCurve, parsed from the source) are replayed
// through our HLE and the printed lines compared with adsrcurve.expected
// (the [x]/[r] channel tags are ignored). Parameter checks follow
// sascore/keyon/keyoff/pitch/pcm/vag.expected.
#include "p3p3ds/hle/hle_modules.hpp"
#include "p3p3ds/kernel_state.hpp"
#include "psprecomp/runtime.hpp"

#include <cstdio>
#include <fstream>
#include <initializer_list>
#include <iostream>
#include <map>
#include <regex>
#include <set>
#include <sstream>
#include <string>
#include <vector>

using namespace p3p3ds::hle;
namespace {
int failures = 0;
#define CHECK(x) do { if (!(x)) { std::cerr << __LINE__ << ": " << #x << "\n"; ++failures; } } while (0)

constexpr std::uint32_t kCore = 0x08900000u, kPcm = 0x08910000u, kOut = 0x08920000u;
const std::string kDir = "references/pspautotests/tests/audio/sascore/";

struct Sas {
    psprecomp::Runtime rt;
    p3p3ds::KernelState k;
    Sas() { register_all_hle_modules(rt, k); }
    std::uint32_t call(std::uint32_t nid, std::initializer_list<std::uint32_t> args) {
        auto &ctx = rt.cpu();
        std::uint32_t r = 4u;
        for (auto v : args) ctx.gpr[r++] = v;
        rt.invoke_import("sceSasCore", nid, ctx);
        return ctx.gpr[2];
    }
    std::uint32_t init(std::uint32_t grain, std::uint32_t voices = 32u, std::uint32_t mode = 1u, std::uint32_t rate = 44100u, std::uint32_t core = kCore) {
        return call(0x42778A9Fu, {core, grain, voices, mode, rate});
    }
    std::uint32_t core() { return call(0xA3589D81u, {kCore, kOut}); }
    std::int32_t height() { return static_cast<std::int32_t>(call(0x74AE582Au, {kCore, 0u})); }
};

std::string fmt(const char *f, std::uint32_t a, int b, std::uint32_t c) { char buf[128]; std::snprintf(buf, sizeof buf, f, a, b, c); return buf; }
std::string fmt2(const char *f, std::uint32_t a, int b) { char buf[128]; std::snprintf(buf, sizeof buf, f, a, b); return buf; }

std::vector<std::string> run_curve(Sas &s, bool attack, int type, std::uint32_t rate) {
    std::vector<std::string> out;
    s.call(0xA0CF2FA4u, {kCore, 0u});                               // key off
    s.core();
    s.call(0xE1CD9561u, {kCore, 0u, kPcm, 256u, 0u});               // PCM voice
    const auto result = attack ? s.call(0x9EC3676Au, {kCore, 0u, 7u, static_cast<std::uint32_t>(type), 1u, 1u, 5u})
                               : s.call(0x9EC3676Au, {kCore, 0u, 7u, 0u, static_cast<std::uint32_t>(type), 1u, 5u});
    if (static_cast<std::int32_t>(result) < 0) { out.push_back(fmt2("  Failed (%08x)", result, 0)); return out; }
    if (attack) s.call(0x019B25EBu, {kCore, 0u, 7u, rate, 1u, 1u, 0u});
    else s.call(0x019B25EBu, {kCore, 0u, 7u, 0x40000000u / 32u, rate, 1u, 0u});
    s.call(0x76F01ACAu, {kCore, 0u});                               // key on
    std::int32_t last = 0, last_change = 0, last_at = 0;
    const int count = attack ? 256 : 128;
    if (!attack) { s.core(); last = s.height(); }
    for (int i = 0; i < count; ++i) {
        s.core();
        const auto h = s.height();
        const auto change = h - last;
        if (last_change != change) {
            out.push_back(change < 0 ? fmt("  Height: -%08x after %d (%08x)", static_cast<std::uint32_t>(-change), i - last_at, static_cast<std::uint32_t>(h))
                                     : fmt("  Height: +%08x after %d (%08x)", static_cast<std::uint32_t>(change), i - last_at, static_cast<std::uint32_t>(h)));
            last_change = change; last_at = i;
        }
        last = h;
    }
    out.push_back(last_change < 0 ? fmt2("  Height: -%08x after %d", static_cast<std::uint32_t>(-last_change), count - last_at)
                                   : fmt2("  Height: +%08x after %d", static_cast<std::uint32_t>(last_change), count - last_at));
    return out;
}

void adsr_curves() {
    std::ifstream src(kDir + "adsrcurve.cpp"), exp(kDir + "adsrcurve.expected");
    if (!src || !exp) { std::cout << "adsrcurve sources absent; skipped\n"; return; }
    std::map<std::string, std::vector<std::string>> expected;
    std::string line, title;
    while (std::getline(exp, line)) {
        if (line.size() < 4) continue;
        const auto text = line.substr(4);
        if (!text.empty() && text[0] != ' ') { // block title; repeated titles keep the first block
            title = expected.count(text) ? std::string{} : text;
            if (!title.empty()) expected[title];
            continue;
        }
        if (!title.empty()) expected[title].push_back(text);
    }
    const std::map<std::string, int> modes{{"PSP_SAS_ADSR_CURVE_MODE_LINEAR_INCREASE", 0}, {"PSP_SAS_ADSR_CURVE_MODE_LINEAR_DECREASE", 1},
        {"PSP_SAS_ADSR_CURVE_MODE_LINEAR_BENT", 2}, {"PSP_SAS_ADSR_CURVE_MODE_EXPONENT_REV", 3}, {"PSP_SAS_ADSR_CURVE_MODE_EXPONENT", 4},
        {"PSP_SAS_ADSR_CURVE_MODE_DIRECT", 5}};
    const std::regex call(R"re(test(Attack|Decay)Curve\("([^"]+)",\s*(\w+),\s*(0x[0-9A-Fa-f]+|\d+)\);)re");
    std::stringstream all; all << src.rdbuf();
    const std::string text = all.str();
    Sas s;
    s.init(64u);
    for (std::uint32_t i = 0; i < 256u * 2u * 16u; ++i) s.rt.memory().store16(kPcm + 2u * i, 0x7FFFu);
    int cases = 0, matched = 0;
    std::set<std::string> seen;
    for (std::sregex_iterator it(text.begin(), text.end(), call), end; it != end; ++it) {
        const auto t = (*it)[2].str();
        const bool attack = (*it)[1] == "Attack";
        const auto rate = static_cast<std::uint32_t>(std::stoul((*it)[4].str(), nullptr, 0));
        const auto got = run_curve(s, attack, modes.at((*it)[3].str()), rate);
        if (seen.count(t)) continue; // duplicated titles: compare the first block only
        seen.insert(t);
        ++cases;
        if (got == expected[t]) { ++matched; continue; }
        if (cases - matched <= 5) {
            std::cerr << "ADSR MISMATCH " << t << "\n";
            for (std::size_t i = 0; i < std::max(got.size(), expected[t].size()) && i < 6; ++i)
                std::cerr << "  got: " << (i < got.size() ? got[i] : "-") << " | exp: " << (i < expected[t].size() ? expected[t][i] : "-") << "\n";
        }
    }
    std::cout << "adsrcurve cases=" << cases << " matched=" << matched << "\n";
    CHECK(cases >= 40 && matched == cases);
}

void parameters() {
    Sas s;
    // sascore.expected
    CHECK(s.init(0x100u, 32u, 0u, 44100u, 0u) == 0x80420005u);
    CHECK(s.init(0x100u, 32u, 0u, 44100u, kCore + 4u) == 0x80420005u);
    for (auto g : {0x20u, 0x5Cu, 0xC00u}) CHECK(s.init(g) == 0x80420001u);
    for (auto g : {0x40u, 0x100u, 0x7C0u, 0x800u}) CHECK(s.init(g) == 0u);
    CHECK(s.init(0x100u, 0u) == 0x80420002u && s.init(0x100u, 33u) == 0x80420002u && s.init(0x100u, 1u) == 0u);
    CHECK(s.init(0x100u, 32u, 2u) == 0x80420003u);
    CHECK(s.init(0x100u, 32u, 0u, 48000u) == 0x80420004u);
    CHECK(s.init(0x100u, 32u, 0u, 44100u) == 0u);
    CHECK(s.call(0x440CA7D8u, {kCore, 0u, 0x1001u, 0u, 0u, 0u}) == 0x80420018u);
    CHECK(s.call(0x440CA7D8u, {kCore, 0u, static_cast<std::uint32_t>(-4096), 4096u, 0u, 0u}) == 0u);
    CHECK(s.call(0x440CA7D8u, {kCore, 32u, 0u, 0u, 0u, 0u}) == 0x80420010u);
    // pitch.expected
    CHECK(s.call(0xAD84D37Fu, {kCore, 0u, 0x4000u}) == 0u && s.call(0xAD84D37Fu, {kCore, 0u, 0x4001u}) == 0x80420012u);
    CHECK(s.call(0xAD84D37Fu, {kCore, 0u, 0xFFFFFFFFu}) == 0x80420012u);
    // pcm.expected / vag.expected sizes and loop positions
    CHECK(s.call(0xE1CD9561u, {kCore, 0u, kPcm, 0u, 0u}) == 0x8042001Au);
    CHECK(s.call(0xE1CD9561u, {kCore, 0u, kPcm, 65537u, 0u}) == 0x8042001Au);
    CHECK(s.call(0xE1CD9561u, {kCore, 0u, kPcm, 256u, 256u}) == 0x80420015u);
    CHECK(s.call(0xE1CD9561u, {kCore, 0u, kPcm, 256u, 0xFFFFFFFFu}) == 0u);
    CHECK(s.call(0x99944089u, {kCore, 0u, kPcm, 0x0Fu, 0u}) == 0x80420014u);
    CHECK(s.call(0x99944089u, {kCore, 0u, kPcm, 0xFFFFFFD0u, 0u}) == 0u);
    CHECK(s.call(0x99944089u, {kCore, 0u, kPcm, 0x10u, 0u}) == 0u);
    // setadsr.expected curve modes and rates
    CHECK(s.call(0x9EC3676Au, {kCore, 0u, 1u, 1u, 0u, 0u, 0u}) == 0x80420013u);  // attack linear decrease
    CHECK(s.call(0x9EC3676Au, {kCore, 0u, 2u, 0u, 0u, 0u, 0u}) == 0x80420013u);  // decay linear increase
    CHECK(s.call(0x9EC3676Au, {kCore, 0u, 0u, 7u, 7u, 7u, 7u}) == 0u);           // nothing updated
    CHECK(s.call(0x019B25EBu, {kCore, 0u, 1u, 0x80000000u, 0u, 0u, 0u}) == 0x80420019u);
    CHECK(s.call(0xCBCD4F79u, {kCore, 0u, 0u, 1u << 13}) == 0x80420013u);
}

void key_on_off() {
    // keyon.expected / keyoff.expected (grain 128, linear 0x1000 rates).
    Sas s;
    s.init(128u);
    CHECK(s.call(0xA0CF2FA4u, {kCore, 0u}) == 0x80420016u);      // key off without key on
    s.call(0xE1CD9561u, {kCore, 0u, kPcm, 256u, 0u});
    s.call(0x9EC3676Au, {kCore, 0u, 15u, 0u, 1u, 1u, 1u});
    s.call(0x019B25EBu, {kCore, 0u, 15u, 0x1000u, 0x1000u, 0x1000u, 0x1000u});
    CHECK(s.call(0x76F01ACAu, {kCore, 0u}) == 0u);
    CHECK(s.call(0x76F01ACAu, {kCore, 0u}) == 0x80420016u);      // twice
    s.core();
    CHECK(s.height() == 0x60000);                                // "Height change: 00000000 -> 00060000"
    for (int i = 0; i < 3; ++i) s.core();
    CHECK(s.height() == 0x1E0000);
    CHECK(s.call(0xA0CF2FA4u, {kCore, 0u}) == 0u);
    s.core();
    CHECK(s.height() == 0x160000);                               // "001e0000 -> 00160000"
    s.call(0x787D04D5u, {kCore, 1u, 1u});                        // pause voice 0
    CHECK(s.call(0x2C8E6AB3u, {kCore}) == 1u);
    CHECK(s.call(0xA0CF2FA4u, {kCore, 0u}) == 0x80420016u);      // while paused
}

void vag_mix() {
    // One VAG block (filter 0, shift 12 -> nibble value * 1) plays at pitch 0x1000.
    Sas s;
    s.init(64u, 32u, 0u);
    auto &m = s.rt.memory();
    for (std::uint32_t b = 0; b < 4u; ++b) {                    // 4 blocks, last flagged end
        m.store8(kPcm + 16u * b, 0x0Cu); m.store8(kPcm + 16u * b + 1u, b == 3u ? 0x01u : 0x00u); // shift 12
        for (std::uint32_t i = 2; i < 16u; ++i) m.store8(kPcm + 16u * b + i, 0x11u);
    }
    s.call(0x99944089u, {kCore, 0u, kPcm, 64u, 0u});
    s.call(0x440CA7D8u, {kCore, 0u, 0x1000u, 0x1000u, 0u, 0u});
    s.call(0x9EC3676Au, {kCore, 0u, 1u, 5u, 1u, 1u, 1u});         // attack direct
    s.call(0x019B25EBu, {kCore, 0u, 1u, 0x40000000u, 0u, 0u, 0u});
    s.call(0x76F01ACAu, {kCore, 0u});
    s.core();
    CHECK(m.load16(kOut) == 0u);                                  // key-on delay: silent
    CHECK(static_cast<std::int16_t>(m.load16(kOut + 4u * 40u)) == 1); // decoded nibble 1 at unity
    for (int i = 0; i < 4; ++i) s.core();
    CHECK((s.call(0x68A46B95u, {kCore}) & 1u) == 1u);             // ended after the end flag
}
} // namespace

int main() {
    parameters();
    key_on_off();
    vag_mix();
    adsr_curves();
    std::cout << "sascore failures=" << failures << "\n";
    return failures == 0 ? 0 : 1;
}
