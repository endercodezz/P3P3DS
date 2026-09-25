#pragma once

#include "ge_state.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace mhp3rd::gpu {

// The vertex of a draw at position `index` of its primitive: through the
// index list when there is one, clamped to the vertices decoded.
inline std::uint16_t primitive_vertex(const std::vector<std::uint16_t> &indices, std::size_t vertex_count,
                                      std::size_t index) {
    const std::size_t last = vertex_count - 1u;
    const std::size_t mapped = indices.empty() ? index : indices[index];
    return static_cast<std::uint16_t>(std::min(mapped, last));
}

// Fills `out` with the triangle list a triangle, strip or fan primitive
// becomes, as indices into its decoded vertices. The order is the one the
// renderer expands vertices in: every triangle in the same place and with the
// same first (provoking) vertex, so an indexed draw of the decoded vertices
// rasterises exactly what a draw of the expanded ones does.
//
// `count` is the number of vertices the primitive consumes (the index count
// when it is indexed), `vertex_count` the number decoded, at most 65536.
// Returns false, leaving `out` empty, for any other primitive.
inline bool triangle_indices(PrimitiveType primitive, std::size_t count, const std::vector<std::uint16_t> &indices,
                             std::size_t vertex_count, std::vector<std::uint16_t> &out) {
    out.clear();
    if (vertex_count == 0u) return primitive == PrimitiveType::Triangles ||
                                   primitive == PrimitiveType::TriangleStrip ||
                                   primitive == PrimitiveType::TriangleFan;
    const auto at = [&](std::size_t index) { return primitive_vertex(indices, vertex_count, index); };
    switch (primitive) {
    case PrimitiveType::Triangles:
        out.reserve(count / 3u * 3u);
        for (std::size_t i = 0; i + 2u < count; i += 3u) {
            out.push_back(at(i));
            out.push_back(at(i + 1u));
            out.push_back(at(i + 2u));
        }
        return true;
    case PrimitiveType::TriangleStrip:
        if (count >= 3u) out.reserve((count - 2u) * 3u);
        for (std::size_t i = 0; i + 2u < count; ++i) {
            const bool odd = (i & 1u) != 0u;
            out.push_back(at(i));
            out.push_back(at(odd ? i + 2u : i + 1u));
            out.push_back(at(odd ? i + 1u : i + 2u));
        }
        return true;
    case PrimitiveType::TriangleFan:
        if (count >= 3u) out.reserve((count - 2u) * 3u);
        for (std::size_t i = 1u; i + 1u < count; ++i) {
            out.push_back(at(0));
            out.push_back(at(i));
            out.push_back(at(i + 1u));
        }
        return true;
    default: return false;
    }
}

} // namespace mhp3rd::gpu
