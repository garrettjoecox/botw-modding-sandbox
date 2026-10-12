// Example: Starter kit
//
// Press ZL + R (the Z and R keys) for a sword, a bow, a shield and arrows,
// all equipped.
//
// To use it: copy everything below the "copy from here" line into my_mod.cpp,
// above every_frame(). Then add this line inside every_frame():
//
//     starter_kit();
//
// How it works:
//   * Items are named the way the game names them, as text in double quotes.
//   * give_and_equip() is a function with parameters: it bundles two steps,
//     so starter_kit() reads like a shopping list.
//   * Weapons always arrive brand new, so their amount is just 1. The amount
//     matters for things that stack, like arrows.
//   * The game equips things a frame or two after it's asked to.
//
// Some item names (wxl actors finds the rest, e.g.  wxl actors shroom):
//   Weapon_Sword_001   Traveler's Sword       Weapon_Bow_001     Traveler's Bow
//   Weapon_Sword_002   Soldier's Broadsword   Weapon_Bow_002     Soldier's Bow
//   Weapon_Sword_070   Master Sword           Weapon_Shield_001  Wooden Shield
//   NormalArrow        Arrow                  FireArrow          Fire Arrow
//   Item_Fruit_A       Apple                  Item_Mushroom_E    Hylian Shroom
//
// Ideas to try:
//   * Change what's in the kit. Add 10 apples.
//   * Make the kit cost 50 rupees, and refuse if Link can't pay.
//   * Give a random weapon: random_number(1, 3) gives back 1, 2 or 3.

#include "botw.h"   // my_mod.cpp already has this line

// ---- copy from here ----

void give_and_equip(const char* item, int amount) {
    give_item(item, amount);
    equip(item);
}

void starter_kit() {
    if (held(ZL) && pressed(R)) {
        give_and_equip("Weapon_Sword_001", 1);    // Traveler's Sword
        give_and_equip("Weapon_Bow_001", 1);      // Traveler's Bow
        give_and_equip("Weapon_Shield_001", 1);   // Wooden Shield
        give_and_equip("NormalArrow", 20);        // 20 arrows
        print("Here's your starter kit!");
    }
}
