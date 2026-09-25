#pragma once

#include "input/bindings.hpp"

#include <cstdint>
#include <string>
#include <vector>

// The player's settings: what the in-game menu changes and settings.ini in the
// per-user data directory keeps.
//
// A value comes from its environment variable when that is set, otherwise from
// settings.ini, otherwise from the default below. A variable decides the value
// for the whole run: the menu shows it but cannot change it, and it is never
// written to settings.ini, so unsetting the variable brings the player's own
// choice back.
//
// Only the main thread reads or writes these.
namespace mhp3rd::settings {

enum class PresentMode { Fifo, Mailbox, Immediate };
// How the game's picture meets the window. Original keeps the PSP's shape with
// black bars, Stretch fills the window by stretching it, and Fill gives the
// game the window's shape: its view widens or narrows to match (the vertical
// field of view stays), and the 2D interface keeps the PSP's proportions.
enum class Aspect { Original, Stretch, Fill };
enum class PerfDisplay { Off, Overlay, OverlayAndLog, Log };
enum class RightStick { Camera, DPad, Off };
// What LT/RT (L2/R2) press past the trigger point. Standard makes them L and
// R like the shoulders; the other two move R onto L2 and put a weapon's attack
// on R2 for shooting: triangle for a bow, circle for a bowgun. The buttons
// they copy keep working.
enum class TriggerProfile { Standard, Bows, Bowguns };
// What answers the game when it asks for text such as the hunter's name.
enum class NameEntry { Keyboard, Fixed };
// Presents per second. The game makes 30 frames a second; the faster rates
// add frames in between with blended movement (frame interpolation), and
// Display follows the display's refresh rate. 30 presents the game's frames
// as they are, as before interpolation existed.
enum class FrameRate { Fps30, Fps45, Fps60, Fps90, Fps120, Display };

struct Settings {
    // Video
    std::uint32_t internal_scale{2u};  // render resolution, multiples of 480x272 (272 lines each); 0: the window's
    std::uint32_t window_scale{2u};    // windowed size, multiples of 480x272
    bool fullscreen{};
    PresentMode present_mode{PresentMode::Fifo};
    Aspect aspect{Aspect::Original};
    bool sharp_screen{};               // nearest instead of linear scaling to the window
    bool sharp_textures{};             // nearest instead of linear texture sampling
    bool texture_pack{true};           // draw an installed HD texture pack's images instead of the game's
    std::string texture_pack_folder;   // a pack used where it is instead of textures/<disc id>; empty: none
    bool unthrottled{};                // let emulated time run ahead of real time
    bool fast_loading{true};           // ...but only while the game loads (kernel/fast_loading.hpp)
    FrameRate frame_rate{FrameRate::Fps30};
    bool frame_rate_auto{true};        // lower the frame rate rather than slow the game
    PerfDisplay perf{PerfDisplay::Off};

    // Text
    std::string font;                  // the game's text font: path, "#face" for a collection; empty: the default
    std::uint32_t font_weight{1u};     // columns the game's glyphs are thickened by, 0 to kMaxFontWeight

    // Audio
    std::uint32_t volume{100u};        // percent
    bool mute{};

