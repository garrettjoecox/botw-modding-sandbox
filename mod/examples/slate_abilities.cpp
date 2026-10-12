// Example: Sheikah Slate abilities
//
// Press ZL + L (the Z and Q keys) to unlock the Slate's abilities: the four
// runes (Remote Bomb, Magnesis, Stasis, Cryonis), the Camera and the Sheikah
// Sensor.
//
// To use it: copy everything below the "copy from here" line into my_mod.cpp,
// above every_frame(). Then add this line inside every_frame():
//
//     slate_abilities();
//
// How it works:
//   * Runes aren't items in the inventory. The game remembers which abilities
//     Link has as yes/no "flags", and when a shrine hands over the runes, its
//     cutscene just switches flags on - IsGet_Obj_Magnetglove for Magnesis, and
//     so on. set_flag() does the same thing.
//   * A flag change lands on the next frame, so flag() read straight after
//     set_flag() still shows the old value.
//
// What's been tested: on a save from the very start of the Great Plateau
// (before its tower), the Sheikah Sensor appeared on the HUD straight away, but
// the four runes did NOT show up in the rune menu (hold Up), even after saving
// and reloading. The game seems to want something more that early in the
// story. It hasn't been tried on a save from later in the game.
//
// Flags are part of the story, so back up your save before trying this.
//
// Ideas to try:
//   * The upgrades Purah gives in Hateno: IsGet_Obj_RemoteBombLv2,
//     IsGet_Obj_StopTimerLv2 and IsGet_Obj_SheikSensorLv2.
//   * Show which abilities Link has with flag("IsGet_Obj_Magnetglove") and
//     draw_text() (see hud.cpp).

#include "botw.h"   // my_mod.cpp already has this line

// ---- copy from here ----

const char* slate_flags[] = {
    "IsGet_Obj_RemoteBomb",    // Remote Bomb
    "IsGet_Obj_Magnetglove",   // Magnesis
    "IsGet_Obj_StopTimer",     // Stasis
    "IsGet_Obj_IceMaker",      // Cryonis
    "IsGet_Obj_Camera",        // Camera
    "IsGet_Obj_SheikSensor",   // Sheikah Sensor
};

void slate_abilities() {
    if (held(ZL) && pressed(L)) {
        for (int i = 0; i < 6; i++) {
            set_flag(slate_flags[i], true);
        }
        print("Sheikah Slate abilities unlocked");
    }
}
