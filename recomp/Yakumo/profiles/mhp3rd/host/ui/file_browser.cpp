#include "ui/file_browser.hpp"

#include "ui/layer.hpp"
#include "ui/widgets.hpp"

#include "gpu/vulkan_renderer.hpp"
#include "install/user_data.hpp"

#include "imgui.h"

#include <SDL3/SDL.h>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <mutex>
#include <system_error>

namespace mhp3rd::ui {
namespace {

namespace fs = std::filesystem;

std::string lower(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return text;
}

bool is_folder(const fs::path &path) {
    std::error_code ec;
    return fs::is_directory(path, ec);
}

// A small button in the row of places above the list.
bool chip(const char *id, const std::string &text, bool active = false) {
    const float font = Layer::get().font_size();
    const float scale = Layer::get().scale();
    const ImVec2 size{ImGui::CalcTextSize(text.c_str()).x + font * 1.2f, std::round(font * 1.7f)};
    // Chips follow each other on a line and wrap when it is full.
    if (ImGui::GetContentRegionAvail().x < size.x) ImGui::NewLine();
    const ImVec2 min = ImGui::GetCursorScreenPos();
    const bool pressed = ImGui::InvisibleButton(id, size, ImGuiButtonFlags_EnableNav);
    const bool focused = ImGui::IsItemFocused();
    const bool hovered = ImGui::IsItemHovered();
    ImDrawList *draw = ImGui::GetWindowDrawList();
    const ImVec2 max{min.x + size.x, min.y + size.y};
    const float rounding = size.y * 0.5f;
    if (active) draw->AddRectFilled(min, max, colors::kAccent, rounding);
    else if (focused) draw->AddRectFilled(min, max, colors::kRowFocus, rounding);
    else if (hovered) draw->AddRectFilled(min, max, colors::kRowHover, rounding);
    draw->AddRect(min, max, focused ? colors::kAccentBright : colors::kPanelEdge, rounding, 0,
                  std::round((focused ? 2.0f : 1.5f) * scale));
    const ImVec2 text_size = ImGui::CalcTextSize(text.c_str());
    draw->AddText({min.x + (size.x - text_size.x) * 0.5f, min.y + (size.y - text_size.y) * 0.5f},
                  active ? colors::kPanel : colors::kText, text.c_str());
    ImGui::SameLine(0.0f, std::round(8.0f * scale));
    return pressed;
}

} // namespace

std::string human_size(std::uint64_t bytes) {
    char text[32];
    if (bytes >= 1'000'000'000u) std::snprintf(text, sizeof(text), "%.1f GB", static_cast<double>(bytes) / 1e9);
    else if (bytes >= 1'000'000u) std::snprintf(text, sizeof(text), "%.0f MB", static_cast<double>(bytes) / 1e6);
    else if (bytes >= 1'000u) std::snprintf(text, sizeof(text), "%.0f KB", static_cast<double>(bytes) / 1e3);
    else std::snprintf(text, sizeof(text), "%llu bytes", static_cast<unsigned long long>(bytes));
    return text;
}

// The system file dialog's answer arrives on its own schedule, possibly on
// another thread, and possibly never (gamescope); the browser polls for it.
struct FileBrowser::SystemDialog {
    std::mutex mutex;
    std::optional<fs::path> path;
    std::string error;
};

fs::path FileBrowser::home() {
#if defined(_WIN32)
    const char *profile = std::getenv("USERPROFILE");
#else
    const char *profile = std::getenv("HOME");
#endif
    if (profile != nullptr && *profile != '\0') return install::path_from_utf8(profile);
    return fs::current_path();
}

FileBrowser::FileBrowser(const fs::path &folder) : FileBrowser(folder, Options{}) {}

FileBrowser::FileBrowser(const fs::path &folder, Options options)
    : options_(std::move(options)), dialog_(std::make_shared<SystemDialog>()) {
    find_places();
    open(!folder.empty() && is_folder(folder) ? folder : home());
}

FileBrowser::~FileBrowser() = default;

void FileBrowser::find_places() {
    places_.clear();
    const fs::path home_dir = home();
    places_.push_back({"Home", home_dir});
    for (const char *name : {"Downloads", "Desktop", "Documents"})
        if (is_folder(home_dir / name)) places_.push_back({name, home_dir / name});
    std::error_code ec;
#if defined(__APPLE__)
    // Every mounted volume; the system disk appears as a link to / and is left out.
    for (const fs::directory_entry &entry : fs::directory_iterator("/Volumes", ec)) {
        if (entry.is_symlink(ec)) continue;
        if (is_folder(entry.path()))
            places_.push_back({install::path_to_utf8(entry.path().filename()), entry.path()});
    }
#elif defined(__linux__)
    // SD cards and USB drives: /run/media/<user>/<label> on most desktops and
    // SteamOS (or /run/media/<device> on newer SteamOS), /media/<user>/<label>
    // on Debian and Ubuntu.
    const char *user = std::getenv("USER");
    for (const char *base : {"/run/media", "/media"}) {
        for (const fs::directory_entry &entry : fs::directory_iterator(base, ec)) {
            if (!is_folder(entry.path())) continue;
            if (user != nullptr && entry.path().filename() == user) {
                std::error_code inner;
                for (const fs::directory_entry &volume : fs::directory_iterator(entry.path(), inner))
                    if (is_folder(volume.path()))
                        places_.push_back({install::path_to_utf8(volume.path().filename()), volume.path()});
            } else {
                places_.push_back({install::path_to_utf8(entry.path().filename()), entry.path()});
            }
        }
    }
#elif defined(_WIN32)
    for (char letter = 'A'; letter <= 'Z'; ++letter) {
        const std::string root = std::string(1, letter) + ":\\";
        if (is_folder(root)) places_.push_back({std::string(1, letter) + ":", fs::path(root)});
    }
#endif
}

bool FileBrowser::listed(const fs::path &file) const {
    const std::string extension = lower(install::path_to_utf8(file.extension()));
    return std::find(options_.extensions.begin(), options_.extensions.end(), extension) != options_.extensions.end();
}

void FileBrowser::open(const fs::path &folder, fs::path focus) {
    std::error_code ec;
    fs::path target = fs::absolute(folder, ec).lexically_normal();
    // "C:\foo\" and "/foo/" normalise to a trailing separator; drop it so the
    // parent is the real parent.
    if (target.has_relative_path() && target.filename().empty()) target = target.parent_path();
    std::vector<Entry> entries;
    std::size_t hidden = 0;
    std::string error;
    fs::directory_iterator it(target, fs::directory_options::skip_permission_denied, ec);
    if (ec) {
        error = "This folder cannot be opened: " + ec.message() + ".";
    } else {
        // Incremented by hand: the range-for form throws on an unreadable entry.
        for (; it != fs::directory_iterator(); it.increment(ec)) {
            if (ec) break;
            const fs::directory_entry &entry = *it;
            const std::string name = install::path_to_utf8(entry.path().filename());
            if (name.empty() || name[0] == '.') continue;
            Entry item;
            item.path = entry.path();
            item.name = name;
            item.directory = is_folder(entry.path());
            item.choosable = item.directory ? options_.choose_on_open && options_.choose_on_open(entry.path())
                                            : listed(entry.path());
            if (!item.directory) {
                std::error_code size_error;
                if (!fs::is_regular_file(entry.path(), size_error)) continue;
                if (!show_all_ && !listed(entry.path())) {
                    ++hidden;
                    continue;
                }
                item.size = fs::file_size(entry.path(), size_error);
            }
            entries.push_back(std::move(item));
        }
        std::sort(entries.begin(), entries.end(), [](const Entry &a, const Entry &b) {
            if (a.directory != b.directory) return a.directory;
            return lower(a.name) < lower(b.name);
        });
    }
    folder_ = target;
    entries_ = std::move(entries);
    hidden_files_ = hidden;
    error_ = std::move(error);
    focus_.reset();
    focus_first_ = true;
    if (!focus.empty()) {
        focus_ = std::move(focus);
        focus_first_ = false;
    }
}

FileBrowser::Result FileBrowser::frame(bool back) {
    const float font = Layer::get().font_size();
    const bool typing = ImGui::GetIO().WantTextInput;
    const fs::path parent = folder_.parent_path();
    const bool has_parent = !parent.empty() && parent != folder_;

    // The system dialog's answer, when one was opened.
    {
        std::lock_guard lock(dialog_->mutex);
        if (dialog_->path) {
            chosen_ = *dialog_->path;
            dialog_->path.reset();
            return Result::Chosen;
        }
        if (!dialog_->error.empty()) {
            error_ = "The system file dialog could not be opened (" + dialog_->error + ").";
            dialog_->error.clear();
        }
    }

    // Back and Backspace go up a folder; at the top they leave the browser.
    if (back || (!typing && ImGui::IsKeyPressed(ImGuiKey_Backspace, false))) {
        if (!has_parent) return Result::Cancelled;
        open(parent, folder_);
    }
    if (ImGui::IsKeyPressed(ImGuiKey_GamepadFaceUp, false)) {
        show_all_ = !show_all_;
        open(folder_);
    }

    // Places, and the switches.
    fs::path go_to;
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + std::round(8.0f * Layer::get().scale()));
    for (std::size_t i = 0; i < places_.size(); ++i) {
        ImGui::PushID(static_cast<int>(i));
        const Place &place = places_[i];
        if (chip("##place", place.name, folder_ == place.path)) go_to = place.path;
        ImGui::PopID();
    }
    if (chip("##all", show_all_ ? "Showing all files" : "Showing " + options_.filter_name + " only")) {
        show_all_ = !show_all_;
        open(folder_);
    }
    // Under gamescope the portal dialog never shows; do not offer it there.
    // On Android it gives content:// documents, which nothing that uses the
    // browser can read by name.
#if defined(MHP3RD_ANDROID_APP)
    const bool system_dialog = false;
#else
    const bool system_dialog = std::getenv("GAMESCOPE_WAYLAND_DISPLAY") == nullptr;
#endif
    if (system_dialog && chip("##system", "System dialog\u2026")) {
        static const SDL_DialogFileFilter kFilters[] = {{"Disc images (*.iso)", "iso"}, {"All files", "*"}};
        const bool folders = !options_.choose_folder.empty();
        auto *state = new std::shared_ptr<SystemDialog>(dialog_);
        const auto callback = [](void *userdata, const char *const *files, int) {
            auto *shared = static_cast<std::shared_ptr<SystemDialog> *>(userdata);
            {
                std::lock_guard lock((*shared)->mutex);
                if (files == nullptr) (*shared)->error = SDL_GetError();
                else if (files[0] != nullptr) (*shared)->path = install::path_from_utf8(files[0]);
            }
            delete shared;
        };
        const std::string start = install::path_to_utf8(folder_);
        if (folders) SDL_ShowOpenFolderDialog(callback, state, Layer::get().renderer().window(), start.c_str(), false);
        else SDL_ShowOpenFileDialog(callback, state, Layer::get().renderer().window(), kFilters, 2, start.c_str(), false);
    }
    ImGui::NewLine();

    // Where we are.
    ImGui::Dummy({0.0f, std::round(font * 0.2f)});
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + std::round(16.0f * Layer::get().scale()));
    ImGui::PushStyleColor(ImGuiCol_Text, colors::kTextDim);
    ImGui::PushTextWrapPos(0.0f);
    ImGui::TextUnformatted(install::path_to_utf8(folder_).c_str());
    ImGui::PopTextWrapPos();
    ImGui::PopStyleColor();
    ImGui::Dummy({0.0f, std::round(font * 0.2f)});

    ImGui::BeginChild("entries", {0.0f, 0.0f}, ImGuiChildFlags_NavFlattened);
    std::optional<std::size_t> activated;
    bool up = false;
    bool choose_here = false;
    if (!options_.choose_folder.empty()) {
        if (focus_first_) {
            focus_next_row();
            focus_first_ = false;
        }
        if (list_row("##choose", options_.choose_folder, "", ListIcon::Folder, true)) choose_here = true;
    }
    if (has_parent) {
        if (focus_first_) {
            focus_next_row();
            focus_first_ = false;
        }
        if (list_row("##parent", "Parent folder", "", ListIcon::ParentFolder)) up = true;
    }
    for (std::size_t i = 0; i < entries_.size(); ++i) {
        const Entry &entry = entries_[i];
        if ((focus_ && *focus_ == entry.path) || focus_first_) {
            focus_next_row();
            focus_.reset();
            focus_first_ = false;
        }
        ImGui::PushID(static_cast<int>(i));
        const bool iso = !entry.directory && lower(install::path_to_utf8(entry.path.extension())) == ".iso";
        const ListIcon icon = entry.directory ? ListIcon::Folder : iso ? ListIcon::Disc : ListIcon::File;
        if (list_row("##entry", entry.name, entry.directory ? "" : human_size(entry.size), icon, entry.choosable)) activated = i;
        ImGui::PopID();
    }
    focus_.reset();
    focus_first_ = false;
    if (!error_.empty()) {
        ImGui::Dummy({0.0f, font * 0.3f});
        ImGui::Indent(std::round(16.0f * Layer::get().scale()));
        paragraph(error_, colors::kDanger);
        ImGui::Unindent(std::round(16.0f * Layer::get().scale()));
    } else if (entries_.empty() || (hidden_files_ > 0 && !show_all_)) {
        ImGui::Dummy({0.0f, font * 0.3f});
        ImGui::Indent(std::round(16.0f * Layer::get().scale()));
        std::string note = entries_.empty() ? options_.empty_note : "";
        if (hidden_files_ > 0 && !show_all_)
            note += (note.empty() ? "" : " ") + std::to_string(hidden_files_) + " other file" +
                    (hidden_files_ == 1 ? " is" : "s are") + " hidden; only " + options_.listed_name + " are listed.";
        paragraph(note, colors::kTextDim);
        ImGui::Unindent(std::round(16.0f * Layer::get().scale()));
    }
    touch_scroll();
    ImGui::EndChild();

    if (!go_to.empty()) open(go_to);
    if (up) open(parent, folder_);
    if (choose_here) {
        chosen_ = folder_;
        return Result::Chosen;
    }
    if (activated) {
        const Entry entry = entries_[*activated];
        if (entry.directory && entry.choosable) {
            chosen_ = entry.path;
            return Result::Chosen;
        }
        if (entry.directory) {
            open(entry.path);
        } else {
            chosen_ = entry.path;
            return Result::Chosen;
        }
    }
    return Result::Browsing;
}

} // namespace mhp3rd::ui
