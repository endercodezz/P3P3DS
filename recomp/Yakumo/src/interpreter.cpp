#include "psprecomp/interpreter.hpp"

#include "psprecomp/allegrex_context.hpp"
#include "psprecomp/common.hpp"
#include "psprecomp/decoder.hpp"
#include "psprecomp/guest_memory.hpp"
#include "psprecomp/runtime.hpp"

#include <algorithm>
#include <bit>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <unordered_map>

namespace psprecomp {

namespace {

bool g_settings_loaded = false;
bool g_enabled = true;
bool g_verbose = false;
std::uint64_t g_budget = 1024u;
std::uint64_t g_instructions = 0u;
std::uint64_t g_entries = 0u;
std::size_t g_announced = 0u;
// Bounded so a title that interprets thousands of distinct overlay addresses
// cannot turn the diagnostic into a memory leak.
constexpr std::size_t kMaxProfiledEntries = 4096u;
std::unordered_map<std::uint32_t, std::uint64_t> g_entry_profile;

void load_settings() {
    if (g_settings_loaded) return;
    g_settings_loaded = true;
    g_enabled = std::getenv("PSPRECOMP_NO_INTERPRETER") == nullptr;
    g_verbose = std::getenv("PSPRECOMP_INTERPRETER_VERBOSE") != nullptr;
    if (const char *text = std::getenv("PSPRECOMP_INTERPRETER_BUDGET")) {
        char *end = nullptr;
        const unsigned long long value = std::strtoull(text, &end, 0);
        if (end != text && *end == '\0' && value != 0ull)
            g_budget = static_cast<std::uint64_t>(value);
    }
}

void note_entry(std::uint32_t pc) {
    ++g_entries;
    if (g_entry_profile.contains(pc) || g_entry_profile.size() >= kMaxProfiledEntries) return;
    g_entry_profile.emplace(pc, 0u);
    // Announce the first fallback unconditionally: an unexpected one is the
    // difference between "the corpus is incomplete" and "the game is slow".
    if (g_announced == 0u || g_verbose) {
        std::cerr << "[interpreter] no recompiled function at " << hex32(pc)
                  << "; interpreting guest code instead\n";
    }
    ++g_announced;
}

void charge_entry(std::uint32_t pc, std::uint64_t instructions) {
    const auto found = g_entry_profile.find(pc);
    if (found != g_entry_profile.end()) found->second += instructions;
}

[[nodiscard]] std::uint32_t bit_reverse32(std::uint32_t value) noexcept {
    value = ((value >> 1u) & 0x55555555u) | ((value & 0x55555555u) << 1u);
    value = ((value >> 2u) & 0x33333333u) | ((value & 0x33333333u) << 2u);
    value = ((value >> 4u) & 0x0F0F0F0Fu) | ((value & 0x0F0F0F0Fu) << 4u);
    value = ((value >> 8u) & 0x00FF00FFu) | ((value & 0x00FF00FFu) << 8u);
    return (value >> 16u) | (value << 16u);
}

// VFPU operand fields. The decoder keeps the raw word, and every vector form
// packs destination/source/target and the two-bit size code the same way.
[[nodiscard]] constexpr std::uint32_t vfpu_length(std::uint32_t word) noexcept {
    return (((word >> 7u) & 1u) | (((word >> 15u) & 1u) << 1u)) + 1u;
}
[[nodiscard]] constexpr std::uint32_t vfpu_vd(std::uint32_t word) noexcept { return word & 0x7Fu; }
[[nodiscard]] constexpr std::uint32_t vfpu_vs(std::uint32_t word) noexcept { return (word >> 8u) & 0x7Fu; }
[[nodiscard]] constexpr std::uint32_t vfpu_vt(std::uint32_t word) noexcept { return (word >> 16u) & 0x7Fu; }

// VFIM expands a binary16 literal. This differs from the VH2F conversion in
// AllegrexContext: the immediate form shifts a NaN/infinity payload into the
// binary32 mantissa instead of preserving it in the low ten bits.
[[nodiscard]] std::uint32_t expand_vfim_half(std::uint32_t half) noexcept {
    const std::uint32_t sign = (half & 0x8000u) << 16u;
    const std::uint32_t exponent = (half >> 10u) & 0x1Fu;
    std::uint32_t mantissa = half & 0x03FFu;
    if (exponent == 0u) {
        if (mantissa == 0u) return sign;
        std::uint32_t shift = 0u;
        while ((mantissa & 0x0400u) == 0u) { mantissa <<= 1u; ++shift; }
        mantissa &= 0x03FFu;
        return sign | ((113u - shift) << 23u) | (mantissa << 13u);
    }
    if (exponent == 31u) return sign | 0x7F800000u | (mantissa << 13u);
    return sign | ((exponent + 112u) << 23u) | (mantissa << 13u);
}

[[nodiscard]] float vfpu_unary_value(std::uint32_t operation, float value) noexcept {
    constexpr float half_pi = 1.57079632679489661923f;
    constexpr float two_over_pi = 0.63661977236758134308f;
    switch (operation) {
    case 0u: return value;                                             // vmov
    case 1u: return std::fabs(value);                                  // vabs
    case 2u: return -value;                                            // vneg
    case 4u: return value <= 0.0f ? 0.0f : (value > 1.0f ? 1.0f : value); // vsat0
    case 5u: return value < -1.0f ? -1.0f : (value > 1.0f ? 1.0f : value); // vsat1
    case 16u: return 1.0f / value;                                     // vrcp
    case 17u: return 1.0f / std::sqrt(value);                          // vrsq
    case 18u: return std::sin(value * half_pi);                        // vsin
    case 19u: return std::cos(value * half_pi);                        // vcos
    case 20u: return std::exp2(value);                                 // vexp2
    case 21u: return std::log2(value);                                 // vlog2
    case 22u: return std::fabs(std::sqrt(value));                      // vsqrt
    case 23u: return std::asin(value) * two_over_pi;                   // vasin
    case 24u: return -1.0f / value;                                    // vnrcp
    case 26u: return -std::sin(value * half_pi);                       // vnsin
    default: return 1.0f / std::exp2(value);                           // vrexp2
    }
}

[[nodiscard]] bool evaluate_fpu_compare(const AllegrexContext &ctx, const DecodedInstruction &d) noexcept {
    const float fs = ctx.fpr[d.rd];
    const float ft = ctx.fpr[d.rt];
    const bool unordered = std::isnan(fs) || std::isnan(ft);
    switch (d.word & 0xFu) {
    case 0u: case 8u: return false;
    case 1u: case 9u: return unordered;
    case 2u: case 10u: return !unordered && fs == ft;
    case 3u: case 11u: return unordered || fs == ft;
    case 4u: case 12u: return fs < ft;
    case 5u: case 13u: return unordered || fs < ft;
    case 6u: case 14u: return fs <= ft;
    default: return unordered || fs <= ft;
    }
}

[[nodiscard]] bool is_likely_branch(OpcodeKind kind) noexcept {
    switch (kind) {
    case OpcodeKind::Beql: case OpcodeKind::Bnel: case OpcodeKind::Blezl: case OpcodeKind::Bgtzl:
    case OpcodeKind::Bltzl: case OpcodeKind::Bgezl: case OpcodeKind::Bltzall: case OpcodeKind::Bgezall:
    case OpcodeKind::Bc1fl: case OpcodeKind::Bc1tl: case OpcodeKind::Bvfl: case OpcodeKind::Bvtl:
        return true;
    default:
        return false;
    }
}

[[nodiscard]] bool is_link_branch(OpcodeKind kind) noexcept {
    switch (kind) {
    case OpcodeKind::Bltzal: case OpcodeKind::Bgezal:
    case OpcodeKind::Bltzall: case OpcodeKind::Bgezall:
        return true;
    default:
        return false;
    }
}

[[nodiscard]] bool evaluate_branch(const AllegrexContext &ctx, const DecodedInstruction &d) noexcept {
    const auto signed_rs = static_cast<std::int32_t>(ctx.gpr[d.rs]);
    switch (d.kind) {
    case OpcodeKind::Beq: case OpcodeKind::Beql: return ctx.gpr[d.rs] == ctx.gpr[d.rt];
    case OpcodeKind::Bne: case OpcodeKind::Bnel: return ctx.gpr[d.rs] != ctx.gpr[d.rt];
    case OpcodeKind::Blez: case OpcodeKind::Blezl: return signed_rs <= 0;
    case OpcodeKind::Bgtz: case OpcodeKind::Bgtzl: return signed_rs > 0;
    case OpcodeKind::Bltz: case OpcodeKind::Bltzl:
    case OpcodeKind::Bltzal: case OpcodeKind::Bltzall: return signed_rs < 0;
    case OpcodeKind::Bgez: case OpcodeKind::Bgezl:
    case OpcodeKind::Bgezal: case OpcodeKind::Bgezall: return signed_rs >= 0;
    case OpcodeKind::Bc1f: case OpcodeKind::Bc1fl: return !ctx.fpu_condition();
    case OpcodeKind::Bc1t: case OpcodeKind::Bc1tl: return ctx.fpu_condition();
    case OpcodeKind::Bvf: case OpcodeKind::Bvfl:
        return ((ctx.vfpu_ctrl[3] >> ((d.word >> 18u) & 7u)) & 1u) == 0u;
    case OpcodeKind::Bvt: case OpcodeKind::Bvtl:
        return ((ctx.vfpu_ctrl[3] >> ((d.word >> 18u) & 7u)) & 1u) != 0u;
    default: return false;
    }
}

// Executes one instruction that does not transfer control. Returns false when
// the runtime stopped or an unsupported-instruction hook asked for a retry; the
// interpreter then hands control back to the outer dispatcher.
bool execute_simple(Runtime &rt, AllegrexContext &ctx, const DecodedInstruction &d, std::uint32_t pc) {
    const auto imm = static_cast<std::uint32_t>(static_cast<std::int32_t>(d.immediate));
    const auto uimm = static_cast<std::uint32_t>(static_cast<std::uint16_t>(d.immediate));
    GuestMemory &memory = rt.memory();
    const std::uint32_t rs = ctx.gpr[d.rs];
    const std::uint32_t rt_value = ctx.gpr[d.rt];

    switch (d.kind) {
    case OpcodeKind::Nop: break;
    // PSP CACHE is a hint against a cache the host mapping does not have.
    case OpcodeKind::Cache: break;
    case OpcodeKind::Sync: memory.memory_barrier(); break;

    case OpcodeKind::Addiu: ctx.set_gpr(d.rt, rs + imm); break;
    case OpcodeKind::Slti:
        ctx.set_gpr(d.rt, static_cast<std::int32_t>(rs) < static_cast<std::int32_t>(imm) ? 1u : 0u);
        break;
    case OpcodeKind::Sltiu: ctx.set_gpr(d.rt, rs < imm ? 1u : 0u); break;
    case OpcodeKind::Andi: ctx.set_gpr(d.rt, rs & uimm); break;
    case OpcodeKind::Ori: ctx.set_gpr(d.rt, rs | uimm); break;
    case OpcodeKind::Xori: ctx.set_gpr(d.rt, rs ^ uimm); break;
    case OpcodeKind::Lui: ctx.set_gpr(d.rt, uimm << 16u); break;

    case OpcodeKind::Add:
        if (!ctx.execute_signed_add(d.rd, d.rs, d.rt)) {
            ctx.pc = pc;
            rt.arithmetic_overflow(pc, d.word);
            return false;
        }
        break;
    case OpcodeKind::Sub:
        if (!ctx.execute_signed_sub(d.rd, d.rs, d.rt)) {
            ctx.pc = pc;
            rt.arithmetic_overflow(pc, d.word);
            return false;
        }
        break;
    case OpcodeKind::Addu: ctx.set_gpr(d.rd, rs + rt_value); break;
    case OpcodeKind::Subu: ctx.set_gpr(d.rd, rs - rt_value); break;
    case OpcodeKind::And: ctx.set_gpr(d.rd, rs & rt_value); break;
    case OpcodeKind::Or: ctx.set_gpr(d.rd, rs | rt_value); break;
    case OpcodeKind::Xor: ctx.set_gpr(d.rd, rs ^ rt_value); break;
    case OpcodeKind::Nor: ctx.set_gpr(d.rd, ~(rs | rt_value)); break;
    case OpcodeKind::Slt:
        ctx.set_gpr(d.rd, static_cast<std::int32_t>(rs) < static_cast<std::int32_t>(rt_value) ? 1u : 0u);
        break;
    case OpcodeKind::Sltu: ctx.set_gpr(d.rd, rs < rt_value ? 1u : 0u); break;
    case OpcodeKind::Max:
        ctx.set_gpr(d.rd, static_cast<std::int32_t>(rs) > static_cast<std::int32_t>(rt_value) ? rs : rt_value);
        break;
    case OpcodeKind::Min:
        ctx.set_gpr(d.rd, static_cast<std::int32_t>(rs) < static_cast<std::int32_t>(rt_value) ? rs : rt_value);
        break;
    case OpcodeKind::Movz: if (rt_value == 0u) ctx.set_gpr(d.rd, rs); break;
    case OpcodeKind::Movn: if (rt_value != 0u) ctx.set_gpr(d.rd, rs); break;

    case OpcodeKind::Sll: ctx.set_gpr(d.rd, rt_value << d.sa); break;
    case OpcodeKind::Srl: ctx.set_gpr(d.rd, rt_value >> d.sa); break;
    case OpcodeKind::Sra:
        ctx.set_gpr(d.rd, static_cast<std::uint32_t>(static_cast<std::int32_t>(rt_value) >> d.sa));
        break;
    case OpcodeKind::Rotr: ctx.set_gpr(d.rd, std::rotr(rt_value, static_cast<int>(d.sa))); break;
    case OpcodeKind::Sllv: ctx.set_gpr(d.rd, rt_value << (rs & 31u)); break;
    case OpcodeKind::Srlv: ctx.set_gpr(d.rd, rt_value >> (rs & 31u)); break;
    case OpcodeKind::Srav:
        ctx.set_gpr(d.rd, static_cast<std::uint32_t>(static_cast<std::int32_t>(rt_value) >> (rs & 31u)));
        break;
    case OpcodeKind::Rotrv: ctx.set_gpr(d.rd, std::rotr(rt_value, static_cast<int>(rs & 31u))); break;
    case OpcodeKind::Clz: ctx.set_gpr(d.rd, static_cast<std::uint32_t>(std::countl_zero(rs))); break;
    case OpcodeKind::Clo: ctx.set_gpr(d.rd, static_cast<std::uint32_t>(std::countl_one(rs))); break;
    case OpcodeKind::Ext: {
        const std::uint32_t size = d.rd + 1u;
        const std::uint32_t mask = size >= 32u ? 0xFFFFFFFFu : ((1u << size) - 1u);
        ctx.set_gpr(d.rt, (rs >> d.sa) & mask);
        break;
    }
    case OpcodeKind::Ins: {
        const std::uint32_t size = d.rd >= d.sa ? d.rd - d.sa + 1u : 0u;
        const std::uint32_t source_mask = size >= 32u ? 0xFFFFFFFFu : (size == 0u ? 0u : ((1u << size) - 1u));
        const std::uint32_t destination_mask = source_mask << d.sa;
        ctx.set_gpr(d.rt, (rt_value & ~destination_mask) | ((rs & source_mask) << d.sa));
        break;
    }
    case OpcodeKind::Seb:
        ctx.set_gpr(d.rd, static_cast<std::uint32_t>(
            static_cast<std::int32_t>(static_cast<std::int8_t>(rt_value))));
        break;
    case OpcodeKind::Seh:
        ctx.set_gpr(d.rd, static_cast<std::uint32_t>(
            static_cast<std::int32_t>(static_cast<std::int16_t>(rt_value))));
        break;
    case OpcodeKind::Bitrev: ctx.set_gpr(d.rd, bit_reverse32(rt_value)); break;
    case OpcodeKind::Wsbh:
        ctx.set_gpr(d.rd, ((rt_value & 0x00FF00FFu) << 8u) | ((rt_value & 0xFF00FF00u) >> 8u));
        break;
    case OpcodeKind::Wsbw:
        ctx.set_gpr(d.rd, ((rt_value & 0x000000FFu) << 24u) | ((rt_value & 0x0000FF00u) << 8u) |
                          ((rt_value & 0x00FF0000u) >> 8u) | ((rt_value & 0xFF000000u) >> 24u));
        break;

    case OpcodeKind::Lw: ctx.set_gpr(d.rt, memory.aot_load32(rs + imm)); break;
    case OpcodeKind::Lwl: ctx.set_gpr(d.rt, memory.aot_load_word_left(rs + imm, rt_value)); break;
    case OpcodeKind::Lwr: ctx.set_gpr(d.rt, memory.aot_load_word_right(rs + imm, rt_value)); break;
    case OpcodeKind::Sw: memory.aot_store32(rs + imm, rt_value); break;
    case OpcodeKind::Swl: memory.aot_store_word_left(rs + imm, rt_value); break;
    case OpcodeKind::Swr: memory.aot_store_word_right(rs + imm, rt_value); break;
    case OpcodeKind::Lh:
        ctx.set_gpr(d.rt, static_cast<std::uint32_t>(static_cast<std::int32_t>(
            static_cast<std::int16_t>(memory.aot_load16(rs + imm)))));
        break;
    case OpcodeKind::Lhu: ctx.set_gpr(d.rt, memory.aot_load16(rs + imm)); break;
    case OpcodeKind::Sh: memory.aot_store16(rs + imm, static_cast<std::uint16_t>(rt_value)); break;
    case OpcodeKind::Lb:
        ctx.set_gpr(d.rt, static_cast<std::uint32_t>(static_cast<std::int32_t>(
            static_cast<std::int8_t>(memory.aot_load8(rs + imm)))));
        break;
    case OpcodeKind::Lbu: ctx.set_gpr(d.rt, memory.aot_load8(rs + imm)); break;
    case OpcodeKind::Sb: memory.aot_store8(rs + imm, static_cast<std::uint8_t>(rt_value)); break;
    case OpcodeKind::Lwc1: ctx.fpr[d.rt] = std::bit_cast<float>(memory.aot_load32(rs + imm)); break;
    case OpcodeKind::Swc1: memory.aot_store32(rs + imm, std::bit_cast<std::uint32_t>(ctx.fpr[d.rt])); break;

    case OpcodeKind::Mfhi: ctx.set_gpr(d.rd, ctx.hi); break;
    case OpcodeKind::Mflo: ctx.set_gpr(d.rd, ctx.lo); break;
    case OpcodeKind::Mthi: ctx.hi = rs; break;
    case OpcodeKind::Mtlo: ctx.lo = rs; break;
    case OpcodeKind::Mult: {
        const std::int64_t product = static_cast<std::int64_t>(static_cast<std::int32_t>(rs)) *
                                     static_cast<std::int64_t>(static_cast<std::int32_t>(rt_value));
        ctx.lo = static_cast<std::uint32_t>(static_cast<std::uint64_t>(product));
        ctx.hi = static_cast<std::uint32_t>(static_cast<std::uint64_t>(product) >> 32u);
        break;
    }
    case OpcodeKind::Multu: {
        const std::uint64_t product = static_cast<std::uint64_t>(rs) * static_cast<std::uint64_t>(rt_value);
        ctx.lo = static_cast<std::uint32_t>(product);
        ctx.hi = static_cast<std::uint32_t>(product >> 32u);
        break;
    }
    case OpcodeKind::Madd:
    case OpcodeKind::Msub: {
        const std::int64_t product = static_cast<std::int64_t>(static_cast<std::int32_t>(rs)) *
                                     static_cast<std::int64_t>(static_cast<std::int32_t>(rt_value));
        const std::uint64_t accumulator = (static_cast<std::uint64_t>(ctx.hi) << 32u) | ctx.lo;
        const std::uint64_t result = d.kind == OpcodeKind::Madd
            ? accumulator + static_cast<std::uint64_t>(product)
            : accumulator - static_cast<std::uint64_t>(product);
        ctx.lo = static_cast<std::uint32_t>(result);
        ctx.hi = static_cast<std::uint32_t>(result >> 32u);
        break;
    }
    case OpcodeKind::Maddu:
    case OpcodeKind::Msubu: {
        const std::uint64_t product = static_cast<std::uint64_t>(rs) * static_cast<std::uint64_t>(rt_value);
        const std::uint64_t accumulator = (static_cast<std::uint64_t>(ctx.hi) << 32u) | ctx.lo;
        const std::uint64_t result = d.kind == OpcodeKind::Maddu ? accumulator + product
                                                                 : accumulator - product;
        ctx.lo = static_cast<std::uint32_t>(result);
        ctx.hi = static_cast<std::uint32_t>(result >> 32u);
        break;
    }
    case OpcodeKind::Div: {
        const auto dividend = static_cast<std::int32_t>(rs);
        const auto divisor = static_cast<std::int32_t>(rt_value);
        if (divisor == 0) {
            ctx.lo = dividend >= 0 ? 0xFFFFFFFFu : 1u;
            ctx.hi = static_cast<std::uint32_t>(dividend);
        } else if (dividend == std::numeric_limits<std::int32_t>::min() && divisor == -1) {
            ctx.lo = 0x80000000u;
            ctx.hi = 0u;
        } else {
            ctx.lo = static_cast<std::uint32_t>(dividend / divisor);
            ctx.hi = static_cast<std::uint32_t>(dividend % divisor);
        }
        break;
    }
    case OpcodeKind::Divu:
        if (rt_value == 0u) {
            ctx.lo = 0xFFFFFFFFu;
            ctx.hi = rs;
        } else {
            ctx.lo = rs / rt_value;
            ctx.hi = rs % rt_value;
        }
        break;

    case OpcodeKind::Mfc1: ctx.set_gpr(d.rt, std::bit_cast<std::uint32_t>(ctx.fpr[d.rd])); break;
    case OpcodeKind::Mtc1: ctx.fpr[d.rd] = std::bit_cast<float>(rt_value); break;
    case OpcodeKind::Cfc1:
        if (d.rd != 31u) {
            ctx.pc = pc;
            rt.unsupported(pc, d.word, "unsupported CFC1 control register");
            return false;
        }
        ctx.set_gpr(d.rt, ctx.fcr31);
        break;
    case OpcodeKind::Ctc1:
        if (d.rd != 31u) {
            ctx.pc = pc;
            rt.unsupported(pc, d.word, "unsupported CTC1 control register");
            return false;
        }
        ctx.fcr31 = rt_value & 0x0181FFFFu;
        break;
    case OpcodeKind::AddS: ctx.fpr[d.sa] = ctx.fpr[d.rd] + ctx.fpr[d.rt]; break;
    case OpcodeKind::SubS: ctx.fpr[d.sa] = ctx.fpr[d.rd] - ctx.fpr[d.rt]; break;
    case OpcodeKind::MulS: {
        const float fs = ctx.fpr[d.rd];
        const float ft = ctx.fpr[d.rt];
        // Allegrex produces a quiet NaN for inf * 0 rather than the host result.
        if ((std::isinf(fs) && ft == 0.0f) || (std::isinf(ft) && fs == 0.0f))
            ctx.set_fpr_bits(d.sa, 0x7FC00000u);
        else ctx.fpr[d.sa] = fs * ft;
        break;
    }
    case OpcodeKind::DivS: ctx.fpr[d.sa] = ctx.fpr[d.rd] / ctx.fpr[d.rt]; break;
    case OpcodeKind::SqrtS: ctx.fpr[d.sa] = std::sqrt(ctx.fpr[d.rd]); break;
    case OpcodeKind::AbsS: ctx.set_fpr_bits(d.sa, ctx.fpr_bits(d.rd) & 0x7FFFFFFFu); break;
    case OpcodeKind::MovS: ctx.set_fpr_bits(d.sa, ctx.fpr_bits(d.rd)); break;
    case OpcodeKind::NegS: ctx.set_fpr_bits(d.sa, ctx.fpr_bits(d.rd) ^ 0x80000000u); break;
    case OpcodeKind::RoundWS: ctx.set_fpr_bits(d.sa, ctx.fpu_float_to_word(ctx.fpr[d.rd], 0u)); break;
    case OpcodeKind::TruncWS: ctx.set_fpr_bits(d.sa, ctx.fpu_float_to_word(ctx.fpr[d.rd], 1u)); break;
    case OpcodeKind::CeilWS: ctx.set_fpr_bits(d.sa, ctx.fpu_float_to_word(ctx.fpr[d.rd], 2u)); break;
    case OpcodeKind::FloorWS: ctx.set_fpr_bits(d.sa, ctx.fpu_float_to_word(ctx.fpr[d.rd], 3u)); break;
    case OpcodeKind::CvtWS: ctx.set_fpr_bits(d.sa, ctx.fpu_float_to_word(ctx.fpr[d.rd], 4u)); break;
    case OpcodeKind::CvtSW:
        ctx.fpr[d.sa] = static_cast<float>(static_cast<std::int32_t>(ctx.fpr_bits(d.rd)));
        break;
    case OpcodeKind::FpuCompare: ctx.set_fpu_condition(evaluate_fpu_compare(ctx, d)); break;

    case OpcodeKind::Vflush:
        // The 0xFFFF0000 encoding retains the prefixes; the others consume them.
        if ((d.word & 0xFFFF0000u) != 0xFFFF0000u) ctx.eat_vfpu_prefixes();
        break;
    case OpcodeKind::Vpfx: {
        const std::uint32_t control = (d.word >> 24u) & 3u;
        std::uint32_t data = d.word & 0x000FFFFFu;
        if (control == 2u) data &= 0x00000FFFu;
        ctx.vfpu_ctrl[control] = data;
        break;
    }
    case OpcodeKind::Viim: {
        const float value[1]{static_cast<float>(static_cast<std::int16_t>(d.word & 0xFFFFu))};
        ctx.write_vfpu_vector_with_destination_prefix(value, (d.word >> 16u) & 0x7Fu, 1u);
        break;
    }
    case OpcodeKind::Vfim: {
        const float value[1]{std::bit_cast<float>(expand_vfim_half(d.word & 0xFFFFu))};
        ctx.write_vfpu_vector_with_destination_prefix(value, (d.word >> 16u) & 0x7Fu, 1u);
        break;
    }
    case OpcodeKind::Vf2h:
        ctx.execute_vfpu_vf2h(vfpu_vd(d.word), vfpu_vs(d.word), vfpu_length(d.word));
        break;
    case OpcodeKind::Vh2f:
        ctx.execute_vfpu_vh2f(vfpu_vd(d.word), vfpu_vs(d.word), vfpu_length(d.word));
        break;
    case OpcodeKind::Vx2i:
        ctx.execute_vfpu_vx2i(vfpu_vd(d.word), vfpu_vs(d.word), vfpu_length(d.word),
                              (d.word >> 16u) & 3u);
        break;
    case OpcodeKind::Vf2i: {
        const std::uint32_t length = vfpu_length(d.word);
        const std::uint32_t destination = vfpu_vd(d.word);
        const std::uint32_t scale_exponent = (d.word >> 16u) & 31u;
        const std::uint32_t mode = (d.word >> 21u) & 31u;
        float source[4]{};
        std::int32_t result[4]{};
        ctx.read_vfpu_vector(source, vfpu_vs(d.word), length);
        ctx.apply_vfpu_source_prefix(source, length, 0u);
        const float scale = std::ldexp(1.0f, static_cast<int>(scale_exponent));
        for (std::uint32_t lane = 0u; lane < length; ++lane) {
            if (std::isnan(source[lane])) {
                result[lane] = std::numeric_limits<std::int32_t>::max();
                continue;
            }
            const auto scaled = static_cast<double>(source[lane] * scale);
            if (scaled > static_cast<double>(std::numeric_limits<std::int32_t>::max())) {
                result[lane] = std::numeric_limits<std::int32_t>::max();
            } else if (scaled <= static_cast<double>(std::numeric_limits<std::int32_t>::min())) {
                result[lane] = std::numeric_limits<std::int32_t>::min();
            } else {
                double rounded = 0.0;
                switch (mode) {
                case 16u: rounded = AllegrexContext::round_ties_to_even(scaled); break;
                case 17u: rounded = std::trunc(scaled); break;
                case 18u: rounded = std::ceil(scaled); break;
                default: rounded = std::floor(scaled); break;
                }
                result[lane] = static_cast<std::int32_t>(rounded);
            }
        }
        const std::uint32_t destination_prefix = ctx.vfpu_ctrl[2];
        for (std::uint32_t lane = 0u; lane < length; ++lane) {
            if (((destination_prefix >> (8u + lane)) & 1u) != 0u) continue;
            ctx.vfpu[AllegrexContext::vfpu_vector_lane_index(destination, length, lane)] =
                std::bit_cast<float>(static_cast<std::uint32_t>(result[lane]));
        }
        ctx.eat_vfpu_prefixes();
        break;
    }
    case OpcodeKind::Vi2f: {
        const std::uint32_t length = vfpu_length(d.word);
        float source[4]{};
        float result[4]{};
        ctx.read_vfpu_vector(source, vfpu_vs(d.word), length);
        ctx.apply_vfpu_source_prefix(source, length, 0u);
        const float scale = std::ldexp(1.0f, -static_cast<int>((d.word >> 16u) & 31u));
        for (std::uint32_t lane = 0u; lane < length; ++lane) {
            const auto integer = static_cast<std::int32_t>(std::bit_cast<std::uint32_t>(source[lane]));
            result[lane] = static_cast<float>(integer) * scale;
        }
        ctx.write_vfpu_vector_with_destination_prefix(result, vfpu_vd(d.word), length);
        break;
    }
    case OpcodeKind::Mtv: ctx.set_vfpu_scalar_bits(d.word & 0xFFu, rt_value); break;
    case OpcodeKind::Mfv: ctx.set_gpr(d.rt, ctx.vfpu_scalar_bits(d.word & 0xFFu)); break;
    case OpcodeKind::VmidT:
        ctx.execute_vfpu_matrix_init(vfpu_vd(d.word), vfpu_length(d.word), 3u);
        break;
    case OpcodeKind::VfpuMatrixInit:
        ctx.execute_vfpu_matrix_init(vfpu_vd(d.word), vfpu_length(d.word), (d.word >> 16u) & 15u);
        break;
    case OpcodeKind::Vmmov:
        ctx.execute_vfpu_vmmov(vfpu_vd(d.word), vfpu_vs(d.word), vfpu_length(d.word));
        break;
    case OpcodeKind::Vmmul: {
        const std::uint32_t side = vfpu_length(d.word);
        float source[16]{};
        float target[16]{};
        float result[16]{};
        ctx.read_vfpu_matrix(source, vfpu_vs(d.word), side);
        ctx.read_vfpu_matrix(target, vfpu_vt(d.word), side);
        for (std::uint32_t a = 0u; a < side; ++a) {
            for (std::uint32_t b = 0u; b < side; ++b) {
                float sum = 0.0f;
                for (std::uint32_t c = 0u; c < side; ++c) sum += source[b * 4u + c] * target[a * 4u + c];
                result[a * 4u + b] = sum;
            }
        }
        ctx.write_vfpu_matrix(result, vfpu_vd(d.word), side);
        ctx.eat_vfpu_prefixes();
        break;
    }
    case OpcodeKind::Vmscl:
        ctx.execute_vfpu_vmscl(vfpu_vd(d.word), vfpu_vs(d.word), vfpu_vt(d.word), vfpu_length(d.word));
        break;
    case OpcodeKind::Vtfm: {
        const std::uint32_t side = ((d.word >> 23u) & 3u) + 1u;
        const std::uint32_t input_length = vfpu_length(d.word);
        float matrix[16]{};
        float target_raw[4]{};
        float target[4]{};
        float result[4]{};
        ctx.read_vfpu_matrix(matrix, vfpu_vs(d.word), side);
        ctx.read_vfpu_vector(target_raw, vfpu_vt(d.word), side);
        for (std::uint32_t lane = 0u; lane < 4u; ++lane)
            target[lane] = lane < input_length ? target_raw[lane] : 0.0f;
        // VTFM/VHTFM implies a homogeneous 1 in the lane the encoded input omits.
        if (side - 1u >= input_length) target[side - 1u] = 1.0f;
        for (std::uint32_t row = 0u; row + 1u < side; ++row) {
            float sum = 0.0f;
            for (std::uint32_t column = 0u; column < side; ++column)
                sum += matrix[row * 4u + column] * target[column];
            result[row] = sum;
        }
        float final_row[4]{matrix[(side - 1u) * 4u + 0u], matrix[(side - 1u) * 4u + 1u],
                           matrix[(side - 1u) * 4u + 2u], matrix[(side - 1u) * 4u + 3u]};
        ctx.apply_vfpu_source_prefix(final_row, 4u, 0u);
        ctx.apply_vfpu_source_prefix(target, 4u, 1u);
        for (std::uint32_t column = 0u; column < 4u; ++column)
            result[side - 1u] += final_row[column] * target[column];
        const std::uint32_t destination_prefix = ctx.vfpu_ctrl[2];
        const std::uint32_t last_lane = side - 1u;
        ctx.vfpu_ctrl[2] = ((destination_prefix & (1u << 8u)) << last_lane) |
                           ((destination_prefix & 3u) << (last_lane * 2u));
        ctx.write_vfpu_vector_with_destination_prefix(result, vfpu_vd(d.word), side);
        break;
    }
    case OpcodeKind::Vidt: {
        const std::uint32_t length = vfpu_length(d.word);
        const std::uint32_t destination = vfpu_vd(d.word);
        float value[4]{};
        value[destination & (length >= 3u ? 3u : 1u)] = 1.0f;
        ctx.write_vfpu_vector_with_destination_prefix(value, destination, length);
        break;
    }
    case OpcodeKind::VfpuVectorInit: {
        const bool one = ((d.word >> 16u) & 31u) == 7u;
        const float fill = one ? 1.0f : 0.0f;
        const float value[4]{fill, fill, fill, fill};
        ctx.write_vfpu_vector_with_destination_prefix(value, vfpu_vd(d.word), vfpu_length(d.word));
        break;
    }
    case OpcodeKind::Vcst: {
        static constexpr std::uint32_t constant_bits[32] = {
            0x00000000u, 0x7F7FFFFFu, 0x3FB504F3u, 0x3F3504F3u,
            0x3F906EBAu, 0x3F22F983u, 0x3EA2F983u, 0x3F490FDBu,
            0x3FC90FDBu, 0x40490FDBu, 0x402DF854u, 0x3FB8AA3Bu,
            0x3EDE5BD9u, 0x3F317218u, 0x40135D8Eu, 0x40C90FDBu,
            0x3F060A92u, 0x3E9A209Bu, 0x40549A78u, 0x3F5DB3D7u,
            0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u,
        };
        const float constant = std::bit_cast<float>(constant_bits[(d.word >> 16u) & 31u]);
        const float value[4]{constant, constant, constant, constant};
        ctx.write_vfpu_vector_with_destination_prefix(value, vfpu_vd(d.word), vfpu_length(d.word));
        break;
    }
    case OpcodeKind::Vocp:
        ctx.execute_vfpu_vocp(vfpu_vd(d.word), vfpu_vs(d.word), vfpu_length(d.word));
        break;
    case OpcodeKind::VfpuHorizontal:
        ctx.execute_vfpu_horizontal(vfpu_vd(d.word), vfpu_vs(d.word), vfpu_length(d.word),
                                    ((d.word >> 16u) & 31u) == 7u);
        break;
    case OpcodeKind::Vrot:
        ctx.execute_vfpu_vrot(vfpu_vd(d.word), vfpu_vs(d.word), vfpu_length(d.word),
                              (d.word >> 16u) & 31u);
        break;
    case OpcodeKind::VfpuVec3: {
        const std::uint32_t length = vfpu_length(d.word);
        // VMUL shares opcode 0x19 with the dot/scale group, so it does not use
        // the sub-operation field that opcode 0x18 encodes add/sub/div in.
        const std::uint32_t operation = (d.word >> 26u) == 0x19u ? 2u : ((d.word >> 23u) & 7u);
        float source[4]{};
        float target[4]{};
        float result[4]{};
        ctx.read_vfpu_vector_with_source_prefix(source, vfpu_vs(d.word), length, 0u);
        ctx.read_vfpu_vector_with_source_prefix(target, vfpu_vt(d.word), length, 1u);
        for (std::uint32_t lane = 0u; lane < length; ++lane) {
            result[lane] = operation == 0u ? source[lane] + target[lane]
                         : operation == 1u ? source[lane] - target[lane]
                         : operation == 2u ? source[lane] * target[lane]
                                           : source[lane] / target[lane];
        }
        ctx.write_vfpu_vector_with_destination_prefix(result, vfpu_vd(d.word), length);
        break;
    }
    case OpcodeKind::Vscl:
        ctx.execute_vfpu_vscl(vfpu_vd(d.word), vfpu_vs(d.word), vfpu_vt(d.word), vfpu_length(d.word));
        break;
    case OpcodeKind::Vdot:
        ctx.execute_vfpu_vdot(vfpu_vd(d.word), vfpu_vs(d.word), vfpu_vt(d.word), vfpu_length(d.word));
        break;
    case OpcodeKind::Vhdp:
        ctx.execute_vfpu_vhdp(vfpu_vd(d.word), vfpu_vs(d.word), vfpu_vt(d.word), vfpu_length(d.word));
        break;
    case OpcodeKind::VcrossQuat:
        ctx.execute_vfpu_cross_quat(vfpu_vd(d.word), vfpu_vs(d.word), vfpu_vt(d.word), vfpu_length(d.word));
        break;
    case OpcodeKind::Vminmax:
        ctx.execute_vfpu_vminmax(vfpu_vd(d.word), vfpu_vs(d.word), vfpu_vt(d.word),
                                 vfpu_length(d.word), ((d.word >> 23u) & 7u) == 3u);
        break;
    case OpcodeKind::VfpuCompare3:
        ctx.execute_vfpu_compare3(vfpu_vd(d.word), vfpu_vs(d.word), vfpu_vt(d.word),
                                  vfpu_length(d.word), (d.word >> 23u) & 7u);
        break;
    case OpcodeKind::Vcmp:
        ctx.execute_vfpu_vcmp(vfpu_vs(d.word), vfpu_vt(d.word), vfpu_length(d.word), d.word & 15u);
        break;
    case OpcodeKind::Vcmov:
        ctx.execute_vfpu_vcmov(vfpu_vd(d.word), vfpu_vs(d.word), vfpu_length(d.word),
                               (d.word >> 16u) & 7u, ((d.word >> 19u) & 1u) != 0u);
        break;
    case OpcodeKind::VfpuUnary: {
        const std::uint32_t length = vfpu_length(d.word);
        const std::uint32_t operation = (d.word >> 16u) & 31u;
        float source[4]{};
        float result[4]{};
        ctx.read_vfpu_vector_with_source_prefix(source, vfpu_vs(d.word), length, 0u);
        for (std::uint32_t lane = 0u; lane < length; ++lane)
            result[lane] = vfpu_unary_value(operation, source[lane]);
        ctx.write_vfpu_vector_with_destination_prefix(result, vfpu_vd(d.word), length);
        break;
    }
    case OpcodeKind::Vsgn: {
        const std::uint32_t length = vfpu_length(d.word);
        float source[4]{};
        float result[4]{};
        ctx.read_vfpu_vector_with_source_prefix(source, vfpu_vs(d.word), length, 0u);
        for (std::uint32_t lane = 0u; lane < length; ++lane)
            result[lane] = source[lane] > 0.0f ? 1.0f : (source[lane] < 0.0f ? -1.0f : 0.0f);
        ctx.write_vfpu_vector_with_destination_prefix(result, vfpu_vd(d.word), length);
        break;
    }
    case OpcodeKind::Vsocp: {
        // VSOCP widens each lane x into the saturated pair (1 - x, x), so a
        // triple or quad source has no defined destination on hardware.
        const std::uint32_t length = vfpu_length(d.word);
        if (length > 2u) {
            ctx.pc = pc;
            rt.unsupported(pc, d.word, "vsocp with triple/quad source");
            return false;
        }
        float source[4]{};
        float result[4]{};
        ctx.read_vfpu_vector_with_source_prefix(source, vfpu_vs(d.word), length, 0u);
        for (std::uint32_t lane = 0u; lane < length; ++lane) {
            const float inverse = 1.0f - source[lane];
            result[lane * 2u] = inverse < 0.0f ? 0.0f : (inverse > 1.0f ? 1.0f : inverse);
            result[lane * 2u + 1u] = source[lane] < 0.0f ? 0.0f
                                   : (source[lane] > 1.0f ? 1.0f : source[lane]);
        }
        ctx.write_vfpu_vector_with_destination_prefix(result, vfpu_vd(d.word), length * 2u);
        break;
    }
    case OpcodeKind::Lvs: {
        const auto offset = static_cast<std::uint32_t>(
            static_cast<std::int32_t>(static_cast<std::int16_t>(d.word & 0xFFFCu)));
        const std::uint32_t scalar = ((d.word >> 16u) & 0x1Fu) | ((d.word & 3u) << 5u);
        ctx.set_vfpu_scalar_bits(scalar, memory.aot_load32(rs + offset));
        break;
    }
    case OpcodeKind::Svs: {
        const auto offset = static_cast<std::uint32_t>(
            static_cast<std::int32_t>(static_cast<std::int16_t>(d.word & 0xFFFCu)));
        const std::uint32_t scalar = ((d.word >> 16u) & 0x1Fu) | ((d.word & 3u) << 5u);
        memory.aot_store32(rs + offset, ctx.vfpu_scalar_bits(scalar));
        break;
    }
    case OpcodeKind::Lvq: {
        const auto offset = static_cast<std::uint32_t>(
            static_cast<std::int32_t>(static_cast<std::int16_t>(d.word & 0xFFFCu)));
        const std::uint32_t vector = ((d.word >> 16u) & 0x1Fu) | ((d.word & 1u) << 5u);
        const std::uint32_t address = rs + offset;
        const float value[4]{
            std::bit_cast<float>(memory.aot_load32(address + 0u)),
            std::bit_cast<float>(memory.aot_load32(address + 4u)),
            std::bit_cast<float>(memory.aot_load32(address + 8u)),
            std::bit_cast<float>(memory.aot_load32(address + 12u))};
        ctx.write_vfpu_vector(value, vector, 4u);
        break;
    }
    case OpcodeKind::Svq: {
        const auto offset = static_cast<std::uint32_t>(
            static_cast<std::int32_t>(static_cast<std::int16_t>(d.word & 0xFFFCu)));
        const std::uint32_t vector = ((d.word >> 16u) & 0x1Fu) | ((d.word & 1u) << 5u);
        float value[4]{};
        ctx.read_vfpu_vector(value, vector, 4u);
        const std::uint32_t address = rs + offset;
        for (std::uint32_t lane = 0u; lane < 4u; ++lane)
            memory.aot_store32(address + lane * 4u, std::bit_cast<std::uint32_t>(value[lane]));
        break;
    }

    default:
        ctx.pc = pc;
        rt.unsupported(pc, d.word, d.mnemonic + " not interpreted");
        return false;
    }
    return true;
}

} // namespace

bool interpreter_fallback_enabled() noexcept {
    load_settings();
    return g_enabled;
}

void set_interpreter_fallback_enabled(bool enabled) noexcept {
    load_settings();
    g_enabled = enabled;
}

std::uint64_t interpreter_instruction_budget() noexcept {
    load_settings();
    return g_budget;
}

InterpreterStats interpreter_stats() noexcept {
    return InterpreterStats{g_instructions, g_entries, g_entry_profile.size()};
}

std::vector<std::pair<std::uint32_t, std::uint64_t>> interpreter_entry_profile() {
    std::vector<std::pair<std::uint32_t, std::uint64_t>> entries(g_entry_profile.begin(),
                                                                 g_entry_profile.end());
    std::sort(entries.begin(), entries.end(), [](const auto &left, const auto &right) {
        if (left.second != right.second) return left.second > right.second;
        return left.first < right.first;
    });
    return entries;
}

void reset_interpreter_stats() noexcept {
    g_instructions = 0u;
    g_entries = 0u;
    g_announced = 0u;
    g_entry_profile.clear();
}

void report_interpreter_stats(std::size_t limit) {
    if (g_instructions == 0u) return;
    std::cerr << "[interpreter] instructions=" << g_instructions
              << " entries=" << g_entries
              << " unique_addresses=" << g_entry_profile.size() << "\n";
    const auto entries = interpreter_entry_profile();
    for (std::size_t index = 0u; index < std::min(limit, entries.size()); ++index) {
        std::cerr << "[interpreter] " << hex32(entries[index].first)
                  << " instructions=" << entries[index].second << "\n";
    }
}

InterpreterExit interpret_allegrex(Runtime &rt, AllegrexContext &ctx,
                                   std::uint64_t instruction_budget) {
    load_settings();
    const std::uint64_t budget = instruction_budget != 0u ? instruction_budget : g_budget;
    GuestMemory &memory = rt.memory();

    std::uint32_t pc = ctx.pc;
    if (!memory.contains(pc, 4u)) return InterpreterExit::Unreachable;

    const std::uint32_t entry_pc = pc;
    note_entry(entry_pc);

    std::uint64_t executed = 0u;
    const auto leave = [&](InterpreterExit exit) {
        g_instructions += executed;
        charge_entry(entry_pc, executed);
        return exit;
    };

    while (true) {
        // A registered address may be an ordinary AOT unit, a profile override
        // or a PSP import stub. All three must run through normal dispatch, so
        // interpreted code reaches invoke_import_cached exactly the way
        // generated code does: by leaving with ctx.pc at the stub.
        if (executed != 0u && rt.has_function(pc)) {
            ctx.pc = pc;
            return leave(InterpreterExit::Dispatch);
        }
        if (executed >= budget) {
            ctx.pc = pc;
            return leave(InterpreterExit::Budget);
        }
        if (!memory.contains(pc, 4u)) {
            ctx.pc = pc;
            rt.stop("Interpreter reached unmapped guest address " + hex32(pc));
            return leave(InterpreterExit::Stopped);
        }

        ctx.pc = pc;
        const DecodedInstruction decoded = decode_allegrex(memory.aot_load32(pc));
        ++executed;

        if (!decoded.has_delay_slot()) {
            if (!execute_simple(rt, ctx, decoded, pc))
                return leave(rt.stopped() ? InterpreterExit::Stopped : InterpreterExit::Dispatch);
            pc += 4u;
            continue;
        }

        if (!memory.contains(pc + 4u, 4u)) {
            ctx.pc = pc + 4u;
            rt.stop("Interpreter reached unmapped delay slot at " + hex32(pc + 4u));
            return leave(InterpreterExit::Stopped);
        }
        const DecodedInstruction slot = decode_allegrex(memory.aot_load32(pc + 4u));
        if (slot.is_control_flow()) {
            ctx.pc = pc + 4u;
            rt.unsupported(pc + 4u, slot.word, "control flow in delay slot");
            return leave(rt.stopped() ? InterpreterExit::Stopped : InterpreterExit::Dispatch);
        }

        const std::uint32_t return_pc = pc + 8u;
        std::uint32_t next = return_pc;
        bool run_slot = true;
        switch (decoded.kind) {
        case OpcodeKind::J:
        case OpcodeKind::Jal:
            if (decoded.kind == OpcodeKind::Jal) ctx.set_gpr(31u, return_pc);
            next = ((pc + 4u) & 0xF0000000u) | (decoded.target << 2u);
            break;
        case OpcodeKind::Jr:
        case OpcodeKind::Jalr: {
            // Both operands are read before the delay slot runs, so a slot that
            // overwrites rs or the link register cannot change the transfer.
            const std::uint32_t jump_target = ctx.gpr[decoded.rs];
            if (decoded.kind == OpcodeKind::Jalr)
                ctx.set_gpr(decoded.rd == 0u ? 31u : decoded.rd, return_pc);
            next = jump_target;
            break;
        }
        default: {
            // The condition is evaluated before the link write, because a
            // branch-and-link whose condition register is $ra must test the old
            // value. A likely branch annuls its delay slot when not taken.
            const bool taken = evaluate_branch(ctx, decoded);
            if (is_link_branch(decoded.kind)) ctx.set_gpr(31u, return_pc);
            if (taken) {
                next = pc + 4u + static_cast<std::uint32_t>(
                    static_cast<std::int32_t>(decoded.immediate) * 4);
            }
            run_slot = taken || !is_likely_branch(decoded.kind);
            break;
        }
        }

        if (run_slot) {
            ++executed;
            ctx.pc = pc + 4u;
            if (!execute_simple(rt, ctx, slot, pc + 4u))
                return leave(rt.stopped() ? InterpreterExit::Stopped : InterpreterExit::Dispatch);
        }
        pc = next;
    }
}

} // namespace psprecomp
