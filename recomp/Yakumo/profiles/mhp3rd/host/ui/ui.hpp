#pragma once

#include <memory>
#include <string>

// The port's own interface, drawn with Dear ImGui over the game's window: the
// in-game menu and the first-run setup screens. Only builds with the renderer
// have it.
namespace mhp3rd::gpu {
class VulkanRenderer;
}
namespace mhp3rd::install {
class InstallerUi;
}

namespace mhp3rd::ui {

// Puts the interface on the renderer's window. False when it cannot start;
// the game then runs without a menu.
bool attach(gpu::VulkanRenderer &renderer);

// Before each game frame is presented: draws what the interface shows over
// the running game (the hint that says how to open the menu, the network
// overlay, and the menu when it is open over the running game).
void draw_over_game();

// After a game frame's window events: whether the player asked for the menu
// (Esc, or L3+R3 on a gamepad).
[[nodiscard]] bool menu_requested();

// Whether the menu, opened now, pauses the game. Settings decide: "Pause the
// game when the menu opens", and during ad hoc play "Pause during
// multiplayer", off by default because a paused game stops answering its
// peers.
[[nodiscard]] bool menu_pauses();

// Runs the menu over the last game frame until the player closes it. The
// caller pauses the game around it. False: the player chose to quit.
bool run_menu();

// Opens the menu over the running game instead: draw_over_game() then draws
// it with every game frame, and the game gets no input until it closes.
void open_menu_over_game();
[[nodiscard]] bool menu_over_game();
// Once, after the player chose to quit in a menu over the running game.
[[nodiscard]] bool take_quit_request();

// The setup screens as an installer front end, or null without a window.
std::unique_ptr<install::InstallerUi> make_setup_screens();

// Shows a problem that keeps the game from starting. With ask_setup, offers
// to run the setup again. Unavailable when there is no window to show it in.
enum class ProblemAnswer { Unavailable, Quit, SetUpAgain };
ProblemAnswer show_problem(const std::string &title, const std::string &message, bool ask_setup);

} // namespace mhp3rd::ui
