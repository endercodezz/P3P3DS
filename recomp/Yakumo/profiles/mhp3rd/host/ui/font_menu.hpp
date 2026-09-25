#pragma once

// The menu's Text section: the font the game's text is drawn with, its
// weight, a preview, and the list of fonts to choose from.
namespace mhp3rd::ui {

// The section's rows, for the menu's Video page.
void font_rows();

// Whether the list of fonts is open.
[[nodiscard]] bool font_list_open();

// Draws the list of fonts while it is open. `back`: the back button was
// pressed this frame, which closes the list. True while the list is open, so
// the menu keeps itself open too.
bool font_list(bool back);

} // namespace mhp3rd::ui
