// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#include "GameSettings.h"

#include <string.h>

#include "Cheats.h"
#include "GameRes.h"
#include "GameVersion.h"
#include "HelpScreen.h"
#include "Macro.h"
#include "OptionsScreen.h"
#include "SGP/FileMan.h"
#include "SGP/LibraryDataBase.h"
#include "SGP/LoadSaveData.h"
#include "SGP/Random.h"
#include "SGP/SGP.h"
#include "SGP/Types.h"
#include "SaveLoadGame.h"
#include "SaveLoadScreen.h"
#include "ScreenIDs.h"
#include "Strategic/Meanwhile.h"
#include "Strategic/QueenCommand.h"
#include "Strategic/StrategicMap.h"
#include "Tactical/Campaign.h"
#include "Tactical/Overhead.h"
#include "Utils/FontControl.h"
#include "Utils/Message.h"
#include "Utils/MusicControl.h"
#include "Utils/SoundControl.h"
#include "Utils/Text.h"

#define GAME_SETTINGS_FILE "../Ja2.set"

GAME_SETTINGS gGameSettings;
GAME_OPTIONS gGameOptions;

// Change this number when we want any who gets the new build to reset the
// options
#define GAME_SETTING_CURRENT_VERSION 522

static void InitGameSettings();

void LoadGameSettings() {
  try {
    AutoSGPFile f(FileMan::openForReadingSmart(GAME_SETTINGS_FILE, false));

    uint8_t data[76];
    if (FileGetSize(f) != sizeof(data)) goto fail;
    FileRead(f, data, sizeof(data));

    uint8_t music_volume;
    uint8_t sound_volume;
    uint8_t speech_volume;
    uint32_t settings_version;
    GAME_SETTINGS &g = gGameSettings;
    uint8_t const *d = data;
    EXTR_I8(d, g.bLastSavedGameSlot)
    EXTR_U8(d, music_volume)
    EXTR_U8(d, sound_volume)
    EXTR_U8(d, speech_volume)
    EXTR_U8A(d, g.fOptions, lengthof(g.fOptions))
    EXTR_SKIP(d, lengthof(g_version_number) + 1)
    EXTR_U32(d, settings_version)
    EXTR_U32(d, g.uiMeanwhileScenesSeenFlags)
    EXTR_BOOL(d, g.fHideHelpInAllScreens)
    EXTR_SKIP(d, 1)
    EXTR_U8(d, g.ubSizeOfDisplayCover)
    EXTR_U8(d, g.ubSizeOfLOS)
    EXTR_SKIP(d, 20)
    Assert(d == endof(data));

    if (settings_version < GAME_SETTING_CURRENT_VERSION) goto fail;

    // Do checking to make sure the settings are valid
    if (g.bLastSavedGameSlot < 0 || NUM_SAVE_GAMES <= g.bLastSavedGameSlot)
      g.bLastSavedGameSlot = -1;

    // Make sure that at least subtitles or speech is enabled
    if (!g.fOptions[TOPTION_SUBTITLES] && !g.fOptions[TOPTION_SPEECH]) {
      g.fOptions[TOPTION_SUBTITLES] = TRUE;
      g.fOptions[TOPTION_SPEECH] = TRUE;
    }

    SetSoundEffectsVolume(sound_volume <= HIGHVOLUME ? sound_volume : MIDVOLUME);
    SetSpeechVolume(speech_volume <= HIGHVOLUME ? speech_volume : MIDVOLUME);
    MusicSetVolume(music_volume <= HIGHVOLUME ? music_volume : MIDVOLUME);

    // If the user doesn't want the help screens present
    if (g.fHideHelpInAllScreens) {
      gHelpScreen.usHasPlayerSeenHelpScreenInCurrentScreen = 0;
    } else {
      // Set it so that every screens help will come up the first time (the 'x'
      // will be set)
      gHelpScreen.usHasPlayerSeenHelpScreenInCurrentScreen = 0xffff;
    }
    return;
  } catch (...) { /* Handled below */
  }

fail:
  InitGameSettings();
}

