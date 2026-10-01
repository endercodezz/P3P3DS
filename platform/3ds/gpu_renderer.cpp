// PICA200 backend for the PSP GE (see gpu_renderer.hpp).
// GE register meanings: PSPSDK psp/pspsdk/src/gu/pspgu.h / pspge.h, as used by
// the software renderer (core/src/ge/software_renderer.cpp), which stays the
// reference for every mapping below. PICA/citro3d behaviour: 3ds/citro3d
// sources (depth range [-1,0]: source/maths/mtx_orthotilt.c; tiled texture
// and framebuffer layout: 8x8 Morton tiles).
#include "gpu_renderer.hpp"

#include "ge_shbin.h"
#include "psprecomp/guest_memory.hpp"

#include <algorithm>
#include <cstring>

namespace p3p3ds::n3ds {
namespace {

constexpr std::uint32_t kTargetSize = 512;          // PSP framebuffers: stride <= 512, 272 rows
constexpr std::uint32_t kMaxTargets = 4;            // 1 MiB of VRAM each
constexpr std::uint32_t kVertexBytes = 1u << 20;    // per GPU frame
constexpr std::uint32_t kTextureBudget = 6u << 20;  // linear memory for decoded textures
constexpr std::uint32_t kMaxDrawsPerFrame = 1500;   // command buffer headroom
// Orientation, measured in Azahar with test bars drawn both by the GPU and by
// CPU tiling: a texture's memory rows run bottom-up (memory row k is sampled
// at t = (h - 1 - k) / h), and NDC y growing with PSP y puts PSP row y where
// t = y / 512 reads it back. So targets need no flip, every CPU upload stores
// PSP row y in memory row h - 1 - y (tiled_row), and the top-screen projection
// keeps image row 0 at the top. Scissor rectangles are in PSP y.
constexpr float kTargetYSign = 1.0f;

constexpr std::uint32_t kTransferFlags =
    GX_TRANSFER_FLIP_VERT(0) | GX_TRANSFER_OUT_TILED(0) | GX_TRANSFER_RAW_COPY(0) |
    GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGBA8) | GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGB8) |
    GX_TRANSFER_SCALING(GX_TRANSFER_SCALE_NO);

std::uint32_t r24(const ge::GeRegisters &g, std::uint32_t i) { return g.reg[i] & 0xFFFFFFu; }
bool on(const ge::GeRegisters &g, std::uint32_t i) { return (g.reg[i] & 1u) != 0u; }

// Memory row holding image row y of a texture h rows tall (see kTargetYSign).
std::uint32_t tiled_row(std::uint32_t y, std::uint32_t h) { return h - 1u - y; }

// Offset of texel (x, y) in a PICA tiled surface of width w (8x8 Morton tiles).
std::uint32_t tiled_index(std::uint32_t x, std::uint32_t y, std::uint32_t w) {
    const std::uint32_t m = (x & 1u) | ((y & 1u) << 1) | ((x & 2u) << 1) | ((y & 2u) << 2) | ((x & 4u) << 2) | ((y & 4u) << 3);
    return ((y >> 3) * (w >> 3) + (x >> 3)) * 64u + m;
}

// RGBA (r in the low byte) -> PICA texel formats.
std::uint32_t to_rgba8(std::uint32_t c) {
    return ((c & 0xFFu) << 24) | (((c >> 8) & 0xFFu) << 16) | (((c >> 16) & 0xFFu) << 8) | (c >> 24);
}
std::uint16_t to_16(std::uint32_t c, GPU_TEXCOLOR f) {
    const std::uint32_t r = c & 0xFFu, g = (c >> 8) & 0xFFu, b = (c >> 16) & 0xFFu, a = c >> 24;
    switch (f) {
    case GPU_RGB565: return static_cast<std::uint16_t>(((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3));
    case GPU_RGBA5551: return static_cast<std::uint16_t>(((r >> 3) << 11) | ((g >> 3) << 6) | ((b >> 3) << 1) | (a >> 7));
    default: return static_cast<std::uint16_t>(((r >> 4) << 12) | ((g >> 4) << 8) | ((b >> 4) << 4) | (a >> 4));
    }
}

GPU_TEXCOLOR pica_format(const ge::TextureInfo &t) {
    const auto f = t.format <= 2 ? t.format : t.format <= 7 ? t.clut_format : 3u;
    return f == 0 ? GPU_RGB565 : f == 1 ? GPU_RGBA5551 : f == 2 ? GPU_RGBA4 : GPU_RGBA8;
}

GPU_BLENDFACTOR blend_factor(std::uint32_t f, bool src_side, std::uint32_t fixed, bool &uses_constant) {
    switch (f) {
    case 0: return src_side ? GPU_DST_COLOR : GPU_SRC_COLOR;
    case 1: return src_side ? GPU_ONE_MINUS_DST_COLOR : GPU_ONE_MINUS_SRC_COLOR;
    case 2: case 6: return GPU_SRC_ALPHA;           // 6-9: doubled alpha, approximated
    case 3: case 7: return GPU_ONE_MINUS_SRC_ALPHA;
    case 4: case 8: return GPU_DST_ALPHA;
    case 5: case 9: return GPU_ONE_MINUS_DST_ALPHA;
    default:
        if (fixed == 0u) return GPU_ZERO;
        if (fixed == 0xFFFFFFu) return GPU_ONE;
        uses_constant = true;
        return GPU_CONSTANT_COLOR;
    }
}

C3D_Mtx target_projection() {
    // (x*w, y*w, z*w, w) in PSP screen pixels -> clip space of a 512x512 target.
    C3D_Mtx m;
    Mtx_Zeros(&m);
    m.r[0].x = 2.0f / kTargetSize; m.r[0].w = -1.0f;
    m.r[1].y = kTargetYSign * 2.0f / kTargetSize; m.r[1].w = -kTargetYSign;
    m.r[2].z = -1.0f / 65535.0f; // depth 0..65535 -> [0,-1]
    m.r[3].w = 1.0f;
    return m;
}

C3D_Mtx screen_projection() {
    // Top screen, 400x240 with y down (rotated framebuffer), z = 0.5 -> -0.5.
    C3D_Mtx m;
    Mtx_OrthoTilt(&m, 0.0f, 400.0f, 240.0f, 0.0f, 0.0f, 1.0f, true);
    return m;
}

} // namespace

