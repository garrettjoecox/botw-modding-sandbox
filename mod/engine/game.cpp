// game.cpp - botw.h's game functions, written with WiiXLaunch imports.
//
// Behind-the-scenes code. Most functions here turn one import into a friendlier
// shape: a return value instead of an out-pointer, the units the player sees,
// and 0 instead of a crash when the game has nothing to report.

#include "engine/engine.h"

namespace {

// --- this frame's buttons ----------------------------------------------------

uint32_t g_Held;
uint32_t g_HeldBefore;

// botw.h's Button order -> WiiXLaunch::BotW::Button (botw/game/controller.hpp).
constexpr int32_t kButtonIds[] = {
    0,   // A
    1,   // B
    2,   // X
    3,   // Y
    6,   // L
    7,   // R
    8,   // ZL
    9,   // ZR
    10,  // PLUS
    11,  // MINUS
    13,  // UP
    15,  // DOWN
    12,  // LEFT
    14,  // RIGHT
};

uint32_t MaskFor(Button button) {
    const uint32_t index = static_cast<uint32_t>(button);
    if (index >= sizeof(kButtonIds) / sizeof(kButtonIds[0])) return 0;
    return Input::MaskFor(kButtonIds[index]);
}

// --- this frame's events -----------------------------------------------------

bool g_KorokFound;
bool g_ShrineFinished;
bool g_TowerActivated;

// --- the equipped weapons ----------------------------------------------------

struct WeaponSlot {
    uint32_t handle;   // valid for this frame only; 0 when nothing readable
    uint32_t id;       // the actor's ID, which stays the same while it's equipped
    bool changed;
    char name[48];
};

WeaponSlot g_Weapons[3];

uint32_t FetchWeapon(Weapon weapon) {
    switch (weapon) {
        case SWORD: return Player::GetEquippedSword();
        case SHIELD: return Player::GetEquippedShield();
        case BOW: return Player::GetEquippedBow();
    }
    return 0;
}

void TrackWeapon(Weapon weapon) {
    WeaponSlot& slot = g_Weapons[weapon];
    slot.changed = false;
    // Each GetEquipped call hands out a new handle for the same weapon, so the
    // actor ID is what says whether it's still the same one.
    const uint32_t handle = FetchWeapon(weapon);
    uint32_t id = 0;
    if (!handle || !Actor::IsValid(handle) || !Actor::GetId(handle, &id)) {
        // While an equip settles the lookup flickers empty for a frame. Keep the
        // ID so the weapon doesn't look brand new when it comes back.
        slot.handle = 0;
        return;
    }
    slot.handle = handle;
    if (id != slot.id) {
        slot.id = id;
        slot.changed = true;
        Actor::GetName(handle, slot.name, sizeof(slot.name));
    }
}

const WeaponSlot* Slot(Weapon weapon) {
    const uint32_t index = static_cast<uint32_t>(weapon);
    return index < 3 ? &g_Weapons[index] : nullptr;
}

// --- requests the engine keeps alive ----------------------------------------

// spawn() requests. botw.actor holds one spawn at a time and refuses another
// until the game has created it, so the rest wait here and go one per frame.
struct SpawnRequest {
    char name[64];
    float x, y, z;
    float size;   // 1 = normal
};

SpawnRequest g_Spawns[16];
uint32_t g_SpawnCount;

// Actor names are letters, digits and underscores, like "Enemy_Bokoblin_Junior".
// (botw.actor's own IsValidActorName can't be used: it also requires the text
// to live in the game's memory, and a mod's text never does.)
bool LooksLikeActorName(const char* name) {
    if (!name || !name[0]) return false;
    uint32_t length = 0;
    for (; name[length]; ++length) {
        const char c = name[length];
        const bool ok = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                        (c >= '0' && c <= '9') || c == '_';
        if (!ok || length >= 63) return false;
    }
    return length >= 2;
}

bool QueueSpawn(const char* name, float x, float y, float z, float size = 1.0f) {
    if (!LooksLikeActorName(name)) {
        print("spawn: \"%s\" isn't an actor name", name ? name : "");
        return false;
    }
    if (g_SpawnCount >= sizeof(g_Spawns) / sizeof(g_Spawns[0])) return false;
    SpawnRequest& request = g_Spawns[g_SpawnCount++];
    Engine::CopyText(request.name, sizeof(request.name), name);
    request.x = x;
    request.y = y;
    request.z = z;
    request.size = size;
    return true;
}

float g_WantedTimeSpeed = 1.0f;
bool g_PushedThisFrame;
uint32_t g_FramesSincePush = 1000;
bool g_HoldingWeather;
uint32_t g_Frame;

uint32_t Link() {
    const uint32_t link = Player::GetPlayerActor();
    return link && Actor::IsValid(link) ? link : 0;
}

} // namespace

