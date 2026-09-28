#include "psp_vfpu.h"
#include "recomp.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

static int failures = 0;
static int tests_run = 0;

#define ASSERT_EXACT(actual, expected, msg) \
    do { \
        tests_run++; \
        if ((actual) != (expected)) { \
            std::fprintf(stderr, \
                "FAIL: %s: got %f, expected %f\n", \
                msg, \
                static_cast<double>(actual), \
                static_cast<double>(expected)); \
            failures++; \
        } \
    } while (0)

#define ASSERT_APPROX(actual, expected, eps, msg) \
    do { \
        tests_run++; \
        if (std::fabsf((actual) - (expected)) > (eps)) { \
            std::fprintf(stderr, \
                "FAIL: %s: got %f, expected %f (eps=%e)\n", \
                msg, \
                static_cast<double>(actual), \
                static_cast<double>(expected), \
                static_cast<double>(eps)); \
            failures++; \
        } \
    } while (0)

#define ASSERT_INT_EQ(actual, expected, msg) \
    do { \
        tests_run++; \
        if ((actual) != (expected)) { \
            std::fprintf(stderr, \
                "FAIL: %s: got 0x%X, expected 0x%X\n", \
                msg, \
                static_cast<unsigned>(actual), \
                static_cast<unsigned>(expected)); \
            failures++; \
        } \
    } while (0)

/// Initialize a clean recomp_context with identity prefixes.
static void init_ctx(recomp_context& ctx) {
    std::memset(&ctx, 0, sizeof(ctx));
    ctx.vfpu_ctrl[VFPU_CTRL_SPREFIX] = 0xE4u;
    ctx.vfpu_ctrl[VFPU_CTRL_TPREFIX] = 0xE4u;
    ctx.vfpu_ctrl[VFPU_CTRL_DPREFIX] = 0x00u;
}

// Helpers available for future tests if needed:
// set_vfpu_single(ctx, flat_idx, val) - write single float
// single_idx(reg) - compute flat index from 7-bit encoding

// ===================================================================
// Test 1: vfpu_sin cardinal values (bit-exact)
// ===================================================================
static void test_sin_cardinals() {
    std::printf("  test_sin_cardinals...\n");
    recomp_context ctx;

    // sin(0.0) = 0.0
    init_ctx(ctx);
    ctx.vfpu[0] = 0.0f;  // S000 (flat 0: mtx0, col0, row0)
    vfpu_vsin(&ctx, nullptr, 0x01, 0x00, 1);  // S010 = sin(S000)
    // vd=0x01: mtx0, col1, row0 -> flat 0*16 + 1*4 + 0 = 4
    ASSERT_EXACT(ctx.vfpu[4], 0.0f, "sin(0.0)");

    // sin(1.0) = 1.0
    init_ctx(ctx);
    ctx.vfpu[0] = 1.0f;
    vfpu_vsin(&ctx, nullptr, 0x01, 0x00, 1);
    ASSERT_EXACT(ctx.vfpu[4], 1.0f, "sin(1.0)");

    // sin(2.0) = 0.0
    init_ctx(ctx);
    ctx.vfpu[0] = 2.0f;
    vfpu_vsin(&ctx, nullptr, 0x01, 0x00, 1);
    ASSERT_EXACT(ctx.vfpu[4], 0.0f, "sin(2.0)");

    // sin(3.0) = -1.0
    init_ctx(ctx);
    ctx.vfpu[0] = 3.0f;
    vfpu_vsin(&ctx, nullptr, 0x01, 0x00, 1);
    ASSERT_EXACT(ctx.vfpu[4], -1.0f, "sin(3.0)");
}

// ===================================================================
// Test 2: vfpu_cos cardinal values (bit-exact)
// ===================================================================
static void test_cos_cardinals() {
    std::printf("  test_cos_cardinals...\n");
    recomp_context ctx;

    // cos(0.0) = 1.0
    init_ctx(ctx);
    ctx.vfpu[0] = 0.0f;
    vfpu_vcos(&ctx, nullptr, 0x01, 0x00, 1);
    ASSERT_EXACT(ctx.vfpu[4], 1.0f, "cos(0.0)");

    // cos(1.0) = 0.0
    init_ctx(ctx);
    ctx.vfpu[0] = 1.0f;
    vfpu_vcos(&ctx, nullptr, 0x01, 0x00, 1);
    ASSERT_EXACT(ctx.vfpu[4], 0.0f, "cos(1.0)");

    // cos(2.0) = -1.0
    init_ctx(ctx);
    ctx.vfpu[0] = 2.0f;
    vfpu_vcos(&ctx, nullptr, 0x01, 0x00, 1);
    ASSERT_EXACT(ctx.vfpu[4], -1.0f, "cos(2.0)");

    // cos(3.0) = 0.0
    init_ctx(ctx);
    ctx.vfpu[0] = 3.0f;
    vfpu_vcos(&ctx, nullptr, 0x01, 0x00, 1);
    ASSERT_EXACT(ctx.vfpu[4], 0.0f, "cos(3.0)");
}

