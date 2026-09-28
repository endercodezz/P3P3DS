#include "psp_vfpu.h"
#include "recomp.h"
#include <cmath>
#include <cstring>
#include <cstdint>

// ---------------------------------------------------------------------------
// Float-to-integer conversions
// ---------------------------------------------------------------------------

void vfpu_vf2in(recomp_context* ctx, uint8_t*,
                uint8_t vd, uint8_t vs, uint8_t imm5,
                uint8_t size) {
    float s[4];
    vfpu_read_vector(s, size, vs, ctx->vfpu);
    vfpu_apply_prefix_st(s, ctx->vfpu_ctrl[VFPU_CTRL_SPREFIX],
                         size);
    float scale = static_cast<float>(1 << imm5);
    float d[4];
    for (int i = 0; i < size; i++) {
        float v = s[i] * scale;
        int32_t iv = static_cast<int32_t>(std::roundf(v));
        std::memcpy(&d[i], &iv, 4);
    }
    // No DPREFIX for integer output
    vfpu_write_vector(d, size, vd, ctx->vfpu, 0);
    vfpu_eat_prefixes(ctx);
}

void vfpu_vf2iz(recomp_context* ctx, uint8_t*,
                uint8_t vd, uint8_t vs, uint8_t imm5,
                uint8_t size) {
    float s[4];
    vfpu_read_vector(s, size, vs, ctx->vfpu);
    vfpu_apply_prefix_st(s, ctx->vfpu_ctrl[VFPU_CTRL_SPREFIX],
                         size);
    float scale = static_cast<float>(1 << imm5);
    float d[4];
    for (int i = 0; i < size; i++) {
        float v = s[i] * scale;
        int32_t iv = static_cast<int32_t>(std::truncf(v));
        std::memcpy(&d[i], &iv, 4);
    }
    vfpu_write_vector(d, size, vd, ctx->vfpu, 0);
    vfpu_eat_prefixes(ctx);
}

void vfpu_vf2iu(recomp_context* ctx, uint8_t*,
                uint8_t vd, uint8_t vs, uint8_t imm5,
                uint8_t size) {
    float s[4];
    vfpu_read_vector(s, size, vs, ctx->vfpu);
    vfpu_apply_prefix_st(s, ctx->vfpu_ctrl[VFPU_CTRL_SPREFIX],
                         size);
    float scale = static_cast<float>(1 << imm5);
    float d[4];
    for (int i = 0; i < size; i++) {
        float v = s[i] * scale;
        int32_t iv = static_cast<int32_t>(std::ceilf(v));
        std::memcpy(&d[i], &iv, 4);
    }
    vfpu_write_vector(d, size, vd, ctx->vfpu, 0);
    vfpu_eat_prefixes(ctx);
}

void vfpu_vf2id(recomp_context* ctx, uint8_t*,
                uint8_t vd, uint8_t vs, uint8_t imm5,
                uint8_t size) {
    float s[4];
    vfpu_read_vector(s, size, vs, ctx->vfpu);
    vfpu_apply_prefix_st(s, ctx->vfpu_ctrl[VFPU_CTRL_SPREFIX],
                         size);
    float scale = static_cast<float>(1 << imm5);
    float d[4];
    for (int i = 0; i < size; i++) {
        float v = s[i] * scale;
        int32_t iv = static_cast<int32_t>(std::floorf(v));
        std::memcpy(&d[i], &iv, 4);
    }
    vfpu_write_vector(d, size, vd, ctx->vfpu, 0);
    vfpu_eat_prefixes(ctx);
}

// ---------------------------------------------------------------------------
// Integer-to-float conversion
// ---------------------------------------------------------------------------

void vfpu_vi2f(recomp_context* ctx, uint8_t*,
               uint8_t vd, uint8_t vs, uint8_t imm5,
               uint8_t size) {
    float s[4];
    vfpu_read_vector(s, size, vs, ctx->vfpu);
    float scale = 1.0f / static_cast<float>(1 << imm5);
    float d[4];
    for (int i = 0; i < size; i++) {
        int32_t iv;
        std::memcpy(&iv, &s[i], 4);
        d[i] = static_cast<float>(iv) * scale;
    }
    vfpu_apply_prefix_d(d, ctx->vfpu_ctrl[VFPU_CTRL_DPREFIX],
                        size);
    vfpu_write_vector(d, size, vd, ctx->vfpu,
                      ctx->vfpu_ctrl[VFPU_CTRL_DPREFIX]);
    vfpu_eat_prefixes(ctx);
}

// ---------------------------------------------------------------------------
// Integer pack/unpack operations
// ---------------------------------------------------------------------------

