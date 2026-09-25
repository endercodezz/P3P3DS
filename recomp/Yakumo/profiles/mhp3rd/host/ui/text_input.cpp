#include "ui/text_input.hpp"

#include "ui/layer.hpp"
#include "ui/widgets.hpp"

#include "gpu/vulkan_renderer.hpp"

#include "imgui.h"

#include <SDL3/SDL.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstring>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

namespace mhp3rd::ui {
namespace {

using Clock = std::chrono::steady_clock;

// How long the counter stays red after a key that could not be typed.
constexpr auto kRefusedTime = std::chrono::milliseconds(600);

enum class KeyKind { Char, Shift, Symbols, Space, Delete, SteamKeyboard, Cancel, Ok };

struct Key {
    KeyKind kind{KeyKind::Char};
    char32_t lower{};
    char32_t upper{};
    float units{1.0f};
};

using Row = std::vector<Key>;

Row char_row(const char *lower, const char *upper) {
    Row row;
    for (std::size_t i = 0; lower[i] != '\0'; ++i)
        row.push_back({KeyKind::Char, static_cast<unsigned char>(lower[i]), static_cast<unsigned char>(upper[i])});
    return row;
}

// The letters page, shifted and not, and the symbols page. Both keep the
// digits on top so a key stays where it was when the page changes.
const std::array<Row, 4> &letter_rows() {
    static const std::array<Row, 4> rows{
        char_row("1234567890", "1234567890"),
        char_row("qwertyuiop", "QWERTYUIOP"),
        char_row("asdfghjkl'", "ASDFGHJKL'"),
        char_row("zxcvbnm,.-", "ZXCVBNM,.-"),
    };
    return rows;
}

const std::array<Row, 4> &symbol_rows() {
    static const std::array<Row, 4> rows{
        char_row("1234567890", "1234567890"),
        char_row("!?&'\"()-_~", "!?&'\"()-_~"),
        char_row("#$%@=+*/\\|", "#$%@=+*/\\|"),
        char_row(":;^`<>[]{}", ":;^`<>[]{}"),
    };
    return rows;
}

std::u32string decode_utf8(const std::string &text) {
    std::u32string out;
    for (std::size_t i = 0; i < text.size();) {
        const auto byte = static_cast<unsigned char>(text[i]);
        int extra = 0;
        char32_t c = byte;
        if (byte >= 0xF0u) {
            extra = 3;
            c = byte & 0x07u;
        } else if (byte >= 0xE0u) {
            extra = 2;
            c = byte & 0x0Fu;
        } else if (byte >= 0xC0u) {
            extra = 1;
            c = byte & 0x1Fu;
        }
        ++i;
        for (int k = 0; k < extra && i < text.size(); ++k, ++i) c = (c << 6u) | (static_cast<unsigned char>(text[i]) & 0x3Fu);
        out.push_back(c);
    }
    return out;
}

std::string encode_utf8(const std::u32string &text) {
    std::string out;
    for (const char32_t c : text) {
        if (c < 0x80u) {
            out.push_back(static_cast<char>(c));
        } else if (c < 0x800u) {
            out.push_back(static_cast<char>(0xC0u | (c >> 6u)));
            out.push_back(static_cast<char>(0x80u | (c & 0x3Fu)));
        } else if (c < 0x10000u) {
            out.push_back(static_cast<char>(0xE0u | (c >> 12u)));
            out.push_back(static_cast<char>(0x80u | ((c >> 6u) & 0x3Fu)));
            out.push_back(static_cast<char>(0x80u | (c & 0x3Fu)));
        } else {
            out.push_back(static_cast<char>(0xF0u | (c >> 18u)));
            out.push_back(static_cast<char>(0x80u | ((c >> 12u) & 0x3Fu)));
            out.push_back(static_cast<char>(0x80u | ((c >> 6u) & 0x3Fu)));
            out.push_back(static_cast<char>(0x80u | (c & 0x3Fu)));
        }
    }
    return out;
}

enum class ShiftState { Off, Once, Locked };

struct Keyboard {
    bool open{};
    bool over_game{};
    TextInputRequest request;
    TextInputDone on_done;
    std::u32string text;
    std::size_t cursor{};
    ShiftState shift{ShiftState::Off};
    bool symbols{};
    int row{1};
    int column{0};
    Clock::time_point refused{};
    Clock::time_point opened{};
    bool steam_keyboard{};
    std::string saved_screen_keyboard_hint;
};

Keyboard &keyboard() {
    static Keyboard value;
    return value;
}

float font() { return Layer::get().font_size(); }
float px(float value) { return std::round(value * Layer::get().scale()); }

bool allowed(char32_t c) {
    const Keyboard &k = keyboard();
    return k.request.allowed ? k.request.allowed(c) : printable_ascii(c);
}

bool steam_keyboard_available() { return SDL_HasScreenKeyboardSupport(); }

// The whole grid for the current page: four rows of characters and a row of
// actions.
std::vector<Row> grid() {
    const Keyboard &k = keyboard();
    const auto &page = k.symbols ? symbol_rows() : letter_rows();
    std::vector<Row> rows(page.begin(), page.end());
    Row actions{{KeyKind::Shift, 0, 0, 1.5f}, {KeyKind::Symbols, 0, 0, 1.5f}, {KeyKind::Space, 0, 0, 2.5f},
                {KeyKind::Delete, 0, 0, 1.5f}};
    if (steam_keyboard_available()) actions.push_back({KeyKind::SteamKeyboard, 0, 0, 1.5f});
    actions.push_back({KeyKind::Cancel, 0, 0, 1.5f});
    actions.push_back({KeyKind::Ok, 0, 0, 1.5f});
    rows.push_back(std::move(actions));
    return rows;
}

char32_t key_char(const Key &key) {
    return keyboard().shift != ShiftState::Off ? key.upper : key.lower;
}

// Starts SDL text input for typed characters, without letting SDL bring up
// a system keyboard over ours unless the player asked for Steam's.
void start_system_text_input(bool show_screen_keyboard) {
    Keyboard &k = keyboard();
    SDL_Window *window = Layer::get().renderer().window();
    if (window == nullptr) return;
    if (SDL_TextInputActive(window)) SDL_StopTextInput(window);
    SDL_SetHint(SDL_HINT_ENABLE_SCREEN_KEYBOARD, show_screen_keyboard ? "1" : "0");
    SDL_StartTextInput(window);
    k.steam_keyboard = show_screen_keyboard;
}

void stop_system_text_input() {
    Keyboard &k = keyboard();
    SDL_Window *window = Layer::get().renderer().window();
    if (window != nullptr && SDL_TextInputActive(window)) SDL_StopTextInput(window);
    SDL_SetHint(SDL_HINT_ENABLE_SCREEN_KEYBOARD,
                k.saved_screen_keyboard_hint.empty() ? nullptr : k.saved_screen_keyboard_hint.c_str());
    k.steam_keyboard = false;
}

void begin(TextInputRequest request, TextInputDone on_done, bool over_game) {
    if (keyboard().open) cancel_text_input();
    Keyboard &k = keyboard();
    k = Keyboard{};
    k.open = true;
    k.over_game = over_game;
    k.request = std::move(request);
    k.on_done = std::move(on_done);
    for (const char32_t c : decode_utf8(k.request.initial))
        if (k.text.size() < k.request.max_length && allowed(c)) k.text.push_back(c);
    k.cursor = k.text.size();
    k.opened = Clock::now();
    if (const char *hint = SDL_GetHint(SDL_HINT_ENABLE_SCREEN_KEYBOARD)) k.saved_screen_keyboard_hint = hint;
    start_system_text_input(false);
}

void finish(bool confirmed) {
    Keyboard &k = keyboard();
    if (!k.open) return;
    k.open = false;
    stop_system_text_input();
    if (k.over_game) {
        Layer &layer = Layer::get();
        layer.set_interactive(false);
        layer.renderer().set_game_input(true);
        layer.renderer().hold_frame(false);
    }
    std::optional<std::string> result;
    if (confirmed) result = encode_utf8(k.text);
    TextInputDone done = std::move(k.on_done);
    k.on_done = nullptr;
    if (done) done(std::move(result));
}

void type(char32_t c) {
    Keyboard &k = keyboard();
    if (!allowed(c) || k.text.size() >= k.request.max_length) {
        k.refused = Clock::now();
        return;
    }
    k.text.insert(k.text.begin() + static_cast<std::ptrdiff_t>(k.cursor), c);
    ++k.cursor;
    if (k.shift == ShiftState::Once) k.shift = ShiftState::Off;
}

void erase_before_cursor() {
    Keyboard &k = keyboard();
    if (k.cursor == 0u) return;
    k.text.erase(k.cursor - 1u, 1u);
    --k.cursor;
}

void move_cursor(int delta) {
    Keyboard &k = keyboard();
    const auto moved = static_cast<long>(k.cursor) + delta;
    k.cursor = static_cast<std::size_t>(std::clamp<long>(moved, 0, static_cast<long>(k.text.size())));
}

void cycle_shift() {
    Keyboard &k = keyboard();
    k.shift = k.shift == ShiftState::Off ? ShiftState::Once
              : k.shift == ShiftState::Once ? ShiftState::Locked
                                            : ShiftState::Off;
}

void press(const Key &key) {
    Keyboard &k = keyboard();
    switch (key.kind) {
    case KeyKind::Char: type(key_char(key)); break;
    case KeyKind::Shift: cycle_shift(); break;
    case KeyKind::Symbols: k.symbols = !k.symbols; break;
    case KeyKind::Space: type(U' '); break;
    case KeyKind::Delete: erase_before_cursor(); break;
    case KeyKind::SteamKeyboard: start_system_text_input(true); break;
    case KeyKind::Cancel: finish(false); break;
    case KeyKind::Ok: finish(true); break;
    }
}

// Left edge and width of every key of a row, in units of the widest row.
std::vector<std::pair<float, float>> spans(const Row &row) {
    float total = 0.0f;
    for (const Key &key : row) total += key.units;
    std::vector<std::pair<float, float>> out;
    float x = 0.0f;
    for (const Key &key : row) {
        const float width = key.units * 10.0f / total;
        out.emplace_back(x, width);
        x += width;
    }
    return out;
}

// Moves the selection, keeping the column under the same spot when the row
// changes between rows of different keys.
void move_selection(int dx, int dy) {
    Keyboard &k = keyboard();
    const std::vector<Row> rows = grid();
    const int row_count = static_cast<int>(rows.size());
    k.row = std::clamp(k.row, 0, row_count - 1);
    k.column = std::clamp(k.column, 0, static_cast<int>(rows[static_cast<std::size_t>(k.row)].size()) - 1);
    if (dx != 0) {
        const int count = static_cast<int>(rows[static_cast<std::size_t>(k.row)].size());
        k.column = ((k.column + dx) % count + count) % count;
    }
    if (dy != 0) {
        const auto from = spans(rows[static_cast<std::size_t>(k.row)])[static_cast<std::size_t>(k.column)];
        const float centre = from.first + from.second * 0.5f;
        k.row = ((k.row + dy) % row_count + row_count) % row_count;
        const auto to = spans(rows[static_cast<std::size_t>(k.row)]);
        k.column = static_cast<int>(to.size()) - 1;
        for (std::size_t i = 0; i < to.size(); ++i) {
            if (centre < to[i].first + to[i].second) {
                k.column = static_cast<int>(i);
                break;
            }
        }
    }
}

const char *action_label(KeyKind kind) {
    const Keyboard &k = keyboard();
    switch (kind) {
    case KeyKind::Shift: return k.shift == ShiftState::Locked ? "CAPS" : "Shift";
    case KeyKind::Symbols: return k.symbols ? "abc" : "#+=";
    case KeyKind::Space: return "Space";
    case KeyKind::Delete: return "Delete";
    case KeyKind::SteamKeyboard: return "Steam";
    case KeyKind::Cancel: return "Cancel";
    case KeyKind::Ok: return "OK";
    case KeyKind::Char: break;
    }
    return "";
}

void handle_keys(const std::vector<Row> &rows) {
    Keyboard &k = keyboard();
    Layer &layer = Layer::get();
    ImGuiIO &io = ImGui::GetIO();

    // Typed text: from a physical keyboard, Steam's keyboard or a script.
    for (const ImWchar c : io.InputQueueCharacters) {
        if (c >= 0x20 && c != 0x7F) type(static_cast<char32_t>(c));
    }
    if (ImGui::IsKeyPressed(ImGuiKey_Backspace, true)) erase_before_cursor();
    if (ImGui::IsKeyPressed(ImGuiKey_Delete, true) && k.cursor < k.text.size()) k.text.erase(k.cursor, 1u);
    if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow, true)) move_cursor(-1);
    if (ImGui::IsKeyPressed(ImGuiKey_RightArrow, true)) move_cursor(1);
    if (ImGui::IsKeyPressed(ImGuiKey_Home, false)) k.cursor = 0u;
    if (ImGui::IsKeyPressed(ImGuiKey_End, false)) k.cursor = k.text.size();
    if (ImGui::IsKeyPressed(ImGuiKey_Enter, false) || ImGui::IsKeyPressed(ImGuiKey_KeypadEnter, false)) {
        finish(true);
        return;
    }
    if (layer.take_back()) {
        finish(false);
        return;
    }