// ===================================================================
// Test 3: eat_prefixes resets correctly
// ===================================================================
static void test_eat_prefixes() {
    std::printf("  test_eat_prefixes...\n");
    recomp_context ctx;
    init_ctx(ctx);

    ctx.vfpu_ctrl[VFPU_CTRL_SPREFIX] = 0xDEADBEEFu;
    ctx.vfpu_ctrl[VFPU_CTRL_TPREFIX] = 0xCAFEBABEu;
    ctx.vfpu_ctrl[VFPU_CTRL_DPREFIX] = 0x12345678u;

    vfpu_eat_prefixes(&ctx);

    ASSERT_INT_EQ(ctx.vfpu_ctrl[VFPU_CTRL_SPREFIX], 0xE4u,
                  "SPREFIX after eat");
    ASSERT_INT_EQ(ctx.vfpu_ctrl[VFPU_CTRL_TPREFIX], 0xE4u,
                  "TPREFIX after eat");
    ASSERT_INT_EQ(ctx.vfpu_ctrl[VFPU_CTRL_DPREFIX], 0x00u,
                  "DPREFIX after eat");
}

// ===================================================================
// Test 3b: vfpu_init_context — a memset(0) context must come out with the
// hardware-reset identity S/T prefixes (0xE4), NOT 0. A zero prefix is the
// explicit "all lanes <- component 0" swizzle; if a freshly-created thread
// kept it, the first VFPU arithmetic op (before any eat_prefixes) silently
// collapses all operand lanes onto component 0. This is the exact root cause
// of issue #27 (Patapon's sceGumLookAt vsub.q zeroing the forward vector ->
// all-zero VIEW matrix). Regression guard for the thread/boot ctx init.
// ===================================================================
static void test_init_context_default_prefix() {
    std::printf("  test_init_context_default_prefix...\n");
    recomp_context ctx;
    std::memset(&ctx, 0, sizeof(ctx));  // mimic the thread/boot memset(0)
    vfpu_init_context(&ctx);

    ASSERT_INT_EQ(ctx.vfpu_ctrl[VFPU_CTRL_SPREFIX], 0xE4u,
                  "init SPREFIX is identity");
    ASSERT_INT_EQ(ctx.vfpu_ctrl[VFPU_CTRL_TPREFIX], 0xE4u,
                  "init TPREFIX is identity");

    // The decisive behavioural check: vsub.q on a freshly-init'd context
    // (no explicit set_prefix) must subtract component-wise, not swizzle
    // every lane to component 0. Inputs mirror Patapon's LookAt forward
    // vector: eye-target where only z differs.
    // vs=0x00 -> mtx0,col0 -> vfpu[0..3]; vt=0x01 -> mtx0,col1 -> vfpu[4..7]
    // (mirrors Patapon's LookAt: vsub.q v26, v00, v01).
    ctx.vfpu[0] = 0.0f;    // eye = (0, 0, 300, 1)
    ctx.vfpu[1] = 0.0f;
    ctx.vfpu[2] = 300.0f;
    ctx.vfpu[3] = 1.0f;
    ctx.vfpu[4] = 0.0f;    // tgt = (0, 0,   0, 1)
    ctx.vfpu[5] = 0.0f;
    ctx.vfpu[6] = 0.0f;
    ctx.vfpu[7] = 1.0f;

    // vsub.q vd=0x08, vs=0x00, vt=0x01, size=4
    // vd=0x08 -> mtx=2, col=0 -> result lands in vfpu[32..35].
    vfpu_vsub(&ctx, nullptr, 0x08, 0x00, 0x01, 4);

    ASSERT_EXACT(ctx.vfpu[32], 0.0f, "default-prefix vsub[0]");
    ASSERT_EXACT(ctx.vfpu[33], 0.0f, "default-prefix vsub[1]");
    // The decisive component: with the prefix-init bug this read 0 (every
    // lane swizzled to component 0 == 0); correct subtraction yields 300.
    ASSERT_EXACT(ctx.vfpu[34], 300.0f,
                 "default-prefix vsub[2] (would be 0 with prefix bug)");
    ASSERT_EXACT(ctx.vfpu[35], 0.0f, "default-prefix vsub[3]");
}

// Probe: does vfpu_write_vector land a quad in a TRANSPOSED dest (bit5=1)?
static void test_transpose_write_quad() {
    std::printf("  test_transpose_write_quad...\n");
    recomp_context ctx;
    init_ctx(ctx);
    float d[4] = {0.0f, 0.0f, 300.0f, 0.0f};
    // reg 0x26 -> transpose=1, mtx=1, col(=row anchor)=2, col-start=0.
    // Expected cells (row-form): vfpu[18,22,26,30].
    vfpu_write_vector(d, 4, 0x26, ctx.vfpu, 0);
    ASSERT_EXACT(ctx.vfpu[18], 0.0f,   "tw[0] -> vfpu[18]");
    ASSERT_EXACT(ctx.vfpu[22], 0.0f,   "tw[1] -> vfpu[22]");
    ASSERT_EXACT(ctx.vfpu[26], 300.0f, "tw[2] -> vfpu[26]");
    ASSERT_EXACT(ctx.vfpu[30], 0.0f,   "tw[3] -> vfpu[30]");
}

