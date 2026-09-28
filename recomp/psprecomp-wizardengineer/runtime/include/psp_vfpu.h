#pragma once
#include <cstdint>

// Forward declaration (defined in recomp.h)
struct recomp_context;

// VFPU control register indices
constexpr int VFPU_CTRL_SPREFIX = 0;
constexpr int VFPU_CTRL_TPREFIX = 1;
constexpr int VFPU_CTRL_DPREFIX = 2;
constexpr int VFPU_CTRL_CC = 3;
constexpr int VFPU_CTRL_INF4 = 4;
constexpr int VFPU_CTRL_RSV5 = 5;
constexpr int VFPU_CTRL_RSV6 = 6;
constexpr int VFPU_CTRL_REV = 7;
constexpr int VFPU_CTRL_RCX0 = 8;
constexpr int VFPU_CTRL_RCX1 = 9;
constexpr int VFPU_CTRL_RCX2 = 10;
constexpr int VFPU_CTRL_RCX3 = 11;
constexpr int VFPU_CTRL_RCX4 = 12;
constexpr int VFPU_CTRL_RCX5 = 13;
constexpr int VFPU_CTRL_RCX6 = 14;
constexpr int VFPU_CTRL_RCX7 = 15;

// ---------------------------------------------------------------------------
// Register helpers
// ---------------------------------------------------------------------------

/// Physical register-file layout follows PPSSPP's convention:
///   vfpu[mtx * 16 + col * 4 + row]
/// i.e. each matrix is stored column-major in the flat array.
/// This single helper is shared by all VFPU translation units so the
/// layout choice stays encapsulated in one place.
inline int vfpu_single_index(int reg) {
    int mtx = (reg >> 2) & 7;
    int col = reg & 3;
    int row = (reg >> 5) & 3;
    return mtx * 16 + col * 4 + row;
}

void vfpu_read_vector(float* dst, int n, int reg,
                      const float vfpu[128]);
void vfpu_write_vector(const float* src, int n, int reg,
                       float vfpu[128], uint32_t dprefix);

// ---------------------------------------------------------------------------
// Prefix helpers
// ---------------------------------------------------------------------------
void vfpu_apply_prefix_st(float* r, uint32_t prefix, int n);
void vfpu_apply_prefix_d(float* r, uint32_t dprefix, int n);
void vfpu_eat_prefixes(recomp_context* ctx);
void vfpu_set_prefix(recomp_context* ctx, int reg_idx,
                     uint32_t data);

/// Initialise a freshly-zeroed context's VFPU control registers to their
/// hardware-reset/"no prefix pending" state. The S/T prefixes default to the
/// identity swizzle 0xE4 (lane i <- component i); a zero prefix is NOT the
/// default -- it is an explicit "all lanes <- component 0" swizzle. Every
/// recomp_context must be passed through this after a memset(0), or the first
/// VFPU arithmetic op on the thread (before any eat_prefixes resets the state)
/// silently swizzles all operand lanes to component 0 and corrupts the result.
void vfpu_init_context(recomp_context* ctx);

// ---------------------------------------------------------------------------
// Control register moves
// ---------------------------------------------------------------------------
void vfpu_mfv(recomp_context* ctx, int rt_idx, uint8_t vd);
void vfpu_mtv(recomp_context* ctx, int rt_idx, uint8_t vd);
void vfpu_mfvc(recomp_context* ctx, int rt_idx, int imm);
void vfpu_mtvc(recomp_context* ctx, int rt_idx, int imm);

// ---------------------------------------------------------------------------
// Immediate loads
// ---------------------------------------------------------------------------
void vfpu_viim(recomp_context* ctx, uint8_t vt, uint16_t imm);
void vfpu_vfim(recomp_context* ctx, uint8_t vt, uint16_t imm);

// ---------------------------------------------------------------------------
// Arithmetic (binary)
// ---------------------------------------------------------------------------
void vfpu_vadd(recomp_context* ctx, uint8_t* rdram,
               uint8_t vd, uint8_t vs, uint8_t vt, uint8_t size);
void vfpu_vsub(recomp_context* ctx, uint8_t* rdram,
               uint8_t vd, uint8_t vs, uint8_t vt, uint8_t size);
void vfpu_vmul(recomp_context* ctx, uint8_t* rdram,
               uint8_t vd, uint8_t vs, uint8_t vt, uint8_t size);
