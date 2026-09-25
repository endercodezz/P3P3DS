#include "psprecomp/common.hpp"
#include "psprecomp/elf32.hpp"
#include "psprecomp/runtime.hpp"

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

} // namespace

int main(int argc, char **argv) {
    std::filesystem::path elf_path = "profiles/p3p/game/eboot.elf";
    std::uint64_t max_dispatches = 1000u;
    bool verbose = false;

    for (int i = 1; i < argc; ++i) {
        std::string_view arg(argv[i]);
        if (arg == "--elf" && i + 1 < argc) {
            elf_path = argv[++i];
        } else if (arg == "--max-dispatches" && i + 1 < argc) {
            max_dispatches = std::stoull(argv[++i]);
        } else if (arg == "--verbose" || arg == "-v") {
            verbose = true;
        } else if (arg == "--help" || arg == "-h") {
            std::cout << "Usage: p3p_pc_bootstrap [options]\n"
                      << "  --elf <path>            Path to decrypted P3P ELF (default: profiles/p3p/game/eboot.elf)\n"
                      << "  --max-dispatches <N>    Maximum dispatch count (default: 1000)\n"
                      << "  --verbose, -v           Enable verbose runtime traces\n";
            return 0;
        }
    }

    std::cout << "====================================================\n";
    std::cout << "   P3P3DS PC Bootstrap Execution Harness\n";
    std::cout << "====================================================\n";
    std::cout << "Target ELF:       " << elf_path.string() << "\n";
    std::cout << "Dispatch budget:  " << max_dispatches << "\n";

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
        // PSP user RAM top is 0x0A000000. Allocate 64 KiB stack growing down.
        constexpr std::uint32_t kStackTop = 0x0A000000u;
        constexpr std::uint32_t kStackSize = 0x10000u; // 64 KiB
        constexpr std::uint32_t kStackBottom = kStackTop - kStackSize;
        const std::uint32_t initial_sp = kStackTop - 0x100u;

        runtime.memory().zero(kStackBottom, kStackSize);
        std::cout << "Stack Arena:      " << psprecomp::hex32(kStackBottom)
                  << " - " << psprecomp::hex32(kStackTop) << "\n";

        // 7. Set $sp
        runtime.cpu().set_gpr(29, initial_sp);

        // 8. Set $ra = 0, $a0 = 0, $a1 = 0
        runtime.cpu().set_gpr(31, 0u);
        runtime.cpu().set_gpr(4, 0u);
        runtime.cpu().set_gpr(5, 0u);

        std::cout << "Initial SP:       " << psprecomp::hex32(initial_sp) << "\n";
        std::cout << "Initial RA:       0x00000000\n";
        std::cout << "Initial A0 / A1:  0x00000000 / 0x00000000\n";

        // 9. Register generated recompiled functions and import wrappers
        psprecomp::register_generated_functions(runtime);
        std::cout << "Registered Funcs: " << runtime.function_count() << "\n";

        if (verbose) {
            std::cout << "\nStarting execution at " << psprecomp::hex32(entry_addr) << "...\n";
        }

        // 10. Execute recompiled code
        runtime.run(entry_addr, max_dispatches);

        // 11. Diagnostic summary
        std::cout << "\n=== Execution Result ===\n";
        std::cout << "Stop Reason:      " << runtime.stop_reason() << "\n";
        std::cout << "Stopped:          " << (runtime.stopped() ? "yes" : "no") << "\n";
        std::cout << "Final Guest PC:   " << psprecomp::hex32(runtime.cpu().pc) << "\n";

        print_registers(runtime.cpu());

        std::cout << "\nExecution milestone reached successfully.\n";
        return 0;

    } catch (const std::exception &e) {
        std::cerr << "Fatal Exception: " << e.what() << "\n";
        return 1;
    }
}
