#include "ui/save_screen.hpp"

#include "ui/file_browser.hpp"
#include "ui/layer.hpp"
#include "ui/widgets.hpp"

#include "install/user_data.hpp"
#include "settings/settings.hpp"
#include "save_data/save_transfer.hpp"
#if defined(MHP3RD_ANDROID_APP)
#include "platform/android_documents.hpp"
#endif

#include "imgui.h"

#include <SDL3/SDL.h>

#include <cctype>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

namespace mhp3rd::ui {
namespace {

namespace fs = std::filesystem;
namespace sd = mhp3rd::savedata;

enum class Stage {
    Closed,
    ChooseImport,
    Review,
    Imported,
    ChooseExport,
    Exported,
    Backup,
    ChooseBackup,
    ConfirmBackup,
    BackedUp,
};

struct Found {
    sd::SaveCheck check;
    sd::FolderSummary incoming;
    sd::FolderSummary current;
};

struct Imported {
    std::string name;
    sd::ImportResult result;
};

struct State {
    Stage stage{Stage::Closed};
    std::unique_ptr<FileBrowser> browser;
    fs::path last_folder;  // where the browser opens next time
    fs::path picked;
    std::vector<Found> found;
    std::size_t other_games{};
    std::vector<Imported> imported;
    fs::path backup_dir;
    sd::ExportResult exported;
    fs::path backup_target;         // the folder chosen for a backup
    std::vector<std::string> backup_conflicts;
    sd::BackupResult backed_up;
    bool focus{};           // focus the first row of a new stage
    bool restart{};         // the player chose to restart
    bool focus_row{};       // focus the Import row when the screen closes
};

State &state() {
    static State s;
    return s;
}

float px(float value) { return std::round(value * Layer::get().scale()); }

fs::path savedata_root() { return sd::memory_stick() / "PSP" / "SAVEDATA"; }

std::string utf8(const fs::path &path) { return install::path_to_utf8(path); }

// The default place for backups, beside the rest of Yakumo's data.
fs::path backups_directory() {
    try {
        return install::user_data_directory() / "save-backups";
    } catch (const std::exception &) {
        return sd::memory_stick().parent_path() / "save-backups";
    }
}

// Percent-encodes a path for a file:// URL.
std::string file_url(const std::string &path) {
    std::string url = "file://";
    for (const unsigned char c : path) {
        if (std::isalnum(c) != 0 || c == '/' || c == '-' || c == '_' || c == '.' || c == '~') {
            url += static_cast<char>(c);
        } else {
            char escaped[4];
            std::snprintf(escaped, sizeof(escaped), "%%%02X", c);
            url += escaped;
        }
    }
    return url;
}

// The Flatpak reads the player's folders but writes only to its own data
// directory and to Downloads, so an export or a backup elsewhere fails there.
void sandbox_note() {
    if (std::getenv("FLATPAK_ID") == nullptr) return;
    ImGui::Indent(px(16.0f));
    paragraph("The Flatpak can read your folders but write only to its own data folder and to Downloads. Pick "
              "Downloads or the backups folder; Open the backups folder shows where that is.",
              colors::kTextDim);
    ImGui::Unindent(px(16.0f));
}

std::string names_text(const std::vector<std::string> &names) {
    std::string text;
    for (std::size_t i = 0; i < names.size(); ++i)
        text += (i == 0 ? "" : i + 1 == names.size() ? " and " : ", ") + sd::save_label(names[i]);
    return text;
}

std::string describe(const sd::FolderSummary &summary) {
    if (!summary.exists) return "None";
    return "Saved " + sd::timestamp_for_display(summary.modified) + ", " + human_size(summary.bytes);
}

void go(Stage stage) {
    State &s = state();
    s.stage = stage;
    s.focus = true;
    ImGui::SetScrollY(0.0f);
}

void close() {
    State &s = state();
    s.stage = Stage::Closed;
    s.browser.reset();
    s.found.clear();
    s.focus_row = true;
}

#if defined(MHP3RD_ANDROID_APP)
void review(const fs::path &picked);

// Android: folders come from the system's document picker, as content://
// trees, so imports are checked in a local copy and exports and backups are
// made locally, then copied into the picked folder. The copies live beside
// the memory stick and are removed afterwards.
fs::path transfer_folder(const char *name) { return sd::memory_stick().parent_path() / "transfer" / name; }

void open_picker(Stage stage) {
    State &s = state();
    std::error_code ec;
    if (stage == Stage::ChooseImport) {
        const std::optional<android::PickedImport> picked = android::pick_saves_to_import(transfer_folder("import"));
        if (!picked) {
            close();
            return;
        }
        if (!picked->error.empty()) std::cout << "[saves] " << picked->error << "\n";
        review(picked->staged);
        return;
    }
    const auto now = std::chrono::system_clock::now();
    const fs::path local = transfer_folder(stage == Stage::ChooseExport ? "export" : "backup");
    fs::remove_all(local, ec);
    fs::create_directories(local, ec);
    fs::path made;
    if (stage == Stage::ChooseExport) {
        s.exported = sd::export_saves(sd::memory_stick(), local, now);
        made = s.exported.folder;
        if (!s.exported.ok) {
            go(Stage::Exported);
            return;
        }
    } else {
        s.backup_target = sd::backup_folder(local, now);
        s.backed_up = sd::back_up_saves(sd::memory_stick(), s.backup_target, false);
        made = s.backed_up.folder;
        if (!s.backed_up.ok) {
            go(Stage::BackedUp);
            return;
        }
    }
    const std::optional<android::PickedExport> copied = android::pick_folder_and_copy(made);
    fs::remove_all(local, ec);
    if (!copied) {
        if (stage == Stage::ChooseBackup) go(Stage::Backup);
        else close();
        return;
    }
    const fs::path shown = fs::path(copied->where) / made.filename();
    std::cout << "[saves] copied to " << copied->where << ": " << (copied->error.empty() ? "done" : copied->error)
              << std::endl;
    if (stage == Stage::ChooseExport) {
        s.exported.folder = shown;
        if (!copied->error.empty()) {
            s.exported.ok = false;
            s.exported.error = copied->error;
        }
        go(Stage::Exported);
    } else {
        s.backed_up.folder = shown;
        s.backup_target = shown;
        if (!copied->error.empty()) {
            s.backed_up.ok = false;
            s.backed_up.error = copied->error;
        }
        go(Stage::BackedUp);
    }
}
#endif

void open_browser(Stage stage) {
#if defined(MHP3RD_ANDROID_APP)
    open_picker(stage);
#else
    State &s = state();
    FileBrowser::Options options;
    options.extensions = {};
    options.filter_name = "folders";
    options.listed_name = "folders";
    options.empty_note = "No folders here.";
    if (stage == Stage::ChooseImport) {
        options.choose_folder = "Import from this folder";
        // A save folder is chosen as soon as it is opened.
        options.choose_on_open = [](const fs::path &folder) {
            std::error_code ec;
            return fs::is_regular_file(folder / "PARAM.SFO", ec);
        };
    } else if (stage == Stage::ChooseExport) {
        options.choose_folder = "Export to this folder";
    } else {
        options.choose_folder = "Back up to this folder";
    }
    s.browser = std::make_unique<FileBrowser>(s.last_folder.empty() ? FileBrowser::home() : s.last_folder,
                                              std::move(options));
    go(stage);
#endif
}

void focus_first() {
    State &s = state();
    if (s.focus) {
        focus_next_row();
        s.focus = false;
    }
}

void review(const fs::path &picked) {
    State &s = state();
    s.picked = picked;
    s.found.clear();
    s.other_games = 0;
    const auto key = sd::game_key();
    for (sd::SaveCheck &check : sd::find_saves(picked, key)) {
        // Other games' saves in a SAVEDATA folder are only counted; one picked
        // on its own is shown, so the player sees why it is refused.
        std::error_code ec;
        if (check.other_game && !fs::equivalent(check.folder, picked, ec)) {
            ++s.other_games;
            continue;
        }
        Found found;
        found.incoming = sd::summarize_folder(check.folder);
        if (check.ok()) found.current = sd::summarize_folder(savedata_root() / check.name);
        found.check = std::move(check);
        s.found.push_back(std::move(found));
    }
    std::cout << "[saves] " << s.found.size() << " save folder(s) found in " << utf8(picked) << "\n";
    for (const Found &f : s.found)
        std::cout << "[saves]   " << f.check.name << ": " << (f.check.ok() ? "ok" : f.check.problem) << "\n";
    go(Stage::Review);
}

void run_import() {
    State &s = state();
    const auto now = std::chrono::system_clock::now();
    s.backup_dir = sd::backup_directory(savedata_root(), now);
    s.imported.clear();
    for (const Found &found : s.found) {
        if (!found.check.ok()) continue;
        Imported item{found.check.name, sd::import_save(found.check, sd::memory_stick(), s.backup_dir)};
        std::cout << "[saves] import " << item.name << " from " << utf8(found.check.folder) << ": "
                  << (item.result.ok ? "done" : item.result.error)
                  << (item.result.backup.empty() ? "" : "; the replaced save is in " + utf8(item.result.backup))
                  << std::endl;
        s.imported.push_back(std::move(item));
    }
    go(Stage::Imported);
}

void finish_backup(bool replace) {
    State &s = state();
    s.backed_up = sd::back_up_saves(sd::memory_stick(), s.backup_target, replace);
    std::cout << "[saves] back up to " << utf8(s.backup_target) << ": "
              << (s.backed_up.ok ? "done" : s.backed_up.error) << std::endl;
    go(Stage::BackedUp);
}

// A backup into `target`: a new folder named by the time, or, without the
// timestamp, the save folders straight into `target` after asking before
// replacing an earlier backup there.
void start_backup(const fs::path &target) {
    State &s = state();
    const bool timestamp = settings::current().backup_timestamp;
    s.backup_target = sd::backup_folder(target, timestamp ? std::optional(std::chrono::system_clock::now())
                                                          : std::nullopt);
    s.backup_conflicts = timestamp ? std::vector<std::string>{} : sd::backup_conflicts(sd::memory_stick(), target);
    if (!s.backup_conflicts.empty()) {
        go(Stage::ConfirmBackup);
        return;
    }
    finish_backup(false);
}

bool browse(bool back) {
    State &s = state();
    ImGui::Indent(px(16.0f));
    paragraph(s.stage == Stage::ChooseImport ? "Import: open a save folder (ULJM05800, ULJM05800QST), or choose a "
                                               "folder that holds them, such as a memory stick's PSP/SAVEDATA."
              : s.stage == Stage::ChooseExport ? "Export: choose the folder to copy your saves to."
                                               : "Back up: choose the folder the backup goes to.",
              colors::kTextDim);
    ImGui::Unindent(px(16.0f));
    const FileBrowser::Result result = s.browser->frame(back);
    if (result == FileBrowser::Result::Browsing) return true;
    s.last_folder = s.browser->folder();
    const fs::path chosen = s.browser->chosen();
    const Stage stage = s.stage;
    s.browser.reset();
    if (result == FileBrowser::Result::Cancelled) {
        // Back from the backup's folder browser returns to the backup's choices.
        if (stage == Stage::ChooseBackup) {
            go(Stage::Backup);
            return true;
        }
        close();
        return false;
    }
    if (stage == Stage::ChooseImport) {
        review(chosen);
    } else if (stage == Stage::ChooseBackup) {
        start_backup(chosen);
    } else {
        s.exported = sd::export_saves(sd::memory_stick(), chosen, std::chrono::system_clock::now());
        std::cout << "[saves] export to " << utf8(chosen) << ": "
                  << (s.exported.ok ? "done, " + utf8(s.exported.folder) : s.exported.error) << std::endl;
        go(Stage::Exported);
    }
    return true;
}

void review_screen(bool back) {
    State &s = state();
    if (back) {
        open_browser(Stage::ChooseImport);
        return;
    }
    std::size_t importable = 0, replacing = 0;
    for (const Found &f : s.found) {
        if (!f.check.ok()) continue;
        ++importable;
        if (f.current.exists) ++replacing;
    }
    section("Import saves");
    ImGui::Indent(px(16.0f));
#if defined(MHP3RD_ANDROID_APP)
    // The picked folder, not the local copy it was checked in.
    paragraph("From " + utf8(s.picked.filename()), colors::kTextDim);
#else
    paragraph("From " + utf8(s.picked), colors::kTextDim);
#endif
    if (s.found.empty())
        paragraph("No saves of Monster Hunter Portable 3rd were found in this folder. Choose a save folder such as "
                  "ULJM05800, or the PSP/SAVEDATA folder that holds it.",
                  colors::kDanger);
    if (s.other_games > 0)
        paragraph(std::to_string(s.other_games) + (s.other_games == 1 ? " save belongs" : " saves belong") +
                      " to other games and " + (s.other_games == 1 ? "is" : "are") + " left out.",
                  colors::kTextDim);
    ImGui::Unindent(px(16.0f));

    for (std::size_t i = 0; i < s.found.size(); ++i) {
        const Found &f = s.found[i];
        ImGui::PushID(static_cast<int>(i));
        section((sd::save_label(f.check.name) + " (" + f.check.name + ")").c_str());
        if (i == 0) focus_first();
        info_row("Save to import", describe(f.incoming));
        if (f.check.ok()) {
            info_row("Current save", describe(f.current));
        } else {
            ImGui::Indent(px(16.0f));
            paragraph("Cannot be imported: " + f.check.problem, colors::kDanger);
            ImGui::Unindent(px(16.0f));
        }
        ImGui::PopID();
    }

    ImGui::Dummy({0.0f, px(12.0f)});
    if (s.found.empty()) focus_first();
    if (importable > 0) {
        const std::string label =
            replacing > 0 ? "Replace and import" : importable == 1 ? "Import this save" : "Import these saves";
        std::string description = "Copies the save" + std::string(importable == 1 ? "" : "s") + " into Yakumo.";
        if (replacing > 0)
            description += " The save" + std::string(replacing == 1 ? " it replaces is" : "s they replace are") +
                           " not deleted: " + (replacing == 1 ? "it moves" : "they move") +
                           " to the saves folder's .backup folder.";
        if (button_row(label.c_str(), {false, {}, description}, replacing > 0 ? colors::kAccentBright : colors::kText))
            run_import();
    }
    if (button_row("Choose another folder", {false, {}, "Back to the folders."})) open_browser(Stage::ChooseImport);
    if (button_row("Cancel", {false, {}, "Import nothing."})) close();
}

void imported_screen(bool back) {
    State &s = state();
    if (back) {
        close();
        return;
    }
    bool any = false, backups = false;
    section("Import saves");
    for (std::size_t i = 0; i < s.imported.size(); ++i) {
        const Imported &item = s.imported[i];
        any = any || item.result.ok;
        backups = backups || !item.result.backup.empty();
        ImGui::PushID(static_cast<int>(i));
        if (i == 0) focus_first();
        info_row(sd::save_label(item.name).c_str(),
                 item.result.ok ? (item.result.backup.empty() ? "Imported" : "Imported; the old save was kept")
                                : "Not imported: " + item.result.error);
        ImGui::PopID();
    }
    ImGui::Indent(px(16.0f));
    if (backups) paragraph("The replaced saves are in " + utf8(s.backup_dir) + ".", colors::kTextDim);
    if (any)
        paragraph("The game reads its saves at the title screen. Restart now to load the imported save; progress "
                  "since your last save is lost. If you keep playing instead, do not save before you restart: "
                  "saving would replace the imported save with the game you are playing.");
    ImGui::Unindent(px(16.0f));
    ImGui::Dummy({0.0f, px(12.0f)});
    if (any && button_row("Restart now", {false, {}, "Closes the game and starts it again at the title screen."},
                          colors::kAccentBright)) {
        s.restart = true;
        close();
    }
    if (button_row(any ? "Later" : "Done", {false, {}, "Back to the menu."})) close();
}

void exported_screen(bool back) {
    State &s = state();
    if (back) {
        close();
        return;
    }
    section("Export saves");
    focus_first();
    if (s.exported.ok) {
        std::string names;
        for (const std::string &name : s.exported.exported)
            names += (names.empty() ? "" : ", ") + sd::save_label(name);
        info_row("Exported", names);
        info_row("To", utf8(s.exported.folder));
        ImGui::Indent(px(16.0f));
        paragraph("The folder is laid out like a memory stick: copy its PSP folder to the root of a PSP's memory "
                  "stick, or import it from this menu on another machine.",
                  colors::kTextDim);
        ImGui::Unindent(px(16.0f));
    } else {
        info_row("Not exported", s.exported.error);
        sandbox_note();
    }
    ImGui::Dummy({0.0f, px(12.0f)});
    if (button_row("Done", {false, {}, "Back to the menu."})) close();
}

void backup_screen(bool back) {
    State &s = state();
    if (back) {
        close();
        return;
    }
    settings::Settings &settings = settings::current();
    const std::vector<std::string> names = sd::saves_to_back_up(sd::memory_stick());
    section("Back up saves");
    ImGui::Indent(px(16.0f));
    paragraph(names.empty() ? "There is no save to back up yet."
                            : "Copies " + names_text(names) + " to a folder. To restore a backup, import it.",
              names.empty() ? colors::kDanger : colors::kTextDim);
    ImGui::Unindent(px(16.0f));
    focus_first();
    if (toggle_row("Add a timestamp to the backup name", settings.backup_timestamp,
                   {false, {},
                    "On: every backup is a new folder named by its date and time, such as 2026-09-19_19-05-12, "
                    "holding the save folders. Off: the save folders go straight into the folder you choose, and "
                    "an earlier backup there is replaced after asking."})) {
        settings.backup_timestamp = !settings.backup_timestamp;
        settings::save();
    }
#if !defined(MHP3RD_ANDROID_APP)
    // An Android app's own folders are out of the player's reach: there a
    // backup always goes to a folder picked in the system's picker.
    if (button_row("Back up to the backups folder",
                   {names.empty(), {}, "Into " + utf8(backups_directory()) + "."})) {
        std::error_code ec;
        fs::create_directories(backups_directory(), ec);
        start_backup(backups_directory());
    }
#endif
    if (button_row("Back up to another folder…", {names.empty(), {}, "Choose where the backup goes."}))
        open_browser(Stage::ChooseBackup);
#if !defined(MHP3RD_ANDROID_APP)
    if (button_row("Open the backups folder", {false, {}, "Show " + utf8(backups_directory()) + "."}))
        open_folder(backups_directory());
#endif
    if (button_row("Cancel", {false, {}, "Back to the menu."})) close();
}

void confirm_backup_screen(bool back) {
    State &s = state();
    if (back) {
        go(Stage::Backup);
        return;
    }
    section("Replace the earlier backup?");
    ImGui::Indent(px(16.0f));
    paragraph(utf8(s.backup_target) + " already holds a backup of " + names_text(s.backup_conflicts) +
              ". Backing up again replaces it with your saves as they are now.");
    ImGui::Unindent(px(16.0f));
    focus_first();
    if (button_row("Cancel", {false, {}, "Keep the earlier backup."})) go(Stage::Backup);
    if (button_row("Replace the backup", {false, {}, "The earlier backup there is replaced."}, colors::kDanger))
        finish_backup(true);
}

void backed_up_screen(bool back) {
    State &s = state();
    if (back) {
        close();
        return;
    }
    section("Back up saves");
    focus_first();
    if (s.backed_up.ok) {
        info_row("Backed up", names_text(s.backed_up.saved));
        info_row("To", utf8(s.backed_up.folder));
    } else {
        info_row("Not backed up", s.backed_up.error);
        sandbox_note();
    }
    ImGui::Dummy({0.0f, px(12.0f)});
#if !defined(MHP3RD_ANDROID_APP)
    if (button_row("Open the folder", {false, {}, "Show the backup in the file manager."}))
        open_folder(s.backed_up.ok ? s.backed_up.folder : s.backup_target);
#endif
    if (button_row("Done", {false, {}, "Back to the menu."})) close();
}

} // namespace

bool open_folder(const fs::path &folder) {
    std::error_code ec;
    fs::create_directories(folder, ec);
    const std::string path = utf8(folder);
    if (SDL_OpenURL(file_url(path).c_str())) return true;
    std::cout << "[menu] cannot open " << path << ": " << SDL_GetError() << "\n";
    return false;
}

void save_rows() {
    State &s = state();
    const bool available = !sd::memory_stick().empty();
    if (s.focus_row) {
        focus_next_row();
        s.focus_row = false;
    }
#if defined(MHP3RD_ANDROID_APP)
    // Android's picker offers no storage's root and no Download folder itself.
    if (button_row("Import save…", {!available, {},
                                     "Copy a save from PPSSPP, a memory stick or an export: pick its PSP folder, or "
                                     "a folder holding it or the save folders. A save it replaces is kept."}))
        open_browser(Stage::ChooseImport);
    if (button_row("Export save…", {!available, {},
                                     "Copy your game data and downloaded quests to a folder you pick (Android does "
                                     "not offer Download itself: make or pick a folder in it)."}))
        open_browser(Stage::ChooseExport);
#else
    if (button_row("Import save…", {!available, {},
                                     "Copy a save from a PSP memory stick, PPSSPP or another installation: choose "
                                     "its folder (ULJM05800, ULJM05800QST) or the PSP/SAVEDATA folder that holds "
                                     "it. A save it replaces is kept, not deleted."}))
        open_browser(Stage::ChooseImport);
    if (button_row("Export save…", {!available, {},
                                     "Copy your game data and downloaded quests to a folder you choose, to take them "
                                     "to a PSP or another machine."}))
        open_browser(Stage::ChooseExport);
#endif
    if (button_row("Back up saves…", {!available, {},
                                      "Copy all of this game's saves, the install data included, to the backups "
                                      "folder or a folder you choose."}))
        go(Stage::Backup);
#if !defined(MHP3RD_ANDROID_APP)
    if (button_row("Open the saves folder",
                   {!available, {}, "Show the folder the game saves to (PSP/SAVEDATA) in the file manager."}))
        open_folder(sd::memory_stick() / "PSP" / "SAVEDATA");
    if (button_row("Open the backups folder", {false, {}, "Show " + utf8(backups_directory()) + "."}))
        open_folder(backups_directory());
#endif
}

bool save_screen_open() { return state().stage != Stage::Closed; }

bool save_screen(bool back) {
    State &s = state();
    switch (s.stage) {
    case Stage::Closed: return false;
    case Stage::ChooseImport:
    case Stage::ChooseExport:
    case Stage::ChooseBackup: return browse(back);
    case Stage::Backup: backup_screen(back); break;
    case Stage::ConfirmBackup: confirm_backup_screen(back); break;
    case Stage::BackedUp: backed_up_screen(back); break;
    case Stage::Review: review_screen(back); break;
    case Stage::Imported: imported_screen(back); break;
    case Stage::Exported: exported_screen(back); break;
    }
    return true;
}

bool take_restart_request() { return std::exchange(state().restart, false); }

} // namespace mhp3rd::ui
