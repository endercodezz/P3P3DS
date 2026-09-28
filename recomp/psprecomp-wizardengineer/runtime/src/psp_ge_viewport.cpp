#include "psp_ge_draw.h"

#include <cmath>
#include <algorithm>

// PSP viewport scale/offset + depth-range registers -> GL viewport + depth
// range (issue #23). Pure math, no GL dependency, so it is unit-testable in
// isolation (tests/test_ge_viewport.cpp).
//
// Mirrors PPSSPP's ConvertViewportAndScissor
// (GPU/Common/GPUStateUtils.cpp:656-798) for the 1:1, non-supersampled,
// non-accurate-depth FBO case the runtime renders into. PPSSPP documents the
// PSP viewport transform as:
//   Xscreen = -offsetX + vpXCenter + vpXScale * Xview
//   Yscreen = -offsetY + vpYCenter + vpYScale * Yview
//   Zscreen =            vpZCenter + vpZScale * Zview
// so the equivalent glViewport(left, top, w, h) is
//   left = vpXCenter - offsetX - |vpXScale|,  w = |2 * vpXScale|
//   top  = vpYCenter - offsetY - |vpYScale|,  h = |2 * vpYScale|
// offsetX/Y are 1/16-subpixel u16 register words (PPSSPP getOffsetX:
// (offsetx & 0xFFFF) / 16). PSP screen origin is top-left; the runtime FBO
// uses GL's bottom-left origin without a vertex Y-flip, so `top` is flipped:
//   gl_y = fb_height - top - h.
// Depth maps the PSP Z window [vpZCenter - vpZScale, vpZCenter + vpZScale]
// (encoded in [0, 65535]) to glDepthRange's [0, 1] -- the same as PPSSPP's
// non-accurate-depth branch (GPUStateUtils.cpp:790-794).
GeViewportDepth ge_compute_viewport_depth(
    float vp_x_scale, float vp_y_scale,
    float vp_x_center, float vp_y_center,
    float vp_z_scale, float vp_z_center,
    uint32_t off_x_raw, uint32_t off_y_raw,
    int fb_height) {
    const float off_x = static_cast<float>(off_x_raw & 0xFFFFu) / 16.0f;
    const float off_y = static_cast<float>(off_y_raw & 0xFFFFu) / 16.0f;

    const float ax = std::fabs(vp_x_scale);
    const float ay = std::fabs(vp_y_scale);
    const float left = vp_x_center - off_x - ax;
    const float top = vp_y_center - off_y - ay;
    const float w = ax * 2.0f;
    const float h = ay * 2.0f;
    const float gl_y = static_cast<float>(fb_height) - top - h;

    float near_z = (vp_z_center - vp_z_scale) / 65535.0f;
    float far_z = (vp_z_center + vp_z_scale) / 65535.0f;
    near_z = std::min(std::max(near_z, 0.0f), 1.0f);
    far_z = std::min(std::max(far_z, 0.0f), 1.0f);

    GeViewportDepth out;
    out.x = static_cast<int>(std::lround(left));
    out.y = static_cast<int>(std::lround(gl_y));
    out.w = static_cast<int>(std::lround(w));
    out.h = static_cast<int>(std::lround(h));
    out.near_z = near_z;
    out.far_z = far_z;
    return out;
}