void vfpu_vi2uc(recomp_context* ctx, uint8_t*,
                uint8_t vd, uint8_t vs, uint8_t /*size*/) {
    float s[4];
    vfpu_read_vector(s, 4, vs, ctx->vfpu);
    uint32_t result = 0;
    for (int i = 0; i < 4; i++) {
        int32_t iv;
        std::memcpy(&iv, &s[i], 4);
        int clamped = (iv < 0) ? 0 : (iv >> 23);
        if (clamped > 255) clamped = 255;
        result |= (static_cast<uint32_t>(clamped) << (i * 8));
    }
    float d;
    std::memcpy(&d, &result, 4);
    vfpu_write_vector(&d, 1, vd, ctx->vfpu, 0);
    vfpu_eat_prefixes(ctx);
}

void vfpu_vi2c(recomp_context* ctx, uint8_t*,
               uint8_t vd, uint8_t vs, uint8_t /*size*/) {
    float s[4];
    vfpu_read_vector(s, 4, vs, ctx->vfpu);
    uint32_t result = 0;
    for (int i = 0; i < 4; i++) {
        int32_t iv;
        std::memcpy(&iv, &s[i], 4);
        result |= ((static_cast<uint32_t>(iv) >> 24) & 0xFF)
                  << (i * 8);
    }
    float d;
    std::memcpy(&d, &result, 4);
    vfpu_write_vector(&d, 1, vd, ctx->vfpu, 0);
    vfpu_eat_prefixes(ctx);
}

void vfpu_vi2us(recomp_context* ctx, uint8_t*,
                uint8_t vd, uint8_t vs, uint8_t /*size*/) {
    float s[4];
    vfpu_read_vector(s, 2, vs, ctx->vfpu);
    uint32_t result = 0;
    for (int i = 0; i < 2; i++) {
        int32_t iv;
        std::memcpy(&iv, &s[i], 4);
        int clamped = (iv < 0) ? 0 : (iv >> 15);
        if (clamped > 65535) clamped = 65535;
        result |= (static_cast<uint32_t>(clamped) << (i * 16));
    }
    float d;
    std::memcpy(&d, &result, 4);
    vfpu_write_vector(&d, 1, vd, ctx->vfpu, 0);
    vfpu_eat_prefixes(ctx);
}

void vfpu_vi2s(recomp_context* ctx, uint8_t*,
               uint8_t vd, uint8_t vs, uint8_t /*size*/) {
    float s[4];
    vfpu_read_vector(s, 2, vs, ctx->vfpu);
    uint32_t result = 0;
    for (int i = 0; i < 2; i++) {
        int32_t iv;
        std::memcpy(&iv, &s[i], 4);
        result |= ((static_cast<uint32_t>(iv) >> 16) & 0xFFFF)
                  << (i * 16);
    }
    float d;
    std::memcpy(&d, &result, 4);
    vfpu_write_vector(&d, 1, vd, ctx->vfpu, 0);
    vfpu_eat_prefixes(ctx);
}

void vfpu_vuc2i(recomp_context* ctx, uint8_t*,
                uint8_t vd, uint8_t vs, uint8_t /*size*/) {
    float s_in;
    vfpu_read_vector(&s_in, 1, vs, ctx->vfpu);
    uint32_t u;
    std::memcpy(&u, &s_in, 4);
    float d[4];
    for (int i = 0; i < 4; i++) {
        uint32_t val = ((u >> (i * 8)) & 0xFF) << 24;
        val |= val >> 8;  // replicate
        std::memcpy(&d[i], &val, 4);
    }
    vfpu_write_vector(d, 4, vd, ctx->vfpu, 0);
    vfpu_eat_prefixes(ctx);
}

void vfpu_vc2i(recomp_context* ctx, uint8_t*,
               uint8_t vd, uint8_t vs, uint8_t /*size*/) {
    float s_in;
    vfpu_read_vector(&s_in, 1, vs, ctx->vfpu);
    uint32_t u;
    std::memcpy(&u, &s_in, 4);
    float d[4];
    for (int i = 0; i < 4; i++) {
        uint32_t val = ((u >> (24 - i * 8)) & 0xFF) << 24;
        std::memcpy(&d[i], &val, 4);
    }
    vfpu_write_vector(d, 4, vd, ctx->vfpu, 0);
    vfpu_eat_prefixes(ctx);
}

void vfpu_vus2i(recomp_context* ctx, uint8_t*,
                uint8_t vd, uint8_t vs, uint8_t /*size*/) {
    float s_in[2];
    vfpu_read_vector(s_in, 2, vs, ctx->vfpu);
    float d[4];
    for (int i = 0; i < 2; i++) {
        uint32_t u;
        std::memcpy(&u, &s_in[i], 4);
        uint32_t lo = (u & 0xFFFF) << 15;
        uint32_t hi = (u >> 16) << 15;
        std::memcpy(&d[i * 2], &lo, 4);
        std::memcpy(&d[i * 2 + 1], &hi, 4);
    }
    vfpu_write_vector(d, 4, vd, ctx->vfpu, 0);
    vfpu_eat_prefixes(ctx);
}

