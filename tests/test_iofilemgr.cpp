// IoFileMgr + VFS through the registered HLE entry points.
// Contracts: references/uofw/src/kd/iofilemgr/iofilemgr.c (async model),
// references/uofw/include/common/errors.h.
#include "p3p3ds/hle/hle_modules.hpp"
#include "p3p3ds/kernel_state.hpp"
#include "p3p3ds/vfs.hpp"
#include "psprecomp/runtime.hpp"

#include <cctype>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <initializer_list>
#include <iostream>
#include <string>
#include <vector>

using namespace p3p3ds::hle;

namespace {
int failures = 0;
#define CHECK(x) do { if (!(x)) { std::cerr << __LINE__ << ": " << #x << "\n"; ++failures; } } while (0)

constexpr std::uint32_t kStub = 0x08A00000u, kRet = 0x08A00100u;
constexpr std::uint32_t kStr = 0x08900000u, kBuf = 0x08910000u, kRes = 0x08920000u;

// Minimal ISO9660 image: root with PSP_GAME/USRDIR/DATA.BIN (4000 bytes) and README.TXT.
std::vector<std::uint8_t> make_iso() {
    std::vector<std::uint8_t> iso(40 * 2048, 0);
    auto record = [&](std::size_t at, std::uint32_t lba, std::uint32_t size, bool dir, const std::string &name) {
        const auto length = static_cast<std::uint8_t>(33 + name.size() + (name.size() % 2 == 0 ? 1 : 0));
        iso[at] = length;
        for (int i = 0; i < 4; ++i) { iso[at + 2 + i] = static_cast<std::uint8_t>(lba >> (8 * i)); iso[at + 10 + i] = static_cast<std::uint8_t>(size >> (8 * i)); }
        iso[at + 25] = dir ? 2 : 0;
        iso[at + 32] = static_cast<std::uint8_t>(name.size());
        std::memcpy(iso.data() + at + 33, name.data(), name.size());
        return at + length;
    };
    const std::size_t pvd = 16 * 2048;
    iso[pvd] = 1; std::memcpy(iso.data() + pvd + 1, "CD001", 5);
    record(pvd + 156, 20, 2048, true, std::string(1, '\0'));
    auto at = record(20 * 2048, 20, 2048, true, std::string(1, '\0'));
    at = record(at, 20, 2048, true, std::string(1, '\1'));
    at = record(at, 21, 2048, true, "PSP_GAME");
    record(at, 24, 11, false, "README.TXT;1");
    at = record(21 * 2048, 21, 2048, true, std::string(1, '\0'));
    at = record(at, 20, 2048, true, std::string(1, '\1'));
    record(at, 22, 2048, true, "USRDIR");
    at = record(22 * 2048, 22, 2048, true, std::string(1, '\0'));
    at = record(at, 21, 2048, true, std::string(1, '\1'));
    record(at, 25, 4000, false, "DATA.BIN;1");
    std::memcpy(iso.data() + 24 * 2048, "hello world", 11);
    for (std::uint32_t i = 0; i < 4000; ++i) iso[25 * 2048 + i] = static_cast<std::uint8_t>(i * 7u);
    return iso;
}

struct Env {
    psprecomp::Runtime rt;
    p3p3ds::KernelState k;
    Env(const std::filesystem::path &dir) {
        register_all_hle_modules(rt, k);
        k.threads().set_import_stubs({kStub});
        k.threads().init_root_thread("A", 0x08804000u, 0x09FFFF00u, 0u);
        const auto iso = dir / "umd.iso";
        const auto bytes = make_iso();
        std::ofstream(iso, std::ios::binary).write(reinterpret_cast<const char *>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
        k.io().mount("disc0:", std::make_shared<p3p3ds::vfs::IsoFileSystem>(iso));
        std::filesystem::create_directories(dir / "ms0/PSP/P3P");
        std::filesystem::create_directories(dir / "mods/bind");
        std::ofstream(dir / "ms0/PSP/P3P/mod.cpk") << "ms0-copy";
        std::ofstream(dir / "mods/mod.cpk") << "mods-copy";
        k.io().mount("ms0:", std::make_shared<p3p3ds::vfs::HostFileSystem>(dir / "ms0"), true);
        k.io().alias("ms0:/PSP/P3P", std::make_shared<p3p3ds::vfs::HostFileSystem>(dir / "mods", true)); // as the runners
    }
    std::uint32_t call(std::uint32_t nid, std::initializer_list<std::uint32_t> args) {
        auto &ctx = rt.cpu();
        std::uint32_t reg = 4u;
        for (auto v : args) ctx.gpr[reg++] = v;
        ctx.gpr[31] = kRet;
        ctx.pc = kStub;
        const auto token = psprecomp::capture_runtime_execution_context();
        rt.invoke_import("IoFileMgrForUser", nid, ctx);
        if (!rt.stopped() && psprecomp::runtime_execution_context_matches(token) && ctx.pc == kStub) ctx.pc = kRet;
        return ctx.gpr[2];
    }
    // Retry waits resume at the stub: run the call again until it returns.
    std::uint32_t call_until_done(std::uint32_t nid, std::initializer_list<std::uint32_t> args) {
        auto r = call(nid, args);
        while (rt.cpu().pc == kStub && !rt.stopped()) r = call(nid, args);
        return r;
    }
    std::uint32_t str(const std::string &s) {
        for (std::size_t i = 0; i <= s.size(); ++i) rt.memory().store8(kStr + static_cast<std::uint32_t>(i), i < s.size() ? static_cast<std::uint8_t>(s[i]) : 0u);
        return kStr;
    }
    std::uint64_t res() { return rt.memory().load32(kRes) | (static_cast<std::uint64_t>(rt.memory().load32(kRes + 4u)) << 32u); }
};
} // namespace

int main() {
    const auto dir = std::filesystem::path(".tmp") / "test_iofilemgr";
    std::filesystem::remove_all(dir);
    std::filesystem::create_directories(dir);
    Env env(dir);
    auto &m = env.rt.memory();

    // VFS normalization rejects escaping the root.
    CHECK(!p3p3ds::vfs::normalize("../x"));
    CHECK(p3p3ds::vfs::normalize("/a//b/./c/../d") == std::string("a/b/d"));

    // Sync open/read/lseek/close (case-insensitive UMD paths).
    const auto t0 = env.k.threads().now();
    const auto fd = env.call(0x109F50BCu, {env.str("disc0:/psp_game/usrdir/data.bin"), 1u, 0u});
    CHECK(fd == 3u);
    CHECK(env.k.threads().now() >= t0 + IoManager::kCommandLatencyUs); // sync calls hold the caller
    CHECK(env.call(0x6A638D83u, {fd, kBuf, 16u}) == 16u);
    CHECK(m.load8(kBuf + 3u) == 21u);
    // 64-bit offset in $a2:$a3, whence in $t0; result in $v0:$v1.
    CHECK(env.call(0x27EB27B8u, {fd, 0u, 0xFFFFFFF0u, 0xFFFFFFFFu, 2u}) == 3984u && env.rt.cpu().gpr[3] == 0u);
    CHECK(env.call(0x6A638D83u, {fd, kBuf, 64u}) == 16u);              // short read at EOF
    CHECK(env.call(0x27EB27B8u, {fd, 0u, 0u, 0u, 3u}) == SCE_ERROR_KERNEL_INVALID_ARGUMENT_IO);
    CHECK(env.call(0x810C4BC3u, {fd}) == 0u);
    CHECK(env.call(0x810C4BC3u, {fd}) == SCE_ERROR_KERNEL_BAD_FILE_DESCRIPTOR);

    // Errors.
    CHECK(env.call(0x109F50BCu, {env.str("disc0:/nope.bin"), 1u, 0u}) == SCE_ERROR_ERRNO_FILE_NOT_FOUND);
    CHECK(env.call(0x109F50BCu, {env.str("flash9:/x"), 1u, 0u}) == SCE_ERROR_KERNEL_NO_SUCH_DEVICE);
    CHECK(env.call(0x109F50BCu, {env.str("disc0:/README.TXT"), 0x0602u, 0u}) == SCE_ERROR_ERRNO_READ_ONLY);
    CHECK(env.call(0x109F50BCu, {env.str("disc0:/../README.TXT"), 1u, 0u}) == SCE_ERROR_KERNEL_NO_SUCH_DEVICE);

    // Async open/read/poll/wait (uOFW do_get_async_stat).
    const auto afd = env.call(0x89AA9906u, {env.str("disc0:/PSP_GAME/USRDIR/DATA.BIN"), 1u, 0u});
    CHECK(afd == 3u);
    CHECK(env.call(0x3251EA56u, {afd, kRes}) == 1u);                    // pending
    CHECK(env.call(0xA0B5A7C2u, {afd, kBuf, 100u}) == SCE_ERROR_KERNEL_ASYNC_BUSY);
    CHECK(env.call_until_done(0xE23EEC33u, {afd, kRes}) == 0u && env.res() == afd);
    CHECK(env.call(0xE23EEC33u, {afd, kRes}) == SCE_ERROR_KERNEL_NO_ASYNC_OP);
    CHECK(env.call(0xA0B5A7C2u, {afd, kBuf, 1000u}) == 0u);
    CHECK(env.call_until_done(0x35DBD746u, {afd, kRes}) == 0u && env.res() == 1000u);
    CHECK(env.call(0x71B19E77u, {afd, 0u, 5u, 0u, 0u}) == 0u);
    CHECK(env.call_until_done(0xE23EEC33u, {afd, kRes}) == 0u && env.res() == 5u);
    CHECK(env.call(0xFF5940B6u, {afd}) == 0u);
    CHECK(env.call_until_done(0xE23EEC33u, {afd, kRes}) == 0u);
    CHECK(env.k.io().get(static_cast<std::int32_t>(afd)) == nullptr);  // freed after the wait

    // Getstat and directories.
    CHECK(env.call(0xACE946E8u, {env.str("disc0:/PSP_GAME/USRDIR/DATA.BIN"), kBuf}) == 0u);
    CHECK(m.load32(kBuf) == 0x216Du && m.load32(kBuf + 8u) == 4000u);
    CHECK(env.call(0xACE946E8u, {env.str("disc0:/PSP_GAME"), kBuf}) == 0u && m.load32(kBuf) == 0x116Du);
    const auto dfd = env.call(0xB29DDF9Cu, {env.str("disc0:/PSP_GAME/USRDIR")});
    std::vector<std::string> names;
    while (env.call(0xE3EB004Cu, {dfd, kBuf}) == 1u) names.push_back(m.read_c_string(kBuf + 88u));
    CHECK((names == std::vector<std::string>{".", "..", "DATA.BIN"}));
    CHECK(env.call(0xEB092469u, {dfd}) == 0u);
    CHECK(env.call(0xB29DDF9Cu, {env.str("disc0:/README.TXT")}) == SCE_ERROR_ERRNO_NOT_A_DIRECTORY);

    // Mod alias: ms0:/PSP/P3P resolves to the mods directory, not ms0 itself.
    const auto mfd = env.call(0x109F50BCu, {env.str("ms0:/PSP/P3P/MOD.CPK"), 1u, 0u});
    m.store8(kBuf + 9u, 0u);
    CHECK(env.call(0x6A638D83u, {mfd, kBuf, 9u}) == 9u && m.read_c_string(kBuf, 16u) == "mods-copy");
    CHECK(env.call(0x109F50BCu, {env.str("ms0:/PSP/P3P/mod1.cpk"), 1u, 0u}) == SCE_ERROR_ERRNO_FILE_NOT_FOUND);
    // Mod Support patch lookup of a missing bind file.
    CHECK(env.call(0xACE946E8u, {env.str("ms0:/PSP/P3P/bind/data/sound/voice/v450001.afs"), kBuf}) == SCE_ERROR_ERRNO_FILE_NOT_FOUND);

    // Cached host lookups (fixed_contents) answer like uncached ones.
    {
        const auto tree = dir / "tree";
        std::filesystem::create_directories(tree / "Bind/Data/Sound");
        std::ofstream(tree / "Bind/Data/Sound/V450001.AFS") << "voice!";
        const p3p3ds::vfs::HostFileSystem plain(tree), cached(tree, true);
        for (const char *path : {"", "bind", "BIND/data", "bind/data/sound/v450001.afs", "./bind//data/sound/V450001.afs",
                                 "bind/data/sound/missing.afs", "bind/missing/x", "bind/data/sound/v450001.afs/x", "../tree"}) {
            const auto a = plain.stat(path), b = cached.stat(path);
            CHECK(a.has_value() == b.has_value());
            // Names compare case-insensitively: on Windows the uncached lookup keeps the
            // requested spelling (the host filesystem matches any case), the cached one
            // returns the name on disk.
            auto same_name = [](std::string x, std::string y) {
                for (auto *s : {&x, &y}) for (auto &c : *s) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
                return x == y;
            };
            if (a && b) CHECK(same_name(a->name, b->name) && a->directory == b->directory && a->size == b->size);
            CHECK((plain.open(path) != nullptr) == (cached.open(path) != nullptr));
        }
        const auto voice = cached.stat("bind/data/sound/v450001.afs");
        CHECK(voice && voice->name == "V450001.AFS" && !voice->directory && voice->size == 6u);
        const auto src = cached.open("BIND/DATA/SOUND/v450001.afs");
        char text[7] = {};
        CHECK(src && src->read(0, text, 6) == 6u && std::string(text) == "voice!");
        const auto listed = cached.list("bind/data/sound");
        CHECK(listed && listed->size() == 1u && (*listed)[0].name == "V450001.AFS");
        CHECK(!p3p3ds::vfs::HostFileSystem(dir / "no_such_root", true).stat("x"));
        // A file added after its directory was listed stays invisible (contents are fixed by contract).
        std::ofstream(tree / "Bind/Data/Sound/late.afs") << "x";
        CHECK(!cached.stat("bind/data/sound/late.afs") && plain.stat("bind/data/sound/late.afs"));
    }

    CHECK(!env.rt.stopped());
    std::cout << "iofilemgr failures=" << failures << "\n";
    return failures == 0 ? 0 : 1;
}
