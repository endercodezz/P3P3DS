#pragma once
// Pieces of a renderer backend that turns GE model primitives into one
// indexed triangle list (the New 3DS batches; see platform/3ds/gpu_renderer.cpp).
#include "p3p3ds/ge/renderer.hpp"

#include <cstdint>

namespace p3p3ds::ge {

// Triangle-list indices of one primitive of `count` vertices: vtx(i) gives
// the list index of the primitive's vertex i, out(a, b, c) takes a triangle.
// Strips and fans are unrolled with the software renderer's winding (odd
// strip triangles swap their first two vertices).
template <class Vtx, class Out>
void append_triangles(Prim prim, std::uint32_t count, Vtx &&vtx, Out &&out) {
    switch (prim) {
    case Prim::Triangles:
        for (std::uint32_t i = 0; i + 2 < count; i += 3) out(vtx(i), vtx(i + 1), vtx(i + 2));
        break;
    case Prim::TriangleStrip:
        for (std::uint32_t i = 0; i + 2 < count; ++i)
            (i & 1u) ? out(vtx(i + 1), vtx(i), vtx(i + 2)) : out(vtx(i), vtx(i + 1), vtx(i + 2));
        break;
    case Prim::TriangleFan:
        for (std::uint32_t i = 1; i + 1 < count; ++i) out(vtx(0), vtx(i), vtx(i + 1));
        break;
    default:
        break;
    }
}

// A run of model draws whose vertices follow each other in guest memory:
// non-indexed primitives advance the GE vertex address by their vertex count,
// so a scene of many small draws (the first battle: about 2,000 per frame,
// median 4 vertices) reads one contiguous block. The renderer reserves room
// for `capacity` vertices at list index `first`, appends each draw's indices
// at once, and converts the whole block when the run ends.
struct ModelRun {
    bool active{};
    std::uint32_t address{};     // guest address of the first vertex
    std::uint32_t vertex_size{}; // bytes per guest vertex (VertexLayout::size)
    std::uint32_t vertex_type{}; // GE register 0x12
    std::uint32_t material{};    // material colour for vertices without one (0 when they have one)
    std::uint32_t count{};       // vertices in the run so far
    std::uint32_t capacity{};    // vertices reserved
    std::uint32_t first{};       // list index of the first vertex

    [[nodiscard]] std::uint32_t end_address() const noexcept { return address + count * vertex_size; }
    // Whether a draw of `n` vertices at `vertex_address` continues this run.
    [[nodiscard]] bool continues(std::uint32_t vertex_address, std::uint32_t type, std::uint32_t mat, std::uint32_t n) const noexcept {
        return active && vertex_address == end_address() && type == vertex_type && mat == material && count + n <= capacity;
    }
    // Appends a draw's vertices; returns the list index of its first vertex.
    std::uint32_t append(std::uint32_t n) noexcept {
        const std::uint32_t base = first + count;
        count += n;
        return base;
    }
};

} // namespace p3p3ds::ge
