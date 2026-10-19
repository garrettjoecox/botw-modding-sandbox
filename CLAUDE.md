# CLAUDE.md

C++ code mods for *Breath of the Wild* (Wii U v208) running in Cemu through the
WiiXLaunch framework. The user is tutoring a beginner (on Windows) who edits
one file, `mod/my_mod.cpp`, and pastes in pieces from `mod/examples/`. Work
happens on the user's Mac: a portable Cemu nightly (arm64, Metal) in `cemu/`.
The Windows set-up (Cemu 2.6, Vulkan, `wxl.cmd`) hasn't been run on a real
Windows machine yet.

`README.md` covers setup and layout. `mod/README.md` covers the student's
curriculum, the examples table and troubleshooting. `todo.md` is the user's
list of ideas.

## Rules

- **No game files, saves or Nintendo assets in git.** `.gitignore` covers
  `cemu/`, `gameFiles/`, `extracted/`, `backups/`, `graphicPacks/` and zips.
  Keep the educational-use notice in `README.md`.
- **Commit or push only when asked**, with no `Co-Authored-By` trailer. The
  user often commits and pushes their own work.
- **Never overwrite `cemu/portable/controllerProfiles/controller0.xml`**: it
  holds the user's key binds. `wxl setup` only installs a profile when there
  isn't one.
- **The save in `cemu/portable` is the user's real game**, and they play
  between sessions. Test only inside `tools/test/session.sh start` … `end`
  (see below). Never restore an older backup over it without diffing it first.
  Never quit a Cemu you didn't start: ask the user to close it.
- **Keep in-game tests short.** Do one focused pass per feature and stop once
  it visibly works. The user often watches and will say "it works"; then clean
  up straight away. Don't try alternatives one by one in the game. Check names
  and behaviour offline first (the dump, the decompilation, the symbol list).
- **`mod/my_mod.cpp` stays the clean template** in commits. It must keep both
  `every_frame()` and `when_game_adds()`; without either, packing fails with
  `references 1 symbol(s) it does not define … _Z11every_framev`.

## Layout

| Path | What |
|---|---|
| `mod/my_mod.cpp` | The student's file: a "Paste examples here" spot, `every_frame()` and `when_game_adds()` |
| `mod/botw.h` | The friendly API (snake_case), commented for a beginner |
| `mod/engine/` | Everything WiiXLaunch: `engine.h` (imports), `main.cpp` (entry, hooks), `game.cpp` (API), `screen.cpp` (drawing, `print`, menus) |
| `mod/examples/*.cpp` | Copy-paste examples. **Not built** (`wxl build` skips them) |
| `mod/mod.json` | id `my_mod`, `heapRequest` 8192, the required surfaces. `wxl build` rewrites `sources` |
| `tools/wxl.py` | The workspace script (`./wxl` on macOS, `wxl.cmd` on Windows) |
| `tools/botw_files.py` | Readers and writers for the game's formats: Yaz0, SARC, MSBT, AAMP, BYML, RSTB |
| `tools/new_items.py` | `wxl items`: builds new items (the Spawning Wand, `Weapon_Sword_900`) from the dump into the graphic pack's `content/` |
| `tools/test/` | In-game test helpers (macOS only, see below) |
| `tools/profiles/` | Default keyboard profiles (the old GamePad scheme) |
| `tools/wiixlaunch-local.patch` | Local changes to WiiXLaunch, applied by `wxl setup` |
| `pack/WiiXLaunch_BotW/` | Prebuilt host graphic pack, the same on every OS |
| `WiiXLaunch/` | Framework checkout at a pinned commit (gitignored) |
| `cemu/` | Cemu, its config, the game, saves (gitignored) |
| `backups/` | Save backups, `save-101c9400-<YYYYmmdd-HHMMSS>` (gitignored) |
| `gameFiles/`, `extracted/` | The user's original dumps (gitignored) |

## Build and run

```bash
open -a Docker                 # first, if `docker info` fails: builds run in the wiixl-dkp image
./wxl build                    # compile mod/ and install my_mod.wxlm into Cemu's graphic pack
./wxl play                     # launch the game (mods load at boot, so restart after every build)
./wxl log [TEXT]               # "my_mod: " lines + loader problems from the last run; --all for everything
./wxl actors [TEXT]            # ~6,300 actor names with English names, from the dump
./wxl items                    # rebuild the new items into the graphic pack (setup runs it too)
./wxl doctor                   # what's installed or missing
```

