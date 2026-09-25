#pragma once

#include "input/touch_controls.hpp"

namespace mhp3rd::ui {

// Draws the on-screen touch controls over the game in the current ImGui frame,
// behind any window: outlines at `opacity`, filled while held.
void draw_touch_controls(const input::touch::Controls &controls, float opacity);

} // namespace mhp3rd::ui
