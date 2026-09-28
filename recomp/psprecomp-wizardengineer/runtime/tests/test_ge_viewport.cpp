// Unit tests for ge_compute_viewport_depth: the PSP viewport scale/offset +
// depth-range registers -> GL viewport + depth range mapping (issue #23).
// Oracle: PPSSPP GPU/Common/GPUStateUtils.cpp ConvertViewportAndScissor
// (1:1, non-supersampled, non-accurate-depth FBO case).
//
// Standalone executable (test_vfpu convention): links psp_ge_viewport.cpp
// only -- pure float/int math, no SDL/GL/scheduler deps.

#include "psp_ge_draw.h"

#include <cmath>
#include <cstdio>
#include <cstdint>

static int failures = 0;
static int tests_run = 0;

#define ASSERT_EQ(actual, expected, msg) \
    do { \
        tests_run++; \
        const long a_ = static_cast<long>(actual); \
        const long e_ = static_cast<long>(expected); \
        if (a_ != e_) { \
            std::fprintf(stderr, \
                "FAIL: %s: got %ld, expected %ld\n", msg, a_, e_); \
            failures++; \
        } \
    } while (0)

#define ASSERT_NEAR(actual, expected, msg) \
    do { \
        tests_run++; \
        const float a_ = static_cast<float>(actual); \
        const float e_ = static_cast<float>(expected); \
        if (std::fabs(a_ - e_) > 1e-4f) { \
            std::fprintf(stderr, \
                "FAIL: %s: got %g, expected %g\n", msg, a_, e_); \
            failures++; \
        } \
    } while (0)

static constexpr int FBH = 272;  // PSP-native FBO height

// PSP defaults (psp_ge.cpp ge_init): a full-screen 480x272 viewport. The
// offsets that center it: off_x = vpXCenter - |vpXScale| = 2048 - 240 = 1808
// (raw 1808*16 = 28928); off_y = 2048 - 136 = 1912 (raw 1912*16 = 30592).
// This MUST reduce to the previously-hardcoded glViewport(0,0,480,272).
static void test_fullscreen_default_matches_hardcoded() {
    GeViewportDepth v = ge_compute_viewport_depth(
        240.0f, -136.0f, 2048.0f, 2048.0f,
        32767.5f, 32767.5f,
        1808u * 16u, 1912u * 16u, FBH);
    ASSERT_EQ(v.x, 0, "fullscreen x == 0");
    ASSERT_EQ(v.y, 0, "fullscreen gl-y == 0 (272 - 0 - 272)");
    ASSERT_EQ(v.w, 480, "fullscreen w == 480 (2*240)");
    ASSERT_EQ(v.h, 272, "fullscreen h == 272 (2*136)");
}

// Default full-range depth: center 32767.5, scale 32767.5 ->
// [0, 65535]/65535 = [0, 1].
static void test_fullscreen_depth_range() {
    GeViewportDepth v = ge_compute_viewport_depth(
        240.0f, -136.0f, 2048.0f, 2048.0f,
        32767.5f, 32767.5f, 0u, 0u, FBH);
    ASSERT_NEAR(v.near_z, 0.0f, "depth near == 0.0");
    ASSERT_NEAR(v.far_z, 1.0f, "depth far == 1.0");
}

// A negative Y-scale (PSP convention) must not flip the rectangle: width and
// height use |scale|, so the rect dims match the positive case.
static void test_negative_yscale_uses_abs() {
    GeViewportDepth pos = ge_compute_viewport_depth(
        240.0f, 136.0f, 2048.0f, 2048.0f, 1.0f, 1.0f,
        1808u * 16u, 1912u * 16u, FBH);
    GeViewportDepth neg = ge_compute_viewport_depth(
        240.0f, -136.0f, 2048.0f, 2048.0f, 1.0f, 1.0f,
        1808u * 16u, 1912u * 16u, FBH);
    ASSERT_EQ(neg.h, pos.h, "|+136| == |-136| height");
    ASSERT_EQ(neg.w, pos.w, "width unaffected by y-scale sign");
    ASSERT_EQ(neg.h, 272, "height == 272");
}

