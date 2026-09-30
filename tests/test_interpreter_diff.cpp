// Differential test: AOT (+ interpreter fallback) versus pure interpretation.
//
// For a deterministic sample of psp_analyze function seeds and pseudo-random
// register files, both engines start from identical CPU/RAM/VRAM state and run
// until the Nth recorded control transfer (Runtime::transfer_budget), a PSP
// import stub, a return to an out-of-memory sentinel, or a runtime stop. All
// GPR/HI/LO/FPR/FCR31/VFPU/VFPU-control state, RAM, VRAM, the last transfer
// and the stop class must be identical.
//
// Usage: test_interpreter_diff [elf] [function_count] [seeds_per_function] [transfer_budget]
// Returns 77 (CTest skip) when the locally supplied ELF is absent.
#include "p3p3ds/interpreter.hpp"

#include "psprecomp/common.hpp"
#include "psprecomp/elf32.hpp"
#include "psprecomp/program_analysis.hpp"
#include "psprecomp/runtime.hpp"

#include <algorithm>
#include <bit>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <random>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace psprecomp { void register_generated_functions(Runtime &); }

namespace {
using psprecomp::AllegrexContext;
using psprecomp::Runtime;

constexpr std::uint32_t kSentinelReturn = 0x00000004u; // outside guest memory
constexpr std::uint32_t kScratch = 0x09000000u;        // pseudo-random pointers land here

struct Outcome {
    AllegrexContext cpu;
    std::vector<std::uint8_t> ram, vram;
    psprecomp::ExecutedTransfer last;
    std::string kind;
    std::uint64_t transfers{};
};

std::string classify(const Runtime &rt, bool budget, bool error) {
    if (budget) return "budget";
    if (error) return "memory_error";
    const auto &reason = rt.stop_reason();
    if (reason.starts_with("Missing HLE")) return "import";
    if (reason.starts_with("No recompiled function") || reason.starts_with("Interpreter left guest memory"))
        return "left_code";
    if (reason.starts_with("Unsupported Allegrex")) return "unsupported:" + reason;
    if (reason.starts_with("Allegrex arithmetic overflow")) return "overflow:" + reason;
    return "other:" + reason;
}

void randomize(AllegrexContext &ctx, std::mt19937 &rng, std::uint32_t gp) {
    ctx = AllegrexContext{};
    for (std::uint32_t r = 1; r < 32; ++r) {
        switch (rng() % 8u) {
        case 0: case 1: ctx.gpr[r] = rng() % 64u; break;
        case 2: ctx.gpr[r] = rng(); break;
        default: ctx.gpr[r] = kScratch + ((rng() % 0x80000u) & ~15u); break;
        }
    }
    ctx.gpr[28] = gp;
    ctx.gpr[29] = 0x09F00000u - ((rng() % 256u) * 16u);
    ctx.gpr[31] = kSentinelReturn;
    ctx.hi = rng(); ctx.lo = rng();
    for (auto &f : ctx.fpr) f = static_cast<float>(static_cast<std::int32_t>(rng() % 2001u) - 1000) / 7.0f;
    for (auto &v : ctx.vfpu) v = static_cast<float>(static_cast<std::int32_t>(rng() % 2001u) - 1000) / 13.0f;
    ctx.vfpu_ctrl[0] = ctx.vfpu_ctrl[1] = 0x000000E4u; // identity source prefixes
}

bool same_cpu(const AllegrexContext &a, const AllegrexContext &b, std::string &why) {
    for (std::size_t i = 0; i < 32; ++i)
        if (a.gpr[i] != b.gpr[i]) { why = "gpr" + std::to_string(i); return false; }
    if (a.hi != b.hi || a.lo != b.lo) { why = "hi/lo"; return false; }
    for (std::size_t i = 0; i < 32; ++i)
        if (std::bit_cast<std::uint32_t>(a.fpr[i]) != std::bit_cast<std::uint32_t>(b.fpr[i])) { why = "fpr" + std::to_string(i); return false; }
    if (a.fcr31 != b.fcr31) { why = "fcr31"; return false; }
    for (std::size_t i = 0; i < a.vfpu.size(); ++i)
        if (std::bit_cast<std::uint32_t>(a.vfpu[i]) != std::bit_cast<std::uint32_t>(b.vfpu[i])) { why = "vfpu" + std::to_string(i); return false; }
    if (a.vfpu_ctrl != b.vfpu_ctrl) { why = "vfpu_ctrl"; return false; }
    return true;
}
} // namespace

