// Example: Jetpack
//
// Hold ZL + X (the Z key and Space) to fly up. The jetpack stops climbing 15
// metres above where you took off. Keep holding ZL and let go of X to float
// back down on a parachute.
//
// To use it: copy everything below the "copy from here" line into my_mod.cpp,
// above every_frame(). Then add this line inside every_frame():
//
//     jetpack();
//
// How it works:
//   * move_link() pushes Link for one frame. Keep calling it, frame after
//     frame, and he keeps moving; stop, and gravity takes over.
//   * Speed is in metres per second. Link is about 1.7 metres tall.
//   * takeoff_y remembers where the flight started, to measure from it.
//   * BotW hurts Link by how FAR he fell, not by how fast he lands, so even a
//     slow parachute landing from high up hurts. That's why there's a limit.
//
// Ideas to try:
//   * Raise the limit to 40 metres. What happens when Link lands?
//   * Then protect him: remember his hearts before he lands, and put back any
//     he loses. (hearts(), set_hearts(); falling_speed() is 0 once he's down.)
//   * Fly forwards: the x and z numbers push Link east-west and north-south.

#include "botw.h"   // my_mod.cpp already has this line

// ---- copy from here ----

float takeoff_y = 0;   // how high Link was when the jetpack started

void jetpack() {
    if (held(ZL) && pressed(X)) {
        takeoff_y = link_y();
    }

    float height = link_y() - takeoff_y;   // metres above the take-off point

    if (held(ZL) && held(X)) {
        if (height < 15) {
            move_link(0, 12, 0);   // up, at 12 metres per second
        } else {
            move_link(0, 0, 0);    // at the limit: hover
        }
        draw_text(560, 420, "%.0f m up", height);
    }

    // The parachute: when Link is falling faster than 3 metres per second,
    // slow him down to 3.
    if (held(ZL) && !held(X) && falling_speed() > 3) {
        move_link(0, -3, 0);
        draw_text(560, 420, "Parachute: %.0f m up", height);
    }
}
