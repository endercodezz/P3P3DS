#include "psp_ge_draw.h"
#include "psp_ge.h"
#include "psp_ge_constants.h"
#include "psp_ge_vertex.h"
#include "psp_ge_texture.h"
#include "psp_ge_shader.h"
#include "psp_event_loop.h"

#include <SDL.h>
#include <glad/glad.h>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <vector>
#include <cstdio>
#include <cstdlib>
#include <cstring>

// ---- Generic FBO pool (issue #59 — honor fb_addr / double buffer) ----
// One GL FBO per distinct guest VRAM framebuffer address. The GE binds the
// FBO for the current FRAMEBUFPTR; present blits the FBO matching the
// displayed (sceDisplaySetFrameBuf) address. Keying on the guest's own
// addresses keeps this generic: a single-buffered title keys everything to
// one slot and degenerates to the old single-surface behavior.
static constexpr int GE_MAX_TARGETS = 4;

struct GeTarget {
    uint32_t key = 0;          // fb_addr normalized to eDRAM offset (0 = empty)
    GLuint fbo = 0;
    GLuint color_tex = 0;
    GLuint depth_rb = 0;
    uint64_t last_use = 0;     // LRU stamp (monotonic counter)
};

static GeTarget g_targets[GE_MAX_TARGETS];
static int g_current_target = -1;   // slot the GE is rendering into
static int g_front_target = -1;     // slot last presented / most recent render
static uint64_t g_target_clock = 0; // monotonic LRU counter

// Normalize any guest framebuffer address to its eDRAM 2 MB offset key.
// Strips the cached/uncached/eDRAM base via the canonical 0x07FFFFFF PSP
// mask, then takes the 16-byte-aligned eDRAM offset (PPSSPP model). Used by
// both ge_draw_select_target (FRAMEBUFPTR) and ge_present_frame
// (sceDisplaySetFrameBuf) so the two address spaces reconcile.
static inline uint32_t ge_fb_key(uint32_t addr) {
    return (addr & 0x07FFFFFFu) & 0x001FFFF0u;
}

// ---- Liveness present (no-flip safety net) ----
static bool g_present_dirty = false;
static std::chrono::steady_clock::time_point g_last_present;
static int g_present_stale_ms = 100;   // PSPRECOMP_PRESENT_STALE_MS

// ---- Module state ----
static GLuint g_vao = 0;
static GLuint g_vbo = 0;
static uint32_t g_frame_counter = 0;
static bool g_has_drawn_prims = false;
static bool g_screenshot_enabled = false;
static bool g_screenshot_done = false;
static char g_screenshot_path[256] = {};
static std::atomic<bool> g_draw_ready{false};

// ---- PRIM counters (file scope so ge_draw_get_stats can read them) ----
// Written only by the render thread inside ge_draw_prim; read racily by
// the debug socket's I command (a few prims stale is fine).
static int g_prim_type_count[8] = {0};   // PRIM types 0-6 + slot 7 safety
static int g_prim_type_count_clear[8] = {0};
static int g_prim_type_zero_count = 0;   // count <= 0 (degenerate)
static int g_prim_total_observed = 0;

// ---- On-demand screenshot request (issue #35 debug socket S command) ----
// Requester (any thread) posts a path + waits on the condvar; the render
// thread polls ge_draw_service_screenshot_request() each event-loop pass
// and performs the GL readback there (GL never leaves the main thread).
static std::mutex g_ss_req_mutex;
static std::condition_variable g_ss_req_cv;
static char g_ss_req_path[512] = {};
enum SsReqState { SS_IDLE = 0, SS_PENDING = 1, SS_DONE = 2, SS_FAILED = 3 };
static int g_ss_req_state = SS_IDLE;

// ---- Packed vertex for VBO upload ----
// Layout: 3 floats (pos) + 2 floats (uv) + 4 bytes (color)
// Total: 12 + 8 + 4 = 24 bytes
struct PackedVertex {
    float pos[3];
    float uv[2];
    uint8_t color[4];
};
static_assert(sizeof(PackedVertex) == 24,
              "PackedVertex must be 24 bytes");

// ---- Screenshot (TGA format -- no external dependency) ----

static bool write_tga(
    const char* path,
    const uint8_t* pixels,
    int width,
    int height
) {
    FILE* f = std::fopen(path, "wb");
    if (!f) {
        std::fprintf(stderr,
            "[DRAW] Cannot open %s for screenshot\n",
            path);
        return false;
    }

    // TGA header (18 bytes) -- uncompressed RGBA
    uint8_t header[18] = {};
    header[2]  = 2;  // Uncompressed true-color
    header[12] = width & 0xFF;
    header[13] = (width >> 8) & 0xFF;
    header[14] = height & 0xFF;
    header[15] = (height >> 8) & 0xFF;
    header[16] = 32;  // bits per pixel
    header[17] = 0x28; // origin upper-left + 8 alpha bits

    std::fwrite(header, 1, 18, f);

    // Write pixels (BGRA for TGA)
    // Input is RGBA, need to swap R and B
    std::vector<uint8_t> bgra(width * height * 4);
    for (int i = 0; i < width * height; i++) {
        bgra[i * 4 + 0] = pixels[i * 4 + 2];  // B
        bgra[i * 4 + 1] = pixels[i * 4 + 1];  // G
        bgra[i * 4 + 2] = pixels[i * 4 + 0];  // R
        bgra[i * 4 + 3] = pixels[i * 4 + 3];  // A
    }
    std::fwrite(bgra.data(), 1, bgra.size(), f);
    std::fclose(f);

    std::fprintf(stderr,
        "[DRAW] Screenshot saved: %s (%dx%d)\n",
        path, width, height);
    return true;
}

