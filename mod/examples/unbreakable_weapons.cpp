// Example: Unbreakable weapons
//
// Swords, shields and bows never wear out.
//
// To use it: copy everything below the "copy from here" line into my_mod.cpp,
// above every_frame(). Then add this line inside every_frame():
//
//     unbreakable_weapons();
//
// How it works:
//   * It remembers each weapon's durability from earlier frames, notices when
//     the game lowers it, and puts it back.
//   * best_durability[3] is an array: three numbers in a row, one for each
//     kind of weapon - best_durability[SWORD], [SHIELD] and [BOW].
//   * protect_weapon(SWORD) and protect_weapon(BOW) run the same steps on
//     different weapons, because the weapon is a parameter.
//   * return leaves a function early. ! means "not".
//
// Ideas to try:
//   * Only protect the sword.
//   * Make each weapon get STRONGER whenever it would have worn down.
//   * Let weapons wear down, but never below half of their best.

#include "botw.h"   // my_mod.cpp already has this line

// ---- copy from here ----

int best_durability[3];   // the most each weapon has had

void protect_weapon(Weapon weapon) {
    if (!has_weapon(weapon)) {
        return;   // nothing equipped there
    }

    int now = durability(weapon);

    if (weapon_changed(weapon)) {
        // A different weapon: start from what it has now.
        best_durability[weapon] = now;
        print("Protecting %s (durability %d)", weapon_name(weapon), now);
        return;
    }

    if (now < best_durability[weapon]) {
        set_durability(weapon, best_durability[weapon]);   // it wore down: undo it
    }
    if (now > best_durability[weapon]) {
        best_durability[weapon] = now;   // the game repaired it: keep the new value
    }
}

void unbreakable_weapons() {
    protect_weapon(SWORD);
    protect_weapon(SHIELD);
    protect_weapon(BOW);
}