void vfpu_vs2i(recomp_context* ctx, uint8_t*,
               uint8_t vd, uint8_t vs, uint8_t /*size*/) {
    float s_in[2];
    vfpu_read_vector(s_in, 2, vs, ctx->vfpu);
    float d[4];
    for (int i = 0; i < 2; i++) {
        uint32_t u;
        std::memcpy(&u, &s_in[i], 4);
        uint32_t lo = (u & 0xFFFF) << 16;
        uint32_t hi = u & 0xFFFF0000u;
        std::memcpy(&d[i * 2], &lo, 4);
        std::memcpy(&d[i * 2 + 1], &hi, 4);
    }
    vfpu_write_vector(d, 4, vd, ctx->vfpu, 0);
    vfpu_eat_prefixes(ctx);
}

// ---------------------------------------------------------------------------
// Half-float conversion
// ---------------------------------------------------------------------------

/// Convert float32 to float16.
static uint16_t float32_to_float16(float f) {
    uint32_t u;
    std::memcpy(&u, &f, 4);
    uint16_t sign = static_cast<uint16_t>((u >> 16) & 0x8000);
    int32_t exp = static_cast<int32_t>((u >> 23) & 0xFF) - 127;
    uint32_t mant = u & 0x007FFFFFu;

    if (exp > 15) {
        return sign | 0x7C00;  // infinity
    }
    if (exp < -14) {
        return sign;  // zero / underflow
    }
    return sign
         | static_cast<uint16_t>((exp + 15) << 10)
         | static_cast<uint16_t>(mant >> 13);
}

void vfpu_vf2h(recomp_context* ctx, uint8_t*,
               uint8_t vd, uint8_t vs, uint8_t size) {
    float s[4];
    vfpu_read_vector(s, size, vs, ctx->vfpu);
    vfpu_apply_prefix_st(s, ctx->vfpu_ctrl[VFPU_CTRL_SPREFIX],
                         size);
    // Pack pairs of half-floats into 32-bit words
    int out_count = (size + 1) / 2;
    float d[4] = {};
    for (int i = 0; i < size; i++) {
        uint16_t h = float32_to_float16(s[i]);
        uint32_t existing;
        std::memcpy(&existing, &d[i / 2], 4);
        if (i & 1) {
            existing |= (static_cast<uint32_t>(h) << 16);
        } else {
            existing = h;
        }
        std::memcpy(&d[i / 2], &existing, 4);
    }
    vfpu_write_vector(d, out_count, vd, ctx->vfpu, 0);
    vfpu_eat_prefixes(ctx);
}

/// Convert float16 to float32 (local helper).
static float f16_to_f32(uint16_t h) {
    uint32_t sign = (static_cast<uint32_t>(h) >> 15) & 1;
    uint32_t exp  = (static_cast<uint32_t>(h) >> 10) & 0x1F;
    uint32_t mant = static_cast<uint32_t>(h) & 0x3FF;

    uint32_t result;
    if (exp == 0) {
        if (mant == 0) {
            result = sign << 31;
        } else {
            exp = 1;
            while (!(mant & 0x400)) { mant <<= 1; exp--; }
            mant &= 0x3FF;
            result = (sign << 31)
                   | ((exp + 127 - 15) << 23)
                   | (mant << 13);
        }
    } else if (exp == 0x1F) {
        result = (sign << 31) | 0x7F800000u | (mant << 13);
    } else {
        result = (sign << 31)
               | ((exp + 127 - 15) << 23)
               | (mant << 13);
    }
    float f;
    std::memcpy(&f, &result, 4);
    return f;
}

void vfpu_vh2f(recomp_context* ctx, uint8_t*,
               uint8_t vd, uint8_t vs, uint8_t size) {
    float s[4];
    int in_count = (size + 1) / 2;
    vfpu_read_vector(s, in_count, vs, ctx->vfpu);
    float d[4];
    for (int i = 0; i < size; i++) {
        uint32_t u;
        std::memcpy(&u, &s[i / 2], 4);
        uint16_t h = (i & 1)
            ? static_cast<uint16_t>(u >> 16)
            : static_cast<uint16_t>(u & 0xFFFF);
        d[i] = f16_to_f32(h);
    }
    vfpu_write_vector(d, size, vd, ctx->vfpu, 0);
    vfpu_eat_prefixes(ctx);
}