// Resolve the FBO id of the most-recently-presented (front) target, or the
// current render target as a fallback. Returns 0 if no target exists yet.
static GLuint front_fbo_id() {
    if (g_front_target >= 0) return g_targets[g_front_target].fbo;
    if (g_current_target >= 0) return g_targets[g_current_target].fbo;
    return 0;
}

/// Read the FBO back, flip, and write a TGA. Render (GL) thread only.
static bool capture_fbo_to_tga(const char* path) {
    GLuint fbo = front_fbo_id();
    glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo);
    std::vector<uint8_t> pixels(480 * 272 * 4);
    glReadPixels(0, 0, 480, 272, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
    // Flip vertically (GL origin is bottom-left)
    std::vector<uint8_t> flipped(480 * 272 * 4);
    for (int y = 0; y < 272; y++) {
        std::memcpy(flipped.data() + y * 480 * 4,
                    pixels.data() + (271 - y) * 480 * 4, 480 * 4);
    }
    return write_tga(path, flipped.data(), 480, 272);
}

// ---- GL state mapping ----

static GLenum map_blend_factor_src(int factor) {
    switch (factor) {
    case GE_SRCBLEND_DSTCOLOR:       return GL_DST_COLOR;
    case GE_SRCBLEND_INVDSTCOLOR:
        return GL_ONE_MINUS_DST_COLOR;
    case GE_SRCBLEND_SRCALPHA:       return GL_SRC_ALPHA;
    case GE_SRCBLEND_INVSRCALPHA:
        return GL_ONE_MINUS_SRC_ALPHA;
    case GE_SRCBLEND_DSTALPHA:       return GL_DST_ALPHA;
    case GE_SRCBLEND_INVDSTALPHA:
        return GL_ONE_MINUS_DST_ALPHA;
    case GE_SRCBLEND_DOUBLESRCALPHA:
        return GL_SRC_ALPHA;  // Approximate
    case GE_SRCBLEND_DOUBLEINVSRCALPHA:
        return GL_ONE_MINUS_SRC_ALPHA;
    case GE_SRCBLEND_DOUBLEDSTALPHA:
        return GL_DST_ALPHA;
    case GE_SRCBLEND_DOUBLEINVDSTALPHA:
        return GL_ONE_MINUS_DST_ALPHA;
    case GE_SRCBLEND_FIXA:           return GL_CONSTANT_COLOR;
    default:                         return GL_SRC_ALPHA;
    }
}

static GLenum map_blend_factor_dst(int factor) {
    switch (factor) {
    case GE_DSTBLEND_SRCCOLOR:       return GL_SRC_COLOR;
    case GE_DSTBLEND_INVSRCCOLOR:
        return GL_ONE_MINUS_SRC_COLOR;
    case GE_DSTBLEND_SRCALPHA:       return GL_SRC_ALPHA;
    case GE_DSTBLEND_INVSRCALPHA:
        return GL_ONE_MINUS_SRC_ALPHA;
    case GE_DSTBLEND_DSTALPHA:       return GL_DST_ALPHA;
    case GE_DSTBLEND_INVDSTALPHA:
        return GL_ONE_MINUS_DST_ALPHA;
    case GE_DSTBLEND_DOUBLESRCALPHA:
        return GL_SRC_ALPHA;
    case GE_DSTBLEND_DOUBLEINVSRCALPHA:
        return GL_ONE_MINUS_SRC_ALPHA;
    case GE_DSTBLEND_DOUBLEDSTALPHA:
        return GL_DST_ALPHA;
    case GE_DSTBLEND_DOUBLEINVDSTALPHA:
        return GL_ONE_MINUS_DST_ALPHA;
    case GE_DSTBLEND_FIXB:           return GL_CONSTANT_COLOR;
    default:
        return GL_ONE_MINUS_SRC_ALPHA;
    }
}

static GLenum map_blend_equation(int op) {
    switch (op) {
    case GE_BLENDMODE_MUL_AND_ADD:              return GL_FUNC_ADD;
    case GE_BLENDMODE_MUL_AND_SUBTRACT:
        return GL_FUNC_SUBTRACT;
    case GE_BLENDMODE_MUL_AND_SUBTRACT_REVERSE:
        return GL_FUNC_REVERSE_SUBTRACT;
    case GE_BLENDMODE_MIN:                      return GL_MIN;
    case GE_BLENDMODE_MAX:                      return GL_MAX;
    case GE_BLENDMODE_ABSDIFF:                  return GL_FUNC_ADD;
    default:                                    return GL_FUNC_ADD;
    }
}

