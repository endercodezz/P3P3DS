#pragma once
// PICA200 (citro3d) backend for the PSP GE: GE draws become GPU draws into
// VRAM render targets, one per PSP framebuffer address, and the displayed
// framebuffer is scaled to the top screen by the GPU.
//
// Transform-mode triangles go to the PICA200 vertex shader as model vertices
// (shaders/ge.v.pica: skinning, world/view/projection, lighting); through
// mode, sprites and clear mode are transformed on the CPU (screen vertices).
// Shared with SoftwareRenderer: vertex decoding, the CPU transform/lighting
// reference and texel decoding (core/src/ge/geometry.cpp). Known differences
// (see docs/3DS_PLATFORM.md section 4.4): GPU-drawn pixels are not written
// back to guest EDRAM (CPU readback sees stale data), points/lines are not
// drawn, fog is not applied, flat shading of transformed triangles is smooth,
// render-to-texture only for a texture that starts exactly at a render
// target, colour masks are per channel.
#include "p3p3ds/ge/geometry.hpp"
#include "p3p3ds/ge/model_batch.hpp"
#include "p3p3ds/ge/renderer.hpp"
#include "p3p3ds/ge/vertex_cache.hpp"
#include "p3p3ds/hle/display.hpp"

#include <citro3d.h>

#include <array>
#include <cstdint>
#include <atomic>
#include <unordered_map>
#include <vector>

namespace p3p3ds::n3ds {

struct GpuStats {
    std::uint64_t draws{}, triangles{}, skipped_prims{}, texture_uploads{}, texture_hits{}, texture_bytes{};
    std::uint64_t target_textures{}, cpu_presents{}, gpu_presents{}, cpu_to_target{}, frame_flushes{};
    std::uint64_t skipped_presents{}; // nothing new to show: no GPU frame, no buffer swap
    std::uint64_t model_draws{}, model_vertices{}; // transform-mode draws done by the vertex shader
    // CPU time (ARM11 system ticks) spent hashing texture data, converting
    // textures to PICA layout, and waiting in C3D_FrameBegin for the GPU.
    std::uint64_t hash_ticks{}, upload_ticks{}, wait_ticks{};
    // CPU time per draw step: target, texture and fragment state (prep),
    // vertex unpack or CPU transform (vertex), shader uniforms (uniform),
    // citro3d state emission and the draw command (submit).
    std::uint64_t prep_ticks{}, vertex_ticks{}, uniform_ticks{}, submit_ticks{};
    // Part of vertex_ticks for model draws: getting the shader vertices into
    // the arena (vertex cache lookup and copy, or unpacking).
    std::uint64_t fill_ticks{};
    // GPU command words added by draws (frames not flushed in between), and
    // why frames were submitted early: command buffer or vertex arena full.
    std::uint64_t command_words{}, counted_draws{}, command_flushes{}, arena_flushes{};
    std::uint64_t model_batches{}; // GPU draws that model draws were merged into
    std::uint64_t fast_draws{};    // model draws that joined a batch on the fast path
    std::uint64_t model_runs{};    // vertex blocks filled for runs of contiguous model draws
    std::uint64_t run_draws{};     // model draws that went into such a run
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
    void sync() override { close_run(); } // read the open run's vertices before the CPU runs
    [[nodiscard]] const ge::DrawStats &stats() const override { return draw_stats_; }

    // CPU store into guest EDRAM (GuestMemory::vram_write_observer).
    void note_cpu_write(std::uint32_t address, std::size_t bytes);
    // Shows the displayed PSP framebuffer on the top screen and ends the GPU frame.
    void present(psprecomp::GuestMemory &memory, const hle::DisplayFramebufInfo &fb);
    // Copies the last presented top-screen image to `rgb` (400x240, RGB, top row
    // first). Waits for the GPU. For frame dumps only.
    bool read_top_screen(std::vector<std::uint8_t> &rgb);

