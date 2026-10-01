#pragma once
// Host input for sceCtrl. An InputSource is sampled with the current vblank
// count (virtual clock, ThreadManager::vblank_count), so a scripted source
// replays bit-identically. Button bits: psp/pspsdk/src/ctrl/pspctrl.h and
// references/uofw/include/ctrl.h (identical values for user buttons).
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace p3p3ds::input {

namespace button {
constexpr std::uint32_t Select = 0x0001u, Start = 0x0008u, Up = 0x0010u, Right = 0x0020u, Down = 0x0040u,
                        Left = 0x0080u, LTrigger = 0x0100u, RTrigger = 0x0200u, Triangle = 0x1000u,
                        Circle = 0x2000u, Cross = 0x4000u, Square = 0x8000u;
} // namespace button

// Analog stick: 0..255 per axis, 128 = centre (pspctrl.h SceCtrlData Lx/Ly).
struct PadState {
    std::uint32_t buttons{0};
    std::uint8_t analog_x{128}, analog_y{128};
    bool operator==(const PadState &) const = default;
};

class InputSource {
public:
    virtual ~InputSource() = default;
    virtual PadState sample(std::uint64_t vblank) = 0;
};

// Button name ("START", "CROSS", "UP", ...; case-insensitive) -> bit.
std::optional<std::uint32_t> button_from_name(std::string_view name);
// "START+CROSS" / "none" / "-" -> mask; nullopt on an unknown name.
std::optional<std::uint32_t> buttons_from_text(std::string_view text);

// Vblank-keyed input script. One event per line, '#' starts a comment:
//   <vblank> <buttons>                 state from this vblank on
//   <vblank> <buttons> <duration>      hold for <duration> vblanks, then release
//   <vblank> stick <x> <y>             analog stick from this vblank on
// <buttons> is "START", "UP+CROSS", "none" or "-". Events must be in
// non-decreasing vblank order; a later event at the same vblank wins.
class InputScript final : public InputSource {
public:
    // Throws std::runtime_error("input script line N: ...") on malformed input.
    static InputScript parse(std::string_view text);
    PadState sample(std::uint64_t vblank) override;
    [[nodiscard]] std::size_t event_count() const noexcept { return events_.size(); }

private:
    struct Event {
        std::uint64_t vblank{};
        enum class Kind { Buttons, Stick } kind{Kind::Buttons};
        std::uint32_t buttons{};
        std::uint8_t x{128}, y{128};
    };
    std::vector<Event> events_;
};

} // namespace p3p3ds::input