// ===================================================================
// Test 4: Prefix swizzle on a pair vector
// ===================================================================
static void test_prefix_swizzle() {
    std::printf("  test_prefix_swizzle...\n");
    recomp_context ctx;
    init_ctx(ctx);

    // Load a pair {3.0, 7.0} into C000 (rows 0,1 of column 0)
    // 7-bit encoding for pair C000: mtx=0, col=0, row=0,
    //   transpose=0 -> reg = 0x00
    // Layout vfpu[mtx*16 + col*4 + row]: pair is contiguous down
    // the column.
    ctx.vfpu[0] = 3.0f;  // mtx0, col0, row0 -> flat 0
    ctx.vfpu[1] = 7.0f;  // mtx0, col0, row1 -> flat 1

    // Set SPREFIX to swizzle Y,X (swap elements):
    // swizzle bits: element 0 reads from index 1 (=0b01),
    //               element 1 reads from index 0 (=0b00)
    // bits 1:0 = 01 (element 0 <- index 1)
    // bits 3:2 = 00 (element 1 <- index 0)
    // Result: 0x01 for swizzle, rest 0
    ctx.vfpu_ctrl[VFPU_CTRL_SPREFIX] = 0x01u;

    // vmov pair: vd=0x04 (C100), vs=0x00 (C000), size=2
    // vd = 0x04 -> mtx = (4>>2)&7 = 1, col = 4&3 = 0
    // For pair: row = (4>>5)&2 = 0, transpose = (4>>5)&1 = 0
    // Elements at mtx=1, col=0, rows 0,1 -> flat 16, 17
    vfpu_vmov(&ctx, nullptr, 0x04, 0x00, 2);

    ASSERT_EXACT(ctx.vfpu[16], 7.0f, "swizzle pair[0]");
    ASSERT_EXACT(ctx.vfpu[17], 3.0f, "swizzle pair[1]");
}

// ===================================================================
// Test 5: vmmul 2x2 identity produces original matrix
// ===================================================================
static void test_vmmul_identity() {
    std::printf("  test_vmmul_identity...\n");
    recomp_context ctx;
    init_ctx(ctx);

    // Layout: element (row r, col c) of mtx m -> flat m*16 + c*4 + r
    // Set up 2x2 identity in matrix 0 (M000)
    ctx.vfpu[0]  = 1.0f;  // mtx0 (0,0)
    ctx.vfpu[4]  = 0.0f;  // mtx0 (0,1)
    ctx.vfpu[1]  = 0.0f;  // mtx0 (1,0)
    ctx.vfpu[5]  = 1.0f;  // mtx0 (1,1)

    // Set up arbitrary 2x2 [[1,2],[3,4]] in matrix 1 (M100)
    ctx.vfpu[16] = 1.0f;  // mtx1 (0,0)
    ctx.vfpu[20] = 2.0f;  // mtx1 (0,1)
    ctx.vfpu[17] = 3.0f;  // mtx1 (1,0)
    ctx.vfpu[21] = 4.0f;  // mtx1 (1,1)

    // vmmul M200, M000, M100 -> result in matrix 2
    // M000 = 0x00, M100 = 0x04, M200 = 0x08
    // PPSSPP semantics: result = transpose(M000) * M100
    //                 = identity * M100 = M100
    vfpu_vmmul(&ctx, nullptr, 0x08, 0x00, 0x04, 2);

    ASSERT_APPROX(ctx.vfpu[32], 1.0f, 1e-5f,
                  "mmul identity [0][0]");
    ASSERT_APPROX(ctx.vfpu[36], 2.0f, 1e-5f,
                  "mmul identity [0][1]");
    ASSERT_APPROX(ctx.vfpu[33], 3.0f, 1e-5f,
                  "mmul identity [1][0]");
    ASSERT_APPROX(ctx.vfpu[37], 4.0f, 1e-5f,
                  "mmul identity [1][1]");
}

// ===================================================================
// Test 6: Non-cardinal trig accuracy (epsilon)
// ===================================================================
static void test_trig_noncardinal() {
    std::printf("  test_trig_noncardinal...\n");
    recomp_context ctx;

    // sin(0.5) should be sin(pi/4) = sqrt(2)/2
    init_ctx(ctx);
    ctx.vfpu[0] = 0.5f;
    vfpu_vsin(&ctx, nullptr, 0x01, 0x00, 1);
    float expected_sin = std::sinf(0.5f
        * static_cast<float>(M_PI_2));
    ASSERT_APPROX(ctx.vfpu[4], expected_sin, 1e-5f,
                  "sin(0.5)");

    // cos(0.5) should be cos(pi/4) = sqrt(2)/2
    init_ctx(ctx);
    ctx.vfpu[0] = 0.5f;
    vfpu_vcos(&ctx, nullptr, 0x01, 0x00, 1);
    float expected_cos = std::cosf(0.5f
        * static_cast<float>(M_PI_2));
    ASSERT_APPROX(ctx.vfpu[4], expected_cos, 1e-5f,
                  "cos(0.5)");
}

