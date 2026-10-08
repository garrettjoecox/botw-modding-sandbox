// Lesson 2 - stamina.wxlm : infinite stamina
//
// Read a value from the game every frame and write it back.
//
// Concepts:
//   * Game state lives in GameData "flags" - named, save-backed variables.
//     botw.gamedata wraps the ones the framework has reverse-engineered.
//   * Units are the game's, not yours: stamina is in "wheel units", where
//     1000 = one full wheel.
//   * Guard every call. Before a save is loaded there is no stamina, and the
//     getters return 0 (false) - never assume the world exists.
//
// Try: only refill while standing still, or cap stamina at half a wheel.

#include <cstdint>

#include <wiixlaunch/imports/botw_gamedata.h>
#include <wiixlaunch/imports/wiixl_core.h>
#include <wiixlaunch/mod_log.h>
#include <wiixlaunch/mod_runtime.h>

namespace Core {
WXL_USE_wiixl_core(RegisterTick);
}

namespace GameData {
WXL_USE_botw_gamedata(GetStamina);
WXL_USE_botw_gamedata(GetMaxStamina);
WXL_USE_botw_gamedata(SetStamina);
}

namespace {

uint32_t g_Refills;

void Tick() {
    float current = 0.0f, max = 0.0f;
    if (!GameData::GetStamina(&current) || !GameData::GetMaxStamina(&max)) return;
    if (max <= 0.0f || current >= max) return;

    GameData::SetStamina(max);

    // Log the first few refills so you can see it working, then go quiet -
    // logging every frame would flood the log at 30 lines a second.
    if (++g_Refills <= 3) {
        WIIXL_LOG("stamina: refilled %.0f -> %.0f (1000 = one wheel)", current, max);
    }
}

} // namespace

extern "C" __attribute__((used)) void WiiXLaunch_ModEntry() {
    if (Core::RegisterTick(&Tick)) WIIXL_LOG("stamina: loaded - stamina never runs out");
}
