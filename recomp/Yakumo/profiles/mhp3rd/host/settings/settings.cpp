#include "settings/settings.hpp"

#include "install/user_data.hpp"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace mhp3rd::settings {
namespace {

// One setting: its key in settings.ini, the variable that overrides it, and
// how both spell its value.
struct Field {
    const char *key;
    const char *variable;  // null: no environment override
    std::function<bool(Settings &, const std::string &)> parse;
    std::function<std::string(const Settings &)> format;
    // Reads the variable's value, which is spelled the way the variable has
    // always been. Null: the variable uses the file's spelling.
    std::function<void(Settings &, const char *)> parse_variable;
};

bool parse_bool(const std::string &text, bool &out) {
    if (text == "1" || text == "true" || text == "on" || text == "yes") out = true;
    else if (text == "0" || text == "false" || text == "off" || text == "no") out = false;
    else return false;
    return true;
}

bool parse_float(const std::string &text, float minimum, float maximum, float &out) {
    char *end = nullptr;
    const float value = std::strtof(text.c_str(), &end);
    if (end == text.c_str() || *end != '\0') return false;
    out = std::clamp(value, minimum, maximum);
    return true;
}

bool parse_uint(const std::string &text, std::uint32_t minimum, std::uint32_t maximum, std::uint32_t &out) {
    char *end = nullptr;
    const unsigned long value = std::strtoul(text.c_str(), &end, 10);
    if (end == text.c_str() || *end != '\0') return false;
    out = static_cast<std::uint32_t>(std::clamp<unsigned long>(value, minimum, maximum));
    return true;
}

// A resolution: a multiple of 480x272, or "auto" (0) for the window's own.
bool parse_scale(const std::string &text, std::uint32_t &out) {
    if (text == "auto") {
        out = 0u;
        return true;
    }
    return parse_uint(text, 0u, kMaxInternalScale, out);
}

std::string format_float(float value) {
    char text[32];
    std::snprintf(text, sizeof(text), "%.2f", static_cast<double>(value));
    return text;
}

// Flags the host has always read as "set means on", whatever the value.
bool variable_present(const char *) { return true; }

// Flags read the way the pad code reads them: 0, no, off and false are off.
bool variable_flag(const char *text) {
    for (const char *off : {"0", "no", "off", "false"})
        if (std::strcmp(text, off) == 0) return false;
    return true;
}

float variable_float(const char *text, float fallback, float minimum, float maximum) {
    char *end = nullptr;
    const float value = std::strtof(text, &end);
    return std::clamp(end != text ? value : fallback, minimum, maximum);
}

template <typename Enum>
struct Names {
    std::vector<std::pair<Enum, const char *>> values;

