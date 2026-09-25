/* VFPU tests — synthetic data only.
 *
 * Two things get pinned here. The first is register addressing, because the
 * layout is what makes a matrix row and column alias correctly and everything
 * else is built on it. The second is that a pending prefix makes arithmetic
 * *trap* rather than compute: partial VFPU that ignores prefixes produces
 * silently wrong numbers, which is the one failure mode this project has spent
 * its whole life avoiding.
 */

#include "psprecomp/vfpu.h"

#include <stdio.h>
#include <string.h>

static int failures;

#define CHECK(cond, ...)                                       \
    do {                                                       \
        if (!(cond)) {                                         \
            printf("FAIL %s:%d: ", __FILE__, __LINE__);        \
            printf(__VA_ARGS__);                               \
            printf("\n");                                      \
            failures++;                                        \
        }                                                      \
    } while (0)

#define CHECK_F(got, want, label)                                            \
    CHECK((got) > (want) - 1e-5f && (got) < (want) + 1e-5f,                   \
          "%s: got %f, want %f", (label), (double)(got), (double)(want))

static void test_register_addressing(void) {
    int r[4];

    /* A quad in matrix 0, column 0 is four consecutive rows: indices
     * 0, 1, 2, 3 under the mtx*4 + col*32 + row layout. */
    int n = psp_vfpu_regs(0x00, 4, r);
    CHECK(n == 4, "a quad names four registers, got %d", n);
    CHECK(r[0] == 0 && r[1] == 1 && r[2] == 2 && r[3] == 3,
          "quad M000: got %d,%d,%d,%d", r[0], r[1], r[2], r[3]);

    /* A single names exactly one. */
    n = psp_vfpu_regs(0x00, 1, r);
    CHECK(n == 1 && r[0] == 0, "single names one register");

    /* The transpose bit (bit 5) walks the other axis. Column-major access of
     * the same matrix must land on a *different* set of registers -- if it did
     * not, transposing a matrix would be a no-op and every rotation would be
     * wrong. */
    int rowwise[4], colwise[4];
    psp_vfpu_regs(0x00, 4, rowwise);
    psp_vfpu_regs(0x20, 4, colwise);
    CHECK(memcmp(rowwise, colwise, sizeof rowwise) != 0,
          "transposed access reaches different registers");
    CHECK(colwise[0] == 0 && colwise[1] == 32 && colwise[2] == 64 && colwise[3] == 96,
          "transposed quad strides by column: got %d,%d,%d,%d",
          colwise[0], colwise[1], colwise[2], colwise[3]);

    /* Every index must stay inside the 128-register file, for every width and
     * every possible field value. */
    for (uint32_t vreg = 0; vreg < 128; vreg++) {
        for (int size = 1; size <= 4; size++) {
            int idx[4];
            int len = psp_vfpu_regs(vreg, size, idx);
            for (int i = 0; i < len; i++) {
                if (idx[i] < 0 || idx[i] >= 128) {
                    printf("FAIL vreg=0x%02X size=%d lane %d -> %d (out of range)\n",
                           vreg, size, i, idx[i]);
                    failures++;
                }
            }
        }
    }
}

static void test_load_store(void) {
    const uint32_t AT = 0x08860000u;

    /* Quad round-trip through guest memory. */
    float in[4] = { 1.5f, -2.25f, 3.75f, 0.5f };
    for (int i = 0; i < 4; i++) psp_write_f32(AT + (uint32_t)i * 4, in[i]);

    psp_lv_q(0x00, AT);
    int r[4];
    psp_vfpu_regs(0x00, 4, r);
    for (int i = 0; i < 4; i++) CHECK_F(psp_cpu.v[r[i]], in[i], "lv.q lane");

    psp_sv_q(0x00, AT + 64);
    for (int i = 0; i < 4; i++)
        CHECK_F(psp_read_f32(AT + 64 + (uint32_t)i * 4), in[i], "sv.q lane");

    /* Quad addresses are 16-byte aligned; a misaligned address is masked down
     * rather than faulting, which is what the hardware does. */
    psp_lv_q(0x04, AT + 7);
    int r2[4];
    psp_vfpu_regs(0x04, 4, r2);
    CHECK_F(psp_cpu.v[r2[0]], in[0], "misaligned lv.q masks the address");

    psp_lv_s(0x08, AT + 4);
    int r3[4];
    psp_vfpu_regs(0x08, 1, r3);
    CHECK_F(psp_cpu.v[r3[0]], in[1], "lv.s");
}

