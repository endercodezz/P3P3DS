// sceUtilitySavedata (core/src/hle/savedata.cpp): the hardware status sequence
// (1, 2 -> Update -> 3 -> Shutdown -> 4 -> 0, pspautotests utility/savedata/*.expected),
// a LISTSAVE / LISTLOAD round trip through the list dialog with the parameter
// block P3P uses (gameName ULUS10512, saveNameList DATA00.., P3PSAVE.BIN),
// cancel (result 1), "no data" (0x80110307) and SIZES.
#include "p3p3ds/hle/hle_modules.hpp"
#include "p3p3ds/kernel_state.hpp"
#include "psprecomp/runtime.hpp"

#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

namespace {
int failures = 0;
#define CHECK(x) do { if (!(x)) { std::cerr << __LINE__ << ": " << #x << "\n"; ++failures; } } while (0)

constexpr std::uint32_t kParam = 0x08900000u, kList = 0x08901000u, kData = 0x08910000u, kFree = 0x08902000u, kNeed = 0x08902100u;
constexpr std::uint32_t kDataSize = 0x159B4u; // P3P's P3PSAVE.BIN buffer

struct Env {
    psprecomp::Runtime rt;
    p3p3ds::KernelState k;
    Env() {
        p3p3ds::hle::register_all_hle_modules(rt, k);
        k.threads().init_root_thread("root", 0x08804000u, 0x09FFFF00u, 0u);
    }
    std::uint32_t call(std::uint32_t nid, std::initializer_list<std::uint32_t> args = {}) {
        auto &ctx = rt.cpu();
        std::uint32_t reg = 4u;
        for (auto v : args) ctx.gpr[reg++] = v;
        ctx.gpr[31] = 0x08A00100u;
        ctx.pc = 0x08A00000u;
        rt.invoke_import("sceUtility", nid, ctx);
        return ctx.gpr[2];
    }
};

void put_string(psprecomp::GuestMemory &m, std::uint32_t at, const std::string &s, std::uint32_t n) {
    for (std::uint32_t i = 0; i < n; ++i) m.store8(at + i, i < s.size() ? static_cast<std::uint8_t>(s[i]) : 0u);
}
std::string get_string(psprecomp::GuestMemory &m, std::uint32_t at, std::uint32_t n) {
    std::string s;
    for (std::uint32_t i = 0; i < n && m.load8(at + i); ++i) s += static_cast<char>(m.load8(at + i));
    return s;
}

void make_param(psprecomp::GuestMemory &m, std::uint32_t mode) {
    m.zero(kParam, 0x600);
    m.store32(kParam, 0x600);
    m.store32(kParam + 0x30, mode);
    put_string(m, kParam + 0x3C, "ULUS10512", 13);
    put_string(m, kParam + 0x4C, "DATA00", 20);
    m.store32(kParam + 0x60, kList);
    for (int i = 0; i < 3; ++i) put_string(m, kList + static_cast<std::uint32_t>(i) * 20u, "DATA0" + std::to_string(i), 20);
    put_string(m, kList + 3u * 20u, "", 20);
    put_string(m, kParam + 0x64, "P3PSAVE.BIN", 13);
    m.store32(kParam + 0x74, kData);
    m.store32(kParam + 0x78, kDataSize);
    m.store32(kParam + 0x7C, kDataSize);
    put_string(m, kParam + 0x80, "Persona 3 Portable", 0x80);
    put_string(m, kParam + 0x100, "Test save", 0x80);
    put_string(m, kParam + 0x180, "Dorm, 4/7", 0x400);
}

// Runs one utility session the way games do; returns the result word.
std::uint32_t run(Env &e, std::uint32_t mode, int choice) {
    make_param(e.rt.memory(), mode);
    CHECK(e.call(0x50C4CD57u, {kParam}) == 0u);                 // InitStart
    CHECK(e.call(0x8874DBE0u) == 1u);                            // INIT
    CHECK(e.call(0x8874DBE0u) == 2u);                            // VISIBLE
    for (int i = 0; i < 10 && e.call(0x8874DBE0u) == 2u; ++i) {
        CHECK(e.call(0xD4B95FFBu, {1u}) == 0u);                  // Update
        if (const auto *dialog = e.k.savedata().pending_dialog()) {
            CHECK(dialog->slots.size() == 3u);
            e.k.savedata().resolve(choice);
        }
    }
    CHECK(e.call(0x8874DBE0u) == 3u);                            // QUIT
    CHECK(e.call(0x9790B33Cu) == 0u);                            // ShutdownStart
    CHECK(e.call(0x8874DBE0u) == 4u);                            // FINISHED
    CHECK(e.call(0x8874DBE0u) == 0u);                            // NONE
    return e.rt.memory().load32(kParam + 0x1C);
}
} // namespace

