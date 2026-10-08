// Lesson 1 - hello.wxlm
//
// The smallest useful mod: prove that your code is inside the game.
//
// Concepts:
//   * WiiXLaunch_ModEntry - called once, when the loader reaches this mod.
//   * Imports - the host exposes "surfaces" (named groups of functions).
//     WXL_USE_<surface>(Symbol) binds one symbol; only bound symbols are
//     imported, and the loader patches their addresses in at load time.
//   * Ticks - Core::RegisterTick gives you a regular per-frame callback.
//   * Logging - WIIXL_LOG lands in cemu/portable/log.txt as [OSConsole] lines.
//
// Try: change the interval, or log something only on the very first tick.

#include <cstdint>

#include <wiixlaunch/imports/wiixl_core.h>
#include <wiixlaunch/mod_log.h>
#include <wiixlaunch/mod_runtime.h>

namespace Core {
WXL_USE_wiixl_core(RegisterTick);
}

namespace {

// Globals live in the mod's .bss and start at zero, like any C++ program.
uint32_t g_Frames;

void Tick() {
    ++g_Frames;
    // Measured: this fires ~60 times a second even though BotW renders at
    // 30 fps - the tick rides the GX2 present, and BotW presents two screens
    // (TV and GamePad) per frame. So 600 ticks is about ten seconds.
    if (g_Frames % 600 == 0) {
        WIIXL_LOG("hello: still here - %u ticks since load", g_Frames);
    }
}

} // namespace

extern "C" __attribute__((used)) void WiiXLaunch_ModEntry() {
    WIIXL_LOG("hello: loaded!");
    if (!Core::RegisterTick(&Tick)) WIIXL_LOG("hello: the host refused a tick");
}
