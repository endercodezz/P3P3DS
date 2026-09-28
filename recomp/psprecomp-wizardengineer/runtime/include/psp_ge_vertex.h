#pragma once
#include "psp_ge.h"
#include "psp_ge_constants.h"
#include <cstdint>
#include <vector>

/// Decoded vertex in a uniform format for GL upload.
/// All PSP vertex formats are converted to this before rendering.
struct DecodedVertex {
    float pos[3];       // Position (x, y, z)
    float uv[2];        // Texture coordinates (u, v)
    uint8_t color[4];   // RGBA color
    float normal[3];    // Normal vector
    bool has_uv;
    bool has_color;
    bool has_normal;
};

/// Compute the byte stride of one PSP vertex based on VTYPE bitfield.
int ge_vertex_stride(uint32_t vtype);

/// Decode `count` vertices from rdram at state.vertex_addr using
/// the vertex format described by state.vertex_type. If index bits
/// are set in VTYPE, reads the index buffer from state.index_addr.
void ge_decode_vertices(
    uint8_t* rdram,
    const GeState& state,
    int prim_type,
    int count,
    std::vector<DecodedVertex>& out
);

/// Apply world*view*proj matrix to positions and tex scale/offset
/// to UVs. Through-mode vertices are mapped to NDC directly.
void ge_transform_vertices(
    std::vector<DecodedVertex>& verts,
    const GeState& state
);

// ---- Game-module degenerate-matrix fallback (issue #47 Phase 5 seam) ----
// When the guest uploads broken matrices (view all-zero; proj NaN/Inf or
// collapsed diagonal — open issue, FPU/VFPU dataflow family), the real
// transform path cannot work. The mapping that produces a usable frame
// anyway is GAME-TUNED (it depends on the title's intended projection),
// so it installs from the game module. Generic default: world-space
// passthrough (positions used as NDC unchanged) plus the one-time warns.
// Signature: world-space position in, NDC out.
using GeDegenerateFallbackFn = void (*)(const float wpos[3], float out[3]);
void ge_vertex_set_degenerate_fallback(GeDegenerateFallbackFn fn);
