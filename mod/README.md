# Your BotW mod

[`my_mod.cpp`](my_mod.cpp) is your mod: the one file you edit. The
[`examples/`](examples/) folder has ready-made pieces to copy into it, such as
infinite stamina, a HUD, a jetpack, or spawning enemies and the Giant Horse.
Examples aren't part of the mod until you paste them in.

## The files

| File | What it's for |
|---|---|
| [`my_mod.cpp`](my_mod.cpp) | **Your mod.** The file you edit |
| [`examples/`](examples/) | Pieces to copy from. These aren't built into the mod |
| [`botw.h`](botw.h) | Everything you can ask the game to do, with an explanation of each |
| `engine/` | Behind the scenes. You don't need to open it until you're curious |
| `mod.json` | Build settings. `wxl build` keeps its file list up to date |

## Build, play, repeat

```text
wxl build     turn my_mod.cpp into a mod and install it
wxl play      start the game
wxl log       read back what print() said last time you played
wxl actors    look up the game's names for things, to spawn them
```

(`./wxl` on macOS, `wxl` on Windows, typed in the top folder of the workspace.)

The game loads mods when it starts. **After each `wxl build`, close the game
and run `wxl play` again.**

## Using an example

1. Open an example, such as `examples/infinite_stamina.cpp`.
2. Copy everything below the line `// ---- copy from here ----`.
3. Paste it into `my_mod.cpp`, above `every_frame()`. There's a "Paste examples
   here" spot.
4. Inside `every_frame()`, add the line the example names. That's what makes it
   run.
5. `wxl build`, then restart the game.

Afterwards, `my_mod.cpp` looks like this:

```cpp
void infinite_stamina() {          // pasted from examples/infinite_stamina.cpp
    if (stamina() < max_stamina()) {
        set_stamina(max_stamina());
    }
}

void every_frame() {
    infinite_stamina();            // the line that makes it run
}
```

You can paste in as many examples as you like; their names don't clash. To
switch one off, put `//` at the start of its line in `every_frame()`.
`double_rupees.cpp` needs one more line, in `when_game_adds()`. Its file says
where.

## The examples

Roughly from simplest to most involved. Each file explains how it works and
ends its header with **Ideas to try**.

| Example | What it does | Buttons | Shows you |
|---|---|---|---|
| `hello.cpp` | Says hello, then reports every 10 seconds | — | variables, `if`, `print()` |
| `button_sounds.cpp` | Clicks every time you press A, B, X or Y | A, B, X, Y | sounds, `\|\|` (or) |
| `infinite_stamina.cpp` | Stamina never runs out | sprint | reading and changing game values |
| `hud.cpp` | A panel with the time, weather, hearts and rupees | — | drawing, screen positions, `%d` and `%.1f` |
| `giant_horse.cpp` | Brings the Giant Horse | ZL + B | spawning, actor names |
| `master_cycle.cpp` | Brings the Master Cycle Zero motorbike | ZL + ZR | spawning |
| `time_warp.cpp` | Speeds up time, skips hours, makes it rain | ZL + D-pad | `held()` vs `pressed()`, `&&`, `%` |
| `starter_kit.cpp` | Gives and equips a sword, bow, shield and arrows | ZL + R | item names, a function with parameters |
| `spawn_enemies.cpp` | Spawns a random enemy in front of Link | ZL + Y | arrays of text, `random_number()` |
| `spawn_rocks.cpp` | Drops a row of five boulders | ZL + A | `for` loops |
| `unbreakable_weapons.cpp` | Weapons never wear out | — | remembering values, arrays |
| `exploration_rewards.cpp` | Rupees for Korok seeds, shrines and towers | — | reacting to events |
| `slate_abilities.cpp` | Unlocks the runes, Camera and Sheikah Sensor (only partly works yet; see the file) | ZL + L | game flags |
| `jetpack.cpp` | Fly up to 15 m, then float down on a parachute | hold ZL + X | physics, game rules |
| `double_rupees.cpp` | Every rupee you pick up counts twice | ZL + Minus tests it | hooks: the game calls your code |

