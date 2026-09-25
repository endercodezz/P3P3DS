#include "ge_state.hpp"

#include "perf/frame_stats.hpp"

#include <algorithm>
#include <array>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <map>
#include <vector>

namespace mhp3rd::gpu {
namespace {

// Display list opcodes (command = word >> 24).
enum Command : std::uint32_t {
    kNop = 0x00,
    kVertexAddress = 0x01,
    kIndexAddress = 0x02,
    kPrimitive = 0x04,
    kBezier = 0x05,
    kSpline = 0x06,
    kBoundingBox = 0x07,
    kJump = 0x08,
    kConditionalJump = 0x09,
    kCall = 0x0A,
    kReturn = 0x0B,
    kEnd = 0x0C,
    kSignal = 0x0E,
    kFinish = 0x0F,
    kBase = 0x10,
    kVertexType = 0x12,
    kOffsetAddress = 0x13,
    kOrigin = 0x14,
    kLightingEnable = 0x17,
    kLightEnable0 = 0x18,  // to 0x1B, one per light
    kCullFaceEnable = 0x1D,
    kTextureMapEnable = 0x1E,
    kFogEnable = 0x1F,
    kAlphaBlendEnable = 0x21,
    kAlphaTestEnable = 0x22,
    kDepthTestEnable = 0x23,
    kBoneMatrixNumber = 0x2A,
    kBoneMatrixData = 0x2B,
    kWorldMatrixNumber = 0x3A,
    kWorldMatrixData = 0x3B,
    kViewMatrixNumber = 0x3C,
    kViewMatrixData = 0x3D,
    kProjMatrixNumber = 0x3E,
    kProjMatrixData = 0x3F,
    kTexGenMatrixNumber = 0x40,
    kTexGenMatrixData = 0x41,
    kViewportXScale = 0x42,
    kViewportYScale = 0x43,
    kViewportZScale = 0x44,
    kViewportXCenter = 0x45,
    kViewportYCenter = 0x46,
    kViewportZCenter = 0x47,
    kTexScaleU = 0x48,
    kTexScaleV = 0x49,
    kTexOffsetU = 0x4A,
    kTexOffsetV = 0x4B,
    kOffsetX = 0x4C,
    kOffsetY = 0x4D,
    // Read off the game rather than recalled: 0x53 carries the material update
    // mask (3 and 7), 0x55 and 0x56 carry colours, 0x58 carries an alpha (0 and
    // 0xFF) and 0x5B a float. That fixes the run as update, emissive, ambient,
    // diffuse, specular, alpha — and an unlit draw takes ambient and alpha.
    kReverseNormal = 0x51,
    kMaterialUpdate = 0x53,
    kMaterialEmissive = 0x54,
    kMaterialAmbient = 0x55,
    kMaterialDiffuse = 0x56,
    kMaterialSpecular = 0x57,
    kMaterialAlpha = 0x58,
    kMaterialSpecularPower = 0x5B,
    // The light block, traced the same way: 0x5C is a colour and 0x5D an
    // alpha (0xFF); 0x5F..0x62 hold one type word per light; 0x63..0x6E three
    // floats per light, the positions, which the game fills with unit axes such
    // as (0, 0, 1), (1, 1, 1) and (-1, -1, -1) for its three directional
    // lights; then directions, attenuations, spot exponents and cutoffs; and
    // 0x8F..0x9A three colours per light — ambient 0x4C4C4C, a diffuse that
    // the game animates, and a specular of 0 — ending right before CULL.
    kAmbientColor = 0x5C,
    kAmbientAlpha = 0x5D,
    kLightMode = 0x5E,
    kLightType0 = 0x5F,
    kLightPosition0 = 0x63,
    kLightDirection0 = 0x6F,
    kLightAttenuation0 = 0x7B,
    kSpotExponent0 = 0x87,
    kSpotCutoff0 = 0x8B,
    kLightColor0 = 0x8F,
    kLightColorEnd = 0x9A,
    kCull = 0x9B,
    kFrameBufferPointer = 0x9C,
    kFrameBufferWidth = 0x9D,
    kDepthBufferPointer = 0x9E,
    kDepthBufferWidth = 0x9F,
    kTextureAddress0 = 0xA0,
    kTextureBufferWidth0 = 0xA8,
    kClutAddress = 0xB0,
    kClutAddressUpper = 0xB1,
    kTextureSize0 = 0xB8,
    kTextureMode = 0xC2,
    kTextureFormat = 0xC3,
    kLoadClut = 0xC4,
    kClutFormat = 0xC5,
    kTextureFilter = 0xC6,
    kTextureWrap = 0xC7,
    kTextureFunction = 0xC9,
    kTextureFlush = 0xCB,
    // Fog: 0xCD and 0xCE are floats (65000 and 1.0 in the menus), 0xCF a colour.
    kFogEnd = 0xCD,
    kFogScale = 0xCE,
    kFogColor = 0xCF,
    kFrameBufferPixelFormat = 0xD2,
    kClearMode = 0xD3,
    kScissor1 = 0xD4,
    kScissor2 = 0xD5,
    kMinZ = 0xD6,
    kMaxZ = 0xD7,
    kAlphaTest = 0xDB,
    kDepthTest = 0xDE,
    kBlendMode = 0xDF,
    kBlendFixedA = 0xE0,
    kBlendFixedB = 0xE1,
    kDepthWriteDisable = 0xE7,
    // Block transfer registers, read off a trace of the quest reward screen,
    // which copies the 16-bit framebuffer at 0x04000000 (512 pixels a row) to
    // 0x093E7EF0 in main memory and then textures from there as 512x512 5650
    // with a row length of 512:
    //   0xB2 0x000000  0xB3 0x040100  source 0x04000000, row length 256
    //   0xB4 0x3E7EF0  0xB5 0x090100  destination 0x093E7EF0, row length 256
    //   0xEB 0, 0xEC 0                source and destination position (only 0
    //                                 seen so far; taken as packed like the size)
    //   0xEE 0x043CFF                 size: 255 | 271 << 10, so 256 x 272
    //   0xEA 1                        start, 4 bytes a pixel
    // As with the texture pointers, the width register carries the high byte
    // of the address. 256 four-byte pixels are the 512 two-byte pixels of a
    // framebuffer row, so the kick's bit 0 selects 4 bytes a pixel over 2.
    kTransferSourceAddress = 0xB2,
    kTransferSourceWidth = 0xB3,
    kTransferDestinationAddress = 0xB4,
    kTransferDestinationWidth = 0xB5,
    kTransferStart = 0xEA,
    kTransferSourcePosition = 0xEB,
    kTransferDestinationPosition = 0xEC,
    kTransferSize = 0xEE,
};

// GE pointers carry their high byte in the companion width register; when that
// byte is zero the address is an offset inside VRAM.
std::uint32_t resolve_ge_address(std::uint32_t address) {
    return (address & 0xFF000000u) == 0u ? (address | 0x04000000u) : address;
}

float decode_float24(std::uint32_t data) {
    const std::uint32_t bits = data << 8u;
    float value{};
    std::memcpy(&value, &bits, sizeof(value));
    return value;
}

std::uint32_t expand_color(std::uint32_t value, std::uint32_t format) {
    switch (format) {
    case 4u: {  // 5650
        const std::uint32_t r = (value & 0x1Fu) * 255u / 31u;
        const std::uint32_t g = ((value >> 5u) & 0x3Fu) * 255u / 63u;
        const std::uint32_t b = ((value >> 11u) & 0x1Fu) * 255u / 31u;
        return 0xFF000000u | (b << 16u) | (g << 8u) | r;
    }
    case 5u: {  // 5551
        const std::uint32_t r = (value & 0x1Fu) * 255u / 31u;
        const std::uint32_t g = ((value >> 5u) & 0x1Fu) * 255u / 31u;
        const std::uint32_t b = ((value >> 10u) & 0x1Fu) * 255u / 31u;
        const std::uint32_t a = ((value >> 15u) & 1u) * 255u;
        return (a << 24u) | (b << 16u) | (g << 8u) | r;
    }
    case 6u: {  // 4444
        const std::uint32_t r = (value & 0xFu) * 17u;
        const std::uint32_t g = ((value >> 4u) & 0xFu) * 17u;
        const std::uint32_t b = ((value >> 8u) & 0xFu) * 17u;
        const std::uint32_t a = ((value >> 12u) & 0xFu) * 17u;
        return (a << 24u) | (b << 16u) | (g << 8u) | r;
    }
    default:
        return value;  // 8888
    }
}

std::uint32_t align_up(std::uint32_t value, std::uint32_t alignment) {
    return (value + alignment - 1u) & ~(alignment - 1u);
}

void identity(std::array<float, 16> &matrix) {
    matrix.fill(0.0f);
    matrix[0] = matrix[5] = matrix[10] = matrix[15] = 1.0f;
}

} // namespace

namespace {

// Where each field of a vertex lies, from decode_vertices(); kNoField when the
// vertex type has none.
constexpr std::uint32_t kNoField = 0xFFFFFFFFu;
struct VertexLayout {
    std::uint32_t texcoord_type{};
    std::uint32_t color_type{};
    std::uint32_t weight_count{};
    std::uint32_t weight_offset{kNoField};
    std::uint32_t texcoord_offset{kNoField};
    std::uint32_t color_offset{kNoField};
    std::uint32_t normal_offset{kNoField};
    std::uint32_t position_offset{kNoField};
    std::uint32_t stride{};
    bool through{};
    bool skinned{};
};

template <typename T>
[[gnu::always_inline]] inline T read_raw(const std::uint8_t *at) {
    T value{};
    std::memcpy(&value, at, sizeof(value));
    return value;
}

// A component of `Type` (1: 8-bit, 2: 16-bit, 3: float), signed or unsigned,
// as decode_vertices() reads it.
template <std::uint32_t Type, bool Signed>
[[gnu::always_inline]] inline float read_field(const std::uint8_t *at) {
    if constexpr (Type == 1u) {
        return Signed ? static_cast<float>(static_cast<std::int8_t>(at[0])) : static_cast<float>(at[0]);
    } else if constexpr (Type == 2u) {
        const std::uint16_t value = read_raw<std::uint16_t>(at);
        return Signed ? static_cast<float>(static_cast<std::int16_t>(value)) : static_cast<float>(value);
    } else if constexpr (Type == 3u) {
        return read_raw<float>(at);
    } else {
        return 0.0f;
    }
}

constexpr std::uint32_t kFieldSize[4] = {0u, 1u, 2u, 4u};

// decode_vertices() for a contiguous run of one morph target, with the
// weight, normal and position formats fixed at compile time. Every value is
// computed by the same expressions in the same order as the general loop, so
// the vertices are the same bit for bit (MHP3RD_CHECK_DECODE compares them).
template <std::uint32_t WeightType, std::uint32_t NormalType, std::uint32_t PositionType>
void decode_run(const std::uint8_t *data, std::uint32_t count, const VertexLayout &layout, Vertex *out,
                const float *bone_matrices) {
    constexpr std::uint32_t normal_size = kFieldSize[NormalType];
    constexpr std::uint32_t position_size = kFieldSize[PositionType];
    constexpr std::uint32_t weight_size = kFieldSize[WeightType];
    const std::uint32_t texcoord_type = layout.texcoord_type;
    const std::uint32_t texcoord_size = kFieldSize[texcoord_type];
    const float texcoord_scale = texcoord_type == 1u ? 1.0f / 128.0f
                                                     : (texcoord_type == 2u && !layout.through ? 1.0f / 32768.0f : 1.0f);
    constexpr float normal_scale = NormalType == 1u ? 1.0f / 128.0f : (NormalType == 2u ? 1.0f / 32768.0f : 1.0f);
    const float position_scale = layout.through ? 1.0f
                                                : (PositionType == 1u ? 1.0f / 128.0f
                                                                      : (PositionType == 2u ? 1.0f / 32768.0f : 1.0f));
    constexpr float weight_scale = WeightType == 1u ? 1.0f / 128.0f : (WeightType == 2u ? 1.0f / 32768.0f : 1.0f);
    const std::uint32_t color_type = layout.color_type;
    const bool skinned = WeightType != 0u && layout.skinned;
    const std::uint32_t bones = std::min(layout.weight_count, 8u);
    for (std::uint32_t i = 0; i < count; ++i) {
        const std::uint8_t *base = data + static_cast<std::size_t>(i) * layout.stride;
        Vertex vertex{};
        if (layout.texcoord_offset != kNoField) {
            const std::uint8_t *at = base + layout.texcoord_offset;
            switch (texcoord_type) {
            case 1u:
                vertex.texcoord[0] = read_field<1u, false>(at) * texcoord_scale;
                vertex.texcoord[1] = read_field<1u, false>(at + texcoord_size) * texcoord_scale;
                break;
            case 2u:
                vertex.texcoord[0] = read_field<2u, false>(at) * texcoord_scale;
                vertex.texcoord[1] = read_field<2u, false>(at + texcoord_size) * texcoord_scale;
                break;
            default:
                vertex.texcoord[0] = read_field<3u, false>(at) * texcoord_scale;
                vertex.texcoord[1] = read_field<3u, false>(at + texcoord_size) * texcoord_scale;
                break;
            }
        }
        if (layout.color_offset != kNoField) {
            const std::uint8_t *at = base + layout.color_offset;
            const std::uint32_t raw = color_type == 7u ? read_raw<std::uint32_t>(at) : read_raw<std::uint16_t>(at);
            vertex.color = expand_color(raw, color_type);
        }
        if constexpr (NormalType != 0u) {
            const std::uint8_t *at = base + layout.normal_offset;
            for (std::uint32_t axis = 0; axis < 3u; ++axis)
                vertex.normal[axis] = read_field<NormalType, true>(at + axis * normal_size) * normal_scale;
        }
        if constexpr (PositionType != 0u) {
            const std::uint8_t *at = base + layout.position_offset;
            for (std::uint32_t axis = 0; axis < 3u; ++axis) {
                float value = read_field<PositionType, true>(at + axis * position_size);
                if (PositionType == 2u && axis == 2u && layout.through)
                    value = static_cast<float>(read_raw<std::uint16_t>(at + axis * position_size));
                vertex.position[axis] = value * position_scale;
            }
        }
        if constexpr (WeightType != 0u) {
            if (skinned) {
                const std::uint8_t *at = base + layout.weight_offset;
                std::array<float, 3> position{};
                std::array<float, 3> normal{};
                for (std::uint32_t bone = 0; bone < bones; ++bone) {
                    const float weight = read_field<WeightType, false>(at + bone * weight_size) * weight_scale;
                    if (weight == 0.0f) continue;
                    const float *m = bone_matrices + bone * 12u;
                    for (std::uint32_t axis = 0; axis < 3u; ++axis) {
                        position[axis] += weight * (vertex.position[0] * m[axis] + vertex.position[1] * m[3u + axis] +
                                                    vertex.position[2] * m[6u + axis] + m[9u + axis]);
                        normal[axis] += weight * (vertex.normal[0] * m[axis] + vertex.normal[1] * m[3u + axis] +
                                                  vertex.normal[2] * m[6u + axis]);
                    }
                }
                vertex.position[0] = position[0];
                vertex.position[1] = position[1];
                vertex.position[2] = position[2];
                vertex.normal = normal;
            }
        }
        out[i] = vertex;
    }
}

using DecodeRun = void (*)(const std::uint8_t *, std::uint32_t, const VertexLayout &, Vertex *, const float *);

template <std::uint32_t WeightType, std::uint32_t NormalType>
constexpr std::array<DecodeRun, 4> runs_for_position() {
    return {nullptr, &decode_run<WeightType, NormalType, 1u>, &decode_run<WeightType, NormalType, 2u>,
            &decode_run<WeightType, NormalType, 3u>};
}

template <std::uint32_t WeightType>
constexpr std::array<std::array<DecodeRun, 4>, 4> runs_for_normal() {
    return {runs_for_position<WeightType, 0u>(), runs_for_position<WeightType, 1u>(),
            runs_for_position<WeightType, 2u>(), runs_for_position<WeightType, 3u>()};
}

// By weight, normal and position format.
constexpr std::array<std::array<std::array<DecodeRun, 4>, 4>, 4> kDecodeRuns{
    runs_for_normal<0u>(), runs_for_normal<1u>(), runs_for_normal<2u>(), runs_for_normal<3u>()};

// MHP3RD_NO_FAST_DECODE decodes every vertex with the general loop, as
// before; MHP3RD_CHECK_DECODE decodes each run both ways and reports runs
// that differ.
bool fast_decode_enabled() {
    static const bool disabled = std::getenv("MHP3RD_NO_FAST_DECODE") != nullptr;
    return !disabled && !perf::alternate_off(perf::NewPath::Decode);
}

} // namespace

VertexFormat vertex_format(std::uint32_t vertex_type) noexcept {
    // The same placement as decode_vertices() below.
    static constexpr std::uint32_t kComponentSize[4] = {0u, 1u, 2u, 4u};
    static constexpr std::uint32_t kColorSize[8] = {0u, 0u, 0u, 0u, 2u, 2u, 2u, 4u};
    std::uint32_t offset = 0u;
    std::uint32_t biggest = 1u;
    const auto place = [&](std::uint32_t component, std::uint32_t components) {
        if (component == 0u) return kNoVertexField;
        offset = align_up(offset, component);
        biggest = std::max(biggest, component);
        const std::uint32_t at = offset;
        offset += component * components;
        return at;
    };
    VertexFormat format{};
    format.weight_offset = place(kComponentSize[(vertex_type >> 9u) & 3u], ((vertex_type >> 14u) & 7u) + 1u);
    format.texcoord_offset = place(kComponentSize[vertex_type & 3u], 2u);
    format.color_offset = place(kColorSize[(vertex_type >> 2u) & 7u], 1u);
    format.normal_offset = place(kComponentSize[(vertex_type >> 5u) & 3u], 3u);
    format.position_offset = place(kComponentSize[(vertex_type >> 7u) & 3u], 3u);
    format.stride = align_up(offset, biggest);
    return format;
}

std::uint32_t decode_vertices(const GuestMemory &memory, std::uint32_t address, std::uint32_t vertex_type,
                              std::uint32_t count, std::vector<Vertex> &out, const float *bone_matrices) {
    // Field order is weights, texcoords, color, normal, position; every field is
    // aligned to its component size and the vertex to the largest of them.
    const std::uint32_t texcoord_type = vertex_type & 3u;
    const std::uint32_t color_type = (vertex_type >> 2u) & 7u;
    const std::uint32_t normal_type = (vertex_type >> 5u) & 3u;
    const std::uint32_t position_type = (vertex_type >> 7u) & 3u;
    const std::uint32_t weight_type = (vertex_type >> 9u) & 3u;
    const std::uint32_t weight_count = ((vertex_type >> 14u) & 7u) + 1u;
    const std::uint32_t morph_count = ((vertex_type >> 18u) & 7u) + 1u;
    const bool through = (vertex_type & (1u << 23u)) != 0u;

    static constexpr std::uint32_t kComponentSize[4] = {0u, 1u, 2u, 4u};
    static constexpr std::uint32_t kColorSize[8] = {0u, 0u, 0u, 0u, 2u, 2u, 2u, 4u};

    std::uint32_t offset = 0u;
    std::uint32_t biggest = 1u;
    const auto place = [&](std::uint32_t component, std::uint32_t components) {
        if (component == 0u) return std::uint32_t{0xFFFFFFFFu};
        offset = align_up(offset, component);
        biggest = std::max(biggest, component);
        const std::uint32_t at = offset;
        offset += component * components;
        return at;
    };

    const std::uint32_t weight_offset = place(kComponentSize[weight_type], weight_count);
    const bool skinned = weight_type != 0u && bone_matrices != nullptr && !through;
    const std::uint32_t texcoord_offset = place(kComponentSize[texcoord_type], 2u);
    const std::uint32_t color_component = kColorSize[color_type];
    const std::uint32_t color_offset = place(color_component, 1u);
    const std::uint32_t normal_offset = place(kComponentSize[normal_type], 3u);
    const std::uint32_t position_offset = place(kComponentSize[position_type], 3u);
    const std::uint32_t stride = align_up(offset, biggest) * morph_count;
    if (stride == 0u) return 0u;

    // Resolve the whole vertex run once and read it directly; a run that is not
    // contiguous in host memory falls back to checked loads.
    const std::uint8_t *data =
        count != 0u ? memory.raw_pointer(address, static_cast<std::size_t>(count) * stride) : nullptr;
    static const bool check_decode = std::getenv("MHP3RD_CHECK_DECODE") != nullptr;
    const DecodeRun fast_run = data != nullptr && morph_count == 1u && position_type != 0u
                                   ? kDecodeRuns[weight_type][normal_type][position_type]
                                   : nullptr;
    if (fast_run != nullptr && (fast_decode_enabled() || check_decode)) {
        VertexLayout layout{};
        layout.texcoord_type = texcoord_type;
        layout.color_type = color_type;
        layout.weight_count = weight_count;
        layout.weight_offset = weight_offset;
        layout.texcoord_offset = texcoord_offset;
        layout.color_offset = color_offset;
        layout.normal_offset = normal_offset;
        layout.position_offset = position_offset;
        layout.stride = stride;
        layout.through = through;
        layout.skinned = skinned;
        out.resize(count);
        fast_run(data, count, layout, out.data(), bone_matrices);
        if (!check_decode) return stride;
    }
    // MHP3RD_CHECK_DECODE: the fast run's vertices, compared below with the
    // general loop's.
    static std::vector<Vertex> fast_copy;
    const bool compare = check_decode && fast_run != nullptr;
    if (compare) fast_copy = out;
    const auto load8 = [&](std::uint32_t at) -> std::uint8_t {
        return data != nullptr ? data[at - address] : memory.load8(at);
    };
    const auto load16 = [&](std::uint32_t at) -> std::uint16_t {
        if (data == nullptr) return memory.load16(at);
        std::uint16_t value{};
        std::memcpy(&value, data + (at - address), sizeof(value));
        return value;
    };
    const auto load32 = [&](std::uint32_t at) -> std::uint32_t {
        if (data == nullptr) return memory.load32(at);
        std::uint32_t value{};
        std::memcpy(&value, data + (at - address), sizeof(value));
        return value;
    };

    const auto read_unsigned = [&](std::uint32_t at, std::uint32_t type) -> float {
        switch (type) {
        case 1u: return static_cast<float>(load8(at));
        case 2u: return static_cast<float>(load16(at));
        case 3u: {
            const std::uint32_t bits = load32(at);
            float value{};
            std::memcpy(&value, &bits, sizeof(value));
            return value;
        }
        default: return 0.0f;
        }
    };

    const auto read_component = [&](std::uint32_t at, std::uint32_t type) -> float {
        switch (type) {
        case 1u: return static_cast<float>(static_cast<std::int8_t>(load8(at)));
        case 2u: return static_cast<float>(static_cast<std::int16_t>(load16(at)));
        case 3u: {
            const std::uint32_t bits = load32(at);
            float value{};
            std::memcpy(&value, &bits, sizeof(value));
            return value;
        }
        default: return 0.0f;
        }
    };

    out.clear();
    out.reserve(count);
    for (std::uint32_t i = 0; i < count; ++i) {
        const std::uint32_t base = address + i * stride;
        Vertex vertex{};
        if (texcoord_offset != 0xFFFFFFFFu) {
            // Texture coordinates are unsigned; reading them signed wrapped the
            // upper half of every 8- and 16-bit UV range to negative values.
            const std::uint32_t component = kComponentSize[texcoord_type];
            const float scale = texcoord_type == 1u ? 1.0f / 128.0f
                                                    : (texcoord_type == 2u && !through ? 1.0f / 32768.0f : 1.0f);
            vertex.texcoord[0] = read_unsigned(base + texcoord_offset, texcoord_type) * scale;
            vertex.texcoord[1] = read_unsigned(base + texcoord_offset + component, texcoord_type) * scale;
        }
        if (color_offset != 0xFFFFFFFFu) {
            const std::uint32_t raw = color_component == 2u ? load16(base + color_offset)
                                                            : load32(base + color_offset);
            vertex.color = expand_color(raw, color_type);
        }
        if (normal_offset != 0xFFFFFFFFu) {
            const std::uint32_t component = kComponentSize[normal_type];
            const float scale = normal_type == 1u ? 1.0f / 128.0f
                                                  : (normal_type == 2u ? 1.0f / 32768.0f : 1.0f);
            for (std::uint32_t axis = 0; axis < 3u; ++axis)
                vertex.normal[axis] = read_component(base + normal_offset + axis * component, normal_type) * scale;
        }
        if (position_offset != 0xFFFFFFFFu) {
            const std::uint32_t component = kComponentSize[position_type];
            // Screen-space vertices keep their integer units; transformed ones
            // are normalized by their component size (128 / 32768, not 127 /
            // 32767 — the GE divides by the magnitude of the sign bit).
            const float scale = through ? 1.0f
                                        : (position_type == 1u ? 1.0f / 128.0f
                                                               : (position_type == 2u ? 1.0f / 32768.0f : 1.0f));
            for (std::uint32_t axis = 0; axis < 3u; ++axis) {
                float value = read_component(base + position_offset + axis * component, position_type);
                // Through-mode Z is unsigned in the 16-bit case.
                if (through && axis == 2u && position_type == 2u)
                    value = static_cast<float>(load16(base + position_offset + axis * component));
                vertex.position[axis] = value * scale;
            }
        }
        if (skinned) {
            // A skinned vertex is stored in its bones' space: the GE blends it
            // by its weights through the bone matrices, and the result is what
            // the world matrix then sees. Without this the raw positions stay
            // inside the unit cube they were normalised into and the whole model
            // collapses onto a couple of pixels.
            const std::uint32_t component = kComponentSize[weight_type];
            const float weight_scale = weight_type == 1u ? 1.0f / 128.0f
                                                         : (weight_type == 2u ? 1.0f / 32768.0f : 1.0f);
            std::array<float, 3> position{};
            std::array<float, 3> normal{};
            for (std::uint32_t bone = 0; bone < weight_count && bone < 8u; ++bone) {
                const float weight =
                    read_unsigned(base + weight_offset + bone * component, weight_type) * weight_scale;
                if (weight == 0.0f) continue;
                // PSP 3x4 matrices are three basis rows plus a translation row,
                // used as a row vector: p' = p * M.
                const float *m = bone_matrices + bone * 12u;
                for (std::uint32_t axis = 0; axis < 3u; ++axis) {
                    position[axis] += weight * (vertex.position[0] * m[axis] + vertex.position[1] * m[3u + axis] +
                                                vertex.position[2] * m[6u + axis] + m[9u + axis]);
                    normal[axis] += weight * (vertex.normal[0] * m[axis] + vertex.normal[1] * m[3u + axis] +
                                              vertex.normal[2] * m[6u + axis]);
                }
            }
            vertex.position[0] = position[0];
            vertex.position[1] = position[1];
            vertex.position[2] = position[2];
            vertex.normal = normal;
        }
        out.push_back(vertex);
    }
    if (compare) {
        static std::uint64_t checked = 0u;
        static std::uint64_t differed = 0u;
        ++checked;
        if (fast_copy.size() != out.size() ||
            std::memcmp(fast_copy.data(), out.data(), out.size() * sizeof(Vertex)) != 0) {
            if (++differed <= 20u)
                std::cout << "[decode-check] vertex type 0x" << std::hex << vertex_type << std::dec << " differs over "
                          << out.size() << " vertices\n";
        }
        if (checked % 100000u == 0u)
            std::cout << "[decode-check] " << checked << " runs compared, " << differed << " differed" << std::endl;
    }
    return stride;
}

void GeState::handle_command(const GuestMemory &memory, std::uint32_t command, std::uint32_t data) {
    registers_[command] = data;
    // Which of 0x53..0x5C actually carries the colour the guest sets is worth
    // reading off the game rather than recalling: MHP3RD_TRACE_MATERIAL reports
    // every distinct value each of them is given.
    if (static const bool trace = std::getenv("MHP3RD_TRACE_MATERIAL") != nullptr;
        trace && command >= 0x53u && command <= 0x5Cu) {
        static std::map<std::uint32_t, std::map<std::uint32_t, std::uint64_t>> seen;
        auto &values = seen[command];
        if (++values[data] == 1u && values.size() <= 24u)
            std::cout << "[material] cmd=0x" << std::hex << command << " value=0x" << data << std::dec << "\n";
    }
    // MHP3RD_TRACE_LIGHTING does the same for the registers around that run
    // that lighting and fog may live in: the enables after 0x17, 0x50..0x52,
    // 0x5D..0x9A and 0xC8..0xD0. Each value is also shown as a 24-bit float,
    // since several of them carry one.
    if (static const bool trace = std::getenv("MHP3RD_TRACE_LIGHTING") != nullptr;
        trace && ((command >= 0x18u && command <= 0x20u) || (command >= 0x50u && command <= 0x52u) ||
                  (command >= 0x5Du && command <= 0x9Au) || (command >= 0xC8u && command <= 0xD0u))) {
        static std::map<std::uint32_t, std::map<std::uint32_t, std::uint64_t>> seen;
        auto &values = seen[command];
        if (++values[data] == 1u && values.size() <= 16u)
            std::cout << "[lighting] cmd=0x" << std::hex << command << " value=0x" << data << std::dec
                      << " float=" << decode_float24(data) << "\n";
    }
    switch (command) {
    // Addresses in a display list are relative: BASE supplies four high bits and
    // OFFSET_ADDR is added on top of them. Conflating the two — and letting
    // OFFSET_ADDR overwrite BASE — produced pointers into nowhere.
    case kVertexAddress: vertex_address_ = relative_address(data); break;
    case kIndexAddress: index_address_ = relative_address(data); break;
    case kBase: base_extended_ = (data & 0x000F0000u) << 8u; break;
    case kVertexType: vertex_type_ = data; break;
    case kOffsetAddress: offset_address_ = data << 8u; break;
    case kOrigin: break;  // handled in execute(), where the list pc is known

    case kCullFaceEnable: culling_enabled_ = (data & 1u) != 0u; break;
    case kCull: cull_clockwise_ = (data & 1u) != 0u; break;
    case kTextureMapEnable: texture_.enabled = (data & 1u) != 0u; break;
    case kLightingEnable: lighting_enabled_ = (data & 1u) != 0u; break;
    case kAlphaBlendEnable: blend_.enabled = (data & 1u) != 0u; break;
    case kAlphaTestEnable: alpha_test_.enabled = (data & 1u) != 0u; break;
    case kDepthTestEnable: depth_.test_enabled = (data & 1u) != 0u; break;
    case kDepthWriteDisable: depth_.write_enabled = (data & 1u) == 0u; break;
    case kDepthTest: depth_.function = data & 7u; break;
    case kMinZ: depth_.range_near = static_cast<std::uint16_t>(data); break;
    case kMaxZ: depth_.range_far = static_cast<std::uint16_t>(data); break;

    case kAlphaTest:
        alpha_test_.function = data & 7u;
        alpha_test_.reference = (data >> 8u) & 0xFFu;
        alpha_test_.mask = (data >> 16u) & 0xFFu;
        break;
    case kBlendMode:
        blend_.source_factor = data & 0xFu;
        blend_.destination_factor = (data >> 4u) & 0xFu;
        blend_.equation = (data >> 8u) & 0xFu;
        break;
    case kBlendFixedA: blend_.fixed_source = data; break;
    case kBlendFixedB: blend_.fixed_destination = data; break;

    case kFrameBufferPointer:
        target_.color_address = resolve_ge_address((target_.color_address & 0xFF000000u) | data);
        break;
    case kFrameBufferWidth:
        target_.color_stride = data & 0xFFFFu;
        target_.color_address =
            resolve_ge_address((target_.color_address & 0x00FFFFFFu) | ((data << 8u) & 0xFF000000u));
        break;
    case kDepthBufferPointer:
        target_.depth_address = resolve_ge_address((target_.depth_address & 0xFF000000u) | data);
        break;
    case kDepthBufferWidth:
        target_.depth_stride = data & 0xFFFFu;
        target_.depth_address =
            resolve_ge_address((target_.depth_address & 0x00FFFFFFu) | ((data << 8u) & 0xFF000000u));
        break;
    case kFrameBufferPixelFormat: target_.color_format = data & 3u; break;

    case kTextureAddress0: texture_.address = resolve_ge_address((texture_.address & 0xFF000000u) | data); break;
    case kTextureBufferWidth0:
        texture_.buffer_width = data & 0xFFFFu;
        texture_.address = resolve_ge_address((texture_.address & 0x00FFFFFFu) | ((data << 8u) & 0xFF000000u));
        break;
    case kTextureSize0:
        texture_.width = static_cast<std::uint16_t>(1u << (data & 0xFu));
        texture_.height = static_cast<std::uint16_t>(1u << ((data >> 8u) & 0xFu));
        break;
    case kTextureFormat: texture_.format = static_cast<TextureFormat>(data & 0xFu); break;
    case kTextureMode: texture_.swizzled = (data & 1u) != 0u; break;
    case kClutAddress: texture_.clut_address = resolve_ge_address((texture_.clut_address & 0xFF000000u) | data); break;
    case kClutAddressUpper:
        texture_.clut_address = resolve_ge_address((texture_.clut_address & 0x00FFFFFFu) | ((data << 8u) & 0xFF000000u));
        break;
    case kClutFormat:
        texture_.clut_format_word = (command << 24u) | data;
        texture_.clut_format = data & 3u;
        texture_.clut_shift = (data >> 2u) & 0x1Fu;
        texture_.clut_mask = (data >> 8u) & 0xFFu;
        texture_.clut_offset = (data >> 16u) & 0x1Fu;
        break;
    case kTextureFilter:
        texture_.min_filter = data & 7u;
        texture_.mag_filter = (data >> 8u) & 1u;
        break;
    case kTextureWrap:
        texture_.wrap_s = data & 1u;
        texture_.wrap_t = (data >> 8u) & 1u;
        break;
    case kTextureFunction:
        texture_.function = data & 7u;
        texture_.alpha_from_texture = ((data >> 8u) & 1u) != 0u;
        break;
    case kTexScaleU: texture_.scale_u = decode_float24(data); break;
    case kTexScaleV: texture_.scale_v = decode_float24(data); break;
    case kTexOffsetU: texture_.offset_u = decode_float24(data); break;
    case kTexOffsetV: texture_.offset_v = decode_float24(data); break;

    case kViewportXScale: viewport_.x_scale = decode_float24(data); break;
    case kViewportYScale: viewport_.y_scale = decode_float24(data); break;
    case kViewportZScale: viewport_.z_scale = decode_float24(data); break;
    case kViewportXCenter: viewport_.x_offset = decode_float24(data); break;
    case kViewportYCenter: viewport_.y_offset = decode_float24(data); break;
    case kViewportZCenter: viewport_.z_offset = decode_float24(data); break;
    case kScissor1:
        viewport_.scissor_x1 = data & 0x3FFu;
        viewport_.scissor_y1 = (data >> 10u) & 0x3FFu;
        break;
    case kScissor2:
        viewport_.scissor_x2 = data & 0x3FFu;
        viewport_.scissor_y2 = (data >> 10u) & 0x3FFu;
        break;
    // The offset is an unsigned 16-bit value with 4 fractional bits; the bits
    // above it are not part of it.
    case kOffsetX: viewport_.offset_x = static_cast<float>(data & 0xFFFFu) / 16.0f; break;
    case kOffsetY: viewport_.offset_y = static_cast<float>(data & 0xFFFFu) / 16.0f; break;

    // The material registers; each write moves material_version_.
    case kMaterialAmbient:
        material_color_ = (material_color_ & 0xFF000000u) | (data & 0x00FFFFFFu);
        ++material_version_;
        break;
    case kMaterialAlpha:
        material_color_ = (material_color_ & 0x00FFFFFFu) | ((data & 0xFFu) << 24u);
        ++material_version_;
        break;
    case kReverseNormal: lighting_.reverse_normals = (data & 1u) != 0u; ++material_version_; break;
    case kMaterialUpdate: lighting_.material_update = data & 7u; ++material_version_; break;
    case kMaterialEmissive: lighting_.material_emissive = data & 0x00FFFFFFu; ++material_version_; break;
    case kMaterialDiffuse: lighting_.material_diffuse = data & 0x00FFFFFFu; ++material_version_; break;
    case kMaterialSpecular: lighting_.material_specular = data & 0x00FFFFFFu; ++material_version_; break;
    case kMaterialSpecularPower: lighting_.specular_power = decode_float24(data); ++material_version_; break;
    case kLightMode: lighting_.mode = data & 1u; ++material_version_; break;

    // The lighting environment: global ambient, lights and fog parameters;
    // each write moves environment_version_.
    case kAmbientColor: lighting_.ambient_color = data & 0x00FFFFFFu; ++environment_version_; break;
    case kAmbientAlpha: lighting_.ambient_alpha = data & 0xFFu; ++environment_version_; break;
    case kLightEnable0:
    case kLightEnable0 + 1:
    case kLightEnable0 + 2:
    case kLightEnable0 + 3:
        lighting_.lights[command - kLightEnable0].enabled = (data & 1u) != 0u;
        ++environment_version_;
        break;

    case kFogEnable: fog_.enabled = (data & 1u) != 0u; break;
    case kFogEnd: fog_.end = decode_float24(data); ++environment_version_; break;
    case kFogScale: fog_.scale = decode_float24(data); ++environment_version_; break;
    case kFogColor: fog_.color = data & 0x00FFFFFFu; ++environment_version_; break;

    case kWorldMatrixNumber: world_write_index_ = data & 0xFu; break;
    case kViewMatrixNumber: view_write_index_ = data & 0xFu; break;
    case kProjMatrixNumber: projection_write_index_ = data & 0xFu; break;
    case kTexGenMatrixNumber: texture_write_index_ = data & 0xFu; break;
    case kBoneMatrixNumber: bone_write_index_ = data & 0x7Fu; break;

    case kWorldMatrixData:
    case kViewMatrixData:
    case kTexGenMatrixData:
    case kProjMatrixData:
    case kBoneMatrixData: {
        // 3x4 matrices arrive as 12 values, three basis rows then a translation
        // row; the projection matrix has all 16. Expand the 3x4 forms into the
        // column-major 4x4 the shader multiplies as M * v.
        const float value = decode_float24(data);
        const auto store_3x4 = [&](std::array<float, 16> &matrix, std::uint32_t &index) {
            if (index < 12u) {
                matrix[(index / 3u) * 4u + (index % 3u)] = value;
                matrix[3] = matrix[7] = matrix[11] = 0.0f;
                matrix[15] = 1.0f;
            }
            ++index;
        };
        if (command == kProjMatrixData) {
            if (projection_write_index_ < 16u) projection_[projection_write_index_] = value;
            ++projection_write_index_;
        } else if (command == kWorldMatrixData) {
            store_3x4(world_, world_write_index_);
        } else if (command == kViewMatrixData) {
            store_3x4(view_, view_write_index_);
        } else if (command == kTexGenMatrixData) {
            store_3x4(texture_matrix_, texture_write_index_);
        } else {
            if (bone_write_index_ < bone_matrices_.size()) bone_matrices_[bone_write_index_] = value;
            ++bone_write_index_;
        }
        break;
    }

    case kLightType0:
    case kLightType0 + 1:
    case kLightType0 + 2:
    case kLightType0 + 3: {
        LightState &light = lighting_.lights[command - kLightType0];
        light.kind = data & 3u;
        light.type = (data >> 8u) & 3u;
        ++environment_version_;
        break;
    }

    case kClearMode:
        // Bit 0 enables clear mode; bits 8..10 select which buffers it writes
        // (color, alpha/stencil, depth).
        clear_mode_ = (data & 1u) != 0u;
        clear_flags_ = (data >> 8u) & 7u;
        break;

    case kTransferSourceAddress:
    case kTransferSourceWidth:
    case kTransferDestinationAddress:
    case kTransferDestinationWidth:
    case kTransferSourcePosition:
    case kTransferDestinationPosition:
    case kTransferSize:
        break;  // read from registers_ when the transfer starts
    case kTransferStart: {
        BlockTransfer transfer{};
        const auto address = [&](std::uint32_t low, std::uint32_t width) {
            return resolve_ge_address(((registers_[width] << 8u) & 0xFF000000u) | (registers_[low] & 0x00FFFFFFu));
        };
        transfer.source = address(kTransferSourceAddress, kTransferSourceWidth);
        transfer.source_stride = registers_[kTransferSourceWidth] & 0xFFFFu;
        transfer.destination = address(kTransferDestinationAddress, kTransferDestinationWidth);
        transfer.destination_stride = registers_[kTransferDestinationWidth] & 0xFFFFu;
        transfer.source_x = registers_[kTransferSourcePosition] & 0x3FFu;
        transfer.source_y = (registers_[kTransferSourcePosition] >> 10u) & 0x3FFu;
        transfer.destination_x = registers_[kTransferDestinationPosition] & 0x3FFu;
        transfer.destination_y = (registers_[kTransferDestinationPosition] >> 10u) & 0x3FFu;
        transfer.width = (registers_[kTransferSize] & 0x3FFu) + 1u;
        transfer.height = ((registers_[kTransferSize] >> 10u) & 0x3FFu) + 1u;
        transfer.bytes_per_pixel = (data & 1u) != 0u ? 4u : 2u;
        static const bool trace = std::getenv("MHP3RD_TRACE_FB_TEXTURES") != nullptr;
        if (trace)
            std::cout << "[fbtex] block transfer 0x" << std::hex << transfer.source << std::dec << " ("
                      << transfer.source_x << "," << transfer.source_y << " row " << transfer.source_stride
                      << ") -> 0x" << std::hex << transfer.destination << std::dec << " (" << transfer.destination_x
                      << "," << transfer.destination_y << " row " << transfer.destination_stride << ") "
                      << transfer.width << "x" << transfer.height << " x" << transfer.bytes_per_pixel << " bytes\n";
        if (transfer_sink_) transfer_sink_(transfer);
        break;
    }

    case kLoadClut: {
        // Blocks of 32 bytes. The PSP reads at most 0x3F of them; a count of
        // exactly 0x40 still loads, which some games rely on.
        const std::uint32_t blocks = (data & 0x7Fu) == 0x40u ? 0x40u : (data & 0x3Fu);
        if (blocks != 0u) {
            texture_.clut_load_bytes = blocks * 32u;
            texture_.clut_max_bytes = std::max(texture_.clut_max_bytes, texture_.clut_load_bytes);
        }
        break;
    }

    case kNop:
    case kTextureFlush:
        break;

    default:
        // The per-light runs: three floats per light for positions, directions
        // and attenuations, one float for the spot exponent and cutoff, and
        // three colours per light.
        if (command >= kLightPosition0 && command < kLightDirection0) {
            const std::uint32_t at = command - kLightPosition0;
            lighting_.lights[at / 3u].position[at % 3u] = decode_float24(data);
        } else if (command >= kLightDirection0 && command < kLightAttenuation0) {
            const std::uint32_t at = command - kLightDirection0;
            lighting_.lights[at / 3u].direction[at % 3u] = decode_float24(data);
        } else if (command >= kLightAttenuation0 && command < kSpotExponent0) {
            const std::uint32_t at = command - kLightAttenuation0;
            lighting_.lights[at / 3u].attenuation[at % 3u] = decode_float24(data);
        } else if (command >= kSpotExponent0 && command < kSpotCutoff0) {
            lighting_.lights[command - kSpotExponent0].spot_exponent = decode_float24(data);
        } else if (command >= kSpotCutoff0 && command < kLightColor0) {
            lighting_.lights[command - kSpotCutoff0].spot_cutoff = decode_float24(data);
        } else if (command >= kLightColor0 && command <= kLightColorEnd) {
            const std::uint32_t at = command - kLightColor0;
            LightState &light = lighting_.lights[at / 3u];
            std::uint32_t &color = at % 3u == 0u ? light.ambient : at % 3u == 1u ? light.diffuse : light.specular;
            color = data & 0x00FFFFFFu;
        } else {
            ++unhandled_commands_;
            trace_unhandled(command, data);
            break;
        }
        ++environment_version_;
        break;
    }
}

namespace {

struct VramCopy {
    std::uint32_t destination{};
    std::uint32_t source{};
    std::uint32_t size{};
};
std::vector<VramCopy> &vram_copies() {
    static std::vector<VramCopy> copies;
    return copies;
}

} // namespace

void note_vram_copy(std::uint32_t destination, std::uint32_t source, std::uint32_t size) {
    std::vector<VramCopy> &copies = vram_copies();
    copies.erase(std::remove_if(copies.begin(), copies.end(),
                                [&](const VramCopy &copy) { return copy.destination == destination; }),
                 copies.end());
    if (copies.size() >= 64u) copies.erase(copies.begin());
    copies.push_back({destination & 0x1FFFFFFFu, source & 0x1FFFFFFFu, size});
}

bool find_vram_copy(std::uint32_t address, std::uint32_t &source, std::uint32_t &destination) {
    address &= 0x1FFFFFFFu;
    const std::vector<VramCopy> &copies = vram_copies();
    for (auto it = copies.rbegin(); it != copies.rend(); ++it) {
        if (address >= it->destination && address - it->destination < it->size) {
            source = it->source;
            destination = it->destination;
            return true;
        }
    }
    return false;
}

// MHP3RD_TRACE_FB_TEXTURES also lists the commands the GE state ignores, with
// the first few values of each, so copies the renderer never sees (block
// transfers, for example) show up next to the textures that read their result.
void GeState::trace_unhandled(std::uint32_t command, std::uint32_t data) {
    static const bool trace = std::getenv("MHP3RD_TRACE_FB_TEXTURES") != nullptr;
    if (!trace) return;
    static std::map<std::uint32_t, std::uint32_t> seen;
    std::uint32_t &count = seen[command];
    if (count >= 4u) return;
    ++count;
    std::cout << "[fbtex] unhandled GE command 0x" << std::hex << command << " data=0x" << data << std::dec << "\n";
}

void GeState::draw_primitive(const GuestMemory &memory, std::uint32_t data) {
    const std::uint32_t count = data & 0xFFFFu;
    const auto primitive = static_cast<PrimitiveType>((data >> 16u) & 7u);
    if (count == 0u || vertex_address_ == 0u) return;

    const std::uint32_t index_type = (vertex_type_ >> 11u) & 3u;
    // One DrawCall is reused for every draw, so its vertex and index vectors
    // keep their capacity instead of being allocated and freed per draw.
    DrawCall &call = call_;
    call.vertices.clear();
    call.indices.clear();
    call.primitive = primitive;
    call.through = (vertex_type_ & (1u << 23u)) != 0u;
    call.texture = texture_;
    call.target = target_;
    call.blend = blend_;
    call.depth = depth_;
    call.alpha_test = alpha_test_;
    call.viewport = viewport_;
    call.culling_enabled = culling_enabled_;
    call.cull_clockwise = cull_clockwise_;
    call.clear_mode = clear_mode_;
    call.clear_flags = clear_flags_;
    call.vertex_type = vertex_type_;
    call.vertex_address = vertex_address_;
    call.index_address = index_type != 0u ? index_address_ : 0u;
    call.primitive_count = count;
    call.material_color = material_color_;
    call.lighting_enabled = lighting_enabled_;
    call.has_vertex_color = ((vertex_type_ >> 2u) & 7u) != 0u;
    call.lighting = lighting_;
    call.fog = fog_;
    call.environment_version = environment_version_;
    call.material_version = material_version_;
    call.world = world_;
    call.view = view_;
    call.projection = projection_;
    call.texture_matrix = texture_matrix_;

    // MHP3RD_TRACE_RENDER: from here to the decoded vertices is "decode".
    const bool split = perf::split_enabled();
    const std::uint64_t split_start = split ? perf::split_ticks() : 0u;
    std::uint32_t vertex_count = count;
    std::uint32_t first_vertex = 0u;
    if (index_type != 0u && index_address_ != 0u) {
        // A list that points its indices outside RAM is malformed; skip the draw
        // rather than faulting the whole runtime on it.
        if (!memory.contains(index_address_, count * (index_type == 1u ? 1u : index_type == 2u ? 2u : 4u))) return;
        const std::uint32_t index_size = index_type == 1u ? 1u : index_type == 2u ? 2u : 4u;
        // Resolve the index run once; a run that is not contiguous in host
        // memory falls back to checked loads.
        const std::uint8_t *raw = memory.raw_pointer(index_address_, static_cast<std::size_t>(count) * index_size);
        call.indices.resize(count);
        std::uint32_t lowest = 0xFFFFFFFFu;
        std::uint32_t highest = 0u;
        for (std::uint32_t i = 0; i < count; ++i) {
            std::uint32_t index = 0u;
            if (raw != nullptr) {
                if (index_type == 1u) {
                    index = raw[i];
                } else if (index_type == 2u) {
                    std::uint16_t value{};
                    std::memcpy(&value, raw + i * 2u, sizeof(value));
                    index = value;
                } else {
                    std::memcpy(&index, raw + i * 4u, sizeof(index));
                }
            } else if (index_type == 1u) {
                index = memory.load8(index_address_ + i);
            } else if (index_type == 2u) {
                index = memory.load16(index_address_ + i * 2u);
            } else {
                index = memory.load32(index_address_ + i * 4u);
            }
            lowest = std::min(lowest, index);
            highest = std::max(highest, index);
            call.indices[i] = static_cast<std::uint16_t>(index);
        }
        // Games draw a mesh as many indexed prims into one shared vertex
        // buffer. Decode only the vertices this prim references, not the
        // whole buffer from vertex 0, and rebase its indices onto them.
        if (count != 0u) {
            first_vertex = lowest;
            for (std::uint16_t &index : call.indices) index = static_cast<std::uint16_t>(index - lowest);
        }
        vertex_count = count != 0u ? highest - lowest + 1u : 0u;
    }

    const std::uint32_t probe = decode_vertices(memory, vertex_address_, vertex_type_, 0u, call.vertices);
    if (probe == 0u) return;
    const std::uint32_t first_address = vertex_address_ + first_vertex * probe;
    if (!memory.contains(first_address, static_cast<std::size_t>(probe) * vertex_count)) return;
    call.raw_vertices = nullptr;
    call.raw_count = 0u;
    call.raw_stride = 0u;
    call.bone_matrices = nullptr;
    std::uint32_t stride = 0u;
    const bool triangles = primitive == PrimitiveType::Triangles || primitive == PrimitiveType::TriangleStrip ||
                           primitive == PrimitiveType::TriangleFan;
    const bool one_morph = ((vertex_type_ >> 18u) & 7u) == 0u;
    const bool positioned = ((vertex_type_ >> 7u) & 3u) != 0u;
    if (raw_vertices_ && !call.through && triangles && one_morph && positioned && vertex_count != 0u) {
        call.raw_vertices = memory.raw_pointer(first_address, static_cast<std::size_t>(probe) * vertex_count);
        if (call.raw_vertices != nullptr) {
            call.raw_count = vertex_count;
            call.raw_stride = probe;
            call.bone_matrices = bone_matrices_.data();
            stride = probe;
        }
    }
    if (call.raw_vertices == nullptr || raw_also_decoded_)
        stride = decode_vertices(memory, first_address, vertex_type_, vertex_count, call.vertices,
                                 bone_matrices_.data());
    if (split) perf::add_split(perf::Split::Decode, perf::split_ticks() - split_start);
    if (stride == 0u || (call.vertices.empty() && call.raw_vertices == nullptr)) return;
    // A prim leaves VADDR/IADDR alone but advances the pointer it consumed, so
    // a run of prims can share one setup. An indexed prim consumes indices, not
    // vertices: advancing the vertex pointer instead walked it off the mesh and
    // every prim after the first read its vertices from the wrong place.
    if (index_type != 0u && index_address_ != 0u)
        index_address_ += count * (index_type == 1u ? 1u : index_type == 2u ? 2u : 4u);
    else
        vertex_address_ += stride * count;

    // Which register values a lit draw is actually made with: one line per
    // distinct combination of vertex type, enables and material registers,
    // printed with every non-zero register lighting may read. Light positions
    // and colours are left out of the key because the game animates them.
    if (static const bool trace = std::getenv("MHP3RD_TRACE_LIGHTING") != nullptr; trace && lighting_enabled_ && !call.vertices.empty()) {
        static std::map<std::vector<std::uint32_t>, std::uint64_t> seen;
        std::vector<std::uint32_t> key{vertex_type_};
        for (std::uint32_t command = 0x18u; command <= 0x1Fu; ++command) key.push_back(registers_[command]);
        for (std::uint32_t command = 0x50u; command <= 0x5Eu; ++command) key.push_back(registers_[command]);
        if (++seen[key] == 1u && seen.size() <= 256u) {
            std::cout << "[lit-draw] vtype=0x" << std::hex << vertex_type_ << " verts=" << std::dec
                      << call.vertices.size() << " n0=(" << call.vertices[0].normal[0] << ","
                      << call.vertices[0].normal[1] << "," << call.vertices[0].normal[2] << ") c0=0x" << std::hex
                      << call.vertices[0].color << " regs";
            for (std::uint32_t command = 0x18u; command <= 0x1Fu; ++command)
                std::cout << " " << command << ":" << registers_[command];
            for (std::uint32_t command = 0x50u; command <= 0x9Au; ++command)
                if (registers_[command] != 0u) std::cout << " " << command << ":" << registers_[command];
            std::cout << std::dec << "\n";
        }
    }

    ++draw_count_;
    vertex_count_ += call.raw_vertices != nullptr ? call.raw_count : call.vertices.size();
    if (draw_sink_) draw_sink_(call);
}

std::uint32_t GeState::execute(const GuestMemory &memory, std::uint32_t pc, std::uint32_t stall, bool &finished) {
    finished = false;
    if (world_[15] == 0.0f) {
        identity(world_);
        identity(view_);
        identity(projection_);
        identity(texture_matrix_);
    }

    for (std::uint32_t steps = 0; steps < 2'000'000u; ++steps) {
        if (stall != 0u && pc == stall) return pc;
        if (!memory.contains(pc, 4u)) return pc;
        const std::uint32_t word = memory.load32(pc);
        const std::uint32_t command = word >> 24u;
        const std::uint32_t data = word & 0x00FFFFFFu;
        // Where the camera came from: the display list the game built holds the
        // view matrix as commands, and that address is the one thing about the
        // camera the host can always point at.
        if (command == kViewMatrixNumber) view_matrix_source_ = pc;
        pc += 4u;

        switch (command) {
        case kJump:
            pc = relative_address(data) & 0x0FFFFFFCu;
            continue;
        case kOrigin:
            // ORIGIN makes later relative addresses count from this command.
            offset_address_ = pc - 4u;
            continue;
        case kConditionalJump:
            // The bounding-box test is not evaluated; taking the jump would skip
            // geometry, so fall through to the next command instead.
            continue;
        case kCall:
            call_stack_.push_back(pc);
            pc = relative_address(data) & 0x0FFFFFFCu;
            continue;
        case kReturn:
            if (!call_stack_.empty()) {
                pc = call_stack_.back();
                call_stack_.pop_back();
            }
            continue;
        case kEnd:
            finished = true;
            return pc;
        case kFinish:
            if (signal_sink_) signal_sink_(0x10000u | (data & 0xFFFFu), pc);
            continue;
        case kSignal:
            if (signal_sink_) signal_sink_(data & 0xFFFFu, pc);
            continue;
        case kPrimitive:
            draw_primitive(memory, data);
            continue;
        case kBezier:
        case kSpline:
            // Curved surfaces are not tessellated yet.
            ++unhandled_commands_;
            trace_unhandled(command, data);
            continue;
        default:
            handle_command(memory, command, data);
            continue;
        }
    }
    return pc;
}

} // namespace mhp3rd::gpu
