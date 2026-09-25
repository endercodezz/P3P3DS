#include "input/bindings.hpp"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <string>
#include <utility>

namespace mhp3rd::input {
namespace {

// Key positions by USB HID usage, the values SDL's scancodes have.
namespace hid {
constexpr std::uint16_t A = 4, D = 7, E = 8, F = 9, I = 12, J = 13, K = 14, L = 15, Q = 20, S = 22, W = 26;
constexpr std::uint16_t X = 27, Z = 29;
constexpr std::uint16_t Return = 40, Backspace = 42, Tab = 43, Space = 44;
constexpr std::uint16_t Right = 79, Left = 80, Down = 81, Up = 82;
constexpr std::uint16_t LeftShift = 225, RightShift = 229;
} // namespace hid

struct KeyName {
    std::uint16_t position;
    const char *name;
};

// Names as SDL gives them, except Enter and Esc, which is what the keys say,
// and the keypad's symbols, spelled out so no name holds the separator " / ".
constexpr KeyName kKeyNames[] = {
    {4, "A"}, {5, "B"}, {6, "C"}, {7, "D"}, {8, "E"}, {9, "F"}, {10, "G"}, {11, "H"}, {12, "I"},
    {13, "J"}, {14, "K"}, {15, "L"}, {16, "M"}, {17, "N"}, {18, "O"}, {19, "P"}, {20, "Q"}, {21, "R"},
    {22, "S"}, {23, "T"}, {24, "U"}, {25, "V"}, {26, "W"}, {27, "X"}, {28, "Y"}, {29, "Z"},
    {30, "1"}, {31, "2"}, {32, "3"}, {33, "4"}, {34, "5"}, {35, "6"}, {36, "7"}, {37, "8"}, {38, "9"},
    {39, "0"}, {40, "Enter"}, {41, "Esc"}, {42, "Backspace"}, {43, "Tab"}, {44, "Space"}, {45, "-"},
    {46, "="}, {47, "["}, {48, "]"}, {49, "\\"}, {50, "#"}, {51, ";"}, {52, "'"}, {53, "`"}, {54, ","},
    {55, "."}, {56, "/"}, {57, "CapsLock"}, {58, "F1"}, {59, "F2"}, {60, "F3"}, {61, "F4"}, {62, "F5"},
    {63, "F6"}, {64, "F7"}, {65, "F8"}, {66, "F9"}, {67, "F10"}, {68, "F11"}, {69, "F12"},
    {70, "PrintScreen"}, {71, "ScrollLock"}, {72, "Pause"}, {73, "Insert"}, {74, "Home"}, {75, "PageUp"},
    {76, "Delete"}, {77, "End"}, {78, "PageDown"}, {79, "Right"}, {80, "Left"}, {81, "Down"}, {82, "Up"},
    {83, "Numlock"}, {84, "Keypad Divide"}, {85, "Keypad Multiply"}, {86, "Keypad Minus"}, {87, "Keypad Plus"},
    {88, "Keypad Enter"}, {89, "Keypad 1"}, {90, "Keypad 2"}, {91, "Keypad 3"}, {92, "Keypad 4"},
    {93, "Keypad 5"}, {94, "Keypad 6"}, {95, "Keypad 7"}, {96, "Keypad 8"}, {97, "Keypad 9"},
    {98, "Keypad 0"}, {99, "Keypad Period"}, {100, "NonUSBackslash"}, {224, "Left Ctrl"}, {225, "Left Shift"},
    {226, "Left Alt"}, {227, "Left GUI"}, {228, "Right Ctrl"}, {229, "Right Shift"}, {230, "Right Alt"},
    {231, "Right GUI"},
};
constexpr const char *kMouseNames[] = {"Mouse Left", "Mouse Middle", "Mouse Right", "Mouse 4", "Mouse 5"};
constexpr const char *kSeparator = " / ";

constexpr ActionInfo kInfo[kActions] = {
    {"stick_up", "Move forward"},  {"stick_left", "Move left"},    {"stick_down", "Move back"},
    {"stick_right", "Move right"}, {"triangle", "△"},              {"circle", "○  (confirm)"},
    {"cross", "×  (back)"},        {"square", "□"},                {"l", "L"},
    {"r", "R"},                    {"start", "START"},             {"select", "SELECT"},
    {"dpad_up", "D-pad up"},       {"dpad_left", "D-pad left"},    {"dpad_down", "D-pad down"},
    {"dpad_right", "D-pad right"}, {"camera_up", "Camera up"},     {"camera_left", "Camera left"},
    {"camera_down", "Camera down"}, {"camera_right", "Camera right"},
};

// SceCtrlButtons for the actions that are buttons.
constexpr std::uint32_t kButtonBits[kActions] = {
    0u, 0u, 0u, 0u, 0x1000u, 0x2000u, 0x4000u, 0x8000u, 0x0100u, 0x0200u, 0x0008u, 0x0001u,
    0x0010u, 0x0080u, 0x0040u, 0x0020u, 0u, 0u, 0u, 0u,
};

Bindings make_default() {
    Bindings b{};
    const auto set = [&](Action action, Binding first, Binding second = kNone) {
        b[static_cast<std::size_t>(action)] = {first, second};
    };
    set(Action::StickUp, key(hid::W));
    set(Action::StickLeft, key(hid::A));
    set(Action::StickDown, key(hid::S));
    set(Action::StickRight, key(hid::D));
    // The game's attacks on the mouse, as on most PC action games; ○ also
    // talks and confirms, so it has a key too.
    set(Action::Triangle, mouse_button(1));
    set(Action::Circle, mouse_button(3), key(hid::F));
    set(Action::Cross, key(hid::Space));
    set(Action::Square, key(hid::E));
    // L puts the camera behind the hunter, R guards, runs and aims.
    set(Action::L, key(hid::Q));
    set(Action::R, key(hid::LeftShift));
    set(Action::Start, key(hid::Return), key(hid::Tab));
    set(Action::Select, key(hid::Backspace));
    set(Action::Up, key(hid::Up));
    set(Action::Left, key(hid::Left));
    set(Action::Down, key(hid::Down));
    set(Action::Right, key(hid::Right));
    // The camera without a mouse, where the old layout moved the hunter.
    set(Action::CameraUp, key(hid::I));
    set(Action::CameraLeft, key(hid::J));
    set(Action::CameraDown, key(hid::K));
    set(Action::CameraRight, key(hid::L));
    return b;
}

Bindings make_classic() {
    Bindings b{};
    const auto set = [&](Action action, Binding first, Binding second = kNone) {
        b[static_cast<std::size_t>(action)] = {first, second};
    };
    set(Action::StickUp, key(hid::I));
    set(Action::StickLeft, key(hid::J));
    set(Action::StickDown, key(hid::K));
    set(Action::StickRight, key(hid::L));
    set(Action::Triangle, key(hid::S));
    set(Action::Circle, key(hid::X));
    set(Action::Cross, key(hid::Z));
    set(Action::Square, key(hid::A));
    set(Action::L, key(hid::Q));
    set(Action::R, key(hid::W));
    set(Action::Start, key(hid::Return));
    set(Action::Select, key(hid::RightShift), key(hid::Backspace));
    set(Action::Up, key(hid::Up));
    set(Action::Left, key(hid::Left));
    set(Action::Down, key(hid::Down));
    set(Action::Right, key(hid::Right));
    return b;
}

bool equal_ignoring_case(std::string_view a, std::string_view b) {
    return a.size() == b.size() && std::equal(a.begin(), a.end(), b.begin(), [](char x, char y) {
               return std::tolower(static_cast<unsigned char>(x)) == std::tolower(static_cast<unsigned char>(y));
           });
}

std::string_view trim(std::string_view text) {
    while (!text.empty() && std::isspace(static_cast<unsigned char>(text.front()))) text.remove_prefix(1);
    while (!text.empty() && std::isspace(static_cast<unsigned char>(text.back()))) text.remove_suffix(1);
    return text;
}

} // namespace

const Bindings &default_bindings() {
    static const Bindings value = make_default();
    return value;
}

const Bindings &classic_bindings() {
    static const Bindings value = make_classic();
    return value;
}

const ActionInfo &info(Action action) { return kInfo[static_cast<std::size_t>(action)]; }

std::string name(Binding binding) {
    if (const int button = mouse_button_of(binding)) return kMouseNames[button - 1];
    const int position = key_position(binding);
    if (position < 0) return {};
    for (const KeyName &k : kKeyNames)
        if (k.position == position) return k.name;
    return "Key " + std::to_string(position);
}

Binding from_name(std::string_view text) {
    text = trim(text);
    for (std::size_t i = 0; i < std::size(kMouseNames); ++i)
        if (equal_ignoring_case(text, kMouseNames[i])) return mouse_button(static_cast<int>(i) + 1);
    for (const KeyName &k : kKeyNames)
        if (equal_ignoring_case(text, k.name)) return key(k.position);
    // SDL's own names for the two keys named differently here.
    if (equal_ignoring_case(text, "Return")) return key(hid::Return);
    if (equal_ignoring_case(text, "Escape")) return key(41);
    if (text.size() > 4u && equal_ignoring_case(text.substr(0, 4), "Key ")) {
        const std::string number(text.substr(4));
        char *end = nullptr;
        const unsigned long value = std::strtoul(number.c_str(), &end, 10);
        if (end != number.c_str() && *end == '\0' && value > 0u && value < kKeyPositions)
            return key(static_cast<std::uint16_t>(value));
    }
    return kNone;
}

std::string format(const Slots &slots) {
    std::string text;
    for (const Binding binding : slots) {
        if (binding == kNone) continue;
        if (!text.empty()) text += kSeparator;
        text += name(binding);
    }
    return text;
}

bool parse(std::string_view text, Slots &slots) {
    Slots parsed{};
    std::size_t count = 0;
    text = trim(text);
    while (!text.empty()) {
        // " / " separates; a lone "/" is the slash key.
        const std::size_t split = text.find(kSeparator);
        const std::string_view part = split == std::string_view::npos ? text : text.substr(0, split);
        text = split == std::string_view::npos ? std::string_view{} : text.substr(split + 3u);
        const Binding binding = from_name(part);
        if (binding == kNone || count == kSlots) return false;
        parsed[count++] = binding;
    }
    slots = parsed;
    return true;
}

void assign(Bindings &bindings, Action action, Binding binding) {
    if (binding == kNone) return;
    Slots &slots = bindings[static_cast<std::size_t>(action)];
    const auto remove = [binding](Slots &s) {
        const auto end = std::remove(s.begin(), s.end(), binding);
        const bool found = end != s.end();
        std::fill(end, s.end(), kNone);
        return found;
    };
    if (remove(slots)) return;
    for (Slots &other : bindings) remove(other);
    for (Binding &slot : slots) {
        if (slot != kNone) continue;
        slot = binding;
        return;
    }
    slots.back() = binding;
}

PadInput read(const Bindings &bindings, const std::function<bool(Binding)> &held) {
    PadInput pad;
    bool on[kActions]{};
    for (std::size_t i = 0; i < kActions; ++i) {
        for (const Binding binding : bindings[i]) on[i] = on[i] || (binding != kNone && held(binding));
        if (on[i]) pad.buttons |= kButtonBits[i];
    }
    const auto axis = [&](Action negative, Action positive) {
        return (on[static_cast<std::size_t>(positive)] ? 127 : 0) - (on[static_cast<std::size_t>(negative)] ? 127 : 0);
    };
    pad.stick_x = axis(Action::StickLeft, Action::StickRight);
    pad.stick_y = axis(Action::StickUp, Action::StickDown);
    pad.camera_x = axis(Action::CameraLeft, Action::CameraRight);
    pad.camera_y = axis(Action::CameraUp, Action::CameraDown);
    return pad;
}

MouseTurn mouse_turn(float counts_x, float counts_y, float degrees_per_count, bool invert_x, bool invert_y,
                     float scale) {
    const float factor = degrees_per_count * scale;
    return {counts_x * factor * (invert_x ? -1.0f : 1.0f), counts_y * factor * (invert_y ? -1.0f : 1.0f)};
}

} // namespace mhp3rd::input