void vfpu_vdiv(recomp_context* ctx, uint8_t* rdram,
               uint8_t vd, uint8_t vs, uint8_t vt, uint8_t size);
void vfpu_vmin(recomp_context* ctx, uint8_t* rdram,
               uint8_t vd, uint8_t vs, uint8_t vt, uint8_t size);
void vfpu_vmax(recomp_context* ctx, uint8_t* rdram,
               uint8_t vd, uint8_t vs, uint8_t vt, uint8_t size);
void vfpu_vscmp(recomp_context* ctx, uint8_t* rdram,
                uint8_t vd, uint8_t vs, uint8_t vt, uint8_t size);
void vfpu_vsge(recomp_context* ctx, uint8_t* rdram,
               uint8_t vd, uint8_t vs, uint8_t vt, uint8_t size);
void vfpu_vslt(recomp_context* ctx, uint8_t* rdram,
               uint8_t vd, uint8_t vs, uint8_t vt, uint8_t size);

// Reduction / special binary ops
void vfpu_vdot(recomp_context* ctx, uint8_t* rdram,
               uint8_t vd, uint8_t vs, uint8_t vt, uint8_t size);
void vfpu_vscl(recomp_context* ctx, uint8_t* rdram,
               uint8_t vd, uint8_t vs, uint8_t vt, uint8_t size);
void vfpu_vhdp(recomp_context* ctx, uint8_t* rdram,
               uint8_t vd, uint8_t vs, uint8_t vt, uint8_t size);
void vfpu_vcrs(recomp_context* ctx, uint8_t* rdram,
               uint8_t vd, uint8_t vs, uint8_t vt, uint8_t size);
void vfpu_vdet(recomp_context* ctx, uint8_t* rdram,
               uint8_t vd, uint8_t vs, uint8_t vt, uint8_t size);

// Compare / conditional
void vfpu_vcmp(recomp_context* ctx, uint8_t* rdram,
               uint8_t vs, uint8_t vt, uint8_t cond, uint8_t size);
void vfpu_vcmov(recomp_context* ctx, uint8_t* rdram,
                uint8_t vd, uint8_t vs, uint8_t cc, uint8_t size);

// ---------------------------------------------------------------------------
// Trig / unary math
// ---------------------------------------------------------------------------
void vfpu_vmov(recomp_context* ctx, uint8_t* rdram,
               uint8_t vd, uint8_t vs, uint8_t size);
void vfpu_vabs(recomp_context* ctx, uint8_t* rdram,
               uint8_t vd, uint8_t vs, uint8_t size);
void vfpu_vneg(recomp_context* ctx, uint8_t* rdram,
               uint8_t vd, uint8_t vs, uint8_t size);
void vfpu_vrcp(recomp_context* ctx, uint8_t* rdram,
               uint8_t vd, uint8_t vs, uint8_t size);
void vfpu_vrsq(recomp_context* ctx, uint8_t* rdram,
               uint8_t vd, uint8_t vs, uint8_t size);
void vfpu_vsin(recomp_context* ctx, uint8_t* rdram,
               uint8_t vd, uint8_t vs, uint8_t size);
void vfpu_vcos(recomp_context* ctx, uint8_t* rdram,
               uint8_t vd, uint8_t vs, uint8_t size);
void vfpu_vexp2(recomp_context* ctx, uint8_t* rdram,
                uint8_t vd, uint8_t vs, uint8_t size);
void vfpu_vlog2(recomp_context* ctx, uint8_t* rdram,
                uint8_t vd, uint8_t vs, uint8_t size);
void vfpu_vsqrt(recomp_context* ctx, uint8_t* rdram,
                uint8_t vd, uint8_t vs, uint8_t size);
void vfpu_vasin(recomp_context* ctx, uint8_t* rdram,
                uint8_t vd, uint8_t vs, uint8_t size);
void vfpu_vnrcp(recomp_context* ctx, uint8_t* rdram,
                uint8_t vd, uint8_t vs, uint8_t size);
void vfpu_vnsin(recomp_context* ctx, uint8_t* rdram,
                uint8_t vd, uint8_t vs, uint8_t size);
void vfpu_vrexp2(recomp_context* ctx, uint8_t* rdram,
                 uint8_t vd, uint8_t vs, uint8_t size);

// Identity / zero / one / saturation
void vfpu_vidt(recomp_context* ctx, uint8_t* rdram,
               uint8_t vd, uint8_t size);
void vfpu_vzero(recomp_context* ctx, uint8_t* rdram,
                uint8_t vd, uint8_t size);
