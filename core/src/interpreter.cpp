#include "p3p3ds/interpreter.hpp"

#include "psprecomp/common.hpp"

#include <bit>
#include <cmath>
#include <limits>

namespace p3p3ds {
namespace {
using psprecomp::AllegrexContext;
using psprecomp::DecodedInstruction;
using psprecomp::Runtime;
using K = psprecomp::OpcodeKind;

std::uint32_t size_code_length(std::uint32_t word) {
    return (((word >> 7u) & 1u) | (((word >> 15u) & 1u) << 1u)) + 1u;
}

bool is_branch(K kind) {
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

bool is_likely(K kind) {
    return kind == K::Beql || kind == K::Bnel || kind == K::Blezl || kind == K::Bgtzl ||
           kind == K::Bltzl || kind == K::Bgezl || kind == K::Bltzall || kind == K::Bgezall ||
           kind == K::Bc1fl || kind == K::Bc1tl || kind == K::Bvfl || kind == K::Bvtl;
}

bool is_link(K kind) {
    return kind == K::Bltzal || kind == K::Bgezal || kind == K::Bltzall || kind == K::Bgezall;
}

bool branch_condition(const AllegrexContext &ctx, const DecodedInstruction &d) {
    const auto s = [&](std::uint32_t r) { return static_cast<std::int32_t>(ctx.gpr[r]); };
    switch (d.kind) {
    case K::Beq: case K::Beql: return ctx.gpr[d.rs] == ctx.gpr[d.rt];
    case K::Bne: case K::Bnel: return ctx.gpr[d.rs] != ctx.gpr[d.rt];
    case K::Blez: case K::Blezl: return s(d.rs) <= 0;
    case K::Bgtz: case K::Bgtzl: return s(d.rs) > 0;
    case K::Bltz: case K::Bltzl: case K::Bltzal: case K::Bltzall: return s(d.rs) < 0;
    case K::Bgez: case K::Bgezl: case K::Bgezal: case K::Bgezall: return s(d.rs) >= 0;
    case K::Bc1f: case K::Bc1fl: return !ctx.fpu_condition();
    case K::Bc1t: case K::Bc1tl: return ctx.fpu_condition();
    case K::Bvf: case K::Bvfl: return ((ctx.vfpu_ctrl[3] >> ((d.word >> 18u) & 7u)) & 1u) == 0u;
    case K::Bvt: case K::Bvtl: return ((ctx.vfpu_ctrl[3] >> ((d.word >> 18u) & 7u)) & 1u) != 0u;
    default: return false;
    }
}

float half_to_float(std::uint32_t half) {
    const std::uint32_t sign = (half & 0x8000u) << 16u;
    const std::uint32_t exponent = (half >> 10u) & 0x1Fu;
    std::uint32_t mantissa = half & 0x03FFu;
    std::uint32_t bits = 0u;
    if (exponent == 0u) {
        if (mantissa == 0u) bits = sign;
        else {
            std::uint32_t shift = 0u;
            while ((mantissa & 0x0400u) == 0u) { mantissa <<= 1u; ++shift; }
            mantissa &= 0x03FFu;
            bits = sign | ((113u - shift) << 23u) | (mantissa << 13u);
        }
    } else if (exponent == 31u) {
        bits = sign | 0x7F800000u | (mantissa << 13u);
    } else {
        bits = sign | ((exponent + 112u) << 23u) | (mantissa << 13u);
    }
    return std::bit_cast<float>(bits);
}

float vfpu_unary(std::uint32_t operation, float v) {
    switch (operation) {
    case 0u: return v;
    case 1u: return std::fabs(v);
    case 2u: return -v;
    case 4u: return v <= 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v);
    case 5u: return v < -1.0f ? -1.0f : (v > 1.0f ? 1.0f : v);
    case 16u: return 1.0f / v;
    case 17u: return 1.0f / std::sqrt(v);
    case 18u: return std::sin(v * 1.57079632679489661923f);
    case 19u: return std::cos(v * 1.57079632679489661923f);
    case 20u: return std::exp2(v);
    case 21u: return std::log2(v);
    case 22u: return std::fabs(std::sqrt(v));
    case 23u: return std::asin(v) * 0.63661977236758134308f;
    case 24u: return -1.0f / v;
    case 26u: return -std::sin(v * 1.57079632679489661923f);
    default: return 1.0f / std::exp2(v);
    }
}

constexpr std::uint32_t kVcstBits[32] = {
    0x00000000u, 0x7F7FFFFFu, 0x3FB504F3u, 0x3F3504F3u,
    0x3F906EBAu, 0x3F22F983u, 0x3EA2F983u, 0x3F490FDBu,
    0x3FC90FDBu, 0x40490FDBu, 0x402DF854u, 0x3FB8AA3Bu,
    0x3EDE5BD9u, 0x3F317218u, 0x40135D8Eu, 0x40C90FDBu,
    0x3F060A92u, 0x3E9A209Bu, 0x40549A78u, 0x3F5DB3D7u,
    0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u,
};

bool is_memory_access(K kind) {
    return kind == K::Sw || kind == K::Swl || kind == K::Swr || kind == K::Sh || kind == K::Sb ||
           kind == K::Swc1 || kind == K::Svs || kind == K::Svq || kind == K::Lw || kind == K::Lh ||
           kind == K::Lhu || kind == K::Lb || kind == K::Lbu || kind == K::Lwl || kind == K::Lwr ||
           kind == K::Lwc1 || kind == K::Lvs || kind == K::Lvq;
}

Interpreter *g_interpreter = nullptr;
std::uint64_t g_max_per_entry = 0u;

bool fallback_hook(Runtime &rt, AllegrexContext &ctx) {
    if (g_interpreter == nullptr) return false;
    const std::uint32_t pc = ctx.pc;
    if (!rt.memory().contains(pc, 4u) || (pc & 3u) != 0u) return false;
    if (!g_interpreter->entry_pcs().contains(pc))
        rt.event("interpreter_enter", {{"target", pc}});
    g_interpreter->note_entry(pc);
    (void)g_interpreter->run(rt, ctx, g_max_per_entry);
    return true;
}
} // namespace

const DecodedInstruction &Interpreter::decode(const psprecomp::GuestMemory &memory, std::uint32_t pc) {
    const std::uint32_t word = memory.load32(pc);
    auto [it, inserted] = cache_.try_emplace(pc);
    if (inserted || it->second.word != word) {
        it->second.word = word;
        it->second.decoded = psprecomp::decode_allegrex(word);
    }
    return it->second.decoded;
}

bool Interpreter::execute_regular(Runtime &rt, AllegrexContext &ctx, const DecodedInstruction &d,
                                  std::uint32_t pc) {
    auto &g = ctx.gpr;
    auto &mem = rt.memory();
    const auto imm = static_cast<std::int32_t>(d.immediate);
    const auto uimm = static_cast<std::uint16_t>(d.immediate);
    const std::uint32_t address = g[d.rs] + static_cast<std::uint32_t>(imm);
    const auto s32 = [](std::uint32_t v) { return static_cast<std::int32_t>(v); };
    if (is_memory_access(d.kind) && rt.frontier_diagnostics) rt.diagnostic_pc = pc;
    ++executed_;
    switch (d.kind) {
    case K::Nop: break;
    case K::Sync: mem.memory_barrier(); break;
    case K::Cache: break;
    case K::Addiu: ctx.set_gpr(d.rt, g[d.rs] + static_cast<std::uint32_t>(imm)); break;
    case K::Slti: ctx.set_gpr(d.rt, s32(g[d.rs]) < imm ? 1u : 0u); break;
    case K::Sltiu: ctx.set_gpr(d.rt, g[d.rs] < static_cast<std::uint32_t>(imm) ? 1u : 0u); break;
    case K::Andi: ctx.set_gpr(d.rt, g[d.rs] & uimm); break;
    case K::Ori: ctx.set_gpr(d.rt, g[d.rs] | uimm); break;
    case K::Xori: ctx.set_gpr(d.rt, g[d.rs] ^ uimm); break;
    case K::Lui: ctx.set_gpr(d.rt, static_cast<std::uint32_t>(uimm) << 16u); break;
    case K::Add:
        if (!ctx.execute_signed_add(d.rd, d.rs, d.rt)) { rt.arithmetic_overflow(pc, d.word); return false; }
        break;
    case K::Addu: ctx.set_gpr(d.rd, g[d.rs] + g[d.rt]); break;
    case K::Sub:
        if (!ctx.execute_signed_sub(d.rd, d.rs, d.rt)) { rt.arithmetic_overflow(pc, d.word); return false; }
        break;
    case K::Subu: ctx.set_gpr(d.rd, g[d.rs] - g[d.rt]); break;
    case K::And: ctx.set_gpr(d.rd, g[d.rs] & g[d.rt]); break;
    case K::Or: ctx.set_gpr(d.rd, g[d.rs] | g[d.rt]); break;
    case K::Xor: ctx.set_gpr(d.rd, g[d.rs] ^ g[d.rt]); break;
    case K::Nor: ctx.set_gpr(d.rd, ~(g[d.rs] | g[d.rt])); break;
    case K::Slt: ctx.set_gpr(d.rd, s32(g[d.rs]) < s32(g[d.rt]) ? 1u : 0u); break;
    case K::Sltu: ctx.set_gpr(d.rd, g[d.rs] < g[d.rt] ? 1u : 0u); break;
    case K::Max: ctx.set_gpr(d.rd, s32(g[d.rs]) > s32(g[d.rt]) ? g[d.rs] : g[d.rt]); break;
    case K::Min: ctx.set_gpr(d.rd, s32(g[d.rs]) < s32(g[d.rt]) ? g[d.rs] : g[d.rt]); break;
    case K::Movz: if (g[d.rt] == 0u) ctx.set_gpr(d.rd, g[d.rs]); break;
    case K::Movn: if (g[d.rt] != 0u) ctx.set_gpr(d.rd, g[d.rs]); break;
    case K::Sll: ctx.set_gpr(d.rd, g[d.rt] << d.sa); break;
    case K::Srl: ctx.set_gpr(d.rd, g[d.rt] >> d.sa); break;
    case K::Sra: ctx.set_gpr(d.rd, static_cast<std::uint32_t>(s32(g[d.rt]) >> d.sa)); break;
    case K::Rotr: ctx.set_gpr(d.rd, std::rotr(g[d.rt], static_cast<int>(d.sa))); break;
    case K::Sllv: ctx.set_gpr(d.rd, g[d.rt] << (g[d.rs] & 31u)); break;
    case K::Srlv: ctx.set_gpr(d.rd, g[d.rt] >> (g[d.rs] & 31u)); break;
    case K::Srav: ctx.set_gpr(d.rd, static_cast<std::uint32_t>(s32(g[d.rt]) >> (g[d.rs] & 31u))); break;
    case K::Rotrv: ctx.set_gpr(d.rd, std::rotr(g[d.rt], static_cast<int>(g[d.rs] & 31u))); break;
    case K::Clz: ctx.set_gpr(d.rd, static_cast<std::uint32_t>(std::countl_zero(g[d.rs]))); break;
    case K::Clo: ctx.set_gpr(d.rd, static_cast<std::uint32_t>(std::countl_one(g[d.rs]))); break;
    case K::Ext: {
        const std::uint32_t size = d.rd + 1u;
        const std::uint32_t mask = size == 32u ? 0xFFFFFFFFu : ((1u << size) - 1u);
        ctx.set_gpr(d.rt, (g[d.rs] >> d.sa) & mask);
        break;
    }
    case K::Ins: {
        const std::uint32_t size = d.rd >= d.sa ? d.rd - d.sa + 1u : 0u;
        const std::uint32_t source_mask = size == 32u ? 0xFFFFFFFFu : (size == 0u ? 0u : ((1u << size) - 1u));
        const std::uint32_t destination_mask = source_mask << d.sa;
        ctx.set_gpr(d.rt, (g[d.rt] & ~destination_mask) | ((g[d.rs] & source_mask) << d.sa));
        break;
    }
    case K::Seb: ctx.set_gpr(d.rd, static_cast<std::uint32_t>(static_cast<std::int32_t>(static_cast<std::int8_t>(g[d.rt])))); break;
    case K::Seh: ctx.set_gpr(d.rd, static_cast<std::uint32_t>(static_cast<std::int32_t>(static_cast<std::int16_t>(g[d.rt])))); break;
    case K::Bitrev: {
        std::uint32_t v = g[d.rt];
        v = ((v >> 1u) & 0x55555555u) | ((v & 0x55555555u) << 1u);
        v = ((v >> 2u) & 0x33333333u) | ((v & 0x33333333u) << 2u);
        v = ((v >> 4u) & 0x0F0F0F0Fu) | ((v & 0x0F0F0F0Fu) << 4u);
        v = ((v >> 8u) & 0x00FF00FFu) | ((v & 0x00FF00FFu) << 8u);
        ctx.set_gpr(d.rd, (v >> 16u) | (v << 16u));
        break;
    }
    case K::Wsbh: ctx.set_gpr(d.rd, ((g[d.rt] & 0x00FF00FFu) << 8u) | ((g[d.rt] & 0xFF00FF00u) >> 8u)); break;
    case K::Wsbw: ctx.set_gpr(d.rd, ((g[d.rt] & 0x000000FFu) << 24u) | ((g[d.rt] & 0x0000FF00u) << 8u) |
                                    ((g[d.rt] & 0x00FF0000u) >> 8u) | ((g[d.rt] & 0xFF000000u) >> 24u)); break;
    case K::Lw: ctx.set_gpr(d.rt, mem.aot_load32(address)); break;
    case K::Lwl: ctx.set_gpr(d.rt, mem.aot_load_word_left(address, g[d.rt])); break;
    case K::Lwr: ctx.set_gpr(d.rt, mem.aot_load_word_right(address, g[d.rt])); break;
    case K::Sw: mem.aot_store32(address, g[d.rt]); break;
    case K::Swl: mem.aot_store_word_left(address, g[d.rt]); break;
    case K::Swr: mem.aot_store_word_right(address, g[d.rt]); break;
    case K::Lh: ctx.set_gpr(d.rt, static_cast<std::uint32_t>(static_cast<std::int32_t>(static_cast<std::int16_t>(mem.aot_load16(address))))); break;
    case K::Lhu: ctx.set_gpr(d.rt, mem.aot_load16(address)); break;
    case K::Sh: mem.aot_store16(address, static_cast<std::uint16_t>(g[d.rt])); break;
    case K::Lb: ctx.set_gpr(d.rt, static_cast<std::uint32_t>(static_cast<std::int32_t>(static_cast<std::int8_t>(mem.aot_load8(address))))); break;
    case K::Lbu: ctx.set_gpr(d.rt, mem.aot_load8(address)); break;
    case K::Sb: mem.aot_store8(address, static_cast<std::uint8_t>(g[d.rt])); break;
    case K::Lwc1: ctx.fpr[d.rt] = std::bit_cast<float>(mem.aot_load32(address)); break;
    case K::Swc1: mem.aot_store32(address, std::bit_cast<std::uint32_t>(ctx.fpr[d.rt])); break;
    case K::Mfhi: ctx.set_gpr(d.rd, ctx.hi); break;
    case K::Mflo: ctx.set_gpr(d.rd, ctx.lo); break;
    case K::Mthi: ctx.hi = g[d.rs]; break;
    case K::Mtlo: ctx.lo = g[d.rs]; break;
    case K::Mult: {
        const std::int64_t p = static_cast<std::int64_t>(s32(g[d.rs])) * static_cast<std::int64_t>(s32(g[d.rt]));
        ctx.lo = static_cast<std::uint32_t>(p); ctx.hi = static_cast<std::uint32_t>(static_cast<std::uint64_t>(p) >> 32u);
        break;
    }
    case K::Multu: {
        const std::uint64_t p = static_cast<std::uint64_t>(g[d.rs]) * static_cast<std::uint64_t>(g[d.rt]);
        ctx.lo = static_cast<std::uint32_t>(p); ctx.hi = static_cast<std::uint32_t>(p >> 32u);
        break;
    }
    case K::Div: {
        const std::int32_t dividend = s32(g[d.rs]);
        const std::int32_t divisor = s32(g[d.rt]);
        if (divisor == 0) { ctx.lo = dividend >= 0 ? 0xFFFFFFFFu : 1u; ctx.hi = static_cast<std::uint32_t>(dividend); }
        else if (dividend == std::numeric_limits<std::int32_t>::min() && divisor == -1) { ctx.lo = 0x80000000u; ctx.hi = 0u; }
        else { ctx.lo = static_cast<std::uint32_t>(dividend / divisor); ctx.hi = static_cast<std::uint32_t>(dividend % divisor); }
        break;
    }
    case K::Divu: {
        const std::uint32_t dividend = g[d.rs], divisor = g[d.rt];
        if (divisor == 0u) { ctx.lo = 0xFFFFFFFFu; ctx.hi = dividend; }
        else { ctx.lo = dividend / divisor; ctx.hi = dividend % divisor; }
        break;
    }
    case K::Madd: case K::Msub: {
        const std::int64_t acc = static_cast<std::int64_t>((static_cast<std::uint64_t>(ctx.hi) << 32u) | ctx.lo);
        const std::int64_t p = static_cast<std::int64_t>(s32(g[d.rs])) * static_cast<std::int64_t>(s32(g[d.rt]));
        const std::int64_t r = d.kind == K::Madd ? acc + p : acc - p;
        ctx.lo = static_cast<std::uint32_t>(r); ctx.hi = static_cast<std::uint32_t>(static_cast<std::uint64_t>(r) >> 32u);
        break;
    }
    case K::Maddu: case K::Msubu: {
        const std::uint64_t acc = (static_cast<std::uint64_t>(ctx.hi) << 32u) | ctx.lo;
        const std::uint64_t p = static_cast<std::uint64_t>(g[d.rs]) * static_cast<std::uint64_t>(g[d.rt]);
        const std::uint64_t r = d.kind == K::Maddu ? acc + p : acc - p;
        ctx.lo = static_cast<std::uint32_t>(r); ctx.hi = static_cast<std::uint32_t>(r >> 32u);
        break;
    }
    case K::Mfc1: ctx.set_gpr(d.rt, std::bit_cast<std::uint32_t>(ctx.fpr[d.rd])); break;
    case K::Mtc1: ctx.fpr[d.rd] = std::bit_cast<float>(g[d.rt]); break;
    case K::Cfc1:
        if (d.rd != 31u) { rt.unsupported(pc, d.word, "unsupported CFC1 control register"); return false; }
        ctx.set_gpr(d.rt, ctx.fcr31);
        break;
    case K::Ctc1:
        if (d.rd != 31u) { rt.unsupported(pc, d.word, "unsupported CTC1 control register"); return false; }
        ctx.fcr31 = g[d.rt] & 0x0181FFFFu;
        break;
    case K::AddS: ctx.fpr[d.sa] = ctx.fpr[d.rd] + ctx.fpr[d.rt]; break;
    case K::SubS: ctx.fpr[d.sa] = ctx.fpr[d.rd] - ctx.fpr[d.rt]; break;
    case K::MulS: {
        const float fs = ctx.fpr[d.rd], ft = ctx.fpr[d.rt];
        if ((std::isinf(fs) && ft == 0.0f) || (std::isinf(ft) && fs == 0.0f)) ctx.set_fpr_bits(d.sa, 0x7FC00000u);
        else ctx.fpr[d.sa] = fs * ft;
        break;
    }
    case K::DivS: ctx.fpr[d.sa] = ctx.fpr[d.rd] / ctx.fpr[d.rt]; break;
    case K::SqrtS: ctx.fpr[d.sa] = std::sqrt(ctx.fpr[d.rd]); break;
    case K::AbsS: ctx.set_fpr_bits(d.sa, ctx.fpr_bits(d.rd) & 0x7FFFFFFFu); break;
    case K::MovS: ctx.set_fpr_bits(d.sa, ctx.fpr_bits(d.rd)); break;
    case K::NegS: ctx.set_fpr_bits(d.sa, ctx.fpr_bits(d.rd) ^ 0x80000000u); break;
    case K::RoundWS: ctx.set_fpr_bits(d.sa, ctx.fpu_float_to_word(ctx.fpr[d.rd], 0u)); break;
    case K::TruncWS: ctx.set_fpr_bits(d.sa, ctx.fpu_float_to_word(ctx.fpr[d.rd], 1u)); break;
    case K::CeilWS: ctx.set_fpr_bits(d.sa, ctx.fpu_float_to_word(ctx.fpr[d.rd], 2u)); break;
    case K::FloorWS: ctx.set_fpr_bits(d.sa, ctx.fpu_float_to_word(ctx.fpr[d.rd], 3u)); break;
    case K::CvtWS: ctx.set_fpr_bits(d.sa, ctx.fpu_float_to_word(ctx.fpr[d.rd], 4u)); break;
    case K::CvtSW: ctx.fpr[d.sa] = static_cast<float>(static_cast<std::int32_t>(ctx.fpr_bits(d.rd))); break;
    case K::FpuCompare: {
        const float fs = ctx.fpr[d.rd], ft = ctx.fpr[d.rt];
        const bool unordered = std::isnan(fs) || std::isnan(ft);
        bool r = false;
        switch (d.word & 0xFu) {
        case 0u: case 8u: r = false; break;
        case 1u: case 9u: r = unordered; break;
        case 2u: case 10u: r = !unordered && fs == ft; break;
        case 3u: case 11u: r = unordered || fs == ft; break;
        case 4u: case 12u: r = fs < ft; break;
        case 5u: case 13u: r = unordered || fs < ft; break;
        case 6u: case 14u: r = fs <= ft; break;
        default: r = unordered || fs <= ft; break;
        }
        ctx.set_fpu_condition(r);
        break;
    }
    case K::Vflush:
        if ((d.word & 0xFFFF0000u) != 0xFFFF0000u) ctx.eat_vfpu_prefixes();
        break;
    case K::Vpfx: {
        const std::uint32_t control = (d.word >> 24u) & 3u;
        std::uint32_t data = d.word & 0x000FFFFFu;
        if (control == 2u) data &= 0x00000FFFu;
        ctx.vfpu_ctrl[control] = data;
        break;
    }
    case K::Viim: {
        const float v[1]{static_cast<float>(static_cast<std::int16_t>(d.word & 0xFFFFu))};
        ctx.write_vfpu_vector_with_destination_prefix(v, (d.word >> 16u) & 0x7Fu, 1u);
        break;
    }
    case K::Vfim: {
        const float v[1]{half_to_float(d.word & 0xFFFFu)};
        ctx.write_vfpu_vector_with_destination_prefix(v, (d.word >> 16u) & 0x7Fu, 1u);
        break;
    }
    case K::Vf2h:
        ctx.execute_vfpu_vf2h(d.word & 0x7Fu, (d.word >> 8u) & 0x7Fu, size_code_length(d.word));
        break;
    case K::Vh2f:
        ctx.execute_vfpu_vh2f(d.word & 0x7Fu, (d.word >> 8u) & 0x7Fu, size_code_length(d.word));
        break;
    case K::Vf2i: {
        const std::uint32_t length = size_code_length(d.word);
        const std::uint32_t destination = d.word & 0x7Fu;
        const std::uint32_t mode = (d.word >> 21u) & 31u;
        float s[4]{}; std::int32_t out[4]{};
        ctx.read_vfpu_vector(s, (d.word >> 8u) & 0x7Fu, length);
        ctx.apply_vfpu_source_prefix(s, length, 0u);
        const float scale = std::ldexp(1.0f, static_cast<int>((d.word >> 16u) & 31u));
        for (std::uint32_t i = 0; i < length; ++i) {
            if (std::isnan(s[i])) { out[i] = std::numeric_limits<std::int32_t>::max(); continue; }
            const double scaled = static_cast<double>(s[i] * scale);
            if (scaled > static_cast<double>(std::numeric_limits<std::int32_t>::max())) out[i] = std::numeric_limits<std::int32_t>::max();
            else if (scaled <= static_cast<double>(std::numeric_limits<std::int32_t>::min())) out[i] = std::numeric_limits<std::int32_t>::min();
            else {
                double rounded = 0.0;
                switch (mode) {
                case 16u: rounded = AllegrexContext::round_ties_to_even(scaled); break;
                case 17u: rounded = std::trunc(scaled); break;
                case 18u: rounded = std::ceil(scaled); break;
                default: rounded = std::floor(scaled); break;
                }
                out[i] = static_cast<std::int32_t>(rounded);
            }
        }
        const std::uint32_t destination_prefix = ctx.vfpu_ctrl[2];
        for (std::uint32_t i = 0; i < length; ++i)
            if (((destination_prefix >> (8u + i)) & 1u) == 0u)
                ctx.vfpu[AllegrexContext::vfpu_vector_lane_index(destination, length, i)] =
                    std::bit_cast<float>(static_cast<std::uint32_t>(out[i]));
        ctx.eat_vfpu_prefixes();
        break;
    }
    case K::Vi2f: {
        const std::uint32_t length = size_code_length(d.word);
        float s[4]{}, out[4]{};
        ctx.read_vfpu_vector(s, (d.word >> 8u) & 0x7Fu, length);
        ctx.apply_vfpu_source_prefix(s, length, 0u);
        const float scale = std::ldexp(1.0f, -static_cast<int>((d.word >> 16u) & 31u));
        for (std::uint32_t i = 0; i < length; ++i)
            out[i] = static_cast<float>(static_cast<std::int32_t>(std::bit_cast<std::uint32_t>(s[i]))) * scale;
        ctx.write_vfpu_vector_with_destination_prefix(out, d.word & 0x7Fu, length);
        break;
    }
    case K::Vx2i:
        ctx.execute_vfpu_vx2i(d.word & 0x7Fu, (d.word >> 8u) & 0x7Fu, size_code_length(d.word), (d.word >> 16u) & 3u);
        break;
    case K::Mtv: ctx.set_vfpu_scalar_bits(d.word & 0xFFu, g[d.rt]); break;
    case K::Mfv: ctx.set_gpr(d.rt, ctx.vfpu_scalar_bits(d.word & 0xFFu)); break;
    case K::VmidT: ctx.execute_vfpu_matrix_init(d.word & 0x7Fu, size_code_length(d.word), 3u); break;
    case K::Vmmov: ctx.execute_vfpu_vmmov(d.word & 0x7Fu, (d.word >> 8u) & 0x7Fu, size_code_length(d.word)); break;
    case K::VfpuMatrixInit:
        ctx.execute_vfpu_matrix_init(d.word & 0x7Fu, size_code_length(d.word), (d.word >> 16u) & 15u);
        break;
    case K::Vidt: {
        const std::uint32_t length = size_code_length(d.word);
        const std::uint32_t destination = d.word & 0x7Fu;
        float v[4]{};
        v[destination & (length >= 3u ? 3u : 1u)] = 1.0f;
        ctx.write_vfpu_vector_with_destination_prefix(v, destination, length);
        break;
    }
    case K::Vtfm: {
        const std::uint32_t side = ((d.word >> 23u) & 3u) + 1u;
        const std::uint32_t input_length = size_code_length(d.word);
        float matrix[16]{}, raw[4]{}, target[4]{}, result[4]{};
        ctx.read_vfpu_matrix(matrix, (d.word >> 8u) & 0x7Fu, side);
        ctx.read_vfpu_vector(raw, (d.word >> 16u) & 0x7Fu, side);
        for (std::uint32_t i = 0; i < 4u; ++i) target[i] = i < input_length ? raw[i] : 0.0f;
        if (side - 1u >= input_length) target[side - 1u] = 1.0f;
        for (std::uint32_t row = 0; row + 1u < side; ++row) {
            float sum = 0.0f;
            for (std::uint32_t column = 0; column < side; ++column) sum += matrix[row * 4u + column] * target[column];
            result[row] = sum;
        }
        float final_row[4]{matrix[(side - 1u) * 4u + 0u], matrix[(side - 1u) * 4u + 1u],
                           matrix[(side - 1u) * 4u + 2u], matrix[(side - 1u) * 4u + 3u]};
        ctx.apply_vfpu_source_prefix(final_row, 4u, 0u);
        ctx.apply_vfpu_source_prefix(target, 4u, 1u);
        for (std::uint32_t column = 0; column < 4u; ++column) result[side - 1u] += final_row[column] * target[column];
        const std::uint32_t destination_prefix = ctx.vfpu_ctrl[2];
        const std::uint32_t last = side - 1u;
        ctx.vfpu_ctrl[2] = ((destination_prefix & (1u << 8u)) << last) | ((destination_prefix & 3u) << (last * 2u));
        ctx.write_vfpu_vector_with_destination_prefix(result, d.word & 0x7Fu, side);
        break;
    }
    case K::VfpuVectorInit: {
        const float value = ((d.word >> 16u) & 31u) == 7u ? 1.0f : 0.0f;
        const float v[4]{value, value, value, value};
        ctx.write_vfpu_vector_with_destination_prefix(v, d.word & 0x7Fu, size_code_length(d.word));
        break;
    }
    case K::Vmmul: {
        const std::uint32_t side = size_code_length(d.word);
        float s[16]{}, t[16]{}, out[16]{};
        ctx.read_vfpu_matrix(s, (d.word >> 8u) & 0x7Fu, side);
        ctx.read_vfpu_matrix(t, (d.word >> 16u) & 0x7Fu, side);
        for (std::uint32_t a = 0; a < side; ++a)
            for (std::uint32_t b = 0; b < side; ++b) {
                float sum = 0.0f;
                for (std::uint32_t c = 0; c < side; ++c) sum += s[b * 4u + c] * t[a * 4u + c];
                out[a * 4u + b] = sum;
            }
        ctx.write_vfpu_matrix(out, d.word & 0x7Fu, side);
        ctx.eat_vfpu_prefixes();
        break;
    }
    case K::Vmscl:
        ctx.execute_vfpu_vmscl(d.word & 0x7Fu, (d.word >> 8u) & 0x7Fu, (d.word >> 16u) & 0x7Fu, size_code_length(d.word));
        break;
    case K::Vrot:
        ctx.execute_vfpu_vrot(d.word & 0x7Fu, (d.word >> 8u) & 0x7Fu, size_code_length(d.word), (d.word >> 16u) & 31u);
        break;
    case K::Vocp: ctx.execute_vfpu_vocp(d.word & 0x7Fu, (d.word >> 8u) & 0x7Fu, size_code_length(d.word)); break;
    case K::VfpuCross: ctx.execute_vfpu_vcrs(d.word & 0x7Fu, (d.word >> 8u) & 0x7Fu, (d.word >> 16u) & 0x7Fu); break;
    case K::VfpuButterfly1: ctx.execute_vfpu_vbfy1(d.word & 0x7Fu, (d.word >> 8u) & 0x7Fu, size_code_length(d.word)); break;
    case K::VfpuSign: ctx.execute_vfpu_vsgn(d.word & 0x7Fu, (d.word >> 8u) & 0x7Fu, size_code_length(d.word)); break;
    case K::VfpuSocp: ctx.execute_vfpu_vsocp(d.word & 0x7Fu, (d.word >> 8u) & 0x7Fu, size_code_length(d.word)); break;
    case K::VfpuI2uc: ctx.execute_vfpu_vi2uc(d.word & 0x7Fu, (d.word >> 8u) & 0x7Fu); break;
    case K::VfpuHorizontal:
        ctx.execute_vfpu_horizontal(d.word & 0x7Fu, (d.word >> 8u) & 0x7Fu, size_code_length(d.word),
                                    ((d.word >> 16u) & 31u) == 7u);
        break;
    case K::VfpuVec3: {
        const std::uint32_t length = size_code_length(d.word);
        const std::uint32_t operation = (d.word >> 26u) == 0x19u ? 2u : ((d.word >> 23u) & 7u);
        float s[4]{}, t[4]{}, out[4]{};
        ctx.read_vfpu_vector_with_source_prefix(s, (d.word >> 8u) & 0x7Fu, length, 0u);
        ctx.read_vfpu_vector_with_source_prefix(t, (d.word >> 16u) & 0x7Fu, length, 1u);
        for (std::uint32_t i = 0; i < length; ++i)
            out[i] = operation == 0u ? s[i] + t[i] : operation == 1u ? s[i] - t[i]
                   : operation == 2u ? s[i] * t[i] : s[i] / t[i];
        ctx.write_vfpu_vector_with_destination_prefix(out, d.word & 0x7Fu, length);
        break;
    }
    case K::Vdot:
        ctx.execute_vfpu_vdot(d.word & 0x7Fu, (d.word >> 8u) & 0x7Fu, (d.word >> 16u) & 0x7Fu, size_code_length(d.word));
        break;
    case K::Vhdp:
        ctx.execute_vfpu_vhdp(d.word & 0x7Fu, (d.word >> 8u) & 0x7Fu, (d.word >> 16u) & 0x7Fu, size_code_length(d.word));
        break;
    case K::VcrossQuat:
        ctx.execute_vfpu_cross_quat(d.word & 0x7Fu, (d.word >> 8u) & 0x7Fu, (d.word >> 16u) & 0x7Fu, size_code_length(d.word));
        break;
    case K::Vminmax:
        ctx.execute_vfpu_vminmax(d.word & 0x7Fu, (d.word >> 8u) & 0x7Fu, (d.word >> 16u) & 0x7Fu, size_code_length(d.word),
                                 ((d.word >> 23u) & 7u) == 3u);
        break;
    case K::VfpuCompare3:
        ctx.execute_vfpu_compare3(d.word & 0x7Fu, (d.word >> 8u) & 0x7Fu, (d.word >> 16u) & 0x7Fu, size_code_length(d.word),
                                  (d.word >> 23u) & 7u);
        break;
    case K::Vcmp:
        ctx.execute_vfpu_vcmp((d.word >> 8u) & 0x7Fu, (d.word >> 16u) & 0x7Fu, size_code_length(d.word), d.word & 15u);
        break;
    case K::Vcmov:
        ctx.execute_vfpu_vcmov(d.word & 0x7Fu, (d.word >> 8u) & 0x7Fu, size_code_length(d.word), (d.word >> 16u) & 7u,
                               ((d.word >> 19u) & 1u) != 0u);
        break;
    case K::Vscl:
        ctx.execute_vfpu_vscl(d.word & 0x7Fu, (d.word >> 8u) & 0x7Fu, (d.word >> 16u) & 0x7Fu, size_code_length(d.word));
        break;
    case K::VfpuUnary: {
        const std::uint32_t length = size_code_length(d.word);
        const std::uint32_t operation = (d.word >> 16u) & 31u;
        float s[4]{}, out[4]{};
        ctx.read_vfpu_vector_with_source_prefix(s, (d.word >> 8u) & 0x7Fu, length, 0u);
        for (std::uint32_t i = 0; i < length; ++i) out[i] = vfpu_unary(operation, s[i]);
        ctx.write_vfpu_vector_with_destination_prefix(out, d.word & 0x7Fu, length);
        break;
    }
    case K::Vcst: {
        const float c = std::bit_cast<float>(kVcstBits[(d.word >> 16u) & 31u]);
        const float v[4]{c, c, c, c};
        ctx.write_vfpu_vector_with_destination_prefix(v, d.word & 0x7Fu, size_code_length(d.word));
        break;
    }
    case K::Lvs: case K::Svs: {
        const std::uint32_t a = g[d.rs] + static_cast<std::uint32_t>(static_cast<std::int32_t>(static_cast<std::int16_t>(d.word & 0xFFFCu)));
        const std::uint32_t scalar = ((d.word >> 16u) & 0x1Fu) | ((d.word & 3u) << 5u);
        if (d.kind == K::Lvs) ctx.set_vfpu_scalar_bits(scalar, mem.aot_load32(a));
        else mem.aot_store32(a, ctx.vfpu_scalar_bits(scalar));
        break;
    }
    case K::Lvq: case K::Svq: {
        const std::uint32_t a = g[d.rs] + static_cast<std::uint32_t>(static_cast<std::int32_t>(static_cast<std::int16_t>(d.word & 0xFFFCu)));
        const std::uint32_t vector = ((d.word >> 16u) & 0x1Fu) | ((d.word & 1u) << 5u);
        float v[4]{};
        if (d.kind == K::Lvq) {
            for (std::uint32_t i = 0; i < 4u; ++i) v[i] = std::bit_cast<float>(mem.aot_load32(a + 4u * i));
            ctx.write_vfpu_vector(v, vector, 4u);
        } else {
            ctx.read_vfpu_vector(v, vector, 4u);
            for (std::uint32_t i = 0; i < 4u; ++i) mem.aot_store32(a + 4u * i, std::bit_cast<std::uint32_t>(v[i]));
        }
        break;
    }
    default:
        --executed_;
        rt.unsupported(pc, d.word, d.mnemonic + " not lowered yet");
        return false;
    }
    return !rt.stopped();
}

Interpreter::Step Interpreter::step(Runtime &rt, AllegrexContext &ctx) {
    const std::uint32_t pc = ctx.pc;
    // unordered_map node references stay valid across the second decode.
    const DecodedInstruction &d = decode(rt.memory(), pc);
    if (!d.has_delay_slot()) {
        if (d.kind == K::Syscall || d.kind == K::Vfpu || d.kind == K::Unsupported) {
            rt.unsupported(pc, d.word, d.mnemonic + " not lowered yet");
            return Step::Stopped;
        }
        if (!execute_regular(rt, ctx, d, pc)) return Step::Stopped;
        ctx.pc = pc + 4u;
        return Step::Continue;
    }
    const DecodedInstruction &slot = decode(rt.memory(), pc + 4u);
    if (slot.is_control_flow()) {
        rt.unsupported(pc + 4u, slot.word, "control flow in delay slot");
        return Step::Stopped;
    }
    ++executed_;
    if (is_branch(d.kind)) {
        const std::uint32_t target = pc + 4u + static_cast<std::uint32_t>(static_cast<std::int32_t>(d.immediate) * 4);
        const std::uint32_t fallthrough = pc + 8u;
        if (is_link(d.kind)) ctx.set_gpr(31, pc + 8u);
        const bool taken = branch_condition(ctx, d);
        if (!is_likely(d.kind) || taken) {
            if (!execute_regular(rt, ctx, slot, pc + 4u)) return Step::Stopped;
        }
        const std::uint32_t next = taken ? target : fallthrough;
        rt.record_transfer(pc, d.word, next);
        ctx.pc = next;
        return Step::Transfer;
    }
    if (d.kind == K::J || d.kind == K::Jal) {
        const std::uint32_t target = ((pc + 4u) & 0xF0000000u) | (d.target << 2u);
        if (d.kind == K::Jal) ctx.set_gpr(31, pc + 8u);
        if (!execute_regular(rt, ctx, slot, pc + 4u)) return Step::Stopped;
        rt.record_transfer(pc, d.word, target);
        ctx.pc = target;
        return Step::Transfer;
    }
    // Jr / Jalr: the target register is read before the link write and slot.
    const std::uint32_t jump_target = ctx.gpr[d.rs];
    if (d.kind == K::Jalr) ctx.set_gpr(d.rd == 0u ? 31u : d.rd, pc + 8u);
    if (!execute_regular(rt, ctx, slot, pc + 4u)) return Step::Stopped;
    rt.record_transfer(pc, d.word, jump_target);
    ctx.pc = jump_target;
    return Step::Transfer;
}

std::uint64_t Interpreter::run(Runtime &rt, AllegrexContext &ctx, std::uint64_t max_instructions,
                              bool yield_to_aot) {
    const std::uint64_t start = executed_;
    while (executed_ - start < max_instructions) {
        if (!yield_to_aot && stop_pcs.contains(ctx.pc)) break;
        if (step(rt, ctx) == Step::Stopped || rt.stopped()) break;
        if (yield_to_aot && rt.has_function(ctx.pc)) break;
        if (!rt.memory().contains(ctx.pc, 4u)) {
            rt.stop("Interpreter left guest memory at " + psprecomp::hex32(ctx.pc));
            break;
        }
    }
    return executed_ - start;
}

void install_interpreter_fallback(Interpreter *interpreter, std::uint64_t max_instructions_per_entry) {
    g_interpreter = interpreter;
    g_max_per_entry = max_instructions_per_entry;
    psprecomp::set_runtime_fallback_hook(interpreter != nullptr ? &fallback_hook : nullptr);
}

} // namespace p3p3ds