On the keyboard, **ZL is the Z key**. A is E, B is Left Shift, X is Space, Y is
F, L is Q, R is R, ZR is V, Minus is M and the D-pad is the arrow keys. The top-level README has
the full layout.

All examples were pasted into one `my_mod.cpp` together, following their own
instructions, and built with no warnings. Every one except
`exploration_rewards.cpp` was tried in the game; that one builds and loads, but
no Korok, shrine or tower was reached in testing. `slate_abilities.cpp` only
partly works so far: the Sheikah Sensor appears, the runes don't (details in
the file). `double_rupees.cpp` was tried
with its ZL + Minus test rather than a real rupee.

## Finding actor names

Everything in the world is an "actor" with an internal name:
`Enemy_Bokoblin_Junior` is a Bokoblin, and `GameRomHorse00L` is the Giant Horse.
`spawn()` and `give_item()` take these names.

- **`wxl actors`** lists every actor in your copy of the game (about 6,300),
  next to its English name where the game has one. Add a word to search both
  columns:

  ```text
  wxl actors hinox
  Enemy_Giant_Junior                           Hinox
  Enemy_Giant_Middle                           Blue Hinox
  Enemy_Giant_Senior                           Black Hinox
  ...
  ```

  It reads the names straight from your game files, so it always matches your
  version. Things the game never names on screen, such as rocks, show just
  the internal name.
- **[The BotW object map](https://objmap.zeldamods.org/)** shows everything the
  game places in the world. Click something to see its internal name (the
  "Actor:" line), or search, e.g. `actor:^"Enemy_Lynel"`. Use it to answer "what's
  the name of that thing I can see in the game?"

Not every name spawns something useful. Enemies, the Giant Horse and boulders
were tried here, and the framework's authors report weapons, animals and NPCs
working too. Armour doesn't work: it appears but can't be moved or removed. And
many names are invisible helpers the game uses internally.

## When something goes wrong

- **The build fails.** Read the *first* error. It names the file and line, e.g.
  `my_mod.cpp:21:5: error: ...`. Common ones:
  - `expected ';'`: a line is missing its `;` at the end.
  - `'xyz' was not declared in this scope`: a typo in a name; or the call is in
    `every_frame()` but the example wasn't pasted, or it was pasted *below*
    `every_frame()`.
  - `redefinition of 'xyz'`: the same example was pasted twice, or two things
    share a name.
  - `spawn: "xyz" isn't an actor name`: there's a space or another character
    that isn't a letter, digit or underscore. Copy names exactly from
    `wxl actors`. (What a misspelled but well-formed name does hasn't been
    tested.)
  - `references 1 symbol(s) it does not define` followed by
    `_Z11every_framev` or `_Z14when_game_addsPKci`: `every_frame()` or
    `when_game_adds()` was deleted or renamed. Both have to stay in `my_mod.cpp`.
- **A warning about `format '%d'`**: the number is a decimal (a `float`), so
  use `%.1f` instead of `%d`.
- **Nothing happens in the game.** Did you restart the game after building? Is
  the example's line inside `every_frame()`? Your code only runs while you're
  playing: not on the title screen, and not while the game is paused.
- **`wxl log`** shows everything `print()` said, and any errors the mod loader
  hit.

## For the tutor: stories from building these

Each of these really happened while making the examples. Most make a good
"why doesn't this work?" discussion.

1. **The jetpack launched Link 600 m into the sky.** The first version wrote an
   upward push of "12" every frame, as the physics notes suggested. On the
   ground that works as a speed. In the air, though, each frame's push *adds*
   to Link's speed, so after one second he was doing 70 m/s and kept coasting
   up for hundreds of metres after the button was released. At 848 m the cold
   started hurting him, and the landing killed him. The fix (`engine/game.cpp`,
   `move_link`) reads Link's real speed from the physics engine and pushes
   only by the difference. Measure before you trust a description.
2. **A slow parachute landing still hurt.** Landing at 3 m/s from 25 m cost
   1.75 hearts, and from 32 m it left Link with a quarter of a heart. BotW
   works out fall damage from how far Link fell, not from how fast he landed.
   That's why the jetpack has a height limit (falls of about 17 m did no
   damage).