GpuRenderer::GpuRenderer() {
    C3D_Init(0x80000);
    top_ = C3D_RenderTargetCreate(240, 400, GPU_RB_RGBA8, -1);
    C3D_RenderTargetSetOutput(top_, GFX_TOP, GFX_LEFT, kTransferFlags);

    dvlb_ = DVLB_ParseFile(reinterpret_cast<u32 *>(const_cast<u8 *>(ge_shbin)), ge_shbin_size);
    shaderProgramInit(&program_);
    shaderProgramSetVsh(&program_, &dvlb_->DVLE[0]);
    C3D_BindProgram(&program_);
    loc_projection_ = shaderInstanceGetUniformLocation(program_.vertexShader, "projection");
    loc_uvscale_ = shaderInstanceGetUniformLocation(program_.vertexShader, "uvscale");

    C3D_AttrInfo *attr = C3D_GetAttrInfo();
    AttrInfo_Init(attr);
    AttrInfo_AddLoader(attr, 0, GPU_FLOAT, 4);         // v0 position (x*w, y*w, z*w, w)
    AttrInfo_AddLoader(attr, 1, GPU_FLOAT, 2);         // v1 texcoord (texels)
    AttrInfo_AddLoader(attr, 2, GPU_UNSIGNED_BYTE, 4); // v2 colour

    vbuf_ = static_cast<Vertex *>(linearAlloc(kVertexBytes));
    C3D_BufInfo *buf = C3D_GetBufInfo();
    BufInfo_Init(buf);
    BufInfo_Add(buf, vbuf_, sizeof(Vertex), 3, 0x210);

    for (int i = 1; i < 6; ++i) C3D_TexEnvInit(C3D_GetTexEnv(i));
    shared_depth_ = vramAlloc(C3D_CalcDepthBufSize(kTargetSize, kTargetSize, GPU_RB_DEPTH24_STENCIL8));
    C3D_TexInit(&fallback_, kTargetSize, kTargetSize, GPU_RGBA8);
    targets_.reserve(kMaxTargets);
}

GpuRenderer::~GpuRenderer() {
    if (in_frame_) C3D_FrameEnd(0);
    for (auto &[key, t] : textures_) C3D_TexDelete(&t.tex);
    for (auto &t : deferred_free_) C3D_TexDelete(&t);
    for (auto &t : targets_) { C3D_RenderTargetDelete(t.rt); C3D_TexDelete(&t.tex); }
    C3D_TexDelete(&fallback_);
    if (shared_depth_) vramFree(shared_depth_);
    linearFree(vbuf_);
    C3D_RenderTargetDelete(top_);
    shaderProgramFree(&program_);
    DVLB_Free(dvlb_);
    C3D_Fini();
}

void GpuRenderer::begin_frame() {
    if (in_frame_) return;
    C3D_FrameBegin(0); // waits until the GPU finished the previous frame
    in_frame_ = true;
    vbuf_used_ = 0;
    bound_ = nullptr;
    bound_screen_ = false;
    for (auto &t : deferred_free_) C3D_TexDelete(&t);
    deferred_free_.clear();
}

void GpuRenderer::flush_frame() {
    // Submit what is queued and start over with an empty vertex buffer and
    // command list (no presentation: the top target is only drawn in present()).
    Target *keep = bound_;
    C3D_FrameEnd(0);
    in_frame_ = false;
    ++gpu_stats_.frame_flushes;
    begin_frame();
    if (keep) bind_target(*keep);
}

