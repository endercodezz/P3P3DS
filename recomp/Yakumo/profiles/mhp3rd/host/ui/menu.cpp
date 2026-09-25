// The in-game menu: Esc, or L3+R3 on a gamepad. It holds every player setting;
// each change applies at once and is saved to settings.ini straight away. It
// pauses the game, or, as the settings say, stays over the running game.

#include "ui/ui.hpp"

#include "ui/font_menu.hpp"
#if defined(MHP3RD_DEBUG_MENU)
#include "debug/debug_tools.hpp"
#include "ui/debug_screen.hpp"
#endif
#include "ui/input_script.hpp"
#include "ui/layer.hpp"
#include "ui/mods_screen.hpp"
#include "ui/save_screen.hpp"
#include "ui/texture_pack_screen.hpp"
#include "ui/text_input.hpp"
#include "ui/touch_overlay.hpp"
#include "ui/widgets.hpp"

#include "adhoc/client.hpp"
#include "adhoc/discovery.hpp"
#include "adhoc/session.hpp"
#include "audio/audio_sink.hpp"
#include "gpu/vulkan_renderer.hpp"
#include "input/bindings.hpp"
#include "install/game_identity.hpp"
#include "install/installer.hpp"
#include "install/user_data.hpp"
#include "perf/frame_stats.hpp"
#include "save_data/save_transfer.hpp"
#include "settings/settings.hpp"
#include "yakumo_version.hpp"
#if defined(MHP3RD_ANDROID_APP)
#include "platform/android_documents.hpp"
#endif

#include "imgui.h"
#include "imgui_internal.h"

#include <SDL3/SDL.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace mhp3rd::ui {
namespace {

using Clock = std::chrono::steady_clock;

// Seconds the "how to open the menu" hint stays up at start, until the menu
// has been opened once.
constexpr double kHintSeconds = 12.0;
// The longest hunter name the game takes: its name buffer holds 12
// characters and a terminator.
constexpr std::size_t kHunterNameLength = 12u;
// Resolutions the menu offers. MHP3RD_INTERNAL_SCALE goes up to 8, but a
// setting that runs out of video memory would fail on every start.
constexpr int kMenuMaxInternalScale = 6;

std::string size_text(std::uint32_t scale) {
    return std::to_string(480u * scale) + "×" + std::to_string(272u * scale);
}

// Row options for the setting stored under `key`: when an environment
// variable decides it for this run, the row is shown but locked.
RowOptions options_for(const char *key, std::string description) {
    RowOptions options;
    options.description = std::move(description);
    if (const char *variable = settings::overridden_by(key)) {
        options.disabled = true;
        options.note = std::string("Set by ") + variable;
    }
    return options;
}

float font_gap() { return Layer::get().font_size() * 0.5f; }

int cycle(int value, int delta, int count) { return ((value + delta) % count + count) % count; }

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

float gain(const settings::Settings &s) { return s.mute ? 0.0f : static_cast<float>(s.volume) / 100.0f; }

class Menu {
public:
    explicit Menu(bool paused) : paused_(paused) {}
    // One frame; false once the menu closes.
    bool frame();
    [[nodiscard]] bool quit() const noexcept { return quit_; }

private:
    enum class Confirm { None, Quit, Setup };

    void video();
    void audio();
    void controls();
    void network();
    void mods();
    void system();
    bool confirm_dialog();

    gpu::VulkanRenderer &renderer() { return Layer::get().renderer(); }

