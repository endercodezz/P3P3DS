# Developer tools: the Debug page

A page of cheats in the in-game menu, for testing without grinding: money,
any item or equipment piece put straight into the boxes, and on a quest
infinite health and stamina, a frozen clock and monsters at 1 health. The
same tools can be driven from a script through a command file.

**It is for developers only.** It is not in release builds, and it never
writes anything while ad hoc play is on, so a test cannot reach another
player's game.

## Turning it on

| Build | What you get |
| --- | --- |
| Release (`-DMHP3RD_RELEASE=ON`, the builds on the releases page) | Nothing. `host/debug/` and the page are not compiled, whatever else is set |
| Developer build (the default, `MHP3RD_RELEASE=OFF`) | Compiled in (CMake option `MHP3RD_DEBUG_MENU`, on by default; `-DMHP3RD_DEBUG_MENU=OFF` leaves it out), but hidden |
| Developer build run with `MHP3RD_DEBUG_MENU=1` | The menu (Esc, or L3+R3) has a **Debug** tab after System |

It is opt-in at run time on purpose: people who build from source to play
never see it by accident.

While the game has ad hoc networking on, or is in a session, joining one or
hosting, every write is refused and logged as refused; the page shows why.
Reading (the status lines) still works.

Every change is logged as a `[debug] ...` line on the console and in the log,
for example `[debug] gave 99 of 99 <item name> (#9); box holds 112`, and the
page shows the last few.

## What the page does

- **Money and points**: shows zenny and both point balances; adds 100,000
  zenny, sets zenny to 9,999,999 (the most the game shows), adds 100,000 of
  each point balance.
