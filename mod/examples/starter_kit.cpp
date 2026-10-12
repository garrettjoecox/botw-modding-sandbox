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
//   * For a weapon, the amount is how much durability it has, in the game's
//     own units: a new Traveler's Sword has 2000. Give it 1 and it breaks on
//     the first hit. For things that stack, like arrows, it's how many.
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
//   * Find another weapon's full durability: equip a new one and print
//     durability(SWORD) - or paste in unbreakable_weapons.cpp, which prints it.
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
        give_and_equip("Weapon_Sword_001", 2000);   // Traveler's Sword, new
        give_and_equip("Weapon_Bow_001", 2200);     // Traveler's Bow, new
        give_and_equip("Weapon_Shield_001", 1200);  // Wooden Shield, new
        give_and_equip("NormalArrow", 20);          // 20 arrows
        print("Here's your starter kit!");
    }
}