// ===================================================================
// Test 7: vadd quad adds element-wise
// ===================================================================
static void test_vadd_quad() {
    std::printf("  test_vadd_quad...\n");
    recomp_context ctx;
    init_ctx(ctx);

    // C000 (quad, mtx=0, col=0, row=0, no transpose)
    ctx.vfpu[0] = 1.0f;
    ctx.vfpu[1] = 2.0f;
    ctx.vfpu[2] = 3.0f;
    ctx.vfpu[3] = 4.0f;

    // C100 (quad, mtx=1, col=0)
    ctx.vfpu[16] = 10.0f;
    ctx.vfpu[17] = 20.0f;
    ctx.vfpu[18] = 30.0f;
    ctx.vfpu[19] = 40.0f;

    // vadd C200, C000, C100 (size=4)
    // C000 = 0x00, C100 = 0x04, C200 = 0x08
    vfpu_vadd(&ctx, nullptr, 0x08, 0x00, 0x04, 4);

    ASSERT_EXACT(ctx.vfpu[32], 11.0f, "vadd quad[0]");
    ASSERT_EXACT(ctx.vfpu[33], 22.0f, "vadd quad[1]");
    ASSERT_EXACT(ctx.vfpu[34], 33.0f, "vadd quad[2]");
    ASSERT_EXACT(ctx.vfpu[35], 44.0f, "vadd quad[3]");
}

// ===================================================================
// Test 8: vdot dot product
// ===================================================================
static void test_vdot() {
    std::printf("  test_vdot...\n");
    recomp_context ctx;
    init_ctx(ctx);

    // Vector {1, 2, 3} in C000 (triple)
    ctx.vfpu[0] = 1.0f;
    ctx.vfpu[1] = 2.0f;
    ctx.vfpu[2] = 3.0f;

    // Vector {4, 5, 6} in C100 (triple)
    ctx.vfpu[16] = 4.0f;
    ctx.vfpu[17] = 5.0f;
    ctx.vfpu[18] = 6.0f;

    // vdot S200, C000, C100 (size=3)
    vfpu_vdot(&ctx, nullptr, 0x08, 0x00, 0x04, 3);

    // dot = 1*4 + 2*5 + 3*6 = 4 + 10 + 18 = 32
    ASSERT_EXACT(ctx.vfpu[32], 32.0f, "vdot triple");
}

// ===================================================================
// Test 9: vcmp EQ mode
// ===================================================================
static void test_vcmp_eq() {
    std::printf("  test_vcmp_eq...\n");
    recomp_context ctx;
    init_ctx(ctx);

    // Pair {1, 2} vs {1, 3}
    ctx.vfpu[0] = 1.0f;
    ctx.vfpu[1] = 2.0f;
    ctx.vfpu[16] = 1.0f;
    ctx.vfpu[17] = 3.0f;

    // vcmp EQ, P000, P100 (cond=1, size=2)
    vfpu_vcmp(&ctx, nullptr, 0x00, 0x04, 1, 2);

    // Element 0: 1==1 -> true (bit 0 set)
    // Element 1: 2==3 -> false (bit 1 not set)
    // OR (bit 4) = true, AND (bit 5) = false
    uint32_t cc = ctx.vfpu_ctrl[VFPU_CTRL_CC];
    ASSERT_INT_EQ(cc & 0x3F, 0x11u,
                  "vcmp EQ pair: bits 0,4 set");
}

// ===================================================================
// Test 10: lv.q into C300..C330 fills 16 DISJOINT slots
// Regression for issue #27: the old layout aliased all four columns
// of a matrix onto the same 4 physical slots (rotated).
// ===================================================================
static void test_lvq_quad_disjoint() {
    std::printf("  test_lvq_quad_disjoint...\n");
    recomp_context ctx;
    init_ctx(ctx);

    // Small fake rdram: PSP address 0x08000000 masks to offset 0.
    uint8_t* rdram = static_cast<uint8_t*>(std::calloc(1, 4096));
    const uint32_t base = 0x08000000u;
    for (int i = 0; i < 16; i++) {
        float f = static_cast<float>(i + 1);  // 1..16, all distinct
        std::memcpy(rdram + i * 4, &f, 4);
    }

    ctx.r[8] = static_cast<int32_t>(base);  // t0
    vfpu_lv_q(&ctx, rdram, 0x0C, 8, 0);   // C300 <- 1,2,3,4
    vfpu_lv_q(&ctx, rdram, 0x0D, 8, 16);  // C310 <- 5,6,7,8
    vfpu_lv_q(&ctx, rdram, 0x0E, 8, 32);  // C320 <- 9,10,11,12
    vfpu_lv_q(&ctx, rdram, 0x0F, 8, 48);  // C330 <- 13,14,15,16

    // All 16 physical slots of mtx3 (flat 48..63) must hold the 16
    // distinct values: column c, row r -> flat 48 + c*4 + r.
    for (int c = 0; c < 4; c++) {
        for (int r = 0; r < 4; r++) {
            char msg[64];
            std::snprintf(msg, sizeof(msg),
                          "lv.q disjoint mtx3 col%d row%d", c, r);
            ASSERT_EXACT(ctx.vfpu[48 + c * 4 + r],
                         static_cast<float>(c * 4 + r + 1), msg);
        }
    }

    // Read back per element through the accessors (single reg
    // encoding: mtx=3, col=c, row=r -> reg = (r<<5)|(3<<2)|c).
    for (int c = 0; c < 4; c++) {
        for (int r = 0; r < 4; r++) {
            float v = 0.0f;
            int reg = (r << 5) | (3 << 2) | c;
            vfpu_read_vector(&v, 1, reg, ctx.vfpu);
            char msg[64];
            std::snprintf(msg, sizeof(msg),
                          "lv.q readback S3%d%d", c, r);
            ASSERT_EXACT(v, static_cast<float>(c * 4 + r + 1), msg);
        }
    }

    std::free(rdram);
}