// Map the PSP depth-compare to a GL depth func. When the active depth range
// is reversed (glDepthRange near>far, issue #23 -- Patapon's ZSCALE<0), GL
// stores depth on the flipped axis, so the inequality direction must flip too
// (PPSSPP does the same in its reversed-depth path). EQUAL/NOTEQUAL/NEVER/
// ALWAYS are direction-agnostic and pass through unchanged.
static GLenum map_depth_func(int func, bool reversed) {
    GLenum gl;
    switch (func) {
    case GE_COMP_NEVER:    return GL_NEVER;
    case GE_COMP_ALWAYS:   return GL_ALWAYS;
    case GE_COMP_EQUAL:    return GL_EQUAL;
    case GE_COMP_NOTEQUAL: return GL_NOTEQUAL;
    case GE_COMP_LESS:     gl = GL_LESS;    break;
    case GE_COMP_LEQUAL:   gl = GL_LEQUAL;  break;
    case GE_COMP_GREATER:  gl = GL_GREATER; break;
    case GE_COMP_GEQUAL:   gl = GL_GEQUAL;  break;
    default:               gl = GL_LEQUAL;  break;
    }
    if (!reversed) return gl;
    switch (gl) {
    case GL_LESS:    return GL_GREATER;
    case GL_LEQUAL:  return GL_GEQUAL;
    case GL_GREATER: return GL_LESS;
    case GL_GEQUAL:  return GL_LEQUAL;
    default:         return gl;
    }
}

static GLenum map_prim_type(int prim_type) {
    switch (prim_type) {
    case GE_PRIM_POINTS:         return GL_POINTS;
    case GE_PRIM_LINES:          return GL_LINES;
    case GE_PRIM_LINE_STRIP:     return GL_LINE_STRIP;
    case GE_PRIM_TRIANGLES:      return GL_TRIANGLES;
    case GE_PRIM_TRIANGLE_STRIP: return GL_TRIANGLE_STRIP;
    case GE_PRIM_TRIANGLE_FAN:   return GL_TRIANGLE_FAN;
    case GE_PRIM_RECTANGLES:     return GL_TRIANGLES;
    default:                     return GL_TRIANGLES;
    }
}

// PSP-native render-target dimensions. The runtime always renders into a
// 480x272 FBO at 1:1 PSP resolution; window upscaling happens later at
// present (blit). So the NDC->screen viewport maps directly to FBO pixels.
static constexpr int PSP_FB_WIDTH = 480;
static constexpr int PSP_FB_HEIGHT = 272;

// Compute the PSP viewport scale/offset + depth-range -> GL viewport/depth
// range for the live state. Generic for all games (issue #23). The math lives
// in the pure ge_compute_viewport_depth (tests/test_ge_viewport.cpp) so it is
// unit-testable without a GL context; callers issue the GL calls themselves.
static GeViewportDepth compute_state_viewport_depth(const GeState& state) {
    return ge_compute_viewport_depth(
        state.viewport_x_scale, state.viewport_y_scale,
        state.viewport_x_center, state.viewport_y_center,
        state.viewport_z_scale, state.viewport_z_center,
        state.offset_x, state.offset_y, PSP_FB_HEIGHT);
}

// A reversed depth range (PSP ZSCALE<0 -> glDepthRange near>far, issue #23)
// means the depth axis is flipped: the GL "far" value is the smaller of the
// two and is what a depth clear must fill so geometry passes the test, and
// the depth-compare direction must flip to match.
static bool depth_range_reversed(const GeViewportDepth& vp) {
    return vp.near_z > vp.far_z;
}

// ---- FBO pool helpers ----

// Create one render target (FBO + RGBA8 color texture + Depth24 renderbuffer,
// 480x272, NEAREST) for the given normalized key. Clears it to black. The
// pool owns the GL objects; ge_draw_shutdown deletes them.
static GeTarget make_target(uint32_t key) {
    GeTarget t;
    t.key = key;
    glGenFramebuffers(1, &t.fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, t.fbo);

    glGenTextures(1, &t.color_tex);
    glBindTexture(GL_TEXTURE_2D, t.color_tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 480, 272, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                           GL_TEXTURE_2D, t.color_tex, 0);

    glGenRenderbuffers(1, &t.depth_rb);
    glBindRenderbuffer(GL_RENDERBUFFER, t.depth_rb);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 480, 272);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                              GL_RENDERBUFFER, t.depth_rb);

    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        std::fprintf(stderr, "[DRAW] FBO incomplete: 0x%04X (key=0x%06X)\n",
                     status, key);
    }
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    t.last_use = ++g_target_clock;
    return t;
}

// Find the pool slot for key, or allocate one (LRU-evicting if full).
static int acquire_target(uint32_t key) {
    for (int i = 0; i < GE_MAX_TARGETS; ++i) {
        if (g_targets[i].fbo != 0 && g_targets[i].key == key) {
            g_targets[i].last_use = ++g_target_clock;
            return i;
        }
    }
    int slot = -1;
    for (int i = 0; i < GE_MAX_TARGETS; ++i) {
        if (g_targets[i].fbo == 0) { slot = i; break; }
    }
    if (slot < 0) {
        slot = 0;  // LRU evict
        for (int i = 1; i < GE_MAX_TARGETS; ++i) {
            if (g_targets[i].last_use < g_targets[slot].last_use) slot = i;
        }
        std::fprintf(stderr,
            "[DRAW] FBO pool overflow: evicting key=0x%06X for key=0x%06X\n",
            g_targets[slot].key, key);
        glDeleteFramebuffers(1, &g_targets[slot].fbo);
        glDeleteTextures(1, &g_targets[slot].color_tex);
        glDeleteRenderbuffers(1, &g_targets[slot].depth_rb);
    }
    g_targets[slot] = make_target(key);
    std::fprintf(stderr, "[DRAW] FBO target created key=0x%06X slot=%d\n",
                 key, slot);
    return slot;
}