    bool paused_{};  // the game is paused behind the menu, rather than running
    int tab_{};
    bool first_frame_{true};
    bool close_{};
    bool quit_{};
    bool was_editing_{};
    bool back_{};  // the back button was pressed this frame
    Confirm confirm_{Confirm::None};
    bool confirm_opened_{};  // the confirmation was on screen last frame
    std::optional<input::Action> binding_;  // waiting for a key for this control
};

bool Menu::frame() {
    Layer &layer = Layer::get();
    // A text field's keyboard takes the whole screen until it closes.
    if (text_input_open()) {
        text_input_frame();
        return true;
    }
    // A texture pack copy keeps the menu open until it ends.
    texture_pack_import_tick();
    if (layer.take_menu_toggle() && !texture_pack_import_busy()) return false;
    const bool back = layer.take_back();
    // The pad's back button closes the menu too, once nothing is being edited.
    const ImGuiKey cancel = layer.confirm_south() ? ImGuiKey_GamepadFaceRight : ImGuiKey_GamepadFaceDown;
    const bool pad_back = ImGui::IsKeyPressed(cancel, false);
    const bool start = ImGui::IsKeyPressed(ImGuiKey_GamepadStart, false);
    // Back closes the font list, or the save import and export, before it
    // closes the menu.
    bool font_list_was_open = (tab_ == 0 && (font_list_open() || texture_pack_screen_open())) ||
                              (tab_ == 4 && mods_screen_open()) || (tab_ == 5 && save_screen_open());
#if defined(MHP3RD_DEBUG_MENU)
    font_list_was_open = font_list_was_open || (tab_ == 6 && debug_screen_open());
#endif
    back_ = back || pad_back;

    begin_panel("##menu", "Yakumo", paused_ ? "Paused" : "Running", true);
    static const char *const kTabs[] = {"Video", "Audio", "Controls", "Network", "Mods", "System", "Debug"};
#if defined(MHP3RD_DEBUG_MENU)
    // The developer tools' page, in developer builds run with MHP3RD_DEBUG_MENU=1.
    const int tab_count = debug::enabled() ? 7 : 6;
#else
    const int tab_count = 6;
#endif
    const bool switched = tab_bar(kTabs, tab_count, tab_) || first_frame_;
    first_frame_ = false;
    begin_content();
    if (switched) {
        focus_next_row();
        ImGui::SetScrollY(0.0f);
    }
    switch (tab_) {
    case 0: video(); break;
    case 1: audio(); break;
    case 2: controls(); break;
    case 3: network(); break;
    case 4: mods(); break;
#if defined(MHP3RD_DEBUG_MENU)
    case 6: debug_page(back_); break;
#endif
    default: system(); break;
    }
    begin_footer();
    if (tab_ >= 4)
        hints({{Control::Confirm, "Select"}, {Control::Back, "Back"}, {Control::Tabs, "Section"},
               {Control::Menu, "Resume"}});
    else
        hints({{Control::Confirm, "Select"},
               {Control::Change, "Change"},
               {Control::Back, "Back"},
               {Control::Tabs, "Section"},
               {Control::Menu, "Resume"}});
    end_panel();

    if (confirm_ != Confirm::None) {
        if ((back || pad_back) && confirm_opened_) confirm_ = Confirm::None;
    } else if (!font_list_was_open && (((back || pad_back) && !was_editing_) || start)) {
        close_ = true;
    }
    if (close_ && !quit_ && texture_pack_import_busy()) close_ = false;
    if (!confirm_dialog()) return false;
    was_editing_ = ImGui::IsAnyItemActive();
    return !close_;
}

void Menu::video() {
    if (font_list(back_)) return;
    if (texture_pack_screen(back_)) return;
    settings::Settings &s = settings::current();
    section("Picture");
    {
        RowOptions o = options_for("video.internal_scale",
                                   "Auto draws the game at the window's own size and follows it (at most 1632 "
                                   "lines). ×1 to ×6 draw 272 lines per step, 480×272 times the step unless the "
                                   "aspect ratio is Fill. Higher is sharper and needs more from the GPU.");
        const std::array<std::uint32_t, 2> size = renderer().target_size();
        const std::string drawn = std::to_string(size[0]) + "×" + std::to_string(size[1]);
        const std::string value =
            (s.internal_scale == 0u ? std::string("Auto") : "×" + std::to_string(s.internal_scale)) + "   " + drawn;
        if (const int delta = choice_row("Resolution", value, o)) {
            // Auto, then ×1 up to the menu's largest (or a larger one a
            // variable once chose).
            const int limit = std::max<int>(kMenuMaxInternalScale, static_cast<int>(s.internal_scale));
            s.internal_scale =
                static_cast<std::uint32_t>(cycle(static_cast<int>(s.internal_scale), delta, limit + 1));
            renderer().set_internal_scale(s.internal_scale);
            settings::save();
        }
    }
#if !defined(__ANDROID__)
    // A phone is always full screen: no window to size.
    {
        const int delta =
            choice_row("Display", s.fullscreen ? "Fullscreen" : "Window",
                       options_for("video.fullscreen", "Fill the screen, or play in a window you can resize."));
        if (delta != 0) {
            s.fullscreen = !s.fullscreen;
            renderer().set_fullscreen(s.fullscreen);
            settings::save();
        }
    }
    {
        RowOptions o = options_for("video.window_scale", "The window's size, in multiples of the PSP's screen.");
        if (s.fullscreen) {
            o.disabled = true;
            o.note = "Fullscreen";
        }
        const std::string value = "×" + std::to_string(s.window_scale) + "   " + size_text(s.window_scale);
        if (const int delta = choice_row("Window size", value, o)) {
            s.window_scale = static_cast<std::uint32_t>(
                cycle(static_cast<int>(s.window_scale) - 1, delta, static_cast<int>(settings::kMaxWindowScale)) + 1);
            renderer().set_window_scale(s.window_scale);
            settings::save();
        }
    }
#endif
    {
        static const char *const kAspects[] = {"Original", "Stretch", "Fill"};
        const int current = static_cast<int>(s.aspect);
        if (const int delta = choice_row(
                "Aspect ratio", kAspects[current],
                options_for("video.aspect", "Original keeps the PSP's shape with black bars at the sides or top. "
                                            "Stretch fills the window by stretching the picture. Fill widens (or "
                                            "narrows) the game's view to the window's shape, keeping its height, "
                                            "and keeps the interface in the PSP's proportions."))) {
            s.aspect = static_cast<settings::Aspect>(cycle(current, delta, 3));
            renderer().set_aspect(s.aspect);
            settings::save();
        }
    }
    if (choice_row("Scaling filter", s.sharp_screen ? "Sharp" : "Smooth",
                   options_for("video.sharp_screen", "How the finished picture is scaled to the window: smooth "
                                                     "(bilinear) or sharp (nearest pixel)."))) {
        s.sharp_screen = !s.sharp_screen;
        renderer().set_sharp_screen(s.sharp_screen);
        settings::save();
    }
    if (choice_row("Texture filter", s.sharp_textures ? "Sharp" : "Smooth",
                   options_for("video.sharp_textures", "How the game's textures are sampled: smooth (bilinear) or "
                                                       "sharp (nearest texel)."))) {
        s.sharp_textures = !s.sharp_textures;
        renderer().set_sharp_textures(s.sharp_textures);
        settings::save();
    }
    {
        RowOptions o = options_for("video.texture_pack",
                                   "Draws an HD texture pack in PPSSPP's format from textures/NPJB40001 in the data "
                                   "folder instead of the game's textures.");
        // The footer shows the note under the description: what is loaded,
        // or where the pack was looked for.
        if (o.note.empty()) {
            const std::string status = renderer().texture_pack_status();
            o.note = status == "Not installed" ? "No pack in " + renderer().texture_pack_folder() : status;
            // A pack used in place that has moved: its path is on the "Pack
            // used from" row below; the footer has room for its name only.
            if (status.rfind("Folder missing: ", 0) == 0)
                o.note = "Pack folder missing: " +
                         install::path_to_utf8(install::path_from_utf8(status.substr(16)).filename());
        }
        if (choice_row("Texture pack", s.texture_pack ? "On" : "Off", o)) {
            s.texture_pack = !s.texture_pack;
            renderer().set_texture_pack(s.texture_pack);
            settings::save();
        }
    }
    texture_pack_rows();
    section("Timing");
    {
        struct Mode {
            settings::PresentMode mode;
            const char *name;
        };
        std::vector<Mode> modes{{settings::PresentMode::Fifo, "On"}};
        if (renderer().supports_present_mode(settings::PresentMode::Mailbox))
            modes.push_back({settings::PresentMode::Mailbox, "Off (mailbox)"});
        if (renderer().supports_present_mode(settings::PresentMode::Immediate))
            modes.push_back({settings::PresentMode::Immediate, "Off (immediate)"});
        int current = 0;
        for (std::size_t i = 0; i < modes.size(); ++i)
            if (modes[i].mode == s.present_mode) current = static_cast<int>(i);
        if (const int delta = choice_row(
                "Vsync", modes[static_cast<std::size_t>(current)].name,
                options_for("video.present_mode", "On waits for the display's refresh and never tears. Off shows "
                                                  "each frame at once; the game's speed is the same either way."))) {
            s.present_mode = modes[static_cast<std::size_t>(cycle(current, delta, static_cast<int>(modes.size())))].mode;
            renderer().set_present_mode(s.present_mode);
            settings::save();
        }
    }
    {
        static const char *const kRates[] = {"30", "45", "60", "90", "120", "Match display"};
        RowOptions o = options_for("video.frame_rate",
                                   "Frames between the game's 30 a second, blending its movement. Steps down by "
                                   "itself rather than slow the game.");
        if (s.unthrottled && !o.disabled) {
            o.disabled = true;
            o.note = "Game speed is Unlimited";
        }
        const int current = static_cast<int>(s.frame_rate);
        std::string value = kRates[current];
        if (s.frame_rate == settings::FrameRate::Display && renderer().display_refresh() > 0.0f)
            value += "  " + std::to_string(static_cast<int>(std::lround(renderer().display_refresh()))) + " Hz";
        if (s.frame_rate != settings::FrameRate::Fps30 && !o.disabled) {
            // Vsync caps the rate at the display's, and the renderer steps
            // down rather than slow the game.
            const double now = renderer().frame_rate_now();
            const double chosen = s.frame_rate == settings::FrameRate::Display
                                      ? static_cast<double>(renderer().display_refresh())
                                      : std::stod(kRates[current]);
            if (now + 0.5 < chosen) {
                value += " (running at " + std::to_string(static_cast<int>(std::lround(now))) + ")";
                o.description = s.present_mode == settings::PresentMode::Fifo && renderer().display_refresh() > 0.0f &&
                                        now + 0.5 >= static_cast<double>(renderer().display_refresh())
                                    ? "With Vsync on, no faster than the display refreshes."
                                    : "Lowered to keep the game at full speed; it tries the chosen rate again later.";
            }
        }
        if (const int delta = choice_row("Frame rate", value, o)) {
            s.frame_rate = static_cast<settings::FrameRate>(cycle(current, delta, 6));
            renderer().set_frame_rate(s.frame_rate);
            settings::save();
        }
    }
    {
        RowOptions o = options_for("video.frame_rate_auto",
                                   "On lowers the frame rate by itself when presenting that often would slow the "
                                   "game. Off keeps the chosen rate, and the game may then run below full speed.");
        if (s.unthrottled || s.frame_rate == settings::FrameRate::Fps30) o.disabled = true;
        if (choice_row("Lower when behind", s.frame_rate_auto ? "On" : "Off", o)) {
            s.frame_rate_auto = !s.frame_rate_auto;
            renderer().set_frame_rate_auto(s.frame_rate_auto);
            settings::save();
        }
    }
    if (choice_row("Game speed", s.unthrottled ? "Unlimited" : "Normal",
                   options_for("video.unthrottled", "Normal holds the game to real time. Unlimited lets it run as "
                                                    "fast as frames can be drawn, which also speeds up the game."))) {
        s.unthrottled = !s.unthrottled;
        settings::save();
    }
    {
        RowOptions o = options_for("video.fast_loading",
                                   "Lets the game run ahead of real time while it loads and is silent, so loads "
                                   "take as long as the computer needs. Never during play, sound, movies or "
                                   "ad hoc play.");
        if (s.unthrottled && !o.disabled) {
            o.disabled = true;
            o.note = "Game speed is Unlimited";
        }
        if (toggle_row("Fast loading", s.fast_loading, o)) {
            s.fast_loading = !s.fast_loading;
            settings::save();
        }
    }
    {
        static const char *const kPerf[] = {"Off", "Overlay", "Overlay and log", "Log only"};
        const int current = static_cast<int>(s.perf);
        if (const int delta = choice_row(
                "Performance", kPerf[current],
                options_for("video.performance", "Frame times and speed in the top-left corner, and a [perf] line "
                                                 "per second on the console. F3 shows or hides the overlay."))) {
            s.perf = static_cast<settings::PerfDisplay>(cycle(current, delta, 4));
            renderer().set_perf_overlay(perf::options().overlay);
            settings::save();
        }
    }
    font_rows();
    ImGui::Dummy({0.0f, font_gap()});
    if (button_row("Restore video defaults", {false, {}, "Every setting on this page back to how Yakumo ships."})) {
        const settings::Settings &d = settings::defaults();
        const auto restore = [&](const char *key, auto &value, const auto &fallback) {
            if (settings::overridden_by(key) == nullptr) value = fallback;
        };
        restore("video.internal_scale", s.internal_scale, d.internal_scale);
        restore("video.fullscreen", s.fullscreen, d.fullscreen);
        restore("video.window_scale", s.window_scale, d.window_scale);
        restore("video.aspect", s.aspect, d.aspect);
        restore("video.sharp_screen", s.sharp_screen, d.sharp_screen);
        restore("video.sharp_textures", s.sharp_textures, d.sharp_textures);
        restore("video.texture_pack", s.texture_pack, d.texture_pack);
        restore("video.present_mode", s.present_mode, d.present_mode);
        restore("video.frame_rate", s.frame_rate, d.frame_rate);
        restore("video.frame_rate_auto", s.frame_rate_auto, d.frame_rate_auto);
        restore("video.unthrottled", s.unthrottled, d.unthrottled);
        restore("video.fast_loading", s.fast_loading, d.fast_loading);
        restore("video.performance", s.perf, d.perf);
        renderer().set_internal_scale(s.internal_scale);
        renderer().set_fullscreen(s.fullscreen);
        renderer().set_window_scale(s.window_scale);
        renderer().set_aspect(s.aspect);
        renderer().set_sharp_screen(s.sharp_screen);
        renderer().set_sharp_textures(s.sharp_textures);
        renderer().set_texture_pack(s.texture_pack);
        renderer().set_present_mode(s.present_mode);
        renderer().set_frame_rate(s.frame_rate);
        renderer().set_frame_rate_auto(s.frame_rate_auto);
        renderer().set_perf_overlay(perf::options().overlay);
        settings::save();
    }
}

void Menu::audio() {
    settings::Settings &s = settings::current();
    audio::AudioSink &sink = audio::AudioSink::instance();
    const bool device = sink.has_device();
    const auto locked = [&](const char *key, const char *description) {
        RowOptions options = options_for(key, description);
        if (!device) {
            options.disabled = true;
            options.note = std::getenv("MHP3RD_NO_AUDIO") != nullptr ? "Off: MHP3RD_NO_AUDIO" : "No audio device";
        }
        return options;
    };
    section("Output");
    int volume = static_cast<int>(s.volume);
    if (slider_row("Volume", volume, 0, 100, 5, "%d%%",
                   locked("audio.volume", "Loudness of everything the game plays."))) {
        s.volume = static_cast<std::uint32_t>(volume);
        sink.set_volume(gain(s));
        settings::save();
    }
    if (toggle_row("Mute", s.mute, locked("audio.mute", "Silence the game without losing the volume setting."))) {
        s.mute = !s.mute;
        sink.set_volume(gain(s));
        settings::save();
    }
    info_row("Device", device ? (paused_ ? "44100 Hz stereo, paused while this menu is open" : "44100 Hz stereo")
                              : "None");
    ImGui::Dummy({0.0f, font_gap()});
    if (button_row("Restore audio defaults", {!device, {}, "Full volume, not muted."})) {
        s.volume = settings::defaults().volume;
        s.mute = settings::defaults().mute;
        sink.set_volume(gain(s));
        settings::save();
    }
}

// A text field row. Returns true when an edit was committed; the new text is
// then in `value`. `id` keeps the field's state apart from other rows'.
// Activated with a gamepad, the field opens the on-screen keyboard; with a
// keyboard or mouse it is edited in place.
struct TextField {
    std::array<char, 132> buffer{};
    bool focused{};
    std::optional<std::string> entered;  // from the on-screen keyboard
    bool refocus{};                      // the keyboard closed; focus the row again
};

bool text_row(const char *id, const char *label, std::string &value, std::size_t max_length, bool allow_empty,
              const std::function<bool(char32_t)> &allowed, const RowOptions &o) {
    static std::map<std::string, TextField> fields;
    TextField &field = fields[id];
    bool committed = false;
    if (field.entered) {
        if (allow_empty || !field.entered->empty()) {
            value = std::move(*field.entered);
            committed = true;
        }
        field.entered.reset();
    }
    if (!ImGui::IsAnyItemActive() || !field.focused)
        std::snprintf(field.buffer.data(), field.buffer.size(), "%s", value.c_str());
    const float row_height = std::round(Layer::get().font_size() * 1.9f);
    const ImVec2 start = ImGui::GetCursorScreenPos();
    const float width = ImGui::GetContentRegionAvail().x;
    ImDrawList *draw = ImGui::GetWindowDrawList();
    // Highlighted like the other rows; the field reports its focus only after
    // it is drawn, so last frame's state is used.
    if (field.focused)
        draw->AddRectFilled(start, {start.x + width, start.y + row_height}, colors::kRowFocus,
                            std::round(6.0f * Layer::get().scale()));
    draw->AddText({start.x + std::round(16.0f * Layer::get().scale()),
                   start.y + (row_height - Layer::get().font_size()) * 0.5f},
                  o.disabled ? colors::kTextDisabled : colors::kText, label);
    const float field_width = std::min(width * 0.45f, Layer::get().font_size() * 12.0f);
    ImGui::SetCursorScreenPos({start.x + width - field_width - std::round(16.0f * Layer::get().scale()),
                               start.y + (row_height - ImGui::GetFrameHeight()) * 0.5f});
    ImGui::SetNextItemWidth(field_width);
    if (o.disabled) ImGui::BeginDisabled();
    const auto filter = [](ImGuiInputTextCallbackData *data) {
        const auto *accepts = static_cast<const std::function<bool(char32_t)> *>(data->UserData);
        return (*accepts)(static_cast<char32_t>(data->EventChar)) ? 0 : 1;
    };
    const std::string widget_id = std::string("##") + id;
    ImGui::InputText(widget_id.c_str(), field.buffer.data(), std::min(max_length + 1u, field.buffer.size()),
                     ImGuiInputTextFlags_CallbackCharFilter, filter,
                     const_cast<std::function<bool(char32_t)> *>(&allowed));
    if (ImGui::IsItemActivated() && Layer::get().input_device() == InputDevice::Gamepad) {
        ImGui::ClearActiveID();
        TextInputRequest request;
        request.title = label;
        request.prompt = o.description;
        request.initial = value;
        request.max_length = max_length;
        request.allowed = allowed;
        open_text_input(std::move(request), [key = std::string(id)](std::optional<std::string> text) {
            TextField &closed = fields[key];
            closed.refocus = true;
            if (text) closed.entered = std::move(*text);
        });
    }
    // The menu was not drawn while the keyboard was up, so its focus is lost.
    if (field.refocus && !text_input_open()) {
        field.refocus = false;
        ImGui::FocusItem();
    }
    field.focused = ImGui::IsItemFocused();
    if (ImGui::IsItemFocused() || ImGui::IsItemHovered()) {
        std::string description = o.description;
        if (!o.note.empty()) description += "\n" + o.note;
        Layer::get().set_description(description);
    }
    if (ImGui::IsItemDeactivatedAfterEdit() && (allow_empty || field.buffer[0] != '\0')) {
        value = field.buffer.data();
        committed = true;
    }
    if (o.disabled) ImGui::EndDisabled();
    ImGui::SetCursorScreenPos({start.x, start.y + row_height});
    ImGui::Dummy({0.0f, 0.0f});
    return committed;
}

// Host names: printable ASCII without spaces.
bool host_character(char32_t c) { return c > 0x20u && c < 0x7Fu; }

// What an address to join may hold: a host name or IPv4 address, a colon and
// a port, or an IPv6 address in brackets.
bool address_character(char32_t c) {
    return (c >= U'0' && c <= U'9') || (c >= U'a' && c <= U'z') || (c >= U'A' && c <= U'Z') || c == U'.' ||
           c == U':' || c == U'-' || c == U'[' || c == U']';
}

void Menu::controls() {
    settings::Settings &s = settings::current();
    section("Gamepad");
    {
        SDL_Gamepad *pad = renderer().gamepad();
        const char *name = pad != nullptr ? SDL_GetGamepadName(pad) : nullptr;
        info_row("Connected", pad == nullptr ? "No gamepad; the keyboard and mouse drive the game"
                                             : (name != nullptr ? name : "Gamepad"));
    }
    if (choice_row("Confirm button", s.confirm_south ? "Bottom (Western)" : "Right, ○ (Japanese)",
                   options_for("input.confirm", "Which face button confirms, in the game and in this menu. The "
                                                "game's prompts show ○ to confirm and × to go back."))) {
        s.confirm_south = !s.confirm_south;
        settings::save();
    }
    int dead_zone = static_cast<int>(std::lround(s.dead_zone * 100.0f));
    if (slider_row("Stick dead zone", dead_zone, 0, 50, 1, "%d%%",
                   options_for("input.dead_zone", "How far the left stick moves before the hunter does. Raise it if "
                                                  "the hunter drifts."))) {
        s.dead_zone = static_cast<float>(dead_zone) / 100.0f;
        settings::save();
    }
    int trigger = static_cast<int>(std::lround(s.trigger * 100.0f));
    if (slider_row("Trigger point", trigger, 5, 100, 5, "%d%%",
                   options_for("input.trigger", "How far LT/RT (L2/R2) travel before they press anything."))) {
        s.trigger = static_cast<float>(trigger) / 100.0f;
        settings::save();
    }
    {
        static const char *const kProfiles[] = {"Standard (L / R)", "Bows (R / △)", "Bowguns (R / ○)"};
        const int current = static_cast<int>(s.trigger_profile);
        if (const int delta = choice_row(
                "Trigger profile", kProfiles[current],
                options_for("input.trigger_profile",
                            "What LT/RT (L2/R2) press. Standard: L and R, like the shoulders. Bows: R to aim and △ "
                            "to shoot. Bowguns: R to aim and ○ to fire. R1, △ and ○ keep working."))) {
            s.trigger_profile = static_cast<settings::TriggerProfile>(cycle(current, delta, 3));
            settings::save();
        }
    }
    {
        static const char *const kModes[] = {"Camera", "D-pad", "Off"};
        const int current = static_cast<int>(s.right_stick);
        if (const int delta = choice_row(
                "Right stick", kModes[current],
                options_for("input.right_stick", "Camera uses the HD release's own right-stick camera. D-pad "
                                                 "presses the D-pad instead, like the PSP's camera controls."))) {
            s.right_stick = static_cast<settings::RightStick>(cycle(current, delta, 3));
            settings::save();
        }
    }
    // On Android a finger drag drives the analog camera whatever the right
    // stick does, so the row stays open there.
    const bool camera = s.right_stick == settings::RightStick::Camera ||
                        settings::kPlatform == settings::Platform::Android;
    {
        RowOptions o = options_for("input.analog_camera",
                                   "Turn and tilt the quest camera as far as the stick is pushed, instead of the "
                                   "game's fixed-speed turn and vertical presets. Release holds the angle; the D-pad "
                                   "and recentre return to the game's camera. Off is the game's own camera, untouched.");
        if (!camera && !o.disabled) {
            o.disabled = true;
            o.note = "Right stick is not the camera";
        }
        if (toggle_row("Analog camera", s.analog_camera, o)) {
            s.analog_camera = !s.analog_camera;
            settings::save();
        }
        o = options_for("input.camera_speed", "How fast the camera turns at full deflection, in degrees a second.");
        if (!s.analog_camera && !o.disabled) {
            o.disabled = true;
            o.note = "Analog camera is off";
        }
        int speed = static_cast<int>(s.camera_speed);
        if (slider_row("Camera speed", speed, 20, 720, 10, "%d deg/s", o)) {
            s.camera_speed = static_cast<float>(speed);
            settings::save();
        }
        o = options_for("input.aim_speed", "How fast a bow or a bowgun aims at full deflection, in degrees a "
                                           "second. The game's own aim moves at about 100 and only past half "
                                           "the stick's travel.");
        if (!s.analog_camera && !o.disabled) {
            o.disabled = true;
            o.note = "Analog camera is off";
        }
        int aim = static_cast<int>(s.aim_speed);
        if (slider_row("Aim speed", aim, 10, 360, 5, "%d deg/s", o)) {
            s.aim_speed = static_cast<float>(aim);
            settings::save();
        }
    }
    {
        RowOptions o = options_for("input.invert_camera_x", "Turn the camera the other way left and right.");
        if (!camera && !o.disabled) {
            o.disabled = true;
            o.note = "Right stick is not the camera";
        }
        if (toggle_row("Invert camera horizontally", s.invert_camera_x, o)) {
            s.invert_camera_x = !s.invert_camera_x;
            settings::save();
        }
        o = options_for("input.invert_camera_y", "Turn the camera the other way up and down.");
        if (!camera && !o.disabled) {
            o.disabled = true;
            o.note = "Right stick is not the camera";
        }
        if (toggle_row("Invert camera vertically", s.invert_camera_y, o)) {
            s.invert_camera_y = !s.invert_camera_y;
            settings::save();
        }
    }
    {
        RowOptions o = options_for("input.right_stick_zone", "How far the right stick moves before it presses the "
                                                             "D-pad.");
        if (s.right_stick != settings::RightStick::DPad && !o.disabled) {
            o.disabled = true;
            o.note = "Right stick is not the D-pad";
        }
        int zone = static_cast<int>(std::lround(s.right_stick_zone * 100.0f));
        if (slider_row("Right stick D-pad point", zone, 10, 100, 5, "%d%%", o)) {
            s.right_stick_zone = static_cast<float>(zone) / 100.0f;
            settings::save();
        }
    }

    section("Hunter name");
    {
        const bool keyboard = s.name_entry == settings::NameEntry::Keyboard;
        if (choice_row("When the game asks for a name", keyboard ? "On-screen keyboard" : "Use the name below",
                       options_for("input.name_entry",
                                   "On-screen keyboard: type the name with the gamepad or the keyboard while the "
                                   "game waits. Otherwise the name below is given at once."))) {
            s.name_entry = keyboard ? settings::NameEntry::Fixed : settings::NameEntry::Keyboard;
            settings::save();
        }
    }
    if (text_row("name", "Hunter name", s.name, kHunterNameLength, false, hunter_name_character,
                 options_for("input.name", "Given when the game asks for a name and the on-screen keyboard is "
                                           "off, and to other players when the network nickname is empty. "
                                           "Letters, digits, spaces and simple punctuation.")))
        settings::save();

    section("Keyboard and mouse");
    {
        RowOptions o = options_for("input.mouse",
                                   "While the game runs, the window takes the pointer: moving the mouse turns the "
                                   "camera and its buttons press what they are bound to. Esc opens this menu and "
                                   "gives the pointer back.");
        if (toggle_row("Mouse", s.mouse, o)) {
            s.mouse = !s.mouse;
            settings::save();
        }
        o = options_for("input.mouse_sensitivity",
                        "Degrees the camera turns for each count of mouse motion. While a bow or a bowgun aims, "
                        "the mouse is slowed as Aim speed is to Camera speed.");
        if (!s.mouse && !o.disabled) {
            o.disabled = true;
            o.note = "Mouse is off";
        }
        int sensitivity = static_cast<int>(std::lround(s.mouse_sensitivity * 100.0f));
        if (slider_row("Mouse sensitivity", sensitivity, 1, 99, 1, "0.%02d deg", o)) {
            s.mouse_sensitivity = static_cast<float>(sensitivity) / 100.0f;
            settings::save();
        }
        o = options_for("input.invert_mouse_x", "Turn the camera the other way when the mouse moves sideways.");
        if (!s.mouse && !o.disabled) {
            o.disabled = true;
            o.note = "Mouse is off";
        }
        if (toggle_row("Invert mouse horizontally", s.invert_mouse_x, o)) {
            s.invert_mouse_x = !s.invert_mouse_x;
            settings::save();
        }
        o = options_for("input.invert_mouse_y", "Push the mouse forward to look down instead of up.");
        if (!s.mouse && !o.disabled) {
            o.disabled = true;
            o.note = "Mouse is off";
        }
        if (toggle_row("Invert mouse vertically", s.invert_mouse_y, o)) {
            s.invert_mouse_y = !s.invert_mouse_y;
            settings::save();
        }
    }
    // One row per control: activate it, then press a key or a mouse button.
    Layer &layer = Layer::get();
    if (binding_) {
        if (const auto pressed = layer.take_captured_binding()) {
            if (*pressed != input::kNone) {
                input::assign(s.bindings, *binding_, *pressed);
                settings::save();
            }
            binding_.reset();
        } else if (!layer.capturing_binding()) {
            binding_.reset();
        }
    }
    for (std::size_t i = 0; i < input::kActions; ++i) {
        const auto action = static_cast<input::Action>(i);
        std::string value = input::format(s.bindings[i]);
        if (binding_ == action) value = "Press a key or a mouse button";
        else if (value.empty()) value = "None";
        const RowOptions o{false, {},
                           "Press a key or a mouse button to add it, or one it has already to remove it; Esc "
                           "cancels. A key taken from another control leaves that one."};
        if (value_row(input::info(action).label, value, o) && !binding_) {
            binding_ = action;
            layer.begin_binding_capture();
        }
    }
    info_row("Esc", "This menu");
    info_row("F3", "Performance overlay");

    ImGui::Dummy({0.0f, font_gap()});
    if (button_row("Restore control defaults",
                   {false, {}, "Every gamepad, keyboard, mouse, touch and name setting back to how Yakumo ships."})) {
        const settings::Settings &d = settings::defaults();
        const auto restore = [&](const char *key, auto &value, const auto &fallback) {
            if (settings::overridden_by(key) == nullptr) value = fallback;
        };
        restore("input.confirm", s.confirm_south, d.confirm_south);
        restore("input.dead_zone", s.dead_zone, d.dead_zone);
        restore("input.trigger", s.trigger, d.trigger);
        restore("input.trigger_profile", s.trigger_profile, d.trigger_profile);
        restore("input.right_stick", s.right_stick, d.right_stick);
        restore("input.right_stick_zone", s.right_stick_zone, d.right_stick_zone);
        restore("input.analog_camera", s.analog_camera, d.analog_camera);
        restore("input.camera_speed", s.camera_speed, d.camera_speed);
        restore("input.aim_speed", s.aim_speed, d.aim_speed);
        restore("input.invert_camera_x", s.invert_camera_x, d.invert_camera_x);
        restore("input.invert_camera_y", s.invert_camera_y, d.invert_camera_y);
        restore("input.name_entry", s.name_entry, d.name_entry);
        restore("input.name", s.name, d.name);
        restore("input.mouse", s.mouse, d.mouse);
        restore("input.mouse_sensitivity", s.mouse_sensitivity, d.mouse_sensitivity);
        restore("input.invert_mouse_x", s.invert_mouse_x, d.invert_mouse_x);
        restore("input.invert_mouse_y", s.invert_mouse_y, d.invert_mouse_y);
        restore("input.touch_controls", s.touch_controls, d.touch_controls);
        restore("input.touch_dpad", s.touch_dpad, d.touch_dpad);
        restore("input.touch_opacity", s.touch_opacity, d.touch_opacity);
        restore("input.touch_size", s.touch_size, d.touch_size);
        restore("input.touch_camera_speed", s.touch_camera_speed, d.touch_camera_speed);
        s.bindings = d.bindings;
        settings::save();
    }

    section("Touch screen");
    {
        RowOptions o = options_for("input.touch_controls",
                                   "A pad drawn over the game once the screen is touched: a stick that appears "
                                   "where the left thumb lands, the face buttons on the right, L and R at the top "
                                   "corners. A drag on the free right half turns the camera. It hides again when a "
                                   "gamepad or the keyboard is used.");
        if (toggle_row("On-screen controls", s.touch_controls, o)) {
            s.touch_controls = !s.touch_controls;
            settings::save();
        }
        const auto off = [&](RowOptions options) {
            if (!s.touch_controls && !options.disabled) {
                options.disabled = true;
                options.note = "On-screen controls are off";
            }
            return options;
        };
        if (toggle_row("D-pad", s.touch_dpad,
                       off(options_for("input.touch_dpad", "A D-pad at the left edge, for the game's menus, the item "
                                                           "box and the camera's D-pad controls. Off gives its place "
                                                           "to the stick.")))) {
            s.touch_dpad = !s.touch_dpad;
            settings::save();
        }
        int opacity = static_cast<int>(std::lround(s.touch_opacity * 100.0f));
        if (slider_row("Controls opacity", opacity, 10, 100, 5, "%d%%",
                       off(options_for("input.touch_opacity", "How strongly the on-screen controls are drawn.")))) {
            s.touch_opacity = static_cast<float>(opacity) / 100.0f;
            settings::save();
        }
        int size = static_cast<int>(std::lround(s.touch_size * 100.0f));
        if (slider_row("Controls size", size, 60, 160, 5, "%d%%",
                       off(options_for("input.touch_size", "The size of the on-screen controls.")))) {
            s.touch_size = static_cast<float>(size) / 100.0f;
            settings::save();
        }
        int speed = static_cast<int>(std::lround(s.touch_camera_speed));
        if (slider_row("Touch camera speed", speed, 30, 720, 10, "%d deg",
                       off(options_for("input.touch_camera_speed",
                                       "Degrees the camera turns for a drag across the height of the screen.")))) {
            s.touch_camera_speed = static_cast<float>(speed);
            settings::save();
        }
    }
    if (button_row("Use the classic keyboard layout",
                   {false, {}, "The keys of earlier versions, for play without a mouse: I J K L move, Z X A S are "
                               "the face buttons, Q and W are L and R."})) {
        s.bindings = input::classic_bindings();
        settings::save();
    }
}

std::string format_duration(std::uint64_t ms) {
    const std::uint64_t seconds = ms / 1000u;
    if (seconds < 60u) return std::to_string(seconds) + " s";
    if (seconds < 3600u) return std::to_string(seconds / 60u) + " min " + std::to_string(seconds % 60u) + " s";
    return std::to_string(seconds / 3600u) + " h " + std::to_string(seconds / 60u % 60u) + " min";
}

std::string format_bytes(std::uint64_t bytes) {
    char text[32];
    if (bytes < 10'000u) std::snprintf(text, sizeof(text), "%llu B", static_cast<unsigned long long>(bytes));
    else if (bytes < 10'000'000u) std::snprintf(text, sizeof(text), "%.1f KB", static_cast<double>(bytes) / 1024.0);
    else std::snprintf(text, sizeof(text), "%.1f MB", static_cast<double>(bytes) / (1024.0 * 1024.0));
    return text;
}

// One line of what the client is doing, for the menu and the overlay.
std::string connection_text(const adhoc::Diagnostics &d) {
    switch (d.state) {
    case adhoc::ServerState::Off:
        return d.server.empty() || !settings::current().adhoc ? "Off line" : "Off line (the game is not on line)";
    case adhoc::ServerState::Connecting:
        if (d.failed_attempts == 0u) return "Connecting…";
        return "Reconnecting, attempt " + std::to_string(d.failed_attempts + 1u) +
               (d.last_error.empty() ? "" : " (" + d.last_error + ")");
    case adhoc::ServerState::Online: break;
    }
    std::string text = "On line";
    if (d.online_ms) text += " for " + format_duration(*d.online_ms);
    if (d.rtt_ms) {
        char rtt[32];
        std::snprintf(rtt, sizeof(rtt), ", %.0f ms round trip", *d.rtt_ms);
        text += rtt;
    }
    return text;
}

std::string group_text(const adhoc::Diagnostics &d) {
    if (d.group) {
        std::string text = *d.group + ", " + std::to_string(d.peers.size() + 1u) + " players";
        if (d.rejoin_ms) text += ", rejoining for " + format_duration(*d.rejoin_ms);
        return text;
    }
    if (d.joining) return "Joining " + *d.joining + "…";
    return "None";
}

std::string traffic_text(const adhoc::Traffic &t) {
    return "in " + std::to_string(t.packets_in) + " (" + format_bytes(t.bytes_in) + "), out " +
           std::to_string(t.packets_out) + " (" + format_bytes(t.bytes_out) + ")";
}

// The on-screen network overlay (menu: Network, or MHP3RD_ADHOC_OVERLAY).
bool &network_overlay() {
    static bool shown = [] {
        const char *text = std::getenv("MHP3RD_ADHOC_OVERLAY");
        return text != nullptr && *text != '\0' && std::string(text) != "0";
    }();
    return shown;
}

std::string &saved_log_path() {
    static std::string path;
    return path;
}

// "MHP3Q000" is Hall 01 in the game's list.
std::string group_name(const std::string &group) {
    if (group.size() == 8u && group.compare(0, 5, "MHP3Q") == 0 &&
        std::all_of(group.begin() + 5, group.end(), [](char c) { return c >= '0' && c <= '9'; })) {
        char text[16];
        std::snprintf(text, sizeof(text), "Hall %02d", std::atoi(group.c_str() + 5) + 1);
        return text;
    }
    return group;
}

std::string players_text(std::size_t count) {
    return std::to_string(count) + (count == 1u ? " player" : " players");
}

// Hosting and joining a session.
void play_together() {
    settings::Settings &s = settings::current();
    static std::string copied;
    static std::string typed_address;
    section("Play together");
    if (adhoc_hosting()) {
        const adhoc::ServerStatus status = adhoc_host_status();
        const std::string suffix =
            status.adhocctl_port == adhoc::kAdhocctlPort ? std::string() : ":" + std::to_string(status.adhocctl_port);
        info_row("Hosting", players_text(status.players.size()) + " connected. Everyone now enters the Online Guild "
                                                                  "Hall and picks the same hall.");
        const std::vector<adhoc::LocalAddress> addresses = adhoc::local_addresses();
        if (addresses.empty()) info_row("Your addresses", "No network is connected");
        for (const adhoc::LocalAddress &address : addresses) {
            const std::string text = address.address + suffix;
            const std::string label =
                text + "   " + address.network + " (" + address.interface + ")###address " + address.address;
            if (button_row(label.c_str(),
                           {false, {},
                            "Copies this address. Players on the same network find you under Join; over a VPN "
                            "without broadcast, such as Tailscale, they type your VPN address there."})) {
                SDL_SetClipboardText(text.c_str());
                copied = text;
            }
        }
        if (!copied.empty()) info_row("Copied", copied);
        ImGui::PushID("hosted players");
        for (std::size_t i = 0; i < status.players.size(); ++i) {
            const adhoc::ServerPlayer &player = status.players[i];
            ImGui::PushID(static_cast<int>(i));
            info_row(player.nickname.c_str(),
                     player.address + "   " + (player.group ? group_name(*player.group) : std::string("not in a hall")));
            ImGui::PopID();
        }
        ImGui::PopID();
        // One ID for Host and Stop, so the focus stays on the row.
        if (button_row("Stop hosting###hosting",
                       {false, {},
                        "Stops the server. Everyone in the session is disconnected, as when a connection drops."},
                       colors::kDanger)) {
            adhoc_host_stop();
            copied.clear();
        }
    } else {
        if (button_row("Host a session###hosting",
                       {false, {},
                        "Runs a server in this game for the others to join: no other program, no port forwarding "
                        "on a local network or a VPN. Then everyone enters the Online Guild Hall."}))
            adhoc_host_start();
        if (const std::string error = adhoc_host_error(); !error.empty()) info_row("Cannot host", error);
    }

    section("Join a session");
    adhoc::Discovery &discovery = adhoc::Discovery::get();
    discovery.start_listening();
    discovery.query(s.adhoc_recent);
    const std::vector<adhoc::FoundHost> hosts = discovery.hosts();
    const bool hosting = adhoc_hosting();
    const auto joined = [&](const std::string &address) {
        return !hosting && s.adhoc && s.adhoc_server == address;
    };
    for (const adhoc::FoundHost &host : hosts) {
        const std::string address = host.join_address();
        const std::string label = (joined(address) ? "Joined " : "Join ") + host.info.name + "   " + address + ", " +
                                  players_text(host.info.players) + "###found " + std::to_string(host.info.session);
        if (button_row(label.c_str(), {false, {}, "A session hosted on this network. Joining it takes you out of any "
                                                  "other; then enter the Online Guild Hall."}))
            adhoc_join(address);
    }
    if (hosts.empty()) info_row("On this network", "Looking for hosted sessions…");
    if (text_row("join_address", "Address", typed_address, 100u, true, address_character,
                 {false, {},
                  "The host's address, for a VPN without broadcast such as Tailscale: the host's screen lists it. "
                  "Confirming it joins."}) &&
        !typed_address.empty()) {
        adhoc_join(typed_address);
        typed_address.clear();
    }
    for (const std::string &address : s.adhoc_recent) {
        // A recent session that is announcing is listed above already.
        if (std::any_of(hosts.begin(), hosts.end(),
                        [&](const adhoc::FoundHost &host) { return host.join_address() == address; }))
            continue;
        const std::string label = (joined(address) ? "Joined " : "Join ") + address + "   recent###recent " + address;
        if (button_row(label.c_str(), {false, {}, "A session you joined before."})) adhoc_join(address);
    }
}

void Menu::network() {
    settings::Settings &s = settings::current();
    adhoc::Client &client = adhoc::Client::get();
    const adhoc::Diagnostics d = client.diagnostics();
    play_together();
    section("Ad hoc play");
    if (toggle_row("Ad hoc play", s.adhoc,
                   options_for("network.adhoc", "Multiplayer with other players. Off, the game says the wireless "
                                                "switch is off. Turning it off in a gathering hall leaves it."))) {
        s.adhoc = !s.adhoc;
        settings::save();
        adhoc_apply_settings();
    }
    if (text_row("server", "Server", s.adhoc_server, 100u, true, host_character,
                 options_for("network.server", "The session or PSP ad hoc server the game goes on line with: host "
                                               "name or address, host:port if it is not on 27312. Joining fills "
                                               "it in; for a public server, type its name. Applies the next time "
                                               "the game goes on line."))) {
        settings::save();
        adhoc_apply_settings();
    }
    if (text_row("nickname", "Nickname", s.adhoc_nickname, 32u, true, printable_ascii,
                 options_for("network.nickname", "The name other players and the server see. Empty: the hunter name. "
                                                 "Applies the next time the game goes on line."))) {
        settings::save();
        adhoc_apply_settings();
    }

    section("Status");
    info_row("Connection", connection_text(d));
    if (adhoc_hosting()) {
        const adhoc::ServerStatus st = adhoc_host_status();
        info_row("Server", "TCP " + std::to_string(st.adhocctl_port) + " and " + std::to_string(st.relay_port) +
                               ", up " + format_duration(st.uptime_ms) + ", " + players_text(st.players.size()) +
                               ", " + std::to_string(st.groups) + " halls, " + std::to_string(st.relay_sessions) +
                               " relay connections, " + std::to_string(st.streams) + " streams");
        info_row("Relayed", std::to_string(st.relayed_packets) + " packets (" + format_bytes(st.relayed_bytes) +
                                "), " + std::to_string(st.dropped) + " datagrams dropped");
    }
    {
        const adhoc::DiscoveryStatus ds = adhoc::Discovery::get().status();
        std::string text = ds.listening ? "Listening on UDP " + std::to_string(adhoc::kDiscoveryPort) + ", " +
                                              std::to_string(ds.hosts) + " hosts heard"
                                        : (ds.listen_error.empty() ? "Not listening" : ds.listen_error);
        if (ds.announcing)
            text += "; announcing " + ds.announce_note + ", " + std::to_string(ds.announcements_sent) + " sent, " +
                    std::to_string(ds.queries_answered) + " queries answered";
        info_row("Discovery", text);
        for (const adhoc::FoundHost &host : adhoc::Discovery::get().hosts())
            info_row(("Host " + host.info.name).c_str(),
                     host.join_address() + ", " + players_text(host.info.players) + ", " + host.info.product +
                         ", heard " + format_duration(host.heard_ms) + " ago");
    }
    if (!d.server_address.empty()) info_row("Server address", d.server_address);
    info_row("You", (s.adhoc_mac.empty() ? std::string("address made up on first use") : s.adhoc_mac) +
                        (d.nickname.empty() ? "" : "   " + d.nickname));
    info_row("Group", group_text(d));
    for (const adhoc::PeerSummary &peer : d.peers)
        info_row(peer.nickname.empty() ? "Player" : peer.nickname.c_str(),
                 adhoc::format_mac(peer.mac) + "   " +
                     (peer.last_heard_ms ? "heard " + format_duration(*peer.last_heard_ms) + " ago" : "not heard yet"));
    for (const adhoc::SocketSummary &socket : d.sockets) {
        const std::string label = socket.kind + " " + std::to_string(socket.port);
        info_row(label.c_str(), socket.state + (socket.peer ? "   " + adhoc::format_mac(*socket.peer) + " port " +
                                                                  std::to_string(socket.peer_port)
                                                            : std::string{}));
    }
    if (d.relay_links_wanted != 0u)
        info_row("Relay links", std::to_string(d.relay_links_up) + " of " + std::to_string(d.relay_links_wanted) +
                                    " up");
    info_row("Per second", traffic_text(d.per_second));
    info_row("Since start", traffic_text(d.total));
    info_row("Problems", std::to_string(d.dropped) + " datagrams dropped, " + std::to_string(d.timeouts) +
                             " calls timed out, " + std::to_string(d.reconnects) + " reconnections");

    section("Troubleshooting");
    if (toggle_row("Network overlay", network_overlay(),
                   {false, {}, "A small panel over the game with the connection, the group and the traffic."}))
        network_overlay() = !network_overlay();
    if (toggle_row("Log every call and packet", adhoc::Client::tracing(),
                   {false, {}, "The same as MHP3RD_TRACE_ADHOC=1: every ad hoc call and packet header goes to the "
                               "console and to the network log. Busy; for finding a problem."}))
        adhoc::Client::set_tracing(!adhoc::Client::tracing());
    if (button_row("Save network log",
                   {false, {}, "Writes the recent network log and this page's state to a file in the data folder's "
                               "logs folder, to send with a problem report."})) {
        std::filesystem::path directory;
        try {
            directory = install::user_data_directory() / "logs";
        } catch (const std::exception &) {
            directory = "logs";
        }
        const std::filesystem::path path = client.save_log(directory);
        saved_log_path() = path.empty() ? "Could not write to " + install::path_to_utf8(directory)
                                        : install::path_to_utf8(path);
        std::cout << "[adhoc] network log: " << saved_log_path() << std::endl;
    }
    if (!saved_log_path().empty()) info_row("Saved", saved_log_path());
    if (button_row("Reconnect now", {d.state == adhoc::ServerState::Off, {},
                                     "Drop the server connection and connect again at once. The group is joined "
                                     "again; a quest in progress may end, as when a connection drops."}))
        client.reconnect_now();
    if (button_row("Disconnect", {!d.group && !d.joining, {},
                                  "Leave the group as if the other players were lost. The game shows its own "
                                  "disconnection message."},
                   colors::kDanger))
        client.disconnect_now();

    ImGui::Dummy({0.0f, font_gap()});
    if (button_row("Restore network defaults", {false, {}, "Ad hoc play off and no server. Your address stays."})) {
        const settings::Settings &defaults = settings::defaults();
        const auto restore = [&](const char *key, auto &value, const auto &fallback) {
            if (settings::overridden_by(key) == nullptr) value = fallback;
        };
        restore("network.adhoc", s.adhoc, defaults.adhoc);
        restore("network.server", s.adhoc_server, defaults.adhoc_server);
        restore("network.nickname", s.adhoc_nickname, defaults.adhoc_nickname);
        settings::save();
        adhoc_apply_settings();
    }
}

void Menu::mods() {
    mods_page(back_);
    if (take_mods_restart_request()) {
        install::request_restart_on_exit();
        quit_ = true;
        close_ = true;
    }
}

void Menu::system() {
    if (save_screen(back_)) {
        if (take_restart_request()) {
            install::request_restart_on_exit();
            quit_ = true;
            close_ = true;
        }
        return;
    }
    std::string data_dir;
    try {
        data_dir = install::path_to_utf8(install::user_data_directory());
    } catch (const std::exception &e) {
        data_dir = e.what();
    }
    section("Game");
    if (button_row("Resume", {false, {}, "Back to the game."})) close_ = true;
    settings::Settings &s = settings::current();
    if (toggle_row("Pause the game when the menu opens", s.menu_pause,
                   options_for("ui.menu_pause", "On: the game stops while this menu is open. Off: it keeps running "
                                                "and playing sound behind the menu, which takes all input. Applies "
                                                "the next time the menu opens."))) {
        s.menu_pause = !s.menu_pause;
        settings::save();
    }
    if (toggle_row("Pause during multiplayer", s.menu_pause_multiplayer,
                   options_for("ui.menu_pause_multiplayer",
                               "In ad hoc play the game keeps running behind the menu unless this is on: a paused "
                               "game stops answering the other players and can drop a quest. Applies the next time "
                               "the menu opens."))) {
        s.menu_pause_multiplayer = !s.menu_pause_multiplayer;
        settings::save();
    }
#if defined(MHP3RD_ANDROID_APP)
    // An Android app's data folder is out of the file manager's reach; its
    // log goes where the player picks instead, to send with a report.
    (void)data_dir;
    if (button_row("Save the log…", {false, {}, "Copies Yakumo's logs (this run's, the previous run's and the logs "
                                                "folder) to a folder you pick, to send with a problem report."})) {
        std::fflush(stdout);
        std::fflush(stderr);
        std::error_code ec;
        const std::filesystem::path storage = install::user_data_directory();
        const std::filesystem::path local = storage / "transfer" /
                                            ("Yakumo log " + savedata::timestamp_for_path(std::chrono::system_clock::now()));
        std::filesystem::remove_all(local.parent_path(), ec);
        std::filesystem::create_directories(local, ec);
        std::filesystem::copy_file(storage / "yakumo.log", local / "yakumo.log", ec);
        std::filesystem::copy_file(storage / "yakumo-previous.log", local / "yakumo-previous.log", ec);
        if (std::filesystem::is_directory(storage / "logs", ec))
            std::filesystem::copy(storage / "logs", local / "logs", std::filesystem::copy_options::recursive, ec);
        const auto copied = android::pick_folder_and_copy(local);
        std::filesystem::remove_all(local.parent_path(), ec);
        if (copied)
            saved_log_path() = copied->error.empty() ? copied->where + "/" + install::path_to_utf8(local.filename())
                                                     : "Not saved: " + copied->error;
    }
    if (!saved_log_path().empty()) info_row("Log", saved_log_path());
#else
    if (button_row("Open the data folder", {false, {}, "Show Yakumo's data folder in the file manager."})) {
        if (!SDL_OpenURL(file_url(data_dir).c_str()))
            std::cout << "[menu] cannot open " << data_dir << ": " << SDL_GetError() << "\n";
    }
#endif
    if (button_row("Set up game data again…",
                   {false, {}, "Choose the disc image again, for example after moving it. The game closes first."}))
        confirm_ = Confirm::Setup;
    if (button_row("Quit game", {false, {}, "Close Yakumo. Progress since your last save is lost."},
                   colors::kDanger))
        confirm_ = Confirm::Quit;

    section("Saves");
    save_rows();

    section("About");
    info_row("Yakumo", std::string(kYakumoVersion));
    info_row("Game", std::string(install::kGameTitle) + " (" + install::kDiscIdDisplay + ")");
    info_row("Data folder", data_dir);
    if (!savedata::memory_stick().empty())
        info_row("Saves folder", install::path_to_utf8(savedata::memory_stick() / "PSP" / "SAVEDATA"));
    info_row("Graphics", "Vulkan on " + renderer().device_name());
    info_row("Interface", std::string("Dear ImGui ") + IMGUI_VERSION);
}

// The quit and setup confirmations. False when the menu should close.
bool Menu::confirm_dialog() {
    if (confirm_ != Confirm::None && !ImGui::IsPopupOpen("##confirm")) ImGui::OpenPopup("##confirm");
    const ImGuiIO &io = ImGui::GetIO();
    const float font = Layer::get().font_size();
    ImGui::SetNextWindowPos({io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f}, ImGuiCond_Always, {0.5f, 0.5f});
    ImGui::SetNextWindowSize({std::min(io.DisplaySize.x * 0.9f, font * 26.0f), 0.0f}, ImGuiCond_Always);
    bool keep_open = true;
    confirm_opened_ = false;
    if (ImGui::BeginPopupModal("##confirm", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoSavedSettings)) {
        if (confirm_ == Confirm::None) {
            ImGui::CloseCurrentPopup();
            ImGui::EndPopup();
            return true;
        }
        confirm_opened_ = true;
        const bool quit = confirm_ == Confirm::Quit;
        heading(quit ? "Quit the game?" : "Set up game data again?");
        paragraph(quit ? "Progress since your last save is lost."
                       : "Yakumo closes the game and opens the setup, where you choose the disc image again. "
                         "Progress since your last save is lost.",
                  colors::kTextDim);
        ImGui::Dummy({0.0f, font * 0.6f});
        const float gap = font * 0.6f;
        const float width = (ImGui::GetContentRegionAvail().x - gap) * 0.5f;
        if (big_button(quit ? "Quit" : "Close and set up", width, true)) {
            if (!quit) install::request_setup_on_exit();
            quit_ = true;
            keep_open = false;
        }
        ImGui::SameLine(0.0f, gap);
        if (big_button("Cancel", width)) confirm_ = Confirm::None;
        // Cancel is the safe default.
        ImGui::SetItemDefaultFocus();
        ImGui::Dummy({0.0f, font * 0.2f});
        ImGui::Dummy({0.0f, 0.0f});
        hints({{Control::Confirm, "Select"}, {Control::Back, "Cancel"}});
        if (confirm_ == Confirm::None || !keep_open) ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }
    return keep_open;
}

// Seconds the menu hint has left, or a negative number once it is gone.
double hint_seconds_left() {
    static const Clock::time_point first_frame = Clock::now();
    return kHintSeconds - std::chrono::duration<double>(Clock::now() - first_frame).count();
}

// The hint shown over the game until the menu has been opened once.
void draw_hint(double seconds_left) {
    Layer &layer = Layer::get();
    const ImGuiIO &io = ImGui::GetIO();
    const float font = layer.font_size();
    const float alpha = static_cast<float>(std::clamp(seconds_left, 0.0, 1.0));
    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, alpha);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {font * 0.8f, font * 0.5f});
    ImGui::SetNextWindowPos({io.DisplaySize.x * 0.5f, io.DisplaySize.y - font}, ImGuiCond_Always, {0.5f, 1.0f});
    ImGui::Begin("##hint", nullptr,
                 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_AlwaysAutoResize |
                     ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav);
    hints({{Control::Menu, "Settings and pause"}});
    ImGui::End();
    ImGui::PopStyleVar(2);
}

