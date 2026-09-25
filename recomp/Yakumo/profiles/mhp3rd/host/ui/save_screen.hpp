#pragma once

// The System page's save import and export: a folder chosen with the file
// browser, the saves found in it checked and shown beside the ones they would
// replace, and the import itself (see save_data/save_transfer.hpp).
#include <filesystem>

namespace mhp3rd::ui {

// The page's rows: Import save…, Export save…, Back up saves…, and the
// buttons that open the saves and backups folders.
void save_rows();

// Shows a folder in the file manager, creating it first. False when it could
// not be opened.
bool open_folder(const std::filesystem::path &folder);

// Whether the import or export screen is open.
[[nodiscard]] bool save_screen_open();

// Draws the open screen in place of the page. `back`: the back button was
// pressed this frame. True while the screen is open.
bool save_screen(bool back);

// Once, after the player chose to restart the game to load an imported save.
[[nodiscard]] bool take_restart_request();

} // namespace mhp3rd::ui