// Bind the FBO for the current FRAMEBUFPTR. key==0 (FRAMEBUFPTR not yet seen)
// keeps the current/last target so early lists still land somewhere.
void ge_draw_select_target(uint32_t fb_addr_raw) {
    uint32_t key = ge_fb_key(fb_addr_raw);
    if (key == 0) {
        if (g_current_target < 0) g_current_target = acquire_target(0);
        glBindFramebuffer(GL_FRAMEBUFFER, g_targets[g_current_target].fbo);
        return;
    }
    int slot = acquire_target(key);
    g_current_target = slot;
    if (g_front_target < 0) g_front_target = slot;
    glBindFramebuffer(GL_FRAMEBUFFER, g_targets[slot].fbo);
}

void ge_draw_mark_dirty() {
    g_present_dirty = true;
}

// ---- Public API ----

void ge_draw_init() {
    // Check screenshot env var
    const char* ss_env =
        std::getenv("PSPRECOMP_SCREENSHOT");
    if (ss_env && ss_env[0]) {
        g_screenshot_enabled = true;
        std::strncpy(g_screenshot_path, ss_env,
                     sizeof(g_screenshot_path) - 1);
    }

    // Liveness-present budget (no-flip safety net). Env-overridable.
    const char* stale_env = std::getenv("PSPRECOMP_PRESENT_STALE_MS");
    if (stale_env && stale_env[0]) {
        int v = std::atoi(stale_env);
        if (v > 0) g_present_stale_ms = v;
    }
    g_last_present = std::chrono::steady_clock::now();

    // FBO targets are created lazily per guest framebuffer address by
    // ge_draw_select_target. No target is pre-created here.
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Create VAO + VBO
    glGenVertexArrays(1, &g_vao);
    glGenBuffers(1, &g_vbo);

    glBindVertexArray(g_vao);
    glBindBuffer(GL_ARRAY_BUFFER, g_vbo);

    // Vertex attributes for PackedVertex layout
    // location 0: position (3 floats, offset 0)
    glVertexAttribPointer(
        0, 3, GL_FLOAT, GL_FALSE,
        sizeof(PackedVertex),
        reinterpret_cast<void*>(0));
    glEnableVertexAttribArray(0);

    // location 1: texcoord (2 floats, offset 12)
    glVertexAttribPointer(
        1, 2, GL_FLOAT, GL_FALSE,
        sizeof(PackedVertex),
        reinterpret_cast<void*>(12));
    glEnableVertexAttribArray(1);

    // location 2: color (4 bytes normalized, offset 20)
    glVertexAttribPointer(
        2, 4, GL_UNSIGNED_BYTE, GL_TRUE,
        sizeof(PackedVertex),
        reinterpret_cast<void*>(20));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);

    // Init shader
    ge_shader_init();

    g_frame_counter = 0;
    g_draw_ready.store(true, std::memory_order_release);

    std::fprintf(stderr,
        "[DRAW] Draw infrastructure initialized\n");
}

