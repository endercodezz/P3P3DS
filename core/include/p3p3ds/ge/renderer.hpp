#pragma once
// GE rendering interface. core/src/hle/ge.cpp executes display lists and calls
// a GeRenderer with the decoded GE register file; backends rasterize.
// SoftwareRenderer is the target-agnostic reference backend: it draws into
// guest EDRAM exactly where the PSP GE would, so CPU readback, render-to-
// texture and display presentation all observe the same pixels.
// Register meanings: PSPSDK psp/pspsdk/src/gu/pspge.h / pspgu.h command list;
// PPSSPP GPU/ge_constants.h was a behavioural cross-check only.
#include <array>
#include <cstdint>
#include <vector>

namespace psprecomp { class GuestMemory; }

namespace p3p3ds::ge {

// GE register file + matrices, as written by display-list commands.
struct GeRegisters {
    std::array<std::uint32_t, 256> reg{};
    std::array<float, 12 * 8> bone{};
    std::array<float, 12> world{}, view{}, tgen{};
    std::array<float, 16> proj{};
    std::array<std::uint32_t, 256> clut{}; // 1 KiB CLUT cache (as 32-bit words)
    std::uint32_t clut_words{};
};

enum class Prim : std::uint32_t { Points = 0, Lines = 1, LineStrip = 2, Triangles = 3, TriangleStrip = 4, TriangleFan = 5, Sprites = 6 };

struct DrawStats {
    std::uint64_t prims{}, vertices{}, triangles{}, sprites{}, pixels{}, clears{}, transfers{};
};

class GeRenderer {
public:
    virtual ~GeRenderer() = default;
    // Draws `count` vertices from `vertex_address` (and `index_address` when
    // the vertex type is indexed). Returns the number of bytes of vertex data
    // consumed so the processor can advance VADDR/IADDR.
    virtual void draw(psprecomp::GuestMemory &memory, const GeRegisters &regs, Prim prim, std::uint32_t count,
                      std::uint32_t vertex_address, std::uint32_t index_address) = 0;
    // Block transfer (TRANSFERSTART) of a rectangle between guest addresses.
    virtual void transfer(psprecomp::GuestMemory &memory, const GeRegisters &regs) = 0;
    [[nodiscard]] virtual const DrawStats &stats() const = 0;
};

// Byte size of one vertex for VTYPE `vtype` and per-component layout.
struct VertexLayout {
    std::uint32_t size{}, weight_offset{}, uv_offset{}, color_offset{}, normal_offset{}, pos_offset{};
    std::uint32_t weights{}, weight_format{}, uv_format{}, color_format{}, normal_format{}, pos_format{}, index_format{};
    std::uint32_t morphs{};
    bool through{};
};
[[nodiscard]] VertexLayout vertex_layout(std::uint32_t vtype);

class SoftwareRenderer final : public GeRenderer {
public:
    void draw(psprecomp::GuestMemory &memory, const GeRegisters &regs, Prim prim, std::uint32_t count,
              std::uint32_t vertex_address, std::uint32_t index_address) override;
    void transfer(psprecomp::GuestMemory &memory, const GeRegisters &regs) override;
    [[nodiscard]] const DrawStats &stats() const override { return stats_; }

private:
    DrawStats stats_;
};

// 24-bit GE float argument -> float.
[[nodiscard]] inline float ge_float(std::uint32_t arg) noexcept {
    const std::uint32_t bits = arg << 8;
    float f;
    static_assert(sizeof(f) == sizeof(bits));
    __builtin_memcpy(&f, &bits, sizeof(f));
    return f;
}

} // namespace p3p3ds::ge
