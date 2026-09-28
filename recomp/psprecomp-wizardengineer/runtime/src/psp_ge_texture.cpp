#include "psp_ge_texture.h"
#include "psp_ge.h"
#include "psp_ge_constants.h"
#include "psp_memory.h"
#include "recomp.h"

#include <glad/glad.h>
#include <array>
#include <vector>
#include <cstdio>
#include <cstring>
#include <algorithm>

// ---- Module state ----
static std::array<TexCacheEntry, TEX_CACHE_SIZE> g_tex_cache;
static bool g_tex_initialized = false;

// ---- FNV-1a hash ----

static uint32_t fnv1a_hash(
    const uint8_t* data, size_t len
) {
    uint32_t hash = 0x811C9DC5u;
    for (size_t i = 0; i < len; i++) {
        hash ^= data[i];
        hash *= 0x01000193u;
    }
    return hash;
}

// ---- Unswizzle ----

/// Unswizzle PSP tiled texture to linear layout.
/// PSP tiles are 16 bytes wide by 8 rows tall.
static void unswizzle_texture(
    const uint8_t* src,
    uint8_t* dst,
    int width_bytes,
    int height
) {
    int bxc = width_bytes / 16;
    int byc = height / 8;

    if (bxc <= 0) bxc = 1;
    if (byc <= 0) byc = 1;

    uint32_t src_off = 0;
    for (int by = 0; by < byc; by++) {
        for (int bx = 0; bx < bxc; bx++) {
            for (int n = 0; n < 8; n++) {
                int dst_off = (by * 8 + n) * width_bytes
                              + bx * 16;
                if (dst_off + 16 <= width_bytes * height
                    && src_off + 16
                       <= static_cast<uint32_t>(
                              width_bytes * height)) {
                    std::memcpy(
                        dst + dst_off, src + src_off, 16);
                }
                src_off += 16;
            }
        }
    }
}

// ---- Format conversion helpers ----

static void convert_5650_to_rgba(
    const uint8_t* src, uint8_t* dst,
    int width, int height
) {
    int pixel_count = width * height;
    const uint16_t* src16 =
        reinterpret_cast<const uint16_t*>(src);
    for (int i = 0; i < pixel_count; i++) {
        uint16_t val = src16[i];
        dst[i * 4 + 0] = static_cast<uint8_t>(
            ((val & 0x1F) * 255) / 31);
        dst[i * 4 + 1] = static_cast<uint8_t>(
            (((val >> 5) & 0x3F) * 255) / 63);
        dst[i * 4 + 2] = static_cast<uint8_t>(
            (((val >> 11) & 0x1F) * 255) / 31);
        dst[i * 4 + 3] = 255;
    }
}

static void convert_5551_to_rgba(
    const uint8_t* src, uint8_t* dst,
    int width, int height
) {
    int pixel_count = width * height;
    const uint16_t* src16 =
        reinterpret_cast<const uint16_t*>(src);
    for (int i = 0; i < pixel_count; i++) {
        uint16_t val = src16[i];
        dst[i * 4 + 0] = static_cast<uint8_t>(
            ((val & 0x1F) * 255) / 31);
        dst[i * 4 + 1] = static_cast<uint8_t>(
            (((val >> 5) & 0x1F) * 255) / 31);
        dst[i * 4 + 2] = static_cast<uint8_t>(
            (((val >> 10) & 0x1F) * 255) / 31);
        dst[i * 4 + 3] = (val & 0x8000) ? 255 : 0;
    }
}

static void convert_4444_to_rgba(
    const uint8_t* src, uint8_t* dst,
    int width, int height
) {
    int pixel_count = width * height;
    const uint16_t* src16 =
        reinterpret_cast<const uint16_t*>(src);
    for (int i = 0; i < pixel_count; i++) {
        uint16_t val = src16[i];
        dst[i * 4 + 0] = static_cast<uint8_t>(
            ((val & 0xF) * 255) / 15);
        dst[i * 4 + 1] = static_cast<uint8_t>(
            (((val >> 4) & 0xF) * 255) / 15);
        dst[i * 4 + 2] = static_cast<uint8_t>(
            (((val >> 8) & 0xF) * 255) / 15);
        dst[i * 4 + 3] = static_cast<uint8_t>(
            (((val >> 12) & 0xF) * 255) / 15);
    }
}

