#include "psp_vfpu.h"
#include "recomp.h"
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <cstdint>

// ---------------------------------------------------------------------------
// NaN-aware clamp helper
// ---------------------------------------------------------------------------

static inline float nanclamp_local(float f, float lo, float hi) {
    float r = (f <= lo) ? lo : f;
    r = (r >= hi) ? hi : r;
    return r;
}

// ---------------------------------------------------------------------------
// Sort operations (partial sort steps on quad registers)
// ---------------------------------------------------------------------------

void vfpu_vsrt1(recomp_context* ctx, uint8_t*,
                uint8_t vd, uint8_t vs, uint8_t size) {
    float s[4], d[4];
    vfpu_read_vector(s, size, vs, ctx->vfpu);
    vfpu_apply_prefix_st(s, ctx->vfpu_ctrl[VFPU_CTRL_SPREFIX],
                         size);
    // Sort: swap pairs (0,1) and (2,3) ascending
    d[0] = std::fminf(s[0], s[1]);
    d[1] = std::fmaxf(s[0], s[1]);
    if (size >= 4) {
        d[2] = std::fminf(s[2], s[3]);
        d[3] = std::fmaxf(s[2], s[3]);
    } else {
        for (int i = 2; i < size; i++) d[i] = s[i];
    }
    vfpu_write_vector(d, size, vd, ctx->vfpu, 0);
    vfpu_eat_prefixes(ctx);
}

void vfpu_vsrt2(recomp_context* ctx, uint8_t*,
                uint8_t vd, uint8_t vs, uint8_t size) {
    float s[4], d[4];
    vfpu_read_vector(s, size, vs, ctx->vfpu);
    vfpu_apply_prefix_st(s, ctx->vfpu_ctrl[VFPU_CTRL_SPREFIX],
                         size);
    // Sort: swap pairs (0,1) and (2,3) descending
    d[0] = std::fmaxf(s[0], s[1]);
    d[1] = std::fminf(s[0], s[1]);
    if (size >= 4) {
        d[2] = std::fmaxf(s[2], s[3]);
        d[3] = std::fminf(s[2], s[3]);
    } else {
        for (int i = 2; i < size; i++) d[i] = s[i];
    }
    vfpu_write_vector(d, size, vd, ctx->vfpu, 0);
    vfpu_eat_prefixes(ctx);
}

void vfpu_vsrt3(recomp_context* ctx, uint8_t*,
                uint8_t vd, uint8_t vs, uint8_t size) {
    float s[4], d[4];
    vfpu_read_vector(s, size, vs, ctx->vfpu);
    vfpu_apply_prefix_st(s, ctx->vfpu_ctrl[VFPU_CTRL_SPREFIX],
                         size);
    // Sort: max(0,3), min(1,2), max(1,2), min(0,3)
    if (size >= 4) {
        d[0] = std::fmaxf(s[0], s[3]);
        d[1] = std::fminf(s[1], s[2]);
        d[2] = std::fmaxf(s[1], s[2]);
        d[3] = std::fminf(s[0], s[3]);
    } else {
        for (int i = 0; i < size; i++) d[i] = s[i];
    }
    vfpu_write_vector(d, size, vd, ctx->vfpu, 0);
    vfpu_eat_prefixes(ctx);
}

void vfpu_vsrt4(recomp_context* ctx, uint8_t*,
                uint8_t vd, uint8_t vs, uint8_t size) {
    float s[4], d[4];
    vfpu_read_vector(s, size, vs, ctx->vfpu);
    vfpu_apply_prefix_st(s, ctx->vfpu_ctrl[VFPU_CTRL_SPREFIX],
                         size);
    // Sort: min(0,3), max(1,2), min(1,2), max(0,3)
    if (size >= 4) {
        d[0] = std::fminf(s[0], s[3]);
        d[1] = std::fmaxf(s[1], s[2]);
        d[2] = std::fminf(s[1], s[2]);
        d[3] = std::fmaxf(s[0], s[3]);
    } else {
        for (int i = 0; i < size; i++) d[i] = s[i];
    }
    vfpu_write_vector(d, size, vd, ctx->vfpu, 0);
    vfpu_eat_prefixes(ctx);
}

