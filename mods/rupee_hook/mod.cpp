// Lesson 9 - rupee_hook.wxlm : double every rupee you pick up
//
// Hook a function inside the game's own code. This is the "real" technique the
// friendly surfaces in lessons 2-7 are built from.
//
// Concepts:
//   * Function addresses come from reverse engineering. This one is from
//     WiiXLaunch/vendor/wiixlaunch-botw/data/symbols-wiiu-v208.csv:
//
//       0x03204a14  void(gdt::Manager*, s32 delta, sead::SafeString* name)
//       "The rupee pickup path: 0x02315500 calls it with the pickup amount
//        and "CurrentRupee" when the actor is collected."
//
//     Addresses are only valid for this exact build (Wii U v208).
//   * InstallHook(target, callback) redirects the game's function to ours and
//     returns a pointer to "the original" - call it to let the game continue.
//     Change the arguments on the way in and you change the game's behaviour.
//   * Calling convention: on PowerPC the arguments arrive in r3, r4, r5...;
//     declaring our callback with the same C signature is enough for the
//     compiler to read them correctly.
//   * Reading game structs: sead::SafeString is { const char* text; vtable* }.
//     We only need the text pointer, so a two-field struct is enough.
//   * The function adds to ANY integer flag by name, not just rupees - so we
//     filter, and log what else passes through (great for discovering flags).
//   * You can CALL game functions too. ZL + Minus calls 0x03204a14 ourselves
//     with 5 rupees, exactly as the pickup code would. Because our hook sits
//     on that address, the call runs through OnAddFlag first - so it doubles
//     to 10. That doubles as a test you can run without finding a rupee.
//   * Game singletons live at fixed addresses: 0x1046d5b0 holds the
//     gdt::Manager pointer (null until a save is loaded - always check).
//
// Try: make rupees count triple only at night (botw.world), or log every flag
// name for ten minutes of play and see what the game tracks.

#include <cstdint>

#include <wiixlaunch/imports/botw_gamedata.h>
#include <wiixlaunch/imports/botw_input.h>
#include <wiixlaunch/imports/wiixl_core.h>
#include <wiixlaunch/mod_log.h>
#include <wiixlaunch/mod_runtime.h>

namespace Core {
WXL_USE_wiixl_core(InstallHook);
WXL_USE_wiixl_core(RegisterTick);
}

namespace Input {
WXL_USE_botw_input(Init);
WXL_USE_botw_input(HeldButtons);
WXL_USE_botw_input(MaskFor);
}

namespace GameData {
WXL_USE_botw_gamedata(GetRupees);
}

namespace {

constexpr uintptr_t kAddFlagS32ByName = 0x03204a14;   // Wii U v208 only
constexpr uintptr_t kGdtManagerPtr = 0x1046d5b0;      // gdt::Manager* singleton
constexpr uintptr_t kSafeStringVtable = 0x10263910;   // sead::SafeString vtable

// WiiXLaunch::BotW::Button enumerators (botw/game/controller.hpp).
constexpr int32_t kZL = 8;
constexpr int32_t kMinus = 11;

struct SafeString {
    const char* text;
    const void* vtable;
};

using AddFlagFn = void (*)(void* manager, int32_t delta, SafeString* name);

// Written once by InstallHook; volatile so the compiler always reloads it.
AddFlagFn volatile g_Original;
uint32_t g_Seen;

bool Equals(const char* a, const char* b) {
    if (!a || !b) return false;
    while (*a && *a == *b) ++a, ++b;
    return *a == *b;
}

void OnAddFlag(void* manager, int32_t delta, SafeString* name) {
    const char* flag = name ? name->text : nullptr;

    if (Equals(flag, "CurrentRupee") && delta > 0) {
        WIIXL_LOG("rupee_hook: picked up %d rupees - paying out %d", delta, delta * 2);
        delta *= 2;
    } else if (++g_Seen <= 20) {
        WIIXL_LOG("rupee_hook: flag %s += %d", flag ? flag : "(null)", delta);
    }

    AddFlagFn original = g_Original;
    if (original) original(manager, delta, name);  // let the game do its work
}

uint32_t g_PrevHeld;

// Call the game's own function, as the rupee pickup code does.
void FakePickup(int32_t amount) {
    void* manager = *reinterpret_cast<void**>(kGdtManagerPtr);
    const uintptr_t addr = reinterpret_cast<uintptr_t>(manager);
    if (addr < 0x10000000 || addr >= 0xa0000000) {
        WIIXL_LOG("rupee_hook: no save loaded");
        return;
    }
    int32_t before = 0;
    GameData::GetRupees(&before);
    SafeString name = {"CurrentRupee", reinterpret_cast<const void*>(kSafeStringVtable)};
    reinterpret_cast<AddFlagFn>(kAddFlagS32ByName)(manager, amount, &name);
    WIIXL_LOG("rupee_hook: fake pickup of %d done (wallet was %d)", amount, before);
}

void Tick() {
    const uint32_t held = Input::HeldButtons();
    const uint32_t pressed = held & ~g_PrevHeld;
    g_PrevHeld = held;
    if ((held & Input::MaskFor(kZL)) && (pressed & Input::MaskFor(kMinus))) FakePickup(5);
}

} // namespace

extern "C" __attribute__((used)) void WiiXLaunch_ModEntry() {
    Input::Init();
    Core::RegisterTick(&Tick);

    const uintptr_t original =
        Core::InstallHook(kAddFlagS32ByName, reinterpret_cast<uintptr_t>(&OnAddFlag));
    if (!original) {
        WIIXL_LOG("rupee_hook: InstallHook refused - is this v208?");
        return;
    }
    g_Original = reinterpret_cast<AddFlagFn>(original);
    WIIXL_LOG("rupee_hook: loaded - rupee pickups are doubled (ZL + Minus: fake pickup)");
}
