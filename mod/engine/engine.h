// engine.h - shared by the engine's own files. my_mod.cpp never includes this.
//
// The engine turns botw.h's simple functions into WiiXLaunch calls. Everything
// a .wxlm can reach in the game is an "import": a function the WiiXLaunch host
// publishes under a surface name (botw.player, botw.gui, ...). WXL_USE_<surface>
// binds one of them; the loader fills in its address when the mod loads.
// The headers for every surface are in WiiXLaunch/sdk/include/wiixlaunch/imports/.
#pragma once

#include <cstdarg>
#include <cstdint>

#include <wiixlaunch/format.hpp>
#include <wiixlaunch/imports/botw_actor.h>
#include <wiixlaunch/imports/botw_events.h>
#include <wiixlaunch/imports/botw_gamedata.h>
#include <wiixlaunch/imports/botw_gui.h>
#include <wiixlaunch/imports/botw_input.h>
#include <wiixlaunch/imports/botw_player.h>
#include <wiixlaunch/imports/botw_pouch.h>
#include <wiixlaunch/imports/botw_sound.h>
#include <wiixlaunch/imports/botw_world.h>
#include <wiixlaunch/imports/wiixl_core.h>
#include <wiixlaunch/mod_log.h>
#include <wiixlaunch/mod_runtime.h>

#include "botw.h"

namespace Core {
WXL_USE_wiixl_core(InstallHook);
}

namespace Player {
WXL_USE_botw_player(Init);
WXL_USE_botw_player(RegisterTick);
WXL_USE_botw_player(GetPlayerActor);
WXL_USE_botw_player(GetPosition);
WXL_USE_botw_player(GetEquippedSword);
WXL_USE_botw_player(GetEquippedShield);
WXL_USE_botw_player(GetEquippedBow);
WXL_USE_botw_player(ActorUnsafeRawPointer);
}

namespace Actor {
WXL_USE_botw_actor(Init);
WXL_USE_botw_actor(IsValid);
WXL_USE_botw_actor(GetId);
WXL_USE_botw_actor(GetName);
WXL_USE_botw_actor(GetLife);
WXL_USE_botw_actor(SetLife);
WXL_USE_botw_actor(GetMaxLife);
WXL_USE_botw_actor(GetHearts);
WXL_USE_botw_actor(SetHearts);
WXL_USE_botw_actor(GetMaxHearts);
WXL_USE_botw_actor(SetControllerVelocity);
WXL_USE_botw_actor(GetHavokVelocity);
WXL_USE_botw_actor(GetMatrix);
WXL_USE_botw_actor(Spawn);
WXL_USE_botw_actor(SpawnScaled);
}

namespace Input {
WXL_USE_botw_input(Init);
WXL_USE_botw_input(HeldButtons);
WXL_USE_botw_input(MaskFor);
}

namespace GameData {
WXL_USE_botw_gamedata(GetStaminaWheels);
WXL_USE_botw_gamedata(GetMaxStaminaWheels);
WXL_USE_botw_gamedata(SetStaminaWheels);
WXL_USE_botw_gamedata(GetRupees);
WXL_USE_botw_gamedata(SetRupees);
WXL_USE_botw_gamedata(AddRupees);
WXL_USE_botw_gamedata(GetKorokCount);
WXL_USE_botw_gamedata(GetFlagBool);
WXL_USE_botw_gamedata(SetFlagBool);
}

namespace Pouch {
WXL_USE_botw_pouch(AddItem);
WXL_USE_botw_pouch(EquipItem);
WXL_USE_botw_pouch(TickEquipRefresh);
WXL_USE_botw_pouch(RemoveItem);
WXL_USE_botw_pouch(GetTypeForName);
WXL_USE_botw_pouch(FindItemIndex);
}

namespace Sound {
WXL_USE_botw_sound(Play);
}

namespace World {
WXL_USE_botw_world(TimeAvailable);
WXL_USE_botw_world(WeatherAvailable);
WXL_USE_botw_world(GetGameTimeHM);
WXL_USE_botw_world(SetGameTimeHM);
WXL_USE_botw_world(GetTimeScale);
WXL_USE_botw_world(SetTimeScale);
WXL_USE_botw_world(IsNight);
WXL_USE_botw_world(GetTemperature);
WXL_USE_botw_world(GetWeather);
WXL_USE_botw_world(WeatherName);
WXL_USE_botw_world(WeatherFromName);
WXL_USE_botw_world(HoldWeather);
WXL_USE_botw_world(TickWeatherHold);
WXL_USE_botw_world(ReleaseWeather);
}

namespace Events {
WXL_USE_botw_events(Init);
WXL_USE_botw_events(Tick);
WXL_USE_botw_events(ConsumeKorokGet);
WXL_USE_botw_events(ConsumeShrineComplete);
WXL_USE_botw_events(ConsumeTowerOpen);
}

namespace Gui {
WXL_USE_botw_gui(Init);
WXL_USE_botw_gui(IsReady);
WXL_USE_botw_gui(RegisterFrame);
WXL_USE_botw_gui(TimeSeconds);
WXL_USE_botw_gui(StyleCreate);
WXL_USE_botw_gui(StyleScale);
WXL_USE_botw_gui(StyleColor);
WXL_USE_botw_gui(StyleShadow);
WXL_USE_botw_gui(StyleAlign);
WXL_USE_botw_gui(MeasureText);
WXL_USE_botw_gui(RoundedBox);
WXL_USE_botw_gui(Text);
}

namespace Engine {

// game.cpp
void BeginFrame();   // read buttons, events and weapons for this frame
void EndFrame();     // keep the game's queued requests moving
bool InGame();       // a save is loaded and Link is on the field

// Buttons kept from the game (game.cpp collects them, main.cpp's controller
// hooks clear them). A mask with one bit per botw.h Button: 1 << A, 1 << B...
uint32_t HiddenButtons();   // 0 when nothing is hidden right now
void HiddenReadDone();      // a controller read has used the mask

// screen.cpp
void Draw();         // botw.gui's frame callback
void BeginDrawing(); // before every_frame: start a fresh list of shapes
void EndDrawing();   // after every_frame: hand that list to Draw()
void Show(const char* text);  // add a line to the on-screen messages

// printf-style formatting into a buffer that is always terminated.
inline void Format(char* out, uint32_t cap, const char* format, va_list args) {
    const uint32_t n = WiiXLaunch::Debug::FormatText(out, cap - 1, format, args);
    out[n < cap ? n : cap - 1] = '\0';
}

inline void FormatTo(char* out, uint32_t cap, const char* format, ...) {
    va_list args;
    va_start(args, format);
    Format(out, cap, format, args);
    va_end(args);
}

// Copies at most cap - 1 characters and always terminates.
inline void CopyText(char* out, uint32_t cap, const char* text) {
    uint32_t i = 0;
    for (; text && text[i] && i + 1 < cap; ++i) out[i] = text[i];
    out[i] = '\0';
}

} // namespace Engine
