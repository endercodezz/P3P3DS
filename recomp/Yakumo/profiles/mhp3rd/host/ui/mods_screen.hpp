#pragma once

// The menu's Mods page: the mods in the mods folder with what each one is
// and changes, turning them on and off, their order where two change the same
// file, the conflicts, when a change applies, and importing a downloaded mod
// with the file browser (or by dropping its folder on the window). Standard
// rows only, so a gamepad, a keyboard, a mouse and a touch screen all work.
namespace mhp3rd::ui {

// Draws the page, or the screen it opened (a mod's details, the import).
// `back`: the back button was pressed this frame.
void mods_page(bool back);

// Whether a screen of the page is open, so back closes it before the menu.
[[nodiscard]] bool mods_screen_open();

// Once, after the player chose "Restart now" to apply a change.
[[nodiscard]] bool take_mods_restart_request();

} // namespace mhp3rd::ui
