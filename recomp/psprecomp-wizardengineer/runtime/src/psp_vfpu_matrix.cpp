#include "psp_vfpu.h"
#include "recomp.h"
#include <cmath>
#include <cstring>

// ---------------------------------------------------------------------------
// Matrix read/write helpers (file-scope, not exposed in header)
// ---------------------------------------------------------------------------

// Physical layout: vfpu[mtx*16 + col*4 + row] (PPSSPP convention; see
// vfpu_single_index in psp_vfpu.h). Flat matrix arrays here follow
// PPSSPP's ReadMatrix/WriteMatrix convention: rd[j*4 + i] where j steps
// along the register's column axis and i along its row axis.

/// Decode the starting row for a size x size matrix register encoding.
static inline int vfpu_matrix_row(int reg, int size) {
    switch (size) {
    case 2:  return (reg >> 5) & 2;
    case 3:  return (reg >> 6) & 1;
    case 4:  return (reg >> 5) & 2;
    default: return (reg >> 5) & 3;  // 1x1
    }
}

/// Read a matrix (size x size) into a flat array, PPSSPP ReadMatrix.
static void read_matrix(float rd[16], int size, int reg,
                        const float vfpu[128]) {
    int row = vfpu_matrix_row(reg, size);
    int transpose = (size == 1) ? 0 : ((reg >> 5) & 1);
    int mtx = (reg >> 2) & 7;
    int col = reg & 3;

    const float* v = vfpu + mtx * 16;
    if (transpose) {
        for (int j = 0; j < size; j++) {
            for (int i = 0; i < size; i++) {
                int index = ((row + i) & 3) * 4 + ((col + j) & 3);
                rd[j * 4 + i] = v[index];
            }
        }
    } else {
        for (int j = 0; j < size; j++) {
            for (int i = 0; i < size; i++) {
                int index = ((col + j) & 3) * 4 + ((row + i) & 3);
                rd[j * 4 + i] = v[index];
            }
        }
    }
}

/// Write a matrix (size x size) from a flat array, PPSSPP WriteMatrix.
static void write_matrix(const float rd[16], int size, int reg,
                         float vfpu[128]) {
    int row = vfpu_matrix_row(reg, size);
    int transpose = (size == 1) ? 0 : ((reg >> 5) & 1);
    int mtx = (reg >> 2) & 7;
    int col = reg & 3;

    float* v = vfpu + mtx * 16;
    if (transpose) {
        for (int j = 0; j < size; j++) {
            for (int i = 0; i < size; i++) {
                int index = ((row + i) & 3) * 4 + ((col + j) & 3);
                v[index] = rd[j * 4 + i];
            }
        }
    } else {
        for (int j = 0; j < size; j++) {
            for (int i = 0; i < size; i++) {
                int index = ((col + j) & 3) * 4 + ((row + i) & 3);
                v[index] = rd[j * 4 + i];
            }
        }
    }
}

// ---------------------------------------------------------------------------
// Quarter-turn trig helpers (needed for vrot)
// ---------------------------------------------------------------------------

static float vfpu_sin_single(float angle) {
    float reduced = std::fmod(angle, 4.0f);
    if (reduced < 0.0f) reduced += 4.0f;
    if (reduced == 0.0f) return 0.0f;
    if (reduced == 1.0f) return 1.0f;
    if (reduced == 2.0f) return 0.0f;
    if (reduced == 3.0f) return -1.0f;
    return std::sinf(reduced * static_cast<float>(M_PI_2));
}

static float vfpu_cos_single(float angle) {
    float reduced = std::fmod(angle, 4.0f);
    if (reduced < 0.0f) reduced += 4.0f;
    if (reduced == 0.0f) return 1.0f;
    if (reduced == 1.0f) return 0.0f;
    if (reduced == 2.0f) return -1.0f;
    if (reduced == 3.0f) return 0.0f;
    return std::cosf(reduced * static_cast<float>(M_PI_2));
}

// ---------------------------------------------------------------------------
// Per-op prefix-scope helpers
//
// PPSSPP applies the S/T source prefixes and the D destination prefix to
// the matrix's LAST ROW (vmscl, vtfm) or the LAST ELEMENT (vmmul) only;
// every earlier row/element is computed from the raw, unprefixed matrix
// data. (Int_Vmmul/Int_Vmscl/Int_Vtfm.) These helpers fold the DPREFIX
// down to that final scope and apply it before write.
// ---------------------------------------------------------------------------

/// Apply the D prefix to the last ELEMENT of an `n` x `n` matrix, the way
/// PPSSPP `Int_Vmmul` does: only the original element-0 mask/sat bits
/// survive, repositioned onto lane `n-1` of the last row. Returns the
/// collapsed prefix so the caller can honor the last-element write mask.
static uint32_t collapse_last_element_dprefix(int n, uint32_t dprefix) {
    uint32_t lastmask = (dprefix & (1u << 8)) << (n - 1);
    uint32_t lastsat = (dprefix & 3u) << (n + n - 2);
    return lastmask | lastsat;
}

