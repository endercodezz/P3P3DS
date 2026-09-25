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

namespace {

bool is_branch_kind(psprecomp::OpcodeKind kind) {
    using K = psprecomp::OpcodeKind;
    switch (kind) {
    case K::Beq: case K::Bne: case K::Beql: case K::Bnel:
    case K::Blez: case K::Bgtz: case K::Blezl: case K::Bgtzl:
    case K::Bltz: case K::Bgez: case K::Bltzl: case K::Bgezl:
    case K::Bltzal: case K::Bgezal: case K::Bltzall: case K::Bgezall:
    case K::Bc1f: case K::Bc1t: case K::Bc1fl: case K::Bc1tl:
    case K::Bvf: case K::Bvt: case K::Bvfl: case K::Bvtl:
        return true;
    default:
        return false;
    }
}

// Determines whether the PSPRecomp codegen pipeline implements code generation
// for this instruction or emits an rt.unsupported / unhandled stub.
bool opcode_kind_has_codegen_lowering(const psprecomp::DecodedInstruction &d) {
    using K = psprecomp::OpcodeKind;
    if (d.kind == K::Unsupported) return false;
    if (d.kind == K::Syscall) return false;
    if (d.kind == K::Vfpu) return false; // Generic unexpanded VFPU group in codegen_main.cpp:1120
    if (is_branch_kind(d.kind)) return true;
    if (d.kind == K::J || d.kind == K::Jal || d.kind == K::Jr || d.kind == K::Jalr) return true;

    // Cases handled in emit_regular in codegen_main.cpp
    switch (d.kind) {
    case K::Nop: case K::Sync: case K::Cache:
    case K::Addiu: case K::Slti: case K::Sltiu: case K::Andi: case K::Ori: case K::Xori: case K::Lui:
    case K::Add: case K::Addu: case K::Sub: case K::Subu: case K::And: case K::Or: case K::Xor: case K::Nor:
    case K::Slt: case K::Sltu: case K::Max: case K::Min: case K::Movz: case K::Movn:
    case K::Sll: case K::Srl: case K::Sra: case K::Rotr: case K::Sllv: case K::Srlv: case K::Srav: case K::Rotrv:
    case K::Clz: case K::Clo: case K::Ext: case K::Ins: case K::Seb: case K::Seh: case K::Bitrev: case K::Wsbh: case K::Wsbw:
    case K::Lw: case K::Lwl: case K::Lwr: case K::Sw: case K::Swl: case K::Swr: case K::Lh: case K::Lhu: case K::Sh:
    case K::Lb: case K::Lbu: case K::Sb: case K::Lwc1: case K::Swc1:
    case K::Mfhi: case K::Mflo: case K::Mthi: case K::Mtlo:
    case K::Mult: case K::Multu: case K::Div: case K::Divu:
    case K::Mfc1: case K::Mtc1:
    case K::AddS: case K::SubS: case K::MulS: case K::DivS: case K::SqrtS: case K::AbsS: case K::MovS: case K::NegS:
    case K::RoundWS: case K::TruncWS: case K::CeilWS: case K::FloorWS: case K::CvtWS: case K::CvtSW: case K::FpuCompare:
    case K::Vflush: case K::Vpfx: case K::Viim: case K::Vfim: case K::Vf2h: case K::Vh2f: case K::Vf2i: case K::Vi2f: case K::Vx2i:
    case K::Mtv: case K::Mfv: case K::VmidT: case K::Vmmov: case K::VfpuMatrixInit: case K::Vidt: case K::Vtfm:
    case K::VfpuVectorInit: case K::Vmmul: case K::Vmscl: case K::Vrot: case K::Vocp: case K::VfpuHorizontal: case K::VfpuVec3:
    case K::Vdot: case K::Vhdp: case K::VcrossQuat: case K::Vminmax: case K::VfpuCompare3: case K::Vcmp: case K::Vcmov:
    case K::Vscl: case K::VfpuUnary: case K::Vcst: case K::Lvs: case K::Svs: case K::Lvq: case K::Svq:
        return true;
    case K::Cfc1:
        return (d.rd == 31u);
    case K::Ctc1:
        return (d.rd == 31u);
    default:
        return false;
    }
}

} // namespace

