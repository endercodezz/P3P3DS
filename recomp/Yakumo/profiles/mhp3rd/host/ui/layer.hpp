#pragma once

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <optional>
#include <string>

union SDL_Event;
struct ImFont;

namespace mhp3rd::gpu {
class VulkanRenderer;
}

// Shared machinery of the interface screens: the Dear ImGui context on the
// renderer's window, the theme and fonts scaled to the window, and input.
//
// Input. While a screen runs (the menu, the setup) it is "interactive": window
// events go to ImGui and none reach the game. While the game runs, events are
// only watched for the menu buttons.
//
// Esc is special. Steam's desktop controller layout on a Steam Deck sends Esc
// together with the B button, and B is a button the game and the menu use. An
// Esc that arrives within a moment of a gamepad button press is therefore
// dropped; a real key press is acted on a few frames late, which nobody notices.
namespace mhp3rd::ui {

enum class InputDevice { Keyboard, Gamepad };

class Layer {
public:
    static Layer &get();

    bool attach(gpu::VulkanRenderer &renderer);
    [[nodiscard]] bool attached() const noexcept { return renderer_ != nullptr; }
    [[nodiscard]] gpu::VulkanRenderer &renderer() noexcept { return *renderer_; }

    void set_interactive(bool interactive);

    // Runs interface frames until `frame` returns false: pumps events, starts
    // an ImGui frame, calls `frame` to build it and presents it over the last
    // game frame (show_game) or a plain background. False if the window was
    // closed meanwhile.
    bool run(const std::function<bool()> &frame, bool show_game);

    // One frame drawn over the running game, outside run().
    void begin_frame();
    void end_frame();

    // Requests from the player, each reported once.
    bool take_menu_toggle();  // L3+R3; Esc while the game runs
    bool take_back();         // Esc while a screen runs
    std::optional<std::filesystem::path> take_dropped_file();
    [[nodiscard]] bool window_closed() const noexcept { return window_closed_; }

    // Binding a control (the menu's keyboard and mouse rows): the next key or
    // mouse button pressed is kept for the caller instead of reaching the
    // interface. Esc or a gamepad button cancels.
    void begin_binding_capture();
    [[nodiscard]] bool capturing_binding() const noexcept { return capturing_binding_; }
    // Once capture has ended: what was pressed, or input::kNone if cancelled.
    std::optional<std::uint16_t> take_captured_binding();

    // False while a face button held since the screen opened is still down;
    // gamepad presses count only once it is released.
    [[nodiscard]] bool gamepad_armed() const noexcept { return gamepad_armed_; }

    // What the player last used, for the button hints.
    [[nodiscard]] InputDevice input_device() const noexcept { return device_; }
    // Whether confirm is the south face button (the pad setting); the menu
    // follows the game's convention.
    [[nodiscard]] bool confirm_south() const;

    // Text size in pixels for the current window, and the matching factor
    // for spacing (1 at 20 px).
    [[nodiscard]] float font_size() const noexcept { return font_size_; }
    [[nodiscard]] float scale() const noexcept { return font_size_ / 20.0f; }

    // Explanation of the focused row, shown above the button hints.
    void set_description(const std::string &text) { description_ = text; }
    [[nodiscard]] const std::string &description() const noexcept { return description_; }

private:
    bool handle_event(const SDL_Event &event);
    void resolve_escape();
    void apply_theme();

    gpu::VulkanRenderer *renderer_{};
    bool interactive_{};
    bool window_closed_{};
    InputDevice device_{InputDevice::Keyboard};
    float font_size_{};
    std::string description_;

    using Clock = std::chrono::steady_clock;
    std::optional<Clock::time_point> escape_pending_;
    Clock::time_point last_pad_button_{};
    // Gamepad navigation waits until no face button is held, so a button held
    // while a screen opens does not activate its first row.
    bool gamepad_armed_{};
    bool menu_toggle_{};
    bool back_{};
    bool capturing_binding_{};
    std::optional<std::uint16_t> captured_binding_;
    std::optional<std::filesystem::path> dropped_;
};

} // namespace mhp3rd::ui
