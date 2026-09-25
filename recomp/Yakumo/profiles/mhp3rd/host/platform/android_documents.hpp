#pragma once

// Saves through Android's document picker (#17). A folder the player picks
// is a tree of content:// documents, not a path, so the save screen works on
// local copies: an import copies the save folders it can find in the picked
// folder into a staging folder and checks them there as on any platform; an
// export or a backup is written to a staging folder and then copied into the
// picked one. Only in the Android app (MHP3RD_ANDROID_APP).

#include <filesystem>
#include <optional>
#include <string>

namespace mhp3rd::android {

struct PickedImport {
    std::filesystem::path staged;  // the local copy to look for saves in
    std::string error;             // empty: the copy worked
};
// Asks for a folder, then copies the save folders (those holding a PARAM.SFO)
// found in it into <staging>/<its name>/SAVEDATA, where find_saves() looks:
// the folder itself, its SAVEDATA and PSP folders and two levels of other
// folders, so a memory stick's root, PPSSPP's PSP folder or an export all
// work. `staging` is emptied first. Nothing when the player cancels.
[[nodiscard]] std::optional<PickedImport> pick_saves_to_import(const std::filesystem::path &staging);

struct PickedExport {
    std::string where;  // the picked folder, for the player
    std::string error;  // empty: the copy worked
};
// Asks for a folder and copies `local` into it as a folder of the same name,
// with everything in it. Nothing when the player cancels.
[[nodiscard]] std::optional<PickedExport> pick_folder_and_copy(const std::filesystem::path &local);

} // namespace mhp3rd::android