/// Write an `n` x `n` matrix, honoring the per-element write mask of
/// `dprefix` on the LAST ROW only (bit `8+i` disables lane i). Earlier
/// rows are always written; masked last-row lanes keep their prior value
/// (mirrors PPSSPP, where ApplyPrefixD's mask check suppresses the write).
static void write_matrix_masked(const float md[16], int n, int reg,
                                float vfpu[128], uint32_t dprefix) {
    float keep[16];
    read_matrix(keep, n, reg, vfpu);
    float out[16];
    std::memcpy(out, md, sizeof(out));
    for (int i = 0; i < n; i++) {
        if ((dprefix >> (8 + i)) & 1) {
            out[(n - 1) * 4 + i] = keep[(n - 1) * 4 + i];
        }
    }
    write_matrix(out, n, reg, vfpu);
}

// ---------------------------------------------------------------------------
// Matrix multiply (vmmul)
// ---------------------------------------------------------------------------

void vfpu_vmmul(recomp_context* ctx, uint8_t*,
                uint8_t vd, uint8_t vs, uint8_t vt,
                uint8_t size) {
    // PPSSPP Int_Vmmul: read vs as-is (transpose is folded into the
    // summation below), d[a*4+b] = sum_c s[b*4+c] * t[a*4+c]. The S/T
    // prefixes swizzle only the FINAL dot (a==b==n-1); the D prefix
    // applies only to that final element.
    float ms[16] = {}, mt[16] = {}, md[16] = {};
    read_matrix(ms, size, vs, ctx->vfpu);
    read_matrix(mt, size, vt, ctx->vfpu);

    for (int a = 0; a < size; a++) {
        for (int b = 0; b < size; b++) {
            if (a == size - 1 && b == size - 1) {
                vfpu_apply_prefix_st(&ms[b * 4],
                                     ctx->vfpu_ctrl[VFPU_CTRL_SPREFIX], 4);
                vfpu_apply_prefix_st(&mt[a * 4],
                                     ctx->vfpu_ctrl[VFPU_CTRL_TPREFIX], 4);
            }
            float sum = 0.0f;
            int lim = (a == size - 1 && b == size - 1) ? 4 : size;
            for (int c = 0; c < lim; c++) {
                sum += ms[b * 4 + c] * mt[a * 4 + c];
            }
            md[a * 4 + b] = sum;
        }
    }

    uint32_t collapsed = collapse_last_element_dprefix(
        size, ctx->vfpu_ctrl[VFPU_CTRL_DPREFIX]);
    vfpu_apply_prefix_d(&md[(size - 1) * 4], collapsed, size);
    write_matrix_masked(md, size, vd, ctx->vfpu, collapsed);
    vfpu_eat_prefixes(ctx);
}

// ---------------------------------------------------------------------------
// Matrix scale (vmscl)
// ---------------------------------------------------------------------------

void vfpu_vmscl(recomp_context* ctx, uint8_t*,
                uint8_t vd, uint8_t vs, uint8_t vt,
                uint8_t size) {
    // PPSSPP Int_Vmscl: rows 0..n-2 are the raw uniform scale; the S/T/D
    // prefixes apply to the LAST ROW only. The T prefix is the vscl-style
    // lane broadcast of the single scalar, so each last-row element keeps
    // the same scale -- the observable last-row effects are the S swizzle
    // (on the matrix row) and the D prefix (mask + saturate).
    float ms[16], md[16];
    read_matrix(ms, size, vs, ctx->vfpu);
    float scale;
    vfpu_read_vector(&scale, 1, vt, ctx->vfpu);

    for (int row = 0; row < size - 1; row++) {
        for (int col = 0; col < size; col++) {
            md[row * 4 + col] = ms[row * 4 + col] * scale;
        }
    }

    vfpu_apply_prefix_st(&ms[(size - 1) * 4],
                         ctx->vfpu_ctrl[VFPU_CTRL_SPREFIX], 4);
    for (int col = 0; col < size; col++) {
        md[(size - 1) * 4 + col] = ms[(size - 1) * 4 + col] * scale;
    }

    uint32_t dprefix = ctx->vfpu_ctrl[VFPU_CTRL_DPREFIX];
    vfpu_apply_prefix_d(&md[(size - 1) * 4], dprefix, size);
    write_matrix_masked(md, size, vd, ctx->vfpu, dprefix);
    vfpu_eat_prefixes(ctx);
}

// ---------------------------------------------------------------------------
// Vector-matrix transform (vtfm2/3/4)
// ---------------------------------------------------------------------------

