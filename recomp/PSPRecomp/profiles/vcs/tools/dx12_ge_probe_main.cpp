#include "ge_gpu_backend.hpp"
#include "vcs_config.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#if defined(_WIN32)
#include <cstdlib>
#endif

namespace {

vcs::GeGpuDrawDescriptor base_draw(std::uint32_t framebuffer) {
    vcs::GeGpuDrawDescriptor draw{};
    draw.framebuffer_address = framebuffer;
    draw.framebuffer_stride = 512u;
    draw.framebuffer_format = 3u;
    draw.primitive = 3u;
    draw.vertex_count = 3u;
    draw.scissor_x0 = 0;
    draw.scissor_y0 = 0;
    draw.scissor_x1 = 479;
    draw.scissor_y1 = 271;
    draw.color_write_mask = 0u;
    draw.depth_test_enabled = false;
    draw.depth_write_enabled = false;
    draw.alpha_test_enabled = false;
    draw.blend_enabled = false;
    draw.fog_enabled = false;
    return draw;
}

void set_triangle(vcs::GeGpuVertex (&triangle)[3], std::uint32_t color) {
    triangle[0].x = 60.0f; triangle[0].y = 230.0f; triangle[0].z = 32768.0f;
    triangle[0].w = 1.0f; triangle[0].rgba = color; triangle[0].u = 0.0f; triangle[0].v = 272.0f;
    triangle[1].x = 240.0f; triangle[1].y = 30.0f; triangle[1].z = 32768.0f;
    triangle[1].w = 1.0f; triangle[1].rgba = color; triangle[1].u = 240.0f; triangle[1].v = 0.0f;
    triangle[2].x = 420.0f; triangle[2].y = 230.0f; triangle[2].z = 32768.0f;
    triangle[2].w = 1.0f; triangle[2].rgba = color; triangle[2].u = 480.0f; triangle[2].v = 272.0f;
}

} // namespace