int main(int argc, char **argv) {
    const std::filesystem::path elf_path = argc > 1 ? argv[1] : "profiles/p3p/game/eboot.elf";
    const std::size_t function_count = argc > 2 ? std::stoul(argv[2]) : 400u;
    const std::size_t seeds_per_function = argc > 3 ? std::stoul(argv[3]) : 3u;
    const std::uint64_t budget = argc > 4 ? std::stoull(argv[4]) : 400u;
    if (!std::filesystem::exists(elf_path)) {
        std::cout << "SKIP: ELF not present: " << elf_path.string() << "\n";
        return 77;
    }
    try {
        const auto elf = psprecomp::Elf32Image::from_file(elf_path);
        Runtime rt;
        (void)elf.load_and_relocate(rt.memory());
        const auto module = elf.find_module_info(rt.memory());
        if (!module) throw std::runtime_error("missing module info");
        psprecomp::register_generated_functions(rt);
        const auto imports = elf.scan_imports(rt.memory(), *module);
        const auto program = psprecomp::analyze_program(elf, rt.memory(), psprecomp::kDefaultPspUserLoadBase);

        p3p3ds::Interpreter pure;
        for (const auto &import : imports) pure.stop_pcs.insert(import.stub_address);
        p3p3ds::Interpreter fallback;
        p3p3ds::install_interpreter_fallback(&fallback);

        const std::vector<std::uint8_t> ram0 = rt.memory().bytes();
        const std::vector<std::uint8_t> vram0 = rt.memory().vram_bytes();
        auto restore = [&] {
            rt.memory().copy_in(psprecomp::GuestMemory::kPhysicalBase, ram0);
            std::memcpy(rt.memory().raw_pointer(psprecomp::GuestMemory::kVramPhysicalBase, vram0.size()),
                        vram0.data(), vram0.size()); // no VRAM-store diagnostics
            rt.memory().reset_vram_writes();
            rt.events.clear();
            rt.recent_transfers.clear();
            rt.last_transfer = {};
            rt.transfers_recorded = 0u;
        };

        std::vector<std::uint32_t> seeds;
        // Only functions whose analyzed CFG is fully lowerable: many analyzer
        // seeds are materialized pointers into data, which say nothing about
        // instruction semantics.
        for (const auto &function : program.functions)
            if (function.unsupported_instruction_count == 0u && !function.truncated &&
                function.labels.size() >= 8u && rt.has_function(function.entry))
                seeds.push_back(function.entry);
        std::sort(seeds.begin(), seeds.end());
        seeds.erase(std::unique(seeds.begin(), seeds.end()), seeds.end());
        const std::size_t stride = std::max<std::size_t>(1u, seeds.size() / std::max<std::size_t>(1u, function_count));

        rt.frontier_diagnostics = true;
        rt.transfer_budget = budget;
        std::size_t cases = 0, failures = 0, functions = 0;
        std::map<std::string, std::size_t> kinds_seen;
        std::size_t interior_cases = 0;
        std::uint64_t total_transfers = 0;
        for (std::size_t index = 0; index < seeds.size() && functions < function_count; index += stride, ++functions) {
            const std::uint32_t function_entry = seeds[index];
            for (std::size_t seed = 0; seed < seeds_per_function; ++seed) {
                std::mt19937 rng(static_cast<std::uint32_t>(function_entry * 2654435761u + seed));
                // Odd seeds start at an interior, unregistered instruction of the
                // same function so the mixed engine must enter via the fallback.
                std::uint32_t entry = function_entry;
                if ((seed & 1u) != 0u) {
                    for (std::uint32_t probe = function_entry + 4u; probe < function_entry + 256u; probe += 4u) {
                        if (program.covered_labels.contains(probe) && !rt.has_function(probe) &&
                            !psprecomp::decode_allegrex(rt.memory().load32(probe - 4u)).has_delay_slot()) {
                            entry = probe;
                            break;
                        }
                    }
                }
                if (entry != function_entry) ++interior_cases;
                AllegrexContext start;
                randomize(start, rng, module->gp);

                auto execute = [&](bool aot) {
                    restore();
                    Outcome out;
                    bool hit_budget = false, error = false;
                    rt.cpu() = start;
                    rt.cpu().pc = entry;
                    try {
                        if (aot) {
                            rt.run(entry, 1'000'000u);
                        } else {
                            // A zero-dispatch run only clears the latched stop state.
                            rt.run(entry, 0u);
                            rt.cpu() = start;
                            rt.cpu().pc = entry;
                            (void)pure.run(rt, rt.cpu(), ~0ull, false);
                            if (!rt.stopped() && pure.stop_pcs.contains(rt.cpu().pc))
                                rt.stop("Missing HLE import (interpreter stop set)");
                        }
                    } catch (const psprecomp::TransferBudgetHalt &) {
                        hit_budget = true;
                    } catch (const psprecomp::Error &) {
                        error = true;
                    }
                    out.cpu = rt.cpu();
                    out.ram = rt.memory().bytes();
                    out.vram = rt.memory().vram_bytes();
                    out.last = rt.last_transfer;
                    out.kind = classify(rt, hit_budget, error);
                    out.transfers = rt.transfers_recorded;
                    return out;
                };
                const Outcome a = execute(true);
                const Outcome b = execute(false);
                ++cases;
                total_transfers += a.transfers;
                ++kinds_seen[a.kind.substr(0, a.kind.find(':'))];
                if (std::getenv("P3P_DIFF_VERBOSE") != nullptr && a.kind.starts_with("unsupported"))
                    std::cout << "UNSUPPORTED entry=" << psprecomp::hex32(entry) << " " << a.kind << "\n";
                std::string why;
                bool ok = a.kind == b.kind && a.transfers == b.transfers;
                if (!ok) why = "outcome " + a.kind + " vs " + b.kind + " transfers " +
                               std::to_string(a.transfers) + " vs " + std::to_string(b.transfers);
                if (ok && !same_cpu(a.cpu, b.cpu, why)) ok = false;
                if (ok && (a.last.pc != b.last.pc || a.last.word != b.last.word || a.last.target != b.last.target)) {
                    ok = false; why = "last transfer";
                }
                if (ok && a.kind != "budget" && a.kind != "memory_error" && a.cpu.pc != b.cpu.pc &&
                    (a.kind == "import" || a.kind == "left_code")) {
                    ok = false; why = "final pc";
                }
                if (ok && a.ram != b.ram) { ok = false; why = "ram"; }
                if (ok && a.vram != b.vram) { ok = false; why = "vram"; }
                if (!ok) {
                    ++failures;
                    if (failures <= 20)
                        std::cout << "MISMATCH entry=" << psprecomp::hex32(entry) << " seed=" << seed << " " << why
                                  << " last=" << psprecomp::hex32(a.last.pc) << "/" << psprecomp::hex32(b.last.pc) << "\n";
                }
            }
        }
        std::cout << "interpreter differential: functions=" << functions << " cases=" << cases
                  << " failures=" << failures << " transfers=" << total_transfers
                  << " interior_starts=" << interior_cases << " fallback_entries=" << fallback.entries()
                  << " fallback_instructions=" << fallback.executed() << " outcomes:";
        for (const auto &[k, n] : kinds_seen) std::cout << ' ' << k << '=' << n;
        std::cout << "\n";
        return failures == 0 ? 0 : 1;
    } catch (const std::exception &e) {
        std::cerr << "Fatal: " << e.what() << "\n";
        return 1;
    }
}
