#pragma once
// PICA200 (citro3d) backend for the PSP GE: GE draws become GPU draws into
// VRAM render targets, one per PSP framebuffer address, and the displayed
// framebuffer is scaled to the top screen by the GPU.
//
// Shared with SoftwareRenderer: vertex decode/transform and texel decoding
// (core/src/ge/geometry.cpp). Known differences (phase 1, see
// docs/3DS_PLATFORM.md section 4.4): GPU-drawn pixels are not written back to
// guest EDRAM (CPU readback sees stale data), points/lines are not drawn,
// lighting/fog are not applied, render-to-texture only for a texture that
// starts exactly at a render target, colour masks are per channel.
#include "p3p3ds/ge/geometry.hpp"
#include "p3p3ds/ge/renderer.hpp"
#include "p3p3ds/hle/display.hpp"

#include <citro3d.h>

#include <cstdint>
#include <unordered_map>
#include <vector>

namespace p3p3ds::n3ds {

struct GpuStats {
    std::uint64_t draws{}, triangles{}, skipped_prims{}, texture_uploads{}, texture_hits{}, texture_bytes{};
    std::uint64_t target_textures{}, cpu_presents{}, gpu_presents{}, cpu_to_target{}, frame_flushes{};
    std::uint64_t skipped_presents{}; // nothing new to show: no GPU frame, no buffer swap
    // CPU time (ARM11 system ticks) spent hashing texture data, converting
    // textures to PICA layout, and waiting in C3D_FrameBegin for the GPU.
    std::uint64_t hash_ticks{}, upload_ticks{}, wait_ticks{};
    std::uint32_t textures{}, targets{};
};

class GpuRenderer final : public ge::GeRenderer {
public:
    GpuRenderer();
    ~GpuRenderer() override;
    GpuRenderer(const GpuRenderer &) = delete;
    GpuRenderer &operator=(const GpuRenderer &) = delete;

    void draw(psprecomp::GuestMemory &memory, const ge::GeRegisters &regs, ge::Prim prim, std::uint32_t count,
              std::uint32_t vertex_address, std::uint32_t index_address) override;
    void transfer(psprecomp::GuestMemory &memory, const ge::GeRegisters &regs) override;
    [[nodiscard]] const ge::DrawStats &stats() const override { return draw_stats_; }

    // CPU store into guest EDRAM (GuestMemory::vram_write_observer).
    void note_cpu_write(std::uint32_t address, std::size_t bytes);
    // Shows the displayed PSP framebuffer on the top screen and ends the GPU frame.
    void present(psprecomp::GuestMemory &memory, const hle::DisplayFramebufInfo &fb);
    // Copies the last presented top-screen image to `rgb` (400x240, RGB, top row
    // first). Waits for the GPU. For frame dumps only.
    bool read_top_screen(std::vector<std::uint8_t> &rgb);

    [[nodiscard]] const GpuStats &gpu_stats() const { return gpu_stats_; }
    // The last presented picture came from guest memory (CPU-written, e.g. a movie frame).
    [[nodiscard]] bool last_present_from_cpu() const { return last_present_cpu_; }

private:
    struct Vertex {
        float x, y, z, w;
        float u, v;
        std::uint8_t r, g, b, a;
    };
    struct Target {
        std::uint32_t address{}, format{};
        C3D_Tex tex{};
        C3D_RenderTarget *rt{};
        std::uint64_t gpu_seq{}, cpu_seq{};
    };
    struct CachedTexture {
        C3D_Tex tex{};
        std::uint64_t hash{};
        std::uint32_t width{}, height{}, bytes{};
        std::uint64_t last_frame{}, checked_frame{};
    };

    void begin_frame();
    void flush_frame();
    Target *target_for(std::uint32_t address, std::uint32_t format, bool create);
    Target *target_containing(std::uint32_t address);
    void bind_target(Target &t);
    void upload_guest_framebuffer(psprecomp::GuestMemory &memory, std::uint32_t address, std::uint32_t stride,
                                  std::uint32_t format);
    void blit(C3D_Tex &source, float src_w, float src_h, bool to_screen);
    const C3D_Tex *bind_texture(psprecomp::GuestMemory &memory, const ge::GeRegisters &regs, float &scale_u, float &scale_v);
    void apply_fragment_state(const ge::GeRegisters &regs, bool textured, bool clear_mode);
    Vertex *alloc_vertices(std::uint32_t count);
    void evict_textures(std::uint32_t needed);

    ge::DrawStats draw_stats_;
    GpuStats gpu_stats_;
    shaderProgram_s program_{};
    DVLB_s *dvlb_{};
    int loc_projection_{-1}, loc_uvscale_{-1};
    C3D_RenderTarget *top_{};
    void *shared_depth_{};
    C3D_Tex fallback_{}; // CPU-converted guest framebuffer (512x512 RGBA8, linear memory)
    std::vector<Target> targets_;
    Target *bound_{};
    bool bound_screen_{};
    std::unordered_map<std::uint64_t, CachedTexture> textures_;
    std::vector<C3D_Tex> deferred_free_;
    std::uint32_t texture_bytes_{};
    Vertex *vbuf_{};
    std::uint32_t vbuf_used_{};
    bool in_frame_{};
    std::uint64_t frame_{1}, seq_{};
    std::vector<ge::ScreenVertex> screen_;
    std::vector<std::uint32_t> rgba_;
    // What fallback_ holds: guest framebuffer address and the target's cpu_seq
    // at upload time (re-upload only after new CPU writes).
    std::uint32_t fallback_address_{};
    std::uint64_t fallback_seq_{~0ull};
    std::uint64_t fallback_frame_{}; // GPU frame that last sampled fallback_
    bool last_present_cpu_{};
    // What the top screen shows: source texture and its version (gpu_seq of
    // a target, or fallback_seq_ for a CPU picture).
    const C3D_Tex *shown_{};
    std::uint64_t shown_seq_{~0ull};
    void end_frame();
};

} // namespace p3p3ds::n3ds
