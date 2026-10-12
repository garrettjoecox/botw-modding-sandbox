// Example: Spawn rocks
//
// Press ZL + A (the Z and E keys) to drop a row of five boulders in front of
// Link.
//
// To use it: copy everything below the "copy from here" line into my_mod.cpp,
// above every_frame(). Then add this line inside every_frame():
//
//     spawn_rocks();
//
// How it works:
//   * A for loop repeats code. for (int i = 1; i <= 5; i++) runs its block five
//     times, with i counting 1, 2, 3, 4, 5.
//   * Each boulder goes a little further out: i * 3 metres.
//   * All five are asked for in the same frame. They queue up and appear one
//     per frame.
//
// More rocks: type  wxl actors rock  for the names. Not all of them are loose
// rocks - many are chunks of cliff or building that won't budge - so try a few.
//
// Ideas to try:
//   * Make a ring of rocks around Link, or a wall.
//   * Drop rocks from the sky with spawn_at(): use link_x(), link_y() + 20 and
//     link_z() for "20 metres above Link". (Stand back!)

#include "botw.h"   // my_mod.cpp already has this line

// ---- copy from here ----

void spawn_rocks() {
    if (held(ZL) && pressed(A)) {
        for (int i = 1; i <= 5; i++) {
            spawn("FldObj_PushRock_A_M_01", i * 3);
        }
        print("Rocks!");
    }
}
