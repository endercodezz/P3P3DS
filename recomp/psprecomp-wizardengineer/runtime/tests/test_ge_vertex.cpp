// Unit tests for ge_transform_vertices through-mode UV normalization
// (dothack-L6c fix). PSP through-mode (sprite/2D) texture coordinates
// arrive in TEXEL units; GL samples in normalized [0,1] coords, so the
// runtime must divide each textured through-mode UV by the texture
// dimensions. Oracle: PPSSPP GPU/Common/SoftwareTransformCommon.cpp
// (uscale /= curTextureWidth; vscale /= curTextureHeight).
//
// Standalone executable (test_vfpu convention): links psp_ge_vertex.cpp
// and calls the real ge_transform_vertices — no SDL/GL/scheduler deps.

#include "psp_ge_vertex.h"
#include "psp_ge.h"

#include <cmath>
#include <cstdio>
#include <vector>

static int failures = 0;
static int tests_run = 0;

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

// Build a through-mode GeState with identity tex scale/offset and a
// 128x128 base texture (log2 = 7 -> tex_size[0] = 7 | (7<<8) = 0x0707).
static GeState make_through_state(bool texture_enable) {
    GeState s;
    s.reset();
    s.vertex_type = 0x00800000u;  // bit 23 = through-mode
    s.tex_scale_u = 1.0f;
    s.tex_scale_v = 1.0f;
    s.tex_offset_u = 0.0f;
    s.tex_offset_v = 0.0f;
    s.tex_size[0] = 7u | (7u << 8);  // 128 x 128
    s.texture_enable = texture_enable;
    return s;
}

static DecodedVertex make_vertex(float u, float v) {
    DecodedVertex vtx;
    vtx.pos[0] = 100.0f;
    vtx.pos[1] = 100.0f;
    vtx.pos[2] = 0.0f;
    vtx.uv[0] = u;
    vtx.uv[1] = v;
    vtx.has_uv = true;
    vtx.has_color = false;
    vtx.has_normal = false;
    return vtx;
}

// Texturing enabled: texel-unit UVs are normalized by texW/texH.
static void test_through_textured_normalizes() {
    GeState s = make_through_state(true);
    std::vector<DecodedVertex> verts = {make_vertex(64.0f, 128.0f)};
    ge_transform_vertices(verts, s);
    ASSERT_NEAR(verts[0].uv[0], 0.5f,
        "through textured u: 64/128 -> 0.5");
    ASSERT_NEAR(verts[0].uv[1], 1.0f,
        "through textured v: 128/128 -> 1.0");
}

// Texturing disabled: a non-textured through quad must NOT be divided.
static void test_through_untextured_unchanged() {
    GeState s = make_through_state(false);
    std::vector<DecodedVertex> verts = {make_vertex(64.0f, 128.0f)};
    ge_transform_vertices(verts, s);
    ASSERT_NEAR(verts[0].uv[0], 64.0f,
        "through untextured u: scale/offset only, no divide");
    ASSERT_NEAR(verts[0].uv[1], 128.0f,
        "through untextured v: scale/offset only, no divide");
}

// Normalization is applied AFTER tex_scale/offset (matches the measured
// poke that divided the post-scale UV).
static void test_through_scale_offset_then_normalize() {
    GeState s = make_through_state(true);
    s.tex_scale_u = 2.0f;
    s.tex_offset_u = 0.0f;
    std::vector<DecodedVertex> verts = {make_vertex(32.0f, 64.0f)};
    ge_transform_vertices(verts, s);
    // u: 32*2 = 64, then /128 = 0.5
    ASSERT_NEAR(verts[0].uv[0], 0.5f,
        "through textured u: (32*2)/128 -> 0.5");
    ASSERT_NEAR(verts[0].uv[1], 0.5f,
        "through textured v: 64/128 -> 0.5");
}

// ---- Collapsed-MVP degeneracy detection (Patapon-restore fix) ----
//
// The guest uploads matrices that are non-zero and non-singular but compose
// into an MVP that maps a whole primitive onto a sub-pixel cluster (#27
// dataflow family). ge_transform_vertices must detect this generically and
// route to the installed degenerate fallback. We install a sentinel fallback
// that stamps a marker so the test can tell which path ran.

static bool g_fallback_fired = false;
static void sentinel_fallback(const float wpos[3], float out[3]) {
    g_fallback_fired = true;
    // Map to a recognizable NDC so we can also assert the values.
    out[0] = wpos[0] / 240.0f - 1.0f;
    out[1] = wpos[1] / 136.0f + 1.0f;
    out[2] = 0.0f;
}

// Build a transform-mode state with the exact broken Patapon title matrices
// (col-major) recorded in .planning/research/patapon-visible-output.md.
static GeState make_broken_title_state() {
    GeState s;
    s.reset();
    s.vertex_type = 0x000183u;  // transform mode (bit 23 clear)
    const float world[12] = {
        1, 0, 0,   0, 0, 1,   0, -1, 0,   240, -136, 0};
    const float view[12] = {
        -0.006f, 0, 0,   0, -0.006f, 0,   0, 0, 1,   1.502f, -0.851f, 1.0f};
    const float proj[16] = {
        0.007f, 0, 0, 0,   0, 0.012f, 0, 0,
        0, 0, -0.002f, 0,  0, 0, -1.0f, 1.0f};
    for (int i = 0; i < 12; i++) { s.world_matrix[i] = world[i];
                                   s.view_matrix[i] = view[i]; }
    for (int i = 0; i < 16; i++) s.proj_matrix[i] = proj[i];
    return s;
}

