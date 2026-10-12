// Example: Double rupees
//
// Every rupee Link picks up counts twice. No rupees nearby? Press ZL + Minus
// (Z and M): the game "picks up" 5 rupees, and you should get 10. (Minus also
// opens the map - the game sees the button too.)
//
// This one needs THREE steps:
//   1. Copy everything below the "copy from here" line into my_mod.cpp, above
//      every_frame().
//   2. Add this line inside every_frame():
//
//          rupee_test_button();
//
//   3. Add this line inside when_game_adds(), before its "return amount;":
//
//          amount = double_rupees(counter, amount);
//
// How it works:
//   * This is a hook: the GAME calls your code. Whenever it adds to one of its
//     counters, it first asks when_game_adds() in my_mod.cpp, which now asks
//     double_rupees(). Whatever comes back is what the game really adds.
//   * return gives a value back to whoever called the function.
//   * same_text() compares two pieces of text. (== doesn't work for text in
//     C++: it checks whether they're stored in the same place.)
//   * Behind the scenes, engine/main.cpp redirects a function inside the
//     game's own code, found at a fixed address by reverse engineering.
//
// Ideas to try:
//   * Triple rupees, but only at night (is_night()).
//   * Find out what else the game counts: print every counter that changes.

#include "botw.h"   // my_mod.cpp already has this line

// ---- copy from here ----

int double_rupees(const char* counter, int amount) {
    if (same_text(counter, "CurrentRupee") && amount > 0) {
        print("Picked up %d rupees - you get %d", amount, amount * 2);
        return amount * 2;
    }
    return amount;   // anything else: leave it alone
}

void rupee_test_button() {
    if (held(ZL) && pressed(MINUS)) {
        game_add_to_counter("CurrentRupee", 5);
    }
}