// The network overlay: a few lines in the top-right corner.
void draw_network_overlay() {
    const adhoc::Diagnostics d = adhoc::Client::get().diagnostics();
    const ImGuiIO &io = ImGui::GetIO();
    const float font = Layer::get().font_size();
    ImGui::SetNextWindowPos({io.DisplaySize.x - font * 0.5f, font * 0.5f}, ImGuiCond_Always, {1.0f, 0.0f});
    ImGui::SetNextWindowBgAlpha(0.7f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {font * 0.5f, font * 0.3f});
    ImGui::Begin("##network", nullptr,
                 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_AlwaysAutoResize |
                     ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav);
    ImGui::SetWindowFontScale(0.75f);
    ImGui::TextUnformatted(connection_text(d).c_str());
    if (adhoc_hosting())
        ImGui::TextUnformatted(("Hosting: " + players_text(adhoc_host_status().players.size())).c_str());
    ImGui::TextUnformatted(("Group: " + group_text(d)).c_str());
    for (const adhoc::PeerSummary &peer : d.peers)
        ImGui::TextUnformatted(("  " + peer.nickname + (peer.last_heard_ms ? "  " + format_duration(*peer.last_heard_ms)
                                                                           : std::string("  -")))
                                   .c_str());
    std::size_t streams = 0;
    for (const adhoc::SocketSummary &socket : d.sockets)
        if (socket.kind != "PDP" && socket.state == "established") ++streams;
    ImGui::TextUnformatted(("Links " + std::to_string(d.relay_links_up) + "/" + std::to_string(d.relay_links_wanted) +
                            ", streams " + std::to_string(streams))
                               .c_str());
    ImGui::TextUnformatted(("/s " + traffic_text(d.per_second)).c_str());
    if (d.dropped != 0u || d.timeouts != 0u)
        ImGui::TextUnformatted(
            ("Dropped " + std::to_string(d.dropped) + ", timeouts " + std::to_string(d.timeouts)).c_str());
    ImGui::End();
    ImGui::PopStyleVar();
}

