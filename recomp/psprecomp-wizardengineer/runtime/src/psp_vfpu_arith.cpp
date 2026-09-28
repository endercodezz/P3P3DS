#include "psp_vfpu.h"
#include "recomp.h"
#include <cmath>
#include <cstring>

// ---------------------------------------------------------------------------
// Binary arithmetic -- all follow the read-prefix-compute-prefix_d-write-eat
// pattern.
// ---------------------------------------------------------------------------

void vfpu_vadd(recomp_context* ctx, uint8_t*,
               uint8_t vd, uint8_t vs, uint8_t vt,
               uint8_t size) {
    float s[4], t[4], d[4];
    vfpu_read_vector(s, size, vs, ctx->vfpu);
    vfpu_read_vector(t, size, vt, ctx->vfpu);
    vfpu_apply_prefix_st(s, ctx->vfpu_ctrl[VFPU_CTRL_SPREFIX],
                         size);
    vfpu_apply_prefix_st(t, ctx->vfpu_ctrl[VFPU_CTRL_TPREFIX],
                         size);
    for (int i = 0; i < size; i++) d[i] = s[i] + t[i];
    vfpu_apply_prefix_d(d, ctx->vfpu_ctrl[VFPU_CTRL_DPREFIX],
                        size);
    vfpu_write_vector(d, size, vd, ctx->vfpu,
                      ctx->vfpu_ctrl[VFPU_CTRL_DPREFIX]);
    vfpu_eat_prefixes(ctx);
}

void vfpu_vsub(recomp_context* ctx, uint8_t*,
               uint8_t vd, uint8_t vs, uint8_t vt,
               uint8_t size) {
    float s[4], t[4], d[4];
    vfpu_read_vector(s, size, vs, ctx->vfpu);
    vfpu_read_vector(t, size, vt, ctx->vfpu);
    vfpu_apply_prefix_st(s, ctx->vfpu_ctrl[VFPU_CTRL_SPREFIX],
                         size);
    vfpu_apply_prefix_st(t, ctx->vfpu_ctrl[VFPU_CTRL_TPREFIX],
                         size);
    for (int i = 0; i < size; i++) d[i] = s[i] - t[i];
    vfpu_apply_prefix_d(d, ctx->vfpu_ctrl[VFPU_CTRL_DPREFIX],
                        size);
    vfpu_write_vector(d, size, vd, ctx->vfpu,
                      ctx->vfpu_ctrl[VFPU_CTRL_DPREFIX]);
    vfpu_eat_prefixes(ctx);
}

void vfpu_vmul(recomp_context* ctx, uint8_t*,
               uint8_t vd, uint8_t vs, uint8_t vt,
               uint8_t size) {
    float s[4], t[4], d[4];
    vfpu_read_vector(s, size, vs, ctx->vfpu);
    vfpu_read_vector(t, size, vt, ctx->vfpu);
    vfpu_apply_prefix_st(s, ctx->vfpu_ctrl[VFPU_CTRL_SPREFIX],
                         size);
    vfpu_apply_prefix_st(t, ctx->vfpu_ctrl[VFPU_CTRL_TPREFIX],
                         size);
    for (int i = 0; i < size; i++) d[i] = s[i] * t[i];
    vfpu_apply_prefix_d(d, ctx->vfpu_ctrl[VFPU_CTRL_DPREFIX],
                        size);
    vfpu_write_vector(d, size, vd, ctx->vfpu,
                      ctx->vfpu_ctrl[VFPU_CTRL_DPREFIX]);
    vfpu_eat_prefixes(ctx);
}

void vfpu_vdiv(recomp_context* ctx, uint8_t*,
               uint8_t vd, uint8_t vs, uint8_t vt,
               uint8_t size) {
    float s[4], t[4], d[4];
    vfpu_read_vector(s, size, vs, ctx->vfpu);
    vfpu_read_vector(t, size, vt, ctx->vfpu);
    vfpu_apply_prefix_st(s, ctx->vfpu_ctrl[VFPU_CTRL_SPREFIX],
                         size);
    vfpu_apply_prefix_st(t, ctx->vfpu_ctrl[VFPU_CTRL_TPREFIX],
                         size);
    for (int i = 0; i < size; i++) d[i] = s[i] / t[i];
    vfpu_apply_prefix_d(d, ctx->vfpu_ctrl[VFPU_CTRL_DPREFIX],
                        size);
    vfpu_write_vector(d, size, vd, ctx->vfpu,
                      ctx->vfpu_ctrl[VFPU_CTRL_DPREFIX]);
    vfpu_eat_prefixes(ctx);
}

