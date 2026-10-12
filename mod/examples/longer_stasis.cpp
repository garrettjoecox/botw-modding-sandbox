// Example: Longer Stasis
//
// Makes the Stasis rune freeze things for three times as long.
//
// To use it: copy everything below the "copy from here" line into my_mod.cpp,
// above every_frame(). Then add this line inside every_frame():
//
//     longer_stasis();
//
// How it works:
//   * How long Stasis lasts isn't in the game's code. It's three numbers in
//     Link's settings file (Player_Link.bgparamlist), counted in frames, 30 to
//     a second:
//         StopTimerLongTime    300  (10 seconds)
//         StopTimerMiddleTime  150  (5 seconds)
//         StopTimerShortTime    60  (2 seconds)
//   * The game loads that file into memory when it starts, and
//     set_link_setting() changes the copy in memory. Nothing is saved: restart
//     the game and they're back to normal, so this sets them every frame.
//
// What's been tested: the three numbers really change in the game's memory
// (read back as 900, 450 and 180). Stasis itself hasn't been tried with them,
// because the test saves don't have the rune yet - it comes from a shrine on
// the Great Plateau.
//
// Ideas to try:
//   * Make Stasis last a whole minute.
//   * StopTimerReloadTime is also 300. It's probably how long the rune takes to
//     recharge - try making it shorter.
//   * Print the settings first with link_setting("StopTimerLongTime").

#include "botw.h"   // my_mod.cpp already has this line

// ---- copy from here ----

void longer_stasis() {
    set_link_setting("StopTimerLongTime", 900);     // 30 seconds (normally 10)
    set_link_setting("StopTimerMiddleTime", 450);   // 15 seconds (normally 5)
    set_link_setting("StopTimerShortTime", 180);    // 6 seconds (normally 2)
}
