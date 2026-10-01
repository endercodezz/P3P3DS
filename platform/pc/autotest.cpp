// p3p_autotest: runs one pspautotests program (an unencrypted test PRX) on
// the P3P3DS runtime and compares its console output with the hardware
// `.expected` transcript.
//
// The PRX is loaded and started through the real sceKernelLoadModule /
// sceKernelStartModule HLE path and executed by the interpreter fallback
// (test PRXs have no AOT units). The interpreter shares its instruction
// helpers with the AOT code generator, so a mismatch against hardware output
// points at semantics both execution paths use.
//
//   p3p_autotest <test.prx> [--expected <file>] [--max-dispatches N] [--output <file>]
// Exit: 0 output identical, 1 mismatch or abnormal stop, 2 usage/setup error.
#include "p3p3ds/hle/hle_modules.hpp"
#include "p3p3ds/interpreter.hpp"
#include "p3p3ds/kernel_state.hpp"
#include "p3p3ds/vfs.hpp"
#include "psprecomp/runtime.hpp"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace {
constexpr std::uint32_t kStub = 0x08A00000u;     // fake import stub PC for harness calls
constexpr std::uint32_t kRootResume = 0x08A00100u; // root thread continues here after StartModule
constexpr std::uint32_t kPath = 0x08A00200u, kStatus = 0x08A00300u;
constexpr std::uint32_t kStackTop = 0x09FFFF00u;

std::vector<std::string> lines(const std::string &text) {
    std::vector<std::string> out;
    std::istringstream in(text);
    for (std::string line; std::getline(in, line);) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        out.push_back(line);
    }
    while (!out.empty() && out.back().empty()) out.pop_back();
    return out;
}

std::string g_output;
} // namespace