void vfpu_vone(recomp_context* ctx, uint8_t* rdram,
               uint8_t vd, uint8_t size);
void vfpu_vsat0(recomp_context* ctx, uint8_t* rdram,
                uint8_t vd, uint8_t vs, uint8_t size);
void vfpu_vsat1(recomp_context* ctx, uint8_t* rdram,
                uint8_t vd, uint8_t vs, uint8_t size);

// Constants
void vfpu_vcst(recomp_context* ctx, uint8_t* rdram,
               uint8_t vd, uint8_t imm5, uint8_t size);

// ---------------------------------------------------------------------------
// Conversions
// ---------------------------------------------------------------------------
void vfpu_vf2in(recomp_context* ctx, uint8_t* rdram,
                uint8_t vd, uint8_t vs, uint8_t imm5, uint8_t size);
void vfpu_vf2iz(recomp_context* ctx, uint8_t* rdram,
                uint8_t vd, uint8_t vs, uint8_t imm5, uint8_t size);
void vfpu_vf2iu(recomp_context* ctx, uint8_t* rdram,
                uint8_t vd, uint8_t vs, uint8_t imm5, uint8_t size);
void vfpu_vf2id(recomp_context* ctx, uint8_t* rdram,
                uint8_t vd, uint8_t vs, uint8_t imm5, uint8_t size);
void vfpu_vi2f(recomp_context* ctx, uint8_t* rdram,
               uint8_t vd, uint8_t vs, uint8_t imm5, uint8_t size);
void vfpu_vi2uc(recomp_context* ctx, uint8_t* rdram,
                uint8_t vd, uint8_t vs, uint8_t size);
void vfpu_vi2c(recomp_context* ctx, uint8_t* rdram,
               uint8_t vd, uint8_t vs, uint8_t size);
void vfpu_vi2us(recomp_context* ctx, uint8_t* rdram,
                uint8_t vd, uint8_t vs, uint8_t size);
void vfpu_vi2s(recomp_context* ctx, uint8_t* rdram,
               uint8_t vd, uint8_t vs, uint8_t size);
void vfpu_vuc2i(recomp_context* ctx, uint8_t* rdram,
                uint8_t vd, uint8_t vs, uint8_t size);
void vfpu_vc2i(recomp_context* ctx, uint8_t* rdram,
               uint8_t vd, uint8_t vs, uint8_t size);
void vfpu_vus2i(recomp_context* ctx, uint8_t* rdram,
                uint8_t vd, uint8_t vs, uint8_t size);
void vfpu_vs2i(recomp_context* ctx, uint8_t* rdram,
               uint8_t vd, uint8_t vs, uint8_t size);
void vfpu_vf2h(recomp_context* ctx, uint8_t* rdram,
               uint8_t vd, uint8_t vs, uint8_t size);
void vfpu_vh2f(recomp_context* ctx, uint8_t* rdram,
               uint8_t vd, uint8_t vs, uint8_t size);

// ---------------------------------------------------------------------------
// Matrix operations
// ---------------------------------------------------------------------------
void vfpu_vmmul(recomp_context* ctx, uint8_t* rdram,
                uint8_t vd, uint8_t vs, uint8_t vt, uint8_t size);
void vfpu_vmscl(recomp_context* ctx, uint8_t* rdram,
                uint8_t vd, uint8_t vs, uint8_t vt, uint8_t size);
void vfpu_vtfm2(recomp_context* ctx, uint8_t* rdram,
                uint8_t vd, uint8_t vs, uint8_t vt);
void vfpu_vtfm3(recomp_context* ctx, uint8_t* rdram,
                uint8_t vd, uint8_t vs, uint8_t vt);
void vfpu_vtfm4(recomp_context* ctx, uint8_t* rdram,
                uint8_t vd, uint8_t vs, uint8_t vt);
void vfpu_vhtfm2(recomp_context* ctx, uint8_t* rdram,
                 uint8_t vd, uint8_t vs, uint8_t vt);
void vfpu_vhtfm3(recomp_context* ctx, uint8_t* rdram,
                 uint8_t vd, uint8_t vs, uint8_t vt);
void vfpu_vhtfm4(recomp_context* ctx, uint8_t* rdram,
                 uint8_t vd, uint8_t vs, uint8_t vt);
void vfpu_vcrsp(recomp_context* ctx, uint8_t* rdram,
                uint8_t vd, uint8_t vs, uint8_t vt);