// The menu while it is open over the running game, and a quit chosen in it.
std::optional<Menu> &menu_over_game_state() {
    static std::optional<Menu> menu;
    return menu;
}
bool &quit_requested() {
    static bool requested = false;
    return requested;
}
Clock::time_point &menu_opened_at() {
    static Clock::time_point opened;
    return opened;
}

void note_menu_opened(bool paused) {
    settings::Settings &s = settings::current();
    if (!s.menu_hint_seen) {
        s.menu_hint_seen = true;
        settings::save();
    }
    std::cout << (paused ? "[menu] opened; the game is paused" : "[menu] opened; the game keeps running")
              << std::endl;
    menu_opened_at() = Clock::now();
}

void note_menu_closed(bool quit) {
    const double seconds = std::chrono::duration<double>(Clock::now() - menu_opened_at()).count();
    char text[64];
    std::snprintf(text, sizeof(text), "%.1f", seconds);
    std::cout << "[menu] closed after " << text << " s" << (quit ? "; quitting" : "") << std::endl;
}

} // namespace

bool attach(gpu::VulkanRenderer &renderer) { return Layer::get().attach(renderer); }

void draw_over_game() {
    Layer &layer = Layer::get();
    if (!layer.attached()) return;
    script::tick();
    std::optional<Menu> &menu = menu_over_game_state();
    // The game's keyboard request: drawn over every game frame until done. A
    // keyboard opened from the menu over the running game is the menu's.
    if (text_input_open() && !menu) {
        layer.begin_frame();
        text_input_frame();
        layer.end_frame();
        return;
    }
    const double hint_left = menu || settings::current().menu_hint_seen ? -1.0 : hint_seconds_left();
    const bool overlay = network_overlay();
    const bool touch = !menu && layer.renderer().touch_controls_visible();
    if (hint_left <= 0.0 && !overlay && !menu && !touch) return;
    layer.begin_frame();
    if (touch) draw_touch_controls(layer.renderer().touch_controls(), settings::current().touch_opacity);
    if (hint_left > 0.0) draw_hint(hint_left);
    if (overlay) draw_network_overlay();
    if (menu && !menu->frame()) {
        const bool quit = menu->quit() || layer.window_closed();
        menu.reset();
        layer.set_interactive(false);
        layer.renderer().set_game_input(true);
        note_menu_closed(quit);
        if (quit) quit_requested() = true;
    }
    layer.end_frame();
}