GpuRenderer::Vertex *GpuRenderer::alloc_vertices(std::uint32_t count) {
    if ((vbuf_used_ + count) * sizeof(Vertex) > kVertexBytes) flush_frame();
    if (count * sizeof(Vertex) > kVertexBytes) return nullptr;
    Vertex *v = vbuf_ + vbuf_used_;
    vbuf_used_ += count;
    return v;
}

GpuRenderer::Target *GpuRenderer::target_for(std::uint32_t address, std::uint32_t format, bool create) {
    for (auto &t : targets_)
        if (t.address == address) { t.format = format; return &t; }
    if (!create || targets_.size() >= kMaxTargets) return nullptr;
    Target t;
    t.address = address;
    t.format = format;
    if (!C3D_TexInitVRAM(&t.tex, kTargetSize, kTargetSize, GPU_RGBA8)) return nullptr;
    t.rt = C3D_RenderTargetCreateFromTex(&t.tex, GPU_TEXFACE_2D, 0, -1);
    if (t.rt == nullptr) { C3D_TexDelete(&t.tex); return nullptr; }
    C3D_FrameBufDepth(&t.rt->frameBuf, shared_depth_, GPU_RB_DEPTH24_STENCIL8);
    t.cpu_seq = ++seq_; // guest EDRAM holds the current contents until the GE draws
    targets_.push_back(t);
    gpu_stats_.targets = static_cast<std::uint32_t>(targets_.size());
    return &targets_.back();
}

GpuRenderer::Target *GpuRenderer::target_containing(std::uint32_t address) {
    for (auto &t : targets_) {
        const std::uint32_t bytes = kTargetSize * 272u * (t.format == 3 ? 4u : 2u);
        if (address >= t.address && address < t.address + bytes) return &t;
    }
    return nullptr;
}

void GpuRenderer::note_cpu_write(std::uint32_t address, std::size_t) {
    if (Target *t = target_containing(psprecomp::GuestMemory::canonical(address))) t->cpu_seq = ++seq_;
}

void GpuRenderer::bind_target(Target &t) {
    C3D_FrameDrawOn(t.rt);
    const C3D_Mtx proj = target_projection();
    C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, loc_projection_, &proj);
    bound_ = &t;
    bound_screen_ = false;
}

void GpuRenderer::upload_guest_framebuffer(psprecomp::GuestMemory &memory, std::uint32_t address, std::uint32_t stride,
                                           std::uint32_t format) {
    // The GPU reads fallback_ only when the frame is submitted: a second upload
    // in the same frame must submit the first use before overwriting it.
    if (in_frame_ && fallback_frame_ == frame_) flush_frame();
    fallback_frame_ = frame_;
    const std::uint32_t bpp = format == 3 ? 4u : 2u;
    auto *dst = static_cast<std::uint32_t *>(fallback_.data);
    const std::uint8_t *src = memory.raw_pointer(address, stride * 272u * bpp);
    for (std::uint32_t y = 0; y < 272u; ++y)
        for (std::uint32_t x = 0; x < 480u; ++x) {
            std::uint32_t rgba = 0xFF000000u;
            if (src != nullptr) {
                const std::uint8_t *p = src + (y * stride + x) * bpp;
                if (bpp == 4u) rgba = p[0] | (p[1] << 8) | (p[2] << 16) | 0xFF000000u;
                else {
                    const auto c = ge::unpack16(p[0] | (p[1] << 8), format);
                    rgba = c[0] | (c[1] << 8) | (c[2] << 16) | 0xFF000000u;
                }
            }
            dst[tiled_index(x, tiled_row(y, kTargetSize), kTargetSize)] = to_rgba8(rgba);
        }
    C3D_TexFlush(&fallback_);
}

void GpuRenderer::blit(C3D_Tex &source, float src_w, float src_h, bool to_screen) {
    Vertex *v = alloc_vertices(6);
    if (v == nullptr) return;
    const float w = to_screen ? 400.0f : src_w, h = to_screen ? 240.0f : src_h;
    const float z = to_screen ? 0.5f : 0.0f;
    const Vertex q[4] = {{0, 0, z, 1, 0, 0, 255, 255, 255, 255}, {w, 0, z, 1, src_w, 0, 255, 255, 255, 255},
                         {0, h, z, 1, 0, src_h, 255, 255, 255, 255}, {w, h, z, 1, src_w, src_h, 255, 255, 255, 255}};
    v[0] = q[0]; v[1] = q[1]; v[2] = q[2]; v[3] = q[2]; v[4] = q[1]; v[5] = q[3];
    C3D_TexEnv *env = C3D_GetTexEnv(0);
    C3D_TexEnvInit(env);
    C3D_TexEnvSrc(env, C3D_Both, GPU_TEXTURE0);
    C3D_TexEnvFunc(env, C3D_Both, GPU_REPLACE);
    C3D_AlphaTest(false, GPU_ALWAYS, 0);
    C3D_AlphaBlend(GPU_BLEND_ADD, GPU_BLEND_ADD, GPU_ONE, GPU_ZERO, GPU_ONE, GPU_ZERO);
    C3D_DepthTest(false, GPU_ALWAYS, GPU_WRITE_COLOR);
    C3D_CullFace(GPU_CULL_NONE);
    C3D_SetScissor(GPU_SCISSOR_DISABLE, 0, 0, 0, 0);
    C3D_TexSetFilter(&source, to_screen ? GPU_LINEAR : GPU_NEAREST, to_screen ? GPU_LINEAR : GPU_NEAREST);
    C3D_TexSetWrap(&source, GPU_CLAMP_TO_EDGE, GPU_CLAMP_TO_EDGE);
    C3D_TexBind(0, &source);
    C3D_FVUnifSet(GPU_VERTEX_SHADER, loc_uvscale_, 1.0f / kTargetSize, 1.0f / kTargetSize, 0.0f, 0.0f);
    C3D_DrawArrays(GPU_TRIANGLES, static_cast<int>(v - vbuf_), 6);
}