void SaveGameSettings() {
  // Record the current settings into the game settins structure
  GAME_SETTINGS &g = gGameSettings;

  uint8_t data[76];
  uint8_t *d = data;
  INJ_I8(d, g.bLastSavedGameSlot)
  uint8_t const music_volume = MusicGetVolume();
  INJ_U8(d, music_volume)
  uint8_t const sound_volume = GetSoundEffectsVolume();
  INJ_U8(d, sound_volume)
  uint8_t const speech_volume = GetSpeechVolume();
  INJ_U8(d, speech_volume)
  INJ_U8A(d, g.fOptions, lengthof(g.fOptions))
  INJ_STR(d, g_version_number, lengthof(g_version_number))
  INJ_SKIP(d, 1)
  INJ_U32(d, GAME_SETTING_CURRENT_VERSION)
  INJ_U32(d, g.uiMeanwhileScenesSeenFlags)
  INJ_BOOL(d, g.fHideHelpInAllScreens)
  INJ_SKIP(d, 1)
  INJ_U8(d, g.ubSizeOfDisplayCover)
  INJ_U8(d, g.ubSizeOfLOS)
  INJ_SKIP(d, 20)
  Assert(d == endof(data));

  AutoSGPFile f(FileMan::openForWriting(GAME_SETTINGS_FILE));
  FileWrite(f, data, sizeof(data));
}

static void InitGameSettings() {
  SetSoundEffectsVolume(63);
  SetSpeechVolume(63);
  MusicSetVolume(63);

  GAME_SETTINGS &g = gGameSettings;
  g.bLastSavedGameSlot = -1;
  g.fOptions[TOPTION_SPEECH] = TRUE;
  g.fOptions[TOPTION_MUTE_CONFIRMATIONS] = FALSE;
  g.fOptions[TOPTION_SUBTITLES] = TRUE;
  g.fOptions[TOPTION_KEY_ADVANCE_SPEECH] = FALSE;
  g.fOptions[TOPTION_ANIMATE_SMOKE] = TRUE;
  g.fOptions[TOPTION_BLOOD_N_GORE] = TRUE;
  g.fOptions[TOPTION_DONT_MOVE_MOUSE] = FALSE;
  g.fOptions[TOPTION_OLD_SELECTION_METHOD] = FALSE;
  g.fOptions[TOPTION_ALWAYS_SHOW_MOVEMENT_PATH] = FALSE;
  g.fOptions[TOPTION_SHOW_MISSES] = FALSE;
  g.fOptions[TOPTION_RTCONFIRM] = FALSE;
  g.fOptions[TOPTION_SLEEPWAKE_NOTIFICATION] = TRUE;
  g.fOptions[TOPTION_USE_METRIC_SYSTEM] = FALSE;
  g.fOptions[TOPTION_MERC_ALWAYS_LIGHT_UP] = FALSE;
  g.fOptions[TOPTION_SMART_CURSOR] = FALSE;
  g.fOptions[TOPTION_SNAP_CURSOR_TO_DOOR] = TRUE;
  g.fOptions[TOPTION_GLOW_ITEMS] = TRUE;
  g.fOptions[TOPTION_TOGGLE_TREE_TOPS] = TRUE;
  g.fOptions[TOPTION_TOGGLE_WIREFRAME] = TRUE;
  g.fOptions[TOPTION_3D_CURSOR] = FALSE;
  g.fOptions[TOPTION_MERC_CASTS_LIGHT] = TRUE;
  g.fOptions[TOPTION_HIDE_BULLETS] = FALSE;
  g.fOptions[TOPTION_TRACKING_MODE] = TRUE;
  g.uiMeanwhileScenesSeenFlags = 0;
  g.fHideHelpInAllScreens = FALSE;
  g.ubSizeOfDisplayCover = 4;
  g.ubSizeOfLOS = 4;

  // Since we just set the settings, save them
  SaveGameSettings();
}

void InitGameOptions() {
  memset(&gGameOptions, 0, sizeof(GAME_OPTIONS));

  // Init the game options
  gGameOptions.fGunNut = FALSE;
  gGameOptions.fSciFi = TRUE;
  gGameOptions.ubDifficultyLevel = DIF_LEVEL_EASY;
  // gGameOptions.fTurnTimeLimit		 = FALSE;
  gGameOptions.fIronManMode = FALSE;
}

