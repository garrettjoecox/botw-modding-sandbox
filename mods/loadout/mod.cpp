// Lesson 5 - loadout.wxlm : press ZL + R for a starter kit
//
// Give the player items and equip them through the game's inventory ("pouch").
//
// Concepts:
//   * Items are actor names. "Weapon_Sword_001" is the Traveler's Sword;
//     the full list is in the game's Actor/ folder or on zeldamods.org.
//   * value means different things per item type - durability for weapons,
//     a count for arrows and materials.
//   * Asynchronous effects - equipping doesn't happen instantly. The game
//     re-reads the pouch a frame or two later, and the framework holds the
//     request until then. Something has to call TickEquipRefresh() every
//     frame or the equip quietly never lands. (Exercise: remove that line.)
//   * Edge detection - act once per press, not once per frame the button is
//     held (compare this frame's buttons with last frame's).
//   * "Succeeded" is not "did what you meant". On the title screen a pouch
//     already exists, so AddItem returns true - and then loading the save
//     replaces that pouch and your items vanish. Even "is there a Link?" is
//     not enough: the title scene has a Link actor too. What we measured is
//     that the title-screen Link has max life 0, so that is the guard.
//
// Try: give a full set of armour, or a random weapon from a list each press.

#include <cstdint>

#include <wiixlaunch/imports/botw_input.h>
#include <wiixlaunch/imports/botw_player.h>
#include <wiixlaunch/imports/botw_pouch.h>
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

namespace Player {
WXL_USE_botw_player(GetPlayerActor);
WXL_USE_botw_player(ActorIsValid);
WXL_USE_botw_player(ActorGetMaxLife);
}

namespace Pouch {
WXL_USE_botw_pouch(AddItem);
WXL_USE_botw_pouch(EquipItem);
WXL_USE_botw_pouch(TickEquipRefresh);
}

namespace {

// WiiXLaunch::BotW::Button enumerators (botw/game/controller.hpp).
constexpr int32_t kR = 7;
constexpr int32_t kZL = 8;

struct Item {
    const char* name;
    int32_t value;
    bool equip;
};

constexpr Item kKit[] = {
    {"Weapon_Sword_001", 22, true},   // Traveler's Sword
    {"Weapon_Bow_001", 20, true},     // Traveler's Bow
    {"Weapon_Shield_001", 10, true},  // Wooden Shield
    {"NormalArrow", 20, true},        // arrows
};

uint32_t g_PrevHeld;

void GiveKit() {
    const uint32_t link = Player::GetPlayerActor();
    if (!link || !Player::ActorIsValid(link) || Player::ActorGetMaxLife(link) <= 0) {
        WIIXL_LOG("loadout: no Link yet - load a save first");
        return;
    }
    for (const Item& item : kKit) {
        if (!Pouch::AddItem(item.name, item.value)) {
            WIIXL_LOG("loadout: could not add %s (no save loaded?)", item.name);
            continue;
        }
        if (item.equip) Pouch::EquipItem(item.name);
        WIIXL_LOG("loadout: gave %s x%d", item.name, item.value);
    }
}

void Tick() {
    Pouch::TickEquipRefresh();  // lets queued equips land

    const uint32_t held = Input::HeldButtons();
    const uint32_t pressed = held & ~g_PrevHeld;
    g_PrevHeld = held;

    const uint32_t zl = Input::MaskFor(kZL);
    if ((held & zl) && (pressed & Input::MaskFor(kR))) GiveKit();
}

} // namespace

extern "C" __attribute__((used)) void WiiXLaunch_ModEntry() {
    Input::Init();
    if (Core::RegisterTick(&Tick)) WIIXL_LOG("loadout: loaded - press ZL + R for a starter kit");
}