    bool parse(const std::string &text, Enum &out) const {
        for (const auto &[value, name] : values) {
            if (text == name) {
                out = value;
                return true;
            }
        }
        return false;
    }
    [[nodiscard]] std::string format(Enum value) const {
        for (const auto &[candidate, name] : values)
            if (candidate == value) return name;
        return values.front().second;
    }
};

const Names<PresentMode> kPresentModes{
    {{PresentMode::Fifo, "vsync"}, {PresentMode::Mailbox, "mailbox"}, {PresentMode::Immediate, "immediate"}}};
const Names<Aspect> kAspects{{{Aspect::Original, "original"}, {Aspect::Stretch, "stretch"}, {Aspect::Fill, "fill"}}};
const Names<PerfDisplay> kPerfDisplays{{{PerfDisplay::Off, "off"},
                                        {PerfDisplay::Overlay, "overlay"},
                                        {PerfDisplay::OverlayAndLog, "overlay+log"},
                                        {PerfDisplay::Log, "log"}}};
const Names<RightStick> kRightSticks{
    {{RightStick::Camera, "camera"}, {RightStick::DPad, "dpad"}, {RightStick::Off, "off"}}};
const Names<TriggerProfile> kTriggerProfiles{{{TriggerProfile::Standard, "standard"},
                                              {TriggerProfile::Bows, "bows"},
                                              {TriggerProfile::Bowguns, "bowguns"}}};
const Names<NameEntry> kNameEntries{{{NameEntry::Keyboard, "keyboard"}, {NameEntry::Fixed, "fixed"}}};
const Names<FrameRate> kFrameRates{{{FrameRate::Fps30, "30"},
                                    {FrameRate::Fps45, "45"},
                                    {FrameRate::Fps60, "60"},
                                    {FrameRate::Fps90, "90"},
                                    {FrameRate::Fps120, "120"},
                                    {FrameRate::Display, "display"}}};

// Written by earlier versions: 1 typed the name into the window, which the
// on-screen keyboard now covers.
constexpr const char *kRetiredTypeNameKey = "input.type_name";

#define BOOL_FIELD(key, member)                                                                                       \
    Field {                                                                                                            \
        key, nullptr, [](Settings &s, const std::string &t) { return parse_bool(t, s.member); },                       \
            [](const Settings &s) { return std::string(s.member ? "1" : "0"); }, nullptr                               \
    }

const std::vector<Field> &fields() {
    static const std::vector<Field> table = {
        {"video.internal_scale", "MHP3RD_INTERNAL_SCALE",
         [](Settings &s, const std::string &t) { return parse_scale(t, s.internal_scale); },
         [](const Settings &s) { return s.internal_scale == 0u ? std::string("auto") : std::to_string(s.internal_scale); },
         [](Settings &s, const char *t) {
             std::uint32_t value = s.internal_scale;
             if (parse_scale(t, value)) s.internal_scale = value;
         }},
        {"video.window_scale", nullptr,
         [](Settings &s, const std::string &t) { return parse_uint(t, 1u, kMaxWindowScale, s.window_scale); },
         [](const Settings &s) { return std::to_string(s.window_scale); }, nullptr},
        BOOL_FIELD("video.fullscreen", fullscreen),
        {"video.present_mode", nullptr,
         [](Settings &s, const std::string &t) { return kPresentModes.parse(t, s.present_mode); },
         [](const Settings &s) { return kPresentModes.format(s.present_mode); }, nullptr},
        // Written by earlier versions, which had Original and Stretch only.
        // Still written, so going back to one of them keeps the choice as
        // near as it can; video.aspect follows it and decides.
        {"video.keep_aspect", nullptr,
         [](Settings &s, const std::string &t) {
             bool keep = true;
             if (!parse_bool(t, keep)) return false;
             s.aspect = keep ? Aspect::Original : Aspect::Stretch;
             return true;
         },
         [](const Settings &s) { return std::string(s.aspect == Aspect::Stretch ? "0" : "1"); }, nullptr},
        {"video.aspect", nullptr, [](Settings &s, const std::string &t) { return kAspects.parse(t, s.aspect); },
         [](const Settings &s) { return kAspects.format(s.aspect); }, nullptr},
        BOOL_FIELD("video.sharp_screen", sharp_screen),
        BOOL_FIELD("video.sharp_textures", sharp_textures),
        {"video.texture_pack", "MHP3RD_TEXTURE_PACK",
         [](Settings &s, const std::string &t) { return parse_bool(t, s.texture_pack); },
         [](const Settings &s) { return std::string(s.texture_pack ? "1" : "0"); },
         // 0/off/no/false turn it off; anything else, a folder included, on.
         [](Settings &s, const char *t) { s.texture_pack = variable_flag(t); }},
        {"video.texture_pack_folder", nullptr,
         [](Settings &s, const std::string &t) {
             s.texture_pack_folder = t;
             return true;
         },
         [](const Settings &s) { return s.texture_pack_folder; }, nullptr},
        {"video.unthrottled", "MHP3RD_UNTHROTTLED",
         [](Settings &s, const std::string &t) { return parse_bool(t, s.unthrottled); },
         [](const Settings &s) { return std::string(s.unthrottled ? "1" : "0"); },
         [](Settings &s, const char *t) { s.unthrottled = variable_present(t); }},
        {"video.fast_loading", "MHP3RD_FAST_LOADING",
         [](Settings &s, const std::string &t) { return parse_bool(t, s.fast_loading); },
         [](const Settings &s) { return std::string(s.fast_loading ? "1" : "0"); },
         [](Settings &s, const char *t) { s.fast_loading = variable_flag(t); }},
        {"video.frame_rate", "MHP3RD_FRAME_RATE",
         [](Settings &s, const std::string &t) { return kFrameRates.parse(t, s.frame_rate); },
         [](const Settings &s) { return kFrameRates.format(s.frame_rate); },
         [](Settings &s, const char *t) {
             if (!kFrameRates.parse(t, s.frame_rate)) s.frame_rate = FrameRate::Fps30;
         }},
        {"video.frame_rate_auto", "MHP3RD_FRAME_RATE_AUTO",
         [](Settings &s, const std::string &t) { return parse_bool(t, s.frame_rate_auto); },
         [](const Settings &s) { return std::string(s.frame_rate_auto ? "1" : "0"); },
         [](Settings &s, const char *t) { s.frame_rate_auto = variable_flag(t); }},
        {"video.performance", "MHP3RD_PERF",
         [](Settings &s, const std::string &t) { return kPerfDisplays.parse(t, s.perf); },
         [](const Settings &s) { return kPerfDisplays.format(s.perf); },
         [](Settings &s, const char *t) {
             // `1` has always meant the overlay and the log, `log` the log only.
             if (std::strcmp(t, "log") == 0) s.perf = PerfDisplay::Log;
             else s.perf = *t != '\0' && variable_flag(t) ? PerfDisplay::OverlayAndLog : PerfDisplay::Off;
         }},
        {"text.font", "MHP3RD_FONT",
         [](Settings &s, const std::string &t) {
             s.font = t;
             return true;
         },
         [](const Settings &s) { return s.font; }, [](Settings &s, const char *t) { s.font = t; }},
        {"text.weight", nullptr,
         [](Settings &s, const std::string &t) { return parse_uint(t, 0u, kMaxFontWeight, s.font_weight); },
         [](const Settings &s) { return std::to_string(s.font_weight); }, nullptr},
        {"audio.volume", nullptr,
         [](Settings &s, const std::string &t) { return parse_uint(t, 0u, 100u, s.volume); },
         [](const Settings &s) { return std::to_string(s.volume); }, nullptr},
        BOOL_FIELD("audio.mute", mute),
        {"input.confirm", "MHP3RD_PAD_FACE",
         [](Settings &s, const std::string &t) {
             if (t == "south") s.confirm_south = true;
             else if (t == "east") s.confirm_south = false;
             else return false;
             return true;
         },
         [](const Settings &s) { return std::string(s.confirm_south ? "south" : "east"); },
         [](Settings &s, const char *t) { s.confirm_south = std::strcmp(t, "xbox") == 0 || std::strcmp(t, "south") == 0; }},
        {"input.dead_zone", "MHP3RD_PAD_DEADZONE",
         [](Settings &s, const std::string &t) { return parse_float(t, 0.0f, 0.9f, s.dead_zone); },
         [](const Settings &s) { return format_float(s.dead_zone); },
         [](Settings &s, const char *t) { s.dead_zone = variable_float(t, 0.15f, 0.0f, 0.9f); }},
        {"input.trigger", "MHP3RD_PAD_TRIGGER",
         [](Settings &s, const std::string &t) { return parse_float(t, 0.05f, 1.0f, s.trigger); },
         [](const Settings &s) { return format_float(s.trigger); },
         [](Settings &s, const char *t) { s.trigger = variable_float(t, 0.25f, 0.05f, 1.0f); }},
        {"input.trigger_profile", "MHP3RD_PAD_TRIGGERS",
         [](Settings &s, const std::string &t) { return kTriggerProfiles.parse(t, s.trigger_profile); },
         [](const Settings &s) { return kTriggerProfiles.format(s.trigger_profile); },
         [](Settings &s, const char *t) {
             if (!kTriggerProfiles.parse(t, s.trigger_profile)) s.trigger_profile = TriggerProfile::Standard;
         }},
        {"input.right_stick", "MHP3RD_PAD_RSTICK_DPAD",
         [](Settings &s, const std::string &t) { return kRightSticks.parse(t, s.right_stick); },
         [](const Settings &s) { return kRightSticks.format(s.right_stick); },
         [](Settings &s, const char *t) { s.right_stick = variable_flag(t) ? RightStick::DPad : RightStick::Camera; }},
        {"input.right_stick_zone", "MHP3RD_PAD_RSTICK_ZONE",
         [](Settings &s, const std::string &t) { return parse_float(t, 0.1f, 1.0f, s.right_stick_zone); },
         [](const Settings &s) { return format_float(s.right_stick_zone); },
         [](Settings &s, const char *t) { s.right_stick_zone = variable_float(t, 0.5f, 0.1f, 1.0f); }},
        {"input.analog_camera", "MHP3RD_ANALOG_CAMERA",
         [](Settings &s, const std::string &t) { return parse_bool(t, s.analog_camera); },
         [](const Settings &s) { return std::string(s.analog_camera ? "1" : "0"); },
         [](Settings &s, const char *t) { s.analog_camera = variable_flag(t); }},
        {"input.camera_speed", "MHP3RD_CAMERA_SPEED",
         [](Settings &s, const std::string &t) { return parse_float(t, 20.0f, 720.0f, s.camera_speed); },
         [](const Settings &s) { return format_float(s.camera_speed); },
         [](Settings &s, const char *t) { s.camera_speed = variable_float(t, 190.0f, 20.0f, 720.0f); }},
        {"input.aim_speed", "MHP3RD_AIM_SPEED",
         [](Settings &s, const std::string &t) { return parse_float(t, 10.0f, 360.0f, s.aim_speed); },
         [](const Settings &s) { return format_float(s.aim_speed); },
         [](Settings &s, const char *t) { s.aim_speed = variable_float(t, 90.0f, 10.0f, 360.0f); }},
        BOOL_FIELD("input.invert_camera_x", invert_camera_x),
        BOOL_FIELD("input.invert_camera_y", invert_camera_y),
        {"input.mouse", "MHP3RD_MOUSE",
         [](Settings &s, const std::string &t) { return parse_bool(t, s.mouse); },
         [](const Settings &s) { return std::string(s.mouse ? "1" : "0"); },
         [](Settings &s, const char *t) { s.mouse = variable_flag(t); }},
        {"input.mouse_sensitivity", "MHP3RD_MOUSE_SENSITIVITY",
         [](Settings &s, const std::string &t) {
             return parse_float(t, kMinMouseSensitivity, kMaxMouseSensitivity, s.mouse_sensitivity);
         },
         [](const Settings &s) { return format_float(s.mouse_sensitivity); },
         [](Settings &s, const char *t) {
             s.mouse_sensitivity = variable_float(t, 0.10f, kMinMouseSensitivity, kMaxMouseSensitivity);
         }},
        BOOL_FIELD("input.touch_controls", touch_controls),
        BOOL_FIELD("input.touch_dpad", touch_dpad),
        {"input.touch_opacity", nullptr,
         [](Settings &s, const std::string &t) {
             return parse_float(t, kMinTouchOpacity, kMaxTouchOpacity, s.touch_opacity);
         },
         [](const Settings &s) { return format_float(s.touch_opacity); }, nullptr},
        {"input.touch_size", nullptr,
         [](Settings &s, const std::string &t) { return parse_float(t, kMinTouchSize, kMaxTouchSize, s.touch_size); },
         [](const Settings &s) { return format_float(s.touch_size); }, nullptr},
        {"input.touch_camera_speed", nullptr,
         [](Settings &s, const std::string &t) {
             return parse_float(t, kMinTouchCameraSpeed, kMaxTouchCameraSpeed, s.touch_camera_speed);
         },
         [](const Settings &s) { return format_float(s.touch_camera_speed); }, nullptr},
        BOOL_FIELD("input.invert_mouse_x", invert_mouse_x),
        BOOL_FIELD("input.invert_mouse_y", invert_mouse_y),
        {"input.name_entry", "MHP3RD_OSK_MODE",
         [](Settings &s, const std::string &t) { return kNameEntries.parse(t, s.name_entry); },
         [](const Settings &s) { return kNameEntries.format(s.name_entry); },
         [](Settings &s, const char *t) {
             if (!kNameEntries.parse(t, s.name_entry)) std::cerr << "[settings] MHP3RD_OSK_MODE: keyboard or fixed\n";
         }},
        {"input.name", "MHP3RD_OSK_TEXT",
         [](Settings &s, const std::string &t) {
             if (t.empty()) return false;
             s.name = t;
             return true;
         },
         [](const Settings &s) { return s.name; }, [](Settings &s, const char *t) { s.name = t; }},
        {"network.adhoc", "MHP3RD_ADHOC",
         [](Settings &s, const std::string &t) { return parse_bool(t, s.adhoc); },
         [](const Settings &s) { return std::string(s.adhoc ? "1" : "0"); },
         [](Settings &s, const char *t) { s.adhoc = variable_flag(t); }},
        {"network.server", "MHP3RD_ADHOC_SERVER",
         [](Settings &s, const std::string &t) {
             s.adhoc_server = t;
             return true;
         },
         [](const Settings &s) { return s.adhoc_server; }, [](Settings &s, const char *t) { s.adhoc_server = t; }},
        {"network.nickname", "MHP3RD_ADHOC_NICKNAME",
         [](Settings &s, const std::string &t) {
             s.adhoc_nickname = t;
             return true;
         },
         [](const Settings &s) { return s.adhoc_nickname; },
         [](Settings &s, const char *t) { s.adhoc_nickname = t; }},
        {"network.mac", "MHP3RD_ADHOC_MAC",
         [](Settings &s, const std::string &t) {
             s.adhoc_mac = t;
             return true;
         },
         [](const Settings &s) { return s.adhoc_mac; }, [](Settings &s, const char *t) { s.adhoc_mac = t; }},
        {"ui.menu_pause", "MHP3RD_MENU_PAUSE",
         [](Settings &s, const std::string &t) { return parse_bool(t, s.menu_pause); },
         [](const Settings &s) { return std::string(s.menu_pause ? "1" : "0"); },
         [](Settings &s, const char *t) { s.menu_pause = variable_flag(t); }},
        {"ui.menu_pause_multiplayer", "MHP3RD_MENU_PAUSE_MULTIPLAYER",
         [](Settings &s, const std::string &t) { return parse_bool(t, s.menu_pause_multiplayer); },
         [](const Settings &s) { return std::string(s.menu_pause_multiplayer ? "1" : "0"); },
         [](Settings &s, const char *t) { s.menu_pause_multiplayer = variable_flag(t); }},
        {"network.recent", nullptr,
         [](Settings &s, const std::string &t) {
             s.adhoc_recent.clear();
             std::size_t start = 0;
             while (start <= t.size()) {
                 const std::size_t end = std::min(t.find(',', start), t.size());
                 if (end > start) s.adhoc_recent.push_back(t.substr(start, end - start));
                 start = end + 1u;
             }
             return true;
         },
         [](const Settings &s) {
             std::string text;
             for (const std::string &address : s.adhoc_recent) text += (text.empty() ? "" : ",") + address;
             return text;
         },
         nullptr},
        {"network.host_port", "MHP3RD_ADHOC_HOST_PORT",
         [](Settings &s, const std::string &t) { return parse_uint(t, 1024u, 65534u, s.adhoc_host_port); },
         [](const Settings &s) { return std::to_string(s.adhoc_host_port); },
         [](Settings &s, const char *t) {
             std::uint32_t value = s.adhoc_host_port;
             if (parse_uint(t, 1024u, 65534u, value)) s.adhoc_host_port = value;
         }},
        BOOL_FIELD("ui.menu_hint_seen", menu_hint_seen),
        BOOL_FIELD("saves.backup_timestamp", backup_timestamp),
        {"ui.last_folder", nullptr,
         [](Settings &s, const std::string &t) {
             s.last_folder = t;
             return true;
         },
         [](const Settings &s) { return s.last_folder; }, nullptr},
    };
    return table;
}

// One key per bound action, "input.bind.triangle=Mouse Left", after the
// fixed table.
const std::vector<Field> &all_fields() {
    static const std::vector<Field> table = [] {
        // Field keys are C strings; these hold them for the program's life.
        static std::vector<std::string> keys(input::kActions);
        std::vector<Field> list = fields();
        for (std::size_t i = 0; i < input::kActions; ++i) {
            keys[i] = std::string("input.bind.") + input::info(static_cast<input::Action>(i)).key;
            list.push_back(Field{keys[i].c_str(), nullptr,
                                 [i](Settings &s, const std::string &t) { return input::parse(t, s.bindings[i]); },
                                 [i](const Settings &s) { return input::format(s.bindings[i]); }, nullptr});
        }
        return list;
    }();
    return table;
}

#undef BOOL_FIELD

struct State {
    bool loaded{};
    Settings values;
    std::filesystem::path data_dir;
    // What settings.ini held, so values the environment decided are written
    // back as the file had them.
    install::SettingsEntries file;
    std::map<std::string, const char *> overrides;
};

State &state() {
    static State value;
    return value;
}

void load(State &s) {
    s.loaded = true;
    s.values = defaults();
    try {
        s.data_dir = install::user_data_directory();
        s.file = install::read_settings_file(s.data_dir);
    } catch (const std::exception &e) {
        std::cerr << "[settings] cannot read settings.ini: " << e.what() << "\n";
    }
    for (const Field &field : all_fields()) {
        if (const auto found = s.file.find(field.key); found != s.file.end() && !field.parse(s.values, found->second))
            std::cerr << "[settings] ignoring " << field.key << "=" << found->second << "\n";
        if (field.variable == nullptr) continue;
        const char *text = std::getenv(field.variable);
        if (text == nullptr) continue;
        field.parse_variable(s.values, text);
        s.overrides[field.key] = field.variable;
    }
    // A fixed name in the environment is meant for unattended runs, which
    // nobody is there to type in, so it also answers at once unless
    // MHP3RD_OSK_MODE says otherwise.
    if (s.overrides.count("input.name_entry") == 0u && std::getenv("MHP3RD_OSK_TEXT") != nullptr) {
        s.values.name_entry = NameEntry::Fixed;
        s.overrides["input.name_entry"] = "MHP3RD_OSK_TEXT";
    }
}

} // namespace

