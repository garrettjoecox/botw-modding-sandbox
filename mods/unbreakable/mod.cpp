// Lesson 6 - unbreakable.wxlm : weapons, bows and shields never wear down
//
// Remember a value across frames and undo the game's changes to it.
//
// Concepts:
//   * Actor handles - everything in the world (Link, a sword, a bokoblin) is
//     an "actor". The host hands out opaque handles instead of raw pointers,
//     and checks them for you: a handle to a destroyed actor reads as invalid
//     rather than crashing.
//   * Handles are tickets, not identities. Every GetEquippedSword() call mints
//     a NEW handle for the same sword, so "did the weapon change?" must compare
//     the actor's ID (botw.actor GetId), never two handles. (The first version
//     of this mod compared handles and re-detected a "new" sword every tick.)
//   * A weapon's durability is its actor "life", the same field that is an
//     enemy's health - in finer units than the game shows: a fresh Traveler's
//     Bow read 2200 here.
//   * State across frames - we track the best durability seen for whatever is
//     equipped, and reset the record when the player swaps weapons.
//
// Gotcha worth discussing: the same number also lives in the inventory
// (botw.pouch, "PouchItem value"), and the game copies actor -> pouch. We fix
// the actor, so the pouch follows. Fixing only the pouch would be undone.
//
// Try: make durability go UP when you hit something, or only protect swords.

#include <cstdint>

#include <wiixlaunch/imports/botw_actor.h>
#include <wiixlaunch/imports/botw_player.h>
#include <wiixlaunch/imports/wiixl_core.h>
#include <wiixlaunch/mod_log.h>
#include <wiixlaunch/mod_runtime.h>

namespace Core {
WXL_USE_wiixl_core(RegisterTick);
}

namespace Player {
WXL_USE_botw_player(GetEquippedSword);
WXL_USE_botw_player(GetEquippedShield);
WXL_USE_botw_player(GetEquippedBow);
WXL_USE_botw_player(ActorIsValid);
WXL_USE_botw_player(ActorGetName);
WXL_USE_botw_player(ActorGetLife);
WXL_USE_botw_player(ActorSetLife);
}

namespace Actor {
WXL_USE_botw_actor(GetId);
}

namespace {

enum Kind { kSword, kShield, kBow };

struct Slot {
    Kind kind;
    const char* label;
    uint32_t id;      // actor ID of the weapon we're tracking (0 = none yet)
    int32_t best;     // highest durability seen for it
    uint32_t saves;   // how many times we've repaired it
};

Slot g_Slots[] = {
    {kSword, "sword", 0, 0, 0},
    {kShield, "shield", 0, 0, 0},
    {kBow, "bow", 0, 0, 0},
};

uint32_t GetEquipped(Kind kind) {
    switch (kind) {
        case kSword: return Player::GetEquippedSword();
        case kShield: return Player::GetEquippedShield();
        case kBow: return Player::GetEquippedBow();
    }
    return 0;
}

void Watch(Slot& s) {
    const uint32_t h = GetEquipped(s.kind);
    uint32_t id = 0;
    if (!h || !Player::ActorIsValid(h) || !Actor::GetId(h, &id)) {
        // Nothing readable right now. Don't forget the weapon: while an equip
        // settles, the lookup flickers empty on alternate ticks, and clearing
        // here made every flicker look like a brand-new weapon.
        return;
    }

    const int32_t life = Player::ActorGetLife(h);
    if (id != s.id) {
        // Newly equipped: start tracking from its current durability.
        s.id = id;
        s.best = life;
        s.saves = 0;
        char name[48] = {};
        Player::ActorGetName(h, name, sizeof(name));
        WIIXL_LOG("unbreakable: now protecting %s %s (durability %d)", s.label, name, life);
        return;
    }

    if (life > s.best) {
        s.best = life;  // repaired or upgraded by the game - keep the new value
    } else if (life < s.best) {
        Player::ActorSetLife(h, s.best);
        if (++s.saves <= 3) {
            WIIXL_LOG("unbreakable: %s took wear %d -> restored to %d", s.label, life, s.best);
        }
    }
}

void Tick() {
    for (Slot& s : g_Slots) Watch(s);
}

} // namespace

extern "C" __attribute__((used)) void WiiXLaunch_ModEntry() {
    if (Core::RegisterTick(&Tick)) WIIXL_LOG("unbreakable: loaded");
}