// ---------------------------------------------------------------------------
// Butterfly operations
// ---------------------------------------------------------------------------

void vfpu_vbfy1(recomp_context* ctx, uint8_t*,
                uint8_t vd, uint8_t vs, uint8_t size) {
    float s[4], d[4];
    vfpu_read_vector(s, size, vs, ctx->vfpu);
    vfpu_apply_prefix_st(s, ctx->vfpu_ctrl[VFPU_CTRL_SPREFIX],
                         size);
    // Butterfly: d[0]=s[0]+s[1], d[1]=s[0]-s[1]
    //            d[2]=s[2]+s[3], d[3]=s[2]-s[3]
    for (int i = 0; i < size; i += 2) {
        if (i + 1 < size) {
            d[i]     = s[i] + s[i + 1];
            d[i + 1] = s[i] - s[i + 1];
        } else {
            d[i] = s[i];
        }
    }
    vfpu_write_vector(d, size, vd, ctx->vfpu, 0);
    vfpu_eat_prefixes(ctx);
}

void vfpu_vbfy2(recomp_context* ctx, uint8_t*,
                uint8_t vd, uint8_t vs, uint8_t size) {
    float s[4], d[4];
    vfpu_read_vector(s, size, vs, ctx->vfpu);
    vfpu_apply_prefix_st(s, ctx->vfpu_ctrl[VFPU_CTRL_SPREFIX],
                         size);
    // Butterfly2: d[0]=s[0]+s[2], d[1]=s[1]+s[3],
    //             d[2]=s[0]-s[2], d[3]=s[1]-s[3]
    if (size >= 4) {
        d[0] = s[0] + s[2];
        d[1] = s[1] + s[3];
        d[2] = s[0] - s[2];
        d[3] = s[1] - s[3];
    } else {
        for (int i = 0; i < size; i++) d[i] = s[i];
    }
    vfpu_write_vector(d, size, vd, ctx->vfpu, 0);
    vfpu_eat_prefixes(ctx);
}

// ---------------------------------------------------------------------------
// Reduction operations
// ---------------------------------------------------------------------------

void vfpu_vsocp(recomp_context* ctx, uint8_t*,
                uint8_t vd, uint8_t vs, uint8_t size) {
    // Saturating opposite clamp pair
    float s[4];
    int in_size = size / 2;
    if (in_size < 1) in_size = 1;
    vfpu_read_vector(s, in_size, vs, ctx->vfpu);
    vfpu_apply_prefix_st(s, ctx->vfpu_ctrl[VFPU_CTRL_SPREFIX],
                         in_size);
    float d[4];
    for (int i = 0; i < in_size && i * 2 + 1 < size; i++) {
        float clamped = nanclamp_local(s[i], 0.0f, 1.0f);
        d[i * 2]     = 1.0f - clamped;
        d[i * 2 + 1] = clamped;
    }
    vfpu_write_vector(d, size, vd, ctx->vfpu, 0);
    vfpu_eat_prefixes(ctx);
}

void vfpu_vfad(recomp_context* ctx, uint8_t*,
               uint8_t vd, uint8_t vs, uint8_t size) {
    // Sum of all elements
    float s[4];
    vfpu_read_vector(s, size, vs, ctx->vfpu);
    vfpu_apply_prefix_st(s, ctx->vfpu_ctrl[VFPU_CTRL_SPREFIX],
                         size);
    float sum = 0.0f;
    for (int i = 0; i < size; i++) sum += s[i];
    float d = sum;
    vfpu_apply_prefix_d(&d,
        ctx->vfpu_ctrl[VFPU_CTRL_DPREFIX], 1);
    vfpu_write_vector(&d, 1, vd, ctx->vfpu,
        ctx->vfpu_ctrl[VFPU_CTRL_DPREFIX]);
    vfpu_eat_prefixes(ctx);
}