// ---- CLUT decode ----

/// Read CLUT palette into RGBA8888 buffer (up to 256 entries).
static void read_clut_palette(
    uint8_t* rdram,
    const GeState& state,
    uint8_t clut_rgba[256 * 4]
) {
    // PPSSPP semantics: CLUTADDR holds the low 24 bits;
    // CLUTADDRUPPER's data word is shifted left 8 to form
    // bits 24-27 (it arrives as e.g. 0x040000 -> 0x04000000).
    uint32_t clut_addr = (state.clut_addr & 0xFFFFFF)
        | ((state.clut_addr_upper << 8) & 0x0F000000);
    clut_addr &= PSP_ADDR_MASK;

    int clut_fmt = state.clut_format & 0x3;
    int clut_shift = (state.clut_format >> 2) & 0x1F;
    int clut_mask = (state.clut_format >> 8) & 0xFF;
    int clut_start = ((state.clut_format >> 16) & 0x1F)
                     * 16;
    (void)clut_shift;
    (void)clut_mask;
    (void)clut_start;

    for (int i = 0; i < 256; i++) {
        if (clut_fmt == GE_CMODE_32BIT_ABGR8888) {
            uint32_t val = psp_mem_read<uint32_t>(
                rdram, clut_addr + i * 4);
            clut_rgba[i * 4 + 0] = val & 0xFF;
            clut_rgba[i * 4 + 1] = (val >> 8) & 0xFF;
            clut_rgba[i * 4 + 2] = (val >> 16) & 0xFF;
            clut_rgba[i * 4 + 3] = (val >> 24) & 0xFF;
        } else if (clut_fmt == GE_CMODE_16BIT_BGR5650) {
            uint16_t val = psp_mem_read<uint16_t>(
                rdram, clut_addr + i * 2);
            clut_rgba[i * 4 + 0] = static_cast<uint8_t>(
                ((val & 0x1F) * 255) / 31);
            clut_rgba[i * 4 + 1] = static_cast<uint8_t>(
                (((val >> 5) & 0x3F) * 255) / 63);
            clut_rgba[i * 4 + 2] = static_cast<uint8_t>(
                (((val >> 11) & 0x1F) * 255) / 31);
            clut_rgba[i * 4 + 3] = 255;
        } else if (clut_fmt == GE_CMODE_16BIT_ABGR5551) {
            uint16_t val = psp_mem_read<uint16_t>(
                rdram, clut_addr + i * 2);
            clut_rgba[i * 4 + 0] = static_cast<uint8_t>(
                ((val & 0x1F) * 255) / 31);
            clut_rgba[i * 4 + 1] = static_cast<uint8_t>(
                (((val >> 5) & 0x1F) * 255) / 31);
            clut_rgba[i * 4 + 2] = static_cast<uint8_t>(
                (((val >> 10) & 0x1F) * 255) / 31);
            clut_rgba[i * 4 + 3] = (val & 0x8000)
                                    ? 255 : 0;
        } else if (clut_fmt == GE_CMODE_16BIT_ABGR4444) {
            uint16_t val = psp_mem_read<uint16_t>(
                rdram, clut_addr + i * 2);
            clut_rgba[i * 4 + 0] = static_cast<uint8_t>(
                ((val & 0xF) * 255) / 15);
            clut_rgba[i * 4 + 1] = static_cast<uint8_t>(
                (((val >> 4) & 0xF) * 255) / 15);
            clut_rgba[i * 4 + 2] = static_cast<uint8_t>(
                (((val >> 8) & 0xF) * 255) / 15);
            clut_rgba[i * 4 + 3] = static_cast<uint8_t>(
                (((val >> 12) & 0xF) * 255) / 15);
        }
    }
}

/// Decode T8 (CLUT8) texture: each byte is a palette index.
static void decode_clut8(
    const uint8_t* src,
    uint8_t* dst,
    int width, int height,
    const uint8_t clut_rgba[256 * 4]
) {
    int pixel_count = width * height;
    for (int i = 0; i < pixel_count; i++) {
        uint8_t idx = src[i];
        dst[i * 4 + 0] = clut_rgba[idx * 4 + 0];
        dst[i * 4 + 1] = clut_rgba[idx * 4 + 1];
        dst[i * 4 + 2] = clut_rgba[idx * 4 + 2];
        dst[i * 4 + 3] = clut_rgba[idx * 4 + 3];
    }
}

