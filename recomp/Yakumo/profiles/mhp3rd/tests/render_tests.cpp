// Renderer helpers that need no GPU: the index lists that let transformed
// draws skip expanding their vertices must name, position by position, the
// vertex the expansion would have written there.

#include "gpu/triangle_indices.hpp"

#include <cstdint>
#include <iostream>
#include <random>
#include <vector>

namespace {

using mhp3rd::gpu::PrimitiveType;

int failures{};

void expect(bool condition, const char *what) {
    if (condition) return;
    ++failures;
    std::cout << "FAIL: " << what << "\n";
}

// The renderer's expansion as it was written before the index lists, kept
// here as the reference: which decoded vertex lands at each output position.
std::vector<std::uint16_t> reference(PrimitiveType primitive, std::size_t count,
                                     const std::vector<std::uint16_t> &indices, std::size_t vertex_count) {
    std::vector<std::uint16_t> out;
    const auto vertex_at = [&](std::size_t index) -> std::uint16_t {
        if (!indices.empty()) {
            const std::size_t mapped = indices[index];
            return static_cast<std::uint16_t>(std::min(mapped, vertex_count - 1u));
        }
        return static_cast<std::uint16_t>(std::min(index, vertex_count - 1u));
    };
    switch (primitive) {
    case PrimitiveType::Triangles:
        for (std::size_t i = 0; i + 2u < count; i += 3u) {
            out.push_back(vertex_at(i));
            out.push_back(vertex_at(i + 1u));
            out.push_back(vertex_at(i + 2u));
        }
        break;
    case PrimitiveType::TriangleStrip:
        for (std::size_t i = 0; i + 2u < count; ++i) {
            const bool odd = (i & 1u) != 0u;
            out.push_back(vertex_at(i));
            out.push_back(vertex_at(odd ? i + 2u : i + 1u));
            out.push_back(vertex_at(odd ? i + 1u : i + 2u));
        }
        break;
    case PrimitiveType::TriangleFan:
        for (std::size_t i = 1u; i + 1u < count; ++i) {
            out.push_back(vertex_at(0));
            out.push_back(vertex_at(i));
            out.push_back(vertex_at(i + 1u));
        }
        break;
    default: break;
    }
    return out;
}

void test_matches_expansion() {
    std::mt19937 random(92u);
    std::vector<std::uint16_t> out;
    for (const PrimitiveType primitive :
         {PrimitiveType::Triangles, PrimitiveType::TriangleStrip, PrimitiveType::TriangleFan}) {
        for (std::size_t count = 0; count < 40u; ++count) {
            for (int indexed = 0; indexed < 2; ++indexed) {
                for (int trial = 0; trial < 8; ++trial) {
                    std::size_t vertex_count = count;
                    std::vector<std::uint16_t> indices;
                    if (indexed != 0) {
                        // Indices past the decoded vertices are clamped, as
                        // the expansion clamps them.
                        vertex_count = 1u + random() % 24u;
                        for (std::size_t i = 0; i < count; ++i)
                            indices.push_back(static_cast<std::uint16_t>(random() % (vertex_count + 4u)));
                    }
                    if (vertex_count == 0u) continue;
                    expect(mhp3rd::gpu::triangle_indices(primitive, count, indices, vertex_count, out),
                           "triangle primitives are supported");
                    expect(out == reference(primitive, count, indices, vertex_count),
                           "index list matches the expansion");
                    expect(out.size() % 3u == 0u, "whole triangles");
                }
            }
        }
    }
}

void test_large_mesh() {
    // A strip over the whole 16-bit index range.
    const std::size_t count = 65536u;
    std::vector<std::uint16_t> out;
    expect(mhp3rd::gpu::triangle_indices(PrimitiveType::TriangleStrip, count, {}, count, out), "strip supported");
    expect(out == reference(PrimitiveType::TriangleStrip, count, {}, count), "large strip matches the expansion");
    expect(out.size() == (count - 2u) * 3u, "large strip triangle count");
}

void test_other_primitives() {
    std::vector<std::uint16_t> out{1u, 2u, 3u};
    for (const PrimitiveType primitive :
         {PrimitiveType::Points, PrimitiveType::Lines, PrimitiveType::LineStrip, PrimitiveType::Sprites}) {
        expect(!mhp3rd::gpu::triangle_indices(primitive, 4u, {}, 4u, out), "other primitives are refused");
        expect(out.empty(), "refused primitives leave no indices");
    }
}

} // namespace

int main() {
    test_matches_expansion();
    test_large_mesh();
    test_other_primitives();
    std::cout << (failures ? "FAIL" : "PASS") << ": render helpers (" << failures << " failures)\n";
    return failures ? 1 : 0;
}