void GpuRenderer::evict_textures(std::uint32_t needed) {
    while (texture_bytes_ + needed > kTextureBudget) {
        auto oldest = textures_.end();
        for (auto it = textures_.begin(); it != textures_.end(); ++it)
            if (it->second.last_frame < frame_ && (oldest == textures_.end() || it->second.last_frame < oldest->second.last_frame))
                oldest = it;
        if (oldest == textures_.end()) return; // everything is in use by this frame
        texture_bytes_ -= oldest->second.bytes;
        C3D_TexDelete(&oldest->second.tex);
        textures_.erase(oldest);
    }
}

const C3D_Tex *GpuRenderer::bind_texture(psprecomp::GuestMemory &memory, const ge::GeRegisters &regs, float &scale_u, float &scale_v) {
    const auto info = ge::texture_info(regs);
    const auto filter_reg = r24(regs, 0xC6), wrap_reg = r24(regs, 0xC7);
    const GPU_TEXTURE_FILTER_PARAM mag = (filter_reg >> 8) & 1u ? GPU_LINEAR : GPU_NEAREST;
    const GPU_TEXTURE_FILTER_PARAM min = filter_reg & 1u ? GPU_LINEAR : GPU_NEAREST;
    const GPU_TEXTURE_WRAP_PARAM wrap_u = wrap_reg & 1u ? GPU_CLAMP_TO_EDGE : GPU_REPEAT;
    const GPU_TEXTURE_WRAP_PARAM wrap_v = (wrap_reg >> 8) & 1u ? GPU_CLAMP_TO_EDGE : GPU_REPEAT;

    // Render-to-texture: a texture that starts at a GPU-drawn framebuffer.
    const std::uint32_t canonical = psprecomp::GuestMemory::canonical(info.address);
    if ((canonical & 0x0F000000u) == 0x04000000u) {
        for (auto &t : targets_)
            if (t.address == canonical && t.gpu_seq > t.cpu_seq) {
                scale_u = scale_v = 1.0f / kTargetSize;
                C3D_TexSetFilter(&t.tex, mag, min);
                C3D_TexSetWrap(&t.tex, wrap_u, wrap_v);
                ++gpu_stats_.target_textures;
                return &t.tex;
            }
    }

    // Cache key: texture parameters plus, for CLUT formats, the palette.
    std::uint64_t key = (static_cast<std::uint64_t>(info.address) << 32) ^
                        (static_cast<std::uint64_t>(info.format) << 4 | info.clut_format | (info.swizzled ? 0x100u : 0u)) ^
                        (static_cast<std::uint64_t>(info.width) << 12) ^ (static_cast<std::uint64_t>(info.height) << 20) ^
                        (static_cast<std::uint64_t>(info.stride) << 40);
    if (info.format >= 4) {
        key ^= static_cast<std::uint64_t>(r24(regs, 0xC5)) * 0x9E3779B97F4A7C15ull;
        for (std::uint32_t i = 0; i < regs.clut_words; ++i) key = (key ^ regs.clut[i]) * 0x100000001B3ull;
    }
    auto it = textures_.find(key);
    if (it != textures_.end() && it->second.checked_frame == frame_) {
        ++gpu_stats_.texture_hits;
    } else {
        const std::uint64_t hash = ge::texture_hash(memory, regs, info);
        if (it != textures_.end() && it->second.hash == hash) {
            it->second.checked_frame = frame_;
            ++gpu_stats_.texture_hits;
        } else {
            if (it != textures_.end()) { // contents changed: never rewrite a texture the GPU may still read
                texture_bytes_ -= it->second.bytes;
                if (it->second.last_frame == frame_) deferred_free_.push_back(it->second.tex);
                else C3D_TexDelete(&it->second.tex);
                textures_.erase(it);
            }
            const auto fmt = pica_format(info);
            const std::uint32_t tw = std::max<std::uint32_t>(info.width, 8u), th = std::max<std::uint32_t>(info.height, 8u);
            const std::uint32_t bytes = tw * th * (fmt == GPU_RGBA8 ? 4u : 2u);
            evict_textures(bytes);
            CachedTexture entry;
            if (!C3D_TexInit(&entry.tex, static_cast<u16>(tw), static_cast<u16>(th), fmt)) {
                evict_textures(kTextureBudget); // free everything not used by this frame and retry
                if (!C3D_TexInit(&entry.tex, static_cast<u16>(tw), static_cast<u16>(th), fmt)) return nullptr;
            }
            ge::decode_texture(memory, regs, info, rgba_);
            for (std::uint32_t y = 0; y < th; ++y)
                for (std::uint32_t x = 0; x < tw; ++x) {
                    const std::uint32_t c = rgba_[static_cast<std::size_t>(y % info.height) * info.width + (x % info.width)];
                    const std::uint32_t at = tiled_index(x, tiled_row(y, th), tw);
                    if (fmt == GPU_RGBA8) static_cast<std::uint32_t *>(entry.tex.data)[at] = to_rgba8(c);
                    else static_cast<std::uint16_t *>(entry.tex.data)[at] = to_16(c, fmt);
                }
            C3D_TexFlush(&entry.tex);
            entry.hash = hash;
            entry.width = tw;
            entry.height = th;
            entry.bytes = bytes;
            entry.checked_frame = frame_;
            texture_bytes_ += bytes;
            ++gpu_stats_.texture_uploads;
            gpu_stats_.texture_bytes += bytes;
            it = textures_.emplace(key, entry).first;
        }
    }
    it->second.last_frame = frame_;
    gpu_stats_.textures = static_cast<std::uint32_t>(textures_.size());
    scale_u = 1.0f / static_cast<float>(it->second.width);
    scale_v = 1.0f / static_cast<float>(it->second.height);
    C3D_TexSetFilter(&it->second.tex, mag, min);
    C3D_TexSetWrap(&it->second.tex, wrap_u, wrap_v);
    return &it->second.tex;
}

