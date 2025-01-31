// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#include "Strategic/PreBattleInterface.h"

#include <algorithm>
#include <stdio.h>
#include <string.h>
#include <wchar.h>

#include "Cheats.h"
#include "Directories.h"
#include "GameScreen.h"
#include "GameSettings.h"
#include "JAScreens.h"
#include "Laptop/History.h"
#include "Macro.h"
#include "MessageBoxScreen.h"
#include "SGP/ButtonSystem.h"
#include "SGP/Debug.h"
#include "SGP/English.h"
#include "SGP/Font.h"
#include "SGP/Input.h"
#include "SGP/MouseSystem.h"
#include "SGP/Random.h"
#include "SGP/Timer.h"
#include "SGP/VObject.h"
#include "SGP/VSurface.h"
#include "SGP/Video.h"
#include "ScreenIDs.h"
#include "Strategic/Assignments.h"
#include "Strategic/AutoResolve.h"
#include "Strategic/CreatureSpreading.h"
#include "Strategic/GameClock.h"
#include "Strategic/MapScreen.h"
#include "Strategic/MapScreenHelicopter.h"
#include "Strategic/MapScreenInterface.h"
#include "Strategic/MapScreenInterfaceBorder.h"
#include "Strategic/MapScreenInterfaceBottom.h"
#include "Strategic/PlayerCommand.h"
#include "Strategic/QueenCommand.h"
#include "Strategic/Quests.h"
#include "Strategic/StrategicMap.h"
#include "Strategic/StrategicMovement.h"
#include "Strategic/StrategicPathing.h"
#include "Strategic/StrategicStatus.h"
#include "Strategic/StrategicTownLoyalty.h"
#include "Strategic/TownMilitia.h"
#include "Tactical/Campaign.h"
#include "Tactical/Overhead.h"
#include "Tactical/SoldierMacros.h"
#include "Tactical/Squads.h"
#include "Tactical/Vehicles.h"
#include "TileEngine/RenderDirty.h"
#include "TileEngine/SysUtil.h"
#include "TileEngine/TacticalPlacementGUI.h"
#include "Utils/Cursors.h"
#include "Utils/FontControl.h"
#include "Utils/MultiLanguageGraphicUtils.h"
#include "Utils/MusicControl.h"
#include "Utils/PopUpBox.h"
#include "Utils/SoundControl.h"
#include "Utils/Text.h"
#include "Utils/TimerControl.h"

extern BOOLEAN gfDelayAutoResolveStart;

BOOLEAN gfTacticalTraversal = FALSE;
GROUP *gpTacticalTraversalGroup = NULL;
SOLDIERTYPE *gpTacticalTraversalChosenSoldier = NULL;

BOOLEAN gfAutomaticallyStartAutoResolve = FALSE;
BOOLEAN gfAutoAmbush = FALSE;
BOOLEAN gfHighPotentialForAmbush = FALSE;
BOOLEAN gfGotoSectorTransition = FALSE;
BOOLEAN gfEnterAutoResolveMode = FALSE;
BOOLEAN gfEnteringMapScreenToEnterPreBattleInterface = FALSE;
BOOLEAN gfIgnoreAllInput = TRUE;

enum  // GraphicIDs for the panel
{
  MAINPANEL,
  TITLE_BAR_PIECE,
  TOP_COLUMN,
  BOTTOM_COLUMN,
  UNINVOLVED_HEADER
};

// The start of the black space
#define TOP_Y 113
// The end of the black space
#define BOTTOM_Y 349
// The internal height of the uninvolved panel
#define INTERNAL_HEIGHT 27
// The actual height of the uninvolved panel
#define ACTUAL_HEIGHT 34
// The height of each row
#define ROW_HEIGHT 10

bool gfDisplayPotentialRetreatPaths = false;

GROUP *gpBattleGroup = NULL;

static MOUSE_REGION PBInterfaceBlanket;
BOOLEAN gfPreBattleInterfaceActive = FALSE;
static GUIButtonRef iPBButton[3];
static BUTTON_PICS *iPBButtonImage[3];
static SGPVObject *uiInterfaceImages;
BOOLEAN gfRenderPBInterface;
static BOOLEAN gfPBButtonsHidden;
BOOLEAN fDisableMapInterfaceDueToBattle = FALSE;

static BOOLEAN gfBlinkHeader;

static uint32_t guiNumInvolved;
static uint32_t guiNumUninvolved;

// SAVE START

// Using the ESC key in the PBI will get rid of the PBI and go back to
// mapscreen, but only if the PBI isn't persistant (!gfPersistantPBI).
BOOLEAN gfPersistantPBI = FALSE;

// Contains general information about the type of encounter the player is faced
// with.  This determines whether or not you can autoresolve the battle or even
// retreat.  This code dictates the header that is used at the top of the PBI.
uint8_t gubEnemyEncounterCode = NO_ENCOUNTER_CODE;

// The autoresolve during tactical battle option needs more detailed information
// than the gubEnemyEncounterCode can provide.  The explicit version contains
// possibly unique codes for reasons not normally used in the PBI.  For example,
// if we were fighting the enemy in a normal situation, then shot at a civilian,
// the civilians associated with the victim would turn hostile, which would
// disable the ability to autoresolve the battle.
BOOLEAN gubExplicitEnemyEncounterCode = NO_ENCOUNTER_CODE;

// Location of the current battle (determines where the animated icon is
// blitted) and if the icon is to be blitted.
BOOLEAN gfBlitBattleSectorLocator = FALSE;

uint8_t gubPBSectorX = 0;
uint8_t gubPBSectorY = 0;
uint8_t gubPBSectorZ = 0;

BOOLEAN gfCantRetreatInPBI = FALSE;
// SAVE END

BOOLEAN gfUsePersistantPBI;

static void MakeButton(uint32_t idx, int16_t x, const wchar_t *text, GUI_CALLBACK click) {
  GUIButtonRef const btn =
      QuickCreateButton(iPBButtonImage[idx], x, 54, MSYS_PRIORITY_HIGHEST - 2, click);
  iPBButton[idx] = btn;

  btn->SpecifyGeneralTextAttributes(text, BLOCKFONT, FONT_BEIGE, 141);
  btn->SpecifyHilitedTextColors(FONT_WHITE, FONT_NEARBLACK);
  btn->SpecifyTextOffsets(8, 7, TRUE);
  btn->SpecifyTextWrappedWidth(51);
  btn->AllowDisabledFastHelp();
  btn->Hide();
}

static void AutoResolveBattleCallback(GUI_BUTTON *btn, int32_t reason);
static void CheckForRobotAndIfItsControlled();
static void DoTransitionFromMapscreenToPreBattleInterface();
static void GoToSectorCallback(GUI_BUTTON *btn, int32_t reason);
static void RetreatMercsCallback(GUI_BUTTON *btn, int32_t reason);