void CDromEjectionErrorMessageBoxCallBack(MessageBoxReturnValue const bExitValue) {
  if (bExitValue == MSG_BOX_RETURN_OK) {
    guiPreviousOptionScreen = GAME_SCREEN;

    // if we are in a game, save the game
    if (gTacticalStatus.fHasAGameBeenStarted) {
      SaveGame(SAVE__ERROR_NUM, pMessageStrings[MSG_CDROM_SAVE]);
    }

    // quit the game
    requestGameExit();
  }
}

void DisplayGameSettings() {
  // Display the version number
  ScreenMsg(FONT_MCOLOR_LTYELLOW, MSG_INTERFACE, L"%ls: %hs (%hs)", pMessageStrings[MSG_VERSION],
            g_version_label, g_version_number);

  // Display the difficulty level
  ScreenMsg(FONT_MCOLOR_LTYELLOW, MSG_INTERFACE, L"%ls: %ls", gzGIOScreenText[GIO_DIF_LEVEL_TEXT],
            gzGIOScreenText[gGameOptions.ubDifficultyLevel + GIO_EASY_TEXT - 1]);

  // Iron man option
  ScreenMsg(FONT_MCOLOR_LTYELLOW, MSG_INTERFACE, L"%ls: %ls",
            gzGIOScreenText[GIO_GAME_SAVE_STYLE_TEXT],
            gzGIOScreenText[GIO_SAVE_ANYWHERE_TEXT + gGameOptions.fIronManMode]);

  // Gun option
  ScreenMsg(FONT_MCOLOR_LTYELLOW, MSG_INTERFACE, L"%ls: %ls", gzGIOScreenText[GIO_GUN_OPTIONS_TEXT],
            gzGIOScreenText[gGameOptions.fGunNut ? GIO_GUN_NUT_TEXT : GIO_REDUCED_GUNS_TEXT]);

  // Sci fi option
  ScreenMsg(FONT_MCOLOR_LTYELLOW, MSG_INTERFACE, L"%ls: %ls", gzGIOScreenText[GIO_GAME_STYLE_TEXT],
            gzGIOScreenText[GIO_REALISTIC_TEXT + gGameOptions.fSciFi]);

  // Timed Turns option
  // JA2Gold: no timed turns
  // ScreenMsg(FONT_MCOLOR_LTYELLOW, MSG_INTERFACE, L"%ls: %ls",
  // gzGIOScreenText[GIO_TIMED_TURN_TITLE_TEXT],
  // gzGIOScreenText[GIO_NO_TIMED_TURNS_TEXT + gGameOptions.fTurnTimeLimit]);

  if (CHEATER_CHEAT_LEVEL()) {
    ScreenMsg(FONT_MCOLOR_LTYELLOW, MSG_INTERFACE, gzLateLocalizedString[STR_LATE_56],
              CurrentPlayerProgressPercentage(), HighestPlayerProgressPercentage());
  }
}

bool MeanwhileSceneSeen(uint8_t const meanwhile_id) {
  return meanwhile_id < NUM_MEANWHILES &&  // XXX assertion/exception?
         gGameSettings.uiMeanwhileScenesSeenFlags & (1U << meanwhile_id);
}

void SetMeanwhileSceneSeen(uint8_t const meanwhile_id) {
  if (meanwhile_id >= NUM_MEANWHILES) return;  // XXX assertion/exception?
  gGameSettings.uiMeanwhileScenesSeenFlags |= 1U << meanwhile_id;
}

BOOLEAN CanGameBeSaved() {
  // if the iron man mode is on
  if (gGameOptions.fIronManMode) {
    // if we are in turn based combat
    if ((gTacticalStatus.uiFlags & TURNBASED) && (gTacticalStatus.uiFlags & INCOMBAT)) {
      // no save for you
      return (FALSE);
    }

    // if there are enemies in the current sector
    if (gWorldSectorX != -1 && gWorldSectorY != -1 && gWorldSectorX != 0 && gWorldSectorY != 0 &&
        NumEnemiesInAnySector(gWorldSectorX, gWorldSectorY, gbWorldSectorZ) > 0) {
      // no save for you
      return (FALSE);
    }

    // All checks failed, so we can save
    return (TRUE);
  } else {
    return (TRUE);
  }
}