/// Dot the last (`row==n-1`) matrix row of an `n` x `n` transform with the
/// S/T-prefixed operands and apply the D prefix to the LAST ELEMENT only,
/// then write the `n`-element result. Rows 0..n-2 of `d` must already hold
/// the raw (unprefixed) dots. Mirrors PPSSPP `Int_Vtfm`'s final-row path.
static void tfm_finish(recomp_context* ctx, float ms[16], float v[4],
                       float d[4], uint8_t vd, int n) {
    vfpu_apply_prefix_st(&ms[(n - 1) * 4],
                         ctx->vfpu_ctrl[VFPU_CTRL_SPREFIX], 4);
    vfpu_apply_prefix_st(v, ctx->vfpu_ctrl[VFPU_CTRL_TPREFIX], n);
    float sum = 0.0f;
    for (int col = 0; col < n; col++) {
        sum += ms[(n - 1) * 4 + col] * v[col];
    }
    d[n - 1] = sum;

    uint32_t collapsed = collapse_last_element_dprefix(
        n, ctx->vfpu_ctrl[VFPU_CTRL_DPREFIX]);
    vfpu_apply_prefix_d(d, collapsed, n);
    vfpu_write_vector(d, n, vd, ctx->vfpu, collapsed);
    vfpu_eat_prefixes(ctx);
}

/// Generic matrix-vector transform: out = M * v. Rows 0..n-2 are raw,
/// unprefixed dots; the S/T/D prefixes apply only to the final row /
/// final element (PPSSPP Int_Vtfm), so tfm_finish handles row n-1.
static void vtfm_impl(recomp_context* ctx,
                       uint8_t vd, uint8_t vs, uint8_t vt,
                       int n) {
    float ms[16], v[4], d[4];
    read_matrix(ms, n, vs, ctx->vfpu);
    vfpu_read_vector(v, n, vt, ctx->vfpu);

    for (int row = 0; row < n - 1; row++) {
        float sum = 0.0f;
        for (int col = 0; col < n; col++) {
            sum += ms[row * 4 + col] * v[col];
        }
        d[row] = sum;
    }

    tfm_finish(ctx, ms, v, d, vd, n);
}

void vfpu_vtfm2(recomp_context* ctx, uint8_t*,
                uint8_t vd, uint8_t vs, uint8_t vt) {
    vtfm_impl(ctx, vd, vs, vt, 2);
}

void vfpu_vtfm3(recomp_context* ctx, uint8_t*,
                uint8_t vd, uint8_t vs, uint8_t vt) {
    vtfm_impl(ctx, vd, vs, vt, 3);
}

void vfpu_vtfm4(recomp_context* ctx, uint8_t*,
                uint8_t vd, uint8_t vs, uint8_t vt) {
    vtfm_impl(ctx, vd, vs, vt, 4);
}

// ---------------------------------------------------------------------------
// Homogeneous transform (vhtfm2/3/4)
// ---------------------------------------------------------------------------

/// Homogeneous transform: the vector's last lane is the implicit 1.0
/// constant. Like vtfm, rows 0..n-2 are raw dots and the S/T/D prefixes
/// apply only to the final row / final element (PPSSPP Int_Vtfm runs both
/// forms). The T prefix scopes to the n-1 real lanes; the homogeneous
/// 1.0 lane is left unprefixed.
static void vhtfm_impl(recomp_context* ctx,
                        uint8_t vd, uint8_t vs, uint8_t vt,
                        int n) {
    float ms[16], v[4], d[4];
    read_matrix(ms, n, vs, ctx->vfpu);
    vfpu_read_vector(v, n - 1, vt, ctx->vfpu);
    v[n - 1] = 1.0f;  // homogeneous coordinate

    for (int row = 0; row < n - 1; row++) {
        float sum = 0.0f;
        for (int col = 0; col < n; col++) {
            sum += ms[row * 4 + col] * v[col];
        }
        d[row] = sum;
    }

    vfpu_apply_prefix_st(&ms[(n - 1) * 4],
                         ctx->vfpu_ctrl[VFPU_CTRL_SPREFIX], 4);
    vfpu_apply_prefix_st(v, ctx->vfpu_ctrl[VFPU_CTRL_TPREFIX], n - 1);
    float sum = 0.0f;
    for (int col = 0; col < n; col++) {
        sum += ms[(n - 1) * 4 + col] * v[col];
    }
    d[n - 1] = sum;

    uint32_t collapsed = collapse_last_element_dprefix(
        n, ctx->vfpu_ctrl[VFPU_CTRL_DPREFIX]);
    vfpu_apply_prefix_d(d, collapsed, n);
    vfpu_write_vector(d, n, vd, ctx->vfpu, collapsed);
    vfpu_eat_prefixes(ctx);
}

