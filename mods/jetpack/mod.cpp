// Lesson 8 - jetpack.wxlm : hold ZL + X to fly upward
//
// Move Link by feeding the physics engine, at the right moment in the frame.
//
// Concepts:
//   * WHERE in the frame your code runs matters. BotW's physics rewrites the
//     actor's position and velocity every frame, so poking them from an
//     ordinary tick does nothing. The character controller's "desired
//     velocity" is an input the physics consumes - but only if it is written
//     between the player's update and the physics step. botw.player's
//     RegisterTick runs exactly there; Core::RegisterTick does not.
//   * It must be written EVERY frame - the controller clears it afterwards.
//     Let go of the buttons and gravity takes over again.
//   * Units are world units per second; Link is about 1.7 units tall.
//
// Try: thrust in the direction the camera faces (botw.camera), or limit fuel
// using the stamina wheel (botw.gamedata).

#include <cstdint>

#include <wiixlaunch/imports/botw_actor.h>
#include <wiixlaunch/imports/botw_input.h>
#include <wiixlaunch/imports/botw_player.h>
#include <wiixlaunch/mod_log.h>
#include <wiixlaunch/mod_runtime.h>

namespace Player {
WXL_USE_botw_player(Init);
WXL_USE_botw_player(RegisterTick);
WXL_USE_botw_player(GetPlayerActor);
}

namespace Actor {
WXL_USE_botw_actor(Init);
WXL_USE_botw_actor(IsValid);
WXL_USE_botw_actor(SetControllerVelocity);
}

namespace Input {
WXL_USE_botw_input(Init);
WXL_USE_botw_input(HeldButtons);
WXL_USE_botw_input(MaskFor);
}

namespace {

// WiiXLaunch::BotW::Button enumerators (botw/game/controller.hpp).
constexpr int32_t kX = 2;
constexpr int32_t kZL = 8;

constexpr float kLiftSpeed = 12.0f;  // world units per second, straight up

bool g_WasFlying;

void PlayerTick() {
    const uint32_t combo = Input::MaskFor(kZL) | Input::MaskFor(kX);
    const bool flying = (Input::HeldButtons() & combo) == combo;

    if (flying) {
        const uint32_t link = Player::GetPlayerActor();
        if (link && Actor::IsValid(link)) Actor::SetControllerVelocity(link, 0.0f, kLiftSpeed, 0.0f);
    }

    if (flying != g_WasFlying) {
        g_WasFlying = flying;
        WIIXL_LOG(flying ? "jetpack: thrust on" : "jetpack: thrust off");
    }
}

} // namespace

extern "C" __attribute__((used)) void WiiXLaunch_ModEntry() {
    Input::Init();
    Actor::Init();
    Player::Init();  // installs the player-update hook that PlayerTick rides on
    if (Player::RegisterTick(&PlayerTick)) WIIXL_LOG("jetpack: loaded - hold ZL + X to fly");
}
