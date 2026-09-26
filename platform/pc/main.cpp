#include "psprecomp/common.hpp"
#include "psprecomp/elf32.hpp"
#include "psprecomp/runtime.hpp"

#include "p3p3ds/kernel_state.hpp"
#include "p3p3ds/hle/hle_modules.hpp"

#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <string>
#include <string_view>

namespace psprecomp {
// Defined in the generated AOT translation unit
void register_generated_functions(Runtime &runtime);
}

namespace {

constexpr std::uint32_t kExpectedEntryPc = 0x08804108u;
constexpr std::uint32_t kExpectedStopPc  = 0x08B4E6A0u;
constexpr std::uint32_t kExpectedReturnRa = 0x08804268u;
constexpr std::string_view kExpectedStopReason = "No recompiled function registered at 0x08B4E6A0";
constexpr std::uint32_t kExpectedSdkVersion = 0x06020010u;
constexpr std::uint32_t kExpectedCompilerVersion = 0x00030306u;
constexpr std::int32_t kExpectedThreadUid = 2;
constexpr std::string_view kExpectedThreadName = "user_main";

void print_registers(const psprecomp::AllegrexContext &ctx) {
    static const char *const kGprNames[32] = {
        "zero", "at", "v0", "v1", "a0", "a1", "a2", "a3",
        "t0",   "t1", "t2", "t3", "t4", "t5", "t6", "t7",
        "s0",   "s1", "s2", "s3", "s4", "s5", "s6", "s7",
        "t8",   "t9", "k0", "k1", "gp", "sp", "fp", "ra"
    };

    std::cout << "\n=== Guest Register State ===\n";
    std::cout << "  PC: " << psprecomp::hex32(ctx.pc) << "\n";
    for (std::size_t i = 0; i < 32; ++i) {
        if (i % 4 == 0) std::cout << "  ";
        std::cout << std::left << std::setw(4) << kGprNames[i] << " = "
                  << psprecomp::hex32(ctx.gpr[i]) << "  ";
        if (i % 4 == 3) std::cout << "\n";
    }
}

struct MilestoneVerificationResult {
    bool passed{false};
    bool stopped{false};
    bool entry_matched{false};
    bool pc_matched{false};
    bool ra_matched{false};
    bool thread_uid_matched{false};
    bool thread_name_matched{false};
    bool sdk_version_matched{false};
    bool compiler_version_matched{false};
    bool stop_reason_matched{false};
    std::string failure_detail;
};

MilestoneVerificationResult verify_milestone(
    std::uint32_t entry_addr,
    const psprecomp::Runtime &runtime,
    const p3p3ds::KernelState &kernel)
{
    MilestoneVerificationResult res;
    res.stopped = runtime.stopped();
    res.entry_matched = (entry_addr == kExpectedEntryPc);
    res.pc_matched = (runtime.cpu().pc == kExpectedStopPc);
    res.ra_matched = (runtime.cpu().gpr[31] == kExpectedReturnRa);
    res.thread_uid_matched = (kernel.threads().current_thread_id() == kExpectedThreadUid);
    res.thread_name_matched = (kernel.threads().current_thread() != nullptr &&
                               kernel.threads().current_thread()->name == kExpectedThreadName);
    res.sdk_version_matched = (kernel.compiled_sdk_version() == kExpectedSdkVersion);
    res.compiler_version_matched = (kernel.compiler_version() == kExpectedCompilerVersion);

    const std::string &reason = runtime.stop_reason();
    res.stop_reason_matched = (reason.find(kExpectedStopReason) != std::string::npos);

    if (!res.stopped) {
        res.failure_detail = "Runtime did not stop (dispatch budget exhausted without hitting HLE/stop)";
    } else if (!res.entry_matched) {
        res.failure_detail = "Entry PC mismatch: expected " + psprecomp::hex32(kExpectedEntryPc) +
                             ", got " + psprecomp::hex32(entry_addr);
    } else if (!res.sdk_version_matched) {
        res.failure_detail = "Kernel SDK version mismatch: expected " + psprecomp::hex32(kExpectedSdkVersion) +
                             ", got " + psprecomp::hex32(kernel.compiled_sdk_version());
    } else if (!res.compiler_version_matched) {
        res.failure_detail = "Kernel compiler version mismatch: expected " + psprecomp::hex32(kExpectedCompilerVersion) +
                             ", got " + psprecomp::hex32(kernel.compiler_version());
    } else if (!res.pc_matched) {
        res.failure_detail = "Stop PC mismatch: expected " + psprecomp::hex32(kExpectedStopPc) +
                             ", got " + psprecomp::hex32(runtime.cpu().pc);
    } else if (!res.ra_matched) {
        res.failure_detail = "Return address ($ra) mismatch: expected " + psprecomp::hex32(kExpectedReturnRa) +
                             ", got " + psprecomp::hex32(runtime.cpu().gpr[31]);
    } else if (!res.thread_uid_matched) {
        res.failure_detail = "Current thread UID mismatch: expected " + std::to_string(kExpectedThreadUid) +
                             ", got " + std::to_string(kernel.threads().current_thread_id());
    } else if (!res.thread_name_matched) {
        res.failure_detail = "Current thread name mismatch: expected " + std::string(kExpectedThreadName) +
                             ", got " + (kernel.threads().current_thread() ? kernel.threads().current_thread()->name : "null");
    } else if (!res.stop_reason_matched) {
        res.failure_detail = "Stop reason mismatch: expected \"" + std::string(kExpectedStopReason) +
                             "\", got: \"" + reason + "\"";
    } else {
        res.passed = true;
    }
    return res;
}

} // namespace

