// Example: Hello, Hyrule!
//
// Says hello when you start playing, then reports every 10 seconds. A quick
// way to check that your mod is running.
//
// To use it: copy everything below the "copy from here" line into my_mod.cpp,
// above every_frame(). Then add this line inside every_frame():
//
//     say_hello();
//
// How it works:
//   * A variable holds a number. hello_frames is written OUTSIDE the
//     function, so it keeps its value from one frame to the next.
//   * if runs some code only when something is true. == asks "is it equal?"
//   * % gives what's left over after dividing, so hello_frames % 300 is 0 on
//     every 300th frame.
//   * print() shows a message at the top of the screen.
//
// Ideas to try:
//   * Change the message.
//   * Report every 5 seconds instead of every 10.
//   * Print how many MINUTES you've been playing.

#include "botw.h"   // my_mod.cpp already has this line

// ---- copy from here ----

int hello_frames = 0;   // how many frames have gone by

void say_hello() {
    hello_frames = hello_frames + 1;

    if (hello_frames == 1) {
        print("Hello, Hyrule!");
    }

    // BotW runs at 30 frames a second, so 300 frames is about 10 seconds
    // (longer if the computer can't keep up).
    if (hello_frames % 300 == 0) {
        print("Still here after %d seconds", hello_frames / 30);
    }
}