/// Decode T4 (CLUT4) texture: each nibble is a palette index.
static void decode_clut4(
    const uint8_t* src,
    uint8_t* dst,
    int width, int height,
    const uint8_t clut_rgba[256 * 4]
) {
    int pixel_count = width * height;
    for (int i = 0; i < pixel_count; i++) {
        uint8_t byte = src[i / 2];
        uint8_t idx = (i & 1) ? (byte >> 4) : (byte & 0xF);
        dst[i * 4 + 0] = clut_rgba[idx * 4 + 0];
        dst[i * 4 + 1] = clut_rgba[idx * 4 + 1];
        dst[i * 4 + 2] = clut_rgba[idx * 4 + 2];
        dst[i * 4 + 3] = clut_rgba[idx * 4 + 3];
    }
}

// ---- Bytes per pixel for a texture format ----

static int tex_format_bpp(int fmt) {
    switch (fmt) {
    case GE_TFMT_5650:
    case GE_TFMT_5551:
    case GE_TFMT_4444:  return 16;
    case GE_TFMT_8888:  return 32;
    case GE_TFMT_CLUT4: return 4;
    case GE_TFMT_CLUT8: return 8;
    case GE_TFMT_DXT1:  return 4;
    case GE_TFMT_DXT3:
    case GE_TFMT_DXT5:  return 8;
    default:            return 32;
    }
}

// ---- Cache operations ----

/// Find cache entry matching (addr, hash) or evict LRU.
static int find_or_evict(
    uint32_t psp_addr, uint32_t content_hash
) {
    int best_slot = 0;
    uint32_t oldest_frame = UINT32_MAX;

    for (int i = 0; i < TEX_CACHE_SIZE; i++) {
        auto& e = g_tex_cache[i];
        if (e.valid
            && e.psp_addr == psp_addr
            && e.content_hash == content_hash) {
            return i;  // Exact match
        }
        if (!e.valid) {
            return i;  // Empty slot
        }
        if (e.last_frame < oldest_frame) {
            oldest_frame = e.last_frame;
            best_slot = i;
        }
    }

    // Evict oldest
    auto& evict = g_tex_cache[best_slot];
    if (evict.gl_tex != 0) {
        glDeleteTextures(1, &evict.gl_tex);
        evict.gl_tex = 0;
    }
    evict.valid = false;
    return best_slot;
}

// ---- Public API ----

void ge_texture_init() {
    for (auto& e : g_tex_cache) {
        e = TexCacheEntry{};
    }
    g_tex_initialized = true;
    std::fprintf(stderr, "[TEX] Texture cache initialized "
                         "(%d entries)\n", TEX_CACHE_SIZE);
}

void ge_texture_shutdown() {
    for (auto& e : g_tex_cache) {
        if (e.gl_tex != 0) {
            glDeleteTextures(1, &e.gl_tex);
            e.gl_tex = 0;
        }
        e.valid = false;
    }
    g_tex_initialized = false;
    std::fprintf(stderr, "[TEX] Texture cache shutdown\n");
}

void ge_texture_invalidate_all() {
    for (auto& e : g_tex_cache) {
        if (e.gl_tex != 0) {
            glDeleteTextures(1, &e.gl_tex);
            e.gl_tex = 0;
        }
        e.valid = false;
    }
}

