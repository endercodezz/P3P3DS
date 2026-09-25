// The developer tools' knowledge of the game's memory (debug/game_state.hpp),
// run on a buffer standing for guest memory: text tables, the item and
// equipment boxes, money and the quest cheats. No game data: the tables and
// names here are made up.
#include "debug/debug_tools.hpp"
#include "debug/game_state.hpp"
#include "debug/guest_ram.hpp"

#include <cstdint>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

namespace {
using namespace mhp3rd::debug;
namespace game = mhp3rd::debug::p3rd;
int failures{};

void check(bool condition, const char *message) {
    if (!condition) {
        ++failures;
        std::cerr << "FAIL: " << message << '\n';
    }
}

constexpr std::uint32_t kBase = 0x08800000u;
constexpr std::size_t kSize = 0x02000000u;  // up to 0x0A800000

// Writes a text table the way the game lays one out: offsets from the table
// start, an end marker, then the strings.
void write_table(Ram &ram, std::uint32_t at, const std::vector<std::string> &strings) {
    const auto count = static_cast<std::uint32_t>(strings.size());
    std::uint32_t text = (count + 1u) * 4u;
    for (std::uint32_t i = 0; i < count; ++i) {
        ram.store32(at + i * 4u, text);
        for (std::size_t c = 0; c <= strings[i].size(); ++c)
            ram.store8(at + text + static_cast<std::uint32_t>(c),
                       c < strings[i].size() ? static_cast<std::uint8_t>(strings[i][c]) : 0u);
        text += static_cast<std::uint32_t>(strings[i].size() + 1u);
    }
    ram.store32(at + count * 4u, 0xFFFFFFFFu);
}

// A text block with the item names at table 3 and one armor table.
void write_text(Ram &ram) {
    const std::uint32_t items = game::kTextBlock + 0x1000u;
    const std::uint32_t chest = game::kTextBlock + 0x3000u;
    ram.store32(game::kTextBlock + 3u * 4u, items - game::kTextBlock);
    ram.store32(game::kTextBlock + 31u * 4u, chest - game::kTextBlock);
    write_table(ram, items, {"-", "Guide", "Tonic", "Ore", "Scale", "Round"});
    write_table(ram, chest, {"No Equipment", "Plain Mail", "Test Suit"});
}

// Item records: category, rarity, carry, use at +4..+7.
void write_item(Ram &ram, std::uint16_t id, std::uint8_t category, std::uint8_t carry, std::uint8_t use) {
    const std::uint32_t at = game::kItemData + id * game::kItemRecord;
    ram.store8(at + 4u, category);
    ram.store8(at + 6u, carry);
    ram.store8(at + 7u, use);
}

void write_items(Ram &ram) {
    write_item(ram, 1, 0, 1, 0);    // Guide: a book
    write_item(ram, 2, 0, 10, 8);   // Tonic: used from the pouch
    write_item(ram, 3, 0, 99, 0);   // Ore: a material
    write_item(ram, 4, 0, 99, 0);   // Scale: a material
    write_item(ram, 5, 1, 60, 0);   // Round: ammo
}

void test_text() {
    BufferRam ram(kBase, kSize);
    check(game::text_table(ram, game::kTextBlock, 3).empty(), "no table before the text is loaded");
    write_text(ram);
    const std::vector<std::string> names = game::text_table(ram, game::kTextBlock, 3);
    check(names.size() == 6u && names[3] == "Ore" && names[5] == "Round", "a table reads back in order");
    check(game::text_entry(ram, game::kTextBlock, 3, 4) == "Scale", "one entry reads alone");
    check(game::text_entry(ram, game::kTextBlock, 3, 6).empty(), "past the end is empty");
    // A broken end marker means it is not a table.
    ram.store32(game::kTextBlock + 0x1000u + 6u * 4u, 0u);
    check(game::text_table(ram, game::kTextBlock, 3).empty(), "a table without its end marker is refused");
}

void test_items() {
    BufferRam ram(kBase, kSize);
    write_text(ram);
    write_items(ram);
    const std::vector<game::Item> items = game::item_list(ram);
    check(items.size() == 5u && items[0].id == 1u, "every named item but the empty id 0");
    check(items[0].group == game::ItemGroup::Other && items[1].group == game::ItemGroup::Consumable &&
              items[2].group == game::ItemGroup::Material && items[4].group == game::ItemGroup::Ammo,
          "items are grouped from their records");

    check(game::free_item_slots(ram) == game::kItemBoxSlots, "an empty box");
    check(game::give_item(ram, 3, 150) == 150u, "150 go in");
    check(game::box_count(ram, 3) == 150u && game::free_item_slots(ram) == game::kItemBoxSlots - 2u,
          "as a full stack of 99 and one of 51");
    check(game::give_item(ram, 3, 50) == 50u && game::box_count(ram, 3) == 200u &&
              game::free_item_slots(ram) == game::kItemBoxSlots - 3u,
          "the part stack is topped up before a new one starts");
    check(game::remove_item(ram, 3) == 200u && game::box_count(ram, 3) == 0u &&
              game::free_item_slots(ram) == game::kItemBoxSlots,
          "remove takes every stack");
    check(game::give_item(ram, 0, 5) == 0u, "id 0 is never given");

    game::give_item(ram, 4, 1);
    check(game::fill_materials(ram, 99) == 1u && game::box_count(ram, 3) == 99u && game::box_count(ram, 4) == 1u &&
              game::box_count(ram, 2) == 0u,
          "fill adds only the materials the box lacks");

    // A full box takes nothing more.
    for (std::uint32_t i = 0; i < game::kItemBoxSlots; ++i) ram.store16(game::kItemBox + i * 4u, 9u);
    check(game::give_item(ram, 2, 5) == 0u, "a full box takes nothing");
}

void test_equipment() {
    BufferRam ram(kBase, kSize);
    write_text(ram);
    check(game::equipment_kind(0) != nullptr && game::equipment_kind(0)->name_table == 31, "chest names in table 31");
    check(game::equipment_kind(99) == nullptr && !game::give_equipment(ram, 99, 1), "an unknown kind is refused");
    check(game::equipment_names(ram, 0).size() == 3u && game::equipment_names(ram, 0)[2] == "Test Suit",
          "a kind's names come from its table");
    // Slot 0 in use.
    ram.store8(game::kEquipmentBox, 1u);
    ram.store8(game::kEquipmentBox + 1u, 5u);
    ram.store16(game::kEquipmentBox + 6u, 0x1234u);
    const auto slot = game::give_equipment(ram, 0, 2);
    const std::uint32_t at = game::kEquipmentBox + game::kEquipmentRecord;
    check(slot && *slot == 1u, "given into the first free slot");
    check(ram.load8(at) == 1u && ram.load8(at + 1u) == 0u && ram.load16(at + 2u) == 2u && ram.load16(at + 6u) == 0u,
          "a new piece: used, its kind and id, no decorations");
    check(ram.load16(game::kEquipmentBox + 6u) == 0x1234u, "the slot in use is untouched");
    check(game::free_equipment_slots(ram) == game::kEquipmentBoxSlots - 2u, "two slots used");
    const auto box = game::equipment_box(ram);
    check(box[1] && box[1]->id == 2u && !box[2], "the box reads back");
}

void test_money_and_name() {
    BufferRam ram(kBase, kSize);
    check(!game::character_loaded(ram), "no name, no character");
    const char16_t name[] = u"Ｈｕｎ　A";  // fullwidth "Hun", a space, "A"
    for (std::size_t i = 0; i < 5; ++i) ram.store16(game::kHunterName + static_cast<std::uint32_t>(i * 2u), name[i]);
    check(game::character_loaded(ram) && game::hunter_name(ram) == "Hun A", "fullwidth names read as ASCII");
    game::set_money(ram, 123u);
    check(game::money(ram) == 123u, "money set");
    game::set_money(ram, 50'000'000u);
    check(game::money(ram) == game::kMostMoney, "money stops at what the game shows");
}

// The quest overlay's header in the task slot.
void load_overlay(Ram &ram, const char *name) {
    ram.store32(game::kTaskSlot, 0x336F574Du);  // "MWo3"
    ram.store32(game::kTaskSlot + 8u, game::kTaskSlot);
    for (std::size_t i = 0; i < 32u; ++i)
        ram.store8(game::kTaskSlot + 32u + static_cast<std::uint32_t>(i),
                   i < std::strlen(name) ? static_cast<std::uint8_t>(name[i]) : 0u);
}

void test_quest() {
    BufferRam ram(kBase, 0x02000000u);
    BufferRam &r = ram;
    check(!game::on_quest(r), "no overlay, no quest");
    load_overlay(r, "lobby_task.ovl");
    check(!game::on_quest(r) && game::monsters(r).empty(), "the village is not a quest");

    // A monster, and a companion with no health.
    const std::uint32_t monster = 0x0A000000u;
    const std::uint32_t companion = 0x0A001000u;
    r.store32(game::kMonsterTable, monster);
    r.store32(game::kMonsterTable + 4u, companion);
    r.store8(monster + game::kMonsterKind, 42u);
    r.store16(monster + game::kMonsterHealth, 4400u);
    r.store16(monster + game::kMonsterMostHealth, 4400u);
    r.store16(game::kMostHealth, 150u);
    r.store16(game::kHealth, 20u);
    r.store16(game::kMostStamina, 900u);
    r.store32(game::kQuestTimeLeft, 90'000u);
    r.store32(game::kQuestTimeLimit, 90'000u);

    HeldCheats all{true, true, true, true};
    held_cheats_frame(r, all);
    check(r.load16(game::kHealth) == 20u, "nothing is held outside a quest");

    load_overlay(r, "game_task.ovl");
    check(game::on_quest(r), "the quest overlay means a quest");
    const auto found = game::monsters(r);
    check(found.size() == 1u && found[0].kind == 42u && found[0].health == 4400, "the companion is left out");

    held_cheats_frame(r, all);
    float stamina = 0.0f;
    const std::uint32_t bits = r.load32(game::kStamina);
    std::memcpy(&stamina, &bits, sizeof(stamina));
    check(r.load16(game::kHealth) == 150u && r.load16(game::kRecoverableHealth) == 150u, "health held at its most");
    check(stamina == 900.0f, "stamina held at its most");
    check(r.load16(monster + game::kMonsterHealth) == 1u, "monsters at 1 health");
    check(r.load16(companion + game::kMonsterHealth) == 0u, "the companion is not touched");

    r.store32(game::kQuestTimeLeft, 89'970u);  // the game counted a second down
    held_cheats_frame(r, all);
    check(r.load32(game::kQuestTimeLeft) == 90'000u, "the clock is held where it was frozen");
    held_cheats_frame(r, HeldCheats{});
    r.store32(game::kQuestTimeLeft, 89'970u);
    held_cheats_frame(r, HeldCheats{});
    check(r.load32(game::kQuestTimeLeft) == 89'970u, "unfrozen, the clock runs");
}

} // namespace

int main() {
    test_text();
    test_items();
    test_equipment();
    test_money_and_name();
    test_quest();
    if (failures != 0) {
        std::cerr << failures << " check(s) failed\n";
        return 1;
    }
    std::cout << "debug tools tests passed\n";
    return 0;
}