void InitPreBattleInterface(GROUP *const battle_group, bool const persistent_pbi) {
  // ARM: Feb01/98 - Cancel out of mapscreen movement plotting if PBI subscreen
  // is coming up
  if (bSelectedDestChar != -1 || fPlotForHelicopter) {
    AbortMovementPlottingMode();
  }

  if (gfPreBattleInterfaceActive) return;

  uint8_t x;
  uint8_t y;
  uint8_t z;
  gfPersistantPBI = persistent_pbi;
  if (persistent_pbi) {
    gfBlitBattleSectorLocator = TRUE;
    gfBlinkHeader = FALSE;

    //	InitializeTacticalStatusAtBattleStart();
    // CJC, Oct 5 98: this is all we should need from
    // InitializeTacticalStatusAtBattleStart()
    if (gubEnemyEncounterCode != BLOODCAT_AMBUSH_CODE &&
        gubEnemyEncounterCode != ENTERING_BLOODCAT_LAIR_CODE &&
        !CheckFact(FACT_FIRST_BATTLE_FOUGHT, 0)) {
      SetFactTrue(FACT_FIRST_BATTLE_BEING_FOUGHT);
    }

    // ATE: Added check for persistent_pbi if !battle_group
    // Searched code and saw that this condition only happens for creatures
    if (guiCurrentScreen == GAME_SCREEN && (battle_group || persistent_pbi)) {
      gpBattleGroup = battle_group;
      gfEnteringMapScreen = TRUE;
      gfEnteringMapScreenToEnterPreBattleInterface = TRUE;
      gfUsePersistantPBI = TRUE;
      return;
    }

    if (gfTacticalTraversal && (battle_group == gpTacticalTraversalGroup || gbWorldSectorZ > 0)) {
      return;
    }

    // Reset the help text for mouse regions
    gMapStatusBarsRegion.SetFastHelpText(L"");

    gfDisplayPotentialRetreatPaths = false;

    gpBattleGroup = battle_group;

    // Calc sector values
    if (battle_group) {
      x = battle_group->ubSectorX;
      y = battle_group->ubSectorY;
      z = battle_group->ubSectorZ;
      fMapPanelDirty = TRUE;
    } else {
      x = SECTORX(gubSectorIDOfCreatureAttack);
      y = SECTORY(gubSectorIDOfCreatureAttack);
      z = 0;
    }
    gubPBSectorX = x;
    gubPBSectorY = y;
    gubPBSectorZ = z;
  } else {  // Calculate the non-persistent situation
    gfBlinkHeader = TRUE;
    x = gubPBSectorX;
    y = gubPBSectorY;
    z = gubPBSectorZ;

    if (HostileCiviliansPresent()) {  // There are hostile civilians, so no
                                      // autoresolve allowed.
      gubExplicitEnemyEncounterCode = HOSTILE_CIVILIANS_CODE;
    } else if (HostileBloodcatsPresent()) {  // There are bloodcats in the
                                             // sector, so no autoresolve allowed
      gubExplicitEnemyEncounterCode = HOSTILE_BLOODCATS_CODE;
    } else if (gbWorldSectorZ != 0) {  // We are underground, so no autoresolve allowed
      SECTORINFO const &sector =
          SectorInfo[SECTOR(x, y)];  // XXX Why check surface info when underground?
      if (sector.ubCreaturesInBattle != 0) {
        gubExplicitEnemyEncounterCode = FIGHTING_CREATURES_CODE;
      } else if (sector.ubAdminsInBattle != 0 || sector.ubTroopsInBattle != 0 ||
                 sector.ubElitesInBattle != 0) {
        gubExplicitEnemyEncounterCode = ENTERING_ENEMY_SECTOR_CODE;
      }
    } else if (gubEnemyEncounterCode == ENTERING_ENEMY_SECTOR_CODE ||
               gubEnemyEncounterCode == ENEMY_ENCOUNTER_CODE ||
               gubEnemyEncounterCode == ENEMY_AMBUSH_CODE ||
               gubEnemyEncounterCode == ENEMY_INVASION_CODE ||
               gubEnemyEncounterCode == BLOODCAT_AMBUSH_CODE ||
               gubEnemyEncounterCode == ENTERING_BLOODCAT_LAIR_CODE ||
               gubEnemyEncounterCode == CREATURE_ATTACK_CODE) {  // Use same code
      gubExplicitEnemyEncounterCode = gubEnemyEncounterCode;
    } else {
      gfBlitBattleSectorLocator = FALSE;
      return;
    }
  }

  fMapScreenBottomDirty = TRUE;
  ChangeSelectedMapSector(x, y, z);
  RenderMapScreenInterfaceBottom();

  /* If we are currently in tactical, then set the flag to automatically bring
   * up the mapscreen. */
  if (guiCurrentScreen == GAME_SCREEN) gfEnteringMapScreen = TRUE;

  if (!fShowTeamFlag) ToggleShowTeamsMode();

  /* Define the blanket region to cover all of the other regions used underneath
   * the panel. */
  MSYS_DefineRegion(&PBInterfaceBlanket, 0, 0, 261, 359, MSYS_PRIORITY_HIGHEST - 5, 0, 0, 0);

  // Create the panel
  char const *const panel_file = GetMLGFilename(MLG_PREBATTLEPANEL);
  uiInterfaceImages = AddVideoObjectFromFile(panel_file);

  // Create the 3 buttons
  iPBButtonImage[0] = LoadButtonImage(INTERFACEDIR "/prebattlebutton.sti", 0, 1);
  iPBButtonImage[1] = UseLoadedButtonImage(iPBButtonImage[0], 0, 1);
  iPBButtonImage[2] = UseLoadedButtonImage(iPBButtonImage[0], 0, 1);
  MakeButton(0, 27, gpStrategicString[STR_PB_AUTORESOLVE_BTN], AutoResolveBattleCallback);
  MakeButton(1, 98, gpStrategicString[STR_PB_GOTOSECTOR_BTN], GoToSectorCallback);
  MakeButton(2, 169, gpStrategicString[STR_PB_RETREATMERCS_BTN], RetreatMercsCallback);

  gfPBButtonsHidden = TRUE;

  /* ARM: This must now be set before any calls utilizing the
   * GetCurrentBattleSectorXYZ() function */
  gfPreBattleInterfaceActive = TRUE;

  CheckForRobotAndIfItsControlled();

  WakeUpAllMercsInSectorUnderAttack();

  // Count the number of players involved or not involved in this battle
  guiNumUninvolved = 0;
  guiNumInvolved = 0;
  uint8_t group_id = 0;
  int8_t best_exp_level = 0;
  bool use_plural_version = false;
  CFOR_EACH_IN_TEAM(i, OUR_TEAM) {
    SOLDIERTYPE const &s = *i;
    if (s.bLife == 0 || s.uiStatusFlags & SOLDIER_VEHICLE) continue;

    if (PlayerMercInvolvedInThisCombat(s)) {
      if (group_id == 0) { /* Record the first groupID. If there is more than one group in
                            * this battle, we can detect it by comparing the first value
                            * with future values. If we do, then we set a flag which
                            * determines whether to use the singular help text or plural
                            * version for the retreat button. */
        group_id = s.ubGroupID;
        if (!gpBattleGroup) gpBattleGroup = GetGroup(group_id);
        if (best_exp_level > s.bExpLevel)
          best_exp_level = s.bExpLevel;  // XXX Determines minimum, not maximum,
                                         // i.e. stays at 0
        if (s.ubPrevSectorID == 255) {   // Not able to retreat (calculate it for group)
          GROUP *const g = GetGroup(group_id);
          Assert(g);
          CalculateGroupRetreatSector(g);
        }
      } else if (group_id != s.ubGroupID) {
        use_plural_version = true;
      }
      ++guiNumInvolved;
    } else {
      ++guiNumUninvolved;
    }
  }

  if (gfPersistantPBI) {
    if (!battle_group) {  // Creatures are attacking
      gubEnemyEncounterCode = CREATURE_ATTACK_CODE;
    } else if (gpBattleGroup->fPlayer) {
      if (gubEnemyEncounterCode != BLOODCAT_AMBUSH_CODE &&
          gubEnemyEncounterCode != ENTERING_BLOODCAT_LAIR_CODE) {
        uint8_t const n_stationary_enemies = NumStationaryEnemiesInSector(x, y);
        if (n_stationary_enemies != 0) {
          gubEnemyEncounterCode = ENTERING_ENEMY_SECTOR_CODE;
        } else {
          gubEnemyEncounterCode = ENEMY_ENCOUNTER_CODE;

          // Don't consider ambushes until the player has reached 25% (normal)
          // progress
          if (gfHighPotentialForAmbush) {
            if (Chance(90)) gubEnemyEncounterCode = ENEMY_AMBUSH_CODE;
          } else {
            uint8_t const n_mobile_enemies = NumMobileEnemiesInSector(x, y);
            uint8_t const n_mercs = PlayerMercsInSector(x, y, z);
            if (gfAutoAmbush && n_mobile_enemies > n_mercs) {
              gubEnemyEncounterCode = ENEMY_AMBUSH_CODE;
            } else if (WhatPlayerKnowsAboutEnemiesInSector(x, y) == KNOWS_NOTHING &&
                       CurrentPlayerProgressPercentage() >=
                           30 - gGameOptions.ubDifficultyLevel *
                                    5) { /* If the enemy outnumbers the players,
                                          * then there is a small chance of the
                                          * enemies ambushing the group */
              if (n_mobile_enemies > n_mercs) {
                SECTORINFO const &sector = SectorInfo[SECTOR(x, y)];
                if (!(sector.uiFlags & SF_ALREADY_VISITED)) {
                  int32_t chance =
                      (uint8_t)(4 - best_exp_level + 2 * gGameOptions.ubDifficultyLevel +
                                CurrentPlayerProgressPercentage() / 10);
                  if (sector.uiFlags & SF_ENEMY_AMBUSH_LOCATION) chance += 20;
                  if (gfCantRetreatInPBI) chance += 20;
                  if ((int32_t)PreRandom(100) < chance) {
                    gubEnemyEncounterCode = ENEMY_AMBUSH_CODE;
                  }
                }
              }
            }
          }
        }
      }
    } else {  // Are enemies invading a town, or just encountered the player.
      uint8_t const sector = SECTOR(x, y);
      if (GetTownIdForSector(sector)) {
        gubEnemyEncounterCode = ENEMY_INVASION_CODE;
      } else {
        switch (sector) {
          case SEC_D2:
          case SEC_D15:
          case SEC_G8:
            // SAM sites not in towns will also be considered to be important
            gubEnemyEncounterCode = ENEMY_INVASION_CODE;
            break;

          default:
            gubEnemyEncounterCode = ENEMY_ENCOUNTER_CODE;
            break;
        }
      }
    }
  }

  gfHighPotentialForAmbush = FALSE;

  if (gfAutomaticallyStartAutoResolve) {
    DisableButton(iPBButton[1]);
    DisableButton(iPBButton[2]);
  }

  gfRenderPBInterface = TRUE;
  MSYS_SetCurrentCursor(CURSOR_NORMAL);
  StopTimeCompression();

  HideAllBoxes();
  fShowAssignmentMenu = FALSE;
  fShowContractMenu = FALSE;
  DisableTeamInfoPanels();
  if (giMapContractButton) giMapContractButton->Hide();
  if (giCharInfoButton[0]) giCharInfoButton[0]->Hide();
  if (giCharInfoButton[1]) giCharInfoButton[1]->Hide();

  if (gubEnemyEncounterCode == ENEMY_ENCOUNTER_CODE) { /* We know how many enemies are here, so
                                                        * until we leave the sector, we will
                                                        * continue to display the value. The flag
                                                        * will get cleared when time advances after
                                                        * the fEnemyInSector flag is clear. */
    SECTORINFO &sector = SectorInfo[SECTOR(x, y)];

    /* Always use these 2 statements together. Without setting the boolean, the
     * flag will never be cleaned up */
    sector.uiFlags |= SF_PLAYER_KNOWS_ENEMIES_ARE_HERE;
    gfResetAllPlayerKnowsEnemiesFlags = TRUE;
  }

  /* Set up fast help for buttons depending on the state of the button, and
   * disable buttons when necessary. */
  if (gfPersistantPBI) {
    wchar_t const *autoresolve_help;
    switch (gubEnemyEncounterCode) {
      case ENTERING_ENEMY_SECTOR_CODE:
      case ENTERING_BLOODCAT_LAIR_CODE:
        // Don't allow autoresolve for player initiated invasion battle types
        DisableButton(iPBButton[0]);
        autoresolve_help = gpStrategicString[STR_PB_DISABLED_AUTORESOLVE_FASTHELP];
        break;

      case ENEMY_AMBUSH_CODE:
      case BLOODCAT_AMBUSH_CODE:
        // Don't allow autoresolve for ambushes
        DisableButton(iPBButton[0]);
        autoresolve_help = gzNonPersistantPBIText[3];
        break;

      default:
        autoresolve_help = gpStrategicString[STR_PB_AUTORESOLVE_FASTHELP];
        break;
    }
    iPBButton[0]->SetFastHelpText(autoresolve_help);
    iPBButton[1]->SetFastHelpText(gpStrategicString[STR_PB_GOTOSECTOR_FASTHELP]);
    if (gfAutomaticallyStartAutoResolve) DisableButton(iPBButton[1]);

    wchar_t const *retreat_help;
    if (gfAutomaticallyStartAutoResolve || gfCantRetreatInPBI ||
        gubEnemyEncounterCode == ENEMY_AMBUSH_CODE ||
        gubEnemyEncounterCode == BLOODCAT_AMBUSH_CODE ||
        gubEnemyEncounterCode == CREATURE_ATTACK_CODE) {
      gfCantRetreatInPBI = FALSE;
      DisableButton(iPBButton[2]);
      retreat_help = gzNonPersistantPBIText[9];
    } else {
      retreat_help = use_plural_version ? gpStrategicString[STR_BP_RETREATPLURAL_FASTHELP]
                                        : gpStrategicString[STR_BP_RETREATSINGLE_FASTHELP];
    }
    iPBButton[2]->SetFastHelpText(retreat_help);
  } else { /* Use the explicit encounter code to determine what gets disable and
            * the associated help text that is used. */

    /* First of all, the retreat button is always disabled seeing a battle is in
     * progress. */
    DisableButton(iPBButton[2]);
    iPBButton[2]->SetFastHelpText(gzNonPersistantPBIText[0]);
    iPBButton[1]->SetFastHelpText(gzNonPersistantPBIText[1]);
    wchar_t const *help;
    switch (gubExplicitEnemyEncounterCode) {
      case CREATURE_ATTACK_CODE:
      case ENEMY_ENCOUNTER_CODE:
      case ENEMY_INVASION_CODE:
        help = gzNonPersistantPBIText[2];
        goto set_help;
      case ENTERING_ENEMY_SECTOR_CODE:
        help = gzNonPersistantPBIText[3];
        goto disable_set_help;
      case ENEMY_AMBUSH_CODE:
        help = gzNonPersistantPBIText[4];
        goto disable_set_help;
      case FIGHTING_CREATURES_CODE:
        help = gzNonPersistantPBIText[5];
        goto disable_set_help;
      case HOSTILE_CIVILIANS_CODE:
        help = gzNonPersistantPBIText[6];
        goto disable_set_help;
      case HOSTILE_BLOODCATS_CODE:
      case BLOODCAT_AMBUSH_CODE:
      case ENTERING_BLOODCAT_LAIR_CODE:
        help = gzNonPersistantPBIText[7];
        goto disable_set_help;

      disable_set_help:
        DisableButton(iPBButton[0]);
      set_help:
        iPBButton[0]->SetFastHelpText(help);
    }
  }

  // Disable the options button when the auto resolve screen comes up
  EnableDisAbleMapScreenOptionsButton(FALSE);

  SetMusicMode(MUSIC_TACTICAL_ENEMYPRESENT);

  DoTransitionFromMapscreenToPreBattleInterface();
}