- **Item box**: *Give or remove items* opens a list of every item the game
  has, with the name the game itself shows (read from the running game, so a
  translation mod's names appear as the game shows them). Search by part of a
  name, filter by group (materials, consumables, ammo, decorations, other),
  choose an amount (1 to 990; stacks hold 99), or switch to *Remove* to take
  every stack of an item out. *Fill materials* puts 99 of every material the
  box does not hold yet into the free slots.
- **Equipment box**: *Give equipment* lists every piece of one kind (the five
  armor parts and the twelve weapon classes), layered and collaboration sets
  included, and puts the chosen piece, new and at level 1, into the first free
  slot. Equip it from the item box in the hunter's house as usual.
- **On a quest**: *Infinite health*, *Infinite stamina*, *Freeze the quest
  timer* and *Monsters at 1 health* are held: applied at every flip while
  they are on and a quest is running. The page also shows the time left, the
  hunter's health and each large monster's health.

Save in the game as usual to keep what was given. The changes are in the
game's own memory, so they are saved exactly like anything the game did
itself.

## How it works

`host/debug/` holds everything; the page is `host/ui/debug_screen.cpp`.

- `game_state.{hpp,cpp}`: every address and layout, and the functions that
  read and change them. They work on a small `Ram` interface
  (`guest_ram.hpp`), so `tests/debug_tools_tests.cpp` runs them on a buffer.
- `debug_tools.{hpp,cpp}`: the switch, the ad hoc guard, the log, the held
  cheats and the request queue. The page never writes guest memory itself: it
  queues a request, and requests run between two game frames, on the thread
  that runs the game (`debug::frame` is called from the flip in
  `hle_media.cpp`, before the menu is drawn). The menu itself also runs at the
  flip, so a request made from it runs at once, still between frames, even
  while the menu pauses the game.
- `debug_console.{hpp,cpp}`: the command file.

Nothing outside the host changed: no runtime header, no generated code, no
overlay.

## The command file

`MHP3RD_DEBUG_COMMANDS=<file>` (with `MHP3RD_DEBUG_MENU=1`) names a file read
while the game runs, like `MHP3RD_INPUT_LIVE`: each line appended to it runs
at the next flip, and its answer is printed as `[debug]` lines. Together with
`MHP3RD_INPUT_LIVE` and window captures it drives a whole test from a shell.

```bash
echo "state" >> cmd.txt              # the character, zenny, free slots
echo "give 9 20" >> cmd.txt          # 20 of item 9
```

| Command | What it does |
| --- | --- |
| `state` | Character name, zenny, free item and equipment slots, how many item names were read |
| `money N` | Zenny to N (at most 9,999,999) |
| `give ID [N]` | N (default 1) of item ID into the item box |
| `remove ID` | Every stack of item ID out of the box |
| `fillmats [N]` | N (default 99) of every material the box lacks |
| `giveequip KIND ID` | One equipment piece of that kind byte and id (kinds below) |
| `item ID` | An item's name, group, rarity, pouch limit and how many the box holds |
| `table T [FIRST] [N]` | Entries of the game's text table T |
| `quest` | Time left, the hunter's health, each large monster's health |
| `monsterhp N` | Every large monster's health to N (at least 1) |
| `find8`/`find16`/`find32 V [LO HI]` | Every place in user memory holding V; narrowed with `next V`, `changed`, `unchanged`, `delta D`; `list` shows them |
| `findbytes HEX` | Every place holding that byte string |
| `peek ADDR [N]` | N bytes (default 64) as hex |
| `poke8`/`poke16`/`poke32 ADDR V` | Write one value |
| `dump PATH [ADDR LEN]` | Guest memory to a file (all of it by default), for comparing snapshots offline |

The writing commands are refused during ad hoc play like the page's.

## What the game keeps where

All addresses are for NPJB-40001, the one executable Yakumo supports, and
were measured in the running game with the tools above: a value the game
shows was searched for, changed in the game, searched again, and the survivor
written to see the game follow. A community cheat list for this disc gave
hints for money, the item box, the player's health, the quest clock and the
monster table; each was checked in this build as described, and no code from
it is used.

### Character and money

| What | Where | How it was found |
| --- | --- | --- |
| Zenny (u32) | `0x09FAC8D4` | The save's funds (shown on the character select screen) searched with `find32` in the village: one place. Changing it changed the Status screen's *Funds* |
| Yukumo Points, Guild Points (u32) | `0x09FAC8CC`, `0x09FAC8D0` | Next to the money; their values match the Status screen's |
| Hunter name (UTF-16, 12 characters) | `0x09F4FCAC` | Found beside the equipped set; fullwidth letters. Empty on the title screen, which is how the page tells whether a character is loaded |

A second copy of the money sits at `0x095B1BC8` while the save is being
loaded; the game copies the character to the addresses above when it enters
the village.

### Item box

1000 slots of `{u16 item id, u16 count}` from `0x09F52CF4`; id 0 is an empty
slot. Found from the hint's address; its contents matched the box in the
game, and an item given at an empty slot showed up in the house's item box
with the count given. Stacks never hold more than 99. Right after it, from
`0x09F53C94`, the game keeps other records, so nothing writes past slot 999.

### Item names and groups

- The game loads one block of its text at start (`0x08A40640`): a header of
  32-bit offsets to tables; each table a list of 32-bit offsets from the
  table to UTF-8 strings, ending with `0xFFFFFFFF`. Table 3 holds the item
  names by item id (979 of them, id 0 a placeholder), table 4 their
  descriptions, table 2 among others the monster names, and tables 5 to 38
  the equipment names and descriptions in pairs. Found by searching a memory
  dump for an item's name and walking back to the offset list and the header
  that points at it. The page reads names from here at run time; nothing of
  the game's text is in the repository.
- The item data, 20 bytes per item id, is part of the executable at
  `0x089D0FA0` (found by searching for the pouch limits of the first
  consumables in a row): `+4` a category (0 items, 1 ammo and coatings, 3
  decorations), `+5` rarity, `+6` how many the pouch holds, `+7` non-zero for
  items used from the pouch, `+12` buy and `+16` sell price. The page's groups
  come from these: materials are category 0 items the pouch holds 99 of and
  that are not used.

### Equipment box

1000 slots of 12 bytes from `0x09F4FE14`, ending where the item box begins:

| Offset | Size | Meaning |
| --- | --- | --- |
| `+0` | u8 | 1 for a used slot, 0 for an empty one |
| `+1` | u8 | Kind (below) |
| `+2` | u16 | Id within the kind: the index into that kind's name table |
| `+4` | u16 | Armor level; for weapons flags the game sets |
| `+6` | u16 × 3 | The item ids of the decorations in its slots |

Found by searching a dump for the ids of the equipped armor (looked up by
name in the name tables) and finding the equipped pieces' records; the list
of the equipped pieces' box indices at `0x09F54B1A` (weapon, chest, arms,
waist, legs, head) confirmed which record is which. The kind of each weapon
class was told apart by poking the equipped weapon's slot to other records
and reading the class the Equipment screen showed, and by which name table
each kind's ids fit.