void vfpu_vhtfm2(recomp_context* ctx, uint8_t*,
                 uint8_t vd, uint8_t vs, uint8_t vt) {
    vhtfm_impl(ctx, vd, vs, vt, 2);
}

void vfpu_vhtfm3(recomp_context* ctx, uint8_t*,
                 uint8_t vd, uint8_t vs, uint8_t vt) {
    vhtfm_impl(ctx, vd, vs, vt, 3);
}

void vfpu_vhtfm4(recomp_context* ctx, uint8_t*,
                 uint8_t vd, uint8_t vs, uint8_t vt) {
    vhtfm_impl(ctx, vd, vs, vt, 4);
}

// ---------------------------------------------------------------------------
// Cross product / quaternion multiply
// ---------------------------------------------------------------------------

void vfpu_vcrsp(recomp_context* ctx, uint8_t*,
                uint8_t vd, uint8_t vs, uint8_t vt) {
    float s[4], t[4], d[4];
    vfpu_read_vector(s, 3, vs, ctx->vfpu);
    vfpu_read_vector(t, 3, vt, ctx->vfpu);
    d[0] = s[1] * t[2] - s[2] * t[1];
    d[1] = s[2] * t[0] - s[0] * t[2];
    d[2] = s[0] * t[1] - s[1] * t[0];
    vfpu_write_vector(d, 3, vd, ctx->vfpu, 0);
    vfpu_eat_prefixes(ctx);
}

void vfpu_vqmul(recomp_context* ctx, uint8_t*,
                uint8_t vd, uint8_t vs, uint8_t vt) {
    float s[4], t[4], d[4];
    vfpu_read_vector(s, 4, vs, ctx->vfpu);
    vfpu_read_vector(t, 4, vt, ctx->vfpu);
    d[0] =  s[0] * t[3] + s[1] * t[2]
           - s[2] * t[1] + s[3] * t[0];
    d[1] = -s[0] * t[2] + s[1] * t[3]
           + s[2] * t[0] + s[3] * t[1];
    d[2] =  s[0] * t[1] - s[1] * t[0]
           + s[2] * t[3] + s[3] * t[2];
    d[3] = -s[0] * t[0] - s[1] * t[1]
           - s[2] * t[2] + s[3] * t[3];
    vfpu_write_vector(d, 4, vd, ctx->vfpu, 0);
    vfpu_eat_prefixes(ctx);
}

// ---------------------------------------------------------------------------
// Matrix unary ops
// ---------------------------------------------------------------------------

void vfpu_vmmov(recomp_context* ctx, uint8_t*,
                uint8_t vd, uint8_t vs, uint8_t size) {
    float ms[16];
    read_matrix(ms, size, vs, ctx->vfpu);
    write_matrix(ms, size, vd, ctx->vfpu);
    vfpu_eat_prefixes(ctx);
}

void vfpu_vmidt(recomp_context* ctx, uint8_t*,
                uint8_t vd, uint8_t size) {
    float md[16] = {};
    for (int i = 0; i < size; i++) {
        md[i * 4 + i] = 1.0f;
    }
    write_matrix(md, size, vd, ctx->vfpu);
    vfpu_eat_prefixes(ctx);
}

void vfpu_vmzero(recomp_context* ctx, uint8_t*,
                 uint8_t vd, uint8_t size) {
    float md[16] = {};
    write_matrix(md, size, vd, ctx->vfpu);
    vfpu_eat_prefixes(ctx);
}

void vfpu_vmone(recomp_context* ctx, uint8_t*,
                uint8_t vd, uint8_t size) {
    float md[16];
    for (int i = 0; i < 16; i++) md[i] = 1.0f;
    write_matrix(md, size, vd, ctx->vfpu);
    vfpu_eat_prefixes(ctx);
}

// ---------------------------------------------------------------------------
// Rotation (vrot)
// ---------------------------------------------------------------------------

void vfpu_vrot(recomp_context* ctx, uint8_t*,
               uint8_t vd, uint8_t vs, uint16_t imm5,
               uint8_t size) {
    float s;
    vfpu_read_vector(&s, 1, vs, ctx->vfpu);
    float sin_val = vfpu_sin_single(s);
    float cos_val = vfpu_cos_single(s);

    // imm5 encodes rotation pattern
    int sinidx = (imm5 >> 2) & 3;
    int cosidx = imm5 & 3;
    bool neg_sin = (imm5 >> 4) & 1;

    float d[4];
    for (int i = 0; i < size; i++) {
        if (i == sinidx) {
            d[i] = neg_sin ? -sin_val : sin_val;
        } else if (i == cosidx) {
            d[i] = cos_val;
        } else {
            d[i] = 0.0f;
        }
    }

    vfpu_write_vector(d, size, vd, ctx->vfpu, 0);
    vfpu_eat_prefixes(ctx);
}
