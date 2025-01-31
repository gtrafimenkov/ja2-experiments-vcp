// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#include "Strategic/TownMilitia.h"

#include <stdio.h>
#include <string.h>
#include <wchar.h>

#include "JAScreens.h"
#include "Laptop/Finances.h"
#include "Laptop/LaptopSave.h"
#include "Local.h"
#include "Macro.h"
#include "MessageBoxScreen.h"
#include "SGP/Debug.h"
#include "SGP/Random.h"
#include "ScreenIDs.h"
#include "Strategic/Assignments.h"
#include "Strategic/CampaignTypes.h"
#include "Strategic/GameClock.h"
#include "Strategic/MapScreen.h"
#include "Strategic/MapScreenInterface.h"
#include "Strategic/MapScreenInterfaceBorder.h"
#include "Strategic/MapScreenInterfaceMap.h"
#include "Strategic/PreBattleInterface.h"
#include "Strategic/QueenCommand.h"
#include "Strategic/StrategicMap.h"
#include "Strategic/StrategicTownLoyalty.h"
#include "Tactical/DialogueControl.h"
#include "Tactical/Interface.h"
#include "Tactical/MilitiaControl.h"
#include "Tactical/Overhead.h"
#include "Tactical/SoldierCreate.h"
#include "Tactical/Squads.h"
#include "Utils/Text.h"

#define SIZE_OF_MILITIA_COMPLETED_TRAINING_LIST 50

// temporary local global variables
uint8_t gubTownSectorServerTownId = BLANK_SECTOR;
int16_t gsTownSectorServerSkipX = -1;
int16_t gsTownSectorServerSkipY = -1;
uint8_t gubTownSectorServerIndex = 0;
BOOLEAN gfYesNoPromptIsForContinue = FALSE;  // this flag remembers whether we're starting new
                                             // training, or continuing
int32_t giTotalCostOfTraining = 0;

// the completed list of sector soldiers for training militia
static SOLDIERTYPE
    *g_list_of_merc_in_sectors_completed_militia_training[SIZE_OF_MILITIA_COMPLETED_TRAINING_LIST];
SOLDIERTYPE *pMilitiaTrainerSoldier = NULL;

// note that these sector values are STRATEGIC INDEXES, not 0-255!
static int16_t gsUnpaidStrategicSector[MAX_CHARACTER_COUNT];

static void HandleCompletionOfTownTrainingByGroupWithTrainer(SOLDIERTYPE *pTrainer);
static void InitFriendlyTownSectorServer(uint8_t ubTownId, int16_t sSkipSectorX,
                                         int16_t sSkipSectorY);
static bool ServeNextFriendlySectorInTown(int16_t *neighbour_x, int16_t *neighbour_y);
static void StrategicAddMilitiaToSector(int16_t sMapX, int16_t sMapY, uint8_t ubRank,
                                        uint8_t ubHowMany);
static void StrategicPromoteMilitiaInSector(int16_t x, int16_t y, uint8_t current_rank, uint8_t n);

