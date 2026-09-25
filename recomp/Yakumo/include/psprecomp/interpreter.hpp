#pragma once

#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

namespace psprecomp {

struct AllegrexContext;
class Runtime;

// Last-resort execution for guest addresses the AOT corpus does not cover.
//
// A statically recompiled corpus only contains the code that was visible when
// it was generated.  A title that swaps overlays into a guest address window at
// run time will eventually jump into code no generated function claims, and the
// outer dispatcher has nothing to call.  Interpreting that code is slow but it
// keeps the guest running, and the report below says which addresses are worth
// adding to the corpus.
enum class InterpreterExit {
    // ctx.pc reached an address the runtime can dispatch normally: a registered
    // function, an import stub, or an address a hook has just made valid.
    Dispatch,
    // The instruction budget ran out.  ctx.pc is the next instruction; the
    // caller regains control so scheduling and preemption still happen.
    Budget,
    // Runtime::stop() was called with a diagnostic.
    Stopped,
    // The entry address holds no readable guest instruction, so the caller
    // keeps its own diagnostic instead.
    Unreachable,
};

struct InterpreterStats {
    std::uint64_t instructions{};
    std::uint64_t entries{};
    std::size_t distinct_entry_addresses{};
};

// Executes instructions from ctx.pc until control leaves the interpreted region
// or `instruction_budget` instructions have run.  Passing zero uses the
// configured default.
InterpreterExit interpret_allegrex(Runtime &runtime, AllegrexContext &ctx,
                                   std::uint64_t instruction_budget = 0u);

// Defaults to enabled; PSPRECOMP_NO_INTERPRETER=1 restores the strict
// "No recompiled function registered" stop.
[[nodiscard]] bool interpreter_fallback_enabled() noexcept;
void set_interpreter_fallback_enabled(bool enabled) noexcept;
// PSPRECOMP_INTERPRETER_BUDGET overrides the per-entry instruction budget.
[[nodiscard]] std::uint64_t interpreter_instruction_budget() noexcept;

[[nodiscard]] InterpreterStats interpreter_stats() noexcept;
// Entry address -> interpreted instructions, most expensive first.  A profile
// uses this to decide which guest addresses deserve a recompiled corpus.
[[nodiscard]] std::vector<std::pair<std::uint32_t, std::uint64_t>> interpreter_entry_profile();
void report_interpreter_stats(std::size_t limit = 20u);
void reset_interpreter_stats() noexcept;

} // namespace psprecomp