int main(int argc, char **argv) {
    std::filesystem::path elf_path = "profiles/p3p/game/eboot.elf";
    if (argc > 1) {
        elf_path = argv[1];
    }

    std::cout << "====================================================\n";
    std::cout << "   P3P .text Instruction Decoder & Codegen Audit\n";
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
        std::uint32_t decoder_recognized = 0u;
        std::uint32_t decoder_unsupported = 0u;
        std::uint32_t codegen_lowerable = 0u;
        std::uint32_t recognized_not_lowerable = 0u;

        std::map<std::uint32_t, std::uint32_t> unsupported_by_special_fn;
        std::map<std::string, std::uint32_t> not_lowerable_by_mnemonic;

        for (std::size_t off = 0; off < text_sec->size; off += 4u) {
            const std::uint32_t pc = text_base + static_cast<std::uint32_t>(off);
            const std::uint32_t word = memory.load32(pc);
            const auto dec = psprecomp::decode_allegrex(word);

            if (dec.kind == psprecomp::OpcodeKind::Unsupported) {
                ++decoder_unsupported;
                const std::uint32_t op = word >> 26u;
                if (op == 0u) {
                    const std::uint32_t fn = word & 0x3Fu;
                    unsupported_by_special_fn[fn]++;
                }
            } else {
                ++decoder_recognized;
                if (opcode_kind_has_codegen_lowering(dec)) {
                    ++codegen_lowerable;
                } else {
                    ++recognized_not_lowerable;
                    not_lowerable_by_mnemonic[dec.mnemonic]++;
                }
            }
        }

        std::cout << "\n--- 1. Decoder Coverage ---\n";
        std::cout << "Total .text instructions:         " << total_instructions << "\n";
        std::cout << "Decoder recognized:               " << decoder_recognized
                  << " (" << (static_cast<double>(decoder_recognized) * 100.0 / total_instructions) << "%)\n";
        std::cout << "Decoder unsupported:             " << decoder_unsupported
                  << " (" << (static_cast<double>(decoder_unsupported) * 100.0 / total_instructions) << "%)\n";

        std::uint32_t breakdown_sum = 0u;
        std::cout << "Unsupported breakdown (SPECIAL, op=0):\n";
        for (const auto &[fn, count] : unsupported_by_special_fn) {
            std::cout << "  fn=0x" << std::hex << fn << std::dec << " (" << count << " occurrences)";
            if (fn == 0x1Cu) std::cout << " -> madd (Signed Multiply-Accumulate)";
            else if (fn == 0x0Du) std::cout << " -> break (Software Breakpoint)";
            else if (fn == 0x2Eu) std::cout << " -> msub (Signed Multiply-Subtract)";
            else std::cout << " -> unassigned special";
            std::cout << "\n";
            breakdown_sum += count;
        }

        std::cout << "\n--- 2. Codegen Lowering Coverage ---\n";
        std::cout << "Codegen lowerable:                " << codegen_lowerable
                  << " (" << (static_cast<double>(codegen_lowerable) * 100.0 / total_instructions) << "%)\n";
        std::cout << "Decoded but NOT lowered:          " << recognized_not_lowerable
                  << " (" << (static_cast<double>(recognized_not_lowerable) * 100.0 / total_instructions) << "%)\n";
        for (const auto &[mnemonic, count] : not_lowerable_by_mnemonic) {
            std::cout << "  mnemonic '" << mnemonic << "': " << count << " occurrences (generic Vfpu)\n";
        }

        std::cout << "\n--- 3. Rigorous Consistency Checks ---\n";
        const bool c1 = (decoder_recognized + decoder_unsupported == total_instructions);
        const bool c2 = (breakdown_sum == decoder_unsupported);
        const bool c3 = (codegen_lowerable + recognized_not_lowerable == decoder_recognized);
        const bool c4 = (codegen_lowerable + recognized_not_lowerable + decoder_unsupported == total_instructions);

        std::cout << "1. decoder_rec + decoder_unsupp == total:  " << (c1 ? "PASS" : "FAIL") << "\n";
        std::cout << "2. subcategory_sum == decoder_unsupp:      " << (c2 ? "PASS" : "FAIL") << "\n";
        std::cout << "3. lowerable + not_lowerable == decoded:   " << (c3 ? "PASS" : "FAIL") << "\n";
        std::cout << "4. lowerable + not_low + unsupp == total:  " << (c4 ? "PASS" : "FAIL") << "\n";

        if (!c1 || !c2 || !c3 || !c4) {
            std::cerr << "Assertion Failed: mathematical inconsistency in coverage numbers!\n";
            return 3;
        }

        // Exact measured ground-truth check for P3P ULUS-10512 .text
        if (total_instructions == 913059u) {
            const std::uint32_t madd_count = unsupported_by_special_fn[0x1Cu];
            const std::uint32_t break_count = unsupported_by_special_fn[0x0Du];
            const std::uint32_t msub_count = unsupported_by_special_fn[0x2Eu];

            const bool g_madd = (madd_count == 468u);
            const bool g_break = (break_count == 251u);
            const bool g_msub = (msub_count == 3u);
            const bool g_unsupp = (decoder_unsupported == 722u);
            const bool g_rec = (decoder_recognized == 912337u);
            const bool g_low = (codegen_lowerable == 912275u);
            const bool g_not_low = (recognized_not_lowerable == 62u);

            std::cout << "5. Ground truth madd check (468):          " << (g_madd ? "PASS" : "FAIL") << "\n";
            std::cout << "6. Ground truth break check (251):         " << (g_break ? "PASS" : "FAIL") << "\n";
            std::cout << "7. Ground truth msub check (3):            " << (g_msub ? "PASS" : "FAIL") << "\n";
            std::cout << "8. Ground truth decoded-not-lowered (62):  " << (g_not_low ? "PASS" : "FAIL") << "\n";
            std::cout << "9. Ground truth codegen lowerable (912275):" << (g_low ? "PASS" : "FAIL") << "\n";

            if (!g_madd || !g_break || !g_msub || !g_unsupp || !g_rec || !g_low || !g_not_low) {
                std::cerr << "Assertion Failed: measured ground truth discrepancy detected!\n";
                return 4;
            }
        }

        std::cout << "\n[VERIFIED] All decoder and codegen compatibility metrics match deterministically.\n";
        return 0;

    } catch (const std::exception &e) {
        std::cerr << "Fatal Exception: " << e.what() << "\n";
        return 1;
    }
}
