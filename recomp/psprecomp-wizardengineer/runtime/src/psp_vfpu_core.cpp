#include "psp_vfpu.h"
#include "recomp.h"
#include <cmath>
#include <cstring>
#include <cstdint>
#include <cstdio>
#include <unordered_set>

// ---------------------------------------------------------------------------
// Register read/write (PPSSPP ReadVector/WriteVector semantics)
//
// Physical layout: vfpu[mtx*16 + col*4 + row] (see vfpu_single_index in
// psp_vfpu.h). Vector access anchors at a fixed column (reg & 3) and
// iterates rows: stride 1 for column-form (C/non-transpose), stride 4
// for row-form (R/transpose).
// ---------------------------------------------------------------------------

/// Decode the starting row for an n-element vector register encoding.
static inline int vfpu_vector_row(int reg, int n) {
    switch (n) {
    case 2:  return (reg >> 5) & 2;
    case 3:  return (reg >> 6) & 1;
    case 4:  return (reg >> 5) & 2;
    default: return 0;
    }
}

void vfpu_read_vector(float* dst, int n, int reg,
                      const float vfpu[128]) {
    if (n == 1) {
        dst[0] = vfpu[vfpu_single_index(reg)];
        return;
    }

    int row = vfpu_vector_row(reg, n);
    int transpose = (reg >> 5) & 1;
    int mtx = (reg >> 2) & 7;
    int col = reg & 3;

    if (transpose) {
        // Row form: fixed row anchor (col here selects the row slot),
        // step across columns -> stride 4 in physical layout.
        const int base = mtx * 16 + col;
        for (int i = 0; i < n; i++) {
            dst[i] = vfpu[base + ((row + i) & 3) * 4];
        }
    } else {
        // Column form: fixed column, step down rows -> stride 1.
        const int base = mtx * 16 + col * 4;
        for (int i = 0; i < n; i++) {
            dst[i] = vfpu[base + ((row + i) & 3)];
        }
    }
}

void vfpu_write_vector(const float* src, int n, int reg,
                       float vfpu[128], uint32_t dprefix) {
    if (n == 1) {
        // Write mask for single: bit 8
        if (!((dprefix >> 8) & 1)) {
            vfpu[vfpu_single_index(reg)] = src[0];
        }
        return;
    }

    int row = vfpu_vector_row(reg, n);
    int transpose = (reg >> 5) & 1;
    int mtx = (reg >> 2) & 7;
    int col = reg & 3;

    for (int i = 0; i < n; i++) {
        // Check write mask: bit (8+i) of dprefix
        if ((dprefix >> (8 + i)) & 1) {
            continue;  // masked -- do not write
        }
        int idx;
        if (transpose) {
            idx = mtx * 16 + col + ((row + i) & 3) * 4;
        } else {
            idx = mtx * 16 + col * 4 + ((row + i) & 3);
        }
        vfpu[idx] = src[i];
    }
}

// ---------------------------------------------------------------------------
// Prefix application
// ---------------------------------------------------------------------------

void vfpu_apply_prefix_st(float* r, uint32_t prefix, int n) {
    if (prefix == 0xE4u) return;  // identity -- fast path

    static const float constants[8] = {
        0.0f, 1.0f, 2.0f, 0.5f,
        3.0f, 1.0f / 3.0f, 0.25f, 1.0f / 6.0f
    };

    float orig[4] = {
        r[0],
        n > 1 ? r[1] : 0.0f,
        n > 2 ? r[2] : 0.0f,
        n > 3 ? r[3] : 0.0f
    };

    for (int i = 0; i < n; i++) {
        int swizzle  = (prefix >> (i * 2)) & 3;
        int abs_bit  = (prefix >> (8 + i)) & 1;
        int const_bit = (prefix >> (12 + i)) & 1;
        int neg_bit  = (prefix >> (16 + i)) & 1;

        if (!const_bit) {
            r[i] = orig[swizzle];
            if (abs_bit) {
                uint32_t u;
                std::memcpy(&u, &r[i], 4);
                u &= 0x7FFFFFFFu;  // clear sign bit
                std::memcpy(&r[i], &u, 4);
            }
        } else {
            r[i] = constants[swizzle + (abs_bit << 2)];
        }

        if (neg_bit) {
            uint32_t u;
            std::memcpy(&u, &r[i], 4);
            u ^= 0x80000000u;  // flip sign bit
            std::memcpy(&r[i], &u, 4);
        }
    }
}

/// NaN-aware max: returns cst if f <= cst, else f.
/// NaN in f propagates (NaN is not <= anything).
static inline float nanmax(float f, float cst) {
    return (f <= cst) ? cst : f;
}

/// NaN-aware min: returns cst if f >= cst, else f.
/// NaN in f propagates (NaN is not >= anything).
static inline float nanmin(float f, float cst) {
    return (f >= cst) ? cst : f;
}

static inline float nanclamp(float f, float lower, float upper) {
    return nanmin(nanmax(f, lower), upper);
}

