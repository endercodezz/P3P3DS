#pragma once
// P3P3DS opt-in frontier diagnostics. No per-instruction event stream.
#include <cstdint>
#include <map>
#include <string>
#include <vector>
namespace psprecomp {
struct ExecutionEvent {
    std::string type;
    std::uint32_t pc{};
    std::int32_t thread{};
    std::map<std::string, std::uint64_t> fields;
    std::string detail;
};
struct ExecutedTransfer {
    std::uint32_t pc{}, word{}, target{};
    std::int32_t thread{-1};
};
// A diagnostic halt after a completed memory write, caught by the PC runner.
struct FrontierHalt {};
// P3P3DS differential tests: Runtime::transfer_budget reached.
struct TransferBudgetHalt {};
}