void GpuRenderer::apply_fragment_state(const ge::GeRegisters &regs, bool textured, bool clear_mode) {
    C3D_TexEnv *env = C3D_GetTexEnv(0);
    C3D_TexEnvInit(env);
    if (clear_mode) {
        // Clear mode: vertex colour/depth, masks from CLEARMODE bits (0x100
        // colour, 0x200 alpha/stencil, 0x400 depth); no tests or blending.
        const auto clear = r24(regs, 0xD3);
        C3D_TexEnvSrc(env, C3D_Both, GPU_PRIMARY_COLOR);
        C3D_TexEnvFunc(env, C3D_Both, GPU_REPLACE);
        int mask = 0;
        if (clear & 0x100u) mask |= GPU_WRITE_RED | GPU_WRITE_GREEN | GPU_WRITE_BLUE;
        if (clear & 0x200u) mask |= GPU_WRITE_ALPHA;
        if (clear & 0x400u) mask |= GPU_WRITE_DEPTH;
        C3D_AlphaTest(false, GPU_ALWAYS, 0);
        C3D_AlphaBlend(GPU_BLEND_ADD, GPU_BLEND_ADD, GPU_ONE, GPU_ZERO, GPU_ONE, GPU_ZERO);
        C3D_DepthTest(true, GPU_ALWAYS, static_cast<GPU_WRITEMASK>(mask));
        C3D_CullFace(GPU_CULL_NONE);
        return;
    }

    if (!textured) {
        C3D_TexEnvSrc(env, C3D_Both, GPU_PRIMARY_COLOR);
        C3D_TexEnvFunc(env, C3D_Both, GPU_REPLACE);
    } else {
        // Texture function (TFUNC 0xC9): modulate, decal, blend, replace, add;
        // bit 8 = use texture alpha, bit 16 = colour doubling.
        const auto func = r24(regs, 0xC9);
        const bool use_alpha = (func & 0x100u) != 0u;
        switch (func & 7u) {
        case 0:
            C3D_TexEnvSrc(env, C3D_RGB, GPU_TEXTURE0, GPU_PRIMARY_COLOR);
            C3D_TexEnvFunc(env, C3D_RGB, GPU_MODULATE);
            break;
        case 1:
            C3D_TexEnvSrc(env, C3D_RGB, GPU_TEXTURE0, GPU_PRIMARY_COLOR, GPU_TEXTURE0);
            C3D_TexEnvOpRgb(env, GPU_TEVOP_RGB_SRC_COLOR, GPU_TEVOP_RGB_SRC_COLOR, GPU_TEVOP_RGB_SRC_ALPHA);
            C3D_TexEnvFunc(env, C3D_RGB, GPU_INTERPOLATE);
            break;
        case 2:
            C3D_TexEnvSrc(env, C3D_RGB, GPU_CONSTANT, GPU_PRIMARY_COLOR, GPU_TEXTURE0);
            C3D_TexEnvFunc(env, C3D_RGB, GPU_INTERPOLATE);
            C3D_TexEnvColor(env, r24(regs, 0xCA) | 0xFF000000u);
            break;
        case 3:
            C3D_TexEnvSrc(env, C3D_RGB, GPU_TEXTURE0);
            C3D_TexEnvFunc(env, C3D_RGB, GPU_REPLACE);
            break;
        default:
            C3D_TexEnvSrc(env, C3D_RGB, GPU_TEXTURE0, GPU_PRIMARY_COLOR);
            C3D_TexEnvFunc(env, C3D_RGB, GPU_ADD);
            break;
        }
        if ((func & 7u) == 1u || !use_alpha) {
            C3D_TexEnvSrc(env, C3D_Alpha, GPU_PRIMARY_COLOR);
            C3D_TexEnvFunc(env, C3D_Alpha, GPU_REPLACE);
        } else if ((func & 7u) == 3u) {
            C3D_TexEnvSrc(env, C3D_Alpha, GPU_TEXTURE0);
            C3D_TexEnvFunc(env, C3D_Alpha, GPU_REPLACE);
        } else {
            C3D_TexEnvSrc(env, C3D_Alpha, GPU_TEXTURE0, GPU_PRIMARY_COLOR);
            C3D_TexEnvFunc(env, C3D_Alpha, GPU_MODULATE);
        }
        if (func & 0x10000u) C3D_TexEnvScale(env, C3D_RGB, GPU_TEVSCALE_2);
    }

    if (on(regs, 0x22)) {
        const auto at = r24(regs, 0xDB); // func, reference; the mask (bits 16-23) is not modelled
        C3D_AlphaTest(true, static_cast<GPU_TESTFUNC>(at & 7u), static_cast<int>((at >> 8) & 0xFFu));
    } else {
        C3D_AlphaTest(false, GPU_ALWAYS, 0);
    }

    if (on(regs, 0x21)) {
        const auto mode = r24(regs, 0xDF);
        bool constant = false;
        const auto fix_a = r24(regs, 0xE0), fix_b = r24(regs, 0xE1);
        const auto src = blend_factor(mode & 0xFu, true, fix_a, constant);
        bool constant_b = false;
        const auto dst = blend_factor((mode >> 4) & 0xFu, false, fix_b, constant_b);
        // One PICA blend constant: when both sides need one, side A wins ([INFERRED] approximation).
        if (constant) C3D_BlendingColor(fix_a | 0xFF000000u);
        else if (constant_b) C3D_BlendingColor(fix_b | 0xFF000000u);
        GPU_BLENDEQUATION eq;
        switch ((mode >> 8) & 0xFu) {
        case 1: eq = GPU_BLEND_SUBTRACT; break;
        case 2: eq = GPU_BLEND_REVERSE_SUBTRACT; break;
        case 3: eq = GPU_BLEND_MIN; break;
        case 4: eq = GPU_BLEND_MAX; break;
        default: eq = GPU_BLEND_ADD; break; // 5 (absolute difference) approximated
        }
        C3D_AlphaBlend(eq, eq, src, dst, src, dst);
    } else {
        C3D_AlphaBlend(GPU_BLEND_ADD, GPU_BLEND_ADD, GPU_ONE, GPU_ZERO, GPU_ONE, GPU_ZERO);
    }

    // Colour mask (0xE8: set bits keep the old value): per channel only. The
    // alpha channel holds stencil on the PSP and is left unchanged, as in the
    // software renderer.
    const auto keep = r24(regs, 0xE8);
    int mask = 0;
    if ((keep & 0xFFu) != 0xFFu) mask |= GPU_WRITE_RED;
    if (((keep >> 8) & 0xFFu) != 0xFFu) mask |= GPU_WRITE_GREEN;
    if (((keep >> 16) & 0xFFu) != 0xFFu) mask |= GPU_WRITE_BLUE;
    const bool depth_test = on(regs, 0x23);
    if (depth_test && (r24(regs, 0xE7) & 1u) == 0u) mask |= GPU_WRITE_DEPTH;
    C3D_DepthTest(depth_test, depth_test ? static_cast<GPU_TESTFUNC>(r24(regs, 0xDE) & 7u) : GPU_ALWAYS,
                  static_cast<GPU_WRITEMASK>(mask));

    // Culling (0x1D): CULL bit 0 selects the culled winding; see the software
    // renderer. [UNVERIFIED] mapping to PICA winding, checked on 3D scenes.
    // Measured in Azahar: this mapping draws the logos and title screen; the
    // opposite one culled them all. [UNVERIFIED] on 3D scenes.
    if (on(regs, 0x1D)) C3D_CullFace((r24(regs, 0x9B) & 1u) ? GPU_CULL_FRONT_CCW : GPU_CULL_BACK_CCW);
    else C3D_CullFace(GPU_CULL_NONE);
}

