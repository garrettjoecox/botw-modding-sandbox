// Example: Ability menu
//
// Press R3 (click the right stick) to open a menu that switches Link's
// abilities on and off: the Sheikah Slate's runes and their upgrades, the
// Champions' abilities, the paraglider and more.
//   Up / Down   choose
//   Right       open a category, or switch the chosen ability on or off
//   Left        go back to the categories (from there, close the menu)
//   R3          close the menu
// Each line shows ON or off. While the menu is open the game doesn't see the
// D-pad, and it never sees R3 at all while this is in use - R3 is normally the
// Sheikah Slate's scope, which would otherwise open with the menu.
//
// To use it: copy everything below the "copy from here" line into my_mod.cpp,
// above every_frame(). Then add this line inside every_frame():
//
//     ability_menu();
//
// How it works:
//   * The game remembers which abilities Link has as yes/no flags, like
//     IsGet_Obj_Magnetglove for Magnesis. Some are also an item in the
//     inventory's key items (the Champions' abilities, the paraglider), so
//     those are given and taken away too.
//   * flag() reads each one for the ON / off column. A flag changes on the
//     next frame, so the column catches up a moment after you press Right.
//   * It's the same kind of menu as spawn_menu.cpp, with different names so
//     both can be pasted in at once.
//
// What's been tested, on a save from the start of the Great Plateau: the menu
// and its ON / off column; switching on the Sheikah Sensor (its gauge appeared
// on the HUD), Revali's Gale (its key item arrived and its HUD icon showed 3
// charges) and the Paraglider (its key item arrived). Actually using them
// wasn't tried. The four runes are known not to show up in the rune menu this
// early in the story - see slate_abilities.cpp.
//
// Back up your save before switching abilities on: they're part of the story.
//
// Ideas to try:
//   * Add the Champions' DLC upgrades: IsGet_Obj_DLC_HeroSoul_Rito and
//     Obj_DLC_HeroSoul_Rito (Revali's Gale +), and so on.
//   * Add an "everything on" line to each category.

#include "botw.h"   // my_mod.cpp already has this line

// ---- copy from here ----

struct Ability {
    const char* label;   // what the menu shows
    const char* flag;    // the game's flag for it
    const char* item;    // its key item, or "" if it has none
};

const Ability abilities_runes[] = {
    {"Remote Bomb", "IsGet_Obj_RemoteBomb", ""},
    {"Magnesis", "IsGet_Obj_Magnetglove", ""},
    {"Stasis", "IsGet_Obj_StopTimer", ""},
    {"Cryonis", "IsGet_Obj_IceMaker", ""},
    {"Camera", "IsGet_Obj_Camera", ""},
    {"Sheikah Sensor", "IsGet_Obj_SheikSensor", ""},
    {"Master Cycle Zero", "IsGet_Obj_Motorcycle", ""},
};

const Ability abilities_upgrades[] = {
    {"Remote Bomb +", "IsGet_Obj_RemoteBombLv2", ""},
    {"Stasis +", "IsGet_Obj_StopTimerLv2", ""},
    {"Sheikah Sensor +", "IsGet_Obj_SheikSensorLv2", ""},
};

const Ability abilities_champions[] = {
    {"Revali's Gale", "IsGet_Obj_HeroSoul_Rito", "Obj_HeroSoul_Rito"},
    {"Daruk's Protection", "IsGet_Obj_HeroSoul_Goron", "Obj_HeroSoul_Goron"},
    {"Mipha's Grace", "IsGet_Obj_HeroSoul_Zora", "Obj_HeroSoul_Zora"},
    {"Urbosa's Fury", "IsGet_Obj_HeroSoul_Gerudo", "Obj_HeroSoul_Gerudo"},
};

const Ability abilities_gear[] = {
    {"Paraglider", "IsGet_PlayerStole2", "PlayerStole2"},
    {"Travel Medallion", "IsGet_Obj_WarpDLC", "Obj_WarpDLC"},
};

struct AbilityGroup {
    const char* label;
    const Ability* items;   // its list
    int count;              // how many are in it
};

const AbilityGroup ability_groups[] = {
    {"Runes", abilities_runes, count_of(abilities_runes)},
    {"Rune upgrades", abilities_upgrades, count_of(abilities_upgrades)},
    {"Champions", abilities_champions, count_of(abilities_champions)},
    {"Gear", abilities_gear, count_of(abilities_gear)},
};

bool ability_open = false;
bool ability_in_group = false;   // false: showing the categories
int ability_group = 0;           // the chosen category
int ability_item = 0;            // the chosen ability inside it

// Switches one ability: its flag, and its key item if it has one.
void toggle_ability(const Ability* ability) {
    bool on = !flag(ability->flag);
    set_flag(ability->flag, on);
    if (ability->item[0] != 0) {   // "" means no item
        if (on) {
            give_item(ability->item, 1);
        } else {
            take_item(ability->item, 1);
        }
    }
    print("%s: %s", ability->label, on ? "ON" : "off");
}

// Fills lines (and values, for the ON / off column) with what the menu shows
// right now, and gives back how many.
int ability_menu_lines(const char** lines, const char** values) {
    if (ability_in_group) {
        const AbilityGroup* group = &ability_groups[ability_group];
        for (int i = 0; i < group->count; i++) {
            lines[i] = group->items[i].label;
            values[i] = flag(group->items[i].flag) ? "ON" : "off";
        }
        return group->count;
    }
    for (int i = 0; i < count_of(ability_groups); i++) {
        lines[i] = ability_groups[i].label;
        values[i] = nullptr;   // no ON / off for a category
    }
    return count_of(ability_groups);
}

void ability_menu() {
    hide_from_game(R3);   // every frame: R3 belongs to this menu now
    if (pressed(R3)) {
        ability_open = !ability_open;
    }
    if (!ability_open) {
        return;
    }
    hide_dpad_from_game();

    const char* lines[32];
    const char* values[32];
    int count = ability_menu_lines(lines, values);

    if (ability_in_group) {
        ability_item = menu_up_down(ability_item, count);
    } else {
        ability_group = menu_up_down(ability_group, count);
    }

    if (pressed(RIGHT)) {
        play_sound("mc_Key_Decide");
        if (!ability_in_group) {
            ability_in_group = true;   // open the chosen category
            ability_item = 0;
        } else {
            toggle_ability(&ability_groups[ability_group].items[ability_item]);
        }
    }

    if (pressed(LEFT)) {
        play_sound("mc_Key_On");
        if (ability_in_group) {
            ability_in_group = false;   // back to the categories
        } else {
            ability_open = false;
            return;
        }
    }

    count = ability_menu_lines(lines, values);   // the level may have just changed
    if (ability_in_group) {
        draw_menu(ability_groups[ability_group].label, lines, count, ability_item, values,
                  "Right: on / off   Left: back");
    } else {
        draw_menu("Abilities", lines, count, ability_group);
    }
}
