// botw.h - everything your mod can ask the game to do.
//
// my_mod.cpp starts with  #include "botw.h"  so it can use what's listed
// here. You only need to know what each function does, not how. (The "how" is
// in the engine/ folder, for when you're curious.)
//
// A few words you'll see:
//   void   the function doesn't give anything back; it just does something
//   int    a whole number: 3, 0, -12
//   float  a number with a decimal point: 2.5, 0.75
//   bool   true or false
//   const char*   text, written in double quotes: "Hello"
#pragma once

// (Lets the compiler check that your %d and %f match the values you pass.)
#define BOTW_CHECK_FORMAT(fmt, args) __attribute__((format(printf, fmt, args)))

// =============================================================================
// Functions YOU write - the game calls them
// =============================================================================

// Runs once every frame while you're playing: about 30 times a second, or
// fewer if the computer can't keep up. Not on the title screen, in loading
// screens, or while the game is paused. Lives in my_mod.cpp.
void every_frame();

// Runs whenever the game adds to one of its counters, such as rupees. Give
// back (return) the amount the game should really add. Lives in my_mod.cpp;
// examples/double_rupees.cpp uses it.
int when_game_adds(const char* counter, int amount);

// =============================================================================
// Printing messages
// =============================================================================

// Shows a message at the top of the screen for a few seconds, and saves it in
// the log (see it later with: wxl log). Works like C's printf:
//
//   print("Hello!");
//   print("I have %d rupees", rupees());         %d  = a whole number
//   print("Stamina: %.1f wheels", stamina());    %.1f = a decimal, 1 digit after the point
//   print("The weather is %s", weather_name());  %s  = text
void print(const char* format, ...) BOTW_CHECK_FORMAT(1, 2);

// =============================================================================
// Buttons
// =============================================================================
//
// On the keyboard: ZL = Z, X = Space, R = R, Minus = M, D-pad = arrow keys.
// The full list is in the top-level README.

enum Button { A, B, X, Y, L, R, ZL, ZR, PLUS, MINUS, UP, DOWN, LEFT, RIGHT };

bool held(Button button);     // true the whole time the button is down
bool pressed(Button button);  // true only on the frame it goes down - once per press

// =============================================================================
// Link
// =============================================================================

float hearts();                 // health in hearts: 2.5 = two and a half hearts
float max_hearts();             // how many heart containers Link has
void  set_hearts(float hearts);

float stamina();                // in wheels: 1.0 = one full stamina wheel
float max_stamina();
void  set_stamina(float wheels);

float link_x();                 // where Link is standing, in metres.
float link_y();                 // y is height above sea level;
float link_z();                 // x and z are east-west and north-south.

// Makes Link fly at this speed, in metres per second: y is up, x is east and
// z is south. It lasts one frame, so call it every frame you want it. (Made
// for flying: on the ground the push is weaker.) See examples/jetpack.cpp.
void move_link(float x, float y, float z);

float falling_speed();          // how fast Link is dropping, in metres per second (0 = not falling)

// =============================================================================
// Spawning things
// =============================================================================
//
// Puts something new into the world: an enemy, an animal, a horse, a rock...
// Use the game's own name for it, like "Enemy_Bokoblin_Junior" (a Bokoblin).
// To look names up, type  wxl actors  (every name), or e.g.  wxl actors hinox
//
// It appears a moment later. Spawn several at once and they come out one per
// frame. See examples/spawn_enemies.cpp, spawn_rocks.cpp and giant_horse.cpp.

bool spawn(const char* name, float distance);                // that many metres in front of Link
bool spawn_at(const char* name, float x, float y, float z);  // at a spot in the world (see link_x())

// =============================================================================
// Weapons Link is holding
// =============================================================================

enum Weapon { SWORD, SHIELD, BOW };

bool        has_weapon(Weapon weapon);       // is one equipped?
const char* weapon_name(Weapon weapon);      // its item name, like "Weapon_Sword_001"
int         durability(Weapon weapon);       // how worn it is; a new Traveler's Bow has 2200
void        set_durability(Weapon weapon, int durability);
bool        weapon_changed(Weapon weapon);   // true on the frame Link equips a different one