void vfpu_apply_prefix_d(float* r, uint32_t dprefix, int n) {
    for (int i = 0; i < n; i++) {
        int sat = (dprefix >> (i * 2)) & 3;
        switch (sat) {
        case 0: break;  // no saturation
        case 1: r[i] = nanclamp(r[i], 0.0f, 1.0f); break;
        case 3: r[i] = nanclamp(r[i], -1.0f, 1.0f); break;
        default: break;  // mode 2 is undefined; treat as none
        }
    }
}

// ---------------------------------------------------------------------------
// Prefix reset
// ---------------------------------------------------------------------------

void vfpu_eat_prefixes(recomp_context* ctx) {
    ctx->vfpu_ctrl[VFPU_CTRL_SPREFIX] = 0xE4u;
    ctx->vfpu_ctrl[VFPU_CTRL_TPREFIX] = 0xE4u;
    ctx->vfpu_ctrl[VFPU_CTRL_DPREFIX] = 0x00u;
}

void vfpu_init_context(recomp_context* ctx) {
    // A freshly memset(0) context leaves S/T prefix == 0, which is the
    // explicit "all lanes <- component 0" swizzle -- NOT the hardware-reset
    // identity (0xE4). The "no prefix pending" state is exactly what
    // vfpu_eat_prefixes installs, so reuse it for the reset default.
    vfpu_eat_prefixes(ctx);
}

void vfpu_set_prefix(recomp_context* ctx, int reg_idx,
                     uint32_t data) {
    ctx->vfpu_ctrl[VFPU_CTRL_SPREFIX + reg_idx] = data;
}

// ---------------------------------------------------------------------------
// Control register moves
// ---------------------------------------------------------------------------

void vfpu_mfv(recomp_context* ctx, int rt_idx, uint8_t vd) {
    int idx = vfpu_single_index(vd);
    uint32_t u;
    std::memcpy(&u, &ctx->vfpu[idx], 4);
    ctx->r[rt_idx] = static_cast<int32_t>(u);
}

void vfpu_mtv(recomp_context* ctx, int rt_idx, uint8_t vd) {
    int idx = vfpu_single_index(vd);
    uint32_t u = static_cast<uint32_t>(ctx->r[rt_idx]);
    std::memcpy(&ctx->vfpu[idx], &u, 4);
}

void vfpu_mfvc(recomp_context* ctx, int rt_idx, int imm) {
    ctx->r[rt_idx] = static_cast<int32_t>(
        ctx->vfpu_ctrl[imm & 0xF]);
}

void vfpu_mtvc(recomp_context* ctx, int rt_idx, int imm) {
    ctx->vfpu_ctrl[imm & 0xF] =
        static_cast<uint32_t>(ctx->r[rt_idx]);
}

// ---------------------------------------------------------------------------
// Immediate loads
// ---------------------------------------------------------------------------

void vfpu_viim(recomp_context* ctx, uint8_t vt, uint16_t imm) {
    int idx = vfpu_single_index(vt);
    ctx->vfpu[idx] = static_cast<float>(
        static_cast<int16_t>(imm));
}

/// Convert 16-bit half-float to 32-bit float.
static float float16_to_float32(uint16_t h) {
    uint32_t sign = (static_cast<uint32_t>(h) >> 15) & 1;
    uint32_t exp  = (static_cast<uint32_t>(h) >> 10) & 0x1F;
    uint32_t mant = static_cast<uint32_t>(h) & 0x3FF;

    uint32_t result;
    if (exp == 0) {
        if (mant == 0) {
            // Zero
            result = sign << 31;
        } else {
            // Denormal: normalize
            exp = 1;
            while (!(mant & 0x400)) {
                mant <<= 1;
                exp--;
            }
            mant &= 0x3FF;
            result = (sign << 31)
                   | ((exp + 127 - 15) << 23)
                   | (mant << 13);
        }
    } else if (exp == 0x1F) {
        // Inf or NaN
        result = (sign << 31) | 0x7F800000u | (mant << 13);
    } else {
        // Normal
        result = (sign << 31)
               | ((exp + 127 - 15) << 23)
               | (mant << 13);
    }

    float f;
    std::memcpy(&f, &result, 4);
    return f;
}

void vfpu_vfim(recomp_context* ctx, uint8_t vt, uint16_t imm) {
    int idx = vfpu_single_index(vt);
    ctx->vfpu[idx] = float16_to_float32(imm);
}

// ---------------------------------------------------------------------------
// Flush (no-op)
// ---------------------------------------------------------------------------

void vfpu_vflush() {
    // intentionally empty
}

// ---------------------------------------------------------------------------
// Unknown opcode stub (log-once warning)
// ---------------------------------------------------------------------------

void vfpu_unknown_stub(recomp_context* /*ctx*/, uint8_t* /*rdram*/,
                       uint32_t opcode, uint32_t pc) {
    static std::unordered_set<uint32_t> warned;
    if (warned.insert(opcode).second) {
        std::fprintf(stderr,
            "[VFPU] Unknown opcode 0x%08X at PC 0x%08X "
            "(first occurrence, suppressing future warnings)\n",
            opcode, pc);
    }
}
