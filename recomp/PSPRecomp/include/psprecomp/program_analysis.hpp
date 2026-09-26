#pragma once

#include "psprecomp/elf32.hpp"

#include <cstdint>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace psprecomp {

struct ExecutableRange {
    std::uint32_t start{};
    std::uint32_t end{};
};

struct FunctionAnalysis {
    std::uint32_t entry{};
    std::set<std::uint32_t> labels; // all decoded instruction addresses (delay slots excluded)
    std::set<std::uint32_t> entry_labels; // function entry and basic-block/return continuations
    std::set<std::uint32_t> direct_calls;
    std::set<std::uint32_t> indirect_call_sites;
    std::size_t basic_block_count{};
    std::size_t unsupported_instruction_count{};
    bool truncated{};
};

struct ProgramAnalysis {
    std::vector<ExecutableRange> executable_ranges;
    std::map<std::uint32_t, std::string> seeds;
    std::vector<FunctionAnalysis> functions;
    std::set<std::uint32_t> covered_labels;
    std::set<std::uint32_t> covered_entry_labels;
    std::size_t overlapping_label_count{};
};

[[nodiscard]] bool is_executable_address(const std::vector<ExecutableRange> &ranges,
                                         std::uint32_t address) noexcept;

[[nodiscard]] std::vector<ExecutableRange> executable_ranges(const Elf32Image &elf,
                                                             std::uint32_t load_base = kDefaultPspUserLoadBase);

[[nodiscard]] FunctionAnalysis analyze_function(std::uint32_t entry,
                                                const GuestMemory &memory,
                                                const std::vector<ExecutableRange> &ranges,
                                                const std::map<std::uint32_t, std::string> &known_seeds = {},
                                                std::size_t max_instructions = 131072u);

[[nodiscard]] ProgramAnalysis analyze_program(const Elf32Image &elf,
                                              const GuestMemory &memory,
                                              std::uint32_t load_base,
                                              std::size_t max_instructions_per_function = 131072u);

} // namespace psprecomp
