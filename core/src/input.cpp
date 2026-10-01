// Host input sources (see input.hpp).
#include "p3p3ds/input.hpp"

#include <algorithm>
#include <cctype>
#include <sstream>
#include <stdexcept>

namespace p3p3ds::input {
namespace {
std::string upper(std::string_view s) {
    std::string out(s);
    for (auto &c : out) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    return out;
}
} // namespace

std::optional<std::uint32_t> button_from_name(std::string_view name) {
    static constexpr std::pair<std::string_view, std::uint32_t> kNames[] = {
        {"SELECT", button::Select}, {"START", button::Start}, {"UP", button::Up}, {"RIGHT", button::Right},
        {"DOWN", button::Down}, {"LEFT", button::Left}, {"L", button::LTrigger}, {"LTRIGGER", button::LTrigger},
        {"R", button::RTrigger}, {"RTRIGGER", button::RTrigger}, {"TRIANGLE", button::Triangle},
        {"CIRCLE", button::Circle}, {"CROSS", button::Cross}, {"SQUARE", button::Square},
    };
    const auto key = upper(name);
    for (const auto &[n, bit] : kNames)
        if (key == n) return bit;
    return std::nullopt;
}

std::optional<std::uint32_t> buttons_from_text(std::string_view text) {
    if (text == "-" || upper(text) == "NONE") return 0u;
    std::uint32_t mask = 0;
    std::size_t start = 0;
    while (start <= text.size()) {
        const auto end = std::min(text.find('+', start), text.size());
        const auto bit = button_from_name(text.substr(start, end - start));
        if (!bit) return std::nullopt;
        mask |= *bit;
        start = end + 1;
    }
    return mask;
}

InputScript InputScript::parse(std::string_view text) {
    InputScript script;
    std::istringstream in{std::string(text)};
    std::string line;
    std::size_t number = 0;
    std::uint64_t last = 0;
    auto fail = [&](const std::string &why) {
        throw std::runtime_error("input script line " + std::to_string(number) + ": " + why);
    };
    std::vector<Event> releases; // pending "hold for N" releases, merged below
    while (std::getline(in, line)) {
        ++number;
        if (const auto hash = line.find('#'); hash != std::string::npos) line.resize(hash);
        std::istringstream words(line);
        std::string first;
        if (!(words >> first)) continue;
        std::uint64_t vblank = 0;
        try { vblank = std::stoull(first); } catch (const std::exception &) { fail("expected a vblank number, got '" + first + "'"); }
        if (vblank < last) fail("vblank " + first + " is earlier than the previous event");
        last = vblank;
        std::string what;
        if (!(words >> what)) fail("missing buttons");
        Event e;
        e.vblank = vblank;
        if (upper(what) == "STICK") {
            int x = -1, y = -1;
            if (!(words >> x >> y) || x < 0 || x > 255 || y < 0 || y > 255) fail("stick needs x y in 0..255");
            e.kind = Event::Kind::Stick;
            e.x = static_cast<std::uint8_t>(x);
            e.y = static_cast<std::uint8_t>(y);
            script.events_.push_back(e);
        } else {
            const auto mask = buttons_from_text(what);
            if (!mask) fail("unknown button in '" + what + "'");
            e.buttons = *mask;
            script.events_.push_back(e);
            std::uint64_t duration = 0;
            if (words >> duration) {
                if (duration == 0) fail("duration must be positive");
                Event release;
                release.vblank = vblank + duration;
                release.buttons = 0;
                releases.push_back(release);
            }
        }
        std::string extra;
        if (words >> extra) fail("unexpected '" + extra + "'");
    }
    // Releases go before explicit events at the same vblank so an explicit
    // state at that vblank wins.
    for (const auto &r : releases) {
        auto at = std::lower_bound(script.events_.begin(), script.events_.end(), r.vblank,
                                   [](const Event &e, std::uint64_t v) { return e.vblank < v; });
        script.events_.insert(at, r);
    }
    return script;
}

PadState InputScript::sample(std::uint64_t vblank) {
    PadState state;
    for (const auto &e : events_) {
        if (e.vblank > vblank) break;
        if (e.kind == Event::Kind::Stick) { state.analog_x = e.x; state.analog_y = e.y; }
        else state.buttons = e.buttons;
    }
    return state;
}

} // namespace p3p3ds::input