void TownMilitiaTrainingCompleted(SOLDIERTYPE *pTrainer, int16_t sMapX, int16_t sMapY) {
  uint8_t ubMilitiaTrained = 0;
  BOOLEAN fFoundOne;
  int16_t sNeighbourX, sNeighbourY;
  uint8_t ubTownId;

  // get town index
  ubTownId = StrategicMap[sMapX + sMapY * MAP_WORLD_X].bNameId;

  if (ubTownId == BLANK_SECTOR) {
    Assert(IsThisSectorASAMSector(sMapX, sMapY, 0));
  }

  // force tactical to update militia status
  gfStrategicMilitiaChangesMade = TRUE;

  // ok, so what do we do with all this training?  Well, in order of decreasing
  // priority: 1) If there's room in training sector, create new GREEN militia
  // guys there 2) If not enough room there, create new GREEN militia guys in
  // friendly sectors of the same town 3) If not enough room anywhere in town,
  // promote a number of GREENs in this sector into regulars 4) If not enough
  // GREENS there to promote, promote GREENs in other sectors. 5) If all
  // friendly sectors of this town are completely filled with REGULAR militia,
  // then training effect is wasted

  while (ubMilitiaTrained < MILITIA_TRAINING_SQUAD_SIZE) {
    // is there room for another militia in the training sector itself?
    if (CountAllMilitiaInSector(sMapX, sMapY) < MAX_ALLOWABLE_MILITIA_PER_SECTOR) {
      // great! Create a new GREEN militia guy in the training sector
      StrategicAddMilitiaToSector(sMapX, sMapY, GREEN_MILITIA, 1);
    } else {
      fFoundOne = FALSE;

      if (ubTownId != BLANK_SECTOR) {
        InitFriendlyTownSectorServer(ubTownId, sMapX, sMapY);

        // check other eligible sectors in this town for room for another
        // militia
        while (ServeNextFriendlySectorInTown(&sNeighbourX, &sNeighbourY)) {
          // is there room for another militia in this neighbouring sector ?
          if (CountAllMilitiaInSector(sNeighbourX, sNeighbourY) <
              MAX_ALLOWABLE_MILITIA_PER_SECTOR) {
            // great! Create a new GREEN militia guy in the neighbouring sector
            StrategicAddMilitiaToSector(sNeighbourX, sNeighbourY, GREEN_MILITIA, 1);

            fFoundOne = TRUE;
            break;
          }
        }
      }

      // if we still haven't been able to train anyone
      if (!fFoundOne) {
        // alrighty, then.  We'll have to *promote* guys instead.

        // are there any GREEN militia men in the training sector itself?
        if (MilitiaInSectorOfRank(sMapX, sMapY, GREEN_MILITIA) > 0) {
          // great! Promote a GREEN militia guy in the training sector to a
          // REGULAR
          StrategicPromoteMilitiaInSector(sMapX, sMapY, GREEN_MILITIA, 1);
        } else {
          if (ubTownId != BLANK_SECTOR) {
            // dammit! Last chance - try to find other eligible sectors in the
            // same town with a Green guy to be promoted
            InitFriendlyTownSectorServer(ubTownId, sMapX, sMapY);

            // check other eligible sectors in this town for room for another
            // militia
            while (ServeNextFriendlySectorInTown(&sNeighbourX, &sNeighbourY)) {
              // are there any GREEN militia men in the neighbouring sector ?
              if (MilitiaInSectorOfRank(sNeighbourX, sNeighbourY, GREEN_MILITIA) > 0) {
                // great! Promote a GREEN militia guy in the neighbouring sector
                // to a REGULAR
                StrategicPromoteMilitiaInSector(sNeighbourX, sNeighbourY, GREEN_MILITIA, 1);

                fFoundOne = TRUE;
                break;
              }
            }
          }

          // if we still haven't been able to train anyone
          if (!fFoundOne) {
            // Well, that's it.  All eligible sectors of this town are full of
            // REGULARs or ELITEs. The training goes to waste in this situation.
            break;  // the main while loop
          }
        }
      }
    }

    // next, please!
    ubMilitiaTrained++;
  }

  // if anyone actually got trained
  if (ubMilitiaTrained > 0) {
    // update the screen display
    fMapPanelDirty = TRUE;

    if (ubTownId != BLANK_SECTOR) {
      // loyalty in this town increases a bit because we obviously care about
      // them...
      IncrementTownLoyalty(ubTownId, LOYALTY_BONUS_FOR_TOWN_TRAINING);
    }
  }

  // the trainer announces to player that he's finished his assignment.  Make
  // his sector flash!
  AssignmentDone(pTrainer, TRUE, FALSE);

  // handle completion of town by training group
  HandleCompletionOfTownTrainingByGroupWithTrainer(pTrainer);
}

int8_t SoldierClassToMilitiaRank(uint8_t const soldier_class) {
  switch (soldier_class) {
    case SOLDIER_CLASS_GREEN_MILITIA:
      return GREEN_MILITIA;
    case SOLDIER_CLASS_REG_MILITIA:
      return REGULAR_MILITIA;
    case SOLDIER_CLASS_ELITE_MILITIA:
      return ELITE_MILITIA;
    default:
      return -1;
  }
}

// add militias of a certain rank
static void StrategicAddMilitiaToSector(int16_t sMapX, int16_t sMapY, uint8_t ubRank,
                                        uint8_t ubHowMany) {
  SECTORINFO *pSectorInfo = &(SectorInfo[SECTOR(sMapX, sMapY)]);

  pSectorInfo->ubNumberOfCivsAtLevel[ubRank] += ubHowMany;

  // update the screen display
  fMapPanelDirty = TRUE;
}

// Promote militias of a certain rank
static void StrategicPromoteMilitiaInSector(int16_t const x, int16_t const y,
                                            uint8_t const current_rank, uint8_t const n) {
  SECTORINFO &si = SectorInfo[SECTOR(x, y)];

  Assert(si.ubNumberOfCivsAtLevel[current_rank] >= n);
  // KM : July 21, 1999 patch fix
  if (si.ubNumberOfCivsAtLevel[current_rank] < n) return;

  si.ubNumberOfCivsAtLevel[current_rank] -= n;
  si.ubNumberOfCivsAtLevel[current_rank + 1] += n;

  fMapPanelDirty = TRUE;
}