GLuint ge_texture_bind(
    uint8_t* rdram,
    uint32_t frame_num
) {
    const GeState& state = ge_get_state();

    if (!state.texture_enable) {
        glBindTexture(GL_TEXTURE_2D, 0);
        return 0;
    }

    // Extract texture parameters
    int log2_w = state.tex_size[0] & 0xFF;
    int log2_h = (state.tex_size[0] >> 8) & 0xFF;
    int width  = 1 << log2_w;
    int height = 1 << log2_h;
    int fmt    = state.tex_format;

    // Compute PSP address
    uint32_t psp_addr = state.tex_addr[0];
    // Combine with TEXBUFWIDTH upper bits for high address
    uint32_t bufw_upper =
        (state.tex_bufw[0] >> 16) & 0xFF;
    psp_addr |= (bufw_upper << 24);
    uint32_t masked_addr = psp_addr & PSP_ADDR_MASK;

    // Compute data size and hash
    int bpp = tex_format_bpp(fmt);
    int data_bytes = (width * height * bpp + 7) / 8;
    int hash_bytes = std::min(data_bytes, 4096);

    if (masked_addr + hash_bytes
        > static_cast<int>(PSP_MEM_SIZE)) {
        glBindTexture(GL_TEXTURE_2D, 0);
        return 0;
    }

    uint32_t content_hash = fnv1a_hash(
        rdram + masked_addr, hash_bytes);

    // Cache lookup
    int slot = find_or_evict(psp_addr, content_hash);
    auto& entry = g_tex_cache[slot];

    if (entry.valid
        && entry.psp_addr == psp_addr
        && entry.content_hash == content_hash) {
        // Cache hit
        entry.last_frame = frame_num;
        glBindTexture(GL_TEXTURE_2D, entry.gl_tex);
        return entry.gl_tex;
    }

    // Cache miss -- decode texture
    bool swizzle = (state.tex_mode & 1) != 0;
    int bufw = state.tex_bufw[0] & 0x7FF;
    if (bufw == 0) bufw = width;
    int width_bytes = (width * bpp + 7) / 8;

    // Read raw texture data from rdram
    std::vector<uint8_t> raw_data(data_bytes);
    if (masked_addr + data_bytes
        <= static_cast<int>(PSP_MEM_SIZE)) {
        std::memcpy(
            raw_data.data(),
            rdram + masked_addr,
            data_bytes);
    }

    // Unswizzle if needed
    std::vector<uint8_t> linear_data;
    const uint8_t* src_data = raw_data.data();
    if (swizzle && width_bytes >= 16 && height >= 8) {
        linear_data.resize(data_bytes);
        unswizzle_texture(
            raw_data.data(), linear_data.data(),
            width_bytes, height);
        src_data = linear_data.data();
    }

    // Convert to RGBA8888
    std::vector<uint8_t> rgba(width * height * 4);

    switch (fmt) {
    case GE_TFMT_8888:
        std::memcpy(rgba.data(), src_data,
                    width * height * 4);
        break;
    case GE_TFMT_5650:
        convert_5650_to_rgba(
            src_data, rgba.data(), width, height);
        break;
    case GE_TFMT_5551:
        convert_5551_to_rgba(
            src_data, rgba.data(), width, height);
        break;
    case GE_TFMT_4444:
        convert_4444_to_rgba(
            src_data, rgba.data(), width, height);
        break;
    case GE_TFMT_CLUT8: {
        uint8_t clut_rgba[256 * 4];
        read_clut_palette(rdram, state, clut_rgba);
        decode_clut8(src_data, rgba.data(),
                     width, height, clut_rgba);
        break;
    }
    case GE_TFMT_CLUT4: {
        uint8_t clut_rgba[256 * 4];
        read_clut_palette(rdram, state, clut_rgba);
        decode_clut4(src_data, rgba.data(),
                     width, height, clut_rgba);
        break;
    }
    default:
        // Unsupported format: magenta fill
        for (int i = 0; i < width * height; i++) {
            rgba[i * 4 + 0] = 0xFF;
            rgba[i * 4 + 1] = 0x00;
            rgba[i * 4 + 2] = 0xFF;
            rgba[i * 4 + 3] = 0xFF;
        }
        break;
    }

    // Upload to GL
    if (entry.gl_tex == 0) {
        glGenTextures(1, &entry.gl_tex);
    }
    glBindTexture(GL_TEXTURE_2D, entry.gl_tex);
    glTexImage2D(
        GL_TEXTURE_2D, 0, GL_RGBA8,
        width, height, 0,
        GL_RGBA, GL_UNSIGNED_BYTE,
        rgba.data());
    glTexParameteri(
        GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
        GL_NEAREST);
    glTexParameteri(
        GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
        GL_NEAREST);
    glTexParameteri(
        GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,
        GL_CLAMP_TO_EDGE);
    glTexParameteri(
        GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,
        GL_CLAMP_TO_EDGE);

    // Store entry
    entry.psp_addr = psp_addr;
    entry.content_hash = content_hash;
    entry.width = width;
    entry.height = height;
    entry.format = fmt;
    entry.last_frame = frame_num;
    entry.valid = true;

    return entry.gl_tex;
}