void vfpu_vmin(recomp_context* ctx, uint8_t*,
               uint8_t vd, uint8_t vs, uint8_t vt,
               uint8_t size) {
    float s[4], t[4], d[4];
    vfpu_read_vector(s, size, vs, ctx->vfpu);
    vfpu_read_vector(t, size, vt, ctx->vfpu);
    vfpu_apply_prefix_st(s, ctx->vfpu_ctrl[VFPU_CTRL_SPREFIX],
                         size);
    vfpu_apply_prefix_st(t, ctx->vfpu_ctrl[VFPU_CTRL_TPREFIX],
                         size);
    for (int i = 0; i < size; i++) {
        d[i] = (s[i] <= t[i]) ? s[i] : t[i];
    }
    vfpu_apply_prefix_d(d, ctx->vfpu_ctrl[VFPU_CTRL_DPREFIX],
                        size);
    vfpu_write_vector(d, size, vd, ctx->vfpu,
                      ctx->vfpu_ctrl[VFPU_CTRL_DPREFIX]);
    vfpu_eat_prefixes(ctx);
}

void vfpu_vmax(recomp_context* ctx, uint8_t*,
               uint8_t vd, uint8_t vs, uint8_t vt,
               uint8_t size) {
    float s[4], t[4], d[4];
    vfpu_read_vector(s, size, vs, ctx->vfpu);
    vfpu_read_vector(t, size, vt, ctx->vfpu);
    vfpu_apply_prefix_st(s, ctx->vfpu_ctrl[VFPU_CTRL_SPREFIX],
                         size);
    vfpu_apply_prefix_st(t, ctx->vfpu_ctrl[VFPU_CTRL_TPREFIX],
                         size);
    for (int i = 0; i < size; i++) {
        d[i] = (s[i] >= t[i]) ? s[i] : t[i];
    }
    vfpu_apply_prefix_d(d, ctx->vfpu_ctrl[VFPU_CTRL_DPREFIX],
                        size);
    vfpu_write_vector(d, size, vd, ctx->vfpu,
                      ctx->vfpu_ctrl[VFPU_CTRL_DPREFIX]);
    vfpu_eat_prefixes(ctx);
}

void vfpu_vscmp(recomp_context* ctx, uint8_t*,
                uint8_t vd, uint8_t vs, uint8_t vt,
                uint8_t size) {
    float s[4], t[4], d[4];
    vfpu_read_vector(s, size, vs, ctx->vfpu);
    vfpu_read_vector(t, size, vt, ctx->vfpu);
    vfpu_apply_prefix_st(s, ctx->vfpu_ctrl[VFPU_CTRL_SPREFIX],
                         size);
    vfpu_apply_prefix_st(t, ctx->vfpu_ctrl[VFPU_CTRL_TPREFIX],
                         size);
    for (int i = 0; i < size; i++) {
        d[i] = (s[i] > t[i]) ? 1.0f
             : ((s[i] < t[i]) ? -1.0f : 0.0f);
    }
    vfpu_apply_prefix_d(d, ctx->vfpu_ctrl[VFPU_CTRL_DPREFIX],
                        size);
    vfpu_write_vector(d, size, vd, ctx->vfpu,
                      ctx->vfpu_ctrl[VFPU_CTRL_DPREFIX]);
    vfpu_eat_prefixes(ctx);
}