int main() {
    Env env;
    auto &m = env.rt.memory();
    const auto root = std::filesystem::temp_directory_path() / "p3p3ds_test_savedata";
    std::filesystem::remove_all(root);
    env.k.savedata().root = root;

    // Nothing saved yet: LISTLOAD reports "no data" without asking.
    CHECK(run(env, 4, 0) == 0x80110307u);

    // LISTSAVE into the second slot.
    for (std::uint32_t i = 0; i < kDataSize; ++i) m.store8(kData + i, static_cast<std::uint8_t>(i * 7u + 3u));
    CHECK(run(env, 5, 1) == 0u);
    CHECK(get_string(m, kParam + 0x4C, 20) == "DATA01");        // the chosen slot is written back
    CHECK(std::filesystem::file_size(root / "ULUS10512DATA01" / "P3PSAVE.BIN") == kDataSize);
    CHECK(std::filesystem::exists(root / "ULUS10512DATA01" / "PARAM.SFO"));

    // LISTLOAD: the dialog lists it with its PARAM.SFO texts; loading restores the bytes.
    make_param(m, 4);
    m.zero(kData, kDataSize);
    CHECK(env.call(0x50C4CD57u, {kParam}) == 0u);
    env.call(0x8874DBE0u);
    env.call(0x8874DBE0u);
    CHECK(env.call(0xD4B95FFBu, {1u}) == 0u);
    const auto *dialog = env.k.savedata().pending_dialog();
    CHECK(dialog != nullptr);
    if (dialog) {
        CHECK(!dialog->slots[0].exists && dialog->slots[1].exists && !dialog->slots[2].exists);
        CHECK(dialog->slots[1].savedata_title == "Test save" && dialog->slots[1].detail == "Dorm, 4/7");
        env.k.savedata().resolve(1);
    }
    CHECK(env.call(0xD4B95FFBu, {1u}) == 0u);
    CHECK(env.call(0x8874DBE0u) == 3u);
    CHECK(m.load32(kParam + 0x1C) == 0u);
    CHECK(m.load32(kParam + 0x7C) == kDataSize);
    bool same = true;
    for (std::uint32_t i = 0; i < kDataSize; ++i) same &= m.load8(kData + i) == static_cast<std::uint8_t>(i * 7u + 3u);
    CHECK(same);
    CHECK(env.call(0x9790B33Cu) == 0u);
    env.call(0x8874DBE0u);
    env.call(0x8874DBE0u);

    // Cancel: result 1, nothing loaded.
    CHECK(run(env, 4, -1) == 1u);

    // SIZES: free space and the size the save would need (sizes.expected field layout).
    make_param(m, 8);
    m.store32(kParam + 0x5D0, kFree);
    m.store32(kParam + 0x5D8, kNeed);
    CHECK(env.call(0x50C4CD57u, {kParam}) == 0u);
    env.call(0x8874DBE0u);
    env.call(0x8874DBE0u);
    CHECK(env.call(0xD4B95FFBu, {1u}) == 0u);
    CHECK(m.load32(kParam + 0x1C) == 0u);
    CHECK(m.load32(kFree) == 0x8000u && m.load32(kFree + 4) > 0u);
    CHECK(m.load32(kNeed) == (kDataSize + 0x1000u + 0x7FFFu) / 0x8000u);
    CHECK(env.call(0x9790B33Cu) == 0u);
    env.call(0x8874DBE0u);
    env.call(0x8874DBE0u);

    // Out of sequence calls.
    CHECK(env.call(0x9790B33Cu) == 0x80110001u);

    std::filesystem::remove_all(root);
    if (failures == 0) std::cout << "test_savedata: all checks passed\n";
    return failures == 0 ? 0 : 1;
}