// ===================================================================
// Test 11: transpose round-trip -- write M-form, read E-form
// vmmov M100 <- E000 must produce the transpose of mtx0 in mtx1.
// ===================================================================
static void test_transpose_roundtrip() {
    std::printf("  test_transpose_roundtrip...\n");
    recomp_context ctx;
    init_ctx(ctx);

    // Fill mtx0 with value == flat offset (0..15).
    for (int i = 0; i < 16; i++) {
        ctx.vfpu[i] = static_cast<float>(i);
    }

    // vmmov vd=M100 (0x04), vs=E000 (0x20, transpose form), size=4.
    vfpu_vmmov(&ctx, nullptr, 0x04, 0x20, 4);

    // mtx1 flat[j*4+i] must equal mtx0 flat[i*4+j].
    for (int j = 0; j < 4; j++) {
        for (int i = 0; i < 4; i++) {
            char msg[64];
            std::snprintf(msg, sizeof(msg),
                          "transpose roundtrip [%d][%d]", j, i);
            ASSERT_EXACT(ctx.vfpu[16 + j * 4 + i],
                         static_cast<float>(i * 4 + j), msg);
        }
    }
}

// ===================================================================
// Test 12: sceGumOrtho regression (issue #27)
// Replays the exact VFPU op sequence of FUN_08857d10 (sceGumOrtho
// helper) with the title-screen argument family and an identity
// stack top, asserting the textbook column-major ortho output.
// Args: l=0, r=480, b=272, t=0, n=-100, f=76.471 (f-n = 176.471).
// ===================================================================
static void test_sce_gum_ortho() {
    std::printf("  test_sce_gum_ortho...\n");
    recomp_context ctx;
    init_ctx(ctx);

    uint8_t* rdram = static_cast<uint8_t*>(std::calloc(1, 4096));
    const uint32_t SP  = 0x08000100u;  // 6 stack floats
    const uint32_t TOP = 0x08000200u;  // gum stack top (identity)
    const uint32_t OUT = 0x08000300u;  // output matrix

    // Stack float family: -(r+l), r-l, -(t+b), t-b, -(f+n), f-n
    const float A[6] = {-480.0f, 480.0f, 272.0f, -272.0f,
                        -(-100.0f + 76.471f), 176.471f};
    for (int i = 0; i < 6; i++) {
        std::memcpy(rdram + ((SP + i * 4) & 0x07FFFFFFu),
                    &A[i], 4);
    }
    const float ident[16] = {1, 0, 0, 0, 0, 1, 0, 0,
                             0, 0, 1, 0, 0, 0, 0, 1};
    std::memcpy(rdram + (TOP & 0x07FFFFFFu), ident, 64);

    ctx.r[4] = static_cast<int32_t>(TOP);   // a0
    ctx.r[29] = static_cast<int32_t>(SP);   // sp

    // Exact op sequence from FUN_08857d10 (cribbed from the
    // /tmp/vfpu_harness differential harness).
    vfpu_lv_q(&ctx, rdram, 0x0C, 4, 0);
    vfpu_lv_q(&ctx, rdram, 0x0D, 4, 16);
    vfpu_lv_q(&ctx, rdram, 0x0E, 4, 32);
    vfpu_lv_q(&ctx, rdram, 0x0F, 4, 48);
    vfpu_vmidt(&ctx, rdram, 0x24, 4);
    vfpu_lv_s(&ctx, rdram, 0x00, 29, 0);
    vfpu_lv_s(&ctx, rdram, 0x20, 29, 4);
    vfpu_lv_s(&ctx, rdram, 0x40, 29, 8);
    vfpu_lv_s(&ctx, rdram, 0x60, 29, 12);
    vfpu_lv_s(&ctx, rdram, 0x01, 29, 16);
    vfpu_lv_s(&ctx, rdram, 0x21, 29, 20);
    vfpu_vfim(&ctx, 0x41, 0x4000u);  // 2.0f
    vfpu_vneg(&ctx, rdram, 0x00, 0x00, 4);
    vfpu_vneg(&ctx, rdram, 0x01, 0x01, 2);
    vfpu_vrcp(&ctx, rdram, 0x20, 0x20, 1);
    vfpu_vrcp(&ctx, rdram, 0x60, 0x60, 1);
    vfpu_vrcp(&ctx, rdram, 0x21, 0x21, 1);
    vfpu_set_prefix(&ctx, 0, 0x000100E4u);
    vfpu_vmul(&ctx, rdram, 0x04, 0x41, 0x20, 1);
    vfpu_set_prefix(&ctx, 0, 0x000100E4u);
    vfpu_vmul(&ctx, rdram, 0x25, 0x41, 0x60, 1);
    vfpu_vmul(&ctx, rdram, 0x46, 0x41, 0x21, 1);
    vfpu_vmul(&ctx, rdram, 0x07, 0x00, 0x20, 1);
    vfpu_vmul(&ctx, rdram, 0x27, 0x40, 0x60, 1);
    vfpu_vmul(&ctx, rdram, 0x47, 0x01, 0x21, 1);
    vfpu_vmmul(&ctx, rdram, 0x28, 0x04, 0x2C, 4);
    ctx.r[4] = static_cast<int32_t>(OUT);
    vfpu_sv_q(&ctx, rdram, 0x08, 4, 0);
    vfpu_sv_q(&ctx, rdram, 0x09, 4, 16);
    vfpu_sv_q(&ctx, rdram, 0x0A, 4, 32);
    vfpu_sv_q(&ctx, rdram, 0x0B, 4, 48);

    float out[16];
    std::memcpy(out, rdram + (OUT & 0x07FFFFFFu), 64);

    // Textbook column-major ortho:
    //   m[0]  =  2/(r-l)   = 2/480     =  0.00416667
    //   m[5]  =  2/(t-b)   = 2/(-272)  = -0.00735294
    //   m[10] = -2/(f-n)   = -2/176.47 = -0.0113334
    //   m[12] = -(r+l)/(r-l) = -1
    //   m[13] = -(t+b)/(t-b) = -1
    //   m[14] = -(f+n)/(f-n) =  0.133337
    //   m[15] =  1
    ASSERT_APPROX(out[0],  0.00416667f, 1e-7f, "ortho m[0]");
    ASSERT_APPROX(out[5], -0.00735294f, 1e-7f, "ortho m[5]");
    ASSERT_APPROX(out[10], -0.0113334f, 1e-6f, "ortho m[10]");
    ASSERT_APPROX(out[12], -1.0f, 1e-5f, "ortho m[12]");
    ASSERT_APPROX(out[13], -1.0f, 1e-5f, "ortho m[13]");
    ASSERT_APPROX(out[14], 0.133337f, 1e-5f, "ortho m[14]");
    ASSERT_EXACT(out[15], 1.0f, "ortho m[15]");

    // All remaining slots must be exactly zero.
    const int zero_slots[] = {1, 2, 3, 4, 6, 7, 8, 9, 11};
    for (int idx : zero_slots) {
        char msg[32];
        std::snprintf(msg, sizeof(msg), "ortho m[%d]==0", idx);
        ASSERT_EXACT(out[idx], 0.0f, msg);
    }

    std::free(rdram);
}

