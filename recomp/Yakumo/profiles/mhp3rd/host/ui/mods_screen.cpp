#include "ui/mods_screen.hpp"

#include "ui/file_browser.hpp"
#include "ui/layer.hpp"
#include "ui/save_screen.hpp"
#include "ui/text_input.hpp"
#include "ui/widgets.hpp"

#include "install/user_data.hpp"
#include "mods/mhp3rd_mods.hpp"
#include "mods/mod_import.hpp"

#include "imgui.h"
#include "imgui_internal.h"

#define STBI_ONLY_PNG
#define STBI_NO_STDIO
#define STBI_NO_LINEAR
#define STBI_NO_HDR
#define STB_IMAGE_STATIC
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstring>
#include <fstream>
#include <iostream>
#include <iterator>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

namespace mhp3rd::ui {
namespace {

namespace fs = std::filesystem;
using mods::Mod;
using mods::ModSession;
using mods::Resolution;

enum class Stage { List, Details, Choose, Review, Result };

// A mod's preview image as an ImGui texture, loaded once and kept.
struct Preview {
    std::unique_ptr<ImTextureData> texture;
    bool failed{};
};

struct State {
    Stage stage{Stage::List};
    std::string mod;  // the mod the details show
    std::unique_ptr<FileBrowser> browser;
    fs::path last_folder;
    mods::ImportCheck check;
    mods::ImportResult result;
    bool focus{};
    bool focus_row{};
    std::string return_to;  // the mod whose row gets the focus back
    bool restart{};
    std::map<fs::path, Preview> previews;
    // Slot targets typed on the keyboard, applied on the next frame.
    std::optional<std::pair<std::size_t, std::string>> typed_slot;
};

State &state() {
    static State s;
    return s;
}

float px(float value) { return std::round(value * Layer::get().scale()); }

std::string utf8(const fs::path &path) { return install::path_to_utf8(path); }

void go(Stage stage) {
    State &s = state();
    s.stage = stage;
    s.focus = true;
    ImGui::SetScrollY(0.0f);
}

void back_to_list() {
    State &s = state();
    s.return_to = s.stage == Stage::Details ? s.mod : std::string();
    s.stage = Stage::List;
    s.browser.reset();
    s.focus_row = s.return_to.empty();
}

void focus_first() {
    State &s = state();
    if (s.focus) {
        focus_next_row();
        s.focus = false;
    }
}

void indented(const std::string &text, ImU32 color = colors::kTextDim) {
    ImGui::Indent(px(16.0f));
    paragraph(text, color);
    ImGui::Unindent(px(16.0f));
}

std::string count(std::size_t n, const char *one, const char *many) {
    return std::to_string(n) + " " + (n == 1u ? one : many);
}

std::string file_name(ModSession &session, mods::FileId file) { return session.library().format().file_name(file); }

std::string mod_name(ModSession &session, const std::string &id) {
    const Mod *mod = session.library().find(id);
    return mod != nullptr ? mod->name : id;
}

const Preview &preview(const fs::path &path) {
    State &s = state();
    Preview &p = s.previews[path];
    if (p.texture || p.failed) return p;
    std::ifstream in(path, std::ios::binary);
    const std::vector<unsigned char> bytes{std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
    int width = 0;
    int height = 0;
    int channels = 0;
    unsigned char *pixels = bytes.empty() ? nullptr
                                          : stbi_load_from_memory(bytes.data(), static_cast<int>(bytes.size()), &width,
                                                                  &height, &channels, 4);
    // The mod manager's previews are 166 pixels square; anything very large
    // is refused rather than uploaded.
    if (pixels == nullptr || width <= 0 || height <= 0 || width > 1024 || height > 1024) {
        if (pixels != nullptr) stbi_image_free(pixels);
        p.failed = true;
        return p;
    }
    p.texture = std::make_unique<ImTextureData>();
    p.texture->Create(ImTextureFormat_RGBA32, width, height);
    std::memcpy(p.texture->GetPixels(), pixels, static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4u);
    stbi_image_free(pixels);
    ImGui::RegisterUserTexture(p.texture.get());
    return p;
}

void draw_preview(const fs::path &path, float size) {
    if (path.empty()) return;
    const Preview &p = preview(path);
    if (!p.texture) return;
    const float scale = size / static_cast<float>(std::max(p.texture->Width, p.texture->Height));
    ImGui::Indent(px(16.0f));
    ImGui::Image(p.texture->GetTexRef(), {static_cast<float>(p.texture->Width) * scale,
                                          static_cast<float>(p.texture->Height) * scale});
    ImGui::Unindent(px(16.0f));
}

// What a mod does, in a line.
std::string changes_text(ModSession &session, const Mod &mod) {
    std::string text;
    std::size_t replaced = 0u;
    std::size_t patched = 0u;
    for (const mods::FileChange &change : mod.changes)
        (change.kind == mods::FileChange::Kind::Replace ? replaced : patched) += 1u;
    if (replaced > 0u) text = "Replaces " + count(replaced, "file", "files");
    if (patched > 0u) text += (text.empty() ? "Patches " : ", patches ") + count(patched, "file", "files");
    if (!mod.slots.empty()) text += text.empty() ? "Replaces equipment you choose" : ", replaces equipment you choose";
    if (!mod.members.empty()) text += (text.empty() ? "Turns on " : ", turns on ") + count(mod.members.size(), "mod", "mods");
    std::string ids;
    std::size_t shown = 0u;
    for (const mods::FileChange &change : mod.changes) {
        if (shown++ == 6u) {
            ids += ", …";
            break;
        }
        ids += (ids.empty() ? "" : ", ") + file_name(session, change.file);
    }
    if (!ids.empty()) text += ": " + ids;
    return text;
}

// The conflicts that name this mod.
std::vector<const Resolution::Conflict *> conflicts_of(const Resolution &resolution, const std::string &id) {
    std::vector<const Resolution::Conflict *> found;
    for (const Resolution::Conflict &c : resolution.conflicts) {
        const bool named = c.winner == id || std::find(c.overridden.begin(), c.overridden.end(), id) != c.overridden.end() ||
                           std::find(c.patched_by.begin(), c.patched_by.end(), id) != c.patched_by.end();
        if (named) found.push_back(&c);
    }
    return found;
}

std::string conflict_text(ModSession &session, const Resolution::Conflict &c) {
    std::string text;
    if (!c.winner.empty()) text = mod_name(session, c.winner) + " wins";
    for (const std::string &loser : c.overridden) text += ", over " + mod_name(session, loser);
    if (!c.patched_by.empty()) {
        text += (text.empty() ? "Patched by " : "; patched by ");
        for (std::size_t i = 0; i < c.patched_by.size(); ++i)
            text += (i == 0 ? "" : ", ") + mod_name(session, c.patched_by[i]);
        if (!c.winner.empty()) text += " on top of the replacement";
    }
    return text;
}

void status_rows(ModSession &session) {
    const std::string note = mods::pending_note();
    if (!note.empty()) indented(note, session.restart_pending() ? colors::kAccentBright : colors::kTextDim);
    if (session.paths().disabled_by != nullptr)
        indented(std::string(session.paths().disabled_by) + " is set: no mod applies this run.", colors::kDanger);
    if (!session.error().empty()) indented(session.error(), colors::kDanger);
    if (session.restart_pending() &&
        button_row("Restart now", {false, {}, "Close the game and start it again with the mods as chosen. Progress "
                                              "since your last save is lost."},
                   colors::kAccentBright))
        state().restart = true;
}

void open_browser() {
    State &s = state();
    FileBrowser::Options options;
    options.extensions = {};
    options.filter_name = "folders";
    options.listed_name = "folders";
    options.empty_note = "No folders here.";
    options.choose_folder = "Import from this folder";
    // A mod folder is chosen as soon as it is opened.
    options.choose_on_open = [](const fs::path &folder) {
        ModSession *session = mods::session();
        return session != nullptr && session->library().format().read(folder).has_value();
    };
    fs::path start = s.last_folder;
    if (start.empty()) {
        start = FileBrowser::home() / "Downloads";
        std::error_code ec;
        if (!fs::is_directory(start, ec)) start = FileBrowser::home();
    }
    s.browser = std::make_unique<FileBrowser>(start, std::move(options));
    go(Stage::Choose);
}

void check_folder(const fs::path &chosen) {
    State &s = state();
    ModSession *session = mods::session();
    if (session == nullptr) return;
    s.check = mods::check_import(chosen, session->library().format(), session->paths().folder);
    std::cout << "[mods] import from " << utf8(chosen) << ": "
              << (s.check.mods.empty() ? s.check.problem : count(s.check.mods.size(), "mod", "mods") + " found")
              << std::endl;
    go(Stage::Review);
}

void browse(bool back) {
    State &s = state();
#if defined(MHP3RD_ANDROID_APP)
    indented("Open the folder of a mod you downloaded and unpacked (the one with its mod.ini), or a folder that "
             "holds several.");
    indented("On Android this lists only folders Yakumo can read by itself, which leaves out Downloads and SD "
             "cards. Importing through Android's file picker is not supported yet.",
             colors::kTextDim);
#else
    indented("Open the folder of a mod you downloaded and unpacked (the one with its mod.ini), or a folder that "
             "holds several. You can also drop the folder on the window.");
#endif
    if (auto dropped = Layer::get().take_dropped_file()) {
        s.last_folder = s.browser->folder();
        s.browser.reset();
        std::error_code ec;
        check_folder(fs::is_directory(*dropped, ec) ? *dropped : dropped->parent_path());
        return;
    }
    const FileBrowser::Result result = s.browser->frame(back);
    if (result == FileBrowser::Result::Browsing) return;
    s.last_folder = s.browser->folder();
    const fs::path chosen = s.browser->chosen();
    s.browser.reset();
    if (result == FileBrowser::Result::Cancelled) {
        back_to_list();
        return;
    }
    check_folder(chosen);
}

void review(ModSession &session, bool back) {
    State &s = state();
    if (back) {
        open_browser();
        return;
    }
    section("Import mods");
    indented("From " + utf8(s.check.chosen));
    if (s.check.mods.empty()) indented(s.check.problem, colors::kDanger);
    for (const mods::ImportCandidate &c : s.check.mods) {
        std::string detail = c.mod.type;
        if (!c.mod.unusable.empty()) detail += ", cannot be used here";
        else if (c.replaces) detail += ", replaces the installed one";
        info_row(c.mod.name.c_str(), detail);
        if (!c.mod.unusable.empty()) indented(c.mod.unusable, colors::kDanger);
    }
    ImGui::Dummy({0.0f, px(12.0f)});
    if (!s.check.mods.empty()) {
        const bool replaces = std::any_of(s.check.mods.begin(), s.check.mods.end(),
                                          [](const mods::ImportCandidate &c) { return c.replaces; });
        focus_first();
        if (button_row(s.check.mods.size() == 1u ? "Import this mod" : "Import these mods",
                       {false, {},
                        "Copies them into the mods folder, turned off; turn them on in the list. " +
                            std::string(replaces ? "A mod installed under the same folder name moves to "
                                                   "mods/.backup; nothing is deleted."
                                                 : "")})) {
            s.result = mods::import_mods(s.check, session.paths().folder);
            std::cout << "[mods] imported " << s.result.imported.size() << " mod(s)"
                      << (s.result.error.empty() ? "" : "; " + s.result.error) << std::endl;
            session.rescan();
            go(Stage::Result);
            return;
        }
    }
    focus_first();
    if (button_row("Choose another folder", {false, {}, "Back to the folders."})) open_browser();
    if (button_row("Cancel", {false, {}, "Import nothing."})) back_to_list();
}

void result_screen(bool back) {
    State &s = state();
    if (back) {
        back_to_list();
        return;
    }
    section("Import mods");
    if (!s.result.imported.empty()) info_row("Imported", count(s.result.imported.size(), "mod", "mods") + ", off for now");
    for (const fs::path &backup : s.result.backups) info_row("Kept", utf8(backup));
    if (!s.result.error.empty()) info_row("Problem", s.result.error);
    indented("Turn a mod on in the list. File mods apply the next time the game loads the file, or at the next start "
             "when a file grows.");
    ImGui::Dummy({0.0f, px(12.0f)});
    focus_first();
    if (button_row("Done", {false, {}, "Back to the mods."})) back_to_list();
}

void details(ModSession &session, bool back) {
    State &s = state();
    mods::ModLibrary &library = session.library();
    const Mod *found = library.find(s.mod);
    if (back || found == nullptr) {
        back_to_list();
        return;
    }
    const Mod mod = *found;  // the list may be re-sorted below
    const mods::ModChoice choice = library.choice(mod.id);
    if (s.typed_slot) {
        const auto [slot, text] = *s.typed_slot;
        s.typed_slot.reset();
        library.set_slot(mod.id, slot, text.empty() ? std::nullopt : library.format().parse_file(text));
        session.commit();
    }
    section(mod.name.c_str());
    status_rows(session);
    draw_preview(mod.preview, px(166.0f));
    focus_first();
    if (toggle_row("On", choice.enabled,
                   {!mod.unusable.empty(), mod.unusable.empty() ? std::string() : "Cannot be used",
                    mod.unusable.empty() ? "Use this mod. A pack turns its mods on and off with it; turning a mod on "
                                           "also turns on the mods it needs."
                                         : mod.unusable})) {
        library.set_enabled(mod.id, !choice.enabled);
        session.commit();
    }
    const auto &order = library.mods();
    const auto place = std::find_if(order.begin(), order.end(), [&mod](const Mod &m) { return m.id == mod.id; });
    const auto position = static_cast<std::size_t>(place - order.begin());
    const std::string priority = std::to_string(position + 1u) + " of " + std::to_string(order.size());
    const int delta = choice_row("Priority", priority,
                                 {false, {},
                                  "Where two mods replace the same file, the one higher in the list wins. Left or "
                                  "right moves this mod up or down."});
    if (delta != 0) {
        library.move(mod.id, -delta);
        session.commit();
    }
    for (std::size_t slot = 0; slot < mod.slots.size(); ++slot) {
        const std::optional<mods::FileId> target = slot < choice.slots.size() ? choice.slots[slot] : std::nullopt;
        const std::string label = "Replaces (" + mod.slots[slot].label + ")";
        if (value_row(label.c_str(), target ? file_name(session, *target) : "Choose…",
                      {false, {},
                       "The file id of the piece this model takes the place of, in four hex digits (0601). "
                       "Community file lists give the ids of each weapon and armour piece. Empty: none."})) {
            TextInputRequest request;
            request.title = label;
            request.prompt = "File id, four hex digits";
            request.initial = target ? file_name(session, *target) : std::string();
            request.max_length = 4u;
            request.allowed = [](char32_t c) { return c < 0x80u && std::isxdigit(static_cast<int>(c)) != 0; };
            open_text_input(std::move(request), [slot](std::optional<std::string> text) {
                if (text) state().typed_slot = std::make_pair(slot, *text);
            });
        }
    }

    section("About");
    if (!mod.author.empty()) info_row("Author", mod.author);
    info_row("Type", mod.type);
    if (!mod.version.empty()) info_row("Made for", mod.version);
    const std::string changes = changes_text(session, mod);
    if (!changes.empty()) info_row("Changes", changes);
    if (!mod.members.empty()) {
        std::string members;
        for (const std::string &member : mod.members) members += (members.empty() ? "" : ", ") + mod_name(session, member);
        info_row("Mods in the pack", members);
    }
    if (!mod.depends.empty()) {
        std::string needs;
        for (const std::string &dependency : mod.depends)
            needs += (needs.empty() ? "" : ", ") + mod_name(session, dependency) +
                     (library.find(dependency) == nullptr ? " (not installed)" : "");
        info_row("Needs", needs);
    }
    info_row("Folder", utf8(mod.folder));
    if (!mod.description.empty()) indented(mod.description, colors::kText);
    if (!mod.unusable.empty()) indented(mod.unusable, colors::kDanger);
    for (const std::string &note : mod.notes) indented(note, colors::kDanger);
    const auto conflicts = conflicts_of(session.wanted(), mod.id);
    if (!conflicts.empty()) {
        section("Conflicts");
        for (const Resolution::Conflict *c : conflicts)
            info_row(file_name(session, c->file).c_str(), conflict_text(session, *c));
    }
    ImGui::Dummy({0.0f, px(12.0f)});
    if (button_row("Open its folder", {false, {}, "Show the mod's folder in the file manager."})) open_folder(mod.folder);
    if (button_row("Back", {false, {}, "Back to the mods."})) back_to_list();
}

void list(ModSession &session) {
    State &s = state();
    if (s.focus_row) {
        focus_next_row();
        s.focus_row = false;
    }
    mods::ModLibrary &library = session.library();
    section("Mods (experimental)");
    indented("Mod support is new and has been tried with few mods. Back up your saves first: a mod that changes "
             "the game's data can end up in them.");
    status_rows(session);
    if (toggle_row("Use mods", library.master(),
                   {session.paths().disabled_by != nullptr,
                    session.paths().disabled_by != nullptr ? std::string("Set by ") + session.paths().disabled_by : "",
                    "Off: the game reads only its own files, whatever is turned on below, for a clean comparison."})) {
        library.set_master(!library.master());
        session.commit();
    }
    if (button_row("Import mod…",
                   {false, {},
                    "Copy a mod you downloaded into the mods folder: choose its unpacked folder, the one with its "
                    "mod.ini. Yakumo does not download mods; unpack .zip, .rar or .7z archives first."}))
        open_browser();
    if (button_row("Open the mods folder",
                   {false, {}, "Show the mods folder in the file manager: one folder per mod, as the mhp3reload mod "
                               "manager has them."}))
        open_folder(session.paths().folder);
    if (button_row("Read the folder again", {false, {}, "Pick up mods added to or removed from the folder."}))
        session.rescan();

    const auto &all = library.mods();
    section(("Installed (" + std::to_string(all.size()) + ")").c_str());
    if (all.empty()) indented("No mods yet. Import one, or copy mod folders into " + utf8(session.paths().folder) + ".");
    const Resolution &wanted = session.wanted();
    for (std::size_t i = 0; i < all.size(); ++i) {
        const Mod &mod = all[i];
        const bool on = library.enabled(mod.id);
        std::string value = !mod.unusable.empty() ? "Cannot be used" : on ? "On" : "Off";
        if (on && !conflicts_of(wanted, mod.id).empty()) value += ", conflict";
        std::string description = mod.type + (mod.author.empty() ? "" : " by " + mod.author) + ". " +
                                  (mod.unusable.empty() ? changes_text(session, mod) : mod.unusable);
        const std::string label = std::to_string(i + 1u) + ". " + mod.name;
        ImGui::PushID(mod.id.c_str());
        if (s.return_to == mod.id) {
            focus_next_row();
            s.return_to.clear();
        }
        if (value_row(label.c_str(), value, {false, {}, description})) {
            s.mod = mod.id;
            go(Stage::Details);
        }
        ImGui::PopID();
    }
    if (!wanted.conflicts.empty()) {
        section("Conflicts");
        indented("These files are changed by more than one mod that is on. The mod higher in the list wins; patches "
                 "apply on top of the winning replacement.");
        for (const Resolution::Conflict &c : wanted.conflicts)
            info_row(file_name(session, c.file).c_str(), conflict_text(session, c));
    }
    if (!mods::problems().empty()) {
        section("Problems this run");
        for (const std::string &problem : mods::problems()) indented(problem, colors::kDanger);
    }
}

} // namespace

void mods_page(bool back) {
    State &s = state();
    ModSession *session = mods::session();
    if (session == nullptr) {
        section("Mods");
        indented("Mods need the game's disc image, which is not open.");
        return;
    }
    switch (s.stage) {
    case Stage::List: list(*session); break;
    case Stage::Details: details(*session, back); break;
    case Stage::Choose: browse(back); break;
    case Stage::Review: review(*session, back); break;
    case Stage::Result: result_screen(back); break;
    }
}

bool mods_screen_open() { return state().stage != Stage::List; }

bool take_mods_restart_request() { return std::exchange(state().restart, false); }

} // namespace mhp3rd::ui