void vfpu_vqmul(recomp_context* ctx, uint8_t* rdram,
                uint8_t vd, uint8_t vs, uint8_t vt);
void vfpu_vmmov(recomp_context* ctx, uint8_t* rdram,
                uint8_t vd, uint8_t vs, uint8_t size);
void vfpu_vmidt(recomp_context* ctx, uint8_t* rdram,
                uint8_t vd, uint8_t size);
void vfpu_vmzero(recomp_context* ctx, uint8_t* rdram,
                 uint8_t vd, uint8_t size);
void vfpu_vmone(recomp_context* ctx, uint8_t* rdram,
                uint8_t vd, uint8_t size);

// ---------------------------------------------------------------------------
// Memory operations
// ---------------------------------------------------------------------------
void vfpu_lv_s(recomp_context* ctx, uint8_t* rdram,
               uint8_t vt, uint8_t rs, int16_t offset);
void vfpu_sv_s(recomp_context* ctx, uint8_t* rdram,
               uint8_t vt, uint8_t rs, int16_t offset);
void vfpu_lv_q(recomp_context* ctx, uint8_t* rdram,
               uint8_t vt, uint8_t rs, int16_t offset);
void vfpu_sv_q(recomp_context* ctx, uint8_t* rdram,
               uint8_t vt, uint8_t rs, int16_t offset);
void vfpu_lvl_q(recomp_context* ctx, uint8_t* rdram,
                uint8_t vt, uint8_t rs, int16_t offset);
void vfpu_lvr_q(recomp_context* ctx, uint8_t* rdram,
                uint8_t vt, uint8_t rs, int16_t offset);
void vfpu_svl_q(recomp_context* ctx, uint8_t* rdram,
                uint8_t vt, uint8_t rs, int16_t offset);
void vfpu_svr_q(recomp_context* ctx, uint8_t* rdram,
                uint8_t vt, uint8_t rs, int16_t offset);

// ---------------------------------------------------------------------------
// Sort / pack / misc
// ---------------------------------------------------------------------------
void vfpu_vsrt1(recomp_context* ctx, uint8_t* rdram,
                uint8_t vd, uint8_t vs, uint8_t size);
void vfpu_vsrt2(recomp_context* ctx, uint8_t* rdram,
                uint8_t vd, uint8_t vs, uint8_t size);
void vfpu_vsrt3(recomp_context* ctx, uint8_t* rdram,
                uint8_t vd, uint8_t vs, uint8_t size);
void vfpu_vsrt4(recomp_context* ctx, uint8_t* rdram,
                uint8_t vd, uint8_t vs, uint8_t size);
void vfpu_vbfy1(recomp_context* ctx, uint8_t* rdram,
                uint8_t vd, uint8_t vs, uint8_t size);
void vfpu_vbfy2(recomp_context* ctx, uint8_t* rdram,
                uint8_t vd, uint8_t vs, uint8_t size);
void vfpu_vsocp(recomp_context* ctx, uint8_t* rdram,
                uint8_t vd, uint8_t vs, uint8_t size);
void vfpu_vfad(recomp_context* ctx, uint8_t* rdram,
               uint8_t vd, uint8_t vs, uint8_t size);
void vfpu_vavg(recomp_context* ctx, uint8_t* rdram,
               uint8_t vd, uint8_t vs, uint8_t size);

// Random
void vfpu_vrnds(recomp_context* ctx, uint8_t* rdram,
                uint8_t vs);
void vfpu_vrndi(recomp_context* ctx, uint8_t* rdram,
                uint8_t vd, uint8_t size);
void vfpu_vrndf1(recomp_context* ctx, uint8_t* rdram,
                 uint8_t vd, uint8_t size);
void vfpu_vrndf2(recomp_context* ctx, uint8_t* rdram,
                 uint8_t vd, uint8_t size);

// Rotation
void vfpu_vrot(recomp_context* ctx, uint8_t* rdram,
               uint8_t vd, uint8_t vs, uint16_t imm5, uint8_t size);

// Wrap by negative (complex)
void vfpu_vwbn(recomp_context* ctx, uint8_t* rdram,
               uint8_t vd, uint8_t vs, uint8_t imm8, uint8_t size);

// Flush / no-op
void vfpu_vflush();

// Unknown opcode stub (log-once warning)
void vfpu_unknown_stub(recomp_context* ctx, uint8_t* rdram,
                       uint32_t opcode, uint32_t pc);