// ===================================================================
// Test: vcmov safe-normalize guard (issue #27 view-matrix fix)
//
// sceGumLookAt normalizes the forward/right/up vectors with a degenerate
// guard:  vcmp(EZ, len2)  ->  vrsq(scale)  ->  vcmov(scale<-0 IF len2==0).
// The vcmov must be a CONDITIONAL move: for a non-degenerate vector the
// CC bit is clear and the rsqrt scale is preserved. The decoder used to
// mis-decode this word (0xD2A06808) as vf2iz, which unconditionally
// re-zeroed the scale and collapsed the whole view matrix to zero.
// ===================================================================
static void test_vcmov_safe_normalize() {
    std::printf("  test_vcmov_safe_normalize...\n");
    recomp_context ctx;

    const int v08 = vfpu_single_index(0x08);  // rsqrt scale slot
    const int v68 = vfpu_single_index(0x68);  // zeroed scratch slot

    // --- Non-degenerate (the real LookAt case): CC bit 0 clear. ---
    init_ctx(ctx);
    const float scale = 1.0f / 300.0f;
    ctx.vfpu[v08] = scale;
    ctx.vfpu[v68] = 0.0f;
    ctx.vfpu_ctrl[VFPU_CTRL_CC] = 0;  // vcmp(EZ) found len2 != 0
    // vcmov v08, v68, cc_field=0 (imm3=0, tf=0): move iff CC[0]==1.
    vfpu_vcmov(&ctx, nullptr, 0x08, 0x68, 0, 1);
    ASSERT_EXACT(ctx.vfpu[v08], scale,
                 "vcmov preserves rsqrt scale when len2 != 0");

    // --- Degenerate (len2 == 0): CC bit 0 set, scale -> 0. ---
    init_ctx(ctx);
    ctx.vfpu[v08] = scale;
    ctx.vfpu[v68] = 0.0f;
    ctx.vfpu_ctrl[VFPU_CTRL_CC] = 0x1;  // vcmp(EZ) found len2 == 0
    vfpu_vcmov(&ctx, nullptr, 0x08, 0x68, 0, 1);
    ASSERT_EXACT(ctx.vfpu[v08], 0.0f,
                 "vcmov zeroes scale when len2 == 0 (CC[0] set)");

    // --- tf=1 inverts the sense: move iff CC[0]==0. ---
    init_ctx(ctx);
    ctx.vfpu[v08] = scale;
    ctx.vfpu[v68] = 0.0f;
    ctx.vfpu_ctrl[VFPU_CTRL_CC] = 0;  // CC[0] clear
    // cc_field = imm3(0) | tf(1)<<3 = 0x08
    vfpu_vcmov(&ctx, nullptr, 0x08, 0x68, 0x08, 1);
    ASSERT_EXACT(ctx.vfpu[v08], 0.0f,
                 "vcmov tf=1 moves when CC[0] clear");
}