void StrategicRemoveMilitiaFromSector(int16_t sMapX, int16_t sMapY, uint8_t ubRank,
                                      uint8_t ubHowMany) {
  SECTORINFO *pSectorInfo = &(SectorInfo[SECTOR(sMapX, sMapY)]);

  // damn well better have that many around to remove!
  Assert(pSectorInfo->ubNumberOfCivsAtLevel[ubRank] >= ubHowMany);

  // KM : July 21, 1999 patch fix
  if (pSectorInfo->ubNumberOfCivsAtLevel[ubRank] < ubHowMany) {
    return;
  }

  pSectorInfo->ubNumberOfCivsAtLevel[ubRank] -= ubHowMany;

  // update the screen display
  fMapPanelDirty = TRUE;
}

// kill pts are (2 * kills) + assists
uint8_t CheckOneMilitiaForPromotion(int16_t const x, int16_t const y, uint8_t const current_rank,
                                    uint8_t kill_points) {
  uint8_t n_promotions = 0;
  switch (current_rank) {
    case GREEN_MILITIA:
      // 2 kill points minimum, 25% chance per kill point
      if (kill_points < 2) break;
      if (!Chance(25 * kill_points)) break;
      StrategicPromoteMilitiaInSector(x, y, GREEN_MILITIA, 1);
      ++n_promotions;
      // Attempt another level up
      kill_points -= 2;
      /* FALLTHROUGH */
    case REGULAR_MILITIA:
      // 5 kill points minimum, 10% chance per kill point
      if (kill_points < 5) break;
      if (!Chance(10 * kill_points)) break;
      StrategicPromoteMilitiaInSector(x, y, REGULAR_MILITIA, 1);
      ++n_promotions;
      break;
  }
  return n_promotions;
}

// call this if the player attacks his own militia
static void HandleMilitiaDefections(int16_t sMapX, int16_t sMapY) {
  uint8_t ubRank;
  uint8_t ubMilitiaCnt;
  uint8_t ubCount;
  uint32_t uiChanceToDefect;

  for (ubRank = 0; ubRank < MAX_MILITIA_LEVELS; ubRank++) {
    ubMilitiaCnt = MilitiaInSectorOfRank(sMapX, sMapY, ubRank);

    // check each guy at each rank to see if he defects
    for (ubCount = 0; ubCount < ubMilitiaCnt; ubCount++) {
      switch (ubRank) {
        case GREEN_MILITIA:
          uiChanceToDefect = 50;
          break;
        case REGULAR_MILITIA:
          uiChanceToDefect = 75;
          break;
        case ELITE_MILITIA:
          uiChanceToDefect = 90;
          break;
        default:
          Assert(0);
          return;
      }

      // roll the bones; should I stay or should I go now?  (for you music fans
      // out there)
      if (Random(100) < uiChanceToDefect) {
        // B'bye!  (for you SNL fans out there)
        StrategicRemoveMilitiaFromSector(sMapX, sMapY, ubRank, 1);
      }
    }
  }
}

uint8_t CountAllMilitiaInSector(int16_t sMapX, int16_t sMapY) {
  uint8_t ubMilitiaTotal = 0;
  uint8_t ubRank;

  // find out if there are any town militia in this SECTOR (don't care about
  // other sectors in same town)
  for (ubRank = 0; ubRank < MAX_MILITIA_LEVELS; ubRank++) {
    ubMilitiaTotal += MilitiaInSectorOfRank(sMapX, sMapY, ubRank);
  }

  return (ubMilitiaTotal);
}

uint8_t MilitiaInSectorOfRank(int16_t sMapX, int16_t sMapY, uint8_t ubRank) {
  return (SectorInfo[SECTOR(sMapX, sMapY)].ubNumberOfCivsAtLevel[ubRank]);
}

BOOLEAN SectorOursAndPeaceful(int16_t sMapX, int16_t sMapY, int8_t bMapZ) {
  // if this sector is currently loaded
  if ((sMapX == gWorldSectorX) && (sMapY == gWorldSectorY) && (bMapZ == gbWorldSectorZ)) {
    // and either there are enemies prowling this sector, or combat is in
    // progress
    if (gTacticalStatus.fEnemyInSector || (gTacticalStatus.uiFlags & INCOMBAT)) {
      return FALSE;
    }
  }

  // if sector is controlled by enemies, it's not ours (duh!)
  if (!bMapZ && StrategicMap[sMapX + sMapY * MAP_WORLD_X].fEnemyControlled) {
    return FALSE;
  }

  if (NumHostilesInSector(sMapX, sMapY, bMapZ)) {
    return FALSE;
  }

  // safe & secure, s'far as we can tell
  return (TRUE);
}