// A half-height viewport in the bottom half of the PSP screen (top-left
// origin) must land in the GL bottom-left origin correctly:
//   top = 136, h = 136 -> gl_y = 272 - 136 - 136 = 0 (bottom of FBO).
static void test_y_flip_bottom_half() {
    // Want left=0,w=480 (full width), top=136,h=136.
    // h=136 -> |yscale|=68. top = vpYCenter - off_y - 68 = 136.
    // Pick vpYCenter=2048 -> off_y = 2048 - 68 - 136 = 1844 (raw *16).
    GeViewportDepth v = ge_compute_viewport_depth(
        240.0f, -68.0f, 2048.0f, 2048.0f, 1.0f, 1.0f,
        1808u * 16u, 1844u * 16u, FBH);
    ASSERT_EQ(v.h, 136, "half height == 136");
    ASSERT_EQ(v.y, 0, "bottom-half PSP -> gl-y 0 (272-136-136)");
}

// OFFSETX/Y are 1/16-subpixel; only the low 16 bits are used. A raw word
// 1808*16 = 28928 must yield off_x = 1808.0 exactly, and high bits ignored.
static void test_offset_subpixel_and_mask() {
    // off raw with garbage high bits set; low 16 = 28928 -> 1808.0.
    uint32_t raw_x = 28928u | 0xFFFF0000u;
    GeViewportDepth v = ge_compute_viewport_depth(
        240.0f, -136.0f, 2048.0f, 2048.0f, 1.0f, 1.0f,
        raw_x, 1912u * 16u, FBH);
    ASSERT_EQ(v.x, 0, "off_x (low16 28928 -> 1808) gives left 0");
}

// Non-default depth window (e.g. minimized Z range) clamps into [0,1] and
// maps center+-scale through /65535.
static void test_depth_clamped_subrange() {
    // center 16384, scale 8192 -> near (16384-8192)/65535, far (16384+8192)/65535
    GeViewportDepth v = ge_compute_viewport_depth(
        240.0f, -136.0f, 2048.0f, 2048.0f,
        8192.0f, 16384.0f, 0u, 0u, FBH);
    ASSERT_NEAR(v.near_z, 8192.0f / 65535.0f, "near == 8192/65535");
    ASSERT_NEAR(v.far_z, 24576.0f / 65535.0f, "far == 24576/65535");
}

// Reversed-Z (issue #23 Fix-2 precondition). A negative ZSCALE (Patapon)
// makes near = (center - scale)/65535 LARGER than far = (center + scale)/65535,
// i.e. glDepthRange(near>far). The draw path keys its glClearDepth(far) and
// reversed depth-func mapping off exactly this near>far ordering, so assert it
// holds for a representative Patapon-style ZSCALE<0 / full ZCENTER.
static void test_reversed_z_range() {
    GeViewportDepth v = ge_compute_viewport_depth(
        240.0f, -136.0f, 2048.0f, 2048.0f,
        -32767.5f, 32767.5f, 0u, 0u, FBH);
    ASSERT_NEAR(v.near_z, 1.0f, "reversed near == 1.0 (center-scale)");
    ASSERT_NEAR(v.far_z, 0.0f, "reversed far == 0.0 (center+scale)");
    tests_run++;
    if (!(v.near_z > v.far_z)) {
        std::fprintf(stderr,
            "FAIL: reversed-Z must yield near>far (got near=%g far=%g)\n",
            v.near_z, v.far_z);
        failures++;
    }
}

int main() {
    test_fullscreen_default_matches_hardcoded();
    test_fullscreen_depth_range();
    test_negative_yscale_uses_abs();
    test_y_flip_bottom_half();
    test_offset_subpixel_and_mask();
    test_depth_clamped_subrange();
    test_reversed_z_range();

    if (failures == 0) {
        std::printf("test_ge_viewport: %d/%d PASS\n", tests_run, tests_run);
        return 0;
    }
    std::fprintf(stderr, "test_ge_viewport: %d/%d FAIL\n",
        tests_run - failures, tests_run);
    return 1;
}
