#include "psprecomp/program_analysis.hpp"

#include "psprecomp/decoder.hpp"

#include <algorithm>
#include <array>
#include <deque>
#include <limits>
#include <optional>
#include <unordered_map>

namespace psprecomp {

std::vector<ExecutableRange> executable_ranges(const Elf32Image &elf, std::uint32_t load_base) {
    std::vector<ExecutableRange> ranges;
    for (std::size_t i = 0; i < elf.segments().size(); ++i) {
        const auto &segment = elf.segments()[i];
        if (segment.type != 1u || (segment.flags & 1u) == 0u || segment.file_size == 0u) continue;
        const std::uint32_t start = elf.segment_runtime_address(i, load_base);
        const std::uint64_t end64 = static_cast<std::uint64_t>(start) + segment.file_size;
        if (end64 > std::numeric_limits<std::uint32_t>::max()) continue;
        ranges.push_back({start, static_cast<std::uint32_t>(end64)});
    }
    std::sort(ranges.begin(), ranges.end(), [](const auto &a, const auto &b) { return a.start < b.start; });
    return ranges;
}

namespace {

std::uint32_t direct_jump_target(std::uint32_t pc, const DecodedInstruction &decoded) {
    return ((pc + 4u) & 0xF0000000u) | (decoded.target << 2u);
}

std::uint32_t branch_target(std::uint32_t pc, const DecodedInstruction &decoded) {
    const auto displacement = static_cast<std::int32_t>(decoded.immediate) * 4;
    return pc + 4u + static_cast<std::uint32_t>(displacement);
}

bool is_conditional_branch(OpcodeKind kind) {
    switch (kind) {
    case OpcodeKind::Beq: case OpcodeKind::Bne: case OpcodeKind::Beql: case OpcodeKind::Bnel:
    case OpcodeKind::Blez: case OpcodeKind::Bgtz: case OpcodeKind::Blezl: case OpcodeKind::Bgtzl:
    case OpcodeKind::Bltz: case OpcodeKind::Bgez: case OpcodeKind::Bltzl: case OpcodeKind::Bgezl:
    case OpcodeKind::Bltzal: case OpcodeKind::Bgezal: case OpcodeKind::Bltzall: case OpcodeKind::Bgezall:
    case OpcodeKind::Bc1f: case OpcodeKind::Bc1t: case OpcodeKind::Bc1fl: case OpcodeKind::Bc1tl:
    case OpcodeKind::Bvf: case OpcodeKind::Bvt: case OpcodeKind::Bvfl: case OpcodeKind::Bvtl:
        return true;
    default:
        return false;
    }
}

void add_seed(std::map<std::uint32_t, std::string> &seeds,
              const std::vector<ExecutableRange> &ranges,
              std::uint32_t address,
              const char *source) {
    if (is_executable_address(ranges, address)) seeds.try_emplace(address, source);
}

std::set<std::uint32_t> collect_global_block_starts(const GuestMemory &memory,
                                                    const std::vector<ExecutableRange> &ranges) {
    std::set<std::uint32_t> starts;
    for (const auto &range : ranges) {
        starts.insert(range.start);
        for (std::uint32_t pc = range.start; pc + 4u <= range.end; pc += 4u) {
            const auto decoded = decode_allegrex(memory.load32(pc));
            if (is_conditional_branch(decoded.kind)) {
                const auto target = branch_target(pc, decoded);
                if (is_executable_address(ranges, target)) starts.insert(target);
                if (is_executable_address(ranges, pc + 8u)) starts.insert(pc + 8u);
            } else if (decoded.kind == OpcodeKind::J) {
                const auto target = direct_jump_target(pc, decoded);
                if (is_executable_address(ranges, target)) starts.insert(target);
            } else if (decoded.kind == OpcodeKind::Jal || decoded.kind == OpcodeKind::Jalr) {
                if (is_executable_address(ranges, pc + 8u)) starts.insert(pc + 8u);
            }
        }
    }
    return starts;
}

using ConstantState = std::array<std::optional<std::uint32_t>, 32>;

void clear_all_constants(ConstantState &constants) {
    for (auto &value : constants) value.reset();
    constants[0] = 0u;
}

void set_constant_and_seed(ConstantState &constants,
                           std::uint32_t reg_index,
                           std::optional<std::uint32_t> value,
                           std::map<std::uint32_t, std::string> &seeds,
                           const std::vector<ExecutableRange> &ranges) {
    if (reg_index == 0u) return;
    constants[reg_index] = value;
    if (value && reg_index != 31u) add_seed(seeds, ranges, *value, "materialized_code_pointer");
}

void propagate_constant(const DecodedInstruction &decoded,
                        std::uint32_t pc,
                        ConstantState &constants,
                        std::map<std::uint32_t, std::string> &seeds,
                        const std::vector<ExecutableRange> &ranges) {
    const auto lhs = constants[decoded.rs];
    const auto rhs = constants[decoded.rt];
    const auto simm = static_cast<std::int32_t>(decoded.immediate);
    const auto uimm = static_cast<std::uint16_t>(decoded.immediate);
    auto binary = [&](auto operation) -> std::optional<std::uint32_t> {
        if (!lhs || !rhs) return std::nullopt;
        return static_cast<std::uint32_t>(operation(*lhs, *rhs));
    };

    switch (decoded.kind) {
    case OpcodeKind::Lui:
        set_constant_and_seed(constants, decoded.rt, uimm << 16u, seeds, ranges);
        break;
    case OpcodeKind::Addiu:
        set_constant_and_seed(constants, decoded.rt,
            lhs ? std::optional<std::uint32_t>(*lhs + static_cast<std::uint32_t>(simm)) : std::nullopt,
            seeds, ranges);
        break;
    case OpcodeKind::Andi:
        set_constant_and_seed(constants, decoded.rt,
            lhs ? std::optional<std::uint32_t>(*lhs & uimm) : std::nullopt, seeds, ranges);
        break;
    case OpcodeKind::Ori:
        set_constant_and_seed(constants, decoded.rt,
            lhs ? std::optional<std::uint32_t>(*lhs | uimm) : std::nullopt, seeds, ranges);
        break;
    case OpcodeKind::Xori:
        set_constant_and_seed(constants, decoded.rt,
            lhs ? std::optional<std::uint32_t>(*lhs ^ uimm) : std::nullopt, seeds, ranges);
        break;
    case OpcodeKind::Slti:
    case OpcodeKind::Sltiu:
        set_constant_and_seed(constants, decoded.rt, std::nullopt, seeds, ranges);
        break;
    case OpcodeKind::Addu:
        set_constant_and_seed(constants, decoded.rd, binary([](auto a, auto b) { return a + b; }), seeds, ranges);
        break;
    case OpcodeKind::Subu:
        set_constant_and_seed(constants, decoded.rd, binary([](auto a, auto b) { return a - b; }), seeds, ranges);
        break;
    case OpcodeKind::And:
        set_constant_and_seed(constants, decoded.rd, binary([](auto a, auto b) { return a & b; }), seeds, ranges);
        break;
    case OpcodeKind::Or:
        set_constant_and_seed(constants, decoded.rd, binary([](auto a, auto b) { return a | b; }), seeds, ranges);
        break;
    case OpcodeKind::Xor:
        set_constant_and_seed(constants, decoded.rd, binary([](auto a, auto b) { return a ^ b; }), seeds, ranges);
        break;
    case OpcodeKind::Nor:
        set_constant_and_seed(constants, decoded.rd, binary([](auto a, auto b) { return ~(a | b); }), seeds, ranges);
        break;
    case OpcodeKind::Sll:
        set_constant_and_seed(constants, decoded.rd,
            rhs ? std::optional<std::uint32_t>(*rhs << decoded.sa) : std::nullopt, seeds, ranges);
        break;
    case OpcodeKind::Srl:
        set_constant_and_seed(constants, decoded.rd,
            rhs ? std::optional<std::uint32_t>(*rhs >> decoded.sa) : std::nullopt, seeds, ranges);
        break;
    case OpcodeKind::Sra:
        set_constant_and_seed(constants, decoded.rd,
            rhs ? std::optional<std::uint32_t>(static_cast<std::uint32_t>(static_cast<std::int32_t>(*rhs) >> decoded.sa)) : std::nullopt,
            seeds, ranges);
        break;
    case OpcodeKind::Ext: {
        const std::uint32_t size = decoded.rd + 1u;
        const std::uint32_t mask = size == 32u ? 0xFFFFFFFFu : ((1u << size) - 1u);
        set_constant_and_seed(constants, decoded.rt,
            lhs ? std::optional<std::uint32_t>((*lhs >> decoded.sa) & mask) : std::nullopt,
            seeds, ranges);
        break;
    }
    case OpcodeKind::Ins: {
        const std::uint32_t size = decoded.rd >= decoded.sa ? decoded.rd - decoded.sa + 1u : 0u;
        const std::uint32_t source_mask = size == 32u ? 0xFFFFFFFFu : (size == 0u ? 0u : ((1u << size) - 1u));
        const std::uint32_t destination_mask = source_mask << decoded.sa;
        const auto destination = constants[decoded.rt];
        set_constant_and_seed(constants, decoded.rt,
            lhs && destination ? std::optional<std::uint32_t>((*destination & ~destination_mask) | ((*lhs & source_mask) << decoded.sa)) : std::nullopt,
            seeds, ranges);
        break;
    }
    case OpcodeKind::Slt:
    case OpcodeKind::Sltu:
    case OpcodeKind::Mfhi:
    case OpcodeKind::Mflo:
        set_constant_and_seed(constants, decoded.rd, std::nullopt, seeds, ranges);
        break;
    case OpcodeKind::Lw: case OpcodeKind::Lwl: case OpcodeKind::Lwr:
    case OpcodeKind::Lh: case OpcodeKind::Lhu:
    case OpcodeKind::Lb: case OpcodeKind::Lbu: case OpcodeKind::Lwc1:
    case OpcodeKind::Mfc1: case OpcodeKind::Cfc1: case OpcodeKind::Mfv:
        set_constant_and_seed(constants, decoded.rt, std::nullopt, seeds, ranges);
        break;
    case OpcodeKind::Jal:
    case OpcodeKind::Bltzal:
    case OpcodeKind::Bgezal:
    case OpcodeKind::Bltzall:
    case OpcodeKind::Bgezall:
        constants[31] = pc + 8u;
        break;
    case OpcodeKind::Jalr:
        if (decoded.rd != 0u) constants[decoded.rd] = pc + 8u;
        break;
    case OpcodeKind::Unsupported:
    case OpcodeKind::Vfpu:
    case OpcodeKind::Syscall:
        clear_all_constants(constants);
        break;
    default:
        break;
    }
    constants[0] = 0u;
}

void collect_materialized_code_pointers(const GuestMemory &memory,
                                        const std::vector<ExecutableRange> &ranges,
                                        std::map<std::uint32_t, std::string> &seeds) {
    const auto block_starts = collect_global_block_starts(memory, ranges);
    for (const auto block_start : block_starts) {
        if (!is_executable_address(ranges, block_start)) continue;
        ConstantState constants{};
        clear_all_constants(constants);
        std::uint32_t pc = block_start;
        while (is_executable_address(ranges, pc)) {
            if (pc != block_start && block_starts.contains(pc)) break;
            const auto decoded = decode_allegrex(memory.load32(pc));
            propagate_constant(decoded, pc, constants, seeds, ranges);
            if (decoded.has_delay_slot()) {
                const auto slot = decode_allegrex(memory.load32(pc + 4u));
                propagate_constant(slot, pc + 4u, constants, seeds, ranges);
                break;
            }
            if (decoded.is_control_flow()) break;
            pc += 4u;
        }
    }

    // Function pointers are frequently assembled immediately before an API call,
    // including in the call's delay slot. A global block boundary can split the
    // LUI from that final ADDIU/ORI when the surrounding code has overlapping
    // entry points. Scan a short straight-line window from every LUI as a
    // conservative, architecture-wide supplement. Only values inside executable
    // ranges become seeds, so data addresses and ordinary constants are ignored.
    for (const auto &range : ranges) {
        for (std::uint32_t start = range.start; start + 4u <= range.end; start += 4u) {
            const auto first = decode_allegrex(memory.load32(start));
            if (first.kind != OpcodeKind::Lui || first.rt == 0u) continue;

            ConstantState constants{};
            clear_all_constants(constants);
            std::uint32_t pc = start;
            for (std::size_t count = 0; count < 24u && is_executable_address(ranges, pc); ++count) {
                const auto decoded = decode_allegrex(memory.load32(pc));
                propagate_constant(decoded, pc, constants, seeds, ranges);
                if (decoded.has_delay_slot()) {
                    if (is_executable_address(ranges, pc + 4u)) {
                        const auto slot = decode_allegrex(memory.load32(pc + 4u));
                        propagate_constant(slot, pc + 4u, constants, seeds, ranges);
                    }
                    break;
                }
                if (decoded.is_control_flow()) break;
                pc += 4u;
            }
        }
    }
}

void collect_relocated_data_code_pointers(const Elf32Image &elf,
                                          const GuestMemory &memory,
                                          std::uint32_t load_base,
                                          const std::vector<ExecutableRange> &ranges,
                                          std::map<std::uint32_t, std::string> &seeds) {
    for (std::size_t i = 0; i < elf.segments().size(); ++i) {
        const auto &segment = elf.segments()[i];
        if (segment.type != 1u || (segment.flags & 1u) != 0u || segment.file_size < 4u) continue;
        const std::uint32_t start = elf.segment_runtime_address(i, load_base);
        const std::uint32_t size = segment.file_size & ~3u;
        for (std::uint32_t offset = 0u; offset < size; offset += 4u) {
            add_seed(seeds, ranges, memory.load32(start + offset), "relocated_data_code_pointer");
        }
    }
}

std::map<std::uint32_t, std::string> collect_initial_seeds(const Elf32Image &elf,
                                                           const GuestMemory &memory,
                                                           std::uint32_t load_base,
                                                           const std::vector<ExecutableRange> &ranges) {
    std::map<std::uint32_t, std::string> seeds;
    const std::uint32_t entry = elf.runtime_entry(load_base);
    add_seed(seeds, ranges, entry, "elf_entry");

    for (const auto &range : ranges) {
        for (std::uint32_t pc = range.start; pc + 4u <= range.end; pc += 4u) {
            const auto decoded = decode_allegrex(memory.load32(pc));
            if (decoded.kind != OpcodeKind::Jal) continue;
            add_seed(seeds, ranges, direct_jump_target(pc, decoded), "direct_jal_target");
        }
    }
    collect_materialized_code_pointers(memory, ranges, seeds);
    collect_relocated_data_code_pointers(elf, memory, load_base, ranges, seeds);
    // R_MIPS_32 relocations are the authoritative source for function pointers
    // stored in read-only tables embedded in the executable segment (init arrays,
    // vtables, callbacks). Scanning raw RX words would confuse J opcodes with pointers.
    for (const auto &site : elf.relocation_sites(load_base)) {
        if (site.type != 2u || !memory.contains(site.patch_address, 4u)) continue;
        add_seed(seeds, ranges, memory.load32(site.patch_address), "relocated_r_mips32_code_pointer");
    }
    return seeds;
}

} // namespace

FunctionAnalysis analyze_function(std::uint32_t entry,
                                  const GuestMemory &memory,
                                  const std::vector<ExecutableRange> &ranges,
                                  const std::map<std::uint32_t, std::string> &known_seeds,
                                  std::size_t max_instructions) {
    FunctionAnalysis result{};
    result.entry = entry;
    std::deque<std::uint32_t> pending_blocks;
    std::set<std::uint32_t> queued_blocks;
    pending_blocks.push_back(entry);
    queued_blocks.insert(entry);

    while (!pending_blocks.empty()) {
        const std::uint32_t block_start = pending_blocks.front();
        pending_blocks.pop_front();
        if (!is_executable_address(ranges, block_start)) continue;
        const bool new_entry = result.entry_labels.insert(block_start).second;
        if (new_entry) ++result.basic_block_count;
        // A branch target may have already been decoded linearly from another block.
        // It still must remain a dispatcher/basic-block entry.
        if (result.labels.contains(block_start)) continue;

        std::uint32_t pc = block_start;
        while (is_executable_address(ranges, pc)) {
            if (result.labels.size() >= max_instructions) {
                result.truncated = true;
                return result;
            }
            if (result.labels.contains(pc)) break;
            if (pc != entry && known_seeds.contains(pc) && pc != block_start) break;

            result.labels.insert(pc);
            const auto decoded = decode_allegrex(memory.load32(pc));
            if (decoded.kind == OpcodeKind::Unsupported || decoded.kind == OpcodeKind::Vfpu) {
                ++result.unsupported_instruction_count;
            }

            if (is_conditional_branch(decoded.kind)) {
                const std::uint32_t taken = branch_target(pc, decoded);
                const std::uint32_t fallthrough = pc + 8u;
                if (is_executable_address(ranges, taken) && queued_blocks.insert(taken).second) pending_blocks.push_back(taken);
                if (is_executable_address(ranges, fallthrough) && queued_blocks.insert(fallthrough).second) pending_blocks.push_back(fallthrough);
                break;
            }

            if (decoded.kind == OpcodeKind::J) {
                const std::uint32_t target = direct_jump_target(pc, decoded);
                if (is_executable_address(ranges, target)) {
                    if (target == entry || !known_seeds.contains(target)) {
                        if (queued_blocks.insert(target).second) pending_blocks.push_back(target);
                    } else {
                        result.direct_calls.insert(target); // tail call to another known entry.
                    }
                }
                break;
            }

            if (decoded.kind == OpcodeKind::Jal) {
                const std::uint32_t target = direct_jump_target(pc, decoded);
                if (is_executable_address(ranges, target)) result.direct_calls.insert(target);
                const std::uint32_t continuation = pc + 8u;
                if (is_executable_address(ranges, continuation) && queued_blocks.insert(continuation).second) {
                    pending_blocks.push_back(continuation);
                }
                break;
            }

            if (decoded.kind == OpcodeKind::Jalr) {
                result.indirect_call_sites.insert(pc);
                const std::uint32_t continuation = pc + 8u;
                if (is_executable_address(ranges, continuation) && queued_blocks.insert(continuation).second) {
                    pending_blocks.push_back(continuation);
                }
                break;
            }

            if (decoded.kind == OpcodeKind::Jr) break;

            pc += decoded.has_delay_slot() ? 8u : 4u;
        }
    }
    return result;
}

bool is_executable_address(const std::vector<ExecutableRange> &ranges, std::uint32_t address) noexcept {
    const auto it = std::upper_bound(ranges.begin(), ranges.end(), address,
        [](std::uint32_t value, const ExecutableRange &range) { return value < range.start; });
    if (it == ranges.begin()) return false;
    const auto &range = *std::prev(it);
    return address >= range.start && address < range.end && (address & 3u) == 0u;
}

ProgramAnalysis analyze_program(const Elf32Image &elf,
                                const GuestMemory &memory,
                                std::uint32_t load_base,
                                std::size_t max_instructions_per_function) {
    ProgramAnalysis program{};
    program.executable_ranges = executable_ranges(elf, load_base);
    program.seeds = collect_initial_seeds(elf, memory, load_base, program.executable_ranges);
    program.functions.reserve(program.seeds.size());

    std::unordered_map<std::uint32_t, std::size_t> label_owners;
    for (const auto &[entry, source] : program.seeds) {
        (void)source;
        auto function = analyze_function(entry, memory, program.executable_ranges, program.seeds,
                                         max_instructions_per_function);
        for (const auto label : function.labels) program.covered_labels.insert(label);
        for (const auto label : function.entry_labels) {
            const auto [it, inserted] = label_owners.emplace(label, program.functions.size());
            if (!inserted && it->second != program.functions.size()) ++program.overlapping_label_count;
            program.covered_entry_labels.insert(label);
        }
        program.functions.push_back(std::move(function));
    }
    return program;
}

} // namespace psprecomp
