// Example: Your own HUD
//
// A panel in the bottom-left corner showing the time, temperature, weather,
// hearts, rupees and stamina.
//
// To use it: copy everything below the "copy from here" line into my_mod.cpp,
// above every_frame(). Then add this line inside every_frame():
//
//     show_hud();
//
// How it works:
//   * The screen is a grid 1280 wide and 720 tall, with (0, 0) in the top-left
//     corner. A bigger y is further DOWN the screen.
//   * draw_box() and draw_text() draw for one frame. Calling them every frame
//     keeps them on screen.
//   * Numbers go into text the same way as with print(): %d, %.1f and %s.
//     %02d means "at least 2 digits, with a 0 in front", so 9 shows as 09.
//
// Ideas to try:
//   * Move the panel somewhere else by changing x and y.
//   * Add a line showing how high Link is: link_y().
//   * Show the hearts line in RED when Link has less than 2 hearts.
//     (draw_colored_text works like draw_text, with a colour.)

#include "botw.h"   // my_mod.cpp already has this line

// ---- copy from here ----

void show_hud() {
    // Bottom-left: the corner BotW's own HUD leaves empty.
    float x = 20;
    float y = 550;

    draw_box(x, y, 320, 150, SHADOW);

    draw_text(x + 20, y + 10, "%02d:%02d   %.0f C", hour(), minute(), temperature());
    draw_text(x + 20, y + 40, "Weather: %s", weather_name());
    draw_text(x + 20, y + 70, "Hearts: %.2f / %.0f", hearts(), max_hearts());
    draw_text(x + 20, y + 100, "Rupees: %d   Stamina: %.1f", rupees(), stamina());
}