3. **Starter-kit weapons broke on the first hit.** The first test gave weapons
   with amounts like 2200 and read their durability straight back: always the
   game's own full value (a Traveler's Sword read 2000), whatever was passed. So
   the kit was changed to pass 1. Wrong: the amount IS the durability, stored on
   the weapon in the inventory. The test only ever looked at the weapon in
   Link's hand, which the game had just created at full strength. Spare copies
   came out with 1 durability, which a student found by playing. Check the
   thing the player will actually see.
4. **The clock stuck at 11:00** (`time_warp.cpp`). That wasn't the mod. On the
   Great Plateau, BotW pins its own clock at 11:00; "skip 3 hours" briefly
   reads 14:00, and then the game puts it back. Always check the game without
   your mod before blaming your code.
5. **"Seconds" ran slow.** `hello.cpp` counts frames, and on the test Mac the
   game ran at about 20 frames a second, so "10 seconds" took 14. A frame count
   is only a clock while the game keeps up.
6. **Minus opens the map** (`double_rupees.cpp`'s test button). Mods see button
   presses, but so does the game.
7. **Every spawn was refused as "not an actor name"**, even
   `Enemy_Bokoblin_Junior`. The framework's name check also tests that the text
   lives in the game's part of memory, and a mod's own text never does. The
   engine now checks names itself (`LooksLikeActorName` in `engine/game.cpp`).
8. **Runes aren't items.** Giving `Obj_Magnetglove` with `give_item()` did
   nothing: the game counts it as a "material" and refuses it. The game's own
   rune cutscene (`Demo003_0` in the game files) just switches on flags like
   `IsGet_Obj_Magnetglove`. Setting those from the mod works, but a flag change
   only lands on the next frame (reading it straight back says "off"). On a save
   from before the Great Plateau tower, the Sheikah Sensor appeared, but the
   runes still didn't, even after a save and reload, so the game wants
   something more that early on.
9. **The Giant Horse fell off a cliff.** It spawned 8 m ahead of Link, who was
   standing on a hilltop, which was past the edge. Spawning is just a position;
   the game's physics decides where things end up.

These are now handled inside the engine, so the examples don't have to:

10. **Only one spawn at a time.** The framework holds a single spawn request and
   refuses the next until the game has created the first. `spawn()` queues
   requests and hands over one per frame, so five boulders in one frame all
   arrive.
11. **Items given on the title screen vanished.** A pouch exists there, so adding
   items "works", and then loading the save replaces it. The title screen even
   has a Link, but his max life is 0. `Engine::InGame()` checks that before
   running `every_frame()`.
12. **"New weapon!" printed every frame** (`unbreakable_weapons.cpp`). Each
   lookup of the equipped sword hands out a new handle, so handles can't be
   compared; the actor's ID can. While an equip settles, the lookup also
   flickers empty every other frame. See `TrackWeapon` in `engine/game.cpp`.
13. **Equips and forced weather silently never happened**, because something has
   to call the game's "keep going" functions every frame (`EndFrame` in
   `engine/game.cpp`).
14. **Every mod grabbed ~200 KB of memory** and the GUI's fonts no longer fit,
    until each mod stated `heapRequest` in `mod.json`.
15. **ZL was on Tab**, which Cemu uses to show the GamePad screen, so it moved
    to Z.

## Behind the curtain

When the student is ready to see how `botw.h` works:

- `engine/game.cpp`: each friendly function, built from WiiXLaunch "imports".
- `engine/main.cpp`: where the game first enters the mod, and the counter hook
  (a game function redirected at a fixed address).
- `engine/screen.cpp`: how `print()` and `draw_text()` reach the screen.
- `WiiXLaunch/sdk/include/wiixlaunch/imports/*.h`: every import there is, with
  the framework authors' notes.
- `WiiXLaunch/vendor/wiixlaunch-botw/data/symbols-wiiu-v208.csv`:
  reverse-engineered game addresses, for writing more hooks.
- [zeldaret/botw](https://github.com/zeldaret/botw): the decompiled game, for
  working out what a function does before hooking it. It targets the Switch
  version 1.5.0, so names carry over to the Wii U but addresses don't.