void ge_draw_shutdown() {
    g_draw_ready.store(false, std::memory_order_release);
    // Auto-capture final frame if any PRIMs were rendered and no screenshot taken yet
    if (g_has_drawn_prims && !g_screenshot_done) {
        glBindFramebuffer(GL_READ_FRAMEBUFFER, front_fbo_id());
        std::vector<uint8_t> pixels(480 * 272 * 4);
        glReadPixels(0, 0, 480, 272, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
        // Flip vertically (GL origin is bottom-left)
        std::vector<uint8_t> flipped(480 * 272 * 4);
        for (int y = 0; y < 272; y++) {
            std::memcpy(flipped.data() + y * 480 * 4,
                        pixels.data() + (271 - y) * 480 * 4, 480 * 4);
        }
        const char* out_path = (g_screenshot_path[0] != '\0')
            ? g_screenshot_path : "frame.tga";
        write_tga(out_path, flipped.data(), 480, 272);
        g_screenshot_done = true;
        std::fprintf(stderr, "[DRAW] Auto-capture: %s\n", out_path);
    }

    ge_shader_shutdown();

    if (g_vbo) {
        glDeleteBuffers(1, &g_vbo);
        g_vbo = 0;
    }
    if (g_vao) {
        glDeleteVertexArrays(1, &g_vao);
        g_vao = 0;
    }
    for (int i = 0; i < GE_MAX_TARGETS; ++i) {
        GeTarget& t = g_targets[i];
        if (t.depth_rb) glDeleteRenderbuffers(1, &t.depth_rb);
        if (t.color_tex) glDeleteTextures(1, &t.color_tex);
        if (t.fbo) glDeleteFramebuffers(1, &t.fbo);
        t = GeTarget{};
    }
    g_current_target = -1;
    g_front_target = -1;

    std::fprintf(stderr,
        "[DRAW] Draw infrastructure shutdown\n");
}

void ge_draw_begin_list() {
    // Bind the FBO for the list's current render-target address (FRAMEBUFPTR).
    // Geometry and clears in this list now target the per-address FBO, so a
    // clear to address X cannot wipe geometry on address Y (issue #59).
    ge_draw_select_target(ge_get_state().framebuf_ptr);
    // Full-buffer default; the precise PSP viewport (scale/offset + depth
    // range) is applied per-PRIM in ge_draw_prim from the live registers,
    // which may be set after begin_list (issue #23).
    glViewport(0, 0, PSP_FB_WIDTH, PSP_FB_HEIGHT);
    g_frame_counter++;
}

void ge_draw_end_list() {
    // Mark the front buffer dirty so the liveness net can publish content
    // even for a title that draws but never calls sceDisplaySetFrameBuf.
    ge_draw_mark_dirty();
}

void ge_draw_prim(
    uint8_t* rdram,
    int prim_type,
    int count
) {
    // Phase 11.4 / Plan 02 Task 2 — alternate-path GE PRIM investigation.
    //
    // Even when count <= 0 we tally [GE_PRIM_DETAIL] before the early-return
    // so the totals match the GE_SUMMARY total_prims figure exactly. The
    // counters here are NEVER rate-limited (they are scalar increments, no
    // I/O per call); the dump happens once in ge_prim_detail_atexit_summary.
    //
    // Tertiary finding from Plan 01 (DIAGNOSTIC §"Raw Log Excerpt"): the
    // GE pipeline processes 243 PRIM commands but only 57 [DRAW_PRIM] log
    // lines reach stderr (all clear=1). The pre-existing [DRAW_PRIM] log
    // rate-limit (first 50 + every 25th + first 5 non-clear) hides the
    // breakdown. This block counts EVERY call by (prim_type, clear) bucket.
    // (Counter definitions moved to file scope -- g_prim_* above -- so the
    // debug socket's I command can snapshot them via ge_draw_get_stats.)
    static bool prim_detail_atexit_installed = false;
    if (!prim_detail_atexit_installed) {
        std::atexit([]() {
            std::fprintf(stderr,
                "[GE_PRIM_DETAIL_SUMMARY] total_observed=%d zero_count=%d\n",
                g_prim_total_observed, g_prim_type_zero_count);
            int real_ns = 0, sprite_nc = 0, clears = 0;
            for (int t = 0; t < 8; ++t) {
                int normal = g_prim_type_count[t];
                int clear = g_prim_type_count_clear[t];
                clears += clear;
                if (t == 6) sprite_nc += normal; else real_ns += normal;
                if (normal == 0 && clear == 0) continue;
                std::fprintf(stderr,
                    "[GE_PRIM_DETAIL] type=%d normal=%d clear=%d total=%d\n",
                    t, normal, clear, normal + clear);
            }
            // Canonical one-line verdict for the verification harness.
            std::fprintf(stderr,
                "[GE_GEOM_VERDICT] real_nonsprite=%d sprite_nonclear=%d "
                "clears=%d => %s\n",
                real_ns, sprite_nc, clears,
                real_ns > 0 ? "GRAPHICS" : "NO-GRAPHICS");
        });
        prim_detail_atexit_installed = true;
    }

    // Self-test for the verification harness (scripts/verify_geometry.sh):
    // PSPRECOMP_GEOM_SELFTEST=1 synthesizes ONE real type-4 (TRIANGLE_STRIP,
    // non-clear, non-sprite) draw through the exact counter + sentinel path, so
    // the GRAPHICS detection can be proven end-to-end without the real asset bug
    // fixed. Proves the detector is not a constant-false oracle. No GL side
    // effects; runs once.
    static bool selftest_done = false;
    if (!selftest_done && std::getenv("PSPRECOMP_GEOM_SELFTEST")) {
        selftest_done = true;
        g_prim_type_count[GE_PRIM_TRIANGLE_STRIP]++;
        std::fprintf(stderr,
            "[GE_GEOM_REAL_DRAW] FIRST real geometry draw: "
            "type=%d count=%d (success-bar (b) MET) [SELFTEST]\n",
            GE_PRIM_TRIANGLE_STRIP, 3);
        std::fflush(stderr);
    }

    g_prim_total_observed++;
    if (count <= 0) {
        g_prim_type_zero_count++;
        return;
    }

    static int prim_call_count = 0;
    prim_call_count++;

    const GeState& state = ge_get_state();
    {
        int t_idx = (prim_type >= 0 && prim_type < 8) ? prim_type : 7;
        if (state.clear_mode) {
            g_prim_type_count_clear[t_idx]++;
        } else {
            g_prim_type_count[t_idx]++;
        }

        // [GE_GEOM_VERDICT] live verification sentinels — the definitive answer
        // to "are graphics being set up?". These print DURING the run and are
        // flushed immediately, so they survive ANY exit path (SIGTERM, SIGKILL,
        // or a hung shutdown) — unlike the atexit [GE_PRIM_DETAIL_SUMMARY],
        // which only fires on a clean return from main(). Pure diagnostic
        // output: changes no game behavior.
        //
        // Success-bar half (b) == ONE real draw with type!=6 (non-sprite) AND
        // clear==0 (non-clear) AND normal>0. The line below appears in the log
        // if and only if that condition is ever met.
        static bool geom_first_real_logged = false;
        if (!state.clear_mode && t_idx != 6 && !geom_first_real_logged) {
            geom_first_real_logged = true;
            std::fprintf(stderr,
                "[GE_GEOM_REAL_DRAW] FIRST real geometry draw: "
                "type=%d count=%d (success-bar (b) MET)\n",
                prim_type, count);
            std::fflush(stderr);
        }

        // Periodic heartbeat: a killed or hung run still leaves a current tally
        // in the log (every 256 PRIMs observed). real_nonsprite>0 == graphics.
        if (g_prim_total_observed % 256 == 0) {
            int real_ns = 0, sprite_nc = 0, clears = 0;
            for (int t = 0; t < 8; ++t) {
                clears += g_prim_type_count_clear[t];
                if (t == 6) sprite_nc += g_prim_type_count[t];
                else real_ns += g_prim_type_count[t];
            }
            std::fprintf(stderr,
                "[GE_GEOM_HEARTBEAT] prims=%d real_nonsprite=%d "
                "sprite_nonclear=%d clears=%d\n",
                g_prim_total_observed, real_ns, sprite_nc, clears);
            std::fflush(stderr);
        }
    }

    // Log a wider window so non-clear PRIMs in the middle of a run are
    // not silently dropped. Also log every 25th PRIM after the first 50
    // and log the first non-clear PRIM unconditionally.
    static int non_clear_logged = 0;
    bool is_non_clear_first = (!state.clear_mode && non_clear_logged < 5);
    if (prim_call_count <= 50
        || (prim_call_count % 25 == 0)
        || is_non_clear_first) {
        std::fprintf(stderr,
            "[DRAW_PRIM] #%d type=%d count=%d clear=%d "
            "vtype=0x%06X vaddr=0x%08X\n",
            prim_call_count, prim_type, count,
            state.clear_mode ? 1 : 0,
            state.vertex_type, state.vertex_addr);
        if (is_non_clear_first) non_clear_logged++;
    }

    // Clear mode: use glClear instead of drawing
    if (state.clear_mode) {
        // Decode first vertex for clear color
        std::vector<DecodedVertex> clear_verts;
        ge_decode_vertices(
            rdram, state, prim_type, count, clear_verts);
        if (!clear_verts.empty()) {
            float r = clear_verts[0].color[0] / 255.0f;
            float g = clear_verts[0].color[1] / 255.0f;
            float b = clear_verts[0].color[2] / 255.0f;
            float a = clear_verts[0].color[3] / 255.0f;
            if (prim_call_count <= 20) {
                std::fprintf(stderr,
                    "[DRAW_CLEAR] color=(%0.2f,%0.2f,%0.2f,%0.2f) "
                    "flags=0x%X verts=%zu\n",
                    r, g, b, a, state.clear_flags,
                    clear_verts.size());
            }
            glClearColor(r, g, b, a);
            if (prim_call_count <= 5) {
                std::fprintf(stderr,
                    "[DRAW_CLEAR] glClearColor(%.3f, %.3f, %.3f, %.3f) "
                    "flags=0x%X\n",
                    r, g, b, a, state.clear_flags);
            }
        }

        GLbitfield clear_bits = 0;
        if (state.clear_flags & 1)
            clear_bits |= GL_COLOR_BUFFER_BIT;
        if (state.clear_flags & 2)
            clear_bits |= GL_STENCIL_BUFFER_BIT;
        if (state.clear_flags & 4)
            clear_bits |= GL_DEPTH_BUFFER_BIT;
        if (clear_bits == 0)
            clear_bits = GL_COLOR_BUFFER_BIT
                         | GL_DEPTH_BUFFER_BIT;

        // Clear depth to the active range's GL "far" value (issue #23). With
        // a reversed range (Patapon ZSCALE<0) GL far is 0.0, not the GL
        // default 1.0; clearing to 1.0 there would put the cleared buffer at
        // the near plane and reject all subsequent depth-tested geometry.
        if (clear_bits & GL_DEPTH_BUFFER_BIT) {
            const GeViewportDepth vp = compute_state_viewport_depth(state);
            glClearDepth(static_cast<GLclampd>(vp.far_z));
        }

        // Temporarily disable depth mask restriction
        glDepthMask(GL_TRUE);
        glClear(clear_bits);
        g_has_drawn_prims = true;  // Clear counts as drawing
        return;
    }

    // Viewport / depth-range (issue #23). Transform-mode draws emit NDC via
    // the real MVP path, so they must map through the PSP viewport
    // scale/offset + depth range to land on screen. Through-mode (2D/sprite)
    // already bakes the full-screen 240/136 mapping into NDC in
    // ge_transform_vertices, so it keeps the full-buffer viewport set by
    // ge_draw_begin_list and an identity depth range.
    bool depth_reversed = false;
    if (!ge_vtype_through(state.vertex_type)) {
        const GeViewportDepth vp = compute_state_viewport_depth(state);
        glViewport(vp.x, vp.y, vp.w, vp.h);
        glDepthRange(vp.near_z, vp.far_z);
        depth_reversed = depth_range_reversed(vp);
    } else {
        glViewport(0, 0, PSP_FB_WIDTH, PSP_FB_HEIGHT);
        glDepthRange(0.0, 1.0);
    }

    // Decode vertices
    std::vector<DecodedVertex> decoded;
    ge_decode_vertices(
        rdram, state, prim_type, count, decoded);
    if (decoded.empty()) return;

    // Transform
    ge_transform_vertices(decoded, state);

    // Handle RECTANGLES: expand 2 verts to 6 (two tris)
    std::vector<PackedVertex> packed;
    if (prim_type == GE_PRIM_RECTANGLES) {
        // Each rectangle is 2 vertices (top-left, bottom-right)
        int rect_count = count / 2;
        packed.reserve(rect_count * 6);
        for (int r = 0; r < rect_count; r++) {
            int i0 = r * 2;
            int i1 = r * 2 + 1;
            if (i1 >= static_cast<int>(decoded.size()))
                break;

            const auto& v0 = decoded[i0];
            const auto& v1 = decoded[i1];

            // Build 4 corners from 2 vertices
            PackedVertex corners[4];
            // v0 (top-left)
            corners[0] = {
                {v0.pos[0], v0.pos[1], v0.pos[2]},
                {v0.uv[0], v0.uv[1]},
                {v0.color[0], v0.color[1],
                 v0.color[2], v0.color[3]}
            };
            // top-right
            corners[1] = {
                {v1.pos[0], v0.pos[1], v0.pos[2]},
                {v1.uv[0], v0.uv[1]},
                {v1.color[0], v1.color[1],
                 v1.color[2], v1.color[3]}
            };
            // bottom-right (v1)
            corners[2] = {
                {v1.pos[0], v1.pos[1], v1.pos[2]},
                {v1.uv[0], v1.uv[1]},
                {v1.color[0], v1.color[1],
                 v1.color[2], v1.color[3]}
            };
            // bottom-left
            corners[3] = {
                {v0.pos[0], v1.pos[1], v1.pos[2]},
                {v0.uv[0], v1.uv[1]},
                {v0.color[0], v0.color[1],
                 v0.color[2], v0.color[3]}
            };

            // Two triangles: 0-1-2 and 0-2-3
            packed.push_back(corners[0]);
            packed.push_back(corners[1]);
            packed.push_back(corners[2]);
            packed.push_back(corners[0]);
            packed.push_back(corners[2]);
            packed.push_back(corners[3]);
        }
    } else {
        // Pack decoded vertices
        packed.resize(decoded.size());
        for (size_t i = 0; i < decoded.size(); i++) {
            const auto& v = decoded[i];
            packed[i] = {
                {v.pos[0], v.pos[1], v.pos[2]},
                {v.uv[0], v.uv[1]},
                {v.color[0], v.color[1],
                 v.color[2], v.color[3]}
            };
        }
    }

    if (packed.empty()) return;

    // Upload to VBO
    glBindVertexArray(g_vao);
    glBindBuffer(GL_ARRAY_BUFFER, g_vbo);
    glBufferData(GL_ARRAY_BUFFER,
                 packed.size() * sizeof(PackedVertex),
                 packed.data(),
                 GL_STREAM_DRAW);

    // Bind shader and set uniforms
    ge_shader_use();
    ge_shader_set_uniforms(state);

    // Bind texture if enabled
    if (state.texture_enable) {
        glActiveTexture(GL_TEXTURE0);
        ge_texture_bind(rdram, g_frame_counter);
    }

    // Set GL state: alpha blend
    if (state.alpha_blend_enable) {
        glEnable(GL_BLEND);
        int src_factor = state.blend_mode & 0xF;
        int dst_factor = (state.blend_mode >> 4) & 0xF;
        int blend_op   = (state.blend_mode >> 8) & 0x7;
        glBlendFunc(
            map_blend_factor_src(src_factor),
            map_blend_factor_dst(dst_factor));
        glBlendEquation(map_blend_equation(blend_op));

        // Handle FIXA/FIXB blend constants
        if (src_factor == GE_SRCBLEND_FIXA) {
            float r = ((state.blend_fix_a) & 0xFF)
                      / 255.0f;
            float g = ((state.blend_fix_a >> 8) & 0xFF)
                      / 255.0f;
            float b = ((state.blend_fix_a >> 16) & 0xFF)
                      / 255.0f;
            glBlendColor(r, g, b, 1.0f);
        }
    } else {
        glDisable(GL_BLEND);
    }

    // Depth test
    if (state.depth_test_enable) {
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(map_depth_func(state.depth_func, depth_reversed));
        glDepthMask(
            state.depth_write_disable
            ? GL_FALSE : GL_TRUE);
    } else {
        glDisable(GL_DEPTH_TEST);
    }

    // Cull face
    if (state.cull_enable) {
        glEnable(GL_CULL_FACE);
        glCullFace(
            state.cull_face == 0
            ? GL_BACK : GL_FRONT);
    } else {
        glDisable(GL_CULL_FACE);
    }

    // Draw
    GLenum gl_prim = map_prim_type(prim_type);
    glDrawArrays(gl_prim, 0,
                 static_cast<GLsizei>(packed.size()));
    g_has_drawn_prims = true;

    glBindVertexArray(0);
}

// Service the pending screenshot/auto-capture from the given front FBO.
static void present_service_screenshot(GLuint fbo) {
    if (!g_screenshot_enabled || g_screenshot_done || !g_has_drawn_prims) {
        return;
    }
    glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo);
    std::vector<uint8_t> pixels(480 * 272 * 4);
    glReadPixels(0, 0, 480, 272, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
    std::vector<uint8_t> flipped(480 * 272 * 4);
    for (int y = 0; y < 272; y++) {
        std::memcpy(flipped.data() + y * 480 * 4,
                    pixels.data() + (271 - y) * 480 * 4, 480 * 4);
    }
    write_tga(g_screenshot_path, flipped.data(), 480, 272);
    g_screenshot_done = true;
}

// Blit the given front FBO to the window and swap. Common to the routine
// (page-flip) present and the liveness net.
static void present_blit(GLuint fbo) {
    SDL_Window* window = psp_get_sdl_window();
    if (!window) return;
    present_service_screenshot(fbo);
    int win_w = 480, win_h = 272;
    SDL_GetWindowSize(window, &win_w, &win_h);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    glBlitFramebuffer(0, 0, 480, 272, 0, 0, win_w, win_h,
                      GL_COLOR_BUFFER_BIT, GL_NEAREST);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    SDL_GL_SwapWindow(window);
    g_present_dirty = false;
    g_last_present = std::chrono::steady_clock::now();
}

void ge_present_frame(
    uint8_t* rdram,
    uint32_t fb_addr,
    uint32_t fb_stride,
    uint32_t fb_format
) {
    (void)rdram;
    (void)fb_stride;
    (void)fb_format;

    // Select the FBO matching the displayed (scanned-out) VRAM address — the
    // front buffer. On key miss for a real (non-zero) address the guest is
    // scanning out a VRAM buffer the GE has not rendered into yet (e.g. the
    // first flip to the back buffer before any list targets it). Create that
    // buffer's own FBO — cleared to black — and present THAT, rather than
    // blitting an unrelated sibling slot (which would publish a different
    // buffer's stale/cleared contents as if they were this frame). This keeps
    // present strictly per-address: a fresh flip shows a clean buffer, and any
    // geometry the GE later renders into this address lands in the same FBO.
    uint32_t key = ge_fb_key(fb_addr);
    int slot = -1;
    for (int i = 0; i < GE_MAX_TARGETS; ++i) {
        if (g_targets[i].fbo != 0 && g_targets[i].key == key) { slot = i; break; }
    }
    if (slot < 0) {
        if (key != 0) {
            // Materialize the displayed buffer's own (cleared) FBO.
            slot = acquire_target(key);
        } else {
            // key==0 (FRAMEBUFPTR/displayed addr not yet meaningful): keep the
            // most-recently-rendered target so present never blanks.
            slot = (g_front_target >= 0) ? g_front_target : g_current_target;
            static int fallback_count = 0;
            if (++fallback_count <= 20) {
                std::fprintf(stderr,
                    "[PRESENT] fallback: no FBO for fb=0x%08X key=0x%06X "
                    "(using slot=%d)\n", fb_addr, key, slot);
            }
        }
    }
    if (slot < 0) return;  // nothing rendered yet

    g_front_target = slot;
    g_targets[slot].last_use = ++g_target_clock;

    static int present_count = 0;
    if (++present_count <= 5) {
        std::fprintf(stderr,
            "[PRESENT] Frame %d (fb=0x%08X key=0x%06X stride=%u fmt=%u)\n",
            present_count, fb_addr, key, fb_stride, fb_format);
    }
    present_blit(g_targets[slot].fbo);
}

// Liveness net (§2.1): if lists were processed but no real page-flip arrived
// within the stale budget, present the current front buffer once. Preserves
// "content becomes visible even without a flip" without making every list a
// frame boundary. Main (GL) thread only — same thread as the routine present.
void ge_draw_present_if_stale(uint8_t* rdram) {
    (void)rdram;
    if (!g_present_dirty) return;
    auto age = std::chrono::steady_clock::now() - g_last_present;
    if (age < std::chrono::milliseconds(g_present_stale_ms)) return;
    GLuint fbo = front_fbo_id();
    if (fbo == 0) { g_present_dirty = false; return; }
    present_blit(fbo);
}

// ---- Debug socket accessors (issue #35) ----

void ge_draw_get_stats(
    uint32_t* frames,
    int* prims_total,
    int* real_nonsprite,
    int* sprite_nonclear,
    int* clears
) {
    // Racy reads of render-thread counters -- diagnostics only.
    if (frames) *frames = g_frame_counter;
    if (prims_total) *prims_total = g_prim_total_observed;
    int real_ns = 0, sprite_nc = 0, clr = 0;
    for (int t = 0; t < 8; ++t) {
        clr += g_prim_type_count_clear[t];
        if (t == 6) sprite_nc += g_prim_type_count[t];
        else real_ns += g_prim_type_count[t];
    }
    if (real_nonsprite) *real_nonsprite = real_ns;
    if (sprite_nonclear) *sprite_nonclear = sprite_nc;
    if (clears) *clears = clr;
}

bool ge_draw_capture_screenshot(const char* path, int timeout_ms) {
    if (!g_draw_ready.load(std::memory_order_acquire)) {
        std::fprintf(stderr,
            "[DRAW] Screenshot request before GL init: %s\n", path);
        return false;
    }
    std::unique_lock<std::mutex> lock(g_ss_req_mutex);
    if (g_ss_req_state == SS_PENDING) {
        // Another client's capture is in flight -- refuse instead of
        // overwriting its path.
        return false;
    }
    std::strncpy(g_ss_req_path, path, sizeof(g_ss_req_path) - 1);
    g_ss_req_path[sizeof(g_ss_req_path) - 1] = '\0';
    g_ss_req_state = SS_PENDING;
    bool finished = g_ss_req_cv.wait_for(
        lock, std::chrono::milliseconds(timeout_ms),
        [] { return g_ss_req_state == SS_DONE
                    || g_ss_req_state == SS_FAILED; });
    bool ok = finished && g_ss_req_state == SS_DONE;
    g_ss_req_state = SS_IDLE;   // reset (also cancels on timeout)
    return ok;
}

void ge_draw_service_screenshot_request() {
    char path[sizeof(g_ss_req_path)];
    {
        std::lock_guard<std::mutex> lock(g_ss_req_mutex);
        if (g_ss_req_state != SS_PENDING) return;
        std::memcpy(path, g_ss_req_path, sizeof(path));
    }
    // GL work outside the request lock (requester only waits on the cv).
    bool ok = g_draw_ready.load(std::memory_order_acquire)
              && capture_fbo_to_tga(path);
    {
        std::lock_guard<std::mutex> lock(g_ss_req_mutex);
        // Only publish if the requester hasn't timed out and reset state.
        if (g_ss_req_state == SS_PENDING) {
            g_ss_req_state = ok ? SS_DONE : SS_FAILED;
        }
    }
    g_ss_req_cv.notify_all();
}
