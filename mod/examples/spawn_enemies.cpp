// Example: Spawn enemies
//
// Press ZL + Y (the Z and F keys) to spawn a random enemy 6 metres in front of
// Link. Careful: they fight back!
//
// To use it: copy everything below the "copy from here" line into my_mod.cpp,
// above every_frame(). Then add this line inside every_frame():
//
//     spawn_enemies();
//
// How it works:
//   * enemies[] is an array of text: a list of names, numbered from 0. With six
//     names, enemies[0] is the first and enemies[5] is the last.
//   * random_number(0, 5) picks one of those numbers.
//   * spawn() takes the game's own name for something. To find more names,
//     type  wxl actors  - or e.g.  wxl actors lizalfos  for just those.
//
// Bigger ones to try (tough with only a few hearts):
//   Enemy_Giant_Junior      Hinox           Enemy_Golem_Junior    Stone Talus
//   Enemy_Lynel_Junior      Lynel           Enemy_Wizzrobe_Fire   Fire Wizzrobe
//
// Ideas to try:
//   * Add more enemies to the list. (Then change the 5 to match!)
//   * Spawn a whole gang: three of the same enemy at once.
//   * Charge 20 rupees per enemy, and refuse if Link can't pay.
//   * Make them giants: spawn_scaled(enemies[pick], 10, 3) spawns one three
//     times the normal size.

#include "botw.h"   // my_mod.cpp already has this line

// ---- copy from here ----

const char* enemies[] = {
    "Enemy_Bokoblin_Junior",   // Bokoblin
    "Enemy_Bokoblin_Middle",   // Blue Bokoblin
    "Enemy_Moriblin_Junior",   // Moblin
    "Enemy_Lizalfos_Junior",   // Lizalfos
    "Enemy_Chuchu_Junior",     // Chuchu
    "Enemy_Octarock_Forest",   // Forest Octorok
};

void spawn_enemies() {
    if (held(ZL) && pressed(Y)) {
        int pick = random_number(0, 5);
        spawn(enemies[pick], 6);
        print("Here comes %s!", enemies[pick]);
    }
}
