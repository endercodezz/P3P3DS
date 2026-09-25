#pragma once

// The menu's Debug page: the developer tools (debug/debug_tools.hpp) for
// testing without grinding. Only in developer builds, shown with
// MHP3RD_DEBUG_MENU=1.

namespace mhp3rd::ui {

// Draws the page. `back`: the back button was pressed this frame.
void debug_page(bool back);

// Whether a screen of the page (such as the item list) is open, which Back
// closes before it closes the menu.
[[nodiscard]] bool debug_screen_open();

} // namespace mhp3rd::ui
