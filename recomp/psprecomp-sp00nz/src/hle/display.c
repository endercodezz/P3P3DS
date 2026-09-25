/* psprecomp — sceDisplay.
 *
 * The PSP's display is a framebuffer address plus a pixel format, scanned out
 * at ~59.94 Hz. There is no blitting and no compositing: a game writes pixels
 * (usually via the GE) and tells sceDisplay where they are.
 *
 * That makes this small but pivotal — it is the first point in bring-up where
 * a recompiled game produces something you can *look at*. psp_display_capture()
 * exists for exactly that: dump whatever the game currently considers the
 * front buffer, and compare it against the same frame from an emulator.
 */

#include "psprecomp/hle.h"

#include <stdio.h>
#include <string.h>

#define PSP_SCREEN_W 480
#define PSP_SCREEN_H 272

/* Pixel formats, as passed to sceDisplaySetFrameBuf. */
#define PSP_DISPLAY_PIXEL_FORMAT_565  0
#define PSP_DISPLAY_PIXEL_FORMAT_5551 1
#define PSP_DISPLAY_PIXEL_FORMAT_4444 2
#define PSP_DISPLAY_PIXEL_FORMAT_8888 3

static uint32_t g_fb_addr;
static uint32_t g_fb_width;      /* in pixels, the stride -- usually 512 */
static uint32_t g_fb_format;
static uint32_t g_mode, g_mode_w, g_mode_h;
static uint64_t g_vblank_count;

void psp_display_reset(void) {
    g_fb_addr = 0;
    g_fb_width = 512;
    g_fb_format = PSP_DISPLAY_PIXEL_FORMAT_8888;
    g_mode = 0;
    g_mode_w = PSP_SCREEN_W;
    g_mode_h = PSP_SCREEN_H;
    g_vblank_count = 0;
}

void psp_display_init(void) { psp_display_reset(); }

uint64_t psp_display_vblanks(void) { return g_vblank_count; }
uint32_t psp_display_framebuffer(void) { return g_fb_addr; }

/* Expand one source pixel to RGBA8888. The 16-bit formats replicate their high
 * bits into the low ones on expansion; simply shifting left leaves the maximum
 * value slightly below full white, which shows up as a washed-out image. */
static void expand(uint32_t px, uint32_t fmt, uint8_t out[4]) {
    switch (fmt) {
    case PSP_DISPLAY_PIXEL_FORMAT_565: {
        uint32_t r = (px & 0x1F), g = (px >> 5) & 0x3F, b = (px >> 11) & 0x1F;
        out[0] = (uint8_t)((r << 3) | (r >> 2));
        out[1] = (uint8_t)((g << 2) | (g >> 4));
        out[2] = (uint8_t)((b << 3) | (b >> 2));
        out[3] = 255;
        break;
    }
    case PSP_DISPLAY_PIXEL_FORMAT_5551: {
        uint32_t r = (px & 0x1F), g = (px >> 5) & 0x1F, b = (px >> 10) & 0x1F;
        out[0] = (uint8_t)((r << 3) | (r >> 2));
        out[1] = (uint8_t)((g << 3) | (g >> 2));
        out[2] = (uint8_t)((b << 3) | (b >> 2));
        out[3] = (px & 0x8000) ? 255 : 0;
        break;
    }
    case PSP_DISPLAY_PIXEL_FORMAT_4444: {
        uint32_t r = (px & 0xF), g = (px >> 4) & 0xF, b = (px >> 8) & 0xF, a = (px >> 12) & 0xF;
        out[0] = (uint8_t)((r << 4) | r);
        out[1] = (uint8_t)((g << 4) | g);
        out[2] = (uint8_t)((b << 4) | b);
        out[3] = (uint8_t)((a << 4) | a);
        break;
    }
    default:
        out[0] = (uint8_t)(px);
        out[1] = (uint8_t)(px >> 8);
        out[2] = (uint8_t)(px >> 16);
        out[3] = (uint8_t)(px >> 24);
        break;
    }
}

/* Write the current framebuffer as a binary PPM. Returns 0 on success, -1 if
 * no framebuffer has been set yet (which is itself worth knowing during
 * bring-up: the game never got as far as showing anything). */
int psp_display_capture(const char *path) {
    if (!g_fb_addr) return -1;

    FILE *f = fopen(path, "wb");
    if (!f) return -1;
    fprintf(f, "P6\n%d %d\n255\n", PSP_SCREEN_W, PSP_SCREEN_H);

    const int bpp = (g_fb_format == PSP_DISPLAY_PIXEL_FORMAT_8888) ? 4 : 2;
    for (int y = 0; y < PSP_SCREEN_H; y++) {
        for (int x = 0; x < PSP_SCREEN_W; x++) {
            uint32_t at = g_fb_addr + (uint32_t)(y * (int)g_fb_width + x) * (uint32_t)bpp;
            uint32_t px = (bpp == 4) ? psp_read32(at) : psp_read16(at);
            uint8_t rgba[4];
            expand(px, g_fb_format, rgba);
            fwrite(rgba, 1, 3, f);       /* PPM is RGB; alpha is dropped */
        }
    }
    fclose(f);
    return 0;
}

/* ---- the calls ----------------------------------------------------------- */

static void hle_SetMode(void) {
    g_mode   = psp_arg(0);
    g_mode_w = psp_arg(1);
    g_mode_h = psp_arg(2);
    psp_ret(SCE_KERNEL_ERROR_OK);
}

static void hle_SetFrameBuf(void) {
    /* (topaddr, bufferwidth, pixelformat, sync) */
    g_fb_addr   = psp_arg(0);
    g_fb_width  = psp_arg(1);
    g_fb_format = psp_arg(2);
    if (!g_fb_width) g_fb_width = 512;
    psp_ret(SCE_KERNEL_ERROR_OK);
}

/* There is no scanout, so a vblank wait returns immediately and bumps the
 * counter. A game's main loop is usually `render(); WaitVblank();`, which
 * means this counter is the frame number -- the most useful single number to
 * have during bring-up, because it tells you whether the game is looping or
 * stuck. */
static void hle_WaitVblank(void) {
    g_vblank_count++;
    psp_ret(SCE_KERNEL_ERROR_OK);
}

void psp_display_register(void) {
    psp_hle_register(0x0E20F177, "sceDisplay", "sceDisplaySetMode",           hle_SetMode);
    psp_hle_register(0x289D82FE, "sceDisplay", "sceDisplaySetFrameBuf",       hle_SetFrameBuf);
    psp_hle_register(0x36CDFADE, "sceDisplay", "sceDisplayWaitVblank",        hle_WaitVblank);
    psp_hle_register(0x46F186C3, "sceDisplay", "sceDisplayWaitVblankStartCB", hle_WaitVblank);
}
