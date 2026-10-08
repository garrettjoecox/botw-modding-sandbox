# BotW code modding (Cemu + WiiXLaunch) for macOS and Windows

> **Educational use only.** This repository is a teaching resource for learning
> game modding, reverse engineering and C++. It is not intended for any other
> purpose. It contains no game files or Nintendo assets and does not help anyone
> obtain them; you need your own legally dumped copy of the game. Not affiliated
> with or endorsed by Nintendo.

Write C++ mods for *The Legend of Zelda: Breath of the Wild* (Wii U v208) and run
them in Cemu. The lessons are in [`mods/README.md`](mods/README.md).

Everything goes through one script: `./wxl` on macOS, `wxl` (i.e. `wxl.cmd`) on
Windows.

```text
wxl setup [--base DIR] [--update DIR] [--dlc DIR]   first-time install
wxl build [MOD ...]                                  compile mods (default: all)
wxl play                                             launch the game
wxl log [TEXT]                                       mod output from the last run
wxl doctor                                           what's installed / missing
wxl host                                             rebuild the framework core (rarely)
```

Mods load at boot, so restart the game after `wxl build`.

## What you need

| | Windows | macOS |
|---|---|---|
| Python 3 | [python.org](https://www.python.org/downloads/) (tick "Add to PATH") | `brew install python` |
| Git | [Git for Windows](https://git-scm.com/download/win) | built in |
| Compiler | [devkitPro installer](https://github.com/devkitPro/installer/releases), select **devkitPPC** (or Docker Desktop) | Docker Desktop (or devkitPro) |
| GPU | Vulkan-capable | Apple Silicon or Intel |
| The game | **Your own** Wii U dump of BotW: base game + update v208 (+ DLC, optional), as decrypted `code/` `content/` `meta/` folders | same |

`wxl` uses an installed devkitPro if it finds one (`C:\devkitPro`,
`/opt/devkitpro`, or `DEVKITPPC`); otherwise it builds inside Docker. Force either
with `--native` / `--docker`.

## First-time setup

```bat
git clone <this workspace> botw
cd botw
wxl setup --base "D:\dumps\BotW" --update "D:\dumps\BotW Update" --dlc "D:\dumps\BotW DLC"
wxl build
wxl play
```

`setup` does the following:
- Downloads Cemu: **2.6 stable on Windows** (Vulkan), or **the latest main-branch
  build on macOS** (native Apple Silicon, Metal).
- Writes Cemu's portable config into `cemu/portable/`.
- Installs the prebuilt WiiXLaunch graphic pack from `pack/`.
- Fetches the WiiXLaunch framework at a pinned commit and applies
  `tools/wiixlaunch-local.patch`.
- Copies your game files into place.

Run `wxl doctor` any time to see what's missing. You can point the dump flags at
any folder that contains the `code/content/meta` folders somewhere inside it.

## Layout

| Path | What | Shared? |
|---|---|---|
| `mods/<name>/` | The lessons: `mod.cpp` + `mod.json` | yes |
| `pack/WiiXLaunch_BotW/` | Prebuilt host graphic pack (same file on every OS) | yes |
| `tools/wxl.py` | The workspace script | yes |
| `tools/profiles/` | Keyboard profiles; Cemu stores OS-specific key codes | yes |
| `tools/wiixlaunch-local.patch` | Local changes to WiiXLaunch (see below) | yes |
| `tools/docker/` | devkitPPC + Pillow image, used when there's no native devkitPro | yes |
| `cemu/` | Cemu, its config, your game files and saves | **no** |
| `WiiXLaunch/` | Framework checkout, fetched by `setup` | no |

`.gitignore` keeps game files, saves, Cemu and build output out of git. Never
commit or send a copy of the game; each person needs their own dump.

## Keyboard

The same physical layout on both OSes. Remap under Cemu's Options > Input settings,
or plug in a controller.

| Key | Button | | Key | Button |
|---|---|---|---|---|
| W A S D | Left stick | | I J K L | Right stick (camera) |
| E | A | | Left Shift | B |
| Space | X (jump) | | F | Y (attack) |
| Q | L | | R | R |
| Z | ZL | | V | ZR |
| Enter | + | | M | − |
| Arrows | D-pad | | C / B | L3 / R3 |

Avoid binding Tab: Cemu shows the GamePad screen while it's held.

## Local changes to WiiXLaunch

`tools/wiixlaunch-local.patch`, applied by `setup`:
- `src/main.cpp`: moves host allocations onto coreinit's MEM2 heap once graphics
  are up (otherwise the GUI's fonts don't fit in the ~3 MB code cave), and turns
  off the logo demo (it covered BotW's hearts).
- The generated import headers and `sdk/` scripts, refreshed with
  `scripts/gen_imports.py` / `make_sdk.py` (upstream had drifted).

## Known issues

- **Occasional garbled 3D geometry after loading a save** on macOS/Metal (UI still
  draws). Seen in 2 of ~12 loads; reloading cleared it. The cause isn't established.
  The suspects are Metal itself and the GUI's memory coming from the game's MEM2
  heap.
- BotW pins the clock at 11:00 at the start of the Great Plateau, so time mods look
  broken there.

## What has been tested where

- **macOS, Cemu nightly + Metal:** everything, in game (see `mods/README.md`).
- **The Windows configuration** (Cemu 2.6, Vulkan, generated config, all nine mods)
  was run on a Mac using Cemu 2.6's Mac build. The pack applied, 9/9 mods loaded,
  and the HUD drew.
- **The Windows-only code in `wxl.py`** (Cemu zip install, Windows paths in
  `settings.xml`, Docker builds through a `/work` mount) was exercised on the Mac.
- **Not yet run on a real Windows machine:** `wxl.cmd`, the Windows keyboard
  key codes, and building with a native Windows devkitPro.

## License

GPL-3.0 (see `LICENSE`), matching [WiiXLaunch](https://github.com/BladesawStudios/WiiXLaunch):
the prebuilt `pack/` contains compiled WiiXLaunch code, and the mods build
against its SDK. No game files or Nintendo assets are included.