void GpuRenderer::draw(psprecomp::GuestMemory &memory, const ge::GeRegisters &regs, ge::Prim prim, std::uint32_t count,
                       std::uint32_t vertex_address, std::uint32_t index_address) {
    ++draw_stats_.prims;
    draw_stats_.vertices += count;
    if (prim == ge::Prim::Points || prim == ge::Prim::Lines || prim == ge::Prim::LineStrip) { ++gpu_stats_.skipped_prims; return; }
    const std::uint32_t color_address = 0x04000000u | (r24(regs, 0x9C) & 0x1FFFF0u);
    const std::uint32_t stride = r24(regs, 0x9D) & 0x7FCu;
    if (stride == 0u) return;
    begin_frame();
    if (draw_stats_.prims % kMaxDrawsPerFrame == 0u) flush_frame();
    Target *target = target_for(color_address, r24(regs, 0xD2) & 3u, true);
    if (target == nullptr) { ++gpu_stats_.skipped_prims; return; }
    if (bound_ != target || bound_screen_) bind_target(*target);
    if (target->cpu_seq > target->gpu_seq) {
        // The guest wrote this framebuffer since the GPU last drew it: start from its contents.
        upload_guest_framebuffer(memory, color_address, stride, target->format);
        fallback_address_ = color_address;
        fallback_seq_ = target->cpu_seq;
        blit(fallback_, 480.0f, 272.0f, false);
        ++gpu_stats_.cpu_to_target;
    }
    target->gpu_seq = ++seq_;

    const auto layout = ge::vertex_layout(r24(regs, 0x12));
    const bool clear_mode = (r24(regs, 0xD3) & 1u) != 0u;
    if (clear_mode) ++draw_stats_.clears;
    const bool textured = ge::draw_is_textured(regs, layout);
    ge::decode_screen_vertices(memory, regs, count, vertex_address, index_address, screen_);

    float su = 1.0f, sv = 1.0f;
    const C3D_Tex *tex = textured ? bind_texture(memory, regs, su, sv) : nullptr;
    apply_fragment_state(regs, tex != nullptr, clear_mode);
    if (tex != nullptr) C3D_TexBind(0, const_cast<C3D_Tex *>(tex));
    C3D_FVUnifSet(GPU_VERTEX_SHADER, loc_uvscale_, su, sv, 0.0f, 0.0f);
    {
        const auto x1 = r24(regs, 0xD4) & 0x3FFu, y1 = (r24(regs, 0xD4) >> 10) & 0x3FFu;
        const auto x2 = r24(regs, 0xD5) & 0x3FFu, y2 = (r24(regs, 0xD5) >> 10) & 0x3FFu;
        if (kTargetYSign > 0.0f) C3D_SetScissor(GPU_SCISSOR_NORMAL, x1, y1, x2 + 1u, y2 + 1u);
        else C3D_SetScissor(GPU_SCISSOR_NORMAL, x1, kTargetSize - 1u - y2, x2 + 1u, kTargetSize - y1);
    }

    const bool flat = (r24(regs, 0x50) & 1u) == 0u;
    std::uint32_t emitted = 0;
    Vertex *out = nullptr;
    auto put = [&](const ge::ScreenVertex &s, float x, float y, float z, float u, float v, const ge::Rgba &c) {
        *out++ = Vertex{x * s.w, y * s.w, z * s.w, s.w, u, v, c[0], c[1], c[2], c[3]};
        ++emitted;
    };
    auto triangle = [&](const ge::ScreenVertex &a, const ge::ScreenVertex &b, const ge::ScreenVertex &d) {
        if (a.clipped || b.clipped || d.clipped) return;
        const auto &ca = flat ? d.color : a.color, &cb = flat ? d.color : b.color;
        put(a, a.x, a.y, a.z, a.u, a.v, ca);
        put(b, b.x, b.y, b.z, b.u, b.v, cb);
        put(d, d.x, d.y, d.z, d.u, d.v, d.color);
    };

    std::uint32_t capacity = 0;
    switch (prim) {
    case ge::Prim::Sprites: capacity = count / 2u * 6u; break;
    case ge::Prim::Triangles: capacity = count / 3u * 3u; break;
    default: capacity = count >= 3u ? (count - 2u) * 3u : 0u; break;
    }
    if (capacity == 0u) return;
    out = alloc_vertices(capacity);
    if (out == nullptr) { ++gpu_stats_.skipped_prims; return; }
    Vertex *first = out;
    const auto &v = screen_;
    switch (prim) {
    case ge::Prim::Sprites:
        C3D_CullFace(GPU_CULL_NONE);
        for (std::uint32_t i = 0; i + 1 < count; i += 2) {
            const auto &a = v[i], &b = v[i + 1];
            // Corners keep the software mapping: u follows x and v follows y from a to b.
            ge::ScreenVertex s = b;
            s.w = 1.0f;
            put(s, a.x, a.y, b.z, a.u, a.v, b.color);
            put(s, b.x, a.y, b.z, b.u, a.v, b.color);
            put(s, a.x, b.y, b.z, a.u, b.v, b.color);
            put(s, a.x, b.y, b.z, a.u, b.v, b.color);
            put(s, b.x, a.y, b.z, b.u, a.v, b.color);
            put(s, b.x, b.y, b.z, b.u, b.v, b.color);
        }
        break;
    case ge::Prim::Triangles:
        for (std::uint32_t i = 0; i + 2 < count; i += 3) triangle(v[i], v[i + 1], v[i + 2]);
        break;
    case ge::Prim::TriangleStrip:
        for (std::uint32_t i = 0; i + 2 < count; ++i)
            (i & 1u) ? triangle(v[i + 1], v[i], v[i + 2]) : triangle(v[i], v[i + 1], v[i + 2]);
        break;
    default: // TriangleFan
        for (std::uint32_t i = 1; i + 1 < count; ++i) triangle(v[0], v[i], v[i + 1]);
        break;
    }
    vbuf_used_ -= capacity - emitted; // return what clipped triangles did not use
    if (emitted == 0u) return;
    C3D_DrawArrays(GPU_TRIANGLES, static_cast<int>(first - vbuf_), static_cast<int>(emitted));
    ++gpu_stats_.draws;
    gpu_stats_.triangles += emitted / 3u;
}