    // A button held when the keyboard opened, such as the one that asked
    // for it, does nothing until it is released.
    if (!layer.gamepad_armed()) return;
    const bool south = layer.confirm_south();
    const ImGuiKey confirm = south ? ImGuiKey_GamepadFaceDown : ImGuiKey_GamepadFaceRight;
    const ImGuiKey back = south ? ImGuiKey_GamepadFaceRight : ImGuiKey_GamepadFaceDown;
    const auto pressed = [](std::initializer_list<ImGuiKey> keys) {
        for (const ImGuiKey key : keys)
            if (ImGui::IsKeyPressed(key, true)) return true;
        return false;
    };
    if (pressed({ImGuiKey_GamepadDpadLeft, ImGuiKey_GamepadLStickLeft})) move_selection(-1, 0);
    if (pressed({ImGuiKey_GamepadDpadRight, ImGuiKey_GamepadLStickRight})) move_selection(1, 0);
    if (pressed({ImGuiKey_GamepadDpadUp, ImGuiKey_GamepadLStickUp})) move_selection(0, -1);
    if (pressed({ImGuiKey_GamepadDpadDown, ImGuiKey_GamepadLStickDown})) move_selection(0, 1);
    if (ImGui::IsKeyPressed(ImGuiKey_GamepadL1, true)) move_cursor(-1);
    if (ImGui::IsKeyPressed(ImGuiKey_GamepadR1, true)) move_cursor(1);
    if (ImGui::IsKeyPressed(back, true)) erase_before_cursor();
    if (ImGui::IsKeyPressed(ImGuiKey_GamepadFaceLeft, false)) cycle_shift();
    if (ImGui::IsKeyPressed(ImGuiKey_GamepadFaceUp, true)) type(U' ');
    if (ImGui::IsKeyPressed(ImGuiKey_GamepadBack, false)) k.symbols = !k.symbols;
    if (ImGui::IsKeyPressed(ImGuiKey_GamepadStart, false)) {
        finish(true);
        return;
    }
    if (ImGui::IsKeyPressed(confirm, false)) {
        const Row &row = rows[static_cast<std::size_t>(std::clamp(k.row, 0, static_cast<int>(rows.size()) - 1))];
        press(row[static_cast<std::size_t>(std::clamp(k.column, 0, static_cast<int>(row.size()) - 1))]);
    }
}

