#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <string_view>

// Keyboard and mouse bindings for the game (#94): which keys and mouse
// buttons press each PSP control, and the mouse's turn in degrees.
//
// A binding is a key, by its position on the keyboard (a USB HID usage, the
// numbers SDL's scancodes use), or a mouse button. Positions rather than
// characters, so W A S D stay where they are on any keyboard layout. Nothing
// here needs SDL, so settings.ini is read the same way in every build.
namespace mhp3rd::input {

enum class Action : std::uint8_t {
    StickUp,
    StickLeft,
    StickDown,
    StickRight,
    Triangle,
    Circle,
    Cross,
    Square,
    L,
    R,
    Start,
    Select,
    Up,
    Left,
    Down,
    Right,
    // The HD release's second stick, pushed fully that way while held.
    CameraUp,
    CameraLeft,
    CameraDown,
    CameraRight,
    Count
};
inline constexpr std::size_t kActions = static_cast<std::size_t>(Action::Count);
// Every action takes up to two keys or buttons.
inline constexpr std::size_t kSlots = 2u;

// 0: none. 1-511: a key position. kMouse + 1..5: a mouse button, numbered as
// SDL numbers them (left, middle, right, back, forward).
using Binding = std::uint16_t;
inline constexpr Binding kNone = 0u;
inline constexpr Binding kMouse = 0x200u;
inline constexpr std::uint16_t kKeyPositions = 512u;
[[nodiscard]] constexpr Binding key(std::uint16_t position) { return position; }
[[nodiscard]] constexpr Binding mouse_button(int button) { return static_cast<Binding>(kMouse + button); }
// The key position, or -1 for a mouse button or none.
[[nodiscard]] constexpr int key_position(Binding binding) {
    return binding != kNone && binding < kKeyPositions ? binding : -1;
}
// The mouse button, 1-5, or 0 for a key or none.
[[nodiscard]] constexpr int mouse_button_of(Binding binding) {
    return binding > kMouse && binding <= kMouse + 5u ? binding - kMouse : 0;
}

using Slots = std::array<Binding, kSlots>;
using Bindings = std::array<Slots, kActions>;

// W A S D to move with the mouse in the other hand: the default.
[[nodiscard]] const Bindings &default_bindings();
// The keyboard-only layout of earlier versions: I J K L to move and the face
// buttons on Z X A S.
[[nodiscard]] const Bindings &classic_bindings();

struct ActionInfo {
    const char *key;    // in settings.ini, after "input.bind."
    const char *label;  // in the menu
};
[[nodiscard]] const ActionInfo &info(Action action);

// "W", "Left Shift", "Mouse Left"; "Key 123" for a position without a name.
[[nodiscard]] std::string name(Binding binding);
// The reverse of name(), ignoring case. kNone if the name is not known.
[[nodiscard]] Binding from_name(std::string_view text);
// Slots as settings.ini keeps them: names separated by " / ", empty for none.
[[nodiscard]] std::string format(const Slots &slots);
// False, leaving `slots` alone, if a name is not known.
bool parse(std::string_view text, Slots &slots);

// The menu's way of changing a binding: pressing what is already bound to
// the action removes it; anything else is taken from any other action and
// added, replacing the second slot when both are full.
void assign(Bindings &bindings, Action action, Binding binding);

// What the held keys and buttons press, in PSP terms.
struct PadInput {
    std::uint32_t buttons{};  // SceCtrlButtons
    int stick_x{};            // -127..127 from the centre, each axis
    int stick_y{};
    int camera_x{};           // the second stick
    int camera_y{};
};
[[nodiscard]] PadInput read(const Bindings &bindings, const std::function<bool(Binding)> &held);

// The mouse's motion as degrees for the camera: positive yaw turns right,
// positive pitch looks down, as camera_input expects. `counts` are relative
// motion; `scale` is what the current camera makes of it (1, or Aim speed's
// share of Camera speed while a bow or a bowgun aims).
struct MouseTurn {
    float yaw{};
    float pitch{};
};
[[nodiscard]] MouseTurn mouse_turn(float counts_x, float counts_y, float degrees_per_count, bool invert_x,
                                   bool invert_y, float scale);

} // namespace mhp3rd::input