void vfpu_vsge(recomp_context* ctx, uint8_t*,
               uint8_t vd, uint8_t vs, uint8_t vt,
               uint8_t size) {
    float s[4], t[4], d[4];
    vfpu_read_vector(s, size, vs, ctx->vfpu);
    vfpu_read_vector(t, size, vt, ctx->vfpu);
    vfpu_apply_prefix_st(s, ctx->vfpu_ctrl[VFPU_CTRL_SPREFIX],
                         size);
    vfpu_apply_prefix_st(t, ctx->vfpu_ctrl[VFPU_CTRL_TPREFIX],
                         size);
    for (int i = 0; i < size; i++) {
        d[i] = (s[i] >= t[i]) ? 1.0f : 0.0f;
    }
    vfpu_apply_prefix_d(d, ctx->vfpu_ctrl[VFPU_CTRL_DPREFIX],
                        size);
    vfpu_write_vector(d, size, vd, ctx->vfpu,
                      ctx->vfpu_ctrl[VFPU_CTRL_DPREFIX]);
    vfpu_eat_prefixes(ctx);
}

void vfpu_vslt(recomp_context* ctx, uint8_t*,
               uint8_t vd, uint8_t vs, uint8_t vt,
               uint8_t size) {
    float s[4], t[4], d[4];
    vfpu_read_vector(s, size, vs, ctx->vfpu);
    vfpu_read_vector(t, size, vt, ctx->vfpu);
    vfpu_apply_prefix_st(s, ctx->vfpu_ctrl[VFPU_CTRL_SPREFIX],
                         size);
    vfpu_apply_prefix_st(t, ctx->vfpu_ctrl[VFPU_CTRL_TPREFIX],
                         size);
    for (int i = 0; i < size; i++) {
        d[i] = (s[i] < t[i]) ? 1.0f : 0.0f;
    }
    vfpu_apply_prefix_d(d, ctx->vfpu_ctrl[VFPU_CTRL_DPREFIX],
                        size);
    vfpu_write_vector(d, size, vd, ctx->vfpu,
                      ctx->vfpu_ctrl[VFPU_CTRL_DPREFIX]);
    vfpu_eat_prefixes(ctx);
}

// ---------------------------------------------------------------------------
// Reduction / special binary ops
// ---------------------------------------------------------------------------

void vfpu_vdot(recomp_context* ctx, uint8_t*,
               uint8_t vd, uint8_t vs, uint8_t vt,
               uint8_t size) {
    float s[4], t[4], d[4];
    vfpu_read_vector(s, size, vs, ctx->vfpu);
    vfpu_read_vector(t, size, vt, ctx->vfpu);
    vfpu_apply_prefix_st(s, ctx->vfpu_ctrl[VFPU_CTRL_SPREFIX],
                         size);
    vfpu_apply_prefix_st(t, ctx->vfpu_ctrl[VFPU_CTRL_TPREFIX],
                         size);
    float sum = 0.0f;
    for (int i = 0; i < size; i++) sum += s[i] * t[i];
    d[0] = sum;
    vfpu_apply_prefix_d(d, ctx->vfpu_ctrl[VFPU_CTRL_DPREFIX], 1);
    vfpu_write_vector(d, 1, vd, ctx->vfpu,
                      ctx->vfpu_ctrl[VFPU_CTRL_DPREFIX]);
    vfpu_eat_prefixes(ctx);
}

void vfpu_vscl(recomp_context* ctx, uint8_t*,
               uint8_t vd, uint8_t vs, uint8_t vt,
               uint8_t size) {
    float s[4], t[4], d[4];
    vfpu_read_vector(s, size, vs, ctx->vfpu);
    vfpu_read_vector(t, 1, vt, ctx->vfpu);
    vfpu_apply_prefix_st(s, ctx->vfpu_ctrl[VFPU_CTRL_SPREFIX],
                         size);
    vfpu_apply_prefix_st(t, ctx->vfpu_ctrl[VFPU_CTRL_TPREFIX],
                         1);
    for (int i = 0; i < size; i++) d[i] = s[i] * t[0];
    vfpu_apply_prefix_d(d, ctx->vfpu_ctrl[VFPU_CTRL_DPREFIX],
                        size);
    vfpu_write_vector(d, size, vd, ctx->vfpu,
                      ctx->vfpu_ctrl[VFPU_CTRL_DPREFIX]);
    vfpu_eat_prefixes(ctx);
}