namespace Engine {

bool InGame() {
    const uint32_t link = Link();
    // The title screen has a Link too, but his max life is 0.
    return link && Actor::GetMaxLife(link) > 0 && World::TimeAvailable();
}

void BeginFrame() {
    ++g_Frame;
    g_PushedThisFrame = false;

    g_HeldBefore = g_Held;
    g_Held = Input::HeldButtons();

    // botw.events only notices changes when something calls its Tick.
    Events::Tick();
    int32_t total = 0, gained = 0;
    g_KorokFound = false;
    while (Events::ConsumeKorokGet(&total, &gained)) g_KorokFound = true;
    g_ShrineFinished = false;
    while (Events::ConsumeShrineComplete()) g_ShrineFinished = true;
    g_TowerActivated = false;
    while (Events::ConsumeTowerOpen()) g_TowerActivated = true;

    TrackWeapon(SWORD);
    TrackWeapon(SHIELD);
    TrackWeapon(BOW);
}

void EndFrame() {
    // Hand the next queued spawn to the game. Spawn() refuses while the one
    // before is still being created; then it's simply tried again next frame.
    // (The player is only the "anchor": the game takes memory for the new actor
    // from the same place as his.)
    if (g_SpawnCount > 0) {
        const uint32_t link = Link();
        const SpawnRequest& next = g_Spawns[0];
        bool asked = false;
        if (link && next.size == 1.0f) {
            asked = Actor::Spawn(next.name, link, next.x, next.y, next.z);
        } else if (link) {
            asked = Actor::SpawnScaled(next.name, link, next.x, next.y, next.z,
                                       next.size, next.size, next.size);
        }
        if (asked) {
            for (uint32_t i = 1; i < g_SpawnCount; ++i) g_Spawns[i - 1] = g_Spawns[i];
            --g_SpawnCount;
        }
    }

    // move_link() writes a push the game only clears while Link is on the
    // ground. In the air it stays, and keeps launching him upward long after
    // the button is let go. So clear it for him once the pushing stops.
    if (g_PushedThisFrame) {
        g_FramesSincePush = 0;
    } else if (g_FramesSincePush < 3) {
        ++g_FramesSincePush;
        const uint32_t link = Link();
        if (link) Actor::SetControllerVelocity(link, 0.0f, 0.0f, 0.0f);
    }

    // An equip lands a frame or two after it's asked for, and only if something
    // keeps calling this.
    Pouch::TickEquipRefresh();

    // A weather hold is only re-applied while someone ticks it.
    if (g_HoldingWeather) World::TickWeatherHold();

    // The game resets the time speed on some transitions (loading zones,
    // cutscenes), so put a custom speed back a few times a second.
    if (g_WantedTimeSpeed != 1.0f && g_Frame % 10 == 0) {
        float scale = 0.0f;
        if (World::GetTimeScale(&scale) && scale != g_WantedTimeSpeed) {
            World::SetTimeScale(g_WantedTimeSpeed);
        }
    }
}

} // namespace Engine

// --- buttons -----------------------------------------------------------------

bool held(Button button) {
    return (g_Held & MaskFor(button)) != 0;
}

bool pressed(Button button) {
    return (g_Held & ~g_HeldBefore & MaskFor(button)) != 0;
}

