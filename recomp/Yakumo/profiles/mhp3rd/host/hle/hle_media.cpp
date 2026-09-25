// sceDisplay, sceCtrl, sceGe_user, sceAudio and sceSasCore. These keep guest
// timing and callbacks behaving like hardware (vblank-paced input reads, GE
// list completion callbacks, blocking audio output); the drawing and the
// mixing themselves live under gpu/ and audio/.
#include "hle_common.hpp"
#include "kernel/fast_loading.hpp"
#include "kernel/load_trace.hpp"

#include "overlays.hpp"

#include "audio/audio_sink.hpp"
#include "audio/sas_core.hpp"

#include "psprecomp/common.hpp"

#include "camera_probe.hpp"
#if defined(MHP3RD_DEBUG_MENU)
#include "debug/debug_tools.hpp"
#endif
#include "camera/camera_input.hpp"
#include "camera/game_aspect.hpp"
#include "camera/game_camera.hpp"
#include "input/bindings.hpp"
#include "settings/settings.hpp"
#include "gpu/ge_state.hpp"
#include "perf/frame_stats.hpp"
#if defined(MHP3RD_HAS_RENDERER)
#include "gpu/vulkan_renderer.hpp"
#include "ui/ui.hpp"
#endif

#include <algorithm>
#include <cmath>
#include <cstring>
#include <array>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <vector>