static void InitFriendlyTownSectorServer(uint8_t ubTownId, int16_t sSkipSectorX,
                                         int16_t sSkipSectorY) {
  // reset globals
  gubTownSectorServerTownId = ubTownId;
  gsTownSectorServerSkipX = sSkipSectorX;
  gsTownSectorServerSkipY = sSkipSectorY;

  gubTownSectorServerIndex = 0;
}

/* Fetch the X,Y of the next town sector on the town list for the town specified
 * at initialization. It will skip an entry that matches the skip X/Y value, if
 * one was specified at initialization.
 * MUST CALL InitFriendlyTownSectorServer() before using! */
static bool ServeNextFriendlySectorInTown(int16_t *const neighbour_x, int16_t *const neighbour_y) {
  while (g_town_sectors[gubTownSectorServerIndex].town != BLANK_SECTOR) {
    int32_t const sector = g_town_sectors[gubTownSectorServerIndex++].sector;

    // if this sector is in the town we're looking for
    if (StrategicMap[SECTOR_INFO_TO_STRATEGIC_INDEX(sector)].bNameId != gubTownSectorServerTownId)
      continue;

    int16_t const x = SECTORX(sector);
    int16_t const y = SECTORY(sector);

    // Make sure we're not supposed to skip it
    if (x == gsTownSectorServerSkipX && y == gsTownSectorServerSkipY) continue;

    // check if it's "friendly" - not enemy controlled, no enemies in it, no
    // combat in progress
    if (!SectorOursAndPeaceful(x, y, 0)) continue;

    *neighbour_x = x;
    *neighbour_y = y;
    return true;
  }
  return false;
}

static void CantTrainMilitiaOkBoxCallback(MessageBoxReturnValue);
static int32_t GetNumberOfUnpaidTrainableSectors();
static void PayMilitiaTrainingYesNoBoxCallback(MessageBoxReturnValue);

void HandleInterfaceMessageForCostOfTrainingMilitia(SOLDIERTYPE *pSoldier) {
  wchar_t sString[128];
  int32_t iNumberOfSectors = 0;

  pMilitiaTrainerSoldier = pSoldier;

  // grab total number of sectors
  iNumberOfSectors = GetNumberOfUnpaidTrainableSectors();
  Assert(iNumberOfSectors > 0);

  // get total cost
  giTotalCostOfTraining = MILITIA_TRAINING_COST * iNumberOfSectors;
  Assert(giTotalCostOfTraining > 0);

  gfYesNoPromptIsForContinue = FALSE;

  if (LaptopSaveInfo.iCurrentBalance < giTotalCostOfTraining) {
    swprintf(sString, lengthof(sString), pMilitiaConfirmStrings[7], giTotalCostOfTraining);
    DoScreenIndependantMessageBox(sString, MSG_BOX_FLAG_OK, CantTrainMilitiaOkBoxCallback);
    return;
  }

  // ok to start training, ask player

  if (iNumberOfSectors > 1) {
    swprintf(sString, lengthof(sString), pMilitiaConfirmStrings[6], iNumberOfSectors,
             giTotalCostOfTraining, pMilitiaConfirmStrings[1]);
  } else {
    swprintf(sString, lengthof(sString), L"%ls%d. %ls", pMilitiaConfirmStrings[0],
             giTotalCostOfTraining, pMilitiaConfirmStrings[1]);
  }

  // if we are in mapscreen, make a pop up
  if (guiCurrentScreen == MAP_SCREEN) {
    DoMapMessageBox(MSG_BOX_BASIC_STYLE, sString, MAP_SCREEN, MSG_BOX_FLAG_YESNO,
                    PayMilitiaTrainingYesNoBoxCallback);
  } else {
    SGPBox const centering_rect = {0, 0, SCREEN_WIDTH, INV_INTERFACE_START_Y};
    DoMessageBox(MSG_BOX_BASIC_STYLE, sString, GAME_SCREEN, MSG_BOX_FLAG_YESNO,
                 PayMilitiaTrainingYesNoBoxCallback, &centering_rect);
  }
}

static void DoContinueMilitiaTrainingMessageBox(int16_t const sSectorX, int16_t const sSectorY,
                                                wchar_t const *const str,
                                                MessageBoxFlags const usFlags,
                                                MSGBOX_CALLBACK const ReturnCallback) {
  if (sSectorX <= 10 && sSectorY >= 6 && sSectorY <= 11) {
    DoLowerScreenIndependantMessageBox(str, usFlags, ReturnCallback);
  } else {
    DoScreenIndependantMessageBox(str, usFlags, ReturnCallback);
  }
}

