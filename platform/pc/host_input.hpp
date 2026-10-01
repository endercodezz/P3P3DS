#pragma once
// PC host input sources for sceCtrl (core/include/p3p3ds/input.hpp).
//
// - XInput gamepad (--gamepad): polled live from the OS xinput1_4.dll,
//   loaded at runtime (no SDK/import library needed). Without a window the
//   run is not paced to wall time, so live input is only useful for manual
//   experiments; deterministic checks use --input scripts.
// - Keyboard: the default key map below is data for a future window backend;
//   there is no keyboard polling without a focused window.
//
// XInput bit values: Microsoft XInput.h XINPUT_GAMEPAD_* (documented ABI).
#include "p3p3ds/input.hpp"

#include <algorithm>
#include <cstdint>
#include <memory>
#include <string_view>
#include <vector>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

namespace p3p3ds::pc {

namespace xinput {
constexpr std::uint16_t DpadUp = 0x0001, DpadDown = 0x0002, DpadLeft = 0x0004, DpadRight = 0x0008, Start = 0x0010,
                        Back = 0x0020, LeftShoulder = 0x0100, RightShoulder = 0x0200, A = 0x1000, B = 0x2000,
                        X = 0x4000, Y = 0x8000;
struct Gamepad { // XINPUT_GAMEPAD layout
    std::uint16_t buttons;
    std::uint8_t left_trigger, right_trigger;
    std::int16_t thumb_lx, thumb_ly, thumb_rx, thumb_ry;
};
} // namespace xinput

// Xbox layout -> PSP: A=Cross, B=Circle, X=Square, Y=Triangle (face positions),
// shoulders or triggers = L/R, Back = Select. Left stick -> analog (Y inverted:
// XInput up is positive, PSP up is 0).
inline input::PadState map_xinput(const xinput::Gamepad &g) {
    using namespace input::button;
    static constexpr std::pair<std::uint16_t, std::uint32_t> kMap[] = {
        {xinput::DpadUp, Up}, {xinput::DpadDown, Down}, {xinput::DpadLeft, Left}, {xinput::DpadRight, Right},
        {xinput::Start, Start}, {xinput::Back, Select}, {xinput::LeftShoulder, LTrigger},
        {xinput::RightShoulder, RTrigger}, {xinput::A, Cross}, {xinput::B, Circle}, {xinput::X, Square},
        {xinput::Y, Triangle}};
    input::PadState pad;
    for (const auto &[from, to] : kMap)
        if (g.buttons & from) pad.buttons |= to;
    if (g.left_trigger > 30) pad.buttons |= LTrigger;
    if (g.right_trigger > 30) pad.buttons |= RTrigger;
    auto axis = [](std::int16_t v) { return static_cast<std::uint8_t>(std::clamp((static_cast<int>(v) + 32768) >> 8, 0, 255)); };
    pad.analog_x = axis(g.thumb_lx);
    pad.analog_y = static_cast<std::uint8_t>(255 - axis(g.thumb_ly));
    return pad;
}

// Default keyboard map (Windows virtual-key codes) for a window backend.
struct KeyBinding { std::uint16_t virtual_key; std::uint32_t button; };
inline constexpr KeyBinding kDefaultKeys[] = {
    {0x26, input::button::Up}, {0x28, input::button::Down}, {0x25, input::button::Left}, {0x27, input::button::Right}, // arrows
    {0x0D, input::button::Start}, {0x08, input::button::Select},                                                     // Enter, Backspace
    {'X', input::button::Cross}, {'Z', input::button::Circle}, {'S', input::button::Square}, {'A', input::button::Triangle},
    {'Q', input::button::LTrigger}, {'W', input::button::RTrigger},
};

#ifdef _WIN32
class XInputSource final : public input::InputSource {
public:
    XInputSource() {
        module_ = LoadLibraryA("xinput1_4.dll");
        if (module_) get_state_ = reinterpret_cast<GetState>(reinterpret_cast<void *>(GetProcAddress(module_, "XInputGetState")));
    }
    ~XInputSource() override { if (module_) FreeLibrary(module_); }
    [[nodiscard]] bool available() const noexcept { return get_state_ != nullptr; }
    input::PadState sample(std::uint64_t) override {
        struct { DWORD packet; xinput::Gamepad pad; } state{};
        if (!get_state_ || get_state_(0, &state) != 0) return {};
        return map_xinput(state.pad);
    }
private:
    using GetState = DWORD(WINAPI *)(DWORD, void *);
    HMODULE module_{};
    GetState get_state_{};
};
#endif

// ORs the buttons of several sources; the first non-centred stick wins.
class CombinedSource final : public input::InputSource {
public:
    void add(std::shared_ptr<input::InputSource> s) { sources_.push_back(std::move(s)); }
    [[nodiscard]] bool empty() const noexcept { return sources_.empty(); }
    input::PadState sample(std::uint64_t vblank) override {
        input::PadState out;
        bool stick = false;
        for (auto &s : sources_) {
            const auto p = s->sample(vblank);
            out.buttons |= p.buttons;
            if (!stick && (p.analog_x != 128 || p.analog_y != 128)) { out.analog_x = p.analog_x; out.analog_y = p.analog_y; stick = true; }
        }
        return out;
    }
private:
    std::vector<std::shared_ptr<input::InputSource>> sources_;
};

} // namespace p3p3ds::pc