static void DoTransitionFromMapscreenToPreBattleInterface() {
  uint32_t uiStartTime, uiCurrTime;
  int32_t iPercentage, iFactor;
  uint32_t uiTimeRange;
  int16_t sStartLeft, sEndLeft, sStartTop, sEndTop;
  int32_t iLeft, iTop, iWidth, iHeight;
  BOOLEAN fEnterAutoResolveMode = FALSE;

  PauseTime(FALSE);

  iWidth = 261;
  iHeight = 359;

  uiTimeRange = 1000;
  iPercentage = 0;
  uiStartTime = GetClock();

  GetScreenXYFromMapXY(gubPBSectorX, gubPBSectorY, &sStartLeft, &sStartTop);
  sStartLeft += MAP_GRID_X / 2;
  sStartTop += MAP_GRID_Y / 2;
  sEndLeft = 131;
  sEndTop = 180;

  // save the mapscreen buffer
  BltVideoSurface(guiEXTRABUFFER, FRAME_BUFFER, 0, 0, NULL);

  if (gfEnterAutoResolveMode) {  // If we are intending on immediately entering
                                 // autoresolve, change the global flag so that it
                                 // will actually
    // render the interface once.  If gfEnterAutoResolveMode is clear, then
    // RenderPreBattleInterface() won't do anything.
    fEnterAutoResolveMode = TRUE;
    gfEnterAutoResolveMode = FALSE;
  }
  // render the prebattle interface
  RenderPreBattleInterface();

  gfIgnoreAllInput = TRUE;

  if (fEnterAutoResolveMode) {  // Change it back
    gfEnterAutoResolveMode = TRUE;
  }

  BlitBufferToBuffer(guiSAVEBUFFER, FRAME_BUFFER, 27, 54, 209, 32);
  RenderButtons();
  BlitBufferToBuffer(FRAME_BUFFER, guiSAVEBUFFER, 27, 54, 209, 32);
  gfRenderPBInterface = TRUE;

  // hide the prebattle interface
  BlitBufferToBuffer(guiEXTRABUFFER, FRAME_BUFFER, 0, 0, 261, 359);
  PlayJA2SampleFromFile(SOUNDSDIR "/laptop power up (8-11).wav", HIGHVOLUME, 1, MIDDLEPAN);
  InvalidateScreen();
  RefreshScreen();

  SGPBox const PBIRect = {0, 0, 261, 359};
  while (iPercentage < 100) {
    uiCurrTime = GetClock();
    iPercentage = (uiCurrTime - uiStartTime) * 100 / uiTimeRange;
    iPercentage = std::min(iPercentage, 100);

    // Factor the percentage so that it is modified by a gravity falling
    // acceleration effect.
    iFactor = (iPercentage - 50) * 2;
    if (iPercentage < 50)
      iPercentage = (uint32_t)(iPercentage + iPercentage * iFactor * 0.01 + 0.5);
    else
      iPercentage = (uint32_t)(iPercentage + (100 - iPercentage) * iFactor * 0.01 + 0.05);

    // Calculate the center point.
    iLeft = sStartLeft - (sStartLeft - sEndLeft + 1) * iPercentage / 100;
    if (sStartTop > sEndTop)
      iTop = sStartTop - (sStartTop - sEndTop + 1) * iPercentage / 100;
    else
      iTop = sStartTop + (sEndTop - sStartTop + 1) * iPercentage / 100;

    SGPBox const DstRect = {iLeft - iWidth * iPercentage / 200, iTop - iHeight * iPercentage / 200,
                            std::max(1, iWidth * iPercentage / 100),
                            std::max(1, iHeight * iPercentage / 100)};

    BltStretchVideoSurface(FRAME_BUFFER, guiSAVEBUFFER, &PBIRect, &DstRect);

    InvalidateScreen();
    RefreshScreen();

    // Restore the previous rect.
    BlitBufferToBuffer(guiEXTRABUFFER, FRAME_BUFFER, DstRect.x, DstRect.y, DstRect.w + 1,
                       DstRect.h + 1);
  }
  BltVideoSurface(guiSAVEBUFFER, FRAME_BUFFER, 0, 0, NULL);
}

