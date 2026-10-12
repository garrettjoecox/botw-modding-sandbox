// Example: The Giant Horse
//
// Press ZL + B (the Z and Left Shift keys) to bring the Giant Horse - the huge
// black horse with the red mane - 5 metres in front of Link. It's wild, like
// the horses you find out in the world.
//
// Stand somewhere open and flat. The first time we tried this, on a hilltop,
// the horse landed past the edge and fell off the cliff.
//
// To use it: copy everything below the "copy from here" line into my_mod.cpp,
// above every_frame(). Then add this line inside every_frame():
//
//     giant_horse();
//
// How it works:
//   * Horses are actors too. The game calls the Giant Horse "GameRomHorse00L"
//     (L for large); type  wxl actors horse  to see the others.
//
// Other horses:
//   GameRomHorseNushi   Lord of the Mountain   GameRomHorseZelda   White Horse
//   GameRomHorse00S     Donkey                 GameRomHorseEpona   Epona
//
// Ideas to try:
//   * A different horse each press (see spawn_enemies.cpp for picking at random).
//   * A Lord of the Mountain herd: five at once, spread out.

#include "botw.h"   // my_mod.cpp already has this line

// ---- copy from here ----

void giant_horse() {
    if (held(ZL) && pressed(B)) {
        spawn("GameRomHorse00L", 5);
        print("The Giant Horse!");
    }
}