    [[nodiscard]] const GpuStats &gpu_stats() const { return gpu_stats_; }
    [[nodiscard]] const ge::VertexCacheStats &vertex_cache_stats() const { return vertex_cache_.stats(); }
    [[nodiscard]] bool vertex_cache_enabled() const { return vertex_cache_enabled_; }
    // The last presented picture came from guest memory (CPU-written, e.g. a movie frame).
    [[nodiscard]] bool last_present_from_cpu() const { return last_present_cpu_; }

private:
    // Screen vertex (CPU-transformed): v0 (x*w, y*w, z*w, w), v2 texels, v3 colour.
    struct Vertex {
        float x, y, z, w;
        float u, v;
        std::uint8_t r, g, b, a;
    };
    // Model vertex for the GPU transform: v0 position, v1 normal, v2 UV,
    // v3 colour, v4/v5 skinning weights (only in the skinned layout).
    struct ShaderVertex {
        float x, y, z;
        float nx, ny, nz;
        float u, v;
        std::uint8_t r, g, b, a;
        float w[8];
    };
    enum class Layout { Screen, Model, Skinned };
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
    void apply_fragment_state(const ge::GeRegisters &regs, bool textured, bool clear_mode, bool no_cull);
    Vertex *alloc_vertices(std::uint32_t count, std::uint32_t &first);
    void *alloc_linear(std::uint32_t bytes, std::uint32_t align);
    void use_buffer(Layout layout);
    void set_scissor(const ge::GeRegisters &regs);
    void set_uniform(int loc, float x, float y, float z, float w);
    void set_matrix(int loc, const C3D_Mtx &m);
    void set_bool(int loc, bool value);
    void use_screen_vertices(float scale_u, float scale_v);
    void draw_model(psprecomp::GuestMemory &memory, const ge::GeRegisters &regs, const ge::VertexLayout &layout, ge::Prim prim,
                    std::uint32_t count, std::uint32_t vertex_address, std::uint32_t index_address, bool start, bool textured,
                    float scale_u, float scale_v);
    void set_model_uniforms(const ge::GeRegisters &regs, const ge::VertexLayout &layout, bool textured, float scale_u, float scale_v);
    // Shader vertices lo .. lo + n - 1 of the guest buffer into `out` (vertex
    // cache copy or unpack).
    void fill_model_vertices(psprecomp::GuestMemory &memory, const ge::GeRegisters &regs, const ge::VertexLayout &layout,
                             std::uint32_t vertex_address, std::uint32_t lo, std::uint32_t n, std::uint8_t *out);
    // Fills the open run's block and returns its unused tail to the arena.
    void close_run();
    void flush_batch();
    void evict_textures(std::uint32_t needed);

