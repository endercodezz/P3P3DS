#pragma once

#include "framebuffer_capture.hpp"
#include "psprecomp/guest_memory.hpp"

#include <cstddef>
#include <cstdint>
#include <span>

namespace vcs {

// Optional native presentation window.  The emulation thread only decodes the
// PSP display framebuffer and hands the pixels over; a dedicated UI thread owns
// the HWND so the window keeps repainting and stays draggable even while the
// guest is inside a long synchronous loading phase.
//
// Enabled with PSPRECOMP_WINDOW=1.  PSPRECOMP_WINDOW_SCALE selects the integer
// scale factor (default 2).  Without the variable the host behaves exactly as
// before and stays completely headless.
[[nodiscard]] bool display_window_enabled();

// Opens the window before the guest produces its first framebuffer so the boot
// is visible from the start.  No-op when the window is disabled.
void display_window_start();

// Replaces the status text shown in the title bar and over an empty frame.
void display_window_set_status(const char *status);

// Publishes one PSP display frame.  Safe to call when the window is disabled.
void display_window_present(const psprecomp::GuestMemory &memory,
                            const FramebufferDescription &description);

// Publishes a tightly packed RGBA8 host frame. Used by the experimental true
// internal-resolution Vulkan preview; unlike StretchDIBits this frame was
// rasterized at the supplied dimensions.
void display_window_present_rgba(std::span<const std::byte> rgba,
                                 std::uint32_t width,
                                 std::uint32_t height);

// Pins the next frames to the source aspect, black-barred, whatever
// Display.AspectRatio says.
//
// For full-motion video. The widescreen fix widens the world by widening the
// frustum, which is right for a scene the game renders and wrong for a picture
// that already exists: a 480x272 movie frame has no more image at the sides to
// reveal, so filling a 21:9 panel with it can only stretch it. Pillarboxing
// shows it at the shape it was authored in.
void display_window_set_aspect_lock(bool locked) noexcept;

// Live PSP button mask sampled from the host keyboard, or 0 when the window is
// disabled or unfocused.  This is ORed with the deterministic vblank pulses so
// scripted validation runs keep behaving identically.
[[nodiscard]] std::uint32_t display_window_buttons();

// Analog stick sample derived from the host keyboard (128,128 when centered).
void display_window_analog(std::uint8_t &x, std::uint8_t &y);

// One poll of the host's keyboard, mouse and pad, already reduced to what the
// PSP pad can express -- plus the one thing it cannot.
//
// The PSP has a single stick, so the game has no register to read a camera
// from. camera_x/camera_y carry the mouse motion and the pad's right stick for
// the guest-side hook that gives the camera a second axis; nothing in the
// stock sceCtrl path looks at them.
struct HostInputState {
    std::uint32_t buttons{};
    std::uint8_t analog_x{128u};
    std::uint8_t analog_y{128u};
    // -127..127, zero at rest.
    int camera_x{};
    int camera_y{};
    // Throttle and brake, kept apart from the button mask on purpose. They
    // reach the guest through the vehicle's own accessors rather than through
    // Cross and Square, so that W and S drive a car without sprinting and
    // jumping on foot. See vcs_vehicle_input.hpp.
    bool accelerate{};
    bool brake{};
};
[[nodiscard]] HostInputState display_window_input();

// True once the user closed the window or pressed Escape.
[[nodiscard]] bool display_window_close_requested();

void display_window_shutdown();

// Native window handle (HWND on Windows, nullptr elsewhere or before the window
// exists) plus its client size. The Vulkan backend needs these to create a
// surface and size its swapchain; it presents directly instead of reading the
// rendered image back to system memory.
struct DisplayWindowSurface {
    void *window{nullptr};
    void *instance{nullptr};
    std::uint32_t width{};
    std::uint32_t height{};
};
[[nodiscard]] DisplayWindowSurface display_window_surface();

} // namespace vcs