/* Load a quad register from an array, for the arithmetic tests. */
static void set_quad(uint32_t vreg, const float f[4]) {
    int r[4];
    psp_vfpu_regs(vreg, 4, r);
    for (int i = 0; i < 4; i++) psp_cpu.v[r[i]] = f[i];
}
static void get_quad(uint32_t vreg, float f[4]) {
    int r[4];
    psp_vfpu_regs(vreg, 4, r);
    for (int i = 0; i < 4; i++) f[i] = psp_cpu.v[r[i]];
}

static void test_arithmetic(void) {
    psp_vfpu_reset();

    const float a[4] = { 1.0f, 2.0f, 3.0f, 4.0f };
    const float b[4] = { 5.0f, 6.0f, 7.0f, 8.0f };
    float out[4];

    /* Three different matrices so vd, vs and vt do not alias. */
    set_quad(0x00, a);
    set_quad(0x04, b);

    psp_vadd(0x08, 0x00, 0x04, 4);
    get_quad(0x08, out);
    for (int i = 0; i < 4; i++) CHECK_F(out[i], a[i] + b[i], "vadd lane");

    psp_vmul(0x08, 0x00, 0x04, 4);
    get_quad(0x08, out);
    for (int i = 0; i < 4; i++) CHECK_F(out[i], a[i] * b[i], "vmul lane");

    psp_vsub(0x08, 0x04, 0x00, 4);
    get_quad(0x08, out);
    for (int i = 0; i < 4; i++) CHECK_F(out[i], b[i] - a[i], "vsub lane");

    /* vdot collapses to one lane: 1*5 + 2*6 + 3*7 + 4*8 = 70. */
    psp_vdot(0x08, 0x00, 0x04, 4);
    int d[4];
    psp_vfpu_regs(0x08, 1, d);
    CHECK_F(psp_cpu.v[d[0]], 70.0f, "vdot");

    /* vscl multiplies every lane by a scalar. */
    int s[4];
    psp_vfpu_regs(0x0C, 1, s);
    psp_cpu.v[s[0]] = 3.0f;
    psp_vscl(0x10, 0x00, 0x0C, 4);
    get_quad(0x10, out);
    for (int i = 0; i < 4; i++) CHECK_F(out[i], a[i] * 3.0f, "vscl lane");

    /* Destination aliasing a source must still be correct: every source lane
     * has to be read before any destination lane is written. */
    set_quad(0x00, a);
    set_quad(0x04, b);
    psp_vadd(0x00, 0x00, 0x04, 4);
    get_quad(0x00, out);
    for (int i = 0; i < 4; i++) CHECK_F(out[i], a[i] + b[i], "vadd into its own source");
}

static void test_prefix_traps(void) {
    psp_vfpu_reset();

    const float a[4] = { 1.0f, 2.0f, 3.0f, 4.0f };
    set_quad(0x00, a);
    set_quad(0x04, a);

    uint64_t before = psp_vfpu_trap_count();

    /* With no prefix pending, arithmetic computes. */
    psp_vadd(0x08, 0x00, 0x04, 4);
    CHECK(psp_vfpu_trap_count() == before, "no prefix, no trap");

    /* With one pending, it must trap instead of quietly ignoring it. */
    psp_vfpu_set_prefix(0, 0x00000055);
    CHECK(psp_vfpu_prefix_pending(), "prefix registers as pending");

    float before_out[4];
    get_quad(0x0C, before_out);
    psp_vadd(0x0C, 0x00, 0x04, 4);

    CHECK(psp_vfpu_trap_count() == before + 1,
          "a pending prefix traps rather than computing");
    CHECK(!psp_vfpu_prefix_pending(), "the prefix is consumed by the attempt");

    float after_out[4];
    get_quad(0x0C, after_out);
    CHECK(memcmp(before_out, after_out, sizeof after_out) == 0,
          "the trapped op wrote nothing -- silently wrong output is the one "
          "outcome worse than an error");
}