void KillPreBattleInterface() {
  if (!gfPreBattleInterfaceActive) return;

  fDisableMapInterfaceDueToBattle = FALSE;
  MSYS_RemoveRegion(&PBInterfaceBlanket);

  // The panel
  DeleteVideoObject(uiInterfaceImages);

  // The 3 buttons
  RemoveButton(iPBButton[0]);
  RemoveButton(iPBButton[1]);
  RemoveButton(iPBButton[2]);
  UnloadButtonImage(iPBButtonImage[0]);
  UnloadButtonImage(iPBButtonImage[1]);
  UnloadButtonImage(iPBButtonImage[2]);

  /*
  MSYS_RemoveRegion( &InvolvedRegion );
  if( guiNumUninvolved )
          MSYS_RemoveRegion( &UninvolvedRegion );
  */

  gfPreBattleInterfaceActive = FALSE;

  // UpdateCharRegionHelpText( );

  // re draw affected regions
  fMapPanelDirty = TRUE;
  fTeamPanelDirty = TRUE;
  fMapScreenBottomDirty = TRUE;
  fCharacterInfoPanelDirty = TRUE;
  gfDisplayPotentialRetreatPaths = false;

  // Enable the options button when the auto resolve  screen comes up
  EnableDisAbleMapScreenOptionsButton(TRUE);

  ColorFillVideoSurfaceArea(guiSAVEBUFFER, 0, 0, 261, 359, 0);

  EnableTeamInfoPanels();
  if (giMapContractButton) giMapContractButton->Show();
  if (giCharInfoButton[0]) giCharInfoButton[0]->Show();
  if (giCharInfoButton[1]) giCharInfoButton[1]->Show();
}

static void RenderPBHeader(int32_t *piX, int32_t *piWidth) {
  int32_t x, width;
  uint8_t const foreground = !gfBlinkHeader               ? FONT_BEIGE
                             : GetJA2Clock() % 1000 < 667 ? FONT_WHITE
                                                          : FONT_LTRED;
  SetFontAttributes(FONT10ARIALBOLD, foreground);
  const wchar_t *str;  // XXX HACK000E
  if (!gfPersistantPBI) {
    str = gzNonPersistantPBIText[8];
  } else
    switch (gubEnemyEncounterCode) {
      case ENEMY_INVASION_CODE:
        str = gpStrategicString[STR_PB_ENEMYINVASION_HEADER];
        break;
      case ENEMY_ENCOUNTER_CODE:
        str = gpStrategicString[STR_PB_ENEMYENCOUNTER_HEADER];
        break;
      case ENEMY_AMBUSH_CODE:
        str = gpStrategicString[STR_PB_ENEMYAMBUSH_HEADER];
        gfBlinkHeader = TRUE;
        break;
      case ENTERING_ENEMY_SECTOR_CODE:
        str = gpStrategicString[STR_PB_ENTERINGENEMYSECTOR_HEADER];
        break;
      case CREATURE_ATTACK_CODE:
        str = gpStrategicString[STR_PB_CREATUREATTACK_HEADER];
        gfBlinkHeader = TRUE;
        break;
      case BLOODCAT_AMBUSH_CODE:
        str = gpStrategicString[STR_PB_BLOODCATAMBUSH_HEADER];
        gfBlinkHeader = TRUE;
        break;
      case ENTERING_BLOODCAT_LAIR_CODE:
        str = gpStrategicString[STR_PB_ENTERINGBLOODCATLAIR_HEADER];
        break;

      default:
        abort();  // HACK000E
    }
  width = StringPixLength(str, FONT10ARIALBOLD);
  x = 130 - width / 2;
  MPrint(x, 4, str);
  InvalidateRegion(0, 0, 231, 12);
  *piX = x;
  *piWidth = width;
}