void vfpu_vhdp(recomp_context* ctx, uint8_t*,
               uint8_t vd, uint8_t vs, uint8_t vt,
               uint8_t size) {
    float s[4], t[4], d[4];
    vfpu_read_vector(s, size, vs, ctx->vfpu);
    vfpu_read_vector(t, size, vt, ctx->vfpu);
    vfpu_apply_prefix_st(s, ctx->vfpu_ctrl[VFPU_CTRL_SPREFIX],
                         size);
    vfpu_apply_prefix_st(t, ctx->vfpu_ctrl[VFPU_CTRL_TPREFIX],
                         size);
    // Homogeneous dot: last source element replaced with 1.0
    float sum = 0.0f;
    for (int i = 0; i < size - 1; i++) sum += s[i] * t[i];
    sum += t[size - 1];  // w component * 1.0
    d[0] = sum;
    vfpu_apply_prefix_d(d, ctx->vfpu_ctrl[VFPU_CTRL_DPREFIX], 1);
    vfpu_write_vector(d, 1, vd, ctx->vfpu,
                      ctx->vfpu_ctrl[VFPU_CTRL_DPREFIX]);
    vfpu_eat_prefixes(ctx);
}

void vfpu_vcrs(recomp_context* ctx, uint8_t*,
               uint8_t vd, uint8_t vs, uint8_t vt,
               uint8_t /*size*/) {
    // Cross product (triple only)
    float s[4], t[4], d[4];
    vfpu_read_vector(s, 3, vs, ctx->vfpu);
    vfpu_read_vector(t, 3, vt, ctx->vfpu);
    vfpu_apply_prefix_st(s, ctx->vfpu_ctrl[VFPU_CTRL_SPREFIX],
                         3);
    vfpu_apply_prefix_st(t, ctx->vfpu_ctrl[VFPU_CTRL_TPREFIX],
                         3);
    d[0] = s[1] * t[2] - s[2] * t[1];
    d[1] = s[2] * t[0] - s[0] * t[2];
    d[2] = s[0] * t[1] - s[1] * t[0];
    vfpu_apply_prefix_d(d, ctx->vfpu_ctrl[VFPU_CTRL_DPREFIX],
                        3);
    vfpu_write_vector(d, 3, vd, ctx->vfpu,
                      ctx->vfpu_ctrl[VFPU_CTRL_DPREFIX]);
    vfpu_eat_prefixes(ctx);
}

void vfpu_vdet(recomp_context* ctx, uint8_t*,
               uint8_t vd, uint8_t vs, uint8_t vt,
               uint8_t /*size*/) {
    // Determinant (pair only)
    float s[4], t[4], d[4];
    vfpu_read_vector(s, 2, vs, ctx->vfpu);
    vfpu_read_vector(t, 2, vt, ctx->vfpu);
    vfpu_apply_prefix_st(s, ctx->vfpu_ctrl[VFPU_CTRL_SPREFIX],
                         2);
    vfpu_apply_prefix_st(t, ctx->vfpu_ctrl[VFPU_CTRL_TPREFIX],
                         2);
    d[0] = s[0] * t[1] - s[1] * t[0];
    vfpu_apply_prefix_d(d, ctx->vfpu_ctrl[VFPU_CTRL_DPREFIX], 1);
    vfpu_write_vector(d, 1, vd, ctx->vfpu,
                      ctx->vfpu_ctrl[VFPU_CTRL_DPREFIX]);
    vfpu_eat_prefixes(ctx);
}

// ---------------------------------------------------------------------------
// Compare / conditional
// ---------------------------------------------------------------------------

