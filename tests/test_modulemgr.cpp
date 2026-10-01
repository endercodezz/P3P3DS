// ModuleMgr against the real UMD image (skips with 77 when absent).
// Contract: references/uofw/src/kd/modulemgr/modulemgr.c (_StartModule).
#include "p3p3ds/hle/hle_modules.hpp"
#include "p3p3ds/kernel_state.hpp"
#include "p3p3ds/vfs.hpp"
#include "psprecomp/elf32.hpp"
#include "psprecomp/runtime.hpp"

#include <filesystem>
#include <iostream>

using namespace p3p3ds::hle;
static int failures = 0;
#define CHECK(x) do { if (!(x)) { std::cerr << __LINE__ << ": " << #x << "\n"; ++failures; } } while (0)

int main() {
    std::filesystem::path iso;
    for (const auto &entry : std::filesystem::directory_iterator("."))
        if (entry.path().extension() == ".iso") iso = entry.path();
    const std::filesystem::path elf_path = "profiles/p3p/game/eboot.elf";
    if (iso.empty() || !std::filesystem::exists(elf_path)) { std::cout << "SKIP: game data absent\n"; return 77; }

    psprecomp::Runtime rt;
    p3p3ds::KernelState k;
    const auto elf = psprecomp::Elf32Image::from_file(elf_path);
    (void)elf.load_and_relocate(rt.memory());
    const auto module = elf.find_module_info(rt.memory());
    std::vector<std::tuple<std::string, std::uint32_t, std::uint32_t>> imports;
    std::uint32_t preacc_stub = 0;
    for (const auto &i : elf.scan_imports(rt.memory(), *module)) {
        imports.emplace_back(i.library, i.nid, i.stub_address);
        if (i.library == "scesupPreAcc" && preacc_stub == 0) preacc_stub = i.stub_address;
    }
    k.modules().set_host_imports(imports);
    register_all_hle_modules(rt, k);
    k.threads().init_root_thread("A", 0x08804000u, 0x09FFFF00u, module->gp);
    k.io().mount("disc0:", std::make_shared<p3p3ds::vfs::IsoFileSystem>(iso));
    auto &ctx = rt.cpu();
    auto call = [&](std::uint32_t nid, std::uint32_t a0, std::uint32_t a1 = 0, std::uint32_t a2 = 0, std::uint32_t a3 = 0) {
        ctx.gpr[4] = a0; ctx.gpr[5] = a1; ctx.gpr[6] = a2; ctx.gpr[7] = a3; ctx.gpr[31] = 0x08A00100u; ctx.pc = 0x08A00000u;
        rt.invoke_import("ModuleMgrForUser", nid, ctx);
        return ctx.gpr[2];
    };
    auto str = [&](const char *s) { for (std::uint32_t i = 0;; ++i) { rt.memory().store8(0x08900000u + i, static_cast<std::uint8_t>(s[i])); if (!s[i]) break; } return 0x08900000u; };

    CHECK(call(0x977DE386u, str("disc0:/PSP_GAME/USRDIR/module/none.prx")) == 0x80010002u);
    const auto uid = call(0x977DE386u, str("disc0:/PSP_GAME/USRDIR/module/libsuppreacc.prx"));
    CHECK(static_cast<std::int32_t>(uid) > 0);
    const auto &m = k.modules().modules.at(static_cast<std::int32_t>(uid));
    CHECK(m.name == "scesupPreAcc_library" && !m.hle_only && m.exports.size() == 8u && m.module_start == 0u);
    // The EBOOT's scesupPreAcc stub now jumps into the loaded module's code.
    CHECK(preacc_stub != 0u && rt.has_function(preacc_stub));
    ctx.pc = preacc_stub;
    rt.run(preacc_stub, 1u);
    CHECK(ctx.pc >= m.base && ctx.pc < m.base + m.size);
    // Entry-less module: StartModule returns its id with status 0, no thread.
    rt.memory().store32(0x08910000u, 0xDEADBEEFu);
    CHECK(call(0x50F0C1ECu, uid, 0u, 0u, 0x08910000u) == uid);
    CHECK(rt.memory().load32(0x08910000u) == 0u);
    CHECK(call(0x50F0C1ECu, uid) == 0x80020001u);           // already started
    // Encrypted Sony library accepted as an HLE module.
    const auto font = call(0x977DE386u, str("disc0:/PSP_GAME/USRDIR/module/libfont.prx"));
    CHECK(static_cast<std::int32_t>(font) > 0 && k.modules().modules.at(static_cast<std::int32_t>(font)).hle_only);
    CHECK(call(0x50F0C1ECu, font) == font);
    CHECK(call(0x50F0C1ECu, 0x7777u) == 0x8002012Eu);
    std::cout << "modulemgr failures=" << failures << "\n";
    return failures == 0 ? 0 : 1;
}
