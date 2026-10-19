// Example: Spawning wand
//
// The Spawning Wand is a new weapon this workspace adds to the game: a rod
// that doesn't shoot. Take it out and a menu of things to spawn appears; put
// it away (or take out the bow) and the menu goes.
//   Up / Down   choose
//   Right       open a category
//   Left        go back to the categories
//   swing       spawn the chosen thing, 5 metres in front of Link
// While the wand is out the game doesn't see the D-pad, so it won't open its
// quick menus or whistle. Put the wand away and the D-pad works as normal
// again, including Right to switch weapons.
// The wand never wears out: hitting things with it doesn't break it.
//
// Whenever Link has no wand and there's a free weapon slot, he's given one.
// If his weapon slots are full, a message says so once.
//
// The game only has the wand after running  wxl items  once (wxl setup does
// it too).
//
// To use it: copy everything below the "copy from here" line into my_mod.cpp,
// above every_frame(). Then add this line inside every_frame():
//
//     spawning_wand();
//
// How it works:
//   * has_item() and has_room_for() decide when to give a wand. ! means
//     "not", so !has_item(wand_item) is true when Link has none.
//   * The inventory takes a moment to show a new item, so after giving one
//     the example waits 10 frames (wand_wait counts them down) before
//     checking again; otherwise it could give two.
//   * same_text() checks whether the equipped sword is the wand, and
//     sword_drawn() whether Link is holding it out. If either isn't true, the
//     function returns before the menu, and nothing else happens.
//   * swung() is true on the frame Link swings, whether he hits anything or
//     not.
//   * weapon_changed() is true on the frame the wand is equipped. Its
//     durability then is remembered, and put back every frame after, so it
//     never wears down.
//   * A struct bundles values together: a WandThing is a label for the menu
//     plus the game's name for the thing, and a WandCategory is a label plus
//     a list of WandThings. -> reads a field through a pointer.
//   * menu_up_down() and draw_menu() (in botw.h) move and draw the menu.
//   * The wand itself isn't C++: tools/new_items.py builds it from the game's
//     own files, as a copy of the Ice Rod with a plain weapon's brain.
//
// What's been tested: in the game, the menu stays hidden while the wand is on
// Link's back, appears when he takes it out, and goes when he takes out the
// bow. sword_drawn() goes by where the wand is, measured while standing,
// walking, swinging and aiming the bow; climbing, swimming and gliding
// weren't measured. Before that: Link is given one wand (and only one),
// and has_room_for() says no once his 8 weapon slots are full. The "no room"
// message itself hasn't been seen. Before that: in the game, the wand shows in the inventory with its
// name and description, equipping it shows a flat list (before categories
// were added), and a swing spawns the chosen thing without firing ice. The
// categories haven't been tried in the game yet. The first press of Y only takes the wand
// out and doesn't count as a swing. Keeping it from wearing out hasn't been
// tried in the game yet.
//
// Ideas to try:
//   * Add your own category, or more things to a list (wxl actors finds names).
//   * Make a category that spawns things giant, with spawn_scaled().
//   * Spawn three in a row with each swing.
//   * Use a different weapon instead: change wand_item (wxl actors Weapon_Sword
//     lists them).

#include "botw.h"   // my_mod.cpp already has this line

// ---- copy from here ----

const char* const wand_item = "Weapon_Sword_900";   // the Spawning Wand

struct WandThing {
    const char* label;   // what the menu shows
    const char* actor;   // the game's name for it
};

const WandThing wand_enemies[] = {
    {"Bokoblin", "Enemy_Bokoblin_Junior"},
    {"Blue Bokoblin", "Enemy_Bokoblin_Middle"},
    {"Black Bokoblin", "Enemy_Bokoblin_Senior"},
    {"Moblin", "Enemy_Moriblin_Junior"},
    {"Blue Moblin", "Enemy_Moriblin_Middle"},
    {"Lizalfos", "Enemy_Lizalfos_Junior"},
    {"Blue Lizalfos", "Enemy_Lizalfos_Middle"},
    {"Chuchu", "Enemy_Chuchu_Junior"},
    {"Keese", "Enemy_Keese"},
    {"Forest Octorok", "Enemy_Octarock_Forest"},
    {"Fire Wizzrobe", "Enemy_Wizzrobe_Fire"},
    {"Ice Wizzrobe", "Enemy_Wizzrobe_Ice"},
    {"Stalkoblin", "Enemy_Bokoblin_Bone_Junior"},
    {"Yiga Footsoldier", "Enemy_Assassin_Junior"},
};

const WandThing wand_big_enemies[] = {
    {"Hinox", "Enemy_Giant_Junior"},
    {"Stone Talus", "Enemy_Golem_Junior"},
    {"Stone Pebblit", "Enemy_Golem_Little"},
    {"Lynel", "Enemy_Lynel_Junior"},
    {"Guardian Stalker", "Enemy_Guardian_A"},
    {"Guardian Scout I", "Enemy_Guardian_Mini_Baby"},
};

const WandThing wand_animals[] = {
    {"Cucco", "Animal_Kokko_A"},
    {"Hylian Retriever", "Animal_Dog_A"},
    {"Woodland Boar", "Animal_Boar_A"},
    {"Mountain Buck", "Animal_Deer_A"},
    {"Honeyvore Bear", "Animal_Bear_A"},
    {"Hateno Cow", "Animal_Cow_A"},
    {"White Goat", "Animal_Goat_A"},
    {"Highland Sheep", "Animal_Sheep_A"},
    {"Grassland Fox", "Animal_Fox_A"},
    {"Maraudo Wolf", "Animal_Wolf_A"},
};

