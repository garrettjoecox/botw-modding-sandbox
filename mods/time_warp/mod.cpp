// Lesson 4 - time_warp.wxlm : bend BotW's clock and weather from the controller.
//
// Hold ZL and tap the D-pad:
//   ZL + Up     cycle time speed  x1 -> x8 -> x32 -> x128 -> x1
//   ZL + Left   skip ahead three hours
//   ZL + Right  force the next weather type (held until released)
//   ZL + Down   back to normal: x1 speed, game-controlled weather
//
// Lesson: the game can override you. On the Great Plateau, before the story
// moves on, BotW itself stops the clock at 11:00 - measured with no mods
// pressing anything. Speed and skips appear to "not work" there; the log says
// "time flowing" because the world flag is set, yet the clock is pinned.
//
// ZL is lock-on, which does nothing harmful on its own, and quick D-pad taps
// only open BotW's quick-select menus when held, so the combos stay out of the
// game's way.

#include <cstdint>

#include <wiixlaunch/imports/botw_input.h>
#include <wiixlaunch/imports/botw_world.h>
#include <wiixlaunch/imports/wiixl_core.h>
#include <wiixlaunch/mod_log.h>
#include <wiixlaunch/mod_runtime.h>

namespace Core {
WXL_USE_wiixl_core(RegisterTick);
}

namespace Input {
WXL_USE_botw_input(Init);
WXL_USE_botw_input(HeldButtons);
WXL_USE_botw_input(MaskFor);
}

namespace World {
WXL_USE_botw_world(TimeAvailable);
WXL_USE_botw_world(WeatherAvailable);
WXL_USE_botw_world(GetGameTimeHM);
WXL_USE_botw_world(GetGameTime);
WXL_USE_botw_world(SetGameTime);
WXL_USE_botw_world(GetTimeScale);
WXL_USE_botw_world(IsTimeFlowing);
WXL_USE_botw_world(SetTimeScale);
WXL_USE_botw_world(GetWeather);
WXL_USE_botw_world(WeatherTypeCount);
WXL_USE_botw_world(WeatherName);
WXL_USE_botw_world(HoldWeather);
WXL_USE_botw_world(TickWeatherHold);
WXL_USE_botw_world(ReleaseWeather);
}

namespace {

// WiiXLaunch::BotW::Button enumerators (botw/game/controller.hpp).
constexpr int32_t kZL = 8;
constexpr int32_t kDLeft = 12;
constexpr int32_t kDUp = 13;
constexpr int32_t kDRight = 14;
constexpr int32_t kDDown = 15;

constexpr float kSpeeds[] = {1.0f, 8.0f, 32.0f, 128.0f};
constexpr uint32_t kSpeedCount = sizeof(kSpeeds) / sizeof(kSpeeds[0]);

uint32_t g_PrevHeld;
uint32_t g_SpeedIndex;
bool g_HoldingWeather;
bool g_WorldSeen;
uint32_t g_Frame;

void LogClock(const char* what) {
    int32_t h = 0, m = 0;
    float scale = 0.0f;
    World::GetGameTimeHM(&h, &m);
    World::GetTimeScale(&scale);
    // Cutscenes, the title screen and the opening in the Shrine of Resurrection
    // hold the clock still; the speed only shows once time is flowing.
    WIIXL_LOG("time_warp: %s - clock %02d:%02d, speed x%.0f, time %s", what, h, m, scale,
              World::IsTimeFlowing() ? "flowing" : "frozen by the game");
}

void LogWeather(const char* what, int32_t type) {
    char name[32] = {};
    World::WeatherName(type, name, sizeof(name));
    WIIXL_LOG("time_warp: %s - weather %d (%s)", what, type, name);
}

void ApplySpeed() {
    World::SetTimeScale(kSpeeds[g_SpeedIndex]);
}

void Tick() {
    ++g_Frame;
    if (!World::TimeAvailable()) {
        g_WorldSeen = false;
        return;  // title screen, loading screen
    }

    if (!g_WorldSeen) {
        g_WorldSeen = true;
        LogClock("world is up");
        if (World::WeatherAvailable()) {
            int32_t w = 0;
            if (World::GetWeather(&w)) LogWeather("world is up", w);
        }
    }

    const uint32_t held = Input::HeldButtons();
    const uint32_t pressed = held & ~g_PrevHeld;
    g_PrevHeld = held;

    if (held & Input::MaskFor(kZL)) {
        if (pressed & Input::MaskFor(kDUp)) {
            g_SpeedIndex = (g_SpeedIndex + 1) % kSpeedCount;
            ApplySpeed();
            LogClock("speed changed");
        }
        if (pressed & Input::MaskFor(kDLeft)) {
            float hours = 0.0f;
            if (World::GetGameTime(&hours)) {
                hours += 3.0f;
                if (hours >= 24.0f) hours -= 24.0f;
                World::SetGameTime(hours);
                LogClock("skipped 3h");
            }
        }
        if (pressed & Input::MaskFor(kDRight) && World::WeatherAvailable()) {
            int32_t cur = 0;
            World::GetWeather(&cur);
            const int32_t count = World::WeatherTypeCount();
            const int32_t next = count > 0 ? (cur + 1) % count : 0;
            World::HoldWeather(next, 1);
            g_HoldingWeather = true;
            LogWeather("forced", next);
        }
        if (pressed & Input::MaskFor(kDDown)) {
            g_SpeedIndex = 0;
            ApplySpeed();
            if (g_HoldingWeather) {
                World::ReleaseWeather();
                g_HoldingWeather = false;
            }
            LogClock("reset");
        }
    }

    // The game rewrites the scale on some transitions (loading zones, cutscenes),
    // so reassert a non-default speed every 15 ticks (a few times a second).
    if (g_SpeedIndex != 0 && (g_Frame % 15) == 0) {
        float scale = 0.0f;
        if (World::GetTimeScale(&scale) && scale != kSpeeds[g_SpeedIndex]) ApplySpeed();
    }

    // A hold is only re-applied while someone ticks it.
    if (g_HoldingWeather) World::TickWeatherHold();
}

} // namespace

extern "C" __attribute__((used)) void WiiXLaunch_ModEntry() {
    Input::Init();
    if (Core::RegisterTick(&Tick)) {
        WIIXL_LOG("time_warp: loaded - hold ZL + D-pad (Up speed, Left +3h, Right weather, Down reset)");
    } else {
        WIIXL_LOG("time_warp: RegisterTick refused");
    }
}