static void PrintConfined(int32_t const x, int32_t const y, int32_t const max_w,
                          wchar_t const *const str) {
  Font font = BLOCKFONT;
  int32_t w = StringPixLength(str, font);
  if (w >= max_w) {
    font = BLOCKFONTNARROW;
    w = StringPixLength(str, font);
  }
  SetFont(font);
  MPrint(x - w, y, str);
}

static void MPrintCentered(int32_t x, int32_t const y, int32_t const w, wchar_t const *const str) {
  x += (w - StringPixLength(str, FontDefault)) / 2;
  MPrint(x, y, str);
}

static wchar_t const *GetSoldierConditionInfo(SOLDIERTYPE const &);

void RenderPreBattleInterface() {
  wchar_t str[100];

  /* If the cursor is inside the rectangle consisting of the rectangle button,
   * then we set up the variables so that the retreat arrows get drawn in the
   * mapscreen. */
  GUI_BUTTON const &retreat = *iPBButton[2];
  if (retreat.Enabled()) {
    bool const mouse_in_reatread_button_area =
        retreat.X() <= gusMouseXPos && gusMouseXPos <= retreat.BottomRightX() &&
        retreat.Y() <= gusMouseYPos && gusMouseYPos <= retreat.BottomRightY();
    if (gfDisplayPotentialRetreatPaths != mouse_in_reatread_button_area) {
      gfDisplayPotentialRetreatPaths = mouse_in_reatread_button_area;
      fMapPanelDirty = TRUE;
    }
  }

  int32_t x;
  int32_t width;
  if (gfRenderPBInterface) {
    gfRenderPBInterface = FALSE;

    SGPVSurface *const dst = guiSAVEBUFFER;
    SetFontDestBuffer(dst);

    if (gfPBButtonsHidden) {
      gfPBButtonsHidden = FALSE;
      ShowButton(iPBButton[0]);
      ShowButton(iPBButton[1]);
      ShowButton(iPBButton[2]);
    } else {
      MarkAButtonDirty(iPBButton[0]);
      MarkAButtonDirty(iPBButton[1]);
      MarkAButtonDirty(iPBButton[2]);
    }

    SGPVObject const *const vo = uiInterfaceImages;
    // Main panel
    BltVideoObject(dst, vo, MAINPANEL, 0, 0);
    // Main title
    RenderPBHeader(&x, &width);
    // Draw the title bars up to the text
    for (int32_t i = x - 12; i > 20; i -= 10) {
      BltVideoObject(dst, vo, TITLE_BAR_PIECE, i, 6);
    }
    for (int32_t i = x + width + 2; i < 231; i += 10) {
      BltVideoObject(dst, vo, TITLE_BAR_PIECE, i, 6);
    }

    {
      int32_t const y =
          BOTTOM_Y - ACTUAL_HEIGHT - ROW_HEIGHT * std::max(guiNumUninvolved, (uint32_t)1);
      BltVideoObject(dst, vo, UNINVOLVED_HEADER, 8, y);
    }

    SetFontForeground(FONT_BEIGE);
    PrintConfined(65, 17, 64, gpStrategicString[STR_PB_LOCATION]);

    wchar_t const *const encounter =
        gubEnemyEncounterCode != CREATURE_ATTACK_CODE ? gpStrategicString[STR_PB_ENEMIES]
        : gubEnemyEncounterCode == BLOODCAT_AMBUSH_CODE ||  // XXX case is unreachable, because
                                                            // of != above
                gubEnemyEncounterCode == ENTERING_BLOODCAT_LAIR_CODE
            ? gpStrategicString[STR_PB_BLOODCATS]
            : gpStrategicString[STR_PB_CREATURES];
    PrintConfined(54, 38, 52, encounter);
    PrintConfined(139, 38, 52, gpStrategicString[STR_PB_MERCS]);
    PrintConfined(224, 38, 52, gpStrategicString[STR_PB_MILITIA]);

    // Draw the bottom columns
    for (int32_t i = 0; i < (int32_t)std::max(guiNumUninvolved, (uint32_t)1); ++i) {
      int32_t const y = BOTTOM_Y - ROW_HEIGHT * (i + 1) + 1;
      BltVideoObject(dst, vo, BOTTOM_COLUMN, 161, y);
    }

    for (int32_t i = 0; i < (int32_t)(21 - std::max(guiNumUninvolved, (uint32_t)1)); ++i) {
      int32_t const y = TOP_Y + ROW_HEIGHT * i;
      BltVideoObject(dst, vo, TOP_COLUMN, 186, y);
    }

    uint8_t const sec_x = gubPBSectorX;
    uint8_t const sec_y = gubPBSectorY;
    uint8_t const sec_z = gubPBSectorZ;

    // Location
    SetFontAttributes(FONT10ARIAL, FONT_YELLOW);
    wchar_t sector_name[128];
    GetSectorIDString(sec_x, sec_y, sec_z, sector_name, lengthof(sector_name), TRUE);
    mprintf(70, 17, L"%ls %ls", gpStrategicString[STR_PB_SECTOR], sector_name);

    SetFont(FONT14ARIAL);
    // Enemy
    wchar_t const *enemies;
    if (gubEnemyEncounterCode == CREATURE_ATTACK_CODE ||
        gubEnemyEncounterCode == BLOODCAT_AMBUSH_CODE ||
        gubEnemyEncounterCode == ENTERING_BLOODCAT_LAIR_CODE ||
        WhatPlayerKnowsAboutEnemiesInSector(sec_x, sec_y) !=
            KNOWS_HOW_MANY) {  // Don't know how many
      enemies = L"?";
    } else {  // Know exactly how many
      int32_t const n = NumEnemiesInSector(sec_x, sec_y);
      swprintf(str, lengthof(str), L"%d", n);
      enemies = str;
    }
    MPrintCentered(57, 36, 27, enemies);
    // Player
    swprintf(str, lengthof(str), L"%d", guiNumInvolved);
    MPrintCentered(142, 36, 27, str);
    // Militia
    swprintf(str, lengthof(str), L"%d", CountAllMilitiaInSector(sec_x, sec_y));
    MPrintCentered(227, 36, 27, str);
    SetFontShadow(FONT_NEARBLACK);

    SetFont(BLOCKFONT2);

    // Print the participants of the battle
    // |  NAME  | ASSIGN |  COND  |   HP   |   BP   |
    {
      int32_t y = TOP_Y + 1;
      CFOR_EACH_IN_TEAM(i, OUR_TEAM) {
        SOLDIERTYPE const &s = *i;
        if (s.bLife == 0) continue;
        if (s.uiStatusFlags & SOLDIER_VEHICLE) continue;
        if (!PlayerMercInvolvedInThisCombat(s)) continue;

        // Name
        MPrintCentered(17, y, 52, s.name);
        // Assignment
        MPrintCentered(72, y, 45, GetMapscreenMercAssignmentString(s));
        // Condition
        MPrintCentered(129, y, 58, GetSoldierConditionInfo(s));
        // HP
        swprintf(str, lengthof(str), L"%d%%", s.bLife * 100 / s.bLifeMax);
        MPrintCentered(189, y, 25, str);
        // BP
        swprintf(str, lengthof(str), L"%d%%", s.bBreath);
        MPrintCentered(217, y, 25, str);

        y += ROW_HEIGHT;
      }
    }

    // Print the uninvolved members of the battle
    // |  NAME  | ASSIGN |  LOC   |  DEST  |  DEP   |
    if (guiNumUninvolved == 0) {
      MPrintCentered(17, BOTTOM_Y - ROW_HEIGHT + 2, 52, gpStrategicString[STR_PB_NONE]);
    } else {
      int32_t y = BOTTOM_Y - ROW_HEIGHT * guiNumUninvolved + 2;
      CFOR_EACH_IN_TEAM(i, OUR_TEAM) {
        SOLDIERTYPE const &s = *i;
        if (s.bLife == 0) continue;
        if (s.uiStatusFlags & SOLDIER_VEHICLE) continue;
        if (PlayerMercInvolvedInThisCombat(s)) continue;

        // Name
        MPrintCentered(17, y, 52, s.name);
        // Assignment
        MPrintCentered(72, y, 54, GetMapscreenMercAssignmentString(s));
        // Location
        GetMapscreenMercLocationString(s, str, lengthof(str));
        MPrintCentered(128, y, 33, str);
        // Destination
        GetMapscreenMercDestinationString(s, str, lengthof(str));
        if (str[0] != L'\0') MPrintCentered(164, y, 41, str);
        // Departure
        GetMapscreenMercDepartureString(s, str, lengthof(str), 0);
        MPrintCentered(208, y, 34, str);
        y += ROW_HEIGHT;
      }
    }

    MarkAllBoxesAsAltered();
    RestoreExternBackgroundRect(0, 0, 261, 359);

    // Restore font destinanation buffer to the frame buffer
    SetFontDestBuffer(FRAME_BUFFER);
  } else if (gfBlinkHeader) {
    RenderPBHeader(&x, &width);  // The text is important enough to blink
  }

  if (gfEnterAutoResolveMode) {
    gfEnterAutoResolveMode = FALSE;
    EnterAutoResolveMode(gubPBSectorX, gubPBSectorY);
  }

  gfIgnoreAllInput = FALSE;
}