const WandThing wand_horses[] = {
    {"Horse", "GameRomHorse"},
    {"Giant Horse", "GameRomHorse00L"},
    {"Donkey", "GameRomHorse00S"},
    {"Lord of the Mountain", "GameRomHorseNushi"},
    {"White Horse", "GameRomHorseZelda"},
    {"Epona", "GameRomHorseEpona"},
    {"Stalhorse", "GameRomHorseBone"},
    {"Master Cycle Zero", "GameROMMotorcycle"},
};

const WandThing wand_weapons[] = {
    {"Traveler's Sword", "Weapon_Sword_001"},
    {"Soldier's Broadsword", "Weapon_Sword_002"},
    {"Knight's Broadsword", "Weapon_Sword_003"},
    {"Master Sword", "Weapon_Sword_070"},
    {"Traveler's Claymore", "Weapon_Lsword_001"},
    {"Traveler's Spear", "Weapon_Spear_001"},
    {"Traveler's Bow", "Weapon_Bow_001"},
    {"Wooden Shield", "Weapon_Shield_001"},
};

const WandThing wand_food[] = {
    {"Apple", "Item_Fruit_A"},
    {"Hearty Durian", "Item_Fruit_D"},
    {"Hylian Shroom", "Item_Mushroom_E"},
    {"Raw Meat", "Item_Meat_01"},
};

const WandThing wand_objects[] = {
    {"Boulder", "FldObj_PushRock_A_M_01"},
    {"White rock", "Obj_LiftRockWhite_A_01"},
};

struct WandCategory {
    const char* label;
    const WandThing* things;   // its list
    int count;                 // how many things are in it
};

const WandCategory wand_categories[] = {
    {"Enemies", wand_enemies, count_of(wand_enemies)},
    {"Big enemies", wand_big_enemies, count_of(wand_big_enemies)},
    {"Animals", wand_animals, count_of(wand_animals)},
    {"Horses", wand_horses, count_of(wand_horses)},
    {"Weapons", wand_weapons, count_of(wand_weapons)},
    {"Food", wand_food, count_of(wand_food)},
    {"Objects", wand_objects, count_of(wand_objects)},
};

bool wand_in_category = false;   // false: showing the categories
int wand_category = 0;           // the chosen category
int wand_choice = 0;             // the chosen thing inside it
int wand_life = 0;               // the wand's durability when it was equipped
int wand_wait = 0;               // frames to wait before checking for a wand again
bool wand_said_no_room = false;  // has the "no room" message been shown?

// Fills lines with what the menu shows right now, and gives back how many.
int wand_menu_lines(const char** lines) {
    if (wand_in_category) {
        const WandCategory* category = &wand_categories[wand_category];
        for (int i = 0; i < category->count; i++) lines[i] = category->things[i].label;
        return category->count;
    }
    for (int i = 0; i < count_of(wand_categories); i++) lines[i] = wand_categories[i].label;
    return count_of(wand_categories);
}

// Gives Link a wand if he has none and there's room for one.
void give_wand_if_needed() {
    if (wand_wait > 0) {
        wand_wait = wand_wait - 1;
        return;
    }
    if (has_item(wand_item)) {
        wand_said_no_room = false;
        return;
    }
    if (!has_room_for(wand_item)) {
        if (!wand_said_no_room) {
            print("No room for a Spawning Wand - make a free weapon slot");
            wand_said_no_room = true;
        }
        return;
    }
    give_item(wand_item, 1400);   // 1400 is a new one's durability
    print("Here's a Spawning Wand!");
    wand_said_no_room = false;
    wand_wait = 10;               // frames
}

void spawning_wand() {
    give_wand_if_needed();

    if (!has_weapon(SWORD) || !same_text(weapon_name(SWORD), wand_item)) {
        return;   // not holding the wand
    }

    // Keep the wand from wearing out.
    if (weapon_changed(SWORD)) {
        wand_life = durability(SWORD);
    }
    set_durability(SWORD, wand_life);

    if (!sword_drawn()) {
        return;   // the wand is on Link's back, or he has the bow out
    }

    // The D-pad belongs to the wand while it's out.
    hide_dpad_from_game();

    const char* lines[32];   // room for 32 lines per list
    int count = wand_menu_lines(lines);

    if (wand_in_category) {
        wand_choice = menu_up_down(wand_choice, count);
    } else {
        wand_category = menu_up_down(wand_category, count);
    }

    if (pressed(RIGHT) && !wand_in_category) {
        play_sound("mc_Key_Decide");
        wand_in_category = true;   // open the chosen category
        wand_choice = 0;
    }
    if (pressed(LEFT) && wand_in_category) {
        play_sound("mc_Key_On");
        wand_in_category = false;  // back to the categories
    }

    if (swung() && wand_in_category) {
        const WandThing* thing = &wand_categories[wand_category].things[wand_choice];
        spawn(thing->actor, 5);
        print("The wand made a %s", thing->label);
    }

    count = wand_menu_lines(lines);   // the level may have just changed
    if (wand_in_category) {
        draw_menu(wand_categories[wand_category].label, lines, count, wand_choice, nullptr,
                  "Swing: spawn   Left: back");
    } else {
        draw_menu("Spawning Wand", lines, count, wand_category, nullptr, "Right: open");
    }
}