    ge::DrawStats draw_stats_;
    GpuStats gpu_stats_;
    shaderProgram_s program_{};
    DVLB_s *dvlb_{};
    struct Uniforms {
        int screen{-1}, world{-1}, bones{-1}, uvxf{-1}, emissive{-1}, matamb{-1}, matdif{-1}, matspe{-1}, sceneamb{-1};
        int matsel{-1}, misc{-1}, lpos{-1}, ldir{-1}, latt{-1}, lspot{-1}, lamb{-1}, ldif{-1}, lspe{-1};
        int skin{-1}, light{-1}, lights[4]{-1, -1, -1, -1};
    } u_;
    // Last values written per float uniform register / bool, so unchanged
    // uniforms add no GPU commands.
    float uniform_cache_[96][4]{};
    bool uniform_known_[96]{};
    int bool_cache_[16]{};
    C3D_AttrInfo attr_[3]{};
    // Every layout's attribute buffer starts at arena_ (draws address their
    // vertices by index), so its configuration is sent only when the layout
    // changes, not per draw.
    Layout attr_bound_{Layout::Screen};
    bool attr_valid_{};
    // GE registers the fragment state was last built from, the bound texture
    // and the scissor: unchanged state is not set again (citro3d re-sends
    // every block that is set, whether or not it changed). blit() sets its
    // own state and clears state_valid_.
    std::array<std::uint32_t, 20> frag_key_{};
    std::uint32_t scissor_key_[2]{};
    const C3D_Tex *bound_tex_{};
    C3D_Tex bound_tex_copy_{};
    bool state_valid_{};
    // Batching: consecutive model draws whose state, uniforms included, is
    // identical (batch_sig_) become one indexed triangle-list draw. The
    // pending draw is submitted before anything else touches the GPU.
    std::vector<std::uint32_t> sig_, batch_sig_;
    std::vector<std::uint16_t> batch_indices_;
    const Target *batch_target_{};
    Layout batch_layout_{Layout::Model};
    bool batch_active_{};
    std::uint32_t index_used_{}; // bytes of the index area (after the vertex arena) used this GPU frame
    // The last model draw that went through the full path: its GE state
    // version and what was derived from it (fast path in draw()).
    bool fast_valid_{}, fast_textured_{};
    std::uint64_t fast_version_{}, fast_frame_{};
    Target *fast_target_{};
    ge::VertexLayout fast_layout_{};
    float fast_su_{1.0f}, fast_sv_{1.0f};
    // Step timing samples every 16th draw (svcGetSystemTick is a system call).
    std::uint32_t timed_seq_{};
    bool timed_{};
    u64 tick_cost_{}; // ticks of one svcGetSystemTick, measured at start and subtracted per step
    // Model draws: the screen matrix and the lighting setup with the GE
    // registers they were built from (rebuilt only when those change).
    std::array<std::uint32_t, 36> screen_key_{};
    C3D_Mtx screen_mtx_{};
    bool screen_valid_{};
    std::array<std::uint32_t, 81> light_key_{};
    ge::LightingSetup light_{};
    bool light_valid_{};
    bool cpu_vertices_{}; // sdmc:/p3p3ds/cpu_vertices.txt: every draw through the CPU transform
    // Shader vertices of model draws kept across frames while their guest
    // bytes stay the same (main heap; sdmc:/p3p3ds/no_vertex_cache.txt turns it off).
    ge::VertexCache vertex_cache_{2u << 20};
    // Non-indexed model draws whose vertices follow each other in guest
    // memory share one arena block, filled once when the run ends (the first
    // battle: about 2,000 draws per frame of 4 vertices). While a run is open
    // nothing else is allocated from the arena (alloc_linear closes it).
    ge::ModelRun run_;
    ge::VertexLayout run_layout_{};
    ge::GeRegisters run_regs_{};                 // material colour registers of the run (0x55, 0x58)
    psprecomp::GuestMemory *run_memory_{};
    std::uint32_t run_offset_{};                 // arena offset of the run's block
    bool vertex_cache_enabled_{true};
    C3D_RenderTarget *top_{};
    void *shared_depth_{};
    C3D_Tex fallback_{}; // CPU-converted guest framebuffer (512x512 RGBA8, linear memory)
    std::vector<Target> targets_;
    Target *bound_{};
    bool bound_screen_{};
    std::unordered_map<std::uint64_t, CachedTexture> textures_;
    std::vector<C3D_Tex> deferred_free_;
    std::uint32_t texture_bytes_{};
    std::uint8_t *arena_{};       // linear memory for vertices and indices of the open GPU frame
    std::uint32_t arena_used_{};
    std::vector<ge::ModelVertex> model_;
    std::uint32_t last_alloc_{}; // offset of the latest allocation (unused tail can be returned)
    bool in_frame_{};
    std::uint64_t frame_{1}, seq_{};
    std::vector<ge::ScreenVertex> screen_;
    std::vector<std::uint32_t> rgba_;
    // What fallback_ holds: guest framebuffer address and the target's cpu_seq
    // at upload time (re-upload only after new CPU writes).
    std::uint32_t fallback_address_{};
    std::uint64_t fallback_seq_{~0ull};
    std::uint64_t fallback_frame_{}; // GPU frame that last sampled fallback_
    std::atomic<bool> last_present_cpu_{}; // written by the GE worker thread, read by the game's thread
    // What the top screen shows: source texture and its version (gpu_seq of
    // a target, or fallback_seq_ for a CPU picture).
    const C3D_Tex *shown_{};
    std::uint64_t shown_seq_{~0ull};
    void end_frame();
};

} // namespace p3p3ds::n3ds
