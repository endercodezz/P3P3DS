#include "camera/camera_input.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <utility>

namespace mhp3rd::camera {
namespace {

constexpr std::size_t kSources = static_cast<std::size_t>(Source::Count);
// A frame longer than this is a pause (the menu, a load), not motion to catch
// up on: one step at most, whatever the stick held meanwhile.
constexpr float kLongestStep = 0.1f;

std::array<Rate, kSources> rates{};
// Degrees from the held rates, and from each motion source on its own.
Turn pending{};
std::array<Turn, kSources> motion{};

void add(Turn &to, const Turn &from) {
    to.yaw_degrees += from.yaw_degrees;
    to.pitch_degrees += from.pitch_degrees;
    to.yaw_held |= from.yaw_held;
    to.pitch_held |= from.pitch_held;
}

} // namespace

void set_rate(Source source, float yaw, float pitch) {
    const auto index = static_cast<std::size_t>(source);
    if (index >= kSources) return;
    rates[index] = Rate{std::clamp(yaw, -1.0f, 1.0f), std::clamp(pitch, -1.0f, 1.0f)};
}

void add_motion(Source source, float yaw_degrees, float pitch_degrees) {
    const auto index = static_cast<std::size_t>(source);
    if (index >= kSources || !std::isfinite(yaw_degrees) || !std::isfinite(pitch_degrees)) return;
    add(motion[index], Turn{yaw_degrees, pitch_degrees, yaw_degrees != 0.0f, pitch_degrees != 0.0f});
}

void advance(float seconds, float degrees_per_second) {
    if (!std::isfinite(seconds) || seconds <= 0.0f) return;
    const float step = std::min(seconds, kLongestStep) * degrees_per_second;
    for (const Rate &rate : rates) {
        pending.yaw_degrees += rate.yaw * step;
        pending.pitch_degrees += rate.pitch * step;
        pending.yaw_held |= rate.yaw != 0.0f;
        pending.pitch_held |= rate.pitch != 0.0f;
    }
}

Turn take() {
    Turn turn = pending;
    for (const Turn &source : motion) add(turn, source);
    pending = Turn{};
    motion.fill(Turn{});
    return turn;
}

Turn peek(Source source) {
    const auto index = static_cast<std::size_t>(source);
    return index < kSources ? motion[index] : Turn{};
}

Turn take(Source source) {
    const auto index = static_cast<std::size_t>(source);
    if (index >= kSources) return Turn{};
    return std::exchange(motion[index], Turn{});
}

void discard() {
    pending = Turn{};
    motion.fill(Turn{});
}

Rate rate(Source source) {
    const auto index = static_cast<std::size_t>(source);
    return index < kSources ? rates[index] : Rate{};
}

void reset() {
    rates.fill(Rate{});
    discard();
}

} // namespace mhp3rd::camera
