#pragma once
// Allegrex interpreter fallback for guest PCs that have no registered AOT entry
// (indirect calls, jump tables, materialized code pointers, interior labels).
//
// Instruction semantics deliberately mirror recomp/PSPRecomp/tools/codegen_main.cpp
// (emit_regular and the branch/jump lowering in emit_function_source), including
// the order of link-register writes, delay-slot execution and condition
// evaluation. tests/test_interpreter_diff.cpp checks that equivalence against
// the generated AOT units instruction-for-instruction.
#include "psprecomp/decoder.hpp"
#include "psprecomp/runtime.hpp"

#include <cstdint>
#include <set>
#include <unordered_map>

namespace p3p3ds {

class Interpreter {
public:
    enum class Step { Continue, Transfer, Stopped };

    // Executes one instruction at ctx.pc. A branch/jump executes its delay slot
    // too and reports Transfer. Unsupported encodings stop the runtime exactly
    // like generated code (Runtime::unsupported).
    Step step(psprecomp::Runtime &rt, psprecomp::AllegrexContext &ctx);

    // Runs from ctx.pc until control reaches a PC with a registered AOT entry,
    // the runtime stops, or `max_instructions` is exhausted. Returns the number
    // of executed instructions (a branch and its delay slot count as two).
    // With `yield_to_aot == false` (differential tests) it never yields to AOT
    // and instead stops before executing any PC in `stop_pcs`.
    std::uint64_t run(psprecomp::Runtime &rt, psprecomp::AllegrexContext &ctx,
                      std::uint64_t max_instructions, bool yield_to_aot = true);
    std::set<std::uint32_t> stop_pcs;

    [[nodiscard]] std::uint64_t executed() const noexcept { return executed_; }
    [[nodiscard]] std::uint64_t entries() const noexcept { return entries_; }
    [[nodiscard]] const std::set<std::uint32_t> &entry_pcs() const noexcept { return entry_pcs_; }
    void note_entry(std::uint32_t pc) { ++entries_; entry_pcs_.insert(pc); }

private:
    struct Cached {
        std::uint32_t word{};
        psprecomp::DecodedInstruction decoded;
    };
    const psprecomp::DecodedInstruction &decode(const psprecomp::GuestMemory &memory, std::uint32_t pc);
    // Returns false when the instruction stopped the runtime.
    bool execute_regular(psprecomp::Runtime &rt, psprecomp::AllegrexContext &ctx,
                         const psprecomp::DecodedInstruction &d, std::uint32_t pc);

    std::unordered_map<std::uint32_t, Cached> cache_;
    std::uint64_t executed_{};
    std::uint64_t entries_{};
    std::set<std::uint32_t> entry_pcs_;
};

// Installs `interpreter` as the Runtime's fallback for unregistered PCs. The
// first entry at each distinct PC is logged as an `interpreter_enter` event.
void install_interpreter_fallback(Interpreter *interpreter,
                                  std::uint64_t max_instructions_per_entry = 1u << 20);

} // namespace p3p3ds
