#pragma once

#include <cstddef>
#include <functional>
#include <optional>
#include <string>

// The on-screen keyboard: a grid of keys drawn by the interface layer, for
// entering text with a gamepad alone. A physical keyboard types into it at the
// same time, and the mouse can click its keys.
//
// Gamepad: the D-pad or left stick moves over the keys, confirm types the key,
// back deletes, the left face button is Shift (once, then Caps, then off), the
// top face button types a space, Select/View switches to the symbols page,
// L1/R1 move the text cursor and Start is OK. Confirm is the face button the
// menu's "Confirm button" setting names.
//
// Keyboard: typing, Backspace, Delete, the arrow keys, Home, End, Enter (OK)
// and Esc (cancel).
//
// It is used in two places. Inside a screen that is already running, such as
// the in-game menu: open it, then call text_input_frame() from the screen's
// frame in place of drawing the screen until it closes. Over the running game,
// for the game's own keyboard requests: open_game_text_input() takes the pad
// from the game and draws the keyboard with every presented game frame.
namespace mhp3rd::ui {

struct TextInputRequest {
    std::string title;           // what is being entered, e.g. "Nickname"
    std::string prompt;          // a line under the title; may be empty
    std::string initial;         // UTF-8
    std::size_t max_length{16};  // in characters
    // The characters that may be entered; the others are shown dimmed and
    // cannot be typed. Null: printable ASCII.
    std::function<bool(char32_t)> allowed;
};

// Called once when the keyboard closes: the text (UTF-8) after OK, or nullopt
// after Cancel.
using TextInputDone = std::function<void(std::optional<std::string>)>;

// Opens the keyboard inside a running screen. A keyboard already open is
// closed first as cancelled.
void open_text_input(TextInputRequest request, TextInputDone on_done);
[[nodiscard]] bool text_input_open();
// Closes an open keyboard as cancelled.
void cancel_text_input();
// Builds this frame of the keyboard and handles its input. Call inside an
// interface frame (Layer::run, or begin_frame/end_frame).
void text_input_frame();

// Opens the keyboard over the running game. The game keeps running, as it
// does behind the PSP's keyboard, but reads a neutral pad, and the window
// keeps the frame that was on screen behind the keyboard. False when there is
// no interface to draw it in.
bool open_game_text_input(TextInputRequest request, TextInputDone on_done);

// Character sets.
[[nodiscard]] bool printable_ascii(char32_t c);
// What a hunter name may hold: letters, digits, space and the punctuation
// the game draws faithfully and that has no special meaning in text.
[[nodiscard]] bool hunter_name_character(char32_t c);

} // namespace mhp3rd::ui