// --- Link --------------------------------------------------------------------

float hearts() {
    float value = 0.0f;
    const uint32_t link = Link();
    if (link) Actor::GetHearts(link, &value);
    return value;
}

float max_hearts() {
    float value = 0.0f;
    const uint32_t link = Link();
    if (link) Actor::GetMaxHearts(link, &value);
    return value;
}

void set_hearts(float value) {
    const uint32_t link = Link();
    if (link) Actor::SetHearts(link, value);
}

float stamina() {
    float wheels = 0.0f;
    GameData::GetStaminaWheels(&wheels);
    return wheels;
}

float max_stamina() {
    float wheels = 0.0f;
    GameData::GetMaxStaminaWheels(&wheels);
    return wheels;
}

void set_stamina(float wheels) {
    GameData::SetStaminaWheels(wheels);
}

namespace {
float LinkPosition(int axis) {
    float position[3] = {0.0f, 0.0f, 0.0f};
    Player::GetPosition(position);
    return position[axis];
}
} // namespace

float link_x() { return LinkPosition(0); }
float link_y() { return LinkPosition(1); }
float link_z() { return LinkPosition(2); }

void move_link(float x, float y, float z) {
    // The character controller's "desired velocity" is a push, not a speed:
    // in the air each frame's push ADDS to Link's speed, so writing a steady
    // 12 launched him past 70 m/s, and stopping left him coasting upward for
    // hundreds of metres. So push by the gap between the speed asked for and
    // the speed he has (from the physics engine), plus a little upward to
    // cancel gravity. Measured: in the air, one unit of push changes his speed
    // by about 0.7 a frame.
    const uint32_t link = Link();
    if (!link) return;
    float now[3] = {0.0f, 0.0f, 0.0f};
    Actor::GetHavokVelocity(link, now);
    Actor::SetControllerVelocity(link, 0.7f * (x - now[0]),
                                 0.7f * (y - now[1]) + 1.0f,
                                 0.7f * (z - now[2]));
    g_PushedThisFrame = true;
}

float falling_speed() {
    // The physics engine's own record of how fast Link is really moving.
    float velocity[3] = {0.0f, 0.0f, 0.0f};
    const uint32_t link = Link();
    if (link) Actor::GetHavokVelocity(link, velocity);
    return velocity[1] < 0.0f ? -velocity[1] : 0.0f;   // y is up, so falling is negative
}

// --- spawning ----------------------------------------------------------------

bool spawn(const char* name, float distance) {
    const uint32_t link = Link();
    float m[12];
    if (!link || !Actor::GetMatrix(link, m)) return false;
    // Link's 3x4 matrix: the third column is the way he's facing, the fourth is
    // where he is. Start 2 m up so the new actor drops onto the ground rather
    // than appearing inside it.
    return QueueSpawn(name, m[3] + m[2] * distance, m[7] + 2.0f, m[11] + m[10] * distance);
}

bool spawn_at(const char* name, float x, float y, float z) {
    return QueueSpawn(name, x, y, z);
}

bool spawn_scaled(const char* name, float distance, float size) {
    const uint32_t link = Link();
    float m[12];
    if (!link || !Actor::GetMatrix(link, m) || size <= 0.0f) return false;
    // The game's "@S" creation setting, the same one the map uses to size the
    // objects it places.
    return QueueSpawn(name, m[3] + m[2] * distance, m[7] + 2.0f, m[11] + m[10] * distance, size);
}

// --- weapons -----------------------------------------------------------------

bool has_weapon(Weapon weapon) {
    const WeaponSlot* slot = Slot(weapon);
    return slot && slot->handle;
}

const char* weapon_name(Weapon weapon) {
    const WeaponSlot* slot = Slot(weapon);
    return slot && slot->handle ? slot->name : "";
}

int durability(Weapon weapon) {
    // A weapon's durability is its actor "life", the same number that is an
    // enemy's health.
    const WeaponSlot* slot = Slot(weapon);
    return slot && slot->handle ? Actor::GetLife(slot->handle) : 0;
}

