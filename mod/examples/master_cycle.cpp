// Example: Master Cycle Zero
//
// Press ZL + ZR (the Z and V keys) to bring the Master Cycle Zero, the
// motorbike from the Champions' Ballad DLC, 3 metres in front of Link. Walk up
// to it and press A to climb on.
//
// To use it: copy everything below the "copy from here" line into my_mod.cpp,
// above every_frame(). Then add this line inside every_frame():
//
//     master_cycle();
//
// How it works:
//   * In the game you summon the bike with a rune; here it's simply spawned,
//     like the Giant Horse. Its actor name is "GameROMMotorcycle".
//   * The game builds the bike on its horse code, so while riding you'll see
//     horse prompts like "Soothe" and "Yah!".
//   * Its model is in the game's v208 update, but it has only been tried with
//     the DLC installed.
//
// Ideas to try:
//   * Spawn a row of bikes (see spawn_rocks.cpp for a for loop).
//   * Give Link the rune instead: set_flag("IsGet_Obj_Motorcycle", true). (See
//     slate_abilities.cpp for why runes might not show up early in the game.)

#include "botw.h"   // my_mod.cpp already has this line

// ---- copy from here ----

void master_cycle() {
    if (held(ZL) && pressed(ZR)) {
        spawn("GameROMMotorcycle", 3);
        print("Master Cycle Zero!");
    }
}