static void AutoResolveBattleCallback(GUI_BUTTON *btn, int32_t reason) {
  if (!gfIgnoreAllInput) {
    if (reason & MSYS_CALLBACK_REASON_LBUTTON_UP) {
      if (IsKeyDown(ALT) && CHEATER_CHEAT_LEVEL()) {
        if (!gfPersistantPBI) {
          return;
        }
        PlayJA2Sample(EXPLOSION_1, HIGHVOLUME, 1, MIDDLEPAN);
        gStrategicStatus.usPlayerKills += NumEnemiesInSector(gubPBSectorX, gubPBSectorY);
        EliminateAllEnemies(gubPBSectorX, gubPBSectorY);
        SetMusicMode(MUSIC_TACTICAL_VICTORY);
        btn->uiFlags &= ~BUTTON_CLICKED_ON;
        btn->Draw();
        InvalidateRegion(btn->X(), btn->Y(), btn->BottomRightX(), btn->BottomRightY());
        ExecuteBaseDirtyRectQueue();
        EndFrameBufferRender();
        RefreshScreen();
        KillPreBattleInterface();
        StopTimeCompression();
        SetMusicMode(MUSIC_TACTICAL_NOTHING);
        return;
      }
      gfEnterAutoResolveMode = TRUE;
    }
  }
}

static void ClearMovementForAllInvolvedPlayerGroups();
static void PutNonSquadMercsInBattleSectorOnSquads(BOOLEAN fExitVehicles);

static void GoToSectorCallback(GUI_BUTTON *btn, int32_t reason) {
  if (!gfIgnoreAllInput) {
    if (reason & MSYS_CALLBACK_REASON_LBUTTON_UP) {
      if (IsKeyDown(ALT) && CHEATER_CHEAT_LEVEL()) {
        if (!gfPersistantPBI) {
          return;
        }
        PlayJA2Sample(EXPLOSION_1, HIGHVOLUME, 1, MIDDLEPAN);
        gStrategicStatus.usPlayerKills += NumEnemiesInSector(gubPBSectorX, gubPBSectorY);
        EliminateAllEnemies(gubPBSectorX, gubPBSectorY);
        SetMusicMode(MUSIC_TACTICAL_VICTORY);
        btn->uiFlags &= ~BUTTON_CLICKED_ON;
        btn->Draw();
        InvalidateRegion(btn->X(), btn->Y(), btn->BottomRightX(), btn->BottomRightY());
        ExecuteBaseDirtyRectQueue();
        EndFrameBufferRender();
        RefreshScreen();
        KillPreBattleInterface();
        StopTimeCompression();
        SetMusicMode(MUSIC_TACTICAL_NOTHING);
        return;
      }
      if (gfPersistantPBI && gpBattleGroup && gpBattleGroup->fPlayer &&
          gubEnemyEncounterCode != ENEMY_AMBUSH_CODE &&
          gubEnemyEncounterCode != CREATURE_ATTACK_CODE &&
          gubEnemyEncounterCode != BLOODCAT_AMBUSH_CODE) {
        gfEnterTacticalPlacementGUI = TRUE;
      }
      btn->uiFlags &= ~BUTTON_CLICKED_ON;
      btn->Draw();
      InvalidateRegion(btn->X(), btn->Y(), btn->BottomRightX(), btn->BottomRightY());
      ExecuteBaseDirtyRectQueue();
      EndFrameBufferRender();
      RefreshScreen();
      if (gubPBSectorX == gWorldSectorX && gubPBSectorY == gWorldSectorY && !gbWorldSectorZ) {
        gfGotoSectorTransition = TRUE;
      }

      // first time going to the sector?
      if (gfPersistantPBI) {
        // put everyone on duty, and remove mercs from vehicles, too
        PutNonSquadMercsInBattleSectorOnSquads(TRUE);

        // we nuke the groups existing route & destination in advance
        ClearMovementForAllInvolvedPlayerGroups();
      } else {  // Clear the battlegroup pointer.
        gpBattleGroup = NULL;
      }

      // must come AFTER anything that needs gpBattleGroup, as it wipes it out
      SetCurrentWorldSector(gubPBSectorX, gubPBSectorY, gubPBSectorZ);

      KillPreBattleInterface();
    }
  }
}

static void RetreatMercsCallback(GUI_BUTTON *btn, int32_t reason) {
  if (!gfIgnoreAllInput) {
    if (reason & MSYS_CALLBACK_REASON_LBUTTON_UP) {
      // get them outta here!
      RetreatAllInvolvedPlayerGroups();

      // NOTE: this code assumes you can never retreat while underground
      HandleLoyaltyImplicationsOfMercRetreat(RETREAT_PBI, gubPBSectorX, gubPBSectorY, 0);
      if (CountAllMilitiaInSector(gubPBSectorX, gubPBSectorY)) {  // Mercs retreat, but enemies
                                                                  // still need to fight the militia
        gfEnterAutoResolveMode = TRUE;
        return;
      }

      // Warp time by 5 minutes so that player can't just go back into the
      // sector he left.
      WarpGameTime(300, WARPTIME_NO_PROCESSING_OF_EVENTS);
      ResetMovementForEnemyGroupsInLocation(gubPBSectorX, gubPBSectorY);

      btn->uiFlags &= ~BUTTON_CLICKED_ON;
      btn->Draw();
      InvalidateRegion(btn->X(), btn->Y(), btn->BottomRightX(), btn->BottomRightY());
      ExecuteBaseDirtyRectQueue();
      EndFrameBufferRender();
      RefreshScreen();
      KillPreBattleInterface();
      StopTimeCompression();
      gpBattleGroup = NULL;
      gfBlitBattleSectorLocator = FALSE;

      SetMusicMode(MUSIC_TACTICAL_NOTHING);
    }
  }
}