// ===================================================================
// Test: full LookAt forward-vector normalize stays finite (#27)
//
// Mirrors the exact runtime op sequence around 0x08857E80..0x08857E9C:
//   vsub forward = eye - tgt = (0,0,300)
//   vdot len2 = dot(fwd,fwd) = 90000
//   vcmp EZ len2 ; vrsq scale=1/sqrt(len2) ; vcmov(guard) ; vscl fwd*=scale
// The forward vector must come out unit-length, NOT zero.
// ===================================================================
static void test_lookat_forward_normalize() {
    std::printf("  test_lookat_forward_normalize...\n");
    recomp_context ctx;
    init_ctx(ctx);

    // v00 = eye (0,0,300), v01 = tgt (0,0,0), as quads.
    const int v00 = vfpu_single_index(0x00);
    ctx.vfpu[v00 + 0] = 0.0f;
    ctx.vfpu[v00 + 1] = 0.0f;
    ctx.vfpu[v00 + 2] = 300.0f;
    ctx.vfpu[v00 + 3] = 1.0f;
    // v01 column already zeroed by init_ctx.

    vfpu_vsub(&ctx, nullptr, 0x26, 0x00, 0x01, 4);   // fwd = eye - tgt
    vfpu_vdot(&ctx, nullptr, 0x08, 0x26, 0x26, 3);   // len2 = 90000
    vfpu_vzero(&ctx, nullptr, 0x68, 1);              // scratch = 0
    vfpu_vcmp(&ctx, nullptr, 0x08, 0x08, 8, 1);      // CC[0] = (len2==0)
    vfpu_vrsq(&ctx, nullptr, 0x08, 0x08, 1);         // scale = 1/sqrt(len2)
    vfpu_vcmov(&ctx, nullptr, 0x08, 0x68, 0, 1);     // guard (no-op here)
    vfpu_set_prefix(&ctx, 2, 0x0000083Fu);
    vfpu_vscl(&ctx, nullptr, 0x26, 0x26, 0x08, 3);   // fwd *= scale

    const int v26 = vfpu_single_index(0x26);
    // The third column slot for reg 0x26 (mtx1,col2): read normalized z.
    float fwd[4];
    vfpu_read_vector(fwd, 3, 0x26, ctx.vfpu);
    const float len = std::sqrtf(fwd[0]*fwd[0] + fwd[1]*fwd[1]
                                 + fwd[2]*fwd[2]);
    (void)v26;
    ASSERT_APPROX(len, 1.0f, 1e-5f,
                  "LookAt forward vector normalized to unit length");
    ASSERT_APPROX(fwd[2], 1.0f, 1e-5f,
                  "LookAt forward z stays finite (was 0 pre-fix)");
}

// ===================================================================
// Test: vmmul applies the S/D prefix to the FINAL ELEMENT only (#27/#67)
//
// PPSSPP Int_Vmmul swizzles S/T only on the final dot (a==b==n-1) and
// applies the D prefix only to that last element; every earlier element
// is the raw, unprefixed dot. Pre-fix OUR vmmul applied NO prefix at all,
// so a live prefix silently vanished. This pins the last-element scope:
// the three earlier 2x2 elements must stay raw while only d[1][1] reacts.
//
// ms = [[1,2],[3,4]], mt = [[10,20],[30,40]] (row b/a, col c).
// d[a*4+b] = sum_c ms[b*4+c]*mt[a*4+c]:
//   d00=50  d01(a0,b1)=110  d10(a1,b0)=110  d11=250  (identity prefixes).
// With SPREFIX = 0x000000 (swizzle every lane <- lane 0, i.e. broadcast
// the first component) applied ONLY to the final dot, the last row of ms
// becomes [3,3,3,3]; d11 = 3*30 + 3*40 = 210, and d00/d01/d10 stay raw.
// ===================================================================
static void test_vmmul_last_element_prefix() {
    std::printf("  test_vmmul_last_element_prefix...\n");
    recomp_context ctx;

    // Layout: matrix element (row r, col c) of mtx m -> flat m*16 + c*4 + r.
    // read_matrix(reg 0x00) yields ms[b*4+c] = vfpu[b*4 + c] (col==row==0),
    // so we write ms[b*4+c] / mt[a*4+c] straight into those flat slots.
    auto setup = [](recomp_context& c) {
        init_ctx(c);
        c.vfpu[0 * 4 + 0] = 1.0f;  // ms row0
        c.vfpu[0 * 4 + 1] = 2.0f;
        c.vfpu[1 * 4 + 0] = 3.0f;  // ms row1
        c.vfpu[1 * 4 + 1] = 4.0f;
        c.vfpu[16 + 0 * 4 + 0] = 10.0f;  // mt row0 (mtx1)
        c.vfpu[16 + 0 * 4 + 1] = 20.0f;
        c.vfpu[16 + 1 * 4 + 0] = 30.0f;  // mt row1
        c.vfpu[16 + 1 * 4 + 1] = 40.0f;
    };

    // Identity-prefix baseline. vmmul M200, M000, M100 (vd=0x08).
    setup(ctx);
    vfpu_vmmul(&ctx, nullptr, 0x08, 0x00, 0x04, 2);
    ASSERT_APPROX(ctx.vfpu[32 + 0], 50.0f, 1e-4f, "vmmul d00 raw");
    ASSERT_APPROX(ctx.vfpu[32 + 4], 110.0f, 1e-4f, "vmmul d01 raw");
    ASSERT_APPROX(ctx.vfpu[33 + 0], 110.0f, 1e-4f, "vmmul d10 raw");
    ASSERT_APPROX(ctx.vfpu[33 + 4], 250.0f, 1e-4f, "vmmul d11 raw");

    // SPREFIX broadcasts lane 0 -> applies to the final element only.
    setup(ctx);
    ctx.vfpu_ctrl[VFPU_CTRL_SPREFIX] = 0x000000u;
    vfpu_vmmul(&ctx, nullptr, 0x08, 0x00, 0x04, 2);
    ASSERT_APPROX(ctx.vfpu[32 + 0], 50.0f, 1e-4f,
                  "vmmul d00 stays raw under S prefix");
    ASSERT_APPROX(ctx.vfpu[32 + 4], 110.0f, 1e-4f,
                  "vmmul d01 stays raw under S prefix");
    ASSERT_APPROX(ctx.vfpu[33 + 0], 110.0f, 1e-4f,
                  "vmmul d10 stays raw under S prefix");
    ASSERT_APPROX(ctx.vfpu[33 + 4], 210.0f, 1e-4f,
                  "vmmul d11 reacts to last-element S prefix");
}