void draw_field(ImDrawList *draw, float width) {
    Keyboard &k = keyboard();
    const float height = std::round(font() * 2.3f);
    const ImVec2 min = ImGui::GetCursorScreenPos();
    const ImVec2 max{min.x + width, min.y + height};
    ImGui::Dummy({width, height});
    draw->AddRectFilled(min, max, IM_COL32(8, 5, 3, 150), px(6.0f));
    draw->AddRect(min, max, colors::kAccent, px(6.0f), 0, px(1.5f));

    // The counter on the right, red while the text is full or a key was
    // refused a moment ago.
    char counter[32];
    std::snprintf(counter, sizeof(counter), "%zu / %zu", k.text.size(), k.request.max_length);
    const bool refused = Clock::now() - k.refused < kRefusedTime;
    const bool full = k.text.size() >= k.request.max_length;
    const ImVec2 counter_size = ImGui::CalcTextSize(counter);
    const float pad = px(14.0f);
    draw->AddText({max.x - pad - counter_size.x, min.y + (height - counter_size.y) * 0.5f},
                  refused ? colors::kDanger : full ? colors::kAccent : colors::kTextDim, counter);

    const float scale = 1.25f;
    ImGui::PushFont(nullptr, ImGui::GetStyle().FontSizeBase * scale);
    const std::string before = encode_utf8(k.text.substr(0, k.cursor));
    const std::string all = encode_utf8(k.text);
    const float text_height = ImGui::GetFontSize();
    const ImVec2 at{min.x + pad, min.y + (height - text_height) * 0.5f};
    draw->PushClipRect(min, {max.x - pad * 2.0f - counter_size.x, max.y}, true);
    draw->AddText(at, colors::kAccentBright, all.c_str());
    const float caret_x = at.x + ImGui::CalcTextSize(before.c_str()).x;
    const double seconds = std::chrono::duration<double>(Clock::now() - k.opened).count();
    if (std::fmod(seconds, 1.0) < 0.6)
        draw->AddRectFilled({caret_x, at.y}, {caret_x + px(2.0f), at.y + text_height}, colors::kAccent);
    draw->PopClipRect();
    ImGui::PopFont();
}

