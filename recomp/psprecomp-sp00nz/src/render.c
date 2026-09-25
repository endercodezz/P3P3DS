/* psprecomp — render backend selection, and the two backends that need no
 * external dependencies. See docs/RENDERER.md.
 *
 * The software backend is not a fallback. It is the reference: every other
 * backend is correct exactly insofar as it agrees with this one on the same
 * display list, and `tests/test_raster.c` pins its behaviour using synthetic
 * lists with no game data and no GPU. A GPU backend that cannot be diffed
 * against something is a backend whose bugs are indistinguishable from the
 * game's.
 */

#include "psprecomp/render.h"
#include "psprecomp/mem.h"

#include <string.h>

/* ---- shared target state ------------------------------------------------- */

static uint32_t g_fb_addr, g_fb_stride;
static uint64_t g_pixels;

uint64_t psp_render_pixels(void) { return g_pixels; }
void     psp_render_reset_pixels(void) { g_pixels = 0; }

/* ---- software backend ---------------------------------------------------- */

static int sw_init(int w, int h) { (void)w; (void)h; return 0; }
static void sw_shutdown(void) { }

static void sw_target(uint32_t addr, uint32_t stride, int fmt) {
    (void)fmt;
    g_fb_addr = addr;
    g_fb_stride = stride;
}

static void put_pixel(int x, int y, uint32_t rgba) {
    if (!g_fb_addr || !g_fb_stride) return;
    if (x < 0 || y < 0 || x >= 480 || y >= 272) return;
    psp_write32(g_fb_addr + (uint32_t)(y * (int)g_fb_stride + x) * 4, rgba);
    g_pixels++;
}

/* Barycentric fill with integer edge functions, so a shared edge belongs to
 * exactly one triangle: adjacent geometry neither double-draws nor leaves
 * seams. Both windings are accepted — back-face culling is not implemented, and
 * rejecting one winding would silently drop half of any real model. */
static void sw_tri(const psp_vertex *a, const psp_vertex *b, const psp_vertex *c) {
    int minx = a->x < b->x ? (a->x < c->x ? a->x : c->x) : (b->x < c->x ? b->x : c->x);
    int maxx = a->x > b->x ? (a->x > c->x ? a->x : c->x) : (b->x > c->x ? b->x : c->x);
    int miny = a->y < b->y ? (a->y < c->y ? a->y : c->y) : (b->y < c->y ? b->y : c->y);
    int maxy = a->y > b->y ? (a->y > c->y ? a->y : c->y) : (b->y > c->y ? b->y : c->y);

    if (minx < 0) minx = 0;
    if (miny < 0) miny = 0;
    if (maxx > 479) maxx = 479;
    if (maxy > 271) maxy = 271;

    const int area = (b->x - a->x) * (c->y - a->y) - (b->y - a->y) * (c->x - a->x);
    if (area == 0) return;

    for (int y = miny; y <= maxy; y++) {
        for (int x = minx; x <= maxx; x++) {
            int w0 = (b->x - a->x) * (y - a->y) - (b->y - a->y) * (x - a->x);
            int w1 = (c->x - b->x) * (y - b->y) - (c->y - b->y) * (x - b->x);
            int w2 = (a->x - c->x) * (y - c->y) - (a->y - c->y) * (x - c->x);
            if ((w0 >= 0 && w1 >= 0 && w2 >= 0) || (w0 <= 0 && w1 <= 0 && w2 <= 0))
                put_pixel(x, y, a->rgba);
        }
    }
}

/* A sprite is the PSP's 2D primitive: two vertices giving opposite corners of
 * an axis-aligned rectangle. The far edge is exclusive so adjacent sprites tile
 * without overlapping. */
static void sw_sprite(const psp_vertex *a, const psp_vertex *b) {
    int x0 = a->x < b->x ? a->x : b->x, x1 = a->x > b->x ? a->x : b->x;
    int y0 = a->y < b->y ? a->y : b->y, y1 = a->y > b->y ? a->y : b->y;
    for (int y = y0; y < y1; y++)
        for (int x = x0; x < x1; x++) put_pixel(x, y, b->rgba);
}

static void sw_draw(int prim, const psp_vertex *v, int count) {
    switch (prim) {
    case PSP_PRIM_SPRITES:
        for (int i = 0; i + 1 < count; i += 2) sw_sprite(&v[i], &v[i + 1]);
        break;
    case PSP_PRIM_TRIANGLES:
        for (int i = 0; i + 2 < count; i += 3) sw_tri(&v[i], &v[i + 1], &v[i + 2]);
        break;
    case PSP_PRIM_TRIANGLE_STRIP:
        for (int i = 0; i + 2 < count; i++) sw_tri(&v[i], &v[i + 1], &v[i + 2]);
        break;
    default:
        break;                       /* points, lines, fans: not yet */
    }
}

static void sw_noop(void) { }

const psp_render_backend psp_render_software = {
    "software", sw_init, sw_shutdown, sw_target, sw_draw, sw_noop, sw_noop
};

/* ---- null backend -------------------------------------------------------- */

/* Draws nothing, and that is the point. During bring-up the question is
 * usually "did the game ask to draw" rather than "does it look right", and the
 * answer is clearer without pixels in the way. */

static int null_init(int w, int h) { (void)w; (void)h; return 0; }
static void null_target(uint32_t a, uint32_t s, int f) { (void)a; (void)s; (void)f; }
static void null_draw(int p, const psp_vertex *v, int n) { (void)p; (void)v; (void)n; }
static void null_noop(void) { }

const psp_render_backend psp_render_null = {
    "null", null_init, null_noop, null_target, null_draw, null_noop, null_noop
};

/* ---- selection ----------------------------------------------------------- */

static const psp_render_backend *g_backend = &psp_render_software;

const psp_render_backend *psp_render_current(void) { return g_backend; }

int psp_render_select(const char *name) {
    static const psp_render_backend *const all[] = {
        &psp_render_software, &psp_render_null
    };
    if (!name) return -1;
    for (size_t i = 0; i < sizeof all / sizeof all[0]; i++) {
        if (strcmp(all[i]->name, name) == 0) { g_backend = all[i]; return 0; }
    }
    return -1;                       /* unknown: keep the current backend */
}