void set_durability(Weapon weapon, int value) {
    const WeaponSlot* slot = Slot(weapon);
    if (slot && slot->handle) Actor::SetLife(slot->handle, value);
}

bool weapon_changed(Weapon weapon) {
    const WeaponSlot* slot = Slot(weapon);
    return slot && slot->changed;
}

// --- inventory and rupees ----------------------------------------------------

bool give_item(const char* name, int amount) {
    return Pouch::AddItem(name, amount) != 0;
}

bool equip(const char* name) {
    return Pouch::EquipItem(name) != 0;
}

int rupees() {
    int32_t value = 0;
    GameData::GetRupees(&value);
    return value;
}

void set_rupees(int amount) {
    GameData::SetRupees(amount);
}

void give_rupees(int amount) {
    GameData::AddRupees(amount);
}

// --- time and weather --------------------------------------------------------

int hour() {
    int32_t h = 0, m = 0;
    World::GetGameTimeHM(&h, &m);
    return h;
}

int minute() {
    int32_t h = 0, m = 0;
    World::GetGameTimeHM(&h, &m);
    return m;
}

void set_time(int h, int m) {
    World::SetGameTimeHM(h, m);
}

float time_speed() {
    float scale = 0.0f;
    World::GetTimeScale(&scale);
    return scale;
}

void set_time_speed(float speed) {
    g_WantedTimeSpeed = speed;
    World::SetTimeScale(speed);
}

bool is_night() {
    return World::IsNight() != 0;
}

float temperature() {
    float celsius = 0.0f;
    World::GetTemperature(&celsius);
    return celsius;
}

const char* weather_name() {
    static char name[24];
    int32_t type = 0;
    name[0] = '\0';
    if (World::WeatherAvailable() && World::GetWeather(&type)) {
        World::WeatherName(type, name, sizeof(name));
    }
    return name;
}

void force_weather(const char* name) {
    int32_t type = 0;
    if (!World::WeatherFromName(name, &type)) {
        print("force_weather: there's no weather called \"%s\"", name);
        return;
    }
    World::HoldWeather(type, 1);
    g_HoldingWeather = true;
}

void stop_forcing_weather() {
    if (!g_HoldingWeather) return;
    World::ReleaseWeather();
    g_HoldingWeather = false;
}

// --- things that just happened -----------------------------------------------

bool found_korok() { return g_KorokFound; }
bool finished_shrine() { return g_ShrineFinished; }
bool activated_tower() { return g_TowerActivated; }

int korok_seeds() {
    int32_t count = 0;
    GameData::GetKorokCount(&count);
    return count;
}

// --- game flags ----------------------------------------------------------------

bool flag(const char* name) {
    uint32_t on = 0;
    return GameData::GetFlagBool(name, &on) && on != 0;
}

bool set_flag(const char* name, bool on) {
    // The game refuses some: flags only its own quest scripts may change, and
    // flags that can be turned on but never off again.
    return GameData::SetFlagBool(name, on ? 1 : 0) != 0;
}

// --- sounds ------------------------------------------------------------------

bool play_sound(const char* name) {
    // botw.sound looks the name up in the game's own list of sound events and
    // plays it. It only reaches one small set (about 51 sounds), and returns
    // false for a name that isn't in it.
    return Sound::Play(name) != 0;
}

// --- extras ------------------------------------------------------------------

bool same_text(const char* a, const char* b) {
    if (!a || !b) return false;
    while (*a && *a == *b) ++a, ++b;
    return *a == *b;
}

int random_number(int low, int high) {
    // xorshift32: a tiny, fast generator. Mixing in the frame count means it
    // doesn't repeat the same sequence every time the game starts.
    static uint32_t state = 0;
    if (state == 0) state = 2463534242u ^ (g_Frame * 2654435761u);
    state ^= state << 13;
    state ^= state >> 17;
    state ^= state << 5;
    if (high < low) return low;
    const uint32_t range = static_cast<uint32_t>(high - low) + 1;
    return low + static_cast<int>(range ? state % range : state);
}