void draw_grid(ImDrawList *draw, const std::vector<Row> &rows, float width) {
    Keyboard &k = keyboard();
    const float key_height = std::round(font() * 2.0f);
    const float gap = px(6.0f);
    for (std::size_t r = 0; r < rows.size(); ++r) {
        const Row &row = rows[r];
        const auto layout = spans(row);
        const ImVec2 origin = ImGui::GetCursorScreenPos();
        for (std::size_t c = 0; c < row.size(); ++c) {
            const Key &key = row[c];
            const float unit = (width + gap) / 10.0f;
            const ImVec2 min{origin.x + layout[c].first * unit, origin.y};
            const ImVec2 max{min.x + layout[c].second * unit - gap, min.y + key_height};
            ImGui::SetCursorScreenPos(min);
            ImGui::PushID(static_cast<int>(r * 100u + c));
            const bool clicked = ImGui::InvisibleButton("##key", {max.x - min.x, max.y - min.y});
            const bool hovered = ImGui::IsItemHovered();
            ImGui::PopID();
            const bool selected = static_cast<int>(r) == k.row && static_cast<int>(c) == k.column;

            std::string label;
            bool enabled = true;
            if (key.kind == KeyKind::Char) {
                const char32_t ch = key_char(key);
                label = encode_utf8(std::u32string(1, ch));
                enabled = allowed(ch);
            } else {
                label = action_label(key.kind);
                if (key.kind == KeyKind::Space) enabled = allowed(U' ');
            }
            const bool lit = (key.kind == KeyKind::Shift && k.shift != ShiftState::Off) ||
                             (key.kind == KeyKind::Symbols && k.symbols) ||
                             (key.kind == KeyKind::SteamKeyboard && k.steam_keyboard) || key.kind == KeyKind::Ok;
            ImU32 fill = key.kind == KeyKind::Char ? colors::kRow : colors::kTrack;
            ImU32 ink = enabled ? colors::kText : colors::kTextDisabled;
            if (lit) {
                fill = colors::kAccent;
                ink = colors::kPanel;
            }
            if (hovered && !selected) fill = lit ? colors::kAccentBright : colors::kRowHover;
            const float rounding = px(6.0f);
            draw->AddRectFilled(min, max, fill, rounding);
            if (selected) {
                if (!lit) draw->AddRectFilled(min, max, colors::kRowFocus, rounding);
                draw->AddRect({min.x - px(2.0f), min.y - px(2.0f)}, {max.x + px(2.0f), max.y + px(2.0f)},
                              colors::kAccentBright, rounding + px(2.0f), 0, px(2.0f));
            }
            const bool big = key.kind == KeyKind::Char;
            ImGui::PushFont(nullptr, ImGui::GetStyle().FontSizeBase * (big ? 1.2f : 0.9f));
            const ImVec2 size = ImGui::CalcTextSize(label.c_str());
            draw->AddText({min.x + (max.x - min.x - size.x) * 0.5f, min.y + (max.y - min.y - size.y) * 0.5f}, ink,
                          label.c_str());
            ImGui::PopFont();
            if (clicked) {
                k.row = static_cast<int>(r);
                k.column = static_cast<int>(c);
                press(key);
                if (!k.open) return;
            }
        }
        ImGui::SetCursorScreenPos({origin.x, origin.y + key_height + gap});
    }
    ImGui::Dummy({width, 0.0f});
}

} // namespace