static void test_compare(void) {
    psp_vfpu_reset();

    const float a[4] = { 1.0f, 2.0f, 3.0f, 4.0f };
    const float b[4] = { 1.0f, 9.0f, 0.0f, 4.0f };
    set_quad(0x00, a);
    set_quad(0x04, b);

    psp_vcmp(1 /* EQ */, 0x00, 0x04, 4);
    /* Lanes 0 and 3 are equal, so bits 0 and 3, plus "any" but not "all". */
    CHECK((psp_cpu.vfpu_cc & 0xF) == 0x9,
          "vcmp EQ per-lane bits: got 0x%X", psp_cpu.vfpu_cc & 0xF);
    CHECK(psp_cpu.vfpu_cc & (1u << 4), "the any-lane bit is set");
    CHECK(!(psp_cpu.vfpu_cc & (1u << 5)), "the all-lanes bit is not");

    psp_vcmp(1, 0x00, 0x00, 4);
    CHECK(psp_cpu.vfpu_cc & (1u << 5), "comparing a vector to itself sets all-lanes");
}

/* Read/write a whole matrix through the same addressing the ops use. */
static void set_matrix(uint32_t v, int n, const float m[4][4]) {
    const uint32_t mtx = (v >> 2) & 7;
    for (int c = 0; c < n; c++) {
        int r[4];
        psp_vfpu_regs((mtx << 2) | (uint32_t)c, n, r);
        for (int i = 0; i < n; i++) psp_cpu.v[r[i]] = m[c][i];
    }
}
static void get_matrix(uint32_t v, int n, float m[4][4]) {
    const uint32_t mtx = (v >> 2) & 7;
    for (int c = 0; c < n; c++) {
        int r[4];
        psp_vfpu_regs((mtx << 2) | (uint32_t)c, n, r);
        for (int i = 0; i < n; i++) m[c][i] = psp_cpu.v[r[i]];
    }
}

static void test_matrix_ops(void) {
    psp_vfpu_reset();
    float m[4][4], out[4][4];

    /* vmidt must produce a real identity: ones on the diagonal, zeros
     * elsewhere. This is what catches the column-addressing bug -- a helper
     * that walks rows while claiming to walk columns still writes four ones,
     * just in the wrong places, so only checking the off-diagonal zeros
     * distinguishes them. */
    psp_vmidt(0x00, 4);
    get_matrix(0x00, 4, out);
    for (int c = 0; c < 4; c++)
        for (int r = 0; r < 4; r++)
            CHECK_F(out[c][r], (c == r) ? 1.0f : 0.0f, "vmidt element");

    psp_vmzero(0x04, 4);
    get_matrix(0x04, 4, out);
    for (int c = 0; c < 4; c++)
        for (int r = 0; r < 4; r++) CHECK_F(out[c][r], 0.0f, "vmzero element");

    /* vmmov must copy every element, including off-diagonal ones -- a
     * diagonal-only copy would pass an identity round-trip. */
    for (int c = 0; c < 4; c++)
        for (int r = 0; r < 4; r++) m[c][r] = (float)(c * 4 + r + 1);
    set_matrix(0x08, 4, m);
    psp_vmmov(0x0C, 0x08, 4);
    get_matrix(0x0C, 4, out);
    for (int c = 0; c < 4; c++)
        for (int r = 0; r < 4; r++) CHECK_F(out[c][r], m[c][r], "vmmov element");

    /* Scaling is orientation-independent, so it can be checked outright. */
    int k[4];
    psp_vfpu_regs(0x40, 1, k);
    psp_cpu.v[k[0]] = 2.5f;
    psp_vmscl(0x10, 0x08, 0x40, 4);
    get_matrix(0x10, 4, out);
    for (int c = 0; c < 4; c++)
        for (int r = 0; r < 4; r++) CHECK_F(out[c][r], m[c][r] * 2.5f, "vmscl element");
}