- A build takes a second or two once the Docker image exists.
- `print()` and `WIIXL_LOG("my_mod: …")` go to `cemu/portable/log.txt` as
  `[OSConsole]` lines. `wxl log` shows only lines with the `my_mod: ` prefix,
  so engine logging must use it too.
- WiiXLaunch is fetched at commit `30e0502` and the patch is applied on top.
  If you change anything in `WiiXLaunch/`, regenerate the patch with
  `git -C WiiXLaunch diff >| tools/wiixlaunch-local.patch` (it currently
  matches that command exactly). If the host itself changed, run `./wxl host`
  to rebuild `pack/`.

## Working on the mod

**Adding to `botw.h`:**
1. Find an import in `WiiXLaunch/sdk/include/wiixlaunch/imports/<surface>.h`.
   The comments there are the framework authors' notes.
2. Bind it with `WXL_USE_<surface>(Name)` in `engine/engine.h`.
3. Add `botw.<surface>@<version>` to `require` in `mod/mod.json`. Line 4 of
   the header gives the version; a symbol added in a later minor version
   needs that version.
4. Implement it in `engine/game.cpp` or `screen.cpp`.
5. Declare it in `botw.h` with a plain-English comment that gives units
   (hearts, wheels, metres, frames) and points to an example.

**Examples** follow one format, so `tools/test/check_examples.py` and students
can rely on it:
- A header: `// Example: …`, what it does and its buttons, then "To use it"
  ending in `Then add this line inside every_frame():` with the call indented
  under it.
- Then "How it works" bullets that teach the C++ involved, "What's been
  tested" where it's partial, and "Ideas to try".
- Then `#include "botw.h"   // my_mod.cpp already has this line` and
  `// ---- copy from here ----`. Everything below that marker gets pasted.
- **Globals and function names must be unique across all examples.** Students
  paste several at once (`menu_open` vs `ability_open`).
- After adding or changing an example, add or update its row in the
  `mod/README.md` table (roughly simplest to most involved) and run
  `tools/test/check_examples.py`. That builds all of them pasted together and
  must report `no warnings`. It restores `my_mod.cpp` and rebuilds afterwards.
- `double_rupees.cpp` is the one that also adds a line inside
  `when_game_adds()`.

**Writing for the student:** use plain words, explain a C++ idea where it
first appears, and be honest about what was and wasn't tested in game. When a
bug teaches something, add it to "For the tutor" in `mod/README.md`.

**Engine code** uses `Engine::` CamelCase functions, `g_` globals and `k`
constants. Every game address is a named `constexpr` with a comment saying
where it came from: the symbol list, the framework's source, or a measurement.

**Keyboard references in the docs are out of date.** `botw.h`, `README.md`,
`mod/README.md` and `tools/profiles/` describe the old GamePad keyboard scheme
(A = E, X = Space, L3 = C, …). The user's own profile is now a Pro Controller
layout (below). Whether to switch the docs over is still undecided; refer to
buttons by name in new text.

## Testing in the game (macOS)

The helpers live in `tools/test/`. Run `tools/test/build.sh` once to compile
`keys`, `winid` and `strip` into `tools/test/bin/` (gitignored). The terminal
needs Accessibility permission (key events) and Screen Recording permission
(screenshots). Keep screenshots in the session scratchpad (`$SCRATCH` below),
not the repo.

```bash
tools/test/session.sh status          # is a test already running?
tools/test/session.sh start           # backs up the user's save + my_mod.cpp, puts the test save in place
#   ...put the test code in mod/my_mod.cpp (paste the example and its call line)...
./wxl build
tools/test/load.sh                    # quits Cemu, launches, taps A until in game (~30 s)
osascript -e 'tell application id "info.cemu.Cemu" to activate'    # keys go to the front app
tools/test/bin/keys "d:6 w:150 t:8 w:150 u:6"                      # hold ZL, tap B, let go
tools/test/shot.sh "$SCRATCH/after"   # Cemu's window -> after.png and after_s.png (Read the _s one)
./wxl log                             # what print() said
tools/test/session.sh end             # quits Cemu, puts back the save and my_mod.cpp, rebuilds
```

- **`session.sh`** refuses to `start` while Cemu runs, because it may be the
  user's game. `start` writes `backups/.test-session`, and `load.sh` won't run
  without that file. `end` must always run, even after a failure; if `status`
  says a test is running at the start of a session, ask the user before
  ending it. The test save is `backups/save-101c9400-20261012-104050`: early
  Great Plateau, before the tower, no runes. Use `TEST_SAVE=<name>` for
  another. Avoid the user's newest saves for tests: they hit the rendering
  glitch twice.