enum {
  COND_EXCELLENT,
  COND_GOOD,
  COND_FAIR,
  COND_WOUNDED,
  COND_FATIGUED,
  COND_BLEEDING,
  COND_UNCONCIOUS,
  COND_DYING,
  COND_DEAD
};

static wchar_t const *GetSoldierConditionInfo(SOLDIERTYPE const &s) {
  // Go from the worst condition to the best
  return s.bLife == 0                           ? pConditionStrings[COND_DEAD]
         : s.bLife < OKLIFE && s.bBleeding != 0 ? pConditionStrings[COND_DYING]
         : s.bBreath < OKBREATH && s.bCollapsed ? pConditionStrings[COND_UNCONCIOUS]
         : s.bBleeding > MIN_BLEEDING_THRESHOLD ? pConditionStrings[COND_BLEEDING]
         : s.bLife * 100 < s.bLifeMax * 50      ? pConditionStrings[COND_WOUNDED]
         : s.bBreath < 50                       ? pConditionStrings[COND_FATIGUED]
         : s.bLife * 100 < s.bLifeMax * 67      ? pConditionStrings[COND_FAIR]
         : s.bLife * 100 < s.bLifeMax * 86      ? pConditionStrings[COND_GOOD]
                                                : pConditionStrings[COND_EXCELLENT];
}

void ActivatePreBattleAutoresolveAction() {
  if (iPBButton[0]->Enabled()) {  // Feign call the autoresolve button using the callback
    AutoResolveBattleCallback(iPBButton[0], MSYS_CALLBACK_REASON_LBUTTON_UP);
  }
}

void ActivatePreBattleEnterSectorAction() {
  if (iPBButton[1]->Enabled()) {  // Feign call the enter sector button using the
                                  // callback
    GoToSectorCallback(iPBButton[1], MSYS_CALLBACK_REASON_LBUTTON_UP);
  }
}

void ActivatePreBattleRetreatAction() {
  if (iPBButton[2]->Enabled()) {  // Feign call the retreat button using the callback
    RetreatMercsCallback(iPBButton[2], MSYS_CALLBACK_REASON_LBUTTON_UP);
  }
}

static void ActivateAutomaticAutoResolveStart() {
  iPBButton[0]->uiFlags |= BUTTON_CLICKED_ON;
  gfIgnoreAllInput = FALSE;
  AutoResolveBattleCallback(iPBButton[0], MSYS_CALLBACK_REASON_LBUTTON_UP);
}

void CalculateNonPersistantPBIInfo() {
  // We need to set up the non-persistant PBI
  if (!gfBlitBattleSectorLocator || gubPBSectorX != gWorldSectorX ||
      gubPBSectorY != gWorldSectorY ||
      gubPBSectorZ != gbWorldSectorZ) {  // Either the locator isn't on or the
                                         // locator info is in a different sector

    // Calculated the encounter type
    gubEnemyEncounterCode = NO_ENCOUNTER_CODE;
    gubExplicitEnemyEncounterCode = NO_ENCOUNTER_CODE;
    if (HostileCiviliansPresent()) {  // There are hostile civilians, so no
                                      // autoresolve allowed.
      gubExplicitEnemyEncounterCode = HOSTILE_CIVILIANS_CODE;
    } else if (HostileBloodcatsPresent()) {  // There are bloodcats in the
                                             // sector, so no autoresolve allowed
      gubExplicitEnemyEncounterCode = HOSTILE_BLOODCATS_CODE;
    } else if (gbWorldSectorZ) {
      UNDERGROUND_SECTORINFO *pSector =
          FindUnderGroundSector(gWorldSectorX, gWorldSectorY, gbWorldSectorZ);
      Assert(pSector);
      if (pSector->ubCreaturesInBattle) {
        gubExplicitEnemyEncounterCode = FIGHTING_CREATURES_CODE;
      } else if (pSector->ubAdminsInBattle || pSector->ubTroopsInBattle ||
                 pSector->ubElitesInBattle) {
        gubExplicitEnemyEncounterCode = ENTERING_ENEMY_SECTOR_CODE;
        gubEnemyEncounterCode = ENTERING_ENEMY_SECTOR_CODE;
      }
    } else {
      SECTORINFO *pSector = &SectorInfo[SECTOR(gWorldSectorX, gWorldSectorY)];
      Assert(pSector);
      if (pSector->ubCreaturesInBattle) {
        gubExplicitEnemyEncounterCode = FIGHTING_CREATURES_CODE;
      } else if (pSector->ubAdminsInBattle || pSector->ubTroopsInBattle ||
                 pSector->ubElitesInBattle) {
        gubExplicitEnemyEncounterCode = ENTERING_ENEMY_SECTOR_CODE;
        gubEnemyEncounterCode = ENTERING_ENEMY_SECTOR_CODE;
      }
    }
    if (gubExplicitEnemyEncounterCode != NO_ENCOUNTER_CODE) {  // Set up the location as well as
                                                               // turning on the blit flag.
      gubPBSectorX = (uint8_t)gWorldSectorX;
      gubPBSectorY = (uint8_t)gWorldSectorY;
      gubPBSectorZ = (uint8_t)gbWorldSectorZ;
      gfBlitBattleSectorLocator = TRUE;
    }
  }
}

static void PutNonSquadMercsInPlayerGroupOnSquads(GROUP *pGroup, BOOLEAN fExitVehicles);

static void PutNonSquadMercsInBattleSectorOnSquads(BOOLEAN fExitVehicles) {
  // IMPORTANT: Have to do this by group, so everyone inside vehicles gets
  // assigned to the same squad.  Needed for the tactical placement interface to
  // work in case of simultaneous multi-vehicle arrivals!
  FOR_EACH_GROUP_SAFE(i) {
    GROUP &g = *i;
    if (!PlayerGroupInvolvedInThisCombat(g)) continue;

    // the helicopter group CAN be involved, if it's on the ground, in which
    // case everybody must get out of it
    if (IsGroupTheHelicopterGroup(g)) {
      // only happens if chopper is on the ground...
      Assert(!fHelicopterIsAirBorne);

      // put anyone in it into movement group
      MoveAllInHelicopterToFootMovementGroup();
    } else {
      PutNonSquadMercsInPlayerGroupOnSquads(&g, fExitVehicles);
    }
  }
}

static void PutNonSquadMercsInPlayerGroupOnSquads(GROUP *const pGroup,
                                                  const BOOLEAN fExitVehicles) {
  int8_t bUniqueVehicleSquad = -1;
  if (pGroup->fVehicle) {
    // put these guys on their own squad (we need to return their group ID, and
    // can only return one, so they need a unique one
    bUniqueVehicleSquad = GetFirstEmptySquad();
  }

  PLAYERGROUP *next;
  for (PLAYERGROUP *p = pGroup->pPlayerList; p; p = next) {
    Assert(p->pSoldier);
    SOLDIERTYPE &s = *p->pSoldier;

    // store ptr to next soldier in group, once removed from group, his info
    // will get memfree'd!
    next = p->next;

    if (!s.bActive || s.bLife == 0 || s.uiStatusFlags & SOLDIER_VEHICLE) continue;

    if (!PlayerMercInvolvedInThisCombat(s) || s.bAssignment < ON_DUTY) continue;
    // if involved, but off-duty (includes mercs inside vehicles!)

    // if in a vehicle, pull him out
    if (s.bAssignment == VEHICLE) {
      if (fExitVehicles) {
        TakeSoldierOutOfVehicle(&s);

        /* put them on the unique squad assigned to people leaving this vehicle.
         * Can't add them to existing squads, because if this is a simultaneous
         * group attack, the mercs could be coming from different sides, and the
         * placement screen can't handle mercs on the same squad arriving from
         * different edges! */
        BOOLEAN const fSuccess = AddCharacterToSquad(&s, bUniqueVehicleSquad);
        (void)fSuccess;
        Assert(fSuccess);
      }
    } else {
      // add him to ANY on duty foot squad
      AddCharacterToAnySquad(&s);
    }

    // stand him up
    MakeSoldiersTacticalAnimationReflectAssignment(&s);
  }
}