void GpuRenderer::transfer(psprecomp::GuestMemory &memory, const ge::GeRegisters &regs) {
    // Block transfers copy guest memory exactly like the reference renderer; a
    // destination inside a GPU framebuffer then holds newer data than the GPU copy.
    ge::SoftwareRenderer copy;
    copy.transfer(memory, regs);
    ++draw_stats_.transfers;
    const auto dst = (r24(regs, 0xB4) & 0xFFFFF0u) | ((r24(regs, 0xB5) >> 16) & 0xFFu) << 24;
    note_cpu_write(dst, 1);
}

void GpuRenderer::present(psprecomp::GuestMemory &memory, const hle::DisplayFramebufInfo &fb) {
    begin_frame();
    C3D_Tex *source = nullptr;
    last_present_cpu_ = false;
    if (fb.active && fb.topaddr != 0u) {
        const auto address = psprecomp::GuestMemory::canonical(fb.topaddr);
        const auto format = static_cast<std::uint32_t>(fb.pixelformat);
        Target *t = target_for(address, format, false);
        if (t != nullptr && t->gpu_seq > t->cpu_seq) {
            source = &t->tex;
            ++gpu_stats_.gpu_presents;
        } else {
            // Movie frames and other CPU-written pictures: convert only when changed.
            const std::uint64_t version = t != nullptr ? t->cpu_seq : ++seq_;
            if (fallback_address_ != address || fallback_seq_ != version) {
                upload_guest_framebuffer(memory, address, static_cast<std::uint32_t>(fb.bufferwidth), format);
                fallback_address_ = address;
                fallback_seq_ = version;
            }
            source = &fallback_;
            last_present_cpu_ = true;
            ++gpu_stats_.cpu_presents;
        }
    }
    C3D_FrameDrawOn(top_);
    bound_ = nullptr;
    bound_screen_ = true;
    const C3D_Mtx proj = screen_projection();
    C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, loc_projection_, &proj);
    if (source == nullptr) C3D_RenderTargetClear(top_, C3D_CLEAR_ALL, 0x000000FFu, 0);
    else blit(*source, 480.0f, 272.0f, true);
    C3D_FrameEnd(0);
    in_frame_ = false;
    ++frame_;
}

