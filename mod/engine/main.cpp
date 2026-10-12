// main.cpp - where the game first enters the mod, and how every_frame() and
// when_game_adds() get called.
//
// Behind-the-scenes code: you never need to look in here. It is ordinary
// WiiXLaunch mod code, the same as in the framework's own examples.

#include "engine/engine.h"

namespace {

// Runs from botw.player's per-frame callback, which fires right after Link's
// own update and before the physics step. That timing is why move_link() works
// from every_frame(): physics only reads Link's push in that window.
void OnPlayerTick() {
    if (!Engine::InGame()) return;
    Engine::BeginFrame();
    Engine::BeginDrawing();
    every_frame();
    Engine::EndDrawing();
    Engine::EndFrame();
}

// --- the counter hook (examples/double_rupees.cpp) ---------------------------
//
// 0x03204a14 is a function inside BotW (Wii U v208 only), found by reverse
// engineering. It adds a number to one of the game's named counters, and the
// rupee pickup code calls it with the pickup amount and "CurrentRupee". From
// WiiXLaunch/vendor/wiixlaunch-botw/data/symbols-wiiu-v208.csv:
//
//   0x03204a14  void(gdt::Manager*, s32 delta, sead::SafeString* name)
//
// InstallHook sends the game's calls to OnAddCounter instead, and gives back
// the address of the original so we can let the game carry on.

constexpr uintptr_t kAddCounter = 0x03204a14;        // the function above
constexpr uintptr_t kGameDataManager = 0x1046d5b0;   // holds the gdt::Manager*
constexpr uintptr_t kSafeStringVtable = 0x10263910;  // sead::SafeString's vtable

// sead::SafeString is { const char* text; vtable* }.
struct SafeString {
    const char* text;
    const void* vtable;
};

using AddCounterFn = void (*)(void* manager, int32_t delta, SafeString* name);

// Written once by InstallHook; volatile so the compiler always reloads it.
AddCounterFn volatile g_OriginalAddCounter;

void OnAddCounter(void* manager, int32_t delta, SafeString* name) {
    if (name && name->text) delta = when_game_adds(name->text, delta);
    AddCounterFn original = g_OriginalAddCounter;
    if (original) original(manager, delta, name);
}

} // namespace

void game_add_to_counter(const char* counter, int amount) {
    // The manager is a game object; until a save is loaded the pointer is null.
    void* manager = *reinterpret_cast<void**>(kGameDataManager);
    const uintptr_t address = reinterpret_cast<uintptr_t>(manager);
    if (address < 0x10000000 || address >= 0xa0000000) return;
    SafeString name = {counter, reinterpret_cast<const void*>(kSafeStringVtable)};
    // Call the game's own function at its address. Our hook sits on it, so this
    // goes through OnAddCounter (and when_game_adds) first, like a real pickup.
    reinterpret_cast<AddCounterFn>(kAddCounter)(manager, amount, &name);
}

// The loader calls this once, when the mod loads (during the game's boot).
// extern "C" keeps the name exactly "WiiXLaunch_ModEntry" so the loader finds
// it; "used" stops the compiler deleting it, since nothing in the mod calls it.
extern "C" __attribute__((used)) void WiiXLaunch_ModEntry() {
    // Each surface installs the game hooks it needs. They're safe to call more
    // than once, so it doesn't matter if another mod already did.
    Input::Init();
    Actor::Init();
    Player::Init();
    Events::Init();
    Gui::Init();

    if (!Player::RegisterTick(&OnPlayerTick)) WIIXL_LOG("my_mod: player tick refused");
    if (!Gui::RegisterFrame(&Engine::Draw)) WIIXL_LOG("my_mod: GUI frame refused");

    const uintptr_t original =
        Core::InstallHook(kAddCounter, reinterpret_cast<uintptr_t>(&OnAddCounter));
    g_OriginalAddCounter = reinterpret_cast<AddCounterFn>(original);
    if (!original) WIIXL_LOG("my_mod: counter hook refused - is this BotW v208?");

    print("Your mod is loaded");
}
