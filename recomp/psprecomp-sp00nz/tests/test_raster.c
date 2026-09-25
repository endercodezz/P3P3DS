/* Rasterizer tests — display list in, pixels out.
 *
 * These use hand-built display lists rather than game data, which is the point:
 * the game cannot yet reach its own draw calls, so waiting for it to render
 * would leave this code completely unmeasured until the last blocker clears.
 * A synthetic list exercises the same path the game will take.
 *
 * The checks are on *where* pixels land, not just how many. A rasterizer that
 * fills the whole screen and one that fills the right rectangle both report a
 * nonzero pixel count, and only one of them is correct.
 */

#include "psprecomp/hle.h"
#include "psprecomp/render.h"
#include "psprecomp/mem.h"
#include "psprecomp/cpu.h"

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

#define FB     0x04000000u          /* eDRAM */
#define LIST   0x08800000u
#define VERTS  0x08810000u

/* VTYPE: 8888 colour, 16-bit position, through mode. */
#define VTYPE_2D  ((7u << 2) | (2u << 7) | (1u << 23))

static uint32_t call(uint32_t nid, uint32_t a0, uint32_t a1, uint32_t a2,
                     uint32_t a3) {
    psp_cpu.r[PSP_REG_A0] = a0;
    psp_cpu.r[PSP_REG_A1] = a1;
    psp_cpu.r[PSP_REG_A2] = a2;
    psp_cpu.r[PSP_REG_A3] = a3;
    psp_hle_call(nid);
    return psp_cpu.r[PSP_REG_V0];
}

static uint32_t *g_list;
static uint32_t g_pc;

static void cmd(uint8_t op, uint32_t arg) {
    psp_write32(LIST + g_pc, ((uint32_t)op << 24) | (arg & 0xFFFFFF));
    g_pc += 4;
}

static void begin_list(void) {
    g_pc = 0;
    (void)g_list;
    /* Addresses do not fit in a 24-bit argument. FBP carries the low 24 bits
     * and FBW smuggles bits 24-31 in its own top byte; VADDR is relative to
     * GE_BASE. Getting this wrong yields a plausible-looking address and
     * silently draws nothing. */
    cmd(0x10, (VERTS >> 8) & 0xFF0000);            /* BASE */
    cmd(0x9C, FB & 0xFFFFFF);                      /* FBP */
    cmd(0x9D, ((FB >> 8) & 0xFF0000) | 480);       /* FBW + address high byte */
    cmd(0x12, VTYPE_2D);                           /* VTYPE */
    cmd(0x01, VERTS & 0xFFFFFF);                   /* VADDR */
}

static void end_list(void) {
    cmd(0x0F, 0);                    /* FINISH */
    cmd(0x0C, 0);                    /* END */
    call(0xAB49E76A, LIST, 0, 0, 0); /* sceGeListEnQueue */
}

/* One 16-bit-position, 8888-colour vertex. Colour precedes position. */
static void vertex(int idx, int x, int y, uint32_t rgba) {
    uint32_t a = VERTS + (uint32_t)idx * 12;   /* 4 colour + 6 pos, padded to 12 */
    psp_write32(a, rgba);
    psp_write16(a + 4, (uint16_t)x);
    psp_write16(a + 6, (uint16_t)y);
    psp_write16(a + 8, 0);
}

static uint32_t pixel(int x, int y) {
    return psp_read32(FB + (uint32_t)(y * 480 + x) * 4);
}

static void clear_fb(void) {
    for (int y = 0; y < 272; y++)
        for (int x = 0; x < 480; x++)
            psp_write32(FB + (uint32_t)(y * 480 + x) * 4, 0);
}

/* A sprite is the PSP's 2D quad: two vertices, opposite corners. */
static void test_sprite(void) {
    psp_ge_reset();
    clear_fb();
    begin_list();
    vertex(0, 100, 50, 0xFF0000FFu);
    vertex(1, 200, 150, 0xFF0000FFu);
    cmd(0x04, (6u << 16) | 2);       /* PRIM sprites, 2 vertices */
    end_list();

    CHECK(psp_ge_pixels() == 100 * 100, "sprite pixel count: %llu (want 10000)",
          (unsigned long long)psp_ge_pixels());
    CHECK(pixel(150, 100) == 0xFF0000FFu, "sprite interior: 0x%08X", pixel(150, 100));
    CHECK(pixel(99, 100) == 0, "left of sprite should be untouched: 0x%08X", pixel(99, 100));
    CHECK(pixel(150, 49) == 0, "above sprite should be untouched: 0x%08X", pixel(150, 49));
    /* Half-open on the far edge, so adjacent sprites tile without overlapping. */
    CHECK(pixel(200, 100) == 0, "far edge is exclusive: 0x%08X", pixel(200, 100));
}

static void test_triangle(void) {
    psp_ge_reset();
    clear_fb();
    begin_list();
    vertex(0, 10, 10, 0xFF00FF00u);
    vertex(1, 110, 10, 0xFF00FF00u);
    vertex(2, 10, 110, 0xFF00FF00u);
    cmd(0x04, (3u << 16) | 3);       /* PRIM triangles */
    end_list();

    CHECK(psp_ge_pixels() > 4000, "triangle should cover ~5000 px, got %llu",
          (unsigned long long)psp_ge_pixels());
    CHECK(pixel(20, 20) == 0xFF00FF00u, "inside triangle: 0x%08X", pixel(20, 20));
    /* The hypotenuse runs from (110,10) to (10,110); (100,100) is well past it. */
    CHECK(pixel(100, 100) == 0, "outside hypotenuse: 0x%08X", pixel(100, 100));
}