static void test_matrix_transform(void) {
    psp_vfpu_reset();
    float m[4][4], v[4];

    /* Transforming a basis vector must yield the corresponding matrix column.
     * That is the property which distinguishes this convention from its
     * transpose -- an identity test would pass either way. */
    for (int c = 0; c < 4; c++)
        for (int r = 0; r < 4; r++) m[c][r] = (float)(c * 4 + r + 1);
    set_matrix(0x08, 4, m);

    for (int basis = 0; basis < 4; basis++) {
        int t[4], d[4];
        psp_vfpu_regs(0x40, 4, t);
        for (int i = 0; i < 4; i++) psp_cpu.v[t[i]] = (i == basis) ? 1.0f : 0.0f;

        psp_vtfm(0x44, 0x08, 0x40, 4);
        psp_vfpu_regs(0x44, 4, d);
        for (int r = 0; r < 4; r++)
            CHECK_F(psp_cpu.v[d[r]], m[basis][r], "vtfm of a basis vector is that column");
    }

    /* Multiplying by the identity must be a no-op, in both operand positions. */
    psp_vmidt(0x00, 4);
    psp_vmmul(0x0C, 0x08, 0x00, 4);
    float out[4][4];
    get_matrix(0x0C, 4, out);
    for (int c = 0; c < 4; c++)
        for (int r = 0; r < 4; r++) CHECK_F(out[c][r], m[c][r], "M * I == M");

    psp_vmmul(0x0C, 0x00, 0x08, 4);
    get_matrix(0x0C, 4, out);
    for (int c = 0; c < 4; c++)
        for (int r = 0; r < 4; r++) CHECK_F(out[c][r], m[c][r], "I * M == M");

    /* vmmul and vtfm must agree: transforming by a product is the same as
     * transforming twice. This is the strongest check available without an
     * external oracle -- it pins the two against each other, though it still
     * cannot detect a consistent transpose of both. */
    float a[4][4], b[4][4];
    for (int c = 0; c < 4; c++)
        for (int r = 0; r < 4; r++) {
            a[c][r] = (float)((c + 1) * (r + 2) % 7) - 3.0f;
            b[c][r] = (float)((c + 3) * (r + 1) % 5) - 2.0f;
        }
    /* Every operand needs its OWN matrix. A register's matrix is (vreg>>2)&7,
     * so 0x08 and 0x48 are both matrix 2 -- an earlier version of this test
     * used both and quietly overwrote A while computing B*x, then blamed the
     * implementation. Matrices here: A=2, B=4, AB=5, x=6, results in 7/0/1. */
    set_matrix(0x08, 4, a);   /* matrix 2 */
    set_matrix(0x10, 4, b);   /* matrix 4 */

    int t[4];
    psp_vfpu_regs(0x18, 4, t);            /* matrix 6 */
    const float in[4] = { 1.0f, -2.0f, 0.5f, 3.0f };
    for (int i = 0; i < 4; i++) psp_cpu.v[t[i]] = in[i];

    /* (A*B) * x */
    psp_vmmul(0x14, 0x08, 0x10, 4);       /* matrix 5 */
    psp_vtfm(0x1C, 0x14, 0x18, 4);        /* matrix 7 */
    float combined[4];
    int d[4];
    psp_vfpu_regs(0x1C, 4, d);
    for (int i = 0; i < 4; i++) combined[i] = psp_cpu.v[d[i]];

    /* A * (B * x) */
    psp_vtfm(0x00, 0x10, 0x18, 4);        /* matrix 0 */
    psp_vtfm(0x04, 0x08, 0x00, 4);        /* matrix 1 */
    psp_vfpu_regs(0x04, 4, d);
    for (int i = 0; i < 4; i++)
        CHECK_F(psp_cpu.v[d[i]], combined[i],
                "vmmul and vtfm agree: (A*B)x == A(Bx)");
}

int main(void) {
    if (psp_mem_init() != 0) { printf("memory init failed\n"); return 1; }
    psp_cpu_reset();
    psp_vfpu_reset();

    test_register_addressing();
    test_load_store();
    test_arithmetic();
    test_prefix_traps();
    test_compare();
    test_matrix_ops();
    test_matrix_transform();

    psp_mem_free();

    if (failures) {
        printf("\n%d check(s) failed\n", failures);
        return 1;
    }
    printf("all VFPU checks passed\n");
    return 0;
}
