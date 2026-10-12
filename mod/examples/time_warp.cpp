// Example: Time warp
//
// Control the clock and the weather. Hold ZL (the Z key) and tap the D-pad
// (the arrow keys):
//   ZL + Up      time goes 4 times faster each tap
//   ZL + Left    skip ahead 3 hours
//   ZL + Right   make it rain
//   ZL + Down    back to normal
//
// To use it: copy everything below the "copy from here" line into my_mod.cpp,
// above every_frame(). Then add this line inside every_frame():
//
//     time_warp();
//
// How it works:
//   * held() is true the whole time a button is down; pressed() is true for
//     just one frame per tap. With held(UP), one tap would speed time up five
//     or six times in a row.
//   * && means "and": both sides have to be true.
//   * % wraps around: (22 + 3) % 24 is 1, so 22:00 plus 3 hours is 01:00.
//   * The game can overrule you. On the Great Plateau, before you leave it,
//     BotW stops its own clock at 11:00 - even with no mods at all.
//
// Ideas to try:
//   * Add ZL + L to go BACK 3 hours. (Careful: in C++, -1 % 24 is -1, not 23.)
//   * Make ZL + Right switch between rain, snow and clear skies.
//   * Make it always night.

#include "botw.h"   // my_mod.cpp already has this line

// ---- copy from here ----

float warp_speed = 1;   // the speed we asked for: 1 is normal

void time_warp() {
    if (held(ZL) && pressed(UP)) {
        warp_speed = warp_speed * 4;
        if (warp_speed > 256) {
            warp_speed = 1;
        }
        set_time_speed(warp_speed);
        print("Time speed x%.0f", warp_speed);
    }

    if (held(ZL) && pressed(LEFT)) {
        set_time((hour() + 3) % 24, minute());
        print("Skipped ahead to %02d:%02d", hour(), minute());
    }

    if (held(ZL) && pressed(RIGHT)) {
        force_weather("HeavyRain");
        print("Rain!");
    }

    if (held(ZL) && pressed(DOWN)) {
        warp_speed = 1;
        set_time_speed(1);
        stop_forcing_weather();
        print("Time and weather back to normal");
    }
}
