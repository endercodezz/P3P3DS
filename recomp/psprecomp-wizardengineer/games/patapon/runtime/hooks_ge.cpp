// games/patapon/runtime/hooks_ge.cpp — Patapon GE vertex fallback
// (issues #46 / #47 Phase 5, original workaround from issue #27).
//
// Moved verbatim from runtime/src/psp_ge_vertex.cpp's NDC-direct branch:
// the ortho mapping used when the guest uploads degenerate matrices (view
// all-zero / proj NaN-or-collapsed — open issue, FPU/VFPU dataflow
// family). Compiled only under -DPSPRECOMP_GAME=patapon.

#include "psp_ge_vertex.h"
#include "patapon_hooks.h"

/// NDC-direct ortho mapping. This is the ortho mapping Patapon intends
/// (viewport scale 240/-136, center 2048, offset 1808/1912 makes
/// NDC-direct equivalent; gum view buffer ~0x090965B0 never written —
/// next divergence layer past the #27 VFPU register-file fix).
/// REMOVAL CRITERION: delete when the guest-side matrix uploads are
/// root-fixed (#27 family) — the core's real transform path then engages
/// automatically and this fallback never fires.
static void patapon_degenerate_fallback(const float wpos[3], float out[3]) {
    out[0] = wpos[0] / 240.0f - 1.0f;
    out[1] = wpos[1] / 136.0f + 1.0f;
    out[2] = 0.0f;
}

void patapon_install_ge_vertex_fallback() {
    ge_vertex_set_degenerate_fallback(patapon_degenerate_fallback);
}
