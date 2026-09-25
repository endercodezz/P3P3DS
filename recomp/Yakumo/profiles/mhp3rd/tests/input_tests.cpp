// Keyboard and mouse bindings, without SDL or game data.
#include "input/bindings.hpp"

#include <cmath>
#include <cstdint>
#include <iostream>
#include <set>

namespace {
using namespace mhp3rd::input;
int failures{};

void check(bool condition, const char *message) {
    if (!condition) {
        ++failures;
        std::cerr << "FAIL: " << message << '\n';
    }
}

Slots slots_of(const Bindings &b, Action action) { return b[static_cast<std::size_t>(action)]; }

void test_names() {
    check(name(key(26)) == "W" && from_name("w") == key(26), "letters round-trip, ignoring case");
    check(name(key(225)) == "Left Shift" && from_name("left shift") == key(225), "modifiers have their names");
    check(name(key(40)) == "Enter" && from_name("Return") == key(40), "Enter also answers to SDL's Return");
    check(name(mouse_button(1)) == "Mouse Left" && from_name("Mouse Right") == mouse_button(3),
          "mouse buttons are named by their place");
    check(name(key(300)) == "Key 300" && from_name("Key 300") == key(300), "unnamed positions keep their number");
    check(from_name("Nonsense") == kNone && from_name("") == kNone, "unknown names are refused");
    for (std::uint16_t position = 1; position < kKeyPositions; ++position)
        if (from_name(name(key(position))) != key(position)) {
            check(false, "every key position round-trips through its name");
            break;
        }
}

void test_settings_spelling() {
    Slots slots{};
    check(parse("Mouse Right / F", slots) && slots[0] == mouse_button(3) && slots[1] == key(9),
          "two bindings separated by a slash");
    check(format(slots) == "Mouse Right / F", "and written back the same way");
    check(parse("/ / F", slots) && slots[0] == key(56) && slots[1] == key(9), "the slash key itself is a name");
    check(parse("F / /", slots) && slots[1] == key(56), "also as the second binding");
    check(parse("", slots) && slots[0] == kNone && slots[1] == kNone, "empty means unbound");
    check(format(slots).empty(), "and unbound is written empty");
    slots = {key(4), kNone};
    check(!parse("A / B / C", slots) && slots[0] == key(4), "three bindings are refused, leaving the old ones");
    check(!parse("Q / Nonsense", slots) && slots[0] == key(4), "an unknown name refuses the whole value");
    const Bindings &defaults = default_bindings();
    for (std::size_t i = 0; i < kActions; ++i) {
        Slots back{};
        check(parse(format(defaults[i]), back) && back == defaults[i], "every default round-trips");
    }
}

void test_layouts() {
    for (const Bindings *layout : {&default_bindings(), &classic_bindings()}) {
        std::set<Binding> seen;
        bool unique = true;
        for (const Slots &slots : *layout)
            for (const Binding binding : slots)
                if (binding != kNone && !seen.insert(binding).second) unique = false;
        check(unique, "no key does two things in a shipped layout");
    }
    const Bindings &d = default_bindings();
    check(slots_of(d, Action::StickUp)[0] == from_name("W") && slots_of(d, Action::StickLeft)[0] == from_name("A") &&
              slots_of(d, Action::StickDown)[0] == from_name("S") && slots_of(d, Action::StickRight)[0] == from_name("D"),
          "the default moves on W A S D");
    check(slots_of(d, Action::Triangle)[0] == mouse_button(1), "the left mouse button attacks");
    const Bindings &c = classic_bindings();
    check(slots_of(c, Action::StickUp)[0] == from_name("I") && slots_of(c, Action::Circle)[0] == from_name("X") &&
              slots_of(c, Action::Cross)[0] == from_name("Z") && slots_of(c, Action::R)[0] == from_name("W") &&
              slots_of(c, Action::Select)[0] == from_name("Right Shift") &&
              slots_of(c, Action::Select)[1] == from_name("Backspace"),
          "the classic layout is the one earlier versions had");
}

void test_assign() {
    Bindings b = default_bindings();
    assign(b, Action::Cross, key(9));  // F, which ○ has
    check(slots_of(b, Action::Cross)[0] == key(44) && slots_of(b, Action::Cross)[1] == key(9),
          "a new key fills the free slot");
    check(slots_of(b, Action::Circle)[0] == mouse_button(3) && slots_of(b, Action::Circle)[1] == kNone,
          "and leaves the control that had it");
    assign(b, Action::Cross, key(10));
    check(slots_of(b, Action::Cross)[0] == key(44) && slots_of(b, Action::Cross)[1] == key(10),
          "with both slots full the second is replaced");
    assign(b, Action::Cross, key(44));
    check(slots_of(b, Action::Cross)[0] == key(10) && slots_of(b, Action::Cross)[1] == kNone,
          "pressing a key the control has removes it");
    assign(b, Action::Cross, kNone);
    check(slots_of(b, Action::Cross)[0] == key(10), "nothing pressed changes nothing");
}

void test_read() {
    const Bindings &d = default_bindings();
    std::set<Binding> held;
    const auto read_held = [&] { return read(d, [&](Binding b) { return held.count(b) != 0u; }); };
    check(read_held().buttons == 0u && read_held().stick_x == 0 && read_held().camera_x == 0, "nothing held, nothing pressed");
    held = {from_name("W"), from_name("D")};
    PadInput pad = read_held();
    check(pad.stick_x == 127 && pad.stick_y == -127, "W and D push the stick up and right");
    held = {from_name("A"), from_name("D")};
    check(read_held().stick_x == 0, "opposite keys cancel");
    held = {mouse_button(1), mouse_button(3), from_name("Left Shift"), from_name("Q")};
    check(read_held().buttons == (0x1000u | 0x2000u | 0x0200u | 0x0100u), "mouse buttons and keys press their buttons");
    held = {from_name("F")};
    check(read_held().buttons == 0x2000u, "either binding of a control presses it");
    held = {from_name("J"), from_name("K")};
    pad = read_held();
    check(pad.camera_x == -127 && pad.camera_y == 127 && pad.buttons == 0u, "camera keys push the second stick");
    held = {from_name("Up"), from_name("Right"), from_name("Enter"), from_name("Backspace")};
    check(read_held().buttons == (0x0010u | 0x0020u | 0x0008u | 0x0001u), "D-pad, START and SELECT");
}

void test_mouse_turn() {
    MouseTurn t = mouse_turn(10.0f, -20.0f, 0.1f, false, false, 1.0f);
    check(std::fabs(t.yaw - 1.0f) < 1e-5f && std::fabs(t.pitch + 2.0f) < 1e-5f,
          "right turns right and forward looks up, by the sensitivity");
    t = mouse_turn(10.0f, -20.0f, 0.1f, true, true, 0.5f);
    check(std::fabs(t.yaw + 0.5f) < 1e-5f && std::fabs(t.pitch - 1.0f) < 1e-5f, "inversion and the aim's share apply");
}

} // namespace

int main() {
    test_names();
    test_settings_spelling();
    test_layouts();
    test_assign();
    test_read();
    test_mouse_turn();
    std::cout << (failures ? "FAIL" : "PASS") << ": input bindings (" << failures << " failures)\n";
    return failures ? 1 : 0;
}
