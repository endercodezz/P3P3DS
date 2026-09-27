#pragma once
#include "psprecomp/program_analysis.hpp"
#include <set>
namespace p3p3ds {
struct FrontierProof {
    bool accepted{};
    std::string reason;
    std::size_t instructions{}, blocks{};
};
FrontierProof validate_frontier(const psprecomp::GuestMemory &memory,
    const std::vector<psprecomp::ExecutableRange> &ranges,
    const std::map<std::uint32_t, std::string> &seeds,
    const std::set<std::uint32_t> &owned,
    const std::set<std::uint32_t> &imports,
    std::uint32_t target, std::uint32_t caller, std::uint32_t word);
}