int main(int argc, char **argv) {
    if (argc < 2) {
        std::cerr << "usage: p3p_autotest <test.prx> [--expected <file>] [--max-dispatches N] [--output <file>]\n";
        return 2;
    }
    const std::filesystem::path prx = argv[1];
    std::filesystem::path expected_path = prx;
    expected_path.replace_extension(".expected");
    std::filesystem::path output_path;
    std::uint64_t budget = 200'000'000u;
    for (int i = 2; i + 1 < argc; i += 2) {
        const std::string a = argv[i];
        if (a == "--expected") expected_path = argv[i + 1];
        else if (a == "--max-dispatches") budget = std::stoull(argv[i + 1]);
        else if (a == "--output") output_path = argv[i + 1];
        else { std::cerr << "unknown option " << a << "\n"; return 2; }
    }
    if (!std::filesystem::exists(prx)) { std::cerr << "missing " << prx.string() << "\n"; return 2; }

    psprecomp::Runtime rt;
    p3p3ds::KernelState kernel;
    // A test program owns the whole user partition ([INFERRED] 0x08800000-
    // 0x0A000000, 24 MiB; the runner's default range starts above P3P's
    // EBOOT). The top 1 MiB stays for ThreadMan stacks, which are carved
    // downward from 0x09FF0000 outside SysMem. pspautotests newlib asks for
    // a 0x1500000-byte heap at startup.
    kernel.sysmem() = p3p3ds::hle::SysMemManager(0x08800000u, 0x09F00000u);
    kernel.io().mount("host0:", std::make_shared<p3p3ds::vfs::HostFileSystem>(prx.parent_path()));
    kernel.io().console_sink = [](std::uint32_t, std::string_view text) { g_output.append(text); };
    kernel.threads().init_root_thread("root", kRootResume, kStackTop, 0u);
    kernel.threads().set_import_stubs({kStub});
    p3p3ds::hle::register_all_hle_modules(rt, kernel);
    // Program exit and harness completion stop the run.
    for (const auto nid : {0x05572A5Fu, 0x2AC9954Bu}) // sceKernelExitGame, sceKernelExitGameWithStatus
        rt.register_hle("LoadExecForUser", nid, [](psprecomp::Runtime &r, psprecomp::AllegrexContext &) { r.stop("Program exited"); });
    // pspautotests harness devices (references/pspautotests/common/common.c):
    // "kemulator:" IS_EMULATOR (3) succeeds, GET_HAS_DISPLAY (1) reports 0,
    // "emulator:" SEND_OUTPUT (2) carries the test transcript. These are
    // emulator conventions of the test suite, not PSP firmware devices.
    rt.register_hle("IoFileMgrForUser", 0x54F5FB11u, [](psprecomp::Runtime &r, psprecomp::AllegrexContext &c) { // sceIoDevctl
        auto &m = r.memory();
        const std::string device = c.gpr[4] ? m.read_c_string(c.gpr[4], 64u) : std::string{};
        const auto cmd = c.gpr[5], in = c.gpr[6], in_len = c.gpr[7], out = c.gpr[8], out_len = c.gpr[9];
        if (device != "emulator:" && device != "kemulator:") { r.stop("Harness: unsupported sceIoDevctl device " + device); return; }
        if (cmd == 2u && in != 0u && m.contains(in, in_len))
            for (std::uint32_t i = 0; i < in_len; ++i) g_output.push_back(static_cast<char>(m.load8(in + i)));
        if (cmd == 1u && out != 0u && out_len >= 4u && m.contains(out, 4u)) m.store32(out, 0u);
        c.set_gpr(2, 0u);
    });
    if (std::getenv("P3P_AUTOTEST_SYSMEM")) // temporary diagnosis: log partition allocations
        rt.register_hle("SysMemUserForUser", 0x237DBD4Fu, [&kernel](psprecomp::Runtime &r, psprecomp::AllegrexContext &c) {
            const std::string name = c.gpr[5] ? r.memory().read_c_string(c.gpr[5], 64u) : std::string{};
            const auto uid = kernel.sysmem().alloc_partition_memory(c.gpr[4], name, c.gpr[6], c.gpr[7], c.gpr[8]);
            std::cout << "alloc part=" << c.gpr[4] << " name=" << name << " type=" << c.gpr[6] << " size=0x" << std::hex << c.gpr[7]
                      << " -> 0x" << static_cast<std::uint32_t>(uid) << " max_free=0x" << kernel.sysmem().max_free_memory() << std::dec << "\n";
            c.set_gpr(2, static_cast<std::uint32_t>(uid));
        });
    rt.register_function(kRootResume, [](psprecomp::Runtime &r, psprecomp::AllegrexContext &) { r.stop("module_start returned to the harness"); },
                         "autotest_root_resume");
    p3p3ds::Interpreter interpreter;
    p3p3ds::install_interpreter_fallback(&interpreter);
    // Diagnostics keep the faulting PC / last transfer; events are capped.
    rt.frontier_diagnostics = true;
    rt.event_budget = 1'000'000u;
    for (const char *type : {"guest_enter", "guest_transfer", "hle_hit", "interpreter_enter", "thread_wait", "thread_wake"})
        rt.event_type_limits[type] = 1000u;

    // sceKernelLoadModule("host0:/<file>") then sceKernelStartModule(uid).
    const std::string path = "host0:/" + prx.filename().string();
    for (std::uint32_t i = 0; i <= path.size(); ++i) rt.memory().store8(kPath + i, i < path.size() ? static_cast<std::uint8_t>(path[i]) : 0u);
    auto &ctx = rt.cpu();
    auto call = [&](const char *lib, std::uint32_t nid, std::initializer_list<std::uint32_t> args) {
        std::uint32_t reg = 4u;
        for (auto v : args) ctx.gpr[reg++] = v;
        ctx.gpr[31] = kRootResume;
        ctx.pc = kStub;
        rt.invoke_import(lib, nid, ctx);
        if (ctx.pc == kStub) ctx.pc = kRootResume;
        return ctx.gpr[2];
    };
    const auto uid = call("ModuleMgrForUser", 0x977DE386u, {kPath, 0u, 0u});
    if (static_cast<std::int32_t>(uid) < 0) { std::cerr << "load failed 0x" << std::hex << uid << "\n"; return 2; }
    call("ModuleMgrForUser", 0x50F0C1ECu, {uid, 0u, 0u, kStatus, 0u});
    try { rt.run(ctx.pc, budget); } catch (const psprecomp::FrontierHalt &) {}
    catch (const std::exception &e) { rt.stop(std::string("Runtime exception: ") + e.what()); }
    if (!rt.stopped()) rt.stop("Dispatch budget exhausted");

    if (!output_path.empty()) std::ofstream(output_path, std::ios::binary) << g_output;
    std::ifstream expected_file(expected_path, std::ios::binary);
    const std::string expected((std::istreambuf_iterator<char>(expected_file)), std::istreambuf_iterator<char>());
    const auto got = lines(g_output), want = lines(expected);
    std::size_t mismatches = 0;
    for (std::size_t i = 0; i < std::max(got.size(), want.size()); ++i) {
        const auto &g = i < got.size() ? got[i] : std::string("<missing>");
        const auto &w = i < want.size() ? want[i] : std::string("<missing>");
        if (g != w && ++mismatches <= 12) std::cout << "line " << i + 1 << "\n  got:  " << g << "\n  want: " << w << "\n";
    }
    if (std::getenv("PSPRECOMP_HLE_HISTOGRAM")) // per-NID HLE call counts
        for (const auto &[key, count] : rt.hle_histogram()) std::cout << "  hle " << key << " x" << count << "\n";
    const bool exited = rt.stop_reason() == "Program exited" || rt.stop_reason() == "module_start returned to the harness";
    std::cout << "final pc=0x" << std::hex << rt.cpu().pc << " diag_pc=0x" << rt.diagnostic_pc << " last transfer 0x"
              << rt.last_transfer.pc << "->0x" << rt.last_transfer.target << " ra=0x" << rt.cpu().gpr[31] << std::dec << "\n";
    std::cout << prx.filename().string() << ": stop=\"" << rt.stop_reason() << "\" lines=" << want.size()
              << " mismatches=" << mismatches << " interpreted=" << interpreter.executed() << "\n";
    return mismatches == 0 && exited && !want.empty() ? 0 : 1;
}
