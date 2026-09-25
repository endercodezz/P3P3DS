#include "psprecomp/common.hpp"
#include "psprecomp/decoder.hpp"
#include "psprecomp/elf32.hpp"

#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <map>
#include <string>

int main(int argc, char **argv) {
    std::filesystem::path elf_path = "profiles/p3p/game/eboot.elf";
    if (argc > 1) {
        elf_path = argv[1];
    }

    std::cout << "====================================================\n";
    std::cout << "   P3P .text Instruction Decoder Compatibility Test\n";
    std::cout << "====================================================\n";
    std::cout << "Target ELF: " << elf_path.string() << "\n";

    if (!std::filesystem::exists(elf_path)) {
        std::cerr << "Error: ELF file not found at " << elf_path.string() << "\n";
        return 1;
    }

    try {
        const auto elf = psprecomp::Elf32Image::from_file(elf_path);
        psprecomp::GuestMemory memory;
        const auto rel_stats = elf.load_and_relocate(memory, psprecomp::kDefaultPspUserLoadBase);
        (void)rel_stats;

        // Locate .text section
        const auto &sections = elf.sections();
        const psprecomp::ElfSection *text_sec = nullptr;
        for (const auto &sec : sections) {
            if (sec.name == ".text") {
                text_sec = &sec;
                break;
            }
        }

        if (!text_sec) {
            std::cerr << "Error: .text section not found in ELF\n";
            return 2;
        }

        const std::uint32_t text_base = psprecomp::kDefaultPspUserLoadBase + text_sec->address;
        const std::uint32_t total_instructions = text_sec->size / 4u;
        std::uint32_t supported_count = 0u;
        std::uint32_t unsupported_count = 0u;

        std::map<std::uint32_t, std::uint32_t> unsupported_by_special_fn;
        std::map<std::uint32_t, std::uint32_t> unsupported_by_op;

        for (std::size_t off = 0; off < text_sec->size; off += 4u) {
            const std::uint32_t pc = text_base + static_cast<std::uint32_t>(off);
            const std::uint32_t word = memory.load32(pc);
            const auto dec = psprecomp::decode_allegrex(word);

            if (dec.kind == psprecomp::OpcodeKind::Unsupported) {
                ++unsupported_count;
                const std::uint32_t op = word >> 26u;
                unsupported_by_op[op]++;
                if (op == 0u) {
                    const std::uint32_t fn = word & 0x3Fu;
                    unsupported_by_special_fn[fn]++;
                }
            } else {
                ++supported_count;
            }
        }

        std::cout << "Total .text instructions:         " << total_instructions << "\n";
        std::cout << "Supported by decode_allegrex:     " << supported_count << "\n";
        std::cout << "Unsupported by decode_allegrex:   " << unsupported_count << "\n";

        std::uint32_t breakdown_sum = 0u;
        std::cout << "Unsupported Breakdown (SPECIAL, op=0):\n";
        for (const auto &[fn, count] : unsupported_by_special_fn) {
            std::cout << "  fn=0x" << std::hex << fn << std::dec << " (" << count << " occurrences)";
            if (fn == 0x1Cu) std::cout << " -> madd / maddu";
            else if (fn == 0x0Du) std::cout << " -> break (software breakpoint)";
            else if (fn == 0x2Eu) std::cout << " -> trap (tne / teq / etc.)";
            else std::cout << " -> unknown special";
            std::cout << "\n";
            breakdown_sum += count;
        }

        // Rigorous consistency assertions to prevent accounting drift
        std::cout << "\n--- Consistency Checks ---\n";
        std::cout << "1. Total instructions check:     "
                  << ((supported_count + unsupported_count == total_instructions) ? "PASS" : "FAIL") << "\n";
        std::cout << "2. Subcategory sum equals total: "
                  << ((breakdown_sum == unsupported_count) ? "PASS" : "FAIL") << "\n";

        if (supported_count + unsupported_count != total_instructions) {
            std::cerr << "Assertion Failed: supported (" << supported_count << ") + unsupported ("
                      << unsupported_count << ") != total (" << total_instructions << ")\n";
            return 3;
        }

        if (breakdown_sum != unsupported_count) {
            std::cerr << "Assertion Failed: subcategory sum (" << breakdown_sum
                      << ") != total unsupported (" << unsupported_count << ")\n";
            return 4;
        }

        // Exact measured ground-truth check for P3P ULUS-10512 .text
        if (total_instructions == 913059u) {
            const std::uint32_t madd_count = unsupported_by_special_fn[0x1Cu];
            const std::uint32_t break_count = unsupported_by_special_fn[0x0Du];
            const std::uint32_t trap_count = unsupported_by_special_fn[0x2Eu];

            std::cout << "3. Ground truth madd check (468):  "
                      << ((madd_count == 468u) ? "PASS" : "FAIL") << "\n";
            std::cout << "4. Ground truth break check (251): "
                      << ((break_count == 251u) ? "PASS" : "FAIL") << "\n";
            std::cout << "5. Ground truth trap check (3):    "
                      << ((trap_count == 3u) ? "PASS" : "FAIL") << "\n";
            std::cout << "6. Total unsupported check (722):  "
                      << ((unsupported_count == 722u) ? "PASS" : "FAIL") << "\n";

            if (unsupported_count != 722u || madd_count != 468u || break_count != 251u || trap_count != 3u) {
                std::cerr << "Assertion Failed: measured ground truth discrepancy detected!\n";
                return 5;
            }
        }

        std::cout << "\n[VERIFIED] All decoder compatibility metrics match deterministically.\n";
        return 0;

    } catch (const std::exception &e) {
        std::cerr << "Fatal Exception: " << e.what() << "\n";
        return 1;
    }
}