| Kind | Name table | Kind | Name table |
| --- | --- | --- | --- |
| 4 head | 29 | 5 Great Sword | 5 |
| 0 chest | 31 | 6 Sword and Shield | 7 |
| 1 arms | 33 | 7 Hammer | 9 |
| 2 waist | 35 | 8 Lance | 11 |
| 3 legs | 37 | 9 Heavy Bowgun | 13 |
| | | 11 Light Bowgun | 15 |
| | | 12 Long Sword | 17 |
| | | 13 Switch Axe | 19 |
| | | 14 Gunlance | 21 |
| | | 15 Bow | 23 |
| | | 16 Dual Blades | 25 |
| | | 17 Hunting Horn | 27 |

Kind 101 records are talismans; the page does not make them. Verified by
giving one piece of every weapon kind and a full collaboration armor set,
then finding each in the equipment box under its own name and class icon, and
equipping the armor.

### On a quest

The task slot at `0x0A05E600` holds `game_task.ovl` while a quest runs and
`lobby_task.ovl` in the village and the Guild Hall (the overlay's header names
it). The addresses below hold other things outside a quest, so the tools read
and write them only while `game_task.ovl` is there.

| What | Where | How it was found |
| --- | --- | --- |
| Health now (s16) | `0x09649B16` | The hint pointed at `0x09649B58`; a dump on a quest showed 150 there and at `0x09649B16` and `0x09649B56`. Writing 50 at `0x09649B16` emptied two thirds of the health bar at once |
| Recoverable health (s16) | `0x09649B56` | Writing it alone changed nothing on screen until health moved: it is the red part of the bar |
| Most health (s16) | `0x09649B58` | 150, the Status screen's *Health* |
| Stamina now (float) | `0x09649DF0` | Three dumps: before sprinting, after sprinting, after resting; the float there went 724, 521, 900 |
| Most stamina (u16) | `0x0964A49A` | 900: the game counts 6 units per point shown |
| Quest time left (u32, frames at 30 a second) | `0x09FB4E68` | Dumps a few seconds apart: the one word that fell by 30 a second, from the 50 minutes of a fresh quest (90,000) |
| Quest time limit (u32) | `0x09FB4E64` | 90,000 beside it for a 50-minute quest |
| Monster table | `0x0A1B0AE0` | Pointers to the large monsters (and companions), in the quest overlay's data. The hint's code read it; on the quest, the first entry's object held the kind of the monster the quest listed and 4400/4400 health |
| Monster kind, health, most health | object `+0x62` (u8), `+0x246` (s16), `+0x288` (s16) | As above. The kind indexes the monster names in text table 2 from entry 308 |

## Not verified, and not done

- *Unlock all quests*, village progress flags and hunter rank are not on the
  page. The hint list has a quest-flag area, and a byte beside the points
  that looked like the rank (`0x09FAC8C5`: 6 on a rank 6 hunter, but 0 on a
  rank 1 hunter) did not change the Status screen's HR when written, so
  neither is understood well enough to write without risking an
  inconsistent save.
- The quest addresses were traced on a gathering quest in the Misty Peaks,
  and the page's switches verified on a low-rank hunting quest there: with infinite health on, health written down
  to 20 was back at its most by the next read; stamina stayed at 900 while
  the hunter sprinted; the clock stood at the same second for over ten
  seconds; with monsters at 1 health the quest's Arzuros read 1/960. Killing
  a monster with that one hit was not tried, nor were multi-monster quests,
  arena quests or the Guild Hall with companions.
- Saves: a late save (every armor, HR 6) and an early one (HR 1, three hours
  played). With the early one the page read the character, set zenny to
  9,999,999 (the Status screen showed it) and gave an item and an armor
  piece; that equipment was not opened in the game's equipment screen on the
  early save.