namespace mhp3rd {
namespace {

constexpr std::uint32_t kEdramBase = 0x04000000u;
constexpr std::uint32_t kEdramSize = 0x00200000u;
constexpr std::uint32_t kAudioSampleRate = 44'100u;

struct DisplayState {
    std::uint32_t mode{};
    std::uint32_t width{480u};
    std::uint32_t height{272u};
    std::uint32_t framebuffer{};
    std::uint32_t buffer_width{};
    std::uint32_t pixel_format{};
};

struct GeCallback {
    std::uint32_t signal_function{};
    std::uint32_t signal_argument{};
    std::uint32_t finish_function{};
    std::uint32_t finish_argument{};
};

struct GeList {
    std::uint32_t pc{};
    std::uint32_t stall{};
    std::int32_t callback{-1};
    bool done{};
};

struct AudioChannel {
    bool reserved{};
    std::uint32_t samples{};
    std::uint32_t format{};
    // Virtual time at which everything handed to this channel has finished
    // playing, and the host ring position its next frames are mixed at.
    std::uint64_t queued_until_us{};
    std::uint64_t cursor{};
};

struct MediaState {
    DisplayState display;
    std::uint32_t ctrl_cycle{};
    std::uint32_t ctrl_mode{};
    std::map<std::int32_t, GeCallback> ge_callbacks;
    std::int32_t next_ge_callback{};
    std::map<std::uint32_t, GeList> ge_lists;
    std::uint32_t next_ge_list{1u};
    std::array<AudioChannel, 8> audio{};
    gpu::GeState ge;
#if defined(MHP3RD_HAS_RENDERER)
    std::unique_ptr<gpu::VulkanRenderer> renderer;
#endif
};

MediaState &media() {
    static MediaState state;
    return state;
}

// Runs a display list: the GE state machine produces draw calls for the
// renderer and raises the guest's signal/finish callbacks in interrupt context.
// A GE block transfer, row by row. A source in a framebuffer the renderer drew
// is written back to guest memory first: the game copies the last frame of a
// hunt this way and textures the quest reward screen's background from it.
void block_transfer(Runtime &rt, const gpu::BlockTransfer &transfer) {
    psprecomp::GuestMemory &memory = rt.memory();
#if defined(MHP3RD_HAS_RENDERER)
    if (media().renderer && media().renderer->available())
        media().renderer->read_back_framebuffer(transfer.source, memory);
#endif
    const std::size_t row_bytes = static_cast<std::size_t>(transfer.width) * transfer.bytes_per_pixel;
    for (std::uint32_t row = 0; row < transfer.height; ++row) {
        const std::uint32_t from =
            transfer.source +
            ((transfer.source_y + row) * transfer.source_stride + transfer.source_x) * transfer.bytes_per_pixel;
        const std::uint32_t to = transfer.destination + ((transfer.destination_y + row) * transfer.destination_stride +
                                                          transfer.destination_x) *
                                                             transfer.bytes_per_pixel;
        const std::uint8_t *source = memory.raw_pointer(from, row_bytes);
        std::uint8_t *destination = memory.raw_pointer(to, row_bytes);
        if (source == nullptr || destination == nullptr) {
            log_once("ge-transfer-range", "[ge] block transfer outside guest memory skipped");
            return;
        }
        std::memmove(destination, source, row_bytes);
    }
#if defined(MHP3RD_HAS_RENDERER)
    // Textures already looked up in this list may have changed.
    if (media().renderer && media().renderer->available()) media().renderer->begin_display_list();
#endif
}

void run_ge_list(Runtime &rt, std::uint32_t id) {
    auto found = media().ge_lists.find(id);
    if (found == media().ge_lists.end()) return;
    const perf::Clock::time_point start = perf::Clock::now();
    GeList &list = found->second;
    const GeCallback *callback = nullptr;
    if (const auto cb = media().ge_callbacks.find(list.callback); cb != media().ge_callbacks.end())
        callback = &cb->second;

    media().ge.set_signal_sink([callback](std::uint32_t signal, std::uint32_t pc) {
        if (callback == nullptr) return;
        const bool finish = (signal & 0x10000u) != 0u;
        const std::uint32_t function = finish ? callback->finish_function : callback->signal_function;
        if (function == 0u) return;
        InterruptCall call{};
        call.function = function;
        call.arguments = {signal & 0xFFFFu, finish ? callback->finish_argument : callback->signal_argument, pc, 0u};
        kernel().queue_interrupt(std::move(call));
    });
#if defined(MHP3RD_HAS_RENDERER)
    if (media().renderer && media().renderer->available()) {
        gpu::VulkanRenderer &renderer = *media().renderer;
        const psprecomp::GuestMemory &memory = rt.memory();
        renderer.begin_display_list();
        media().ge.set_raw_vertices(renderer.gpu_decode(), renderer.check_gpu_decode());
        media().ge.set_draw_sink([&renderer, &memory](const gpu::DrawCall &call) { renderer.submit(call, memory); });
    }
#endif
    media().ge.set_transfer_sink([&rt](const gpu::BlockTransfer &transfer) { block_transfer(rt, transfer); });

    bool finished = false;
    try {
        const perf::SplitScope split(perf::Split::Lists);
        list.pc = media().ge.execute(rt.memory(), list.pc, list.stall, finished);
    } catch (const psprecomp::Error &error) {
        // A malformed list must not take the whole run down: drop it and carry on.
        log_once("ge-list-error", std::string("[ge] display list aborted: ") + error.what());
        finished = true;
    }
    list.done = finished;
    media().ge.set_draw_sink(nullptr);
    media().ge.set_transfer_sink(nullptr);
    perf::add_render_time(perf::Clock::now() - start);
}

#if defined(MHP3RD_HAS_RENDERER)
// The mouse's motion since the previous pump, as degrees for the camera
// layer. Added after the flip, so whoever drives the camera takes it in the
// update this frame leads to.
void feed_mouse(gpu::VulkanRenderer &renderer) {
    const settings::Settings &s = settings::current();
    // A drag on the touch screen: Touch camera speed degrees for the screen's
    // height, slowed while aiming as the mouse is.
    const gpu::MouseMotion drag = renderer.take_touch_motion();
    if (drag.x != 0.0f || drag.y != 0.0f) {
        const float scale = camera::game_camera_degrees_per_second() / std::max(s.camera_speed, 1.0f);
        const float degrees = s.touch_camera_speed * scale;
        camera::add_motion(camera::Source::Touch, drag.x * degrees, drag.y * degrees);
    }
    const gpu::MouseMotion motion = renderer.take_mouse_motion();
    if (motion.x == 0.0f && motion.y == 0.0f) return;
    // While a bow or a bowgun aims, Aim speed's share of Camera speed, as
    // for the stick.
    const float scale = camera::game_camera_degrees_per_second() / std::max(s.camera_speed, 1.0f);
    const input::MouseTurn turn =
        input::mouse_turn(motion.x, motion.y, s.mouse_sensitivity, s.invert_mouse_x, s.invert_mouse_y, scale);
    camera::add_motion(camera::Source::Mouse, turn.yaw, turn.pitch);
    static const bool trace = std::getenv("MHP3RD_TRACE_PAD") != nullptr;
    if (trace)
        std::cout << "[pad] mouse " << motion.x << "," << motion.y << " -> " << turn.yaw << "," << turn.pitch
                  << " degrees" << std::endl;
}
#endif

// The emulated time of the vblank the frame being flipped started from. The
// game starts a frame every other vblank, when its vblank handler has counted
// two since the last one; the flip comes when the frame's code has run, and
// on a slower machine that is often after the vblank between, so the latest
// vblank is not the frame's own. Frames are kept on a grid of two vblanks
// from the one before: the latest start on that grid not after the latest
// vblank. A flip that comes before a whole step, or two steps late, starts
// the grid again at its latest vblank.
std::uint64_t frame_start_us(std::uint64_t latest_vblank_us) {
    static std::uint64_t previous = 0u;
    static bool known = false;
    constexpr std::uint64_t kStep = 2u * kVBlankPeriodUs;
    std::uint64_t start = latest_vblank_us;
    if (known && latest_vblank_us >= previous + kStep && latest_vblank_us < previous + 3u * kStep)
        start = previous + kStep * ((latest_vblank_us - previous) / kStep);
    previous = start;
    known = true;
    return start;
}

void present_frame(Runtime &rt) {
    load_trace::note_flip();
    // Overlays are swapped between frames; re-check before drawing the next one.
    revalidate_overlays(rt);
#if defined(MHP3RD_DEBUG_MENU)
    // Between two game frames: the developer tools' queued writes and held
    // cheats land here, never while guest code runs.
    debug::frame(rt);
#endif
#if defined(MHP3RD_HAS_RENDERER)
    if (!media().renderer || !media().renderer->available()) {
        perf::end_frame(kernel().now_us());
        return;
    }
    gpu::VulkanRenderer &renderer = *media().renderer;
    // The guest passes a VRAM offset when the high byte is zero.
    const std::uint32_t address = (media().display.framebuffer & 0xFF000000u) == 0u
                                      ? (media().display.framebuffer | 0x04000000u)
                                      : media().display.framebuffer;
    const perf::Clock::time_point present_start = perf::Clock::now();
    // The GE only draws into VRAM, so a framebuffer in main memory was
    // written by the CPU (the movie player's sceJpegCsc) and has to be shown
    // from memory.
    constexpr std::uint32_t kPixelFormat8888 = 3u;
    const DisplayState &display = media().display;
    if ((address & 0x1F000000u) != kEdramBase && display.pixel_format == kPixelFormat8888) {
        const std::size_t bytes = static_cast<std::size_t>(display.buffer_width) * display.height * 4u;
        renderer.upload_frame(address, rt.memory().raw_pointer(address, bytes), display.width, display.height,
                              display.buffer_width);
    }
    ui::draw_over_game();
    renderer.write_back_frame(rt.memory());
    // A load running fast flips far more often than the display refreshes.
    renderer.set_fast_forward(fast_loading::active());
    // The real time the frame stands for, which frame interpolation spaces
    // its presents by: that of the vblank the game's frame started from.
    const bool presented = renderer.present(address, kernel().real_time_of(frame_start_us(kernel().last_vblank_us())));
    // The frame's camera has been measured by now, so the hunt for the guest
    // variables behind it can compare RAM against it.
    probe::camera_frame(rt, media().ge.view_matrix_source());
    // The game's flip is the camera's frame: the camera update runs once
    // between two flips, however many presents interpolation adds. The stick is
    // already shaped and inverted by the input layer; its rate becomes degrees
    // over the real time since the previous flip.
    {
        static perf::Clock::time_point previous_flip = present_start;
        const float seconds = std::chrono::duration<float>(present_start - previous_flip).count();
        previous_flip = present_start;
        camera::set_rate(camera::Source::Stick, (static_cast<int>(renderer.pad().right_x) - 0x80) / 127.0f,
                         (static_cast<int>(renderer.pad().right_y) - 0x80) / 127.0f);
        camera::game_camera_frame(rt);
        // The view's shape follows the picture's: the game builds its next
        // projection with the aspect ratio of the target it will draw into.
        camera::game_aspect_frame(rt, renderer.game_aspect());
        camera::advance(seconds, camera::game_camera_degrees_per_second());
    }
    perf::add_render_time(perf::Clock::now() - present_start);
    // A frame ends when its image has been handed to the swapchain, or with
    // frame interpolation when the presents after it are scheduled.
    perf::end_frame(kernel().now_us(), presented);

    // Optional frame capture, independent of the window.
    static const char *screenshot_dir = std::getenv("MHP3RD_SCREENSHOT_DIR");
    static const std::uint64_t screenshot_every = [] {
        const char *text = std::getenv("MHP3RD_SCREENSHOT_EVERY");
        return text != nullptr ? std::strtoull(text, nullptr, 10) : 60ull;
    }();
    if (screenshot_dir != nullptr && screenshot_every != 0u &&
        renderer.frames_presented() % screenshot_every == 0u) {
        const std::string path = std::string(screenshot_dir) + "/frame_" +
                                 std::to_string(renderer.frames_presented()) + ".bmp";
        if (renderer.capture_frame(path))
            std::cout << "[render] frame " << renderer.frames_presented() << " (" << renderer.draws_submitted()
                      << " draws) -> " << path << "\n";
    }
    const bool window_open = renderer.pump_events();
    feed_mouse(renderer);
    camera::game_camera_anticipate_aim(rt);
    if (!window_open) {
        rt.stop("window closed");
    } else if (ui::take_quit_request()) {
        rt.stop("quit from the menu");
    } else if (!ui::menu_over_game() && ui::menu_requested()) {
        if (ui::menu_pauses()) {
            // The menu pauses the game: guest code and emulated time stand
            // still while it runs in here, and the device stops playing.
            renderer.pause_interpolation();
            audio::AudioSink::instance().set_paused(true);
            const bool keep_playing = ui::run_menu();
            audio::AudioSink::instance().set_paused(false);
            // Resume at normal speed rather than racing to make up the pause,
            // and keep the pause out of the frame statistics.
            kernel().resync_real_time();
            perf::restart_measurement();
            if (!keep_playing) rt.stop("quit from the menu");
        } else {
            // The game keeps running, sound and pacing included; the menu is
            // drawn over each frame and takes all input until it closes.
            ui::open_menu_over_game();
        }
    }
#else
    (void)rt;
    perf::end_frame(kernel().now_us());
#endif
}

void register_display_ctrl(HleRegistrar &hle) {
    hle.add("sceDisplay", "sceDisplaySetMode", [](Runtime &, AllegrexContext &ctx) {
        media().display.mode = arg(ctx, 0);
        media().display.width = arg(ctx, 1);
        media().display.height = arg(ctx, 2);
        kernel().finish(ctx, 0u);
    });
    // The guest flipping the framebuffer is the end of a frame.
    hle.add("sceDisplay", "sceDisplaySetFrameBuf", [](Runtime &rt, AllegrexContext &ctx) {
        media().display.framebuffer = arg(ctx, 0);
        media().display.buffer_width = arg(ctx, 1);
        media().display.pixel_format = arg(ctx, 2);
        present_frame(rt);
        kernel().finish(ctx, 0u);
    });

    hle.add("sceCtrl", "sceCtrlSetSamplingCycle", [](Runtime &, AllegrexContext &ctx) {
        const std::uint32_t previous = media().ctrl_cycle;
        media().ctrl_cycle = arg(ctx, 0);
        kernel().finish(ctx, previous);
    });
    hle.add("sceCtrl", "sceCtrlSetSamplingMode", [](Runtime &, AllegrexContext &ctx) {
        const std::uint32_t previous = media().ctrl_mode;
        media().ctrl_mode = arg(ctx, 0);
        // The mode is recorded but not acted on: the game subtracts 128 from Lx
        // unconditionally, so reporting the neutral 0x80 is right in both modes.
        if (std::getenv("MHP3RD_TRACE_PAD") != nullptr && media().ctrl_mode != previous)
            std::cout << "[pad] sceCtrlSetSamplingMode " << media().ctrl_mode << "\n";
        kernel().finish(ctx, previous);
    });
    // Reading the controller buffer blocks until the next sample (vblank).
    hle.add("sceCtrl", "sceCtrlReadBufferPositive", [](Runtime &rt, AllegrexContext &ctx) {
        const std::uint32_t address = arg(ctx, 0);
        const std::uint32_t count = std::clamp<std::uint32_t>(arg(ctx, 1), 1u, 64u);
        std::uint32_t buttons = 0u;
        std::uint8_t analog_x = 0x80u;
        std::uint8_t analog_y = 0x80u;
        std::uint8_t right_x = 0x80u;
        std::uint8_t right_y = 0x80u;
#if defined(MHP3RD_HAS_RENDERER)
        if (media().renderer && media().renderer->available()) {
            // The pad as it is now, not as it was at the last flip (#8).
            // MHP3RD_PAD_AT_FLIP keeps the state of the flip, as before.
            static const bool at_flip = std::getenv("MHP3RD_PAD_AT_FLIP") != nullptr;
            if (!at_flip) media().renderer->sample_pad();
            const gpu::PadState pad = media().renderer->pad();
            buttons = pad.buttons;
            fast_loading::note_buttons(buttons != 0u);
            analog_x = pad.analog_x;
            analog_y = pad.analog_y;
            right_x = pad.right_x;
            right_y = pad.right_y;
            // Keep the game's digital commands neutral while the analog
            // camera consumes these axes: otherwise the game's one-shot
            // vertical command fires from the same push and glides the camera
            // against what the port is doing. The physical D-pad stays available.
            if (camera::game_camera_driving()) {
                right_x = 0x80u;
                right_y = 0x80u;
            } else if (camera::game_camera_aim_boost()) {
                // Past the dead zone, any push reaches the game at full
                // length in the same direction, so its aim steps and the
                // driver decides how far. With the stick idle, the mouse's
                // direction stands in for it.
                int dx = static_cast<int>(right_x) - 0x80;
                int dy = static_cast<int>(right_y) - 0x80;
                if (dx == 0 && dy == 0) {
                    if (const auto mouse = camera::game_camera_mouse_aim()) {
                        dx = static_cast<int>(std::lround(mouse->x * 127.0f));
                        dy = static_cast<int>(std::lround(mouse->y * 127.0f));
                    }
                }
                const float length = std::sqrt(static_cast<float>(dx * dx + dy * dy));
                if (length > 0.0f) {
                    right_x = static_cast<std::uint8_t>(std::clamp(0x80 + static_cast<int>(std::lround(dx * 127.0f / length)), 0, 255));
                    right_y = static_cast<std::uint8_t>(std::clamp(0x80 + static_cast<int>(std::lround(dy * 127.0f / length)), 0, 255));
                }
            } else if (right_x == 0x80u && right_y == 0x80u &&
                       settings::current().right_stick == settings::RightStick::Camera) {
                // The game's own camera: the mouse switches its turn on while
                // it moves sideways. Not in the D-pad mode, where the same
                // bits move cursors in the game's menus.
                if (const int turn = camera::game_camera_mouse_stock_turn()) right_x = turn > 0 ? 0xFFu : 0x01u;
            }
        }
#endif
        auto &memory = rt.memory();
        for (std::uint32_t i = 0; i < count; ++i) {
            const std::uint32_t entry = address + i * 16u;
            memory.store32(entry, static_cast<std::uint32_t>(kernel().now_us()));
            memory.store32(entry + 4u, buttons);
            memory.store8(entry + 8u, analog_x);
            memory.store8(entry + 9u, analog_y);
            // Bytes 10 and 11 are the HD release's second stick, not padding.
            // Leaving them zero reads as a full diagonal deflection and turns
            // the camera every frame; 0x80 is the centre the guest tests for.
            memory.store8(entry + 10u, right_x);
            memory.store8(entry + 11u, right_y);
            for (std::uint32_t j = 12u; j < 16u; ++j) memory.store8(entry + j, 0u);
        }
        WaitState wait{};
        wait.type = WaitType::VBlank;
        kernel().block(ctx, wait, count);
    });
}

void register_ge(HleRegistrar &hle) {
    hle.add("sceGe_user", "sceGeEdramGetAddr", [](Runtime &, AllegrexContext &ctx) { kernel().finish(ctx, kEdramBase); });
    hle.add("sceGe_user", "sceGeEdramGetSize", [](Runtime &, AllegrexContext &ctx) { kernel().finish(ctx, kEdramSize); });
    hle.add("sceGe_user", "sceGeEdramSetAddrTranslation", [](Runtime &, AllegrexContext &ctx) { kernel().finish(ctx, 0u); });
    hle.add("sceGe_user", "sceGeSetCallback", [](Runtime &rt, AllegrexContext &ctx) {
        const std::uint32_t data = arg(ctx, 0);
        auto &memory = rt.memory();
        const std::int32_t id = media().next_ge_callback++;
        media().ge_callbacks[id] = GeCallback{memory.load32(data), memory.load32(data + 4u), memory.load32(data + 8u),
                                              memory.load32(data + 12u)};
        kernel().finish(ctx, static_cast<std::uint32_t>(id));
    });
    hle.add("sceGe_user", "sceGeUnsetCallback", [](Runtime &, AllegrexContext &ctx) {
        media().ge_callbacks.erase(static_cast<std::int32_t>(arg(ctx, 0)));
        kernel().finish(ctx, 0u);
    });
    const auto enqueue = [](Runtime &rt, AllegrexContext &ctx) {
        const std::uint32_t id = media().next_ge_list++;
        GeList list{};
        list.pc = arg(ctx, 0) & 0x0FFFFFFFu;
        list.stall = arg(ctx, 1) & 0x0FFFFFFFu;
        list.callback = static_cast<std::int32_t>(arg(ctx, 2));
        media().ge_lists[id] = std::move(list);
        perf::count_display_list();
        run_ge_list(rt, id);
        kernel().finish(ctx, id);
    };
    hle.add("sceGe_user", "sceGeListEnQueue", enqueue);
    hle.add("sceGe_user", "sceGeListEnQueueHead", enqueue);
    hle.add("sceGe_user", "sceGeListUpdateStallAddr", [](Runtime &rt, AllegrexContext &ctx) {
        auto found = media().ge_lists.find(arg(ctx, 0));
        if (found == media().ge_lists.end()) {
            kernel().finish(ctx, error::kIllegalArgument);
            return;
        }
        found->second.stall = arg(ctx, 1) & 0x0FFFFFFFu;
        run_ge_list(rt, arg(ctx, 0));
        kernel().finish(ctx, 0u);
    });
    hle.add("sceGe_user", "sceGeListSync", [](Runtime &, AllegrexContext &ctx) {
        auto found = media().ge_lists.find(arg(ctx, 0));
        // 0 = completed, 2 = still stalled (drawing).
        const std::uint32_t state = found == media().ge_lists.end() || found->second.done ? 0u : 2u;
        kernel().finish(ctx, state);
    });
    hle.add("sceGe_user", "sceGeDrawSync", [](Runtime &, AllegrexContext &ctx) {
        std::erase_if(media().ge_lists, [](const auto &item) { return item.second.done; });
        kernel().finish(ctx, 0u);
    });
    hle.add("sceGe_user", "sceGeBreak", [](Runtime &, AllegrexContext &ctx) { kernel().finish(ctx, 0u); });
    hle.add("sceGe_user", "sceGeContinue", [](Runtime &, AllegrexContext &ctx) { kernel().finish(ctx, 0u); });
}

// sceAudioOutput*Blocking returns once the previously submitted buffer has
// drained, which is what paces the guest's audio thread: the channel's queue
// is tracked in virtual time and the thread waits on the kernel clock, never
// on the host. The samples themselves go straight to the sink.
void audio_output(Runtime &rt, AllegrexContext &ctx) {
    const std::uint32_t channel = arg(ctx, 0);
    if (channel >= media().audio.size()) {
        kernel().finish(ctx, 0x80260002u);
        return;
    }
    AudioChannel &state = media().audio[channel];
    const std::uint32_t left = arg(ctx, 1);
    const std::uint32_t right = arg(ctx, 2);
    const std::uint32_t buffer = arg(ctx, 3);
    const std::uint32_t frames = state.samples;
    // Format 0x10 is mono: one sample per frame instead of a stereo pair.
    const bool mono = (state.format & 0x10u) != 0u;
    const std::size_t words = frames * (mono ? 1u : 2u);

    if (buffer != 0u && frames != 0u) {
        static std::vector<std::int16_t> staging;
        staging.resize(frames * 2u);
        if (const std::uint8_t *source = rt.memory().raw_pointer(buffer, words * 2u)) {
            for (std::uint32_t frame = 0; frame < frames; ++frame) {
                const std::size_t index = mono ? frame : frame * 2u;
                const auto sample = static_cast<std::int16_t>(source[index * 2u] | (source[index * 2u + 1u] << 8));
                staging[frame * 2u] = sample;
                staging[frame * 2u + 1u] = mono ? sample
                                                : static_cast<std::int16_t>(source[(index + 1u) * 2u] |
                                                                            (source[(index + 1u) * 2u + 1u] << 8));
            }
            // How loud the buffer is after the channel's volume, worked out
            // as the sink mixes it (0x8000 is full volume): 0 means the sink
            // would add nothing but zeros.
            const std::int32_t gains[2] = {static_cast<std::int32_t>(std::min<std::uint32_t>(left, 0x8000u)),
                                           static_cast<std::int32_t>(std::min<std::uint32_t>(right, 0x8000u))};
            int peak = 0;
            for (std::size_t i = 0; i < frames * 2u; ++i)
                peak = std::max(peak, std::abs((static_cast<std::int32_t>(staging[i]) * gains[i & 1u]) >> 15));
            load_trace::note_audio_peak(peak);
            // While a load runs faster than real time its silence is dropped:
            // played, it would pile up faster than the device plays it. The
            // channel's cursor stays where it was and catches up with the
            // device when sound comes back.
            if (!fast_loading::note_audio(peak))
                audio::AudioSink::instance().mix(state.cursor, staging.data(), frames, left, right);
        } else {
            log_once("audio-buffer", "[audio] output buffer is not a single mapped range; dropping it");
        }
    }

    const std::uint64_t now = kernel().now_us();
    if (state.queued_until_us < now) state.queued_until_us = now;
    const std::uint64_t previous_end = state.queued_until_us;
    state.queued_until_us += static_cast<std::uint64_t>(frames) * 1'000'000u / kAudioSampleRate;
    if (previous_end > now)
        kernel().delay_current(ctx, previous_end - now, frames);
    else
        kernel().finish(ctx, frames);
}

// __sceSasCore renders one grain into a guest buffer; the guest then hands that
// buffer to a sceAudio channel itself, so nothing here reaches the sink.
void sas_render(Runtime &rt, std::uint32_t core, std::uint32_t output, bool mix,
                std::uint32_t left_volume, std::uint32_t right_volume) {
    audio::SasCore &sas = audio::sas_core(core);
    const std::size_t frames = sas.grain();
    static std::vector<std::int16_t> staging;
    staging.resize(frames * 2u);
    sas.render(rt.memory(), staging.data(), frames);

    std::uint8_t *destination = rt.memory().raw_pointer(output, frames * 4u);
    if (destination == nullptr) {
        log_once("sas-output", "[sas] output buffer is not a single mapped range; dropping the grain");
        return;
    }
    for (std::size_t frame = 0; frame < frames; ++frame) {
        for (std::size_t side = 0; side < 2u; ++side) {
            const std::size_t index = frame * 2u + side;
            std::int32_t value = staging[index];
            if (mix) {
                const std::uint32_t gain = side == 0u ? left_volume : right_volume;
                value = (value * static_cast<std::int32_t>(std::min(gain, 0x1000u))) >> 12;
                value += static_cast<std::int16_t>(destination[index * 2u] | (destination[index * 2u + 1u] << 8));
                value = std::clamp(value, -32768, 32767);
            }
            destination[index * 2u] = static_cast<std::uint8_t>(value);
            destination[index * 2u + 1u] = static_cast<std::uint8_t>(static_cast<std::uint32_t>(value) >> 8u);
        }
    }
}

void register_audio(HleRegistrar &hle) {
    audio::AudioSink::instance().initialize();

    hle.add("sceAudio", "sceAudioChReserve", [](Runtime &, AllegrexContext &ctx) {
        auto channel = static_cast<std::int32_t>(arg(ctx, 0));
        auto &channels = media().audio;
        if (channel < 0) {
            channel = -1;
            for (std::size_t i = 0; i < channels.size(); ++i) {
                if (!channels[i].reserved) {
                    channel = static_cast<std::int32_t>(i);
                    break;
                }
            }
        }
        if (channel < 0 || channel >= static_cast<std::int32_t>(channels.size()) || channels[static_cast<std::size_t>(channel)].reserved) {
            kernel().finish(ctx, 0x80260002u);
            return;
        }
        channels[static_cast<std::size_t>(channel)] = AudioChannel{true, arg(ctx, 1), arg(ctx, 2), 0u, 0u};
        kernel().finish(ctx, static_cast<std::uint32_t>(channel));
    });
    hle.add("sceAudio", "sceAudioChRelease", [](Runtime &, AllegrexContext &ctx) {
        if (arg(ctx, 0) < media().audio.size()) media().audio[arg(ctx, 0)].reserved = false;
        kernel().finish(ctx, 0u);
    });
    hle.add("sceAudio", "sceAudioSetChannelDataLen", [](Runtime &, AllegrexContext &ctx) {
        if (arg(ctx, 0) < media().audio.size()) media().audio[arg(ctx, 0)].samples = arg(ctx, 1);
        kernel().finish(ctx, 0u);
    });
    hle.add("sceAudio", "sceAudioChangeChannelConfig", [](Runtime &, AllegrexContext &ctx) {
        if (arg(ctx, 0) < media().audio.size()) media().audio[arg(ctx, 0)].format = arg(ctx, 1);
        kernel().finish(ctx, 0u);
    });
    hle.add("sceAudio", "sceAudioChangeChannelVolume", [](Runtime &, AllegrexContext &ctx) { kernel().finish(ctx, 0u); });
    // How much of what the channel was given is still to play, in samples.
    hle.add("sceAudio", "sceAudioGetChannelRestLength", [](Runtime &, AllegrexContext &ctx) {
        const std::uint32_t channel = arg(ctx, 0);
        if (channel >= media().audio.size()) {
            kernel().finish(ctx, 0x80260002u);
            return;
        }
        const std::uint64_t now = kernel().now_us();
        const AudioChannel &state = media().audio[channel];
        const std::uint64_t remaining = state.queued_until_us > now ? state.queued_until_us - now : 0u;
        kernel().finish(ctx, static_cast<std::uint32_t>(remaining * kAudioSampleRate / 1'000'000u));
    });
    hle.add("sceAudio", "sceAudioOutputPannedBlocking", audio_output);
    hle.try_add("sceAudio", "sceAudioOutputPanned", audio_output);

    hle.add("sceSasCore", "__sceSasInit", [](Runtime &, AllegrexContext &ctx) {
        audio::sas_core(arg(ctx, 0)).init(arg(ctx, 1), arg(ctx, 2), arg(ctx, 3));
        kernel().finish(ctx, 0u);
    });
    hle.add("sceSasCore", "__sceSasSetVoice", [](Runtime &, AllegrexContext &ctx) {
        audio::sas_core(arg(ctx, 0)).set_voice(arg(ctx, 1), arg(ctx, 2), arg(ctx, 3), arg(ctx, 4) != 0u);
        kernel().finish(ctx, 0u);
    });
    hle.try_add("sceSasCore", "__sceSasSetVoicePCM", [](Runtime &, AllegrexContext &ctx) {
        audio::sas_core(arg(ctx, 0)).set_voice_pcm(arg(ctx, 1), arg(ctx, 2), arg(ctx, 3),
                                                   static_cast<std::int32_t>(arg(ctx, 4)));
        kernel().finish(ctx, 0u);
    });
    hle.add("sceSasCore", "__sceSasSetPitch", [](Runtime &, AllegrexContext &ctx) {
        audio::sas_core(arg(ctx, 0)).set_pitch(arg(ctx, 1), arg(ctx, 2));
        kernel().finish(ctx, 0u);
    });
    hle.add("sceSasCore", "__sceSasSetVolume", [](Runtime &, AllegrexContext &ctx) {
        audio::sas_core(arg(ctx, 0)).set_volume(arg(ctx, 1), static_cast<std::int32_t>(arg(ctx, 2)),
                                                static_cast<std::int32_t>(arg(ctx, 3)));
        kernel().finish(ctx, 0u);
    });
    hle.add("sceSasCore", "__sceSasSetSimpleADSR", [](Runtime &, AllegrexContext &ctx) {
        audio::sas_core(arg(ctx, 0)).set_simple_adsr(arg(ctx, 1), arg(ctx, 2), arg(ctx, 3));
        kernel().finish(ctx, 0u);
    });
    hle.add("sceSasCore", "__sceSasSetKeyOn", [](Runtime &, AllegrexContext &ctx) {
        audio::sas_core(arg(ctx, 0)).key_on(arg(ctx, 1));
        kernel().finish(ctx, 0u);
    });
    hle.add("sceSasCore", "__sceSasSetKeyOff", [](Runtime &, AllegrexContext &ctx) {
        audio::sas_core(arg(ctx, 0)).key_off(arg(ctx, 1));
        kernel().finish(ctx, 0u);
    });
    hle.try_add("sceSasCore", "__sceSasSetPause", [](Runtime &, AllegrexContext &ctx) {
        audio::sas_core(arg(ctx, 0)).set_pause(arg(ctx, 1), arg(ctx, 2) != 0u);
        kernel().finish(ctx, 0u);
    });
    hle.try_add("sceSasCore", "__sceSasGetEnvelopeHeight", [](Runtime &, AllegrexContext &ctx) {
        kernel().finish(ctx, static_cast<std::uint32_t>(audio::sas_core(arg(ctx, 0)).envelope_height(arg(ctx, 1))));
    });
    // Reverb is not modelled, so the sends are accepted and dropped.
    for (const char *name : {"__sceSasRevType", "__sceSasRevParam", "__sceSasRevEVOL", "__sceSasRevVON"})
        hle.add("sceSasCore", name, [](Runtime &, AllegrexContext &ctx) { kernel().finish(ctx, 0u); });
    hle.add("sceSasCore", "__sceSasGetOutputmode", [](Runtime &, AllegrexContext &ctx) {
        kernel().finish(ctx, audio::sas_core(arg(ctx, 0)).output_mode());
    });
    hle.add("sceSasCore", "__sceSasGetEndFlag", [](Runtime &, AllegrexContext &ctx) {
        kernel().finish(ctx, audio::sas_core(arg(ctx, 0)).end_flag());
    });
    hle.add("sceSasCore", "__sceSasCore", [](Runtime &rt, AllegrexContext &ctx) {
        sas_render(rt, arg(ctx, 0), arg(ctx, 1), false, 0u, 0u);
        kernel().finish(ctx, 0u);
    });
    hle.try_add("sceSasCore", "__sceSasCoreWithMix", [](Runtime &rt, AllegrexContext &ctx) {
        sas_render(rt, arg(ctx, 0), arg(ctx, 1), true, arg(ctx, 2), arg(ctx, 3));
        kernel().finish(ctx, 0u);
    });
}

} // namespace

void initialize_renderer() {
#if defined(MHP3RD_HAS_RENDERER)
    (void)ensure_renderer();
#else
    std::cout << "Renderer: not built\n";
#endif
}

void register_media(HleRegistrar &hle) {
    initialize_renderer();
    register_display_ctrl(hle);
    register_ge(hle);
    register_audio(hle);
}

#if defined(MHP3RD_HAS_RENDERER)
gpu::VulkanRenderer *active_renderer() {
    return media().renderer && media().renderer->available() ? media().renderer.get() : nullptr;
}

gpu::VulkanRenderer *ensure_renderer() {
    static bool tried = false;
    if (tried) return active_renderer();
    tried = true;
    if (std::getenv("MHP3RD_NO_RENDER") != nullptr) {
        std::cout << "Renderer: disabled by MHP3RD_NO_RENDER\n";
        return nullptr;
    }
    auto renderer = std::make_unique<gpu::VulkanRenderer>();
    std::string error;
    gpu::RendererConfig config;
    // Tells windows apart when several instances run side by side, e.g. two
    // players testing ad hoc play on one machine.
    if (const char *title = std::getenv("MHP3RD_WINDOW_TITLE"); title != nullptr && *title != '\0')
        config.title = title;
    if (!renderer->initialize(config, error)) {
        std::cerr << "Renderer: unavailable (" << error << "); running headless\n";
        return nullptr;
    }
    media().renderer = std::move(renderer);
    ui::attach(*media().renderer);
    // Frame interpolation presents between flips: while the kernel waits for
    // real time, and while the game's code runs.
    kernel().set_idle_hook([](std::chrono::steady_clock::time_point wake) {
        if (gpu::VulkanRenderer *active = active_renderer()) active->present_until(wake);
    });
    kernel().set_poll_hook([] {
        if (gpu::VulkanRenderer *active = active_renderer()) active->present_due();
    });
    return media().renderer.get();
}
#endif

} // namespace mhp3rd