void vfpu_vavg(recomp_context* ctx, uint8_t*,
               uint8_t vd, uint8_t vs, uint8_t size) {
    // Average of all elements
    float s[4];
    vfpu_read_vector(s, size, vs, ctx->vfpu);
    vfpu_apply_prefix_st(s, ctx->vfpu_ctrl[VFPU_CTRL_SPREFIX],
                         size);
    float sum = 0.0f;
    for (int i = 0; i < size; i++) sum += s[i];
    float d = sum / static_cast<float>(size);
    vfpu_apply_prefix_d(&d,
        ctx->vfpu_ctrl[VFPU_CTRL_DPREFIX], 1);
    vfpu_write_vector(&d, 1, vd, ctx->vfpu,
        ctx->vfpu_ctrl[VFPU_CTRL_DPREFIX]);
    vfpu_eat_prefixes(ctx);
}

// ---------------------------------------------------------------------------
// Random number operations
// ---------------------------------------------------------------------------

// Simple VFPU random state (not cryptographically secure)
static uint32_t vfpu_rng_state = 0x3F800001u;

static uint32_t vfpu_rng_next() {
    // xorshift32 PRNG
    vfpu_rng_state ^= vfpu_rng_state << 13;
    vfpu_rng_state ^= vfpu_rng_state >> 17;
    vfpu_rng_state ^= vfpu_rng_state << 5;
    return vfpu_rng_state;
}

void vfpu_vrnds(recomp_context* ctx, uint8_t*,
                uint8_t vs) {
    float s;
    vfpu_read_vector(&s, 1, vs, ctx->vfpu);
    uint32_t u;
    std::memcpy(&u, &s, 4);
    if (u != 0) vfpu_rng_state = u;
    vfpu_eat_prefixes(ctx);
}

void vfpu_vrndi(recomp_context* ctx, uint8_t*,
                uint8_t vd, uint8_t size) {
    float d[4];
    for (int i = 0; i < size; i++) {
        uint32_t r = vfpu_rng_next();
        std::memcpy(&d[i], &r, 4);
    }
    vfpu_write_vector(d, size, vd, ctx->vfpu, 0);
    vfpu_eat_prefixes(ctx);
}

void vfpu_vrndf1(recomp_context* ctx, uint8_t*,
                 uint8_t vd, uint8_t size) {
    float d[4];
    for (int i = 0; i < size; i++) {
        // Random float in [1.0, 2.0)
        uint32_t r = vfpu_rng_next();
        r = (r & 0x007FFFFFu) | 0x3F800000u;
        std::memcpy(&d[i], &r, 4);
    }
    vfpu_write_vector(d, size, vd, ctx->vfpu, 0);
    vfpu_eat_prefixes(ctx);
}

void vfpu_vrndf2(recomp_context* ctx, uint8_t*,
                 uint8_t vd, uint8_t size) {
    float d[4];
    for (int i = 0; i < size; i++) {
        // Random float in [2.0, 4.0)
        uint32_t r = vfpu_rng_next();
        r = (r & 0x007FFFFFu) | 0x40000000u;
        std::memcpy(&d[i], &r, 4);
    }
    vfpu_write_vector(d, size, vd, ctx->vfpu, 0);
    vfpu_eat_prefixes(ctx);
}

// ---------------------------------------------------------------------------
// Wrap by negative (vwbn)
// ---------------------------------------------------------------------------

void vfpu_vwbn(recomp_context* ctx, uint8_t*,
               uint8_t vd, uint8_t vs, uint8_t imm8,
               uint8_t size) {
    float s[4], d[4];
    vfpu_read_vector(s, size, vs, ctx->vfpu);
    vfpu_apply_prefix_st(s, ctx->vfpu_ctrl[VFPU_CTRL_SPREFIX],
                         size);
    // Wrap by negative: implementation based on PPSSPP
    float bn = static_cast<float>(1 << imm8);
    for (int i = 0; i < size; i++) {
        float val = s[i];
        if (val < 0.0f) {
            d[i] = val + bn;
        } else if (val >= bn) {
            d[i] = val - bn;
        } else {
            d[i] = val;
        }
    }
    vfpu_apply_prefix_d(d, ctx->vfpu_ctrl[VFPU_CTRL_DPREFIX],
                        size);
    vfpu_write_vector(d, size, vd, ctx->vfpu,
                      ctx->vfpu_ctrl[VFPU_CTRL_DPREFIX]);
    vfpu_eat_prefixes(ctx);
}
