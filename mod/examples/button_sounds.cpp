// Example: Button sounds
//
// Plays a click, the game's own keyboard sound, every time you press A, B, X
// or Y.
//
// To use it: copy everything below the "copy from here" line into my_mod.cpp,
// above every_frame(). Then add this line inside every_frame():
//
//     button_sounds();
//
// How it works:
//   * play_sound() plays one of the game's sound effects by its name.
//   * || means "or": the if runs when any one of the four buttons was pressed.
//
// Every sound play_sound() can reach (the game keeps these 51 always ready):
//   Keyboard      mc_Key_On  mc_Key_Decide  mc_EnableKey_Decide  mc_ExitKey_Decide
//                 mc_KeyTypeChange_Decide  mc_KeyTypeChangeWindow_Appear
//                 mc_KeyType_Changed  mc_CharacterChange_Decide
//   Title screen  TitleCursorMove  TitleCursorDecide  TitleStartPressed  GameStart
//   Screens       ChangeScreen_A  ChangeScreen_B  mc_DemoStart
//   Upgrades      mc_HeartUp  mc_HeartUp_00  mc_HeartUp_01  mc_ExtraHeartUp
//                 mc_ExtraGanbari_Up  mc_LastCompleteOpen  mc_LastCompleteOpen_00
//                 mc_LastCompleteOpen_LFE
//   Warnings      mc_HeartAlert_L  mc_HeartAlert_S
//   Map           MapMarker_1  MapMarker_2  MapMarker_3  MapMarker_4  MapMarker_5
//                 mc_SetWarpMarker  BikeAppearPointPreview  BikeAppearPointDecide
//   Horses        mc_ChangeHorseMane  mc_ChangeHorseReins  mc_ChangeHorseSaddle
//   Monks         mc_PriestVoice000  ...  mc_PriestVoice006
//   amiibo        AmiiboHit  AmiiboPreview  AmiiboSuccess  AmiiboError
//   Other         Solve_HiddenKorok  IceMaker_Marker_OK  IceMaker_Marker_NG
//                 Event_Demo169_0
//
// Ideas to try:
//   * Give each button its own sound.
//   * Play "mc_HeartUp" when the starter kit arrives (see starter_kit.cpp).
//   * Play a Monk's voice when you find a Korok seed (see exploration_rewards.cpp).

#include "botw.h"   // my_mod.cpp already has this line

// ---- copy from here ----

void button_sounds() {
    if (pressed(A) || pressed(B) || pressed(X) || pressed(Y)) {
        play_sound("mc_Key_On");
    }
}