int main() {
#if !defined(_WIN32)
    std::cout << "dx12_ge_probe: SKIP (Windows only)\n";
    return 0;
#else
    try {
        const std::filesystem::path root =
            std::filesystem::temp_directory_path() / "vcsnative_dx12_ge_probe";
        std::filesystem::create_directories(root);
        const std::filesystem::path ini = root / "VCSNative.ini";
        {
            std::ofstream out(ini, std::ios::trunc);
            out << "[Display]\nEnabled=false\n"
                << "[Rendering]\nBackend=DirectX12\nDX12GEColor=true\n"
                << "HardwareTransform=false\nInternalResolutionMode=PSP\n"
                << "MSAA=1\n"
                << "[Diagnostics]\nLogToFile=false\n";
        }
        _putenv_s("PSPRECOMP_CONFIG", ini.string().c_str());
        _putenv_s("PSPRECOMP_DX12_GE_READBACK", "1");
        _putenv_s("PSPRECOMP_DX12_GE_STRICT", "1");
        vcs::initialize_vcs_configuration(root);
        std::string error;
        if (!vcs::initialize_ge_gpu_backend(error)) {
            std::cerr << "dx12_ge_probe: FAIL initialize: " << error << "\n";
            return 2;
        }
        const vcs::GeGpuBackendReport start = vcs::ge_gpu_backend_report();
        if (start.active != vcs::GeGpuBackendKind::DirectX12 ||
            !vcs::ge_gpu_backend_graphics_ready()) {
            std::cerr << "dx12_ge_probe: FAIL backend not active: " << start.message << "\n";
            vcs::shutdown_ge_gpu_backend();
            return 3;
        }

        constexpr std::uint32_t display_fb = 0x00088000u;
        constexpr std::uint32_t offscreen_fb = 0x000A0000u;
        vcs::ge_gpu_backend_set_display_framebuffer(display_fb);

        // Pass 1: draw an offscreen framebuffer target. This target must stay
        // resident as a D3D12 SRV; there is deliberately no CPU texture upload.
        vcs::GeGpuDrawDescriptor offscreen = base_draw(offscreen_fb);
        vcs::GeGpuVertex source_triangle[3]{};
        set_triangle(source_triangle, 0xFF3030FFu);
        vcs::ge_gpu_backend_record_draw(offscreen);
        vcs::ge_gpu_backend_accumulate_color_triangles(offscreen, source_triangle);

        // Pass 2: composite that offscreen framebuffer into the displayed target
        // by sampling the framebuffer address directly. Stage 44.6 must resolve
        // this GPU->GPU, without asking the GE renderer to decode VRAM bytes.
        vcs::GeGpuDrawDescriptor feedback = base_draw(display_fb);
        feedback.texture_enabled = true;
        feedback.texture_format = 3u;
        feedback.texture_address = offscreen_fb;
        feedback.texture_buffer_width = 512u;
        feedback.texture_width = 480u;
        feedback.texture_height = 272u;
        feedback.texture_level_addresses[0] = offscreen_fb;
        feedback.texture_level_buffer_widths[0] = 512u;
        feedback.texture_level_widths[0] = 480u;
        feedback.texture_level_heights[0] = 272u;
        feedback.texture_function = 3u; // REPLACE
        feedback.texture_use_alpha = true;
        feedback.texture_clamp_u = true;
        feedback.texture_clamp_v = true;
        vcs::ge_gpu_backend_prepare_texture_keys(feedback);
        if (vcs::ge_gpu_backend_texture_needed(feedback)) {
            std::cerr << "dx12_ge_probe: FAIL framebuffer feedback requested CPU decode\n";
            vcs::shutdown_ge_gpu_backend();
            return 4;
        }
        vcs::GeGpuVertex feedback_triangle[3]{};
        set_triangle(feedback_triangle, 0xFFFFFFFFu);
        vcs::ge_gpu_backend_record_draw(feedback);
        vcs::ge_gpu_backend_accumulate_color_triangles(feedback, feedback_triangle);

        // Pass 3: same-target feedback. D3D12 forbids sampling a resource while
        // writing it as an RTV, so the backend must take an on-GPU snapshot.
        vcs::GeGpuDrawDescriptor self_feedback = feedback;
        self_feedback.texture_address = display_fb;
        self_feedback.texture_level_addresses[0] = display_fb;
        vcs::ge_gpu_backend_prepare_texture_keys(self_feedback);
        vcs::GeGpuVertex self_triangle[3]{};
        set_triangle(self_triangle, 0xFFFFFFFFu);
        self_triangle[0].x = 150.0f;
        self_triangle[1].x = 240.0f;
        self_triangle[2].x = 330.0f;
        vcs::ge_gpu_backend_record_draw(self_feedback);
        vcs::ge_gpu_backend_accumulate_color_triangles(self_feedback, self_triangle);

        // Pass 4: exercise the Stage 44.7 GPU-side hardware transform. The
        // vertices remain model-space; root constants + VS must map them to
        // clip space without CPU matrix multiplication.
        vcs::GeGpuDrawDescriptor hw_draw = base_draw(display_fb);
        vcs::GeGpuHardwareTransform hw{};
        hw.model_to_clip = {1.0f,0.0f,0.0f,0.0f,
                            0.0f,1.0f,0.0f,0.0f,
                            0.0f,0.0f,1.0f,0.0f,
                            0.0f,0.0f,0.0f,1.0f};
        hw.model_to_view_z = {0.0f,0.0f,1.0f,0.0f};
        hw.viewport_scale_x = 240.0f;
        hw.viewport_scale_y = 136.0f;
        hw.viewport_scale_z = 32767.5f;
        hw.viewport_center_x = 240.0f;
        hw.viewport_center_y = 136.0f;
        hw.viewport_center_z = 32767.5f;
        hw.depth_clip_enabled = true;
        hw.logical_prim_batches = 1u;
        hw.unique_vertices_decoded = 3u;
        vcs::GeGpuVertex hw_triangle[3]{};
        hw_triangle[0].x=-0.65f; hw_triangle[0].y=-0.55f; hw_triangle[0].z=0.0f; hw_triangle[0].w=1.0f; hw_triangle[0].rgba=0xFF30FF30u;
        hw_triangle[1].x= 0.00f; hw_triangle[1].y= 0.65f; hw_triangle[1].z=0.0f; hw_triangle[1].w=1.0f; hw_triangle[1].rgba=0xFF30FF30u;
        hw_triangle[2].x= 0.65f; hw_triangle[2].y=-0.55f; hw_triangle[2].z=0.0f; hw_triangle[2].w=1.0f; hw_triangle[2].rgba=0xFF30FF30u;
        vcs::ge_gpu_backend_record_draw(hw_draw);
        vcs::ge_gpu_backend_accumulate_hardware_triangles(hw_draw, hw, hw_triangle, {});

        if (!vcs::ge_gpu_backend_finish_color_frame(1u)) {
            std::cerr << "dx12_ge_probe: FAIL finish frame\n";
            vcs::shutdown_ge_gpu_backend();
            return 6;
        }
        const vcs::GeGpuBackendReport report = vcs::ge_gpu_backend_report();
        std::vector<std::byte> rgba(report.game_frame_readback_bytes);
        if (rgba.empty() || !vcs::ge_gpu_backend_copy_game_frame_rgba(rgba)) {
            std::cerr << "dx12_ge_probe: FAIL readback\n";
            vcs::shutdown_ge_gpu_backend();
            return 5;
        }
        std::size_t changed = 0u;
        for (std::size_t i = 0u; i + 3u < rgba.size(); i += 4u) {
            const auto r = static_cast<unsigned char>(rgba[i + 0u]);
            const auto g = static_cast<unsigned char>(rgba[i + 1u]);
            const auto b = static_cast<unsigned char>(rgba[i + 2u]);
            if (r != 0u || g != 0u || b != 0u) ++changed;
        }
        const bool feedback_ok = report.dx12_native_framebuffer_targets >= 2u &&
                                 report.dx12_gpu_feedback_draws >= 2u &&
                                 report.dx12_self_feedback_snapshots >= 1u &&
                                 report.dx12_resolves >= (report.dx12_msaa_samples > 1u ? 1u : 0u) &&
                                 report.dx12_depth_bits >= 16u &&
                                 report.release_candidate_ready;
        vcs::shutdown_ge_gpu_backend();
        std::filesystem::remove_all(root);
        if (changed < 100u || !feedback_ok) {
            std::cerr << "dx12_ge_probe: FAIL changed_pixels=" << changed
                      << " targets=" << report.dx12_native_framebuffer_targets
                      << " feedback=" << report.dx12_gpu_feedback_draws
                      << " self_snapshots=" << report.dx12_self_feedback_snapshots << "\n";
            return 7;
        }
        std::cout << "dx12_ge_probe: PASS changed_pixels=" << changed
                  << " target=" << report.offscreen_width << 'x' << report.offscreen_height
                  << " pipelines=" << report.unique_pipeline_keys
                  << " framebuffer_targets=" << report.dx12_native_framebuffer_targets
                  << " gpu_feedback=" << report.dx12_gpu_feedback_draws
                  << " self_feedback=" << report.dx12_self_feedback_snapshots << "\n";
        return 0;
    } catch (const std::exception &e) {
        std::cerr << "dx12_ge_probe: FAIL exception: " << e.what() << "\n";
        return 8;
    }
#endif
}