// continue training?
static void HandleInterfaceMessageForContinuingTrainingMilitia(SOLDIERTYPE *const pSoldier) {
  wchar_t sString[128];
  int16_t sSectorX = 0, sSectorY = 0;
  wchar_t sStringB[128];

  sSectorX = pSoldier->sSectorX;
  sSectorY = pSoldier->sSectorY;
  uint8_t const sector = SECTOR(sSectorX, sSectorY);

  Assert(!SectorInfo[sector].fMilitiaTrainingPaid);

  pMilitiaTrainerSoldier = pSoldier;

  gfYesNoPromptIsForContinue = TRUE;

  // is there enough loyalty to continue training
  if (!DoesSectorMercIsInHaveSufficientLoyaltyToTrainMilitia(pSoldier)) {
    // loyalty too low to continue training
    swprintf(sString, lengthof(sString), pMilitiaConfirmStrings[8],
             pTownNames[GetTownIdForSector(sector)], MIN_RATING_TO_TRAIN_TOWN);
    DoContinueMilitiaTrainingMessageBox(sSectorX, sSectorY, sString, MSG_BOX_FLAG_OK,
                                        CantTrainMilitiaOkBoxCallback);
    return;
  }

  if (IsAreaFullOfMilitia(sSectorX, sSectorY, pSoldier->bSectorZ)) {
    // we're full!!! go home!
    uint8_t const bTownId = GetTownIdForSector(sector);
    if (bTownId == BLANK_SECTOR) {
      // wilderness SAM site
      GetSectorIDString(sSectorX, sSectorY, 0, sStringB, lengthof(sStringB), TRUE);
      swprintf(sString, lengthof(sString), pMilitiaConfirmStrings[9], sStringB);
    } else {
      // town
      swprintf(sString, lengthof(sString), pMilitiaConfirmStrings[9], pTownNames[bTownId],
               MIN_RATING_TO_TRAIN_TOWN);
    }
    DoContinueMilitiaTrainingMessageBox(sSectorX, sSectorY, sString, MSG_BOX_FLAG_OK,
                                        CantTrainMilitiaOkBoxCallback);
    return;
  }

  // continue training always handles just one sector at a time
  giTotalCostOfTraining = MILITIA_TRAINING_COST;

  // can player afford to continue training?
  if (LaptopSaveInfo.iCurrentBalance < giTotalCostOfTraining) {
    // can't afford to continue training
    swprintf(sString, lengthof(sString), pMilitiaConfirmStrings[7], giTotalCostOfTraining);
    DoContinueMilitiaTrainingMessageBox(sSectorX, sSectorY, sString, MSG_BOX_FLAG_OK,
                                        CantTrainMilitiaOkBoxCallback);
    return;
  }

  // ok to continue, ask player

  GetSectorIDString(sSectorX, sSectorY, 0, sStringB, lengthof(sStringB), TRUE);
  swprintf(sString, lengthof(sString), pMilitiaConfirmStrings[3], sStringB,
           pMilitiaConfirmStrings[4], giTotalCostOfTraining);

  // ask player whether he'd like to continue training
  // DoContinueMilitiaTrainingMessageBox( sSectorX, sSectorY, sString,
  // MSG_BOX_FLAG_YESNO, PayMilitiaTrainingYesNoBoxCallback );
  DoMapMessageBox(MSG_BOX_BASIC_STYLE, sString, MAP_SCREEN, MSG_BOX_FLAG_YESNO,
                  PayMilitiaTrainingYesNoBoxCallback);
}

static void ContinueTrainingInThisSector();
static void MilitiaTrainingRejected();
static void StartTrainingInAllUnpaidTrainableSectors();

// IMPORTANT: This same callback is used both for initial training and for
// continue training prompt use 'gfYesNoPromptIsForContinue' flag to tell them
// apart
static void PayMilitiaTrainingYesNoBoxCallback(MessageBoxReturnValue const bExitValue) {
  Assert(giTotalCostOfTraining > 0);

  // yes
  if (bExitValue == MSG_BOX_RETURN_YES) {
    // does the player have enough
    if (LaptopSaveInfo.iCurrentBalance >= giTotalCostOfTraining) {
      if (gfYesNoPromptIsForContinue) {
        ContinueTrainingInThisSector();
      } else {
        StartTrainingInAllUnpaidTrainableSectors();
      }

      // this completes the training prompt sequence
      pMilitiaTrainerSoldier = NULL;
    } else  // can't afford it
    {
      StopTimeCompression();
      DoMapMessageBox(MSG_BOX_BASIC_STYLE, pMilitiaConfirmStrings[2], MAP_SCREEN, MSG_BOX_FLAG_OK,
                      CantTrainMilitiaOkBoxCallback);
    }
  } else if (bExitValue == MSG_BOX_RETURN_NO) {
    StopTimeCompression();

    MilitiaTrainingRejected();
  }
}