    // Controls
    bool confirm_south{};              // confirm (circle) on the south face button
    float dead_zone{0.15f};
    float trigger{0.25f};
    TriggerProfile trigger_profile{TriggerProfile::Standard};
    RightStick right_stick{RightStick::Camera};
    float right_stick_zone{0.5f};
    // Drives the ordinary quest camera's yaw and pitch from how far the stick
    // is pushed, instead of the game's fixed-speed turn and vertical presets.
    // On by default. Off writes nothing at all, so the camera is exactly as
    // the game made it.
    bool analog_camera{true};
    // Degrees per second at full deflection, before the stick's own curve.
    float camera_speed{190.0f};
    // Degrees per second at full deflection while a bow or a bowgun aims.
    float aim_speed{90.0f};
    bool invert_camera_x{};
    bool invert_camera_y{};
    // Keyboard and mouse. With the mouse on, the window captures the pointer
    // while the game runs and the mouse turns the camera; Esc frees it.
    bool mouse{true};
    float mouse_sensitivity{0.10f};    // degrees of camera turn per count of mouse motion
    bool invert_mouse_x{};
    bool invert_mouse_y{};
    input::Bindings bindings{input::default_bindings()};
    // On-screen controls for a touch screen, shown once the screen is touched
    // and hidden again when a gamepad or the keyboard is used.
    bool touch_controls{true};
    bool touch_dpad{true};             // the D-pad among them, for the game's menus
    float touch_opacity{0.5f};         // 0.1-1
    float touch_size{1.0f};            // 0.6-1.6 of the default size
    float touch_camera_speed{180.0f};  // degrees the camera turns for a drag across the screen's height
    NameEntry name_entry{NameEntry::Keyboard};  // on-screen keyboard, or the name below at once
    std::string name{"Hunter"};        // the fixed name

    // Network (ad hoc play through a PSP ad hoc server)
    bool adhoc{};                      // wireless switch on: the game may go on line
    std::string adhoc_server;          // host or host:port of the server; empty: none
    std::string adhoc_nickname;        // shown to other players; empty: the hunter name
    std::string adhoc_mac;             // this player's virtual MAC, made up on first use
    std::vector<std::string> adhoc_recent;  // sessions joined lately, the latest first
    std::uint32_t adhoc_host_port{27312};   // the built-in server's adhocctl port; the relay is on the next

    // Interface
    bool menu_pause{true};             // opening the menu pauses the game
    bool menu_pause_multiplayer{};     // ...also during ad hoc play, where a paused game stops answering its peers
    bool menu_hint_seen{};             // the "Esc / L3+R3 opens the menu" hint was shown
    std::string last_folder;           // where the setup's file browser was last used

    // Saves
    bool backup_timestamp{true};       // a backup made from the menu goes to a new folder named by its time
};

inline constexpr std::uint32_t kMaxInternalScale = 8u;
inline constexpr std::uint32_t kMaxWindowScale = 4u;
inline constexpr std::uint32_t kMaxFontWeight = 2u;
inline constexpr float kMinMouseSensitivity = 0.01f;
inline constexpr float kMinTouchOpacity = 0.1f;
inline constexpr float kMaxTouchOpacity = 1.0f;
inline constexpr float kMinTouchSize = 0.6f;
inline constexpr float kMaxTouchSize = 1.6f;
inline constexpr float kMinTouchCameraSpeed = 30.0f;
inline constexpr float kMaxTouchCameraSpeed = 720.0f;
inline constexpr float kMaxMouseSensitivity = 0.99f;

// The platforms whose defaults differ. A phone plays full screen with a
// finger or a pad, so a few settings start otherwise there (defaults_for).
enum class Platform { Desktop, Android };
#if defined(__ANDROID__)
inline constexpr Platform kPlatform = Platform::Android;
#else
inline constexpr Platform kPlatform = Platform::Desktop;
#endif
// The defaults on `platform`: Desktop is Settings{} as declared above;
// Android differs in video.aspect (fill: a phone is wider than the PSP),
// video.fullscreen (on: there is no window) and input.mouse (off: a phone
// has no mouse to capture, and an emulator's pointer would turn the camera).
[[nodiscard]] Settings defaults_for(Platform platform);

// Loads the settings on first use, starting from defaults().
[[nodiscard]] Settings &current();
// This platform's defaults, for keys settings.ini lacks and for "Restore
// defaults".
[[nodiscard]] const Settings &defaults();
// Writes current() to settings.ini, leaving values set by environment
// variables at what the file had. Failures are reported on the console.
void save();

// The environment variable that decides the setting stored under `key`
// (for example "video.internal_scale") for this run, or null.
[[nodiscard]] const char *overridden_by(const char *key);

} // namespace mhp3rd::settings
