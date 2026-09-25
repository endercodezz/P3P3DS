// The menu's Debug page: the developer tools for testing without grinding.
// Everything it changes goes through debug::request, which writes guest memory
// between two game frames and logs a "[debug] ..." line; everything it shows
// is read through debug::read. The game's structures are in
// debug/game_state.hpp.

#include "ui/debug_screen.hpp"

#include "debug/debug_tools.hpp"
#include "debug/game_state.hpp"
#include "debug/guest_ram.hpp"
#include "ui/text_input.hpp"
#include "ui/widgets.hpp"

#include "imgui.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdio>
#include <map>
#include <string>
#include <utility>
#include <vector>

namespace mhp3rd::ui {
namespace {

namespace game = debug::p3rd;

enum class Screen { Page, Items, Equipment };

// What the page shows, read from guest memory once a frame.
struct Snapshot {
    bool loaded{};
    std::string name;
    std::uint32_t money{};
    std::uint32_t points1{};
    std::uint32_t points2{};
    std::uint32_t free_items{};
    std::uint32_t free_equipment{};
    std::map<std::uint16_t, std::uint32_t> in_box;  // item id -> count
    std::map<std::pair<std::uint8_t, std::uint16_t>, std::uint32_t> equipment_owned;
    debug::HeldCheats held;
};

struct State {
    Screen screen{Screen::Page};
    bool focus{};
    std::string search;
    int group{};       // 0: all, then the item groups
    int amount{3};     // index into kAmounts
    bool remove{};
    int kind{};        // index into game::equipment_kinds()
    std::vector<game::Item> items;  // the game's item list, read once
    std::map<std::uint8_t, std::vector<std::string>> equipment_names;
    Snapshot snap;
};

State &state() {
    static State s;
    return s;
}

constexpr std::array<std::uint32_t, 6> kAmounts{1u, 5u, 10u, 99u, 500u, 990u};
constexpr std::array<game::ItemGroup, 5> kGroups{game::ItemGroup::Material, game::ItemGroup::Consumable,
                                                  game::ItemGroup::Ammo, game::ItemGroup::Decoration,
                                                  game::ItemGroup::Other};
constexpr std::uint32_t kMoneyStep = 100'000u;

int cycle(int value, int delta, int count) { return ((value + delta) % count + count) % count; }

std::string thousands(std::uint32_t value) {
    std::string digits = std::to_string(value);
    for (int i = static_cast<int>(digits.size()) - 3; i > 0; i -= 3) digits.insert(static_cast<std::size_t>(i), ",");
    return digits;
}

std::string lower(std::string text) {
    for (char &c : text) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return text;
}

bool matches(const std::string &name, const std::string &search) {
    return search.empty() || lower(name).find(lower(search)) != std::string::npos;
}

void read_snapshot() {
    State &s = state();
    debug::read([&](const debug::Ram &ram) {
        Snapshot &n = s.snap;
        n = {};
        n.loaded = game::character_loaded(ram);
        n.name = game::hunter_name(ram);
        if (s.items.empty()) s.items = game::item_list(ram);
        if (s.equipment_names.empty())
            for (const game::EquipmentKind &k : game::equipment_kinds())
                s.equipment_names[k.kind] = game::equipment_names(ram, k.kind);
        n.held = debug::held_cheats();
        if (!n.loaded) return;
        n.money = game::money(ram);
        n.points1 = ram.load32(game::kPoints1);
        n.points2 = ram.load32(game::kPoints2);
        for (const game::ItemStack &stack : game::item_box(ram))
            if (stack.id == 0u) ++n.free_items;
            else n.in_box[stack.id] += stack.count;
        for (const auto &piece : game::equipment_box(ram))
            if (!piece) ++n.free_equipment;
            else ++n.equipment_owned[{piece->kind, piece->id}];
    });
}

// Why a write cannot be made now, or "" when it can.
std::string write_note() {
    if (std::string why = debug::blocked_reason(); !why.empty()) return "Off: " + why;
    if (!state().snap.loaded) return "No character loaded";
    return {};
}

RowOptions write_row(std::string description) {
    RowOptions o;
    o.description = std::move(description);
    o.note = write_note();
    o.disabled = !o.note.empty();
    return o;
}

void open_search() {
    TextInputRequest request;
    request.title = "Search";
    request.prompt = "Part of a name, in the game's own text. Empty shows everything.";
    request.initial = state().search;
    request.max_length = 24u;
    open_text_input(std::move(request), [](std::optional<std::string> text) {
        if (text) state().search = std::move(*text);
        state().focus = true;
    });
}

void focus_once() {
    State &s = state();
    if (s.focus) {
        focus_next_row();
        s.focus = false;
    }
}

void page() {
    State &s = state();
    const Snapshot &n = s.snap;
    section("Developer tools");
    focus_once();
    info_row("Character", n.loaded ? n.name : std::string("None loaded"));
    if (!debug::blocked_reason().empty())
        info_row("Writes", "Off while " + debug::blocked_reason());

    section("Money and points");
    info_row("Zenny", thousands(n.money) + "z");
    if (button_row("Add 100,000 zenny", write_row("Adds to the hunter's money, up to the 9,999,999 the game shows.")))
        debug::request("add zenny", [](debug::Ram &ram) {
            game::set_money(ram, game::money(ram) + kMoneyStep);
            return "zenny now " + std::to_string(game::money(ram));
        });
    if (button_row("Zenny to 9,999,999", write_row("The most money the game shows.")))
        debug::request("max zenny", [](debug::Ram &ram) {
            game::set_money(ram, game::kMostMoney);
            return "zenny now " + std::to_string(game::money(ram));
        });
    info_row("Yukumo Points", thousands(n.points1));
    info_row("Guild Points", thousands(n.points2));
    if (button_row("Add 100,000 of each point", write_row("Yukumo Points and Guild Points, up to 9,999,999.")))
        debug::request("add points", [](debug::Ram &ram) {
            for (const std::uint32_t at : {game::kPoints1, game::kPoints2})
                ram.store32(at, std::min(ram.load32(at) + kMoneyStep, game::kMostMoney));
            return "points now " + std::to_string(ram.load32(game::kPoints1)) + " Yukumo, " +
                   std::to_string(ram.load32(game::kPoints2)) + " Guild";
        });

    section("Item box");
    info_row("Free slots##items", std::to_string(n.free_items) + " of " + std::to_string(game::kItemBoxSlots));
    {
        RowOptions o = write_row("Any item by name, with a count, or take an item out of the box.");
        if (s.items.empty()) {
            o.disabled = true;
            o.note = "The game's item names are not loaded yet";
        }
        if (value_row("Give or remove items", std::to_string(s.items.size()) + " items", o)) {
            s.screen = Screen::Items;
            s.focus = true;
        }
    }
    if (button_row("Fill materials",
                   write_row("99 of every material the box does not hold yet, while it has free slots.")))
        debug::request("fill materials", [](debug::Ram &ram) {
            return "added 99 of " + std::to_string(game::fill_materials(ram, game::kMostPerStack)) +
                   " materials; " + std::to_string(game::free_item_slots(ram)) + " slots left";
        });

    section("Equipment box");
    info_row("Free slots##equipment",
             std::to_string(n.free_equipment) + " of " + std::to_string(game::kEquipmentBoxSlots));
    if (value_row("Give equipment", "",
                  write_row("Any weapon or armor piece, layered and collaboration sets included, new and at "
                            "level 1. Equip it from the item box in the hunter's house."))) {
        s.screen = Screen::Equipment;
        s.focus = true;
    }

    section("On a quest");
    {
        debug::HeldCheats held = n.held;
        bool changed = false;
        const RowOptions quest = write_row("");
        const auto held_row = [&](const char *label, bool &value, const char *description) {
            RowOptions o = quest;
            o.description = description;
            if (toggle_row(label, value, o)) {
                value = !value;
                changed = true;
            }
        };
        held_row("Infinite health", held.health, "Keeps the hunter's health at its most, every frame.");
        held_row("Infinite stamina", held.stamina, "Keeps the hunter's stamina at its most, every frame.");
        held_row("Freeze the quest timer", held.timer, "The quest clock stops where it is.");
        held_row("Monsters at 1 health", held.one_hit, "Every large monster's health is set to 1, so the next hit "
                                                         "ends it.");
        if (changed) debug::set_held_cheats(held);
        // Each row needs an id of its own for the pad to move between them.
        int row = 0;
        for (const std::string &line : debug::quest_status())
            info_row(("Quest##quest" + std::to_string(row++)).c_str(), line);
    }

    section("Log");
    for (const std::string &line : debug::recent_log()) paragraph(line, colors::kTextDim);
}

void items_screen(bool back) {
    State &s = state();
    if (back) {
        s.screen = Screen::Page;
        s.focus = true;
        return;
    }
    const Snapshot &n = s.snap;
    section(s.remove ? "Remove items" : "Give items");
    focus_once();
    if (button_row(("Search: " + (s.search.empty() ? std::string("everything") : s.search) + "###search").c_str(),
                   {false, {}, "Type part of a name."}))
        open_search();
    static const char *const kGroupNames[] = {"All", "Materials", "Consumables", "Ammo", "Decorations", "Other"};
    if (const int d = choice_row("Group", kGroupNames[s.group])) s.group = cycle(s.group, d, 6);
    if (const int d = choice_row("Action", s.remove ? "Remove every stack" : "Give")) s.remove = !s.remove;
    if (!s.remove)
        if (const int d = choice_row("Amount", std::to_string(kAmounts[static_cast<std::size_t>(s.amount)]),
                                     {false, {}, "How many to give. Stacks hold 99 each."}))
            s.amount = cycle(s.amount, d, static_cast<int>(kAmounts.size()));
    const std::string note = write_note();
    section("Items");
    std::size_t shown = 0u;
    for (const game::Item &item : s.items) {
        if (s.group != 0 && item.group != kGroups[static_cast<std::size_t>(s.group - 1)]) continue;
        if (!matches(item.name, s.search)) continue;
        const auto held = n.in_box.find(item.id);
        const std::uint32_t count = held == n.in_box.end() ? 0u : held->second;
        if (s.remove && count == 0u) continue;
        ++shown;
        const std::string id = "##item" + std::to_string(item.id);
        const std::string detail = (count != 0u ? std::to_string(count) + " in box   " : std::string()) +
                                   game::group_name(item.group) + "  #" + std::to_string(item.id);
        if (list_row(id.c_str(), item.name, detail, ListIcon::None, count != 0u) && note.empty()) {
            const std::uint16_t item_id = item.id;
            const std::string name = item.name;
            if (s.remove) {
                debug::request("remove " + name, [item_id, name](debug::Ram &ram) {
                    return "removed " + std::to_string(game::remove_item(ram, item_id)) + " " + name + " (#" +
                           std::to_string(item_id) + ")";
                });
            } else {
                const std::uint32_t amount = kAmounts[static_cast<std::size_t>(s.amount)];
                debug::request("give " + name, [item_id, name, amount](debug::Ram &ram) {
                    const std::uint32_t given = game::give_item(ram, item_id, amount);
                    return "gave " + std::to_string(given) + " of " + std::to_string(amount) + " " + name + " (#" +
                           std::to_string(item_id) + "); box holds " + std::to_string(game::box_count(ram, item_id));
                });
            }
        }
    }
    if (shown == 0u) paragraph(s.remove ? "The box holds none of these." : "No item matches.", colors::kTextDim);
    if (!note.empty()) paragraph(note, colors::kDanger);
}

void equipment_screen(bool back) {
    State &s = state();
    if (back) {
        s.screen = Screen::Page;
        s.focus = true;
        return;
    }
    const std::vector<game::EquipmentKind> &kinds = game::equipment_kinds();
    const game::EquipmentKind &kind = kinds[static_cast<std::size_t>(s.kind)];
    section("Give equipment");
    focus_once();
    if (const int d = choice_row("Kind", kind.label)) s.kind = cycle(s.kind, d, static_cast<int>(kinds.size()));
    if (button_row(("Search: " + (s.search.empty() ? std::string("everything") : s.search) + "###search").c_str(),
                   {false, {}, "Type part of a name."}))
        open_search();
    const std::string note = write_note();
    section(kind.label);
    const std::vector<std::string> &names = s.equipment_names[kind.kind];
    std::size_t shown = 0u;
    // Id 0 is "no equipment".
    for (std::size_t id = 1; id < names.size(); ++id) {
        if (names[id].empty() || !matches(names[id], s.search)) continue;
        ++shown;
        const auto piece = std::make_pair(kind.kind, static_cast<std::uint16_t>(id));
        const auto owned = s.snap.equipment_owned.find(piece);
        const std::uint32_t count = owned == s.snap.equipment_owned.end() ? 0u : owned->second;
        const std::string row_id = "##equip" + std::to_string(id);
        const std::string detail =
            (count != 0u ? std::to_string(count) + " owned   " : std::string()) + "#" + std::to_string(id);
        if (list_row(row_id.c_str(), names[id], detail, ListIcon::None, count != 0u) && note.empty()) {
            const std::string name = names[id];
            const std::string label = kind.label;
            debug::request("give " + name, [piece, name, label](debug::Ram &ram) {
                const std::optional<std::uint32_t> slot = game::give_equipment(ram, piece.first, piece.second);
                return slot ? "gave " + label + " " + name + " (kind " + std::to_string(piece.first) + ", #" +
                                  std::to_string(piece.second) + ") in equipment box slot " + std::to_string(*slot)
                            : "equipment box full: " + name + " not given";
            });
        }
    }
    if (shown == 0u) paragraph("Nothing matches.", colors::kTextDim);
    if (!note.empty()) paragraph(note, colors::kDanger);
}

} // namespace

void debug_page(bool back) {
    read_snapshot();
    switch (state().screen) {
    case Screen::Items: items_screen(back); break;
    case Screen::Equipment: equipment_screen(back); break;
    default: page(); break;
    }
}

bool debug_screen_open() { return state().screen != Screen::Page; }

} // namespace mhp3rd::ui