int main(int argc, char **argv) {
    std::filesystem::path elf_path = "profiles/p3p/game/eboot.elf";
    std::uint64_t max_dispatches = 1000u;
    bool verbose = false;
    bool verify_mode = false;

    for (int i = 1; i < argc; ++i) {
        std::string_view arg(argv[i]);
        if (arg == "--elf" && i + 1 < argc) {
            elf_path = argv[++i];
        } else if (arg == "--max-dispatches" && i + 1 < argc) {
            max_dispatches = std::stoull(argv[++i]);
        } else if (arg == "--verbose" || arg == "-v") {
            verbose = true;
        } else if (arg == "--verify-milestone" || arg == "--verify") {
            verify_mode = true;
        } else if (arg == "--help" || arg == "-h") {
            std::cout << "Usage: p3p_pc_bootstrap [options]\n"
                      << "  --elf <path>            Path to decrypted P3P ELF (default: profiles/p3p/game/eboot.elf)\n"
                      << "  --max-dispatches <N>    Maximum dispatch count (default: 1000)\n"
                      << "  --verify-milestone      Strict milestone verification (returns 0 only on exact milestone match)\n"
                      << "  --verbose, -v           Enable verbose runtime traces\n";
            return 0;
        }
    }

    std::cout << "====================================================\n";
    std::cout << "   P3P3DS PC Bootstrap Execution Harness\n";
    std::cout << "====================================================\n";
    std::cout << "Target ELF:       " << elf_path.string() << "\n";
    std::cout << "Dispatch budget:  " << max_dispatches << "\n";
    std::cout << "Verify mode:      " << (verify_mode ? "STRICT (--verify-milestone)" : "standard") << "\n";

    try {
        // 1. Load decrypted P3P ELF
        if (!std::filesystem::exists(elf_path)) {
            std::cerr << "Error: ELF file not found at " << elf_path.string() << "\n";
            return 1;
        }
        const auto elf = psprecomp::Elf32Image::from_file(elf_path);
        const std::uint32_t load_base = psprecomp::kDefaultPspUserLoadBase;
        const std::uint32_t entry_addr = elf.runtime_entry(load_base);

        std::cout << "ELF Type:         " << elf.type()
                  << (elf.is_psp_prx() ? " (PSP PRX relocatable)" : " (Static ELF)") << "\n";
        std::cout << "Runtime Entry:    " << psprecomp::hex32(entry_addr) << "\n";

        // 2. Initialize PSPRecomp Runtime with 32 MiB PSP RAM
        psprecomp::Runtime runtime(32u * 1024u * 1024u);

        // 3. Load & relocate ELF into GuestMemory
        const auto rel_stats = elf.load_and_relocate(runtime.memory(), load_base);
        std::cout << "Relocations:      " << rel_stats.total << " total (R_26="
                  << rel_stats.r_mips_26 << ", R_32=" << rel_stats.r_mips_32
                  << ", R_HI=" << rel_stats.r_mips_hi16 << ", R_LO=" << rel_stats.r_mips_lo16 << ")\n";

        // 4. Find relocated PSP module info
        const auto module = elf.find_module_info(runtime.memory(), load_base);
        if (!module) {
            std::cerr << "Error: Failed to find relocated PSP module info in ELF\n";
            return 2;
        }
        std::cout << "Module Info:      " << module->name << " v"
                  << static_cast<int>(module->major_version) << "."
                  << static_cast<int>(module->minor_version) << "\n";
        std::cout << "Module GP:        " << psprecomp::hex32(module->gp) << "\n";
        std::cout << "Module Stubs:     " << psprecomp::hex32(module->stub_top)
                  << " - " << psprecomp::hex32(module->stub_end) << "\n";

        // 5. Set $gp from module info
        runtime.cpu().set_gpr(28, module->gp);

        // 6. Setup minimal valid module_start stack
        constexpr std::uint32_t kStackTop = 0x0A000000u;
        constexpr std::uint32_t kStackSize = 0x10000u; // 64 KiB
        constexpr std::uint32_t kStackBottom = kStackTop - kStackSize;
        const std::uint32_t initial_sp = kStackTop - 0x100u;

        runtime.memory().zero(kStackBottom, kStackSize);
        std::cout << "Stack Arena:      " << psprecomp::hex32(kStackBottom)
                  << " - " << psprecomp::hex32(kStackTop) << "\n";

        // 7. Set $sp
        runtime.cpu().set_gpr(29, initial_sp);

        // 8. Set $ra = thread return sentinel, $a0 = 0, $a1 = 0
        runtime.cpu().set_gpr(31, p3p3ds::hle::ThreadManager::kThreadReturnSentinel);
        runtime.cpu().set_gpr(4, 0u);
        runtime.cpu().set_gpr(5, 0u);

        std::cout << "Initial SP:       " << psprecomp::hex32(initial_sp) << "\n";
        std::cout << "Initial RA:       " << psprecomp::hex32(p3p3ds::hle::ThreadManager::kThreadReturnSentinel)
                  << " (Thread return sentinel)\n";
        std::cout << "Initial A0 / A1:  0x00000000 / 0x00000000\n";

        // 9. Register generated recompiled functions and import wrappers
        psprecomp::register_generated_functions(runtime);
        std::cout << "Registered Entries: " << runtime.function_count()
                  << " (functions, block labels, and import wrappers)\n";

        // 10. Register target-agnostic HLE service modules
        p3p3ds::KernelState kernel_state;
        kernel_state.threads().init_root_thread("root", entry_addr, initial_sp, module->gp);
        p3p3ds::hle::register_all_hle_modules(runtime, kernel_state);

        if (verbose) {
            std::cout << "\nStarting execution at " << psprecomp::hex32(entry_addr) << "...\n";
        }

        // 11. Execute recompiled code
        runtime.run(entry_addr, max_dispatches);

        // 12. Diagnostic summary
        std::cout << "\n=== Execution Result ===\n";
        std::cout << "Stop Reason:      " << runtime.stop_reason() << "\n";
        std::cout << "Stopped:          " << (runtime.stopped() ? "yes" : "no") << "\n";
        std::cout << "Final Guest PC:   " << psprecomp::hex32(runtime.cpu().pc) << "\n";
        std::cout << "Kernel SDK Ver:   " << psprecomp::hex32(kernel_state.compiled_sdk_version()) << "\n";

        print_registers(runtime.cpu());

        // 12. Milestone Verification
        const auto v = verify_milestone(entry_addr, runtime, kernel_state);
        std::cout << "\n=== Milestone Verification ===\n";
        std::cout << "Target:           module_start -> user_main -> sub_08B4E6A0\n";
        std::cout << "Entry (0x" << std::hex << kExpectedEntryPc << "):   "
                  << (v.entry_matched ? "OK" : "FAILED") << "\n";
        std::cout << "SDK Ver (0x" << std::hex << kExpectedSdkVersion << "): "
                  << (v.sdk_version_matched ? "OK" : "FAILED") << "\n";
        std::cout << "Compiler Ver (0x" << std::hex << kExpectedCompilerVersion << "): "
                  << (v.compiler_version_matched ? "OK" : "FAILED") << "\n";
        std::cout << "Final PC (0x" << std::hex << kExpectedStopPc << "): "
                  << (v.pc_matched ? "OK" : "FAILED") << "\n";
        std::cout << "Return RA (0x" << std::hex << kExpectedReturnRa << "):"
                  << (v.ra_matched ? "OK" : "FAILED") << "\n";
        std::cout << "Thread UID (" << std::dec << kExpectedThreadUid << "):      "
                  << (v.thread_uid_matched ? "OK" : "FAILED") << "\n";
        std::cout << "Thread Name (" << kExpectedThreadName << "): "
                  << (v.thread_name_matched ? "OK" : "FAILED") << "\n";
        std::cout << "Stop Reason:      " << (v.stop_reason_matched ? "OK" : "FAILED") << "\n";
        std::cout << "Result:           " << (v.passed ? "[VERIFIED] Milestone passed" : "[FAILED] " + v.failure_detail) << "\n";

        if (v.passed) {
            std::cout << "\nExecution milestone reached and verified successfully.\n";
            return 0;
        } else {
            std::cerr << "\nMilestone verification failed: " << v.failure_detail << "\n";
            return 3;
        }

    } catch (const std::exception &e) {
        std::cerr << "Fatal Exception: " << e.what() << "\n";
        return 1;
    }
}
