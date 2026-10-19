// Example: Spawn menu
//
// Press R3 (click the right stick) to open a menu of things to spawn:
//   Up / Down   choose
//   Right       open a category, or spawn the thing that's chosen
//   Left        go back to the categories (from there, close the menu)
//   R3          close the menu
// Spawns land 5 metres in front of Link, and the menu stays open on the same
// line, so you can spawn as many as you like. While the menu is open the game
// doesn't see the D-pad, so it won't whistle or open its own quick menus. It
// never sees R3 at all while this is in use: R3 is normally the Sheikah
// Slate's scope, which would otherwise open with the menu.
//
// To use it: copy everything below the "copy from here" line into my_mod.cpp,
// above every_frame(). Then add this line inside every_frame():
//
//     spawn_menu();
//
// How it works:
//   * A struct bundles values together: a Spawnable is a label for the menu
//     plus the game's name for the thing. Each category is a list of them.
//   * The menu is a few variables - is it open, are we inside a category,
//     which line is chosen - and every frame the buttons change them and the
//     menu is drawn from them.
//   * menu_up_down() and draw_menu() (in botw.h) do the moving and the
//     drawing: holding Up or Down repeats, and long lists scroll.
//   * -> reads a field through a pointer: thing->actor is the actor of the
//     thing that thing points at.
//   * hide_from_game(R3) is called every frame, and hide_dpad_from_game()
//     every frame the menu is open. Hiding only lasts a moment, so it has to
//     be asked for again each frame.
//
// Ideas to try:
//   * Add your own category, or more things to a list (wxl actors finds names).
//   * Show the game's name for the chosen thing at the bottom of the panel.
//   * Make everything spawn giant with spawn_scaled().

#include "botw.h"   // my_mod.cpp already has this line

// ---- copy from here ----

struct Spawnable {
    const char* label;   // what the menu shows
    const char* actor;   // the game's name for it
};

const Spawnable menu_enemies[] = {
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

const Spawnable menu_big_enemies[] = {
    {"Hinox", "Enemy_Giant_Junior"},
    {"Stone Talus", "Enemy_Golem_Junior"},
    {"Stone Pebblit", "Enemy_Golem_Little"},
    {"Lynel", "Enemy_Lynel_Junior"},
    {"Guardian Stalker", "Enemy_Guardian_A"},
    {"Guardian Scout I", "Enemy_Guardian_Mini_Baby"},
};

const Spawnable menu_animals[] = {
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

const Spawnable menu_horses[] = {
    {"Horse", "GameRomHorse"},
    {"Giant Horse", "GameRomHorse00L"},
    {"Donkey", "GameRomHorse00S"},
    {"Lord of the Mountain", "GameRomHorseNushi"},
    {"White Horse", "GameRomHorseZelda"},
    {"Epona", "GameRomHorseEpona"},
    {"Stalhorse", "GameRomHorseBone"},
    {"Master Cycle Zero", "GameROMMotorcycle"},
};

const Spawnable menu_weapons[] = {
    {"Traveler's Sword", "Weapon_Sword_001"},
    {"Soldier's Broadsword", "Weapon_Sword_002"},
    {"Knight's Broadsword", "Weapon_Sword_003"},
    {"Master Sword", "Weapon_Sword_070"},
    {"Traveler's Claymore", "Weapon_Lsword_001"},
    {"Traveler's Spear", "Weapon_Spear_001"},
    {"Traveler's Bow", "Weapon_Bow_001"},
    {"Wooden Shield", "Weapon_Shield_001"},
};

const Spawnable menu_food[] = {
    {"Apple", "Item_Fruit_A"},
    {"Hearty Durian", "Item_Fruit_D"},
    {"Hylian Shroom", "Item_Mushroom_E"},
    {"Raw Meat", "Item_Meat_01"},
};

const Spawnable menu_objects[] = {
    {"Boulder", "FldObj_PushRock_A_M_01"},
    {"White rock", "Obj_LiftRockWhite_A_01"},
};

struct Category {
    const char* label;
    const Spawnable* items;   // its list
    int count;                // how many things are in it
};

const Category menu_categories[] = {
    {"Enemies", menu_enemies, count_of(menu_enemies)},
    {"Big enemies", menu_big_enemies, count_of(menu_big_enemies)},
    {"Animals", menu_animals, count_of(menu_animals)},
    {"Horses", menu_horses, count_of(menu_horses)},
    {"Weapons", menu_weapons, count_of(menu_weapons)},
    {"Food", menu_food, count_of(menu_food)},
    {"Objects", menu_objects, count_of(menu_objects)},
};

bool menu_open = false;
bool menu_in_category = false;   // false: showing the categories
int menu_category = 0;           // the chosen category
int menu_item = 0;               // the chosen thing inside it

// Fills lines with what the menu shows right now, and gives back how many.
int spawn_menu_lines(const char** lines) {
    if (menu_in_category) {
        const Category* category = &menu_categories[menu_category];
        for (int i = 0; i < category->count; i++) lines[i] = category->items[i].label;
        return category->count;
    }
    for (int i = 0; i < count_of(menu_categories); i++) lines[i] = menu_categories[i].label;
    return count_of(menu_categories);
}

void spawn_menu() {
    hide_from_game(R3);   // every frame: R3 belongs to this menu now
    if (pressed(R3)) {
        menu_open = !menu_open;
    }
    if (!menu_open) {
        return;
    }
    hide_dpad_from_game();

    const char* lines[32];   // room for 32 lines per list
    int count = spawn_menu_lines(lines);

    if (menu_in_category) {
        menu_item = menu_up_down(menu_item, count);
    } else {
        menu_category = menu_up_down(menu_category, count);
    }

    if (pressed(RIGHT)) {
        play_sound("mc_Key_Decide");
        if (!menu_in_category) {
            menu_in_category = true;   // open the chosen category
            menu_item = 0;
        } else {
            const Spawnable* thing = &menu_categories[menu_category].items[menu_item];
            spawn(thing->actor, 5);
            print("Spawned %s", thing->label);
        }
    }

    if (pressed(LEFT)) {
        play_sound("mc_Key_On");
        if (menu_in_category) {
            menu_in_category = false;   // back to the categories
        } else {
            menu_open = false;
            return;
        }
    }

    count = spawn_menu_lines(lines);   // the level may have just changed
    if (menu_in_category) {
        draw_menu(menu_categories[menu_category].label, lines, count, menu_item);
    } else {
        draw_menu("Spawn", lines, count, menu_category);
    }
}
