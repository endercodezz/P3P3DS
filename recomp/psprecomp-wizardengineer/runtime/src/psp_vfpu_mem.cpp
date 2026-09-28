#include "psp_vfpu.h"
#include "recomp.h"
#include <cstring>

// Single-register flat index comes from the shared vfpu_single_index()
// inline in psp_vfpu.h (layout: vfpu[mtx*16 + col*4 + row]).

// ---------------------------------------------------------------------------
// Memory load/store operations
// All use 0x07FFFFFFU address masking (via psp_mem_read/write)
// ---------------------------------------------------------------------------

void vfpu_lv_s(recomp_context* ctx, uint8_t* rdram,
               uint8_t vt, uint8_t rs, int16_t offset) {
    uint32_t addr =
        static_cast<uint32_t>(ctx->r[rs]) + offset;
    float val = psp_mem_read<float>(rdram, addr);
    int idx = vfpu_single_index(vt);
    ctx->vfpu[idx] = val;
}

void vfpu_sv_s(recomp_context* ctx, uint8_t* rdram,
               uint8_t vt, uint8_t rs, int16_t offset) {
    uint32_t addr =
        static_cast<uint32_t>(ctx->r[rs]) + offset;
    int idx = vfpu_single_index(vt);
    psp_mem_write<float>(rdram, addr, ctx->vfpu[idx]);
}

void vfpu_lv_q(recomp_context* ctx, uint8_t* rdram,
               uint8_t vt, uint8_t rs, int16_t offset) {
    uint32_t addr =
        static_cast<uint32_t>(ctx->r[rs]) + offset;
    addr &= ~0xFu;  // 16-byte align
    float d[4];
    for (int i = 0; i < 4; i++) {
        d[i] = psp_mem_read<float>(rdram, addr + i * 4);
    }
    vfpu_write_vector(d, 4, vt, ctx->vfpu, 0);
}

void vfpu_sv_q(recomp_context* ctx, uint8_t* rdram,
               uint8_t vt, uint8_t rs, int16_t offset) {
    uint32_t addr =
        static_cast<uint32_t>(ctx->r[rs]) + offset;
    addr &= ~0xFu;  // 16-byte align
    float s[4];
    vfpu_read_vector(s, 4, vt, ctx->vfpu);
    for (int i = 0; i < 4; i++) {
        psp_mem_write<float>(rdram, addr + i * 4, s[i]);
    }
}

void vfpu_lvl_q(recomp_context* ctx, uint8_t* rdram,
                uint8_t vt, uint8_t rs, int16_t offset) {
    uint32_t addr =
        static_cast<uint32_t>(ctx->r[rs]) + offset;
    float d[4];
    vfpu_read_vector(d, 4, vt, ctx->vfpu);  // read current
    int shift = (addr >> 2) & 3;
    uint32_t aligned = addr & ~0xFu;
    for (int i = shift; i < 4; i++) {
        d[i - shift] =
            psp_mem_read<float>(rdram, aligned + i * 4);
    }
    vfpu_write_vector(d, 4, vt, ctx->vfpu, 0);
}

void vfpu_lvr_q(recomp_context* ctx, uint8_t* rdram,
                uint8_t vt, uint8_t rs, int16_t offset) {
    uint32_t addr =
        static_cast<uint32_t>(ctx->r[rs]) + offset;
    float d[4];
    vfpu_read_vector(d, 4, vt, ctx->vfpu);
    int shift = (addr >> 2) & 3;
    uint32_t aligned = addr & ~0xFu;
    for (int i = 0; i <= shift; i++) {
        d[4 - shift - 1 + i] =
            psp_mem_read<float>(rdram, aligned + i * 4);
    }
    vfpu_write_vector(d, 4, vt, ctx->vfpu, 0);
}

void vfpu_svl_q(recomp_context* ctx, uint8_t* rdram,
                uint8_t vt, uint8_t rs, int16_t offset) {
    uint32_t addr =
        static_cast<uint32_t>(ctx->r[rs]) + offset;
    float s[4];
    vfpu_read_vector(s, 4, vt, ctx->vfpu);
    int shift = (addr >> 2) & 3;
    uint32_t aligned = addr & ~0xFu;
    for (int i = shift; i < 4; i++) {
        psp_mem_write<float>(rdram, aligned + i * 4,
                             s[i - shift]);
    }
}

void vfpu_svr_q(recomp_context* ctx, uint8_t* rdram,
                uint8_t vt, uint8_t rs, int16_t offset) {
    uint32_t addr =
        static_cast<uint32_t>(ctx->r[rs]) + offset;
    float s[4];
    vfpu_read_vector(s, 4, vt, ctx->vfpu);
    int shift = (addr >> 2) & 3;
    uint32_t aligned = addr & ~0xFu;
    for (int i = 0; i <= shift; i++) {
        psp_mem_write<float>(rdram, aligned + i * 4,
                             s[4 - shift - 1 + i]);
    }
}
