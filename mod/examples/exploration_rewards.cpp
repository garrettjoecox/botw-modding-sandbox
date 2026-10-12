// Example: Exploration rewards
//
// Earn rupees for finding Korok seeds, finishing shrines and activating
// Sheikah towers.
//
// To use it: copy everything below the "copy from here" line into my_mod.cpp,
// above every_frame(). Then add this line inside every_frame():
//
//     exploration_rewards();
//
// How it works:
//   * found_korok() is true for just one frame, right after Link gets a seed,
//     so each reward is paid once.
//   * give_rupees() goes through the game's own rupee counter, so the number on
//     screen goes up.
//
// Ideas to try:
//   * Pay more for each seed as the total goes up (korok_seeds()).
//   * Give a weapon for each shrine instead of rupees (see starter_kit.cpp).
//   * Pay double at night.

#include "botw.h"   // my_mod.cpp already has this line

// ---- copy from here ----

void exploration_rewards() {
    if (found_korok()) {
        give_rupees(50);
        print("Korok seed number %d! +50 rupees", korok_seeds());
    }

    if (finished_shrine()) {
        give_rupees(300);
        print("Shrine complete! +300 rupees");
    }

    if (activated_tower()) {
        give_rupees(100);
        print("Tower activated! +100 rupees");
    }
}