bool GpuRenderer::read_top_screen(std::vector<std::uint8_t> &rgb) {
    if (in_frame_) return false;
    auto *buffer = static_cast<std::uint32_t *>(linearAlloc(240u * 400u * 4u));
    if (buffer == nullptr) return false;
    const std::uint32_t flags = GX_TRANSFER_FLIP_VERT(0) | GX_TRANSFER_OUT_TILED(0) | GX_TRANSFER_RAW_COPY(0) |
                                GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGBA8) | GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGBA8) |
                                GX_TRANSFER_SCALING(GX_TRANSFER_SCALE_NO);
    C3D_SyncDisplayTransfer(static_cast<u32 *>(top_->frameBuf.colorBuf), GX_BUFFER_DIM(240, 400), reinterpret_cast<u32 *>(buffer),
                            GX_BUFFER_DIM(240, 400), flags);
    GSPGPU_InvalidateDataCache(buffer, 240u * 400u * 4u);
    rgb.resize(400u * 240u * 3u);
    // Same layout as the LCD framebuffer: column sx of the screen is row sx, bottom pixel first.
    for (std::uint32_t sy = 0; sy < 240u; ++sy)
        for (std::uint32_t sx = 0; sx < 400u; ++sx) {
            const std::uint32_t c = buffer[sx * 240u + (239u - sy)]; // 0xRRGGBBAA
            auto *p = &rgb[(static_cast<std::size_t>(sy) * 400u + sx) * 3u];
            p[0] = static_cast<std::uint8_t>(c >> 24); p[1] = static_cast<std::uint8_t>(c >> 16); p[2] = static_cast<std::uint8_t>(c >> 8);
        }
    linearFree(buffer);
    return true;
}

} // namespace p3p3ds::n3ds