void vfpu_vcmp(recomp_context* ctx, uint8_t*,
               uint8_t vs, uint8_t vt, uint8_t cond,
               uint8_t size) {
    float s[4], t[4];
    vfpu_read_vector(s, size, vs, ctx->vfpu);
    vfpu_read_vector(t, size, vt, ctx->vfpu);
    vfpu_apply_prefix_st(s, ctx->vfpu_ctrl[VFPU_CTRL_SPREFIX],
                         size);
    vfpu_apply_prefix_st(t, ctx->vfpu_ctrl[VFPU_CTRL_TPREFIX],
                         size);

    uint32_t cc = 0;
    int or_val = 0;
    int and_val = 1;

    for (int i = 0; i < size; i++) {
        bool result = false;
        switch (cond) {
        case 0:  // FL (always false)
            result = false;
            break;
        case 1:  // EQ
            result = (s[i] == t[i]);
            break;
        case 2:  // LT
            result = (s[i] < t[i]);
            break;
        case 3:  // LE
            result = (s[i] <= t[i]);
            break;
        case 4:  // TR (always true)
            result = true;
            break;
        case 5:  // NE
            result = (s[i] != t[i]);
            break;
        case 6:  // GE
            result = (s[i] >= t[i]);
            break;
        case 7:  // GT
            result = (s[i] > t[i]);
            break;
        case 8:  // EZ (equal to zero)
            result = (s[i] == 0.0f);
            break;
        case 9:  // EN (either NaN)
            result = std::isnan(s[i]) || std::isnan(t[i]);
            break;
        case 10: // EI (either infinity)
            result = std::isinf(s[i]) || std::isinf(t[i]);
            break;
        case 11: // ES (either NaN or infinity)
            result = std::isnan(s[i]) || std::isinf(s[i])
                  || std::isnan(t[i]) || std::isinf(t[i]);
            break;
        case 12: // NZ (not zero)
            result = (s[i] != 0.0f);
            break;
        case 13: // NN (neither NaN)
            result = !std::isnan(s[i]) && !std::isnan(t[i]);
            break;
        case 14: // NI (neither infinity)
            result = !std::isinf(s[i]) && !std::isinf(t[i]);
            break;
        case 15: // NS (neither NaN nor infinity)
            result = !std::isnan(s[i]) && !std::isinf(s[i])
                  && !std::isnan(t[i]) && !std::isinf(t[i]);
            break;
        }
        if (result) {
            cc |= (1u << i);
            or_val = 1;
        } else {
            and_val = 0;
        }
    }

    // Bit 4 = OR of all per-element results
    if (or_val) cc |= (1u << 4);
    // Bit 5 = AND of all per-element results
    if (and_val) cc |= (1u << 5);

    ctx->vfpu_ctrl[VFPU_CTRL_CC] = cc;
    vfpu_eat_prefixes(ctx);
}

void vfpu_vcmov(recomp_context* ctx, uint8_t*,
                uint8_t vd, uint8_t vs, uint8_t cc_field,
                uint8_t size) {
    // cc_field packs the PSP vcmov operands as decoded by the Rust
    // decoder: imm3 = bits[2:0] (CC bit selector), tf = bit[3]
    // (the true/false sense bit, op[19]). Matches PPSSPP Int_Vcmov:
    // the conditional move fires when ((CC >> imm3) & 1) == !tf.
    const int imm3 = cc_field & 7;
    const bool tf = (cc_field >> 3) & 1;

    float s[4], d[4];
    vfpu_read_vector(s, size, vs, ctx->vfpu);
    vfpu_apply_prefix_st(s, ctx->vfpu_ctrl[VFPU_CTRL_SPREFIX],
                         size);
    // D is read as the T operand and the T prefix applies to it.
    vfpu_read_vector(d, size, vd, ctx->vfpu);
    vfpu_apply_prefix_st(d, ctx->vfpu_ctrl[VFPU_CTRL_TPREFIX],
                         size);

    uint32_t cc = ctx->vfpu_ctrl[VFPU_CTRL_CC];

    if (imm3 < 6) {
        if ((int)((cc >> imm3) & 1) == (int)(!tf)) {
            for (int i = 0; i < size; i++) d[i] = s[i];
        }
    } else if (imm3 == 6) {
        // Per-element: move lane i when CC[i] matches the tf sense.
        for (int i = 0; i < size; i++) {
            if ((int)((cc >> i) & 1) == (int)(!tf)) d[i] = s[i];
        }
    }
    // imm3 == 7 is invalid on hardware (PPSSPP logs and no-ops).

    vfpu_apply_prefix_d(d, ctx->vfpu_ctrl[VFPU_CTRL_DPREFIX],
                        size);
    vfpu_write_vector(d, size, vd, ctx->vfpu,
                      ctx->vfpu_ctrl[VFPU_CTRL_DPREFIX]);
    vfpu_eat_prefixes(ctx);
}