static DecodedVertex make_pos_vertex(float x, float y, float z) {
    DecodedVertex v;
    v.pos[0] = x; v.pos[1] = y; v.pos[2] = z;
    v.uv[0] = 0.0f; v.uv[1] = 0.0f;
    v.has_uv = false; v.has_color = false; v.has_normal = false;
    return v;
}

// The broken title matrices must be detected as collapsed and routed to the
// fallback (which stamps the marker).
static void test_collapsed_mvp_routes_to_fallback() {
    g_fallback_fired = false;
    ge_vertex_set_degenerate_fallback(sentinel_fallback);
    GeState s = make_broken_title_state();
    // A title glyph quad spanning a non-trivial model extent.
    std::vector<DecodedVertex> verts = {
        make_pos_vertex(0.0f, 0.0f, 0.0f),
        make_pos_vertex(40.0f, 0.0f, 0.0f),
        make_pos_vertex(40.0f, 24.0f, 0.0f),
        make_pos_vertex(0.0f, 24.0f, 0.0f),
    };
    ge_transform_vertices(verts, s);
    tests_run++;
    if (!g_fallback_fired) {
        std::fprintf(stderr,
            "FAIL: collapsed MVP did not route to fallback\n");
        failures++;
    }
    ge_vertex_set_degenerate_fallback(nullptr);
}

// A healthy, full-screen ortho MVP must NOT be flagged as collapsed: the
// fallback must stay silent and the real transform path must run.
static void test_healthy_mvp_no_fallback() {
    g_fallback_fired = false;
    ge_vertex_set_degenerate_fallback(sentinel_fallback);
    GeState s;
    s.reset();
    s.vertex_type = 0x000183u;
    // Identity world & view; ortho proj mapping x in [0,480]/y in [0,272]
    // to NDC [-1,1] (col-major 4x4). m0 = 2/480, m5 = 2/272.
    s.world_matrix[0] = 1; s.world_matrix[4] = 1; s.world_matrix[8] = 1;
    s.view_matrix[0] = 1; s.view_matrix[4] = 1; s.view_matrix[8] = 1;
    s.proj_matrix[0]  = 2.0f / 480.0f;
    s.proj_matrix[5]  = 2.0f / 272.0f;
    s.proj_matrix[10] = -1.0f;
    s.proj_matrix[12] = -1.0f;  // x offset
    s.proj_matrix[13] = -1.0f;  // y offset
    s.proj_matrix[15] = 1.0f;
    std::vector<DecodedVertex> verts = {
        make_pos_vertex(0.0f, 0.0f, 0.0f),
        make_pos_vertex(480.0f, 0.0f, 0.0f),
        make_pos_vertex(480.0f, 272.0f, 0.0f),
        make_pos_vertex(0.0f, 272.0f, 0.0f),
    };
    ge_transform_vertices(verts, s);
    tests_run++;
    if (g_fallback_fired) {
        std::fprintf(stderr,
            "FAIL: healthy MVP wrongly routed to degenerate fallback\n");
        failures++;
    }
    ge_vertex_set_degenerate_fallback(nullptr);
}

// A legitimately tiny prim (small model extent) under the SAME broken matrices
// must NOT trigger the collapse detector: its small NDC image is consistent
// with its small input, so we cannot prove the MVP is broken from it. This
// guards against over-triggering on genuinely small geometry.
static void test_tiny_prim_no_overtrigger() {
    g_fallback_fired = false;
    ge_vertex_set_degenerate_fallback(sentinel_fallback);
    GeState s = make_broken_title_state();
    // Sub-unit model spread: below MODEL_SPREAD_MIN.
    std::vector<DecodedVertex> verts = {
        make_pos_vertex(0.0f, 0.0f, 0.0f),
        make_pos_vertex(0.2f, 0.2f, 0.0f),
    };
    ge_transform_vertices(verts, s);
    tests_run++;
    if (g_fallback_fired) {
        std::fprintf(stderr,
            "FAIL: tiny prim over-triggered the collapse detector\n");
        failures++;
    }
    ge_vertex_set_degenerate_fallback(nullptr);
}

int main() {
    test_through_textured_normalizes();
    test_through_untextured_unchanged();
    test_through_scale_offset_then_normalize();
    test_collapsed_mvp_routes_to_fallback();
    test_healthy_mvp_no_fallback();
    test_tiny_prim_no_overtrigger();

    if (failures == 0) {
        std::printf("test_ge_vertex: %d/%d PASS\n", tests_run, tests_run);
        return 0;
    }
    std::fprintf(stderr, "test_ge_vertex: %d/%d FAIL\n",
        tests_run - failures, tests_run);
    return 1;
}