static void CantTrainMilitiaOkBoxCallback(MessageBoxReturnValue const bExitValue) {
  MilitiaTrainingRejected();
}

// IMPORTANT: This same callback is used both for initial training and for
// continue training prompt use 'gfYesNoPromptIsForContinue' flag to tell them
// apart
static void MilitiaTrainingRejected() {
  if (gfYesNoPromptIsForContinue) {
    // take all mercs in that sector off militia training
    ResetAssignmentOfMercsThatWereTrainingMilitiaInThisSector(pMilitiaTrainerSoldier->sSectorX,
                                                              pMilitiaTrainerSoldier->sSectorY);
  } else {
    // take all mercs in unpaid sectors EVERYWHERE off militia training
    ResetAssignmentsForMercsTrainingUnpaidSectorsInSelectedList();
  }

  // this completes the training prompt sequence
  pMilitiaTrainerSoldier = NULL;
}

void HandleMilitiaStatusInCurrentMapBeforeLoadingNewMap() {
  if (gTacticalStatus.Team[MILITIA_TEAM].bSide != 0) {
    // handle militia defections and reset team to friendly
    HandleMilitiaDefections(gWorldSectorX, gWorldSectorY);
    gTacticalStatus.Team[MILITIA_TEAM].bSide = 0;
  } else if (!gfAutomaticallyStartAutoResolve) {  // Don't promote militia if we
                                                  // are going directly to
                                                  // autoresolve to finish the
                                                  // current battle.
    HandleMilitiaPromotions();
  }
}

bool CanNearbyMilitiaScoutThisSector(int16_t const sec_x, int16_t const sec_y) {
  int16_t const scout_range = 1;
  int16_t const xstart = sec_x > scout_range ? sec_x - scout_range : 1;
  int16_t const ystart = sec_y > scout_range ? sec_y - scout_range : 1;
  int16_t const xend = sec_x < 16 - scout_range ? sec_x + scout_range : 16;
  int16_t const yend = sec_y < 16 - scout_range ? sec_y + scout_range : 16;
  for (int16_t y = ystart; y <= yend; ++y) {
    for (int16_t x = xstart; x <= xend; ++x) {
      uint8_t(&n_milita)[MAX_MILITIA_LEVELS] = SectorInfo[SECTOR(x, y)].ubNumberOfCivsAtLevel;
      if (n_milita[GREEN_MILITIA] + n_milita[REGULAR_MILITIA] + n_milita[ELITE_MILITIA] != 0)
        return true;
    }
  }
  return false;
}

BOOLEAN IsAreaFullOfMilitia(const int16_t sector_x, const int16_t sector_y, const int8_t sector_z) {
  if (sector_z != 0) return TRUE;

  uint32_t count_milita = 0;
  uint32_t max_milita = 0;

  int8_t const town_id = GetTownIdForSector(SECTOR(sector_x, sector_y));
  if (town_id != BLANK_SECTOR) {
    FOR_EACH_SECTOR_IN_TOWN(i, town_id) {
      int16_t const town_x = SECTORX(i->sector);
      int16_t const town_y = SECTORY(i->sector);
      if (SectorOursAndPeaceful(town_x, town_y, 0)) {
        // don't count GREEN militia, they can be trained into regulars first
        count_milita += MilitiaInSectorOfRank(town_x, town_y, REGULAR_MILITIA);
        count_milita += MilitiaInSectorOfRank(town_x, town_y, ELITE_MILITIA);
        max_milita += MAX_ALLOWABLE_MILITIA_PER_SECTOR;
      }
    }
  } else if (IsThisSectorASAMSector(sector_x, sector_y, 0)) {
    // don't count GREEN militia, they can be trained into regulars first
    count_milita += MilitiaInSectorOfRank(sector_x, sector_y, REGULAR_MILITIA);
    count_milita += MilitiaInSectorOfRank(sector_x, sector_y, ELITE_MILITIA);
    max_milita += MAX_ALLOWABLE_MILITIA_PER_SECTOR;
  }

  return count_milita >= max_milita;
}

// handle completion of assignment by this soldier too and inform the player
static void HandleCompletionOfTownTrainingByGroupWithTrainer(SOLDIERTYPE *pTrainer) {
  int16_t sSectorX = 0, sSectorY = 0;
  int8_t bSectorZ = 0;

  // get the sector values
  sSectorX = pTrainer->sSectorX;
  sSectorY = pTrainer->sSectorY;
  bSectorZ = pTrainer->bSectorZ;

  CFOR_EACH_IN_CHAR_LIST(c) {
    SOLDIERTYPE *const pSoldier = c->merc;
    if ((pSoldier->bAssignment == TRAIN_TOWN) && (pSoldier->sSectorX == sSectorX) &&
        (pSoldier->sSectorY == sSectorY) && (pSoldier->bSectorZ == bSectorZ)) {
      // done assignment
      AssignmentDone(pSoldier, FALSE, FALSE);
    }
  }
}

