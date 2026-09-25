#pragma once

#include <cstdint>

// What the player asks the camera to do, independent of the device asking and
// of how the game's camera is driven (#103).
//
// Two kinds of source feed it. A rate source (a stick, camera keys) holds a
// speed until it changes, as a fraction of Camera speed; the port turns that
// into degrees using the real time between frames, so a push means the same
// turn per second at any frame rate. A motion source (a mouse, a touch drag)
// adds degrees directly, once. Every source adds together, and whoever drives
// the game's camera takes the sum once per camera update.
//
// Signs: positive yaw turns right, positive pitch looks down, the way a
// non-inverted stick or mouse is pushed. Inversion belongs to each source.
namespace mhp3rd::camera {

enum class Source : std::uint8_t { Stick, Keys, Mouse, Touch, Count };

// Held speed from a rate source, each axis -1..1 of Camera speed.
void set_rate(Source source, float yaw, float pitch);
// Degrees from a motion source, added to what the next camera update takes.
void add_motion(Source source, float yaw_degrees, float pitch_degrees);

// Once per presented game frame: turns the held rates into degrees over the
// real time since the previous frame. `degrees_per_second` is full deflection.
void advance(float seconds, float degrees_per_second);

struct Turn {
    float yaw_degrees{};
    float pitch_degrees{};
    // Some source is asking for yaw or pitch right now, even if this update's
    // degrees happen to round to nothing.
    bool yaw_held{};
    bool pitch_held{};
};
// What has built up since the last take, which it clears.
[[nodiscard]] Turn take();
// What one motion source alone has added since the last take: peek leaves it,
// take(source) clears it, so a take() after that returns everything else.
// For a camera that has to treat the mouse apart, like a bow's aim.
[[nodiscard]] Turn peek(Source source);
[[nodiscard]] Turn take(Source source);
// Drops what has built up, for when nothing can take it: a menu, a cutscene,
// a camera mode the port does not drive. Otherwise the camera would jump by
// all of it on the next update it does drive.
void discard();
// Forgets every source's held rate as well, for tests and a fresh start.
void reset();

// The rate a source holds now, for traces.
struct Rate {
    float yaw{};
    float pitch{};
};
[[nodiscard]] Rate rate(Source source);

} // namespace mhp3rd::camera
