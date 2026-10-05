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
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <cstring>

namespace p3p3ds::n3ds {
namespace {

constexpr std::uint32_t kTargetSize = 512;          // PSP framebuffers: stride <= 512, 272 rows
constexpr std::uint32_t kMaxTargets = 4;            // 1 MiB of VRAM each
constexpr std::uint32_t kVertexBytes = 1u << 20;    // vertices and indices per GPU frame
static_assert(kVertexBytes / 28u < 0x10000u, "u16 indices count vertices from the arena start");
constexpr std::uint32_t kTextureBudget = 6u << 20;  // linear memory for decoded textures
constexpr std::uint32_t kCommandBufferBytes = 0xC0000; // C3D_Init
// Submit the queued frame when the citro3d command buffer (kCommandBufferBytes)
// is this full: GPUCMD_Add panics (svcBreak) on overflow. One draw adds a few
// KiB at most. Measured on hardware: a frame left open across vblanks (no new
// picture to present) overflowed at the first Shadow with a draw-count rule,
// and again with C3D_GetCmdBufUsage(), which is only updated when a frame is
// submitted (3ds/citro3d/source/base.c). The live fill comes from libctru.
constexpr float kCmdBufFlushUsage = 0.75f;

float command_buffer_fill() {
    u32 *buffer = nullptr;
    u32 size = 0, offset = 0;
    GPUCMD_GetBuffer(&buffer, &size, &offset);
    return size != 0u ? static_cast<float>(offset) / static_cast<float>(size) : 0.0f;
}
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

// Row-major 4x4 product a * b.
C3D_Mtx multiply(const C3D_Mtx &a, const C3D_Mtx &b) {
    C3D_Mtx m;
    Mtx_Multiply(&m, &a, &b);
    return m;
}

// Rows of a GE 4x3 matrix (four columns of three floats: x' = x*m0 + y*m3 +
// z*m6 + m9) as a 4x4 matrix with the row (0, 0, 0, 1).
C3D_Mtx ge_matrix43(const float *m) {
    C3D_Mtx r;
    Mtx_Zeros(&r);
    for (int j = 0; j < 3; ++j) {
        r.r[j].x = m[j]; r.r[j].y = m[3 + j]; r.r[j].z = m[6 + j]; r.r[j].w = m[9 + j];
    }
    r.r[3].w = 1.0f;
    return r;
}

C3D_Mtx screen_projection() {
    // Top screen, 400x240 with y down (rotated framebuffer), z = 0.5 -> -0.5.
    C3D_Mtx m;
    Mtx_OrthoTilt(&m, 0.0f, 400.0f, 240.0f, 0.0f, 0.0f, 1.0f, true);
    return m;
}

} // namespace

GpuRenderer::GpuRenderer() {
    C3D_Init(kCommandBufferBytes);
    top_ = C3D_RenderTargetCreate(240, 400, GPU_RB_RGBA8, -1);
    C3D_RenderTargetSetOutput(top_, GFX_TOP, GFX_LEFT, kTransferFlags);

    dvlb_ = DVLB_ParseFile(reinterpret_cast<u32 *>(const_cast<u8 *>(ge_shbin)), ge_shbin_size);
    shaderProgramInit(&program_);
    shaderProgramSetVsh(&program_, &dvlb_->DVLE[0]);
    C3D_BindProgram(&program_);
    auto loc = [this](const char *name) { return shaderInstanceGetUniformLocation(program_.vertexShader, name); };
    u_.screen = loc("screen"); u_.world = loc("world"); u_.bones = loc("bones"); u_.uvxf = loc("uvxf");
    u_.emissive = loc("emissive"); u_.matamb = loc("matamb"); u_.matdif = loc("matdif"); u_.matspe = loc("matspe");
    u_.sceneamb = loc("sceneamb"); u_.matsel = loc("matsel"); u_.misc = loc("misc");
    u_.lpos = loc("lpos"); u_.ldir = loc("ldir"); u_.latt = loc("latt"); u_.lspot = loc("lspot");
    u_.lamb = loc("lamb"); u_.ldif = loc("ldif"); u_.lspe = loc("lspe");
    u_.skin = loc("skin"); u_.light = loc("light");
    for (int i = 0; i < 4; ++i) {
        char name[4] = {'l', static_cast<char>('0' + i), 0, 0};
        u_.lights[i] = loc(name);
    }
    for (auto &b : bool_cache_) b = -1;

    // Attribute layouts (shader inputs v0..v5). Inputs a layout lacks are
    // fixed zero attributes; the shader only reads them when skinning or
    // lighting is on, which only the model layouts do.
    {
        C3D_AttrInfo &a = attr_[0]; // screen
        AttrInfo_Init(&a);
        AttrInfo_AddLoader(&a, 0, GPU_FLOAT, 4);         // v0 (x*w, y*w, z*w, w)
        AttrInfo_AddLoader(&a, 2, GPU_FLOAT, 2);         // v2 texels
        AttrInfo_AddLoader(&a, 3, GPU_UNSIGNED_BYTE, 4); // v3 colour
        AttrInfo_AddFixed(&a, 1);
        AttrInfo_AddFixed(&a, 4);
        AttrInfo_AddFixed(&a, 5);
    }
    for (int k = 1; k < 3; ++k) {
        C3D_AttrInfo &a = attr_[k]; // model, skinned model
        AttrInfo_Init(&a);
        AttrInfo_AddLoader(&a, 0, GPU_FLOAT, 3);         // v0 position (w = 1)
        AttrInfo_AddLoader(&a, 1, GPU_FLOAT, 3);         // v1 normal
        AttrInfo_AddLoader(&a, 2, GPU_FLOAT, 2);         // v2 UV
        AttrInfo_AddLoader(&a, 3, GPU_UNSIGNED_BYTE, 4); // v3 colour
        if (k == 2) {
            AttrInfo_AddLoader(&a, 4, GPU_FLOAT, 4);     // v4 weights 0-3
            AttrInfo_AddLoader(&a, 5, GPU_FLOAT, 4);     // v5 weights 4-7
        } else {
            AttrInfo_AddFixed(&a, 4);
            AttrInfo_AddFixed(&a, 5);
        }
    }
    for (int reg : {1, 4, 5}) {
        C3D_FVec *f = C3D_FixedAttribGetWritePtr(reg);
        f->x = f->y = f->z = f->w = 0.0f;
    }

    arena_ = static_cast<std::uint8_t *>(linearAlloc(kVertexBytes));
    if (FILE *f = std::fopen("sdmc:/p3p3ds/cpu_vertices.txt", "r")) { cpu_vertices_ = true; std::fclose(f); }

    for (int i = 1; i < 6; ++i) C3D_TexEnvInit(C3D_GetTexEnv(i));
    shared_depth_ = vramAlloc(C3D_CalcDepthBufSize(kTargetSize, kTargetSize, GPU_RB_DEPTH24_STENCIL8));
    C3D_TexInit(&fallback_, kTargetSize, kTargetSize, GPU_RGBA8);
    targets_.reserve(kMaxTargets);
}

GpuRenderer::~GpuRenderer() {
    if (in_frame_) end_frame();
    for (auto &[key, t] : textures_) C3D_TexDelete(&t.tex);
    for (auto &t : deferred_free_) C3D_TexDelete(&t);
    for (auto &t : targets_) { C3D_RenderTargetDelete(t.rt); C3D_TexDelete(&t.tex); }
    C3D_TexDelete(&fallback_);
    if (shared_depth_) vramFree(shared_depth_);
    linearFree(arena_);
    C3D_RenderTargetDelete(top_);
    shaderProgramFree(&program_);
    DVLB_Free(dvlb_);
    C3D_Fini();
}

void GpuRenderer::begin_frame() {
    if (in_frame_) return;
    const u64 start = svcGetSystemTick();
    C3D_FrameBegin(0); // waits until the GPU finished the previous frame
    gpu_stats_.wait_ticks += svcGetSystemTick() - start;
    in_frame_ = true;
    arena_used_ = 0;
    last_alloc_ = 0;
    bound_ = nullptr;
    bound_screen_ = false;
    for (auto &t : deferred_free_) C3D_TexDelete(&t);
    deferred_free_.clear();
}

// C3D_FrameEnd(0) flushes the data cache over the whole 12 MiB linear heap on
// every call (3ds/citro3d/source/renderqueue.c). Textures are flushed when
// they are written (C3D_TexFlush), so only this frame's vertices need it;
// GX_CMDLIST_FLUSH makes citro3d flush just the command list.
void GpuRenderer::end_frame() {
    if (arena_used_ != 0u) GSPGPU_FlushDataCache(arena_, arena_used_);
    C3D_FrameEnd(GX_CMDLIST_FLUSH);
    in_frame_ = false;
}

void GpuRenderer::flush_frame() {
    // Submit what is queued and start over with an empty vertex buffer and
    // command list (no presentation: the top target is only drawn in present()).
    Target *keep = bound_;
    end_frame();
    ++gpu_stats_.frame_flushes;
    begin_frame();
    if (keep) bind_target(*keep);
}

// Vertices and indices of the open GPU frame, at an offset that is a multiple
// of align (a vertex stride: the draw then addresses its vertices by index
// from arena_); a full arena submits the frame first (the GPU must finish
// reading it before reuse).
void *GpuRenderer::alloc_linear(std::uint32_t bytes, std::uint32_t align) {
    bytes = (bytes + 3u) & ~3u;
    if (bytes > kVertexBytes) return nullptr;
    std::uint32_t at = (arena_used_ + align - 1u) / align * align;
    if (at + bytes > kVertexBytes) {
        ++gpu_stats_.arena_flushes;
        flush_frame();
        at = 0;
    }
    last_alloc_ = at;
    arena_used_ = at + bytes;
    return arena_ + at;
}

GpuRenderer::Vertex *GpuRenderer::alloc_vertices(std::uint32_t count, std::uint32_t &first) {
    constexpr auto stride = static_cast<std::uint32_t>(sizeof(Vertex));
    auto *v = static_cast<Vertex *>(alloc_linear(count * stride, stride));
    if (v != nullptr) first = static_cast<std::uint32_t>(reinterpret_cast<std::uint8_t *>(v) - arena_) / stride;
    return v;
}

// One attribute buffer at arena_ per layout; set only when the layout changes
// (the buffer configuration is about 40 command words).
void GpuRenderer::use_buffer(Layout layout) {
    if (attr_valid_ && attr_bound_ == layout) return;
    C3D_SetAttrInfo(&attr_[static_cast<int>(layout)]);
    C3D_BufInfo buf;
    BufInfo_Init(&buf);
    if (layout == Layout::Screen) BufInfo_Add(&buf, arena_, sizeof(Vertex), 3, 0x210);
    else if (layout == Layout::Model) BufInfo_Add(&buf, arena_, offsetof(ShaderVertex, w), 4, 0x3210);
    else BufInfo_Add(&buf, arena_, sizeof(ShaderVertex), 6, 0x543210);
    C3D_SetBufInfo(&buf);
    attr_bound_ = layout;
    attr_valid_ = true;
}

void GpuRenderer::set_uniform(int loc, float x, float y, float z, float w) {
    if (loc < 0 || loc >= 96) return;
    float *c = uniform_cache_[loc];
    if (uniform_known_[loc] && c[0] == x && c[1] == y && c[2] == z && c[3] == w) return;
    c[0] = x; c[1] = y; c[2] = z; c[3] = w;
    uniform_known_[loc] = true;
    C3D_FVUnifSet(GPU_VERTEX_SHADER, loc, x, y, z, w);
}

void GpuRenderer::set_matrix(int loc, const C3D_Mtx &m) {
    for (int i = 0; i < 4; ++i) set_uniform(loc + i, m.r[i].x, m.r[i].y, m.r[i].z, m.r[i].w);
}

void GpuRenderer::set_bool(int loc, bool value) {
    const int id = loc - 0x68;
    if (loc < 0 || id < 0 || id >= 16) return;
    if (bool_cache_[id] == static_cast<int>(value)) return;
    bool_cache_[id] = value ? 1 : 0;
    C3D_BoolUnifSet(GPU_VERTEX_SHADER, loc, value);
}

// Screen vertices: identity world, no skinning or lighting; UVs in texels.
void GpuRenderer::use_screen_vertices(float scale_u, float scale_v) {
    set_uniform(u_.world + 0, 1.0f, 0.0f, 0.0f, 0.0f);
    set_uniform(u_.world + 1, 0.0f, 1.0f, 0.0f, 0.0f);
    set_uniform(u_.world + 2, 0.0f, 0.0f, 1.0f, 0.0f);
    set_uniform(u_.uvxf, scale_u, scale_v, 0.0f, 0.0f);
    set_bool(u_.skin, false);
    set_bool(u_.light, false);
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
    std::uint32_t first = 0;
    Vertex *v = alloc_vertices(6, first);
    if (v == nullptr) return;
    state_valid_ = false; // the state below is not the GE's
    set_matrix(u_.screen, to_screen ? screen_projection() : target_projection());
    use_screen_vertices(1.0f / kTargetSize, 1.0f / kTargetSize);
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
    use_buffer(Layout::Screen);
    C3D_DrawArrays(GPU_TRIANGLES, static_cast<int>(first), 6);
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
        key = (key ^ regs.clut_hash) * 0x100000001B3ull; // palette hash computed at LOADCLUT
    }
    auto it = textures_.find(key);
    if (it != textures_.end() && it->second.checked_frame == frame_) {
        ++gpu_stats_.texture_hits;
    } else {
        const u64 hash_start = svcGetSystemTick();
        const std::uint64_t hash = ge::texture_hash(memory, regs, info);
        gpu_stats_.hash_ticks += svcGetSystemTick() - hash_start;
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
            const u64 upload_start = svcGetSystemTick();
            ge::decode_texture(memory, regs, info, rgba_);
            for (std::uint32_t y = 0; y < th; ++y)
                for (std::uint32_t x = 0; x < tw; ++x) {
                    const std::uint32_t c = rgba_[static_cast<std::size_t>(y % info.height) * info.width + (x % info.width)];
                    const std::uint32_t at = tiled_index(x, tiled_row(y, th), tw);
                    if (fmt == GPU_RGBA8) static_cast<std::uint32_t *>(entry.tex.data)[at] = to_rgba8(c);
                    else static_cast<std::uint16_t *>(entry.tex.data)[at] = to_16(c, fmt);
                }
            C3D_TexFlush(&entry.tex);
            gpu_stats_.upload_ticks += svcGetSystemTick() - upload_start;
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

void GpuRenderer::apply_fragment_state(const ge::GeRegisters &regs, bool textured, bool clear_mode, bool no_cull) {
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
    // renderer. Through mode: measured in Azahar, this mapping draws the logos
    // and title screen (the opposite one culled them all). Transform mode culls
    // the opposite winding [VERIFIED on the PC renderer, first battle: the
    // through-mode mapping culled the faces turned to the camera].
    const bool through = (r24(regs, 0x12) & (1u << 23)) != 0u;
    if (on(regs, 0x1D) && !no_cull) C3D_CullFace(((r24(regs, 0x9B) & 1u) != 0u) == through ? GPU_CULL_FRONT_CCW : GPU_CULL_BACK_CCW);
    else C3D_CullFace(GPU_CULL_NONE); // sprites are never culled
}

void GpuRenderer::set_scissor(const ge::GeRegisters &regs) {
    const std::uint32_t a = r24(regs, 0xD4), b = r24(regs, 0xD5);
    if (state_valid_ && scissor_key_[0] == a && scissor_key_[1] == b) return;
    scissor_key_[0] = a;
    scissor_key_[1] = b;
    const auto x1 = a & 0x3FFu, y1 = (a >> 10) & 0x3FFu, x2 = b & 0x3FFu, y2 = (b >> 10) & 0x3FFu;
    if (kTargetYSign > 0.0f) C3D_SetScissor(GPU_SCISSOR_NORMAL, x1, y1, x2 + 1u, y2 + 1u);
    else C3D_SetScissor(GPU_SCISSOR_NORMAL, x1, kTargetSize - 1u - y2, x2 + 1u, kTargetSize - y1);
}

void GpuRenderer::draw(psprecomp::GuestMemory &memory, const ge::GeRegisters &regs, ge::Prim prim, std::uint32_t count,
                       std::uint32_t vertex_address, std::uint32_t index_address) {
    ++draw_stats_.prims;
    draw_stats_.vertices += count;
    if (prim == ge::Prim::Points || prim == ge::Prim::Lines || prim == ge::Prim::LineStrip) { ++gpu_stats_.skipped_prims; return; }
    const std::uint32_t color_address = 0x04000000u | (r24(regs, 0x9C) & 0x1FFFF0u);
    const std::uint32_t stride = r24(regs, 0x9D) & 0x7FCu;
    if (stride == 0u) return;
    const u64 prep_start = svcGetSystemTick();
    begin_frame();
    if (command_buffer_fill() > kCmdBufFlushUsage) {
        ++gpu_stats_.command_flushes;
        flush_frame();
    }
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

    float su = 1.0f, sv = 1.0f;
    const C3D_Tex *tex = textured ? bind_texture(memory, regs, su, sv) : nullptr;
    // Fragment state from the GE registers it depends on; unchanged state is not set again.
    const bool sprites = prim == ge::Prim::Sprites;
    const std::array<std::uint32_t, 20> key = {
        r24(regs, 0xC9), r24(regs, 0xCA), regs.reg[0x22] & 1u, r24(regs, 0xDB), regs.reg[0x21] & 1u, r24(regs, 0xDF),
        r24(regs, 0xE0), r24(regs, 0xE1), r24(regs, 0xE8), regs.reg[0x23] & 1u, r24(regs, 0xDE), r24(regs, 0xE7),
        regs.reg[0x1D] & 1u, r24(regs, 0x9B), layout.through ? 1u : 0u, r24(regs, 0xD3), tex != nullptr ? 1u : 0u,
        sprites ? 1u : 0u, 0u, 0u};
    if (!state_valid_ || key != frag_key_) {
        apply_fragment_state(regs, tex != nullptr, clear_mode, sprites);
        frag_key_ = key;
    }
    if (tex != nullptr && (!state_valid_ || tex != bound_tex_ || std::memcmp(tex, &bound_tex_copy_, sizeof(C3D_Tex)) != 0)) {
        C3D_TexBind(0, const_cast<C3D_Tex *>(tex));
        bound_tex_ = tex;
        std::memcpy(&bound_tex_copy_, tex, sizeof(C3D_Tex));
    }
    set_scissor(regs);
    state_valid_ = true;
    u32 *cmd_base = nullptr;
    u32 cmd_size = 0, cmd_before = 0;
    GPUCMD_GetBuffer(&cmd_base, &cmd_size, &cmd_before);
    const u64 vertex_start = svcGetSystemTick();
    gpu_stats_.prep_ticks += vertex_start - prep_start;
    auto count_commands = [&] {
        u32 *b = nullptr;
        u32 s = 0, after = 0;
        GPUCMD_GetBuffer(&b, &s, &after);
        if (b == cmd_base && after >= cmd_before) {
            gpu_stats_.command_words += after - cmd_before;
            ++gpu_stats_.counted_draws;
        }
    };

    // Transform-mode triangles: the vertex shader transforms, skins and lights.
    if (!layout.through && !clear_mode && !sprites && !cpu_vertices_ &&
        draw_model(memory, regs, layout, prim, count, vertex_address, index_address, tex != nullptr, su, sv)) {
        count_commands();
        return;
    }

    ge::decode_screen_vertices(memory, regs, count, vertex_address, index_address, screen_);

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
    std::uint32_t first = 0;
    out = alloc_vertices(capacity, first);
    if (out == nullptr) { ++gpu_stats_.skipped_prims; return; }
    const auto &v = screen_;
    switch (prim) {
    case ge::Prim::Sprites:
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
    arena_used_ = last_alloc_ + emitted * static_cast<std::uint32_t>(sizeof(Vertex)); // return the unused tail
    const u64 uniform_start = svcGetSystemTick();
    gpu_stats_.vertex_ticks += uniform_start - vertex_start;
    if (emitted == 0u) return;
    set_matrix(u_.screen, target_projection());
    use_screen_vertices(su, sv);
    const u64 submit_start = svcGetSystemTick();
    gpu_stats_.uniform_ticks += submit_start - uniform_start;
    use_buffer(Layout::Screen);
    C3D_DrawArrays(GPU_TRIANGLES, static_cast<int>(first), static_cast<int>(emitted));
    gpu_stats_.submit_ticks += svcGetSystemTick() - submit_start;
    count_commands();
    ++gpu_stats_.draws;
    gpu_stats_.triangles += emitted / 3u;
}

// Transform-mode triangles through the vertex shader (shaders/ge.v.pica): the
// CPU only unpacks the referenced vertices (morph targets blended) and the
// indices; transform, skinning and lighting run on the PICA200 with the GE
// matrices and light registers as uniforms. Returns false for a draw it
// cannot take (the caller then uses the CPU transform).
bool GpuRenderer::draw_model(psprecomp::GuestMemory &memory, const ge::GeRegisters &regs, const ge::VertexLayout &layout,
                             ge::Prim prim, std::uint32_t count, std::uint32_t vertex_address, std::uint32_t index_address,
                             bool textured, float scale_u, float scale_v) {
    GPU_Primitive_t primitive;
    switch (prim) {
    case ge::Prim::Triangles: primitive = GPU_TRIANGLES; break;
    case ge::Prim::TriangleStrip: primitive = GPU_TRIANGLE_STRIP; break;
    case ge::Prim::TriangleFan: primitive = GPU_TRIANGLE_FAN; break;
    default: return false;
    }
    if (count < 3u) return true;

    // Referenced vertex range.
    std::uint32_t lo = 0, hi = count - 1u;
    if (layout.index_format != 0u) {
        lo = ~0u; hi = 0u;
        for (std::uint32_t i = 0; i < count; ++i) {
            const auto index = ge::vertex_index(memory, layout, index_address, i);
            lo = std::min(lo, index);
            hi = std::max(hi, index);
        }
    }
    const std::uint32_t n = hi - lo + 1u;
    if (n > 0xFFFFu) return false;
    const bool skinned = layout.weights != 0u;
    const std::uint32_t stride = skinned ? sizeof(ShaderVertex) : static_cast<std::uint32_t>(offsetof(ShaderVertex, w));
    const std::uint32_t vertex_bytes = n * stride; // stride is a multiple of 4
    const std::uint32_t index_bytes = layout.index_format != 0u ? count * 2u : 0u;
    const u64 vertex_start = svcGetSystemTick();
    auto *block = static_cast<std::uint8_t *>(alloc_linear(vertex_bytes + index_bytes, stride));
    if (block == nullptr) return false;
    // Index of the first vertex from arena_ (the attribute buffer base); the
    // arena holds fewer than 0x10000 vertices of any layout.
    const std::uint32_t first = static_cast<std::uint32_t>(block - arena_) / stride;

    ge::ModelVertex m;
    ShaderVertex s;
    for (std::uint32_t k = 0; k < n; ++k) {
        if (!ge::decode_model_vertex(memory, regs, layout, vertex_address, lo + k, m)) m = ge::ModelVertex{};
        s.x = m.pos[0]; s.y = m.pos[1]; s.z = m.pos[2];
        s.nx = m.normal[0]; s.ny = m.normal[1]; s.nz = m.normal[2];
        s.u = m.uv[0]; s.v = m.uv[1];
        s.r = m.color[0]; s.g = m.color[1]; s.b = m.color[2]; s.a = m.color[3];
        std::memcpy(s.w, m.weights, sizeof s.w);
        std::memcpy(block + k * stride, &s, stride);
    }
    std::uint16_t *indices = nullptr;
    if (index_bytes != 0u) {
        indices = reinterpret_cast<std::uint16_t *>(block + vertex_bytes);
        for (std::uint32_t i = 0; i < count; ++i)
            indices[i] = static_cast<std::uint16_t>(first + ge::vertex_index(memory, layout, index_address, i) - lo);
    }
    const u64 uniform_start = svcGetSystemTick();
    gpu_stats_.vertex_ticks += uniform_start - vertex_start;

    // screen = target projection x viewport x PROJ x VIEW (see the shader
    // header): the screen mapping of decode_screen_vertices as a matrix.
    const float vpx = ge::ge_float(r24(regs, 0x42)), vpy = ge::ge_float(r24(regs, 0x43)), vpz = ge::ge_float(r24(regs, 0x44));
    const float vcx = ge::ge_float(r24(regs, 0x45)), vcy = ge::ge_float(r24(regs, 0x46)), vcz = ge::ge_float(r24(regs, 0x47));
    const float offx = static_cast<float>(r24(regs, 0x4C) & 0xFFFFu) / 16.0f, offy = static_cast<float>(r24(regs, 0x4D) & 0xFFFFu) / 16.0f;
    C3D_Mtx a;
    Mtx_Zeros(&a);
    a.r[0].x = 2.0f * vpx / kTargetSize; a.r[0].w = 2.0f * (vcx - offx) / kTargetSize - 1.0f;
    a.r[1].y = kTargetYSign * 2.0f * vpy / kTargetSize; a.r[1].w = kTargetYSign * (2.0f * (vcy - offy) / kTargetSize - 1.0f);
    a.r[2].z = -vpz / 65535.0f; a.r[2].w = -vcz / 65535.0f;
    a.r[3].w = 1.0f;
    C3D_Mtx p;
    const auto &pr = regs.proj; // column-major: clip.x = e.x*p0 + e.y*p4 + e.z*p8 + p12
    for (int i = 0; i < 4; ++i) { p.r[i].x = pr[i]; p.r[i].y = pr[4 + i]; p.r[i].z = pr[8 + i]; p.r[i].w = pr[12 + i]; }
    set_matrix(u_.screen, multiply(a, multiply(p, ge_matrix43(regs.view.data()))));
    const C3D_Mtx world = ge_matrix43(regs.world.data());
    for (int i = 0; i < 3; ++i) set_uniform(u_.world + i, world.r[i].x, world.r[i].y, world.r[i].z, world.r[i].w);

    if (textured) {
        const auto info = ge::texture_info(regs);
        const float tw = static_cast<float>(info.width) * scale_u, th = static_cast<float>(info.height) * scale_v;
        set_uniform(u_.uvxf, ge::ge_float(r24(regs, 0x48)) * tw, ge::ge_float(r24(regs, 0x49)) * th,
                    ge::ge_float(r24(regs, 0x4A)) * tw, ge::ge_float(r24(regs, 0x4B)) * th);
    }

    set_bool(u_.skin, skinned);
    if (skinned) {
        for (std::uint32_t b = 0; b < layout.weights && b < 8u; ++b) {
            const C3D_Mtx bone = ge_matrix43(&regs.bone[b * 12u]);
            for (int j = 0; j < 3; ++j)
                set_uniform(u_.bones + static_cast<int>(b) * 3 + j, bone.r[j].x, bone.r[j].y, bone.r[j].z, bone.r[j].w);
        }
    }

    const ge::LightingSetup l = ge::lighting_setup(regs, layout);
    set_bool(u_.light, l.enabled);
    if (l.enabled) {
        auto color = [this](int loc, const std::array<float, 4> &c, float alpha) { set_uniform(loc, c[0], c[1], c[2], alpha); };
        color(u_.emissive, l.emissive, 0.0f);
        color(u_.matamb, l.material_ambient, l.material_ambient[3]);
        color(u_.matdif, l.material_diffuse, 0.0f);
        color(u_.matspe, l.material_specular, 0.0f);
        color(u_.sceneamb, l.scene_ambient, l.scene_ambient[3]);
        set_uniform(u_.matsel, l.vertex_ambient ? 1.0f : 0.0f, l.vertex_diffuse ? 1.0f : 0.0f, l.vertex_specular ? 1.0f : 0.0f,
                    l.reverse_normals ? -1.0f : 1.0f);
        set_uniform(u_.misc, l.specular_power, 0.0f, 0.0f, 0.0f);
        for (int i = 0; i < 4; ++i) {
            const auto &li = l.lights[i];
            set_bool(u_.lights[i], li.enabled);
            if (!li.enabled) continue;
            set_uniform(u_.lpos + i, li.position[0], li.position[1], li.position[2], 0.0f);
            float d[3] = {li.direction[0], li.direction[1], li.direction[2]};
            const float dl = std::sqrt(d[0] * d[0] + d[1] * d[1] + d[2] * d[2]);
            if (dl > 0.0f) for (auto &c : d) c /= dl;
            set_uniform(u_.ldir + i, d[0], d[1], d[2], 0.0f);
            if (li.kind == 0u) set_uniform(u_.latt + i, 1.0f, 0.0f, 0.0f, 0.0f);
            else set_uniform(u_.latt + i, li.attenuation[0], li.attenuation[1], li.attenuation[2], 0.0f);
            set_uniform(u_.lspot + i, li.kind == 2u ? li.spot_exponent : 0.0f, li.kind == 2u ? li.spot_cutoff : -2.0f,
                        li.kind != 0u ? 1.0f : 0.0f, li.components == 2u ? l.specular_power : 1.0f);
            color(u_.lamb + i, li.ambient, 0.0f);
            color(u_.ldif + i, li.diffuse, 0.0f);
            if (li.components == 1u) color(u_.lspe + i, li.specular, 0.0f);
            else set_uniform(u_.lspe + i, 0.0f, 0.0f, 0.0f, 0.0f);
        }
    }

    const u64 submit_start = svcGetSystemTick();
    gpu_stats_.uniform_ticks += submit_start - uniform_start;
    use_buffer(skinned ? Layout::Skinned : Layout::Model);
    if (indices != nullptr) C3D_DrawElements(primitive, static_cast<int>(count), C3D_UNSIGNED_SHORT, indices);
    else C3D_DrawArrays(primitive, static_cast<int>(first), static_cast<int>(count));
    gpu_stats_.submit_ticks += svcGetSystemTick() - submit_start;
    ++gpu_stats_.draws;
    ++gpu_stats_.model_draws;
    gpu_stats_.model_vertices += n;
    gpu_stats_.triangles += primitive == GPU_TRIANGLES ? count / 3u : count - 2u;
    return true;
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
    // The picture to show and its version. When the screen already shows it,
    // nothing is submitted: draws of the frame being built stay queued until
    // the game displays it (the game runs at 30 frames/s or less, presents
    // come every vblank).
    C3D_Tex *source = nullptr;
    std::uint64_t source_seq = 0;
    Target *t = nullptr;
    std::uint32_t address = 0, format = 0;
    bool cpu = false;
    if (fb.active && fb.topaddr != 0u) {
        address = psprecomp::GuestMemory::canonical(fb.topaddr);
        format = static_cast<std::uint32_t>(fb.pixelformat);
        t = target_for(address, format, false);
        if (t != nullptr && t->gpu_seq > t->cpu_seq) {
            source = &t->tex;
            source_seq = t->gpu_seq;
        } else {
            cpu = true;
            source = &fallback_;
        }
    }
    if (!cpu && source == shown_ && source_seq == shown_seq_) {
        ++gpu_stats_.skipped_presents;
        return;
    }
    begin_frame();
    last_present_cpu_ = cpu;
    if (cpu) {
        // Movie frames and other CPU-written pictures: convert only when changed.
        const std::uint64_t version = t != nullptr ? t->cpu_seq : ++seq_;
        if (fallback_address_ != address || fallback_seq_ != version) {
            upload_guest_framebuffer(memory, address, static_cast<std::uint32_t>(fb.bufferwidth), format);
            fallback_address_ = address;
            fallback_seq_ = version;
        }
        ++gpu_stats_.cpu_presents;
    } else if (source != nullptr) {
        ++gpu_stats_.gpu_presents;
    }
    shown_ = source;
    shown_seq_ = source_seq;
    C3D_FrameDrawOn(top_);
    bound_ = nullptr;
    bound_screen_ = true;
    if (source == nullptr) C3D_RenderTargetClear(top_, C3D_CLEAR_ALL, 0x000000FFu, 0);
    else blit(*source, 480.0f, 272.0f, true);
    end_frame();
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