- **`keys`** tokens: `d:<key>` down, `u:<key>` up, `t:<key>` tap (120 ms),
  `w:<ms>` wait. The game runs at about 20 fps on this Mac, so hold keys for
  at least 100 ms. For a `held(X) && pressed(Y)` combo, press X down, wait,
  tap Y, wait, let X go.
- **`bin/strip out.png a.png b.png …`** stacks screenshots so you can compare
  them in one read.
- **Use `print()` to check a test.** `./wxl log TEXT` gives a reliable answer.
  Screenshots are for seeing what the user sees.

The user's binds, as a Wii U Pro Controller on the keyboard (macOS virtual
keycodes):

| Button | Key | Code | | Button | Key | Code |
|---|---|---|---|---|---|---|
| A | Space | 49 | | ZL | Z | 6 |
| B | C | 8 | | ZR | X | 7 |
| X | Left Command | 55 | | Plus | Return | 36 |
| Y | Left Option | 58 | | Minus | Delete | 51 |
| L | E | 14 | | D-pad ↑ ↓ ← → | arrows | 126 125 123 124 |
| R | R | 15 | | L3 / R3 | `,` / `.` | 43 / 47 |
| Left stick | W A S D | 13 0 1 2 | | Right stick ↑ ↓ ← → | T G F G | 17 5 3 5 |

G is bound to both right-stick down and right-stick right. In the game, the
D-pad opens BotW's quick menus (Down whistles), Minus opens the map and R3 the
Slate's scope. Use `hide_from_game()` when a mod takes over a button.

## Finding things out

