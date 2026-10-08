// Lesson 3 - hud.wxlm : draw your own on-screen panel
//
// Put information on screen using the game's own fonts and UI art.
//
// Concepts:
//   * Frame callbacks vs. ticks - botw.gui calls you once per frame *while it
//     is building the UI*. Drawing calls only work inside that callback.
//   * Layout space is always 1280 x 720, origin top-left, whatever resolution
//     Cemu actually renders at.
//   * Assets stream in - fonts take a moment to load after boot. Until
//     IsReady() is true, text silently draws nothing.
//   * Styles - a style id bundles font size, colour and shadow. Create it
//     once, use it every frame.
//   * Colours are 0xRRGGBBAA.
//
// Try: hide the panel with a button press (botw.gui Pressed), colour the
// hearts line red when low, or add the player's position (botw.player).

#include <cstdarg>
#include <cstdint>

#include <wiixlaunch/format.hpp>
#include <wiixlaunch/imports/botw_gamedata.h>
#include <wiixlaunch/imports/botw_gui.h>
#include <wiixlaunch/imports/botw_player.h>
#include <wiixlaunch/imports/botw_world.h>
#include <wiixlaunch/mod_log.h>
#include <wiixlaunch/mod_runtime.h>

namespace Gui {
WXL_USE_botw_gui(Init);
WXL_USE_botw_gui(IsReady);
WXL_USE_botw_gui(RegisterFrame);
WXL_USE_botw_gui(StyleCreate);
WXL_USE_botw_gui(StyleScale);
WXL_USE_botw_gui(StyleColor);
WXL_USE_botw_gui(StyleShadow);
WXL_USE_botw_gui(RoundedBox);
WXL_USE_botw_gui(Text);
}

namespace World {
WXL_USE_botw_world(TimeAvailable);
WXL_USE_botw_world(GetGameTimeHM);
WXL_USE_botw_world(GetWeather);
WXL_USE_botw_world(WeatherName);
WXL_USE_botw_world(GetTemperature);
}

namespace Player {
WXL_USE_botw_player(GetPlayerActor);
WXL_USE_botw_player(ActorIsValid);
WXL_USE_botw_player(ActorGetLife);
WXL_USE_botw_player(ActorGetMaxLife);
}

namespace GameData {
WXL_USE_botw_gamedata(GetRupees);
WXL_USE_botw_gamedata(GetStamina);
}

namespace {

uint32_t g_Style;
bool g_SaidReady;

// printf-style formatting into a buffer. There is no libc here; the
// framework's formatter supports %d %u %s %f (with precision) and widths.
void Format(char* out, uint32_t cap, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    const uint32_t n = WiiXLaunch::Debug::FormatText(out, cap - 1, fmt, args);
    va_end(args);
    out[n < cap ? n : cap - 1] = '\0';
}

// Bottom-left: the one corner BotW's own HUD leaves empty (hearts and
// stamina top-left, messages top-right, minimap bottom-right).
constexpr float kPanelX = 20.0f;
constexpr float kPanelY = 550.0f;

void Line(int row, const char* text) {
    Gui::Text(kPanelX + 20.0f, kPanelY + 10.0f + 30.0f * row, text, g_Style);
}

void Frame() {
    if (!Gui::IsReady()) return;  // fonts still streaming in
    if (!g_SaidReady) {
        g_SaidReady = true;
        WIIXL_LOG("hud: GUI assets ready, drawing");
    }
    if (!World::TimeAvailable()) return;  // title screen / loading: no world to report

    if (!g_Style) {
        g_Style = Gui::StyleCreate();
        Gui::StyleScale(g_Style, 0.75f);
        Gui::StyleColor(g_Style, 0xFFFCC6FF, 0xFFFCC6FF);  // the game's cream text
        Gui::StyleShadow(g_Style, 1, 0x00000080, 2.0f, 2.0f);
    }

    Gui::RoundedBox(kPanelX, kPanelY, 320.0f, 150.0f, 0x000000A0, 12.0f);

    char line[64];
    int32_t h = 0, m = 0;
    World::GetGameTimeHM(&h, &m);
    float celsius = 0.0f;
    World::GetTemperature(&celsius);
    Format(line, sizeof(line), "%02d:%02d   %.0f C", h, m, celsius);
    Line(0, line);

    int32_t weather = 0;
    char weatherName[24] = "?";
    if (World::GetWeather(&weather)) World::WeatherName(weather, weatherName, sizeof(weatherName));
    Format(line, sizeof(line), "Weather: %s", weatherName);
    Line(1, line);

    const uint32_t link = Player::GetPlayerActor();
    if (link && Player::ActorIsValid(link)) {
        // Life is stored in quarter hearts.
        const int32_t life = Player::ActorGetLife(link);
        const int32_t max = Player::ActorGetMaxLife(link);
        if (max > 0) {
            Format(line, sizeof(line), "Hearts: %.2f / %d", life / 4.0f, max / 4);
            Line(2, line);
        }
    }

    int32_t rupees = 0;
    float stamina = 0.0f;
    GameData::GetRupees(&rupees);
    GameData::GetStamina(&stamina);
    Format(line, sizeof(line), "Rupees: %d   Stamina: %.1f", rupees, stamina / 1000.0f);
    Line(3, line);
}

} // namespace

extern "C" __attribute__((used)) void WiiXLaunch_ModEntry() {
    Gui::Init();
    if (Gui::RegisterFrame(&Frame)) WIIXL_LOG("hud: loaded - panel in the bottom-left corner");
}