bool printable_ascii(char32_t c) { return c >= 0x20u && c < 0x7Fu; }

bool hunter_name_character(char32_t c) {
    if ((c >= U'a' && c <= U'z') || (c >= U'A' && c <= U'Z') || (c >= U'0' && c <= U'9')) return true;
    // Every other printable ASCII character shows in the name, but '*' draws
    // as a bullet, and the brackets, quotes, backslash, caret, backquote,
    // percent and bar are left out as characters text formatting may claim.
    return c != 0u && c < 0x80u && std::strchr(" !#$&'()+,-./:;=?@_~", static_cast<int>(c)) != nullptr;
}

void open_text_input(TextInputRequest request, TextInputDone on_done) {
    begin(std::move(request), std::move(on_done), false);
}

bool text_input_open() { return keyboard().open; }

void cancel_text_input() { finish(false); }

bool open_game_text_input(TextInputRequest request, TextInputDone on_done) {
    Layer &layer = Layer::get();
    if (!layer.attached()) return false;
    // The frame on screen now stays behind the keyboard: the game blanks the
    // screen while the PSP's own keyboard would cover it.
    layer.renderer().hold_frame(true);
    layer.renderer().set_game_input(false);
    layer.set_interactive(true);
    begin(std::move(request), std::move(on_done), true);
    return true;
}

