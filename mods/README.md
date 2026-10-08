# BotW modding lessons

Nine small WiiXLaunch mods for Breath of the Wild (Wii U v208, Cemu). Each one
adds one idea on top of the last. Every `mod.cpp` opens with a **Concepts** list
and ends its header with **Try:** exercises.

```text
wxl build [mod ...]     compile and install (default: all lessons)
wxl play                launch the game (restart after rebuilding)
wxl log                 your WIIXL_LOG lines from the last run
wxl log stamina         ...only lines containing "stamina"
```

(`./wxl` on macOS, `wxl` on Windows, run from the workspace root.)

To run only some lessons, delete the other `.wxlm` files from
`cemu/portable/graphicPacks/WiiXLaunch_BotW/content/WiiXLaunch/mods/`.

## The lessons

| # | Mod | Controls | New idea | Tested in game |
|---|---|---|---|---|
| 1 | `hello` | none | Entry point, imports, ticks, logging | Yes: log every ~10 s |
| 2 | `stamina` | sprint | Read and write game state every frame; game units | Yes: refilled 970 → 1000 while sprinting |
| 3 | `hud` | none | Drawing with the game's fonts; frame callbacks; styles | Yes: panel bottom-left with live clock, weather, hearts, rupees |
| 4 | `time_warp` | ZL + D-pad | Input combos; world clock and weather; **the game can override you** | Yes: x32 moved 09:10 → 11:00 in ~10 s; forced HeavyRain shows on screen |
| 5 | `loadout` | ZL + R | Inventory; asynchronous equips; "returned true" ≠ "worked" | Yes: sword, bow, shield and arrows equipped |
| 6 | `unbreakable` | none | Actor handles vs. actor IDs; state across frames | Yes: bow shot 2200 → 2100, restored to 2200 |
| 7 | `bounty` | none | Event queues; something must pump them | Loads only. No korok, shrine or tower was reached in testing |
| 8 | `jetpack` | hold ZL + X | *When* in the frame you write matters; physics inputs | Yes: ~3 s of thrust put Link high enough for −17 °C |
| 9 | `rupee_hook` | ZL + Minus = test | Raw function hook at a known address; calling game code | Yes via the test key: 5 → 10 through the hook. Not tested with a real rupee pickup |

ZL is the **Z** key on both keyboard profiles (see the top-level README).

## Things that went wrong while writing these

Each of these is a real bug from building these mods, and each makes a good
"why doesn't this work?" exercise:

1. **Every mod grabbed ~200 KB of memory** because none set `heapRequest`, and
   the GUI's fonts then didn't fit. Fix: `"heapRequest": 8192` in each `mod.json`.
   The boot log's `Arena:` lines show what each mod was granted.
2. **The clock stuck at 11:00** (`time_warp`). It wasn't the mod. With no mods
   touching anything, the clock ran 09:11 → 09:39 → 10:08 → 10:37 and then held
   at 11:00. At this point in the story, BotW pins it there. Always measure the
   game without your mod before blaming your code.
3. **Items given on the title screen vanished** (`loadout`). `AddItem` returned
   true because a pouch already exists there, and loading the save replaced it.
   Even "is there a Link?" passes on the title screen. The title-screen Link has
   max life 0, so that became the guard.
4. **"Now protecting sword" printed every tick** (`unbreakable`). Two separate
   causes: (a) every `GetEquippedSword()` call mints a new handle, so handles
   can't be compared; use `botw.actor GetId`. (b) While an equip settles, the
   lookup returns nothing on alternate ticks, and clearing state on that looked
   like a brand-new weapon.
5. **The tick runs ~60 times a second, not 30.** BotW renders 30 fps but presents
   two screens (TV + GamePad) per frame, and the tick rides the present.
6. **ZL was on Tab, and Cemu shows the GamePad screen while Tab is held.**
   Remapped to Z.
7. **The Minus press also opened the map** (`rupee_hook` test key). Mods see
   input, but so does the game. `botw.gui CaptureInput` exists to hide input
   from the game.

## Where to go next

- `WiiXLaunch/sdk/include/wiixlaunch/imports/*.h`: every surface and its
  functions, with the framework authors' comments.
- `WiiXLaunch/vendor/wiixlaunch-botw/data/symbols-wiiu-v208.csv`: reverse-engineered
  addresses with confidence levels, for writing hooks like lesson 9.
- `WiiXLaunch/examples/patch_mod`: patching instruction bytes directly
  (`WIIXL_DECLARE_PATCH`).
- [zeldaret/botw](https://github.com/zeldaret/botw): the decompilation, for
  understanding what a function does before hooking it. It targets Switch
  1.5.0, so names carry over to the Wii U build but addresses don't.