void AddSectorForSoldierToListOfSectorsThatCompletedMilitiaTraining(SOLDIERTYPE *pSoldier) {
  int32_t iCounter = 0;
  int16_t sSector = 0, sCurrentSector = 0;

  // get the sector value
  sSector = pSoldier->sSectorX + pSoldier->sSectorY * MAP_WORLD_X;

  while (g_list_of_merc_in_sectors_completed_militia_training[iCounter] != NULL) {
    // get the current soldier
    const SOLDIERTYPE *const pCurrentSoldier =
        g_list_of_merc_in_sectors_completed_militia_training[iCounter];

    // get the current sector value
    sCurrentSector = pCurrentSoldier->sSectorX + pCurrentSoldier->sSectorY * MAP_WORLD_X;

    // is the merc's sector already in the list?
    if (sCurrentSector == sSector) {
      // already here
      return;
    }

    iCounter++;

    Assert(iCounter < SIZE_OF_MILITIA_COMPLETED_TRAINING_LIST);
  }

  // add merc to the list
  g_list_of_merc_in_sectors_completed_militia_training[iCounter] = pSoldier;
}

// clear out the list of training sectors...should be done once the list is
// posted
void ClearSectorListForCompletedTrainingOfMilitia() {
  FOR_EACH(SOLDIERTYPE *, i, g_list_of_merc_in_sectors_completed_militia_training) *i = 0;
}

void HandleContinueOfTownTraining() {
  int32_t iCounter = 0;
  BOOLEAN fContinueEventPosted = FALSE;

  while (g_list_of_merc_in_sectors_completed_militia_training[iCounter] != NULL) {
    // get the soldier
    SOLDIERTYPE &s = *g_list_of_merc_in_sectors_completed_militia_training[iCounter];
    if (s.bActive) {
      fContinueEventPosted = TRUE;

      class DialogueEventContinueTrainingMilitia : public CharacterDialogueEvent {
       public:
        DialogueEventContinueTrainingMilitia(SOLDIERTYPE &soldier)
            : CharacterDialogueEvent(soldier) {}

        bool Execute() {
          HandleInterfaceMessageForContinuingTrainingMilitia(&soldier_);
          return false;
        }
      };

      DialogueEvent::Add(new DialogueEventContinueTrainingMilitia(s));
    }

    // next entry
    iCounter++;
  }

  // now clear the list
  ClearSectorListForCompletedTrainingOfMilitia();

  if (fContinueEventPosted) {
    // ATE: If this event happens in tactical mode we will be switching at some
    // time to mapscreen...
    if (guiCurrentScreen == GAME_SCREEN) {
      gfEnteringMapScreen = TRUE;
    }

    // If the militia view isn't currently active, then turn it on when
    // prompting to continue training.
    if (!fShowMilitia) {
      ToggleShowMilitiaMode();
    }
  }
}

static void AddIfTrainingUnpaidSector(SOLDIERTYPE const &s) {
  if (!CanCharacterTrainMilitia(&s)) return;
  // Check if this sector is a town and needs equipment.
  if (SectorInfo[SECTOR(s.sSectorX, s.sSectorY)].fMilitiaTrainingPaid) return;
  int16_t const sector = CALCULATE_STRATEGIC_INDEX(s.sSectorX, s.sSectorY);
  for (int16_t *i = gsUnpaidStrategicSector;; ++i) {
    if (*i == 0) {
      *i = sector;
      break;
    }
    if (*i == sector) break;  // Do not add duplicate
  }
}

static void BuildListOfUnpaidTrainableSectors() {
  memset(gsUnpaidStrategicSector, 0, sizeof(gsUnpaidStrategicSector));

  if (fInMapMode) {
    for (int32_t i = 0; i != MAX_CHARACTER_COUNT; ++i) {
      MapScreenCharacterSt const &c = gCharactersList[i];
      if (!c.merc) continue;
      if (!c.selected && i != bSelectedAssignChar) continue;
      AddIfTrainingUnpaidSector(*c.merc);
    }
  } else {  // Handle for tactical
    AddIfTrainingUnpaidSector(*gUIFullTarget);
  }
}

static int32_t GetNumberOfUnpaidTrainableSectors() {
  BuildListOfUnpaidTrainableSectors();
  int32_t n = 0;
  FOR_EACH(int16_t const, i, gsUnpaidStrategicSector) {
    if (*i != 0) ++n;
  }
  return n;
}

static void PayForTrainingInSector(uint8_t ubSector);