// =============================================================================
// Inventory and rupees
// =============================================================================
//
// Items are named the way the game names them: "Weapon_Sword_001" is the
// Traveler's Sword. examples/starter_kit.cpp lists a few, and  wxl actors
// looks up the rest (e.g.  wxl actors apple).

// amount = how many. For a weapon it's the durability instead, in the game's
// units: a new Traveler's Sword has 2000. (Give a weapon 1 and it breaks on the
// first hit.)
bool give_item(const char* name, int amount);
bool equip(const char* name);                  // equips an item Link already has

int  rupees();
void set_rupees(int amount);
void give_rupees(int amount);                  // negative takes some away; rupees() shows it next frame

// =============================================================================
// Time and weather
// =============================================================================

int   hour();                         // 0 to 23
int   minute();                       // 0 to 59
void  set_time(int hour, int minute);
float time_speed();                   // 1 = normal speed
void  set_time_speed(float speed);    // 2 = twice as fast, 0.5 = half speed
bool  is_night();

float temperature();                  // in degrees Celsius, where Link is

// Weather names: "Bluesky", "Cloudy", "Rain", "HeavyRain", "Snow",
// "HeavySnow", "ThunderStorm", "ThunderRain", "BlueskyRain".
const char* weather_name();           // the weather right now
void        force_weather(const char* name);  // keep this weather until told to stop
void        stop_forcing_weather();

// =============================================================================
// Things that just happened
// =============================================================================
//
// Each one is true for a single frame, right after it happens. See
// examples/exploration_rewards.cpp.

bool found_korok();       // Link just got a Korok seed
bool finished_shrine();   // Link just finished a shrine
bool activated_tower();   // Link just activated a Sheikah tower

int  korok_seeds();       // how many Korok seeds Link has found so far

// =============================================================================
// Game flags
// =============================================================================
//
// The game remembers thousands of yes/no facts by name: which runes Link has
// ("IsGet_Obj_Magnetglove"), which shrines are done, what people have told him.
// Changing the wrong one can confuse a quest, so back up your save first.
// See examples/slate_abilities.cpp.

bool flag(const char* name);                 // is it on?
bool set_flag(const char* name, bool on);    // false if the game refused

// =============================================================================
// Drawing on the screen
// =============================================================================
//
// The screen is 1280 wide and 720 tall, whatever size your window is.
// (0, 0) is the top-left corner; y gets bigger going DOWN.
//
// Draw every frame you want something to show; it disappears when you stop.

using Color = unsigned int;           // written 0xRRGGBBAA, e.g. 0xFF0000FF is red

const Color WHITE  = 0xFFFFFFFF;
const Color BLACK  = 0x000000FF;
const Color RED    = 0xFF4040FF;
const Color GREEN  = 0x40FF40FF;
const Color BLUE   = 0x4080FFFF;
const Color YELLOW = 0xFFE040FF;
const Color CREAM  = 0xFFFCC6FF;      // the colour of BotW's own text
const Color SHADOW = 0x000000A0;      // see-through black, for backgrounds

void draw_text(float x, float y, const char* format, ...) BOTW_CHECK_FORMAT(3, 4);
void draw_colored_text(float x, float y, Color color, const char* format, ...) BOTW_CHECK_FORMAT(4, 5);
void draw_box(float x, float y, float width, float height, Color color);

// =============================================================================
// Sounds
// =============================================================================
//
// Plays one of the game's own sound effects by name, e.g. "mc_Key_Decide".
// Only a small set of about 51 sounds can be reached this way (menus, the
// keyboard, the title screen, upgrades); examples/button_sounds.cpp lists them.

bool play_sound(const char* name);   // false if the game has no sound by that name

// =============================================================================
// Handy extras
// =============================================================================

bool same_text(const char* a, const char* b);  // are these two pieces of text the same?
int  random_number(int low, int high);         // a random whole number from low to high

// Makes the game add to one of its counters itself, the same way it does when
// you pick something up. See examples/double_rupees.cpp.
void game_add_to_counter(const char* counter, int amount);