void text_input_frame() {
    Keyboard &k = keyboard();
    if (!k.open) return;
    Layer &layer = Layer::get();
    // The menu does not open over the keyboard.
    (void)layer.take_menu_toggle();

    const std::vector<Row> rows = grid();
    handle_keys(rows);
    if (!k.open) return;

    const ImGuiIO &io = ImGui::GetIO();
    ImGui::GetBackgroundDrawList()->AddRectFilled({0, 0}, io.DisplaySize, colors::kBackdrop);
    const float margin = std::round(std::min(io.DisplaySize.x, io.DisplaySize.y) * 0.03f);
    const float width = std::min(io.DisplaySize.x - 2.0f * margin, font() * 34.0f);
    ImGui::SetNextWindowPos({io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f}, ImGuiCond_Always, {0.5f, 0.5f});
    ImGui::SetNextWindowSize({width, 0.0f}, ImGuiCond_Always);
    ImGui::Begin("##keyboard", nullptr,
                 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings |
                     ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoScrollWithMouse);
    ImDrawList *draw = ImGui::GetWindowDrawList();
    const float inner = ImGui::GetContentRegionAvail().x;

    ImGui::PushFont(nullptr, ImGui::GetStyle().FontSizeBase * 1.3f);
    ImGui::PushStyleColor(ImGuiCol_Text, colors::kAccentBright);
    ImGui::TextUnformatted(k.request.title.c_str());
    ImGui::PopStyleColor();
    ImGui::PopFont();
    const ImVec2 line = ImGui::GetCursorScreenPos();
    draw->AddRectFilledMultiColor(line, {line.x + inner, line.y + px(2.0f)}, colors::kAccent,
                                  IM_COL32(143, 93, 36, 0), IM_COL32(143, 93, 36, 0), colors::kAccent);
    ImGui::Dummy({0.0f, px(8.0f)});
    if (!k.request.prompt.empty()) {
        paragraph(k.request.prompt, colors::kTextDim);
        ImGui::Dummy({0.0f, px(4.0f)});
    }
    draw_field(draw, inner);
    ImGui::Dummy({0.0f, px(10.0f)});
    draw_grid(draw, rows, inner);
    if (!k.open) {
        ImGui::End();
        return;
    }

    draw->AddLine({line.x, ImGui::GetCursorScreenPos().y}, {line.x + inner, ImGui::GetCursorScreenPos().y},
                  IM_COL32(143, 93, 36, 90), px(1.0f));
    ImGui::Dummy({0.0f, px(8.0f)});
    if (layer.input_device() == InputDevice::Gamepad) {
        hints({{Control::Confirm, "Type"},
               {Control::Delete, "Delete"},
               {Control::Shift, "Shift"},
               {Control::Space, "Space"}});
        ImGui::Dummy({0.0f, px(4.0f)});
        hints({{Control::Cursor, "Cursor"}, {Control::Symbols, "Symbols"}, {Control::Start, "OK"}});
    } else {
        hints({{Control::Start, "OK"}, {Control::Back, "Cancel"}, {Control::Delete, "Delete"},
               {Control::Cursor, "Cursor"}});
    }
    ImGui::End();
}

} // namespace mhp3rd::ui
