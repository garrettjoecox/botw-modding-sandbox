// Example: Infinite stamina
//
// Stamina never runs out: sprint, climb and swim as long as you like.
//
// To use it: copy everything below the "copy from here" line into my_mod.cpp,
// above every_frame(). Then add this line inside every_frame():
//
//     infinite_stamina();
//
// How it works:
//   * stamina() GIVES BACK Link's stamina right now, so it can be used like a
//     number. set_stamina() CHANGES it.
//   * < means "less than".
//   * The game counts stamina in wheels: 1.0 is one full wheel, 0.5 is half.
//
// Ideas to try:
//   * Only refill when stamina drops below half a wheel.
//   * Refill hearts too: hearts(), max_hearts() and set_hearts().
//   * What happens with set_stamina(3)?

#include "botw.h"   // my_mod.cpp already has this line

// ---- copy from here ----

void infinite_stamina() {
    if (stamina() < max_stamina()) {
        set_stamina(max_stamina());
    }
}
