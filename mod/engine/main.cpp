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

// --- hiding buttons from the game ---------------------------------------------
//
// The game reads the controllers once a frame through two functions (Wii U
// v208, from wiixlaunch-botw's controller code): 0x030d9f24 for the GamePad
// (which the keyboard drives in Cemu) and 0x030da168 for Pro Controllers and
// Wii Remotes. Hooked here BEFORE botw.input installs its own hooks on them:
// WiiXLaunch calls hooks first-installed-first, so ours wraps everything.
// botw.input still records the real buttons (for held() and pressed()), and
// only then do these hooks clear the hidden buttons, before the game sees them.

constexpr uintptr_t kReadGamePad = 0x030d9f24;
constexpr uintptr_t kReadOtherPads = 0x030da168;

using PadReadFn = void (*)(void* pads);
PadReadFn volatile g_OriginalReadGamePad;
PadReadFn volatile g_OriginalReadOtherPads;

// Each botw.h Button (A, B, X, Y, L, R, ZL, ZR, PLUS, MINUS, UP, DOWN, LEFT,
// RIGHT, L3, R3) as a bit in each controller's own layout - the same values
// wiixlaunch-botw's controller code translates from. The Wii Remote's D-pad
// only: its other buttons don't line up with these.
constexpr uint32_t kGamePadBits[] = {0x8000, 0x4000, 0x2000, 0x1000, 0x0020, 0x0010, 0x0080, 0x0040,
                                     0x0008, 0x0004, 0x0200, 0x0100, 0x0800, 0x0400, 0x40000, 0x20000};
constexpr uint32_t kProBits[] = {0x0010, 0x0040, 0x0008, 0x0020, 0x2000, 0x0200, 0x0080, 0x0004,
                                 0x0400, 0x1000, 0x0001, 0x4000, 0x0002, 0x8000, 0x20000, 0x10000};
constexpr uint32_t kRemoteBits[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x0008, 0x0004, 0x0001, 0x0002, 0, 0};

uint32_t ControllerBits(uint32_t buttons, const uint32_t* table) {
    uint32_t bits = 0;
    for (uint32_t i = 0; i < 16; ++i) {
        if (buttons & (1u << i)) bits |= table[i];
    }
    return bits;
}

// Clears bits in a controller's hold / trigger / release words.
void ClearButtons(uint8_t* status, uint32_t bits) {
    for (int i = 0; i < 3; ++i) *reinterpret_cast<uint32_t*>(status + 4 * i) &= ~bits;
}

void OnReadGamePad(void* pads) {
    PadReadFn original = g_OriginalReadGamePad;
    if (original) original(pads);
    const uint32_t hidden = Engine::HiddenButtons();
    if (!hidden) return;
    ClearButtons(static_cast<uint8_t*>(pads) + 0x14, ControllerBits(hidden, kGamePadBits));   // VPADStatus
    Engine::HiddenReadDone();
}

void OnReadOtherPads(void* pads) {
    PadReadFn original = g_OriginalReadOtherPads;
    if (original) original(pads);
    const uint32_t hidden = Engine::HiddenButtons();
    if (!hidden) return;
    for (int channel = 0; channel < 4; ++channel) {
        uint8_t* kpad = static_cast<uint8_t*>(pads) + 0x1118 + channel * 0xF08;
        ClearButtons(kpad, ControllerBits(hidden, kRemoteBits));                  // Wii Remote
        if (kpad[0x5C] == 0x1f) ClearButtons(kpad + 0x60, ControllerBits(hidden, kProBits));   // Pro Controller
    }
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
    // First, so they wrap botw.input's hooks on the same functions (see above).
    g_OriginalReadGamePad = reinterpret_cast<PadReadFn>(
        Core::InstallHook(kReadGamePad, reinterpret_cast<uintptr_t>(&OnReadGamePad)));
    g_OriginalReadOtherPads = reinterpret_cast<PadReadFn>(
        Core::InstallHook(kReadOtherPads, reinterpret_cast<uintptr_t>(&OnReadOtherPads)));
    if (!g_OriginalReadGamePad || !g_OriginalReadOtherPads) WIIXL_LOG("my_mod: controller hooks refused");

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
