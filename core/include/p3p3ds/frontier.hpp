#pragma once
#include "psprecomp/program_analysis.hpp"
#include <set>
namespace p3p3ds {
struct FrontierProof {
    bool accepted{};
    std::string reason;
    std::size_t instructions{}, blocks{};
};
enum class FrontierOriginKind { DirectJal, ThreadEntry };
struct FrontierOrigin {
    FrontierOriginKind kind{FrontierOriginKind::DirectJal};
    std::uint32_t caller{}, word{};
    std::int32_t thread_uid{};
    std::uint32_t entry_pc{};
    static FrontierOrigin direct_jal(std::uint32_t caller, std::uint32_t word) {
        return {FrontierOriginKind::DirectJal,caller,word,0,0};
    }
    static FrontierOrigin thread_entry(std::int32_t uid, std::uint32_t entry) {
        return {FrontierOriginKind::ThreadEntry,0,0,uid,entry};
    }
};
FrontierProof validate_frontier(const psprecomp::GuestMemory &memory,
    const std::vector<psprecomp::ExecutableRange> &ranges,
    const std::map<std::uint32_t, std::string> &seeds,
    const std::set<std::uint32_t> &owned,
    const std::set<std::uint32_t> &imports,
    std::uint32_t target, FrontierOrigin origin);
inline FrontierProof validate_frontier(const psprecomp::GuestMemory &memory,
    const std::vector<psprecomp::ExecutableRange> &ranges,
    const std::map<std::uint32_t, std::string> &seeds,
    const std::set<std::uint32_t> &owned,
    const std::set<std::uint32_t> &imports,
    std::uint32_t target, std::uint32_t caller, std::uint32_t word) {
    return validate_frontier(memory,ranges,seeds,owned,imports,target,
        FrontierOrigin::direct_jal(caller,word));
}
}