- **What the framework offers:**
  - `WiiXLaunch/sdk/include/wiixlaunch/imports/*.h`: every import, with notes.
  - `WiiXLaunch/vendor/wiixlaunch-botw/include/wiixlaunch/botw/{surfaces,game}/*.hpp`:
    how each import works, with game offsets and structures (the path to
    Link's settings came from the armour code).
  - `WiiXLaunch/vendor/wiixlaunch-botw/docs/internal-ref/impl-progress.md`:
    what is implemented and what is still TODO.
- **Game addresses:** `WiiXLaunch/vendor/wiixlaunch-botw/data/symbols-wiiu-v208.csv`
  lists address, signature and notes. Hook with `Core::InstallHook(address, &fn)`,
  which returns the original function (keep it in a `volatile` pointer). Hooks
  on one function run first-installed-first.
- **What a function does:** the [zeldaret/botw](https://github.com/zeldaret/botw)
  decompilation. It targets Switch 1.5.0: names and structures carry over,
  addresses don't. Fetch single files with `curl` from raw.githubusercontent.com
  into the scratchpad.
  - [cemu_graphic_packs](https://github.com/cemu-project/cemu_graphic_packs)
    patches (e.g. FPS++ under `src/BreathOfTheWild/Mods/`) contain more v208
    addresses.
- **The game's data:** read it with `tools/botw_files.py` (`yaz0_decompress`,
  `sarc_files`, `msbt_texts`, `actor_names`, `english_names`). Run Python with
  `-I` and `sys.path.insert(0, "tools")`. Look in the update's content first,
  `cemu/portable/mlc01/usr/title/0005000e/101c9400/content`, then the base
  game's, `cemu/games/BotW/content`. The DLC is under `0005000c`.
  - What a cutscene sets: `Event/<name>.sbeventpack` → Yaz0 → SARC →
    `EventFlow/<name>.bfevfl`, then scan its strings. This is how
    `Demo003_0`, which sets the `IsGet_Obj_*` rune flags, was found.
  - Flag names: `Pack/Bootup.pack` → `GameData/gamedata.ssarc` →
    `bool_data` and `s32_data`.
  - Actors: `Actor/Pack/<name>.sbactorpack`, or inside `Pack/TitleBG.pack` for
    always-loaded actors such as `GameROMPlayer`. Link's numbers are in its
    `Actor/GeneralParamList/Player_Link.bgparamlist`.
  - English names: `Bootup_USen.pack` → `Message/Msg_USen.product.ssarc` →
    `ActorType/*.msbt`.
- **Saves:** `cemu/portable/mlc01/usr/save/00050000/101c9400/user/80000001/<slot>/game_data.sav`.
  The newest slot is the one with the latest modification time. From offset
  0x0C, the file is 8-byte big-endian records: `crc32(name)`, then the value.
  Diff a save from before and after something happens to find which flags
  changed.
- **Memory you can't find documented:** add a temporary function to
  `engine/game.cpp` that `print()`s `TEST …` lines with raw words, and call it
  from `my_mod.cpp` on a button combo. Read the results with `./wxl log TEST`,
  then remove the function. Check every pointer before following it
  (0x10000000 ≤ p < 0xA0000000, 4-byte aligned). The path to Link's settings
  was measured this way.
- **Actor names in the world:** [objmap.zeldamods.org](https://objmap.zeldamods.org/).
  Click a thing for its "Actor:" line, or search `actor:^"Enemy_"`.

## How the engine works (measured, v208 only)

- **When `every_frame()` runs:** from `botw.player`'s `RegisterTick`, after
  Link's update and before physics. `Engine::InGame()` gates it: Link exists,
  his max life is above 0 (the title screen has a Link with 0) and
  `World::TimeAvailable()`. So it never runs on the title screen, in loading
  screens or while paused. The framework logs `PlayerTick: my_mod ran` for
  its first three calls; in practice that means a save has loaded, and
  `load.sh` waits for it.
- **Each frame:** `BeginFrame` reads the buttons, events and weapons.
  `EndFrame` hands over one queued spawn, clears a stale `move_link` push and
  publishes the hidden-button mask. It also keeps equips and forced weather
  alive (`TickEquipRefresh`, `TickWeatherHold`) and re-applies a custom time
  speed; without those calls they silently never happen.
- **Drawing:** shapes are recorded during `every_frame()` and replayed in
  `botw.gui`'s frame callback, on a 1280×720 screen.
  - `StyleCreate` copies `Styles::Message` (centred vertically), so the
    engine calls `StyleAlign(id, 0, 0)` to get top-left text.
  - Menus: `draw_menu()` and `menu_up_down()` in `screen.cpp`.
- **Hiding buttons:** the engine hooks the controller reads before
  `Input::Init`, so `botw.input` still records the real buttons:
  - `0x030d9f24`, the GamePad (`VPADStatus` at `pads+0x14`).
  - `0x030da168`, the other controllers (`pads+0x1118+ch*0xF08`; a Pro
    Controller when byte `+0x5C == 0x1f`, its buttons at `+0x60`).

  The bit tables per button are in `main.cpp`. The mask covers the next 2
  reads. The D-pad was verified hidden on the GamePad and the Pro Controller,
  and so was R3.
- **The counter hook:** the game's add-to-counter function at `0x03204a14`
  is hooked to call `when_game_adds()`. The `gdt::Manager*` is at
  `*0x1046d5b0` and the `SafeString` vtable is at `0x10263910`. `AddRupees`
  doesn't go through it.
- **Spawning:**
  - `botw.actor` `Spawn` takes world coordinates. The engine works them out
    from Link's matrix: forward is `m[2], m[6], m[10]` and position is
    `m[3], m[7], m[11]`, plus 2 m up.
  - The framework holds one pending spawn at a time, so the engine queues 16
    and releases one per frame.
  - The framework's `IsValidActorName` rejects every string in mod memory
    (it wants pointers ≥ 0x10000000), so the engine uses `LooksLikeActorName`
    instead.
  - `spawn_scaled` uses `SpawnScaled` (the `@S` creation parameter) and only
    works at spawn time.
- **Movement:** `SetControllerVelocity` acts as a velocity on the ground but
  adds up every frame in the air (a steady 12 launched Link to 848 m).
  - `move_link` pushes `0.7 × (target − GetHavokVelocity)`, plus 1 on y.
  - Use `GetHavokVelocity` for Link's speed; `GetLinearVelocity` is useless
    for him.
  - Fall damage depends on the distance fallen (about 17 m is safe), not the
    landing speed.
- **Items:**
  - For a weapon, `AddItem`'s amount is its durability: Traveler's Sword 2000,
    Bow 2200, Wooden Shield 1200. Give 1 and it breaks on the first hit; the
    equipped copy starts at full life, which hid this bug.
  - A new weapon handle comes back on every lookup, so compare actor IDs
    (`TrackWeapon`).
  - `has_room_for` compares `botw.pouch` `SlotCount` (0 sword, 1 bow, 3 shield)
    with the s32 flags `WeaponPorchStockNum` / `BowPorchStockNum` /
    `ShieldPorchStockNum` (start 8 / 5 / 4). `SlotCount` updates in the same
    frame as `AddItem`.
  - Items given on the title screen are lost when the save loads.
- **Flags:**
  - `set_flag` takes effect on the next frame.
  - Runes are `IsGet_Obj_*` flags, not items: `AddItem` treats them as
    Material and silently fails.
  - On the early test save the runes never appear in the D-pad-Up menu, even
    after a save and reload. That's unresolved; the Sheikah Sensor HUD does
    work.
  - Champion abilities (`Obj_HeroSoul_*`) and the Paraglider (`PlayerStole2`)
    are key items as well as flags.
- **Link's settings:**
  - The path: actor`+0x39c` → `+0x78` GParamList → `+0x244` array, object
    `0x15` (Player).
  - Each record is `[crc32 name][next][type][value]`. Decimal numbers have
    the type word `0x1030ef08`, and the value is 12 bytes after the hash.
  - The Stasis times are 300/150/60 frames. Writes were verified by reading
    them back; Stasis itself is untested, because the saves have no rune.
- **Sound:** `botw.sound` `Play` reaches only 51 system events. The list is in
  `examples/button_sounds.cpp`; `botw.sound`'s `DumpEvents` writes them to the
  log.
- **Memory:** without `heapRequest` in `mod.json`, each mod took about 200 KB
  and the GUI's fonts no longer fitted.

## New items (measured)

- `wxl items` writes `Actor/Pack/<name>.sbactorpack`, `Actor/ActorInfo.product.sbyml`,
  every `Pack/Bootup_<lang>.pack`, `UI/StockItem/<name>.sbitemico` and the RSTB
  into `cemu/portable/graphicPacks/WiiXLaunch_BotW/content/`. Cemu reads them in
  place of the game's. They also apply to the user's own play.
- The Spawning Wand is the Ice Rod's pack with `AIProgramUser EquipItemBase` and
  `ASUser EquipItemCommon` (from `Weapon_Sword_001`), `WeaponSubType ""`, and the
  rod, cold and compendium tags dropped. It doesn't shoot. Files that change get
  the new name, because the game caches files by path.
- Name and description go in `ActorType/WeaponSmallSword.msbt` (`<name>_Name`,
  `<name>_Desc`). The icon is the Ice Rod's.
- RSTB: ActorInfo has no entry. A new file gets the entry of the file it was
  copied from, plus its growth and 0x400. The message archive's entry is its
  decompressed size + 408.
- The model list's `MulColor` / `AddColor` made no visible difference to the Ice
  Rod. `BaseScale` is set to 1.1.
- The writers round-trip the game's files: SARC, BYML and RSTB exactly, AAMP by
  value (the layout differs from Nintendo's).
- `sword_drawn()` has no import behind it: it goes by where the sword is in
  Link's frame. On his back (put away, or bow out) it's 1.3–1.5 m up, within
  0.3 m sideways and not forward. In hand it's 0.4–1.0 m up, or off to the
  side or in front mid-swing. It takes 6 frames to count as put away and 2 to
  count as drawn. Climbing, swimming and gliding weren't measured. The test
  save's start is near cliffs: sprinting or walking there killed Link twice.
- `swung()` is the framework's attack counter. Taking a weapon out with the
  first press of Y doesn't count as a swing.

## Known issues and open threads

- **Garbled 3D geometry on Metal** happens while Cemu compiles new shaders.
  Running the same thing again renders fine.
- **The clock stays at 11:00** early on the Great Plateau. The game pins it;
  it isn't a bug in the mod.
- **Bullet time** has no import.
  - FPS++ gives `0x02D5F760`, the arrow-time stamina drain (`lfs f1, 0xFC(r11)`),
    as a starting point. The decompilation slows time with
    `ksys::VFR::setTimeMultiplier`.
  - Hovering while aiming didn't trigger it.
  - The user hasn't chosen between a "hang in the air" button and real
    reverse engineering.
- **Runes on early saves:** the next step is to diff a save from before and
  after the first shrine.
- **Key binds in the docs:** see "Working on the mod".
- **Remote:** `origin` is on GitHub over SSH. `gh` isn't logged in.

## Shell gotchas

- The user's zsh aliases `cp` and `rm` to their `-i` forms, so use
  `command cp -f` and `command rm -f`.
- `noclobber` is on, so overwrite a file with `>|`.
- Cemu's process is `Cemu.app/Contents/MacOS/Cemu` (`pgrep -f`). Its bundle
  id is `info.cemu.Cemu`.