/* Winding must not matter while back-face culling is unimplemented, or half of
 * any real model silently disappears. */
static void test_winding(void) {
    psp_ge_reset();
    clear_fb();
    begin_list();
    vertex(0, 10, 10, 0xFFFFFFFFu);
    vertex(1, 10, 110, 0xFFFFFFFFu);
    vertex(2, 110, 10, 0xFFFFFFFFu);
    cmd(0x04, (3u << 16) | 3);
    end_list();
    CHECK(pixel(20, 20) == 0xFFFFFFFFu, "reversed winding still fills: 0x%08X",
          pixel(20, 20));
}

/* Off-screen geometry must clip, not scribble outside the framebuffer or wrap
 * to the opposite edge. */
static void test_clipping(void) {
    psp_ge_reset();
    clear_fb();
    begin_list();
    vertex(0, -50, -50, 0xFFFF0000u);
    vertex(1, 50, 50, 0xFFFF0000u);
    cmd(0x04, (6u << 16) | 2);
    end_list();

    CHECK(psp_ge_pixels() == 50 * 50, "clipped sprite: %llu (want 2500)",
          (unsigned long long)psp_ge_pixels());
    CHECK(pixel(0, 0) == 0xFFFF0000u, "clipped sprite covers origin");
    CHECK(pixel(479, 271) == 0, "far corner untouched: 0x%08X", pixel(479, 271));
}

/* Transformed geometry is not implemented. It must be *counted*, not drawn at
 * the wrong place — wrong pixels are harder to diagnose than no pixels. */
static void test_transformed_is_skipped(void) {
    psp_ge_reset();
    clear_fb();
    g_pc = 0;
    cmd(0x10, (VERTS >> 8) & 0xFF0000);
    cmd(0x9C, FB & 0xFFFFFF);
    cmd(0x9D, ((FB >> 8) & 0xFF0000) | 480);
    cmd(0x12, VTYPE_2D & ~(1u << 23));   /* through bit cleared */
    cmd(0x01, VERTS & 0xFFFFFF);
    vertex(0, 10, 10, 0xFFFFFFFFu);
    vertex(1, 110, 110, 0xFFFFFFFFu);
    cmd(0x04, (6u << 16) | 2);
    end_list();

    CHECK(psp_ge_pixels() == 0, "transformed geometry must not be drawn, got %llu px",
          (unsigned long long)psp_ge_pixels());
}

static void test_triangle_strip(void) {
    psp_ge_reset();
    clear_fb();
    begin_list();
    vertex(0, 10, 10, 0xFFFFFFFFu);
    vertex(1, 110, 10, 0xFFFFFFFFu);
    vertex(2, 10, 110, 0xFFFFFFFFu);
    vertex(3, 110, 110, 0xFFFFFFFFu);
    cmd(0x04, (4u << 16) | 4);       /* strip: 4 vertices -> 2 triangles */
    end_list();

    /* Two triangles sharing an edge tile the square without a seam. */
    CHECK(pixel(20, 20) == 0xFFFFFFFFu, "strip tri 0: 0x%08X", pixel(20, 20));
    CHECK(pixel(100, 100) == 0xFFFFFFFFu, "strip tri 1: 0x%08X", pixel(100, 100));
}

/* The backend interface itself. The software path is the reference every other
 * backend is diffed against, so selection has to be predictable: an unknown
 * name must not silently leave you rendering into nothing. */
static void test_backend_selection(void) {
    CHECK(psp_render_current() != NULL, "there is always a backend");
    CHECK(strcmp(psp_render_current()->name, "software") == 0,
          "software is the default, got %s", psp_render_current()->name);

    CHECK(psp_render_select("null") == 0, "null backend selectable");
    CHECK(strcmp(psp_render_current()->name, "null") == 0, "null is active");

    /* An unknown name keeps the current backend rather than falling back to
     * something arbitrary -- a typo should not silently change what renders. */
    CHECK(psp_render_select("vulkan-that-does-not-exist") != 0,
          "unknown backend rejected");
    CHECK(strcmp(psp_render_current()->name, "null") == 0,
          "rejected selection leaves the backend alone");

    CHECK(psp_render_select(NULL) != 0, "NULL name rejected");

    /* The null backend must draw nothing: it is what bring-up uses to ask
     * "did the game request a draw" without pixels confusing the answer. */
    psp_ge_reset();
    clear_fb();
    begin_list();
    vertex(0, 10, 10, 0xFFFFFFFFu);
    vertex(1, 60, 60, 0xFFFFFFFFu);
    cmd(0x04, (6u << 16) | 2);
    end_list();
    CHECK(psp_ge_pixels() == 0, "null backend wrote %llu pixels",
          (unsigned long long)psp_ge_pixels());

    CHECK(psp_render_select("software") == 0, "software reselectable");
}

int main(void) {
    if (psp_mem_init() != 0) { printf("memory init failed\n"); return 1; }
    psp_cpu_reset();
    psp_hle_init();
    psp_cpu.r[PSP_REG_SP] = 0x08F00000u;

    test_sprite();
    test_triangle();
    test_winding();
    test_clipping();
    test_transformed_is_skipped();
    test_triangle_strip();
    test_backend_selection();

    psp_mem_free();
    printf(failures ? "raster: %d failure(s)\n" : "raster: all tests passed\n", failures);
    return failures ? 1 : 0;
}