void WakeUpAllMercsInSectorUnderAttack() {
  FOR_EACH_IN_TEAM(i, OUR_TEAM) {
    SOLDIERTYPE &s = *i;
    if (s.bLife == 0) continue;
    if (s.uiStatusFlags & SOLDIER_VEHICLE) continue;
    if (!s.fMercAsleep) continue;
    if (!PlayerMercInvolvedInThisCombat(s)) continue;
    // Involved, but asleep, force him wake him up
    SetMercAwake(&s, FALSE, TRUE);
  }
}

// we are entering the sector, clear out all mvt orders for grunts
static void ClearMovementForAllInvolvedPlayerGroups() {
  FOR_EACH_GROUP(i) {
    GROUP &g = *i;
    if (!PlayerGroupInvolvedInThisCombat(g)) continue;
    // clear their strategic movement (mercpaths and waypoints)
    ClearMercPathsAndWaypointsForAllInGroup(g);
  }
}

void RetreatAllInvolvedPlayerGroups() {
  // make sure guys stop their off duty assignments, like militia training!
  // but don't exit vehicles - drive off in them!
  PutNonSquadMercsInBattleSectorOnSquads(FALSE);

  FOR_EACH_GROUP(i) {
    GROUP &g = *i;
    if (!PlayerGroupInvolvedInThisCombat(g)) continue;
    // Don't retreat empty vehicle groups!
    if (g.fVehicle && !DoesVehicleGroupHaveAnyPassengers(g)) continue;
    ClearMercPathsAndWaypointsForAllInGroup(g);
    RetreatGroupToPreviousSector(g);
  }
}

static BOOLEAN CurrentBattleSectorIs(int16_t sSectorX, int16_t sSectorY, int16_t sSectorZ);

bool PlayerMercInvolvedInThisCombat(SOLDIERTYPE const &s) {
  Assert(s.bActive);
  return !s.fBetweenSectors && s.bAssignment != IN_TRANSIT && s.bAssignment != ASSIGNMENT_POW &&
         s.bAssignment != ASSIGNMENT_DEAD && !(s.uiStatusFlags & SOLDIER_VEHICLE) &&
         // Robot is involved iff it has a valid controller with it
         (!AM_A_ROBOT(&s) || s.robot_remote_holder) && !SoldierAboardAirborneHeli(s) &&
         CurrentBattleSectorIs(s.sSectorX, s.sSectorY, s.bSectorZ);
}

bool PlayerGroupInvolvedInThisCombat(GROUP const &g) {
  /* Player group, non-empty, not between sectors, in the right sector, isn't a
   * group of in transit, dead, or POW mercs, and either not the helicopter
   * group, or the heli is on the ground */
  return g.fPlayer && g.ubGroupSize != 0 && !g.fBetweenSectors &&
         !GroupHasInTransitDeadOrPOWMercs(g) &&
         (!IsGroupTheHelicopterGroup(g) || !fHelicopterIsAirBorne) &&
         CurrentBattleSectorIs(g.ubSectorX, g.ubSectorY, g.ubSectorZ);
}

static BOOLEAN CurrentBattleSectorIs(int16_t sSectorX, int16_t sSectorY, int16_t sSectorZ) {
  int16_t sBattleSectorX, sBattleSectorY, sBattleSectorZ;
  BOOLEAN fSuccess;

  fSuccess = GetCurrentBattleSectorXYZ(&sBattleSectorX, &sBattleSectorY, &sBattleSectorZ);
  Assert(fSuccess);

  if ((sSectorX == sBattleSectorX) && (sSectorY == sBattleSectorY) &&
      (sSectorZ == sBattleSectorZ)) {
    // yup!
    return (TRUE);
  } else {
    // wrong sector, no battle here
    return (FALSE);
  }
}

static void CheckForRobotAndIfItsControlled() {
  // search for the robot on player's team
  FOR_EACH_IN_TEAM(s, OUR_TEAM) {
    if (s->bLife != 0 && AM_A_ROBOT(s)) {
      // check whether it has a valid controller with it. This sets its
      // robot_remote_holder field.
      UpdateRobotControllerGivenRobot(s);

      // if he has a controller, set controllers
      if (s->robot_remote_holder != NULL) {
        UpdateRobotControllerGivenController(s->robot_remote_holder);
      }

      break;
    }
  }
}

void LogBattleResults(const uint8_t ubVictoryCode) {
  int16_t sSectorX;
  int16_t sSectorY;
  int16_t sSectorZ;
  GetCurrentBattleSectorXYZ(&sSectorX, &sSectorY, &sSectorZ);
  uint8_t code;
  if (ubVictoryCode == LOG_VICTORY) {
    switch (gubEnemyEncounterCode) {
      case ENEMY_INVASION_CODE:
        code = HISTORY_DEFENDEDTOWNSECTOR;
        break;
      case ENEMY_ENCOUNTER_CODE:
        code = HISTORY_WONBATTLE;
        break;
      case ENEMY_AMBUSH_CODE:
        code = HISTORY_WIPEDOUTENEMYAMBUSH;
        break;
      case ENTERING_ENEMY_SECTOR_CODE:
        code = HISTORY_SUCCESSFULATTACK;
        break;
      case CREATURE_ATTACK_CODE:
        code = HISTORY_CREATURESATTACKED;
        break;
      case BLOODCAT_AMBUSH_CODE:
      case ENTERING_BLOODCAT_LAIR_CODE:
        code = HISTORY_SLAUGHTEREDBLOODCATS;
        break;
      default:
        return;
    }
  } else {
    switch (gubEnemyEncounterCode) {
      case ENEMY_INVASION_CODE:
        code = HISTORY_LOSTTOWNSECTOR;
        break;
      case ENEMY_ENCOUNTER_CODE:
        code = HISTORY_LOSTBATTLE;
        break;
      case ENEMY_AMBUSH_CODE:
        code = HISTORY_FATALAMBUSH;
        break;
      case ENTERING_ENEMY_SECTOR_CODE:
        code = HISTORY_UNSUCCESSFULATTACK;
        break;
      case CREATURE_ATTACK_CODE:
        code = HISTORY_CREATURESATTACKED;
        break;
      case BLOODCAT_AMBUSH_CODE:
      case ENTERING_BLOODCAT_LAIR_CODE:
        code = HISTORY_KILLEDBYBLOODCATS;
        break;
      default:
        return;
    }
  }
  AddHistoryToPlayersLog(code, 0, GetWorldTotalMin(), sSectorX, sSectorY);
}

void HandlePreBattleInterfaceStates() {
  if (gfEnteringMapScreenToEnterPreBattleInterface && !gfEnteringMapScreen) {
    gfEnteringMapScreenToEnterPreBattleInterface = FALSE;
    if (!gfUsePersistantPBI) {
      InitPreBattleInterface(0, false);
      gfUsePersistantPBI = TRUE;
    } else {
      InitPreBattleInterface(gpBattleGroup, true);
    }
  } else if (gfDelayAutoResolveStart && gfPreBattleInterfaceActive) {
    gfDelayAutoResolveStart = FALSE;
    gfAutomaticallyStartAutoResolve = TRUE;
  } else if (gfAutomaticallyStartAutoResolve) {
    gfAutomaticallyStartAutoResolve = FALSE;
    ActivateAutomaticAutoResolveStart();
  }
}