// ===================================================================
// Test: vtfm applies the S/T/D prefix to the FINAL ROW only (#27/#67)
//
// PPSSPP Int_Vtfm computes rows 0..n-2 from the raw vector and applies
// the S/T prefixes (and the last-element D) only on the final row. Pre-fix
// OUR vtfm applied the T prefix to the WHOLE input vector and D to the
// WHOLE result -- wrong scope. This pins the last-row scope: with a T
// prefix that negates all lanes, the earlier rows stay raw and only the
// final row's dot uses the negated vector.
//
// 3x3 M = [[1,2,3],[4,5,6],[7,8,9]], v = (1,1,1):
//   identity   -> d = (6, 15, 24)
//   T-negate   -> d = (6, 15, -24)   (only the last row flips sign)
// ===================================================================
static void test_vtfm_last_row_prefix() {
    std::printf("  test_vtfm_last_row_prefix...\n");
    recomp_context ctx;

    auto setup = [](recomp_context& c) {
        init_ctx(c);
        // read_matrix(reg 0x00) -> ms[row*4+col] = vfpu[row*4+col].
        c.vfpu[0 * 4 + 0] = 1.0f; c.vfpu[0 * 4 + 1] = 2.0f;
        c.vfpu[0 * 4 + 2] = 3.0f;
        c.vfpu[1 * 4 + 0] = 4.0f; c.vfpu[1 * 4 + 1] = 5.0f;
        c.vfpu[1 * 4 + 2] = 6.0f;
        c.vfpu[2 * 4 + 0] = 7.0f; c.vfpu[2 * 4 + 1] = 8.0f;
        c.vfpu[2 * 4 + 2] = 9.0f;
        // v = (1,1,1) in mtx1 col0 (reg 0x04), rows 0..2.
        c.vfpu[16 + 0] = 1.0f; c.vfpu[16 + 1] = 1.0f;
        c.vfpu[16 + 2] = 1.0f;
    };

    // Identity baseline: vtfm3 C200, M000, C100 (vd=0x08).
    setup(ctx);
    vfpu_vtfm3(&ctx, nullptr, 0x08, 0x00, 0x04);
    ASSERT_APPROX(ctx.vfpu[32 + 0], 6.0f, 1e-4f, "vtfm3 d0 raw");
    ASSERT_APPROX(ctx.vfpu[32 + 1], 15.0f, 1e-4f, "vtfm3 d1 raw");
    ASSERT_APPROX(ctx.vfpu[32 + 2], 24.0f, 1e-4f, "vtfm3 d2 raw");

    // TPREFIX negates all three lanes -> last row only.
    setup(ctx);
    ctx.vfpu_ctrl[VFPU_CTRL_TPREFIX] = 0x000700E4u;  // neg lanes 0,1,2
    vfpu_vtfm3(&ctx, nullptr, 0x08, 0x00, 0x04);
    ASSERT_APPROX(ctx.vfpu[32 + 0], 6.0f, 1e-4f,
                  "vtfm3 d0 stays raw under T prefix");
    ASSERT_APPROX(ctx.vfpu[32 + 1], 15.0f, 1e-4f,
                  "vtfm3 d1 stays raw under T prefix");
    ASSERT_APPROX(ctx.vfpu[32 + 2], -24.0f, 1e-4f,
                  "vtfm3 d2 reacts to last-row T prefix");
}

// ===================================================================
// main
// ===================================================================

int main() {
    std::printf("Running VFPU runtime tests...\n\n");

    test_sin_cardinals();
    test_cos_cardinals();
    test_eat_prefixes();
    test_init_context_default_prefix();
    test_transpose_write_quad();
    test_prefix_swizzle();
    test_vmmul_identity();
    test_trig_noncardinal();
    test_vadd_quad();
    test_vdot();
    test_vcmp_eq();
    test_lvq_quad_disjoint();
    test_transpose_roundtrip();
    test_sce_gum_ortho();
    test_vcmov_safe_normalize();
    test_lookat_forward_normalize();
    test_vmmul_last_element_prefix();
    test_vtfm_last_row_prefix();

    std::printf("\n%d tests run, %d failures\n",
                tests_run, failures);

    if (failures == 0) {
        std::printf("ALL TESTS PASSED\n");
        return 0;
    } else {
        std::printf("SOME TESTS FAILED\n");
        return 1;
    }
}