Settings &current() {
    State &s = state();
    if (!s.loaded) load(s);
    return s.values;
}

Settings defaults_for(Platform platform) {
    Settings values{};
    if (platform == Platform::Android) {
        values.aspect = Aspect::Fill;
        values.fullscreen = true;
        values.mouse = false;
    }
    return values;
}

const Settings &defaults() {
    static const Settings value = defaults_for(kPlatform);
    return value;
}

void save() {
    State &s = state();
    if (!s.loaded) load(s);
    install::SettingsEntries entries;
    try {
        // Re-read, so a key the installer wrote since start-up survives.
        entries = install::read_settings_file(s.data_dir);
        for (const Field &field : all_fields()) {
            if (s.overrides.count(field.key) != 0u) {
                const auto kept = s.file.find(field.key);
                if (kept != s.file.end()) entries[field.key] = kept->second;
                else entries.erase(field.key);
                continue;
            }
            entries[field.key] = field.format(s.values);
        }
        entries.erase(kRetiredTypeNameKey);
        install::write_settings_file(s.data_dir, entries);
    } catch (const std::exception &e) {
        std::cerr << "[settings] cannot write settings.ini: " << e.what() << "\n";
    }
}

const char *overridden_by(const char *key) {
    State &s = state();
    if (!s.loaded) load(s);
    const auto found = s.overrides.find(key);
    return found != s.overrides.end() ? found->second : nullptr;
}

} // namespace mhp3rd::settings