static void StartTrainingInAllUnpaidTrainableSectors() {
  SetAssignmentForList(TRAIN_TOWN, 0);
  // Pay up in each sector
  BuildListOfUnpaidTrainableSectors();
  FOR_EACH(int16_t const, i, gsUnpaidStrategicSector) {
    if (*i == 0) continue;
    PayForTrainingInSector(STRATEGIC_INDEX_TO_SECTOR_INFO(*i));
  }
}

static void ContinueTrainingInThisSector() {
  uint8_t ubSector;

  Assert(pMilitiaTrainerSoldier);

  // pay up in the sector where pMilitiaTrainerSoldier is
  ubSector = SECTOR(pMilitiaTrainerSoldier->sSectorX, pMilitiaTrainerSoldier->sSectorY);
  PayForTrainingInSector(ubSector);
}

static void ResetDoneFlagForAllMilitiaTrainersInSector(uint8_t ubSector);

static void PayForTrainingInSector(uint8_t ubSector) {
  Assert(!SectorInfo[ubSector].fMilitiaTrainingPaid);

  // spend the money
  AddTransactionToPlayersBook(TRAIN_TOWN_MILITIA, ubSector, GetWorldTotalMin(),
                              -(MILITIA_TRAINING_COST));

  // mark this sector sectors as being paid up
  SectorInfo[ubSector].fMilitiaTrainingPaid = TRUE;

  // reset done flags
  ResetDoneFlagForAllMilitiaTrainersInSector(ubSector);
}

static void ResetDoneFlagForAllMilitiaTrainersInSector(uint8_t ubSector) {
  FOR_EACH_IN_TEAM(pSoldier, OUR_TEAM) {
    if (pSoldier->bAssignment == TRAIN_TOWN &&
        SECTOR(pSoldier->sSectorX, pSoldier->sSectorY) == ubSector && pSoldier->bSectorZ == 0) {
      pSoldier->fDoneAssignmentAndNothingToDoFlag = FALSE;
      pSoldier->usQuoteSaidExtFlags &= ~SOLDIER_QUOTE_SAID_DONE_ASSIGNMENT;
    }
  }
}

BOOLEAN MilitiaTrainingAllowedInSector(int16_t sSectorX, int16_t sSectorY, int8_t bSectorZ) {
  BOOLEAN fSamSitePresent = FALSE;

  if (bSectorZ != 0) {
    return (FALSE);
  }

  fSamSitePresent = IsThisSectorASAMSector(sSectorX, sSectorY, bSectorZ);

  if (fSamSitePresent) {
    // all SAM sites may have militia trained at them
    return (TRUE);
  }

  uint8_t const bTownId = GetTownIdForSector(SECTOR(sSectorX, sSectorY));
  return (MilitiaTrainingAllowedInTown(bTownId));
}

BOOLEAN MilitiaTrainingAllowedInTown(int8_t bTownId) {
  switch (bTownId) {
    case DRASSEN:
    case ALMA:
    case GRUMM:
    case CAMBRIA:
    case BALIME:
    case MEDUNA:
    case CHITZENA:
      return (TRUE);

    case OMERTA:
    case ESTONI:
    case SAN_MONA:
    case TIXA:
    case ORTA:
      // can't keep militia in these towns
      return (FALSE);

    case BLANK_SECTOR:
    default:
      // not a town sector!
      return (FALSE);
  }
}

static size_t PromoteMilitia(wchar_t *const str, size_t const length, size_t n, int8_t const count,
                             wchar_t const *const singular, wchar_t const *const plural) {
  if (count > 0) {
    if (n != 0) n += swprintf(str + n, length - n, L" ");
    if (count == 1) {
      n += swprintf(str + n, length - n, singular);
    } else {
      n += swprintf(str + n, length - n, plural, count);
    }
  }
  return n;
}

void BuildMilitiaPromotionsString(wchar_t *const str, size_t const length) {
  str[0] = L'\0';

  if (gbMilitiaPromotions == 0) return;

  size_t n = 0;
  n = PromoteMilitia(str, length, n, gbGreenToElitePromotions, gzLateLocalizedString[STR_LATE_29],
                     gzLateLocalizedString[STR_LATE_22]);
  n = PromoteMilitia(str, length, n, gbGreenToRegPromotions, gzLateLocalizedString[STR_LATE_30],
                     gzLateLocalizedString[STR_LATE_23]);
  n = PromoteMilitia(str, length, n, gbRegToElitePromotions, gzLateLocalizedString[STR_LATE_31],
                     gzLateLocalizedString[STR_LATE_24]);

  // Clear the fields
  gbGreenToElitePromotions = 0;
  gbGreenToRegPromotions = 0;
  gbRegToElitePromotions = 0;
  gbMilitiaPromotions = 0;
}
