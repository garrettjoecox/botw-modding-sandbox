// Lesson 7 - bounty.wxlm : get paid for exploring
//
// React to things happening in the game, instead of polling a value.
//
// Concepts:
//   * Events - botw.events notices progress (a korok found, a shrine
//     finished, a tower activated) and queues it. You "consume" events, so each
//     one is handled exactly once even if you check every frame.
//   * Someone has to drive it - events only advance when a mod calls
//     Events::Tick() from a per-frame callback. Forget that and nothing ever
//     fires, with no error. (A good debugging exercise: comment it out.)
//   * Writing state the player can see - rupees are a GameData flag, and
//     AddRupees goes through the game's own flag system, so the HUD updates.
//
// Try: scale the reward by how many koroks are left, or charge 10 rupees
// every time Link takes damage.

#include <cstdint>

#include <wiixlaunch/imports/botw_events.h>
#include <wiixlaunch/imports/botw_gamedata.h>
#include <wiixlaunch/imports/wiixl_core.h>
#include <wiixlaunch/mod_log.h>
#include <wiixlaunch/mod_runtime.h>

namespace Core {
WXL_USE_wiixl_core(RegisterTick);
}

namespace Events {
WXL_USE_botw_events(Init);
WXL_USE_botw_events(Tick);
WXL_USE_botw_events(ConsumeKorokGet);
WXL_USE_botw_events(ConsumeShrineComplete);
WXL_USE_botw_events(ConsumeTowerOpen);
}

namespace GameData {
WXL_USE_botw_gamedata(AddRupees);
WXL_USE_botw_gamedata(GetRupees);
}

namespace {

constexpr int32_t kKorokReward = 50;
constexpr int32_t kShrineReward = 300;
constexpr int32_t kTowerReward = 100;

void Pay(int32_t amount, const char* why) {
    GameData::AddRupees(amount);
    int32_t wallet = 0;
    GameData::GetRupees(&wallet);
    WIIXL_LOG("bounty: +%d rupees for %s (wallet now %d)", amount, why, wallet);
}

void Tick() {
    Events::Tick();  // let the event system look for changes this frame

    int32_t total = 0, gained = 0;
    while (Events::ConsumeKorokGet(&total, &gained)) {
        Pay(kKorokReward * (gained > 0 ? gained : 1), "a korok seed");
        WIIXL_LOG("bounty: %d korok seeds so far", total);
    }
    while (Events::ConsumeShrineComplete()) Pay(kShrineReward, "a shrine");
    while (Events::ConsumeTowerOpen()) Pay(kTowerReward, "a tower");
}

} // namespace

extern "C" __attribute__((used)) void WiiXLaunch_ModEntry() {
    Events::Init();
    if (Core::RegisterTick(&Tick)) {
        WIIXL_LOG("bounty: loaded - korok %d, shrine %d, tower %d rupees", kKorokReward,
                  kShrineReward, kTowerReward);
    }
}
