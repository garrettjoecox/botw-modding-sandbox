// my_mod.cpp - your mod. This is the file you edit.
//
// The examples/ folder has ready-made pieces to copy in. Each one says what to
// paste, and the one line to add to every_frame() so it runs.
//
// After you change anything:  wxl build  then restart the game with  wxl play

#include "botw.h"

// Paste examples here, above every_frame().



// The game runs this every frame while you play: about 30 times a second.
void every_frame() {
}

// The game runs this just before it adds to one of its counters, such as your
// rupees. Whatever you return is what it really adds. (examples/double_rupees.cpp
// uses it.) Keep this function, even if you don't use it.
int when_game_adds(const char* counter, int amount) {
    return amount;
}