bool menu_pauses() {
    const settings::Settings &s = settings::current();
    return adhoc_session_active() ? s.menu_pause_multiplayer : s.menu_pause;
}

void open_menu_over_game() {
    std::optional<Menu> &menu = menu_over_game_state();
    if (menu) return;
    Layer &layer = Layer::get();
    note_menu_opened(false);
    layer.renderer().set_game_input(false);
    layer.set_interactive(true);
    menu.emplace(false);
}

bool menu_over_game() { return menu_over_game_state().has_value(); }

bool take_quit_request() { return std::exchange(quit_requested(), false); }

bool menu_requested() {
    Layer &layer = Layer::get();
    if (!layer.attached() || text_input_open()) return false;
    // The on-screen menu button, then Esc or L3+R3.
    const bool touched = layer.renderer().take_touch_menu();
    return layer.take_menu_toggle() || touched;
}

bool run_menu() {
    Layer &layer = Layer::get();
    note_menu_opened(true);
    layer.renderer().set_game_input(false);
    layer.set_interactive(true);
    Menu menu(true);
    const bool window_open = layer.run([&] { return menu.frame(); }, true);
    layer.set_interactive(false);
    layer.renderer().set_game_input(true);
    note_menu_closed(menu.quit());
    return window_open && !menu.quit();
}

} // namespace mhp3rd::ui
