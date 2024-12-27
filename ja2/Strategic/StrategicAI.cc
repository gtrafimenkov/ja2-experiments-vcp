// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#include "Strategic/StrategicAI.h"

#include <algorithm>
#include <stdexcept>
#include <string.h>

#include "GameSettings.h"
#include "Macro.h"
#include "SGP/Debug.h"
#include "SGP/FileMan.h"
#include "SGP/Font.h"
#include "SGP/MemMan.h"
#include "SGP/Random.h"
#include "SGP/Types.h"
#include "Strategic/CampaignInit.h"
#include "Strategic/CampaignTypes.h"
#include "Strategic/GameClock.h"
#include "Strategic/GameEventHook.h"
#include "Strategic/GameInit.h"
#include "Strategic/MapScreen.h"
#include "Strategic/PlayerCommand.h"
#include "Strategic/PreBattleInterface.h"
#include "Strategic/QueenCommand.h"
#include "Strategic/Quests.h"
#include "Strategic/Scheduling.h"
#include "Strategic/StrategicMap.h"
#include "Strategic/StrategicMovement.h"
#include "Strategic/StrategicMovementCosts.h"
#include "Strategic/StrategicPathing.h"
#include "Strategic/TownMilitia.h"
#include "Tactical/Campaign.h"
#include "Tactical/InterfaceDialogue.h"
#include "Tactical/MapInformation.h"
#include "Tactical/Overhead.h"
#include "Tactical/SoldierProfile.h"
#include "Tactical/SoldierTile.h"
#include "TileEngine/ExplosionControl.h"
#include "Utils/FontControl.h"
#include "Utils/Message.h"

#define SAI_VERSION 29

/*
STRATEGIC AI -- UNDERLYING PHILOSOPHY
The most fundamental part of the strategic AI which takes from reality and gives
to gameplay is the manner the queen attempts to take her towns back.  Finances
and owning mines are the most important way to win the game.  As the player
takes more mines over, the queen will focus more on quality and defense.  In the
beginning of the game, she will focus more on offense than mid-game or end-game.

REALITY
The queen owns the entire country, and the player starts the game with a small
lump of cash, enough to hire some mercenaries for about a week.  In that week,
the queen may not notice what is going on, and the player would believably take
over one of the towns before she could feasibly react.  As soon as her military
was aware of the situation, she would likely proceed to send 300-400 troops to
annihilate the opposition, and the game would be over relatively quickly.  If
the player was a prodigy, and managed to hold the town against such a major
assault, he would probably lose in the long run being forced into a defensive
position and running out of money quickly while the queen could continue to pump
out the troops.  On the other hand, if the player somehow managed to take over
most of the mines, he would be able to casually walk over the queen eventually
just from the sheer income allowing him to purchase several of the best mercs.
That would have the effect of making the game impossibly difficult in the
beginning of the game, and a joke at the end (this is very much like Master Of
Orion II on the more difficult settings )

GAMEPLAY
Because we want the game to be like a normal game and make it fun, we need to
make the game easy in the beginning and harder at the end.  In order to
accomplish this, I feel that pure income shouldn't be the factor for the queen,
because she would likely crucify a would-be leader in his early days.  So, in
the beginning of the game, the forces would already be situated with the
majority of forces being the administrators in the towns, and army troops and
elites in the more important sectors.  Restricting the queen's offensive
abilities using a distance penalty would mean that the furthest sectors from the
queen's palace would be much easier to defend because she would only be allowed
to send x number of troops.  As you get closer to the queen, she would be
allowed to send larger forces to attack those towns in question.  Also, to
further increase the games difficulty as the campaign progresses in the player's
favor, we could also increase the quality of the queen's troops based purely on
the peek progress percentage.  This is calculated using a formula that
determines how well the player is doing by combining loyalty of towns owned,
income generated, etc.  So, in the beginning of the game, the quality is at the
worst, but once you capture your first mines/towns, it permanently  increase the
queen's quality rating, effectively bumping up the stakes.  By the time you
capture four or five mines, the queen is going to focus more (but not
completely) on quality defense as she prepares for your final onslaught.  This
quality rating will augment the experience level, equipment rating, and/or
attribute ratings of the queen's troops.  I would maintain a table of these
enhancements based on the current quality rating hooking into the difficulty all
along.

//EXPLANATION OF THE WEIGHT SYSTEM:
The strategic AI has two types of groups:  garrisons and patrol groups.  Each of
these groups contain information of it's needs, mainly desired population.  If
the current population is greater than the desired population, and the group
will get a negative weight assigned to it, which means that it is willing to
give up troops to areas that need them more.  On the other hand, if a group has
less than the desired population, then the weight will be positive, meaning they
are requesting reinforcements.

The weight generated will range between -100 and +100.  The calculated weight is
modified by the priority of the group.  If the priority of the group is high,
they
*/

// Modifies the number of troops the queen has at the beginning of the game on
// top of all of the garrison and patrol groups.  Additionally, there are a total
// of 16 sectors that are LEVEL 1, 2, or 3 garrison groups.  The lower the level,
// the more troops stay in that sector, and the rest will also be used as a
// secondary pool when the primary pool runs dry.  So basically, this number is
// only part of the equation.
#define EASY_QUEENS_POOL_OF_TROOPS 150
#define NORMAL_QUEENS_POOL_OF_TROOPS 200
#define HARD_QUEENS_POOL_OF_TROOPS 400

// Modifies the starting values as well as the desired values for all of the
// garrisons.
#define EASY_INITIAL_GARRISON_PERCENTAGES 70
#define NORMAL_INITIAL_GARRISON_PERCENTAGES 100
#define HARD_INITIAL_GARRISON_PERCENTAGES 125

#define EASY_MIN_ENEMY_GROUP_SIZE 3
#define NORMAL_MIN_ENEMY_GROUP_SIZE 4
#define HARD_MIN_ENEMY_GROUP_SIZE 6

// Sets the starting alert chances.  Everytime an enemy arrives in a new sector,
// or the player, this is the chance the enemy will detect the player in adjacent
// sectors.  This chance is associated with each side checked.  Stationary groups
// do this check periodically.
#define EASY_ENEMY_STARTING_ALERT_LEVEL 5
#define NORMAL_ENEMY_STARTING_ALERT_LEVEL 20
#define HARD_ENEMY_STARTING_ALERT_LEVEL 60

// When an enemy spots and chases a player group, the alertness value decrements
// by this value.  The higher the value, the less of a chance the enemy will spot
// and attack subsequent groups.  This minimizes the aggressiveness of the enemy.
// Ranges from 1-100 (but recommend 20-60).
#define EASY_ENEMY_STARTING_ALERT_DECAY 75
#define NORMAL_ENEMY_STARTING_ALERT_DECAY 50
#define HARD_ENEMY_STARTING_ALERT_DECAY 25
// The base time that the queen can think about reinforcements for refilling
// lost patrol groups, town garrisons, etc. She only is allowed one action per
// 'turn'.
#define EASY_TIME_EVALUATE_IN_MINUTES 480
#define NORMAL_TIME_EVALUATE_IN_MINUTES 360
#define HARD_TIME_EVALUATE_IN_MINUTES 180
// The variance added on.
#define EASY_TIME_EVALUATE_VARIANCE 240
#define NORMAL_TIME_EVALUATE_VARIANCE 180
#define HARD_TIME_EVALUATE_VARIANCE 120

// When a player takes control of a sector, don't allow any enemy reinforcements
// to enter the sector for a limited amount of time.  This essentially dumbs down
// the AI, making it less aggressive.
#define EASY_GRACE_PERIOD_IN_HOURS 144   // 6 days
#define NORMAL_GRACE_PERIOD_IN_HOURS 96  // 4 days
#define HARD_GRACE_PERIOD_IN_HOURS 48    // 2 days

// Defines how many days must pass before the queen is willing to refill a
// defeated patrol group.
#define EASY_PATROL_GRACE_PERIOD_IN_DAYS 16
#define NORMAL_PATROL_GRACE_PERIOD_IN_DAYS 12
#define HARD_PATROL_GRACE_PERIOD_IN_DAYS 8

// Certain conditions can cause the queen to go into a "full alert" mode.  This
// means that temporarily, the queen's forces will automatically succeed adjacent
// checks until x number of enemy initiated battles occur.  The same variable is
// what is used to determine the free checks.
#define EASY_NUM_AWARE_BATTLES 1
#define NORMAL_NUM_AWARE_BATTLES 2
#define HARD_NUM_AWARE_BATTLES 3

BOOLEAN gfAutoAIAware = FALSE;

// Saved vars
BOOLEAN gfExtraElites = 0;  // Set when queen compositions are augmented with bonus elites.
static int32_t giGarrisonArraySize = 0;
int32_t giPatrolArraySize = 0;
int32_t giForcePercentage = 0;  // Modifies the starting group sizes relative by percentage
int32_t giArmyAlertness = 0;    // The chance the group will spot an adjacent player/militia
int32_t giArmyAlertnessDecay =
    0;                           // How much the spotting chance decreases when spot check succeeds
uint8_t gubNumAwareBattles = 0;  // When non-zero, this means the queen is very aware and searching
                                 // for players.  Every time there is an enemy initiated battle,
                                 // this counter decrements until zero.  Until that point, all
                                 // adjacent sector checks automatically succeed.
BOOLEAN gfQueenAIAwake = FALSE;  // This flag turns on/off the strategic decisions.  If it's off, no
                                 // reinforcements or assaults will happen.
                                 //@@@Alex, this flag is ONLY set by the first meanwhile scene which
                                 // calls an action.  If this action isn't called, the AI will never
                                 // turn on.  It is completely dependant on this action.  It can be
                                 // toggled at will in the AIViewer for testing purposes.
int32_t giReinforcementPool = 0;  // How many troops the queen has in reserve in noman's land. These
                                  // guys are spawned as needed in P3.
int32_t giReinforcementPoints = 0;    // the entire army's capacity to provide reinforcements.
int32_t giRequestPoints = 0;          // the entire army's need for reinforcements.
uint8_t gubSAIVersion = SAI_VERSION;  // Used for adding new features to be saved.
uint8_t gubQueenPriorityPhase =
    0;  // Defines how far into defence the queen is -- abstractly related
        // to defcon index ranging from 0-10. 10 is the most defensive
// Used for authorizing the use of the first battle meanwhile scene AFTER the
// battle is complete.  This is the case used when the player attacks a town, and
// is set once militia are sent to investigate.
BOOLEAN gfFirstBattleMeanwhileScenePending = FALSE;

// After the first battle meanwhile scene is finished, this flag is set, and the
// queen orders patrol groups to immediately fortify all towns.
BOOLEAN gfMassFortificationOrdered = FALSE;

uint8_t gubMinEnemyGroupSize = 0;
uint8_t gubHoursGracePeriod = 0;
uint16_t gusPlayerBattleVictories = 0;
BOOLEAN gfUseAlternateQueenPosition = FALSE;

// padding for generic globals
#define SAI_PADDING_BYTES 97
int8_t gbPadding[SAI_PADDING_BYTES];  // XXX HACK000B
// patrol group info plus padding
#define SAVED_PATROL_GROUPS 50
static PATROL_GROUP *gPatrolGroup;
// army composition info plus padding
#define SAVED_ARMY_COMPOSITIONS 60
ARMY_COMPOSITION gArmyComp[NUM_ARMY_COMPOSITIONS];
// garrison info plus padding
#define SAVED_GARRISON_GROUPS 100
GARRISON_GROUP *gGarrisonGroup = NULL;

extern uint8_t gubNumGroupsArrivedSimultaneously;

// This refers to the number of force points that are *saved* for the AI to use.
// This is basically an array of each group.  When the queen wants to send forces
// to attack a town that is defended, the initial number of forces that she would
// send would be considered too weak.  So, instead, she will send that force to
// the sector's adjacent sector, and stage, while
uint8_t *gubGarrisonReinforcementsDenied = NULL;
uint8_t *gubPatrolReinforcementsDenied = NULL;

// Unsaved vars
BOOLEAN gfDisplayStrategicAILogs = FALSE;

// The army composition defines attributes for the various garrisons.  The
// priority reflects how important the sector is to the queen, the elite/troop
// percentages refer to the desired composition of the group.  The admin
// percentage has recently been changed to reflect the starting percentage of the
// garrison that are administrators.  Note that elite% + troop% = 100, and the
// admin% is not related in this effect.  If the admin% is non-zero, then that
// garrison is assigned only x% of the force as admins, with NO troops or elites.
// All reinforcements use the composition of the troop/elite for refilling.
//@@@Alex, the send reinforcement composition isn't complete.  Either sends all
// troops or troops based off of the composition of the source garrison.
//  It is my intention to add this.

// If you change the MAX_STRATEGIC_TEAM_SIZE, then all the garrison sizes
// (start, desired) will have to be changed accordingly.

#define M(composition, prio, elite, troop, admin, desired, start)                            \
  {                                                                                          \
    composition, prio, elite, troop, admin, desired, start, { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } \
  }

static const ARMY_COMPOSITION gOrigArmyComp[NUM_ARMY_COMPOSITIONS] =
    {  // COMPOSITION          PRIO ELITE% TROOP% ADMIN DESIRED# START#
       //                                              START%
        M(QUEEN_DEFENCE, 100, 100, 0, 0, 32, 32),
        M(MEDUNA_DEFENCE, 95, 55, 45, 0, 16, 20),
        M(MEDUNA_SAMSITE, 96, 65, 35, 0, 20, 20),
        M(LEVEL1_DEFENCE, 40, 20, 80, 0, 12, 20),
        M(LEVEL2_DEFENCE, 30, 10, 90, 0, 10, 20),
        M(LEVEL3_DEFENCE, 20, 5, 95, 0, 8, 20),
        M(ORTA_DEFENCE, 90, 50, 50, 0, 18, 19),
        M(EAST_GRUMM_DEFENCE, 80, 20, 80, 0, 15, 15),
        M(WEST_GRUMM_DEFENCE, 70, 0, 100, 40, 15, 15),
        M(GRUMM_MINE, 85, 25, 75, 45, 15, 15),
        M(OMERTA_WELCOME_WAGON, 0, 0, 100, 0, 0, 3),
        M(BALIME_DEFENCE, 60, 45, 55, 20, 10, 4),
        M(TIXA_PRISON, 80, 10, 90, 15, 15, 15),
        M(TIXA_SAMSITE, 85, 10, 90, 0, 12, 12),
        M(ALMA_DEFENCE, 74, 15, 85, 0, 11, 20),
        M(ALMA_MINE, 80, 20, 80, 45, 15, 20),
        M(CAMBRIA_DEFENCE, 50, 0, 100, 30, 10, 6),
        M(CAMBRIA_MINE, 60, 15, 85, 40, 11, 6),
        M(CHITZENA_DEFENCE, 30, 0, 100, 75, 12, 10),
        M(CHITZENA_MINE, 40, 0, 100, 75, 10, 10),
        M(CHITZENA_SAMSITE, 75, 10, 90, 0, 9, 9),
        M(DRASSEN_AIRPORT, 30, 0, 100, 85, 12, 10),
        M(DRASSEN_DEFENCE, 20, 0, 100, 80, 10, 8),
        M(DRASSEN_MINE, 35, 0, 100, 75, 11, 9),
        M(DRASSEN_SAMSITE, 50, 0, 100, 0, 10, 10),
        M(ROADBLOCK, 20, 2, 98, 0, 8, 0),
        M(SANMONA_SMALL, 0, 0, 0, 0, 0, 0)};

#undef M

#define M(size, prio, p1, p2, p3, p4)                                           \
  {                                                                             \
    size, prio, {p1, p2, p3, p4}, -1, 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } \
  }

// Patrol definitions
// NOTE:	  A point containing 0 is actually the same as SEC_A1, but because
// nobody is using SEC_A1 in any 				of the patrol groups, I am coding 0
// to be ignored. NOTE:		Must have at least two points.
static const PATROL_GROUP gOrigPatrolGroup[] = {
    // SIZE PRIO POINT1    POINT2    POINT3    POINT4
    M(8, 40, SEC_B1, SEC_C1, SEC_C3, SEC_A3), M(6, 35, SEC_B4, SEC_B7, SEC_C7, 0),
    M(6, 25, SEC_A8, SEC_B8, SEC_B9, 0), M(6, 30, SEC_B10, SEC_B12, 0, 0),
    M(7, 45, SEC_A11, SEC_A14, SEC_D14, 0),
    // 5
    M(6, 50, SEC_C8, SEC_C9, SEC_D9, 0), M(12, 55, SEC_D3, SEC_G3, 0, 0),
    M(10, 50, SEC_D6, SEC_D7, SEC_F7, 0), M(10, 55, SEC_E8, SEC_E11, SEC_F11, 0),
    M(10, 60, SEC_E12, SEC_E15, 0, 0),
    // 10
    M(12, 60, SEC_G4, SEC_G7, 0, 0), M(12, 65, SEC_G10, SEC_G12, SEC_F12, 0),
    M(12, 65, SEC_G13, SEC_G15, 0, 0), M(10, 65, SEC_H15, SEC_J15, 0, 0),
    M(14, 65, SEC_H12, SEC_J12, SEC_J13, 0),
    // 15
    M(13, 70, SEC_H9, SEC_I9, SEC_I10, SEC_J10), M(11, 70, SEC_K11, SEC_K14, SEC_J14, 0),
    M(12, 75, SEC_J2, SEC_K2, 0, 0), M(12, 80, SEC_I3, SEC_J3, 0, 0),
    M(12, 80, SEC_J6, SEC_K6, 0, 0),
    // 20
    M(13, 85, SEC_K7, SEC_K10, 0, 0), M(12, 90, SEC_L10, SEC_M10, 0, 0),
    M(12, 90, SEC_N9, SEC_N10, 0, 0), M(12, 80, SEC_L7, SEC_L8, SEC_M8, SEC_M9),
    M(14, 80, SEC_H4, SEC_H5, SEC_I5, 0),
    // 25
    M(7, 40, SEC_D4, SEC_E4, SEC_E5, 0), M(7, 50, SEC_C10, SEC_C11, SEC_D11, SEC_D12),
    M(8, 40, SEC_A15, SEC_C15, SEC_C16, 0), M(12, 30, SEC_L13, SEC_M13, SEC_M14, SEC_L14)
    // 29
};

#undef M

#define PATROL_GROUPS 29

#define M(sector, composition)                                  \
  {                                                             \
    sector, composition, 0, 0, { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } \
  }

static const GARRISON_GROUP gOrigGarrisonGroup[] = {
    // SECTOR   MILITARY COMPOSITION
    M(SEC_P3, QUEEN_DEFENCE), M(SEC_O3, MEDUNA_DEFENCE), M(SEC_O4, MEDUNA_DEFENCE),
    M(SEC_N3, MEDUNA_DEFENCE), M(SEC_N4, MEDUNA_SAMSITE),
    // 5
    M(SEC_N5, MEDUNA_DEFENCE), M(SEC_M3, LEVEL1_DEFENCE), M(SEC_M4, LEVEL1_DEFENCE),
    M(SEC_M5, LEVEL1_DEFENCE), M(SEC_N6, LEVEL1_DEFENCE),
    // 10
    M(SEC_M2, LEVEL2_DEFENCE), M(SEC_L3, LEVEL2_DEFENCE), M(SEC_L4, LEVEL2_DEFENCE),
    M(SEC_L5, LEVEL2_DEFENCE), M(SEC_M6, LEVEL2_DEFENCE),
    // 15
    M(SEC_N7, LEVEL1_DEFENCE), M(SEC_L2, LEVEL3_DEFENCE), M(SEC_K3, LEVEL3_DEFENCE),
    M(SEC_K5, LEVEL3_DEFENCE), M(SEC_L6, LEVEL3_DEFENCE),
    // 20
    M(SEC_M7, LEVEL3_DEFENCE), M(SEC_N8, LEVEL3_DEFENCE), M(SEC_K4, ORTA_DEFENCE),
    M(SEC_G1, WEST_GRUMM_DEFENCE), M(SEC_G2, EAST_GRUMM_DEFENCE),
    // 25
    M(SEC_H1, WEST_GRUMM_DEFENCE), M(SEC_H2, EAST_GRUMM_DEFENCE), M(SEC_H3, GRUMM_MINE),
    M(SEC_A9, OMERTA_WELCOME_WAGON), M(SEC_L11, BALIME_DEFENCE),
    // 30
    M(SEC_L12, BALIME_DEFENCE), M(SEC_J9, TIXA_PRISON), M(SEC_I8, TIXA_SAMSITE),
    M(SEC_H13, ALMA_DEFENCE), M(SEC_H14, ALMA_DEFENCE),
    // 35
    M(SEC_I13, ALMA_DEFENCE), M(SEC_I14, ALMA_MINE), M(SEC_F8, CAMBRIA_DEFENCE),
    M(SEC_F9, CAMBRIA_DEFENCE), M(SEC_G8, CAMBRIA_DEFENCE),
    // 40
    M(SEC_G9, CAMBRIA_DEFENCE), M(SEC_H8, CAMBRIA_MINE), M(SEC_A2, CHITZENA_DEFENCE),
    M(SEC_B2, CHITZENA_MINE), M(SEC_D2, CHITZENA_SAMSITE),
    // 45
    M(SEC_B13, DRASSEN_AIRPORT), M(SEC_C13, DRASSEN_DEFENCE), M(SEC_D13, DRASSEN_MINE),
    M(SEC_D15, DRASSEN_SAMSITE), M(SEC_G12, ROADBLOCK),
    // 50
    M(SEC_M10, ROADBLOCK), M(SEC_G6, ROADBLOCK), M(SEC_C9, ROADBLOCK), M(SEC_K10, ROADBLOCK),
    M(SEC_G7, ROADBLOCK),
    // 55
    M(SEC_G3, ROADBLOCK), M(SEC_C5, SANMONA_SMALL)
    // 57
};

#undef M

extern int16_t sWorldSectorLocationOfFirstBattle;

#define SAIReportError(a)  // define it out

// returns the number of reinforcements permitted to be sent.  Will increased if
// the denied counter is non-zero.
static int32_t GarrisonReinforcementsRequested(int32_t iGarrisonID,
                                               uint8_t *pubExtraReinforcements) {
  int32_t iReinforcementsRequested;
  int32_t iExistingForces;
  SECTORINFO *pSector;

  pSector = &SectorInfo[gGarrisonGroup[iGarrisonID].ubSectorID];
  iExistingForces = pSector->ubNumAdmins + pSector->ubNumTroops + pSector->ubNumElites;
  iReinforcementsRequested =
      gArmyComp[gGarrisonGroup[iGarrisonID].ubComposition].bDesiredPopulation - iExistingForces;

  // Record how many of the reinforcements are additionally provided due to
  // being denied in the past.  This will grow until it is finally excepted or an
  // absolute max is made.
  *pubExtraReinforcements = (uint8_t)(gubGarrisonReinforcementsDenied[iGarrisonID] /
                                      (6 - gGameOptions.ubDifficultyLevel));
  // Make sure the number of extra reinforcements don't bump the force size past
  // the max of MAX_STRATEGIC_TEAM_SIZE.
  *pubExtraReinforcements =
      (uint8_t)std::min((int32_t)*pubExtraReinforcements,
                        std::min((int32_t)(*pubExtraReinforcements),
                                 MAX_STRATEGIC_TEAM_SIZE - iReinforcementsRequested));

  iReinforcementsRequested = std::min(MAX_STRATEGIC_TEAM_SIZE, iReinforcementsRequested);

  return iReinforcementsRequested;
}

static int32_t PatrolReinforcementsRequested(PATROL_GROUP const *const pg) {
  GROUP *const g = GetGroup(pg->ubGroupID);
  int32_t size = pg->bSize;
  if (g) size -= g->ubGroupSize;
  return size;
}

static int32_t ReinforcementsAvailable(int32_t iGarrisonID) {
  SECTORINFO *pSector;
  int32_t iReinforcementsAvailable;

  pSector = &SectorInfo[gGarrisonGroup[iGarrisonID].ubSectorID];
  iReinforcementsAvailable = pSector->ubNumTroops + pSector->ubNumElites + pSector->ubNumAdmins;
  iReinforcementsAvailable -=
      gArmyComp[gGarrisonGroup[iGarrisonID].ubComposition].bDesiredPopulation;

  switch (gGarrisonGroup[iGarrisonID].ubComposition) {
    case LEVEL1_DEFENCE:
    case LEVEL2_DEFENCE:
    case LEVEL3_DEFENCE:
    case ALMA_DEFENCE:
    case ALMA_MINE:
      // Legal spawning locations
      break;
    default:
      // No other sector permitted to send surplus troops
      return 0;
  }

  return iReinforcementsAvailable;
}

static BOOLEAN PlayerForceTooStrong(uint8_t ubSectorID, uint16_t usOffensePoints,
                                    uint16_t *pusDefencePoints) {
  SECTORINFO *pSector;
  uint8_t ubSectorX, ubSectorY;

  ubSectorX = (uint8_t)SECTORX(ubSectorID);
  ubSectorY = (uint8_t)SECTORY(ubSectorID);
  pSector = &SectorInfo[ubSectorID];

  *pusDefencePoints = pSector->ubNumberOfCivsAtLevel[GREEN_MILITIA] * 1 +
                      pSector->ubNumberOfCivsAtLevel[REGULAR_MILITIA] * 2 +
                      pSector->ubNumberOfCivsAtLevel[ELITE_MILITIA] * 3 +
                      PlayerMercsInSector(ubSectorX, ubSectorY, 0) * 5;
  if (*pusDefencePoints > usOffensePoints) {
    return TRUE;
  }
  return FALSE;
}

static void SendReinforcementsForGarrison(int32_t iDstGarrisonID, uint16_t usDefencePoints,
                                          GROUP **pOptionalGroup);

static void RequestAttackOnSector(uint8_t ubSectorID, uint16_t usDefencePoints) {
  int32_t i;
  for (i = 0; i < giGarrisonArraySize; i++) {
    if (gGarrisonGroup[i].ubSectorID == ubSectorID && !gGarrisonGroup[i].ubPendingGroupID) {
      SendReinforcementsForGarrison(i, usDefencePoints, NULL);
      return;
    }
  }
}

static bool AdjacentSectorIsImportantAndUndefended(uint8_t const sector_id) {
  switch (StrategicMap[SECTOR_INFO_TO_STRATEGIC_INDEX(sector_id)].bNameId) {
    case OMERTA:
    case SAN_MONA:
    case ESTONI:
      // These towns aren't important.
      return false;
  }
  SECTORINFO const &si = SectorInfo[sector_id];
  return si.ubNumTroops == 0 && si.ubNumElites == 0 && si.ubNumAdmins == 0 &&
         si.ubTraversability[THROUGH_STRATEGIC_MOVE] == TOWN && !PlayerSectorDefended(sector_id);
}

static void ValidatePendingGroups() {}

static void ValidateWeights(int32_t iID) {}

static void ReassignAIGroup(GROUP **pGroup);

static void ValidateGroup(GROUP *pGroup) {
  if (!pGroup->ubSectorX || !pGroup->ubSectorY || pGroup->ubSectorX > 16 ||
      pGroup->ubSectorY > 16) {
    if (gTacticalStatus.uiFlags & LOADING_SAVED_GAME) {
      RemoveGroupFromStrategicAILists(*pGroup);
      RemoveGroup(*pGroup);
      return;
    }
  }
  if (!pGroup->ubNextX || !pGroup->ubNextY) {
    if (!pGroup->fPlayer && pGroup->pEnemyGroup->ubIntention != STAGING &&
        pGroup->pEnemyGroup->ubIntention != REINFORCEMENTS) {
      if (gTacticalStatus.uiFlags & LOADING_SAVED_GAME) {
        RemoveGroupFromStrategicAILists(*pGroup);
        ReassignAIGroup(&pGroup);
        return;
      }
    }
  }
}

static void ValidateLargeGroup(GROUP *pGroup) {}

void InitStrategicAI() {
  gfExtraElites = FALSE;
  gubNumAwareBattles = 0;
  gfQueenAIAwake = FALSE;
  giReinforcementPoints = 0;
  giRequestPoints = 0;
  gubSAIVersion = SAI_VERSION;
  gubQueenPriorityPhase = 0;
  gfFirstBattleMeanwhileScenePending = FALSE;
  gfMassFortificationOrdered = FALSE;
  gusPlayerBattleVictories = 0;
  gfUseAlternateQueenPosition = FALSE;

  // 475 is 7:55am in minutes since midnight, the time the game starts on day 1
  uint32_t evaluate_time = 475;
  uint8_t const difficulty = gGameOptions.ubDifficultyLevel;
  switch (difficulty) {
    case DIF_LEVEL_EASY:
      giReinforcementPool = EASY_QUEENS_POOL_OF_TROOPS;
      giForcePercentage = EASY_INITIAL_GARRISON_PERCENTAGES;
      giArmyAlertness = EASY_ENEMY_STARTING_ALERT_LEVEL;
      giArmyAlertnessDecay = EASY_ENEMY_STARTING_ALERT_DECAY;
      gubMinEnemyGroupSize = EASY_MIN_ENEMY_GROUP_SIZE;
      gubHoursGracePeriod = EASY_GRACE_PERIOD_IN_HOURS;
      evaluate_time += EASY_TIME_EVALUATE_IN_MINUTES + Random(EASY_TIME_EVALUATE_VARIANCE);
      break;

    case DIF_LEVEL_MEDIUM:
      giReinforcementPool = NORMAL_QUEENS_POOL_OF_TROOPS;
      giForcePercentage = NORMAL_INITIAL_GARRISON_PERCENTAGES;
      giArmyAlertness = NORMAL_ENEMY_STARTING_ALERT_LEVEL;
      giArmyAlertnessDecay = NORMAL_ENEMY_STARTING_ALERT_DECAY;
      gubMinEnemyGroupSize = NORMAL_MIN_ENEMY_GROUP_SIZE;
      gubHoursGracePeriod = NORMAL_GRACE_PERIOD_IN_HOURS;
      evaluate_time += NORMAL_TIME_EVALUATE_IN_MINUTES + Random(NORMAL_TIME_EVALUATE_VARIANCE);
      break;

    case DIF_LEVEL_HARD:
      giReinforcementPool = HARD_QUEENS_POOL_OF_TROOPS;
      giForcePercentage = HARD_INITIAL_GARRISON_PERCENTAGES;
      giArmyAlertness = HARD_ENEMY_STARTING_ALERT_LEVEL;
      giArmyAlertnessDecay = HARD_ENEMY_STARTING_ALERT_DECAY;
      gubMinEnemyGroupSize = HARD_MIN_ENEMY_GROUP_SIZE;
      gubHoursGracePeriod = HARD_GRACE_PERIOD_IN_HOURS;
      evaluate_time += HARD_TIME_EVALUATE_IN_MINUTES + Random(HARD_TIME_EVALUATE_VARIANCE);
      break;

    default:
      throw std::logic_error("invalid difficulty level");
  }
  AddStrategicEvent(EVENT_EVALUATE_QUEEN_SITUATION, evaluate_time, 0);

  // Initialize the sectorinfo structure so all sectors don't point to a
  // garrisonID.
  FOR_EACH(SECTORINFO, i, SectorInfo) { i->ubGarrisonID = NO_GARRISON; }

  /* Copy over the original army composition as it does get modified during the
   * campaign. This bulletproofs starting the game over again. */
  memcpy(gArmyComp, gOrigArmyComp, sizeof(gArmyComp));

  // Eliminate more perimeter defenses on the easier levels.
  switch (difficulty) {
    case DIF_LEVEL_EASY:
      gArmyComp[LEVEL2_DEFENCE].bDesiredPopulation = 0;
      gArmyComp[LEVEL2_DEFENCE].bStartPopulation = 0;
      /* FALLTHROUGH */
    case DIF_LEVEL_MEDIUM:
      gArmyComp[LEVEL3_DEFENCE].bDesiredPopulation = 0;
      gArmyComp[LEVEL3_DEFENCE].bStartPopulation = 0;
      break;
  }

  // Initialize the patrol group definitions
  giPatrolArraySize = lengthof(gOrigPatrolGroup);
  if (!gPatrolGroup) {  // Allocate it (otherwise, we just overwrite it because
                        // the size never changes)
    gPatrolGroup = MALLOCN(PATROL_GROUP, lengthof(gOrigPatrolGroup));
  }
  memcpy(gPatrolGroup, gOrigPatrolGroup, sizeof(gOrigPatrolGroup));
  gubPatrolReinforcementsDenied = MALLOCNZ(uint8_t, giPatrolArraySize);

  // Initialize the garrison group definitions
  giGarrisonArraySize = lengthof(gOrigGarrisonGroup);
  if (!gGarrisonGroup) {
    gGarrisonGroup = MALLOCN(GARRISON_GROUP, lengthof(gOrigGarrisonGroup));
  }
  memcpy(gGarrisonGroup, gOrigGarrisonGroup, sizeof(gOrigGarrisonGroup));
  gubGarrisonReinforcementsDenied = MALLOCNZ(uint8_t, giGarrisonArraySize);

  // Modify initial force sizes?
  int32_t const force_percentage = giForcePercentage;
  if (force_percentage != 100) { /* The initial force sizes are being modified, so go through each
                                  * of the army compositions and adjust them accordingly. */
    for (int32_t i = 0; i != NUM_ARMY_COMPOSITIONS; ++i) {
      ARMY_COMPOSITION &a = gArmyComp[i];
      if (i != QUEEN_DEFENCE) {
        a.bDesiredPopulation =
            std::min(MAX_STRATEGIC_TEAM_SIZE, a.bDesiredPopulation * force_percentage / 100);
        if (a.bStartPopulation != MAX_STRATEGIC_TEAM_SIZE) { /* If the value is
                                                              * MAX_STRATEGIC_TEAM_SIZE, then that
                                                              * means the particular sector is a
                                                              * spawning location. Don't modify the
                                                              * value if it is
                                                              * MAX_STRATEGIC_TEAM_SIZE. Everything
                                                              * else is game. */
          a.bStartPopulation =
              std::min(MAX_STRATEGIC_TEAM_SIZE, a.bStartPopulation * force_percentage / 100);
        }
      } else {
        a.bDesiredPopulation = std::min(32, a.bDesiredPopulation * force_percentage / 100);
        a.bStartPopulation = a.bDesiredPopulation;
      }
    }
    for (int32_t i = 0; i != giPatrolArraySize;
         ++i) {  // Force modified range within 1 - MAX_STRATEGIC_TEAM_SIZE.
      int8_t &size = gPatrolGroup[i].bSize;
      size = std::max((int8_t)gubMinEnemyGroupSize,
                      (int8_t)std::min(MAX_STRATEGIC_TEAM_SIZE, size * force_percentage / 100));
    }
  }

  /* Initialize the garrisons based on the initial sizes (all variances are plus
   * or minus 1). */
  for (int32_t i = 0; i != giGarrisonArraySize; ++i) {
    GARRISON_GROUP &gg = gGarrisonGroup[i];
    SECTORINFO &si = SectorInfo[gg.ubSectorID];
    si.ubGarrisonID = i;
    ARMY_COMPOSITION const &ac = gArmyComp[gg.ubComposition];
    int32_t start_pop = ac.bStartPopulation;
    int32_t const desired_pop = ac.bDesiredPopulation;
    int32_t const iPriority = ac.bPriority;
    int32_t const elite_chance = ac.bElitePercentage;
    int32_t const troop_chance = ac.bTroopPercentage + elite_chance;
    int32_t const admin_chance = ac.bAdminPercentage;

    switch (gg.ubComposition) {
      case ROADBLOCK:
        si.uiFlags |= SF_ENEMY_AMBUSH_LOCATION;
        start_pop = Chance(20) ? ac.bDesiredPopulation : 0;
        break;

      case SANMONA_SMALL:
        start_pop = 0;  // Not appropriate until Kingpin is killed.
        break;
    }

    if (start_pop != 0) {
      if (gg.ubSectorID != SEC_P3) {
        // if population is less than maximum
        if (start_pop != MAX_STRATEGIC_TEAM_SIZE) {
          // then vary it a bit (+/- 25%)
          start_pop = start_pop * (100 + Random(51) - 25) / 100;
        }

        start_pop =
            std::max((int32_t)gubMinEnemyGroupSize, std::min(MAX_STRATEGIC_TEAM_SIZE, start_pop));
      }

      if (admin_chance != 0) {
        si.ubNumAdmins = admin_chance * start_pop / 100;
      } else
        for (int32_t cnt = start_pop; cnt != 0;
             --cnt) {  // For each soldier randomly determine the type.
          int32_t const roll = Random(100);
          if (roll < elite_chance) {
            ++si.ubNumElites;
          } else if (roll < troop_chance) {
            ++si.ubNumTroops;
          }
        }

      switch (gg.ubComposition) {
        case CAMBRIA_DEFENCE:
        case CAMBRIA_MINE:
        case ALMA_MINE:
        case GRUMM_MINE:
          // Fill up extra start slots with troops
          start_pop -= si.ubNumAdmins;
          si.ubNumTroops = start_pop;
          break;

        case DRASSEN_AIRPORT:
        case DRASSEN_DEFENCE:
        case DRASSEN_MINE:
          si.ubNumAdmins = std::max((uint8_t)5, si.ubNumAdmins);
          break;

        case TIXA_PRISON:
          si.ubNumAdmins = std::max((uint8_t)8, si.ubNumAdmins);
          break;
      }
    }

    if (admin_chance != 0 && si.ubNumAdmins < gubMinEnemyGroupSize) {
      si.ubNumAdmins = gubMinEnemyGroupSize;
    }

    /* Calculate weight (range is -20 to +20 before multiplier). The multiplier
     * of 3 brings it to a range of -96 to +96 which is close enough to a
     * plus/minus 100%. The resultant percentage is then converted based on the
     * priority. */
    int32_t weight = (desired_pop - start_pop) * 3;
    if (weight > 0) {  // Modify it by its priority.
      // Generates a value between 2 and 100
      weight = weight * iPriority / 96;
      weight = std::max(weight, 2);
      giRequestPoints += weight;
    } else if (weight < 0) {  // Modify it by its reverse priority
      // Generates a value between -2 and -100
      weight = weight * (100 - iPriority) / 96;
      weight = std::min(weight, -2);
      giReinforcementPoints -= weight;
    }
    gg.bWeight = weight;

    /* Post an event which allows them to check adjacent sectors periodically.
     * Spread them out so that they process at different times. */
    AddPeriodStrategicEventWithOffset(EVENT_CHECK_ENEMY_CONTROLLED_SECTOR,
                                      140 - 20 * difficulty + Random(4), 475 + i, gg.ubSectorID);
  }

  // Initialize each of the patrol groups
  for (int32_t i = 0; i != giPatrolArraySize; ++i) {
    PATROL_GROUP &pg = gPatrolGroup[i];
    uint8_t n_troops = pg.bSize + Random(3) - 1;
    n_troops = std::max(gubMinEnemyGroupSize, std::min((uint8_t)MAX_STRATEGIC_TEAM_SIZE, n_troops));
    /* Note on adding patrol groups: The patrol group can't actually start on
     * the first waypoint, so we set it to the second way point for
     * initialization, and then add the waypoints from 0 up */
    GROUP &g = *CreateNewEnemyGroupDepartingFromSector(pg.ubSectorID[1], 0, n_troops, 0);
    ENEMYGROUP &eg = *g.pEnemyGroup;

    if (i == 3 || i == 4) { /* Special case: Two patrol groups are administrator
                             * groups -- rest are troops */
      eg.ubNumAdmins = eg.ubNumTroops;
      eg.ubNumTroops = 0;
    }
    pg.ubGroupID = g.ubGroupID;
    eg.ubIntention = PATROL;
    g.ubMoveType = ENDTOEND_FORWARDS;
    FOR_EACH(uint8_t const, i, pg.ubSectorID) {
      if (*i == 0) break;
      AddWaypointIDToPGroup(&g, *i);
    }
    RandomizePatrolGroupLocation(&g);
    ValidateGroup(&g);
  }

  /* Choose one cache map out of five possible maps. Select the sector randomly,
   * set up the flags to use the alternate map, then place 8-12 regular troops
   * there (no AI though). Changing MAX_STRATEGIC_TEAM_SIZE may require changes
   * to to the defending force here. */
  static uint8_t const cache_sectors[] = {SEC_E11, SEC_H5, SEC_H10, SEC_J12, SEC_M9};
  SECTORINFO &si = SectorInfo[cache_sectors[Random(lengthof(cache_sectors))]];
  si.uiFlags |= SF_USE_ALTERNATE_MAP;
  si.ubNumTroops = 6 + difficulty * 2;

  ValidateWeights(1);
}

void KillStrategicAI() {
  if (gPatrolGroup) {
    MemFree(gPatrolGroup);
    gPatrolGroup = NULL;
  }
  if (gGarrisonGroup) {
    MemFree(gGarrisonGroup);
    gGarrisonGroup = NULL;
  }
  if (gubPatrolReinforcementsDenied) {
    MemFree(gubPatrolReinforcementsDenied);
    gubPatrolReinforcementsDenied = NULL;
  }
  if (gubGarrisonReinforcementsDenied) {
    MemFree(gubGarrisonReinforcementsDenied);
    gubGarrisonReinforcementsDenied = NULL;
  }
  DeleteAllStrategicEventsOfType(EVENT_EVALUATE_QUEEN_SITUATION);
}

BOOLEAN OkayForEnemyToMoveThroughSector(uint8_t ubSectorID) {
  SECTORINFO *pSector;
  pSector = &SectorInfo[ubSectorID];
  if (pSector->uiTimeLastPlayerLiberated &&
      pSector->uiTimeLastPlayerLiberated + (gubHoursGracePeriod * 3600) > GetWorldTotalSeconds()) {
    return FALSE;
  }
  return TRUE;
}

static BOOLEAN EnemyPermittedToAttackSector(GROUP **pGroup, uint8_t ubSectorID) {
  SECTORINFO *pSector;
  BOOLEAN fPermittedToAttack = TRUE;

  pSector = &SectorInfo[ubSectorID];
  fPermittedToAttack = OkayForEnemyToMoveThroughSector(ubSectorID);
  if (pGroup && *pGroup && pSector->ubGarrisonID != NO_GARRISON) {
    if (gGarrisonGroup[pSector->ubGarrisonID].ubPendingGroupID) {
      GROUP *pPendingGroup;
      pPendingGroup = GetGroup(gGarrisonGroup[pSector->ubGarrisonID].ubPendingGroupID);
      if (pPendingGroup == *pGroup) {
        if (fPermittedToAttack) {
          if (GroupAtFinalDestination(*pGroup)) {  // High priority reinforcements have arrived.
                                                   // This overrides most other situations.
            return TRUE;
          }
        } else {  // Reassign the group
          ReassignAIGroup(pGroup);
        }
      }
    }
  }
  if (!fPermittedToAttack) {
    return FALSE;
  }
  // If Hill-billies are alive, then enemy won't attack the sector.
  switch (ubSectorID) {
    case SEC_F10:
      // Hill-billy farm -- not until hill billies are dead.
      if (CheckFact(FACT_HILLBILLIES_KILLED, FALSE)) return FALSE;
      break;
    case SEC_A9:
    case SEC_A10:
      // Omerta -- not until Day 2 at 7:45AM.
      if (GetWorldTotalMin() < 3345) return FALSE;
      break;
    case SEC_B13:
    case SEC_C13:
    case SEC_D13:
      // Drassen -- not until Day 3 at 6:30AM.
      if (GetWorldTotalMin() < 4710) return FALSE;
      break;
    case SEC_C5:
    case SEC_C6:
    case SEC_D5:
      // San Mona -- not until Kingpin is dead.
      return CheckFact(FACT_KINGPIN_DEAD, 0);

    case SEC_G1:
      if (PlayerSectorDefended(SEC_G2) &&
          (PlayerSectorDefended(SEC_H1) || PlayerSectorDefended(SEC_H2))) {
        return FALSE;
      }
      break;
    case SEC_H2:
      if (PlayerSectorDefended(SEC_H2) &&
          (PlayerSectorDefended(SEC_G1) || PlayerSectorDefended(SEC_G2))) {
        return FALSE;
      }
      break;
  }
  return TRUE;
}

enum SAIMOVECODE { DIRECT, EVASIVE, STAGE };
static void MoveSAIGroupToSector(GROUP **, uint8_t sector, SAIMOVECODE, uint8_t intention);

static BOOLEAN HandlePlayerGroupNoticedByPatrolGroup(const GROUP *const pPlayerGroup,
                                                     GROUP *pEnemyGroup) {
  uint16_t usDefencePoints;
  uint16_t usOffensePoints;

  uint8_t const ubSectorID = SECTOR(pPlayerGroup->ubSectorX, pPlayerGroup->ubSectorY);
  usOffensePoints = pEnemyGroup->pEnemyGroup->ubNumAdmins * 2 +
                    pEnemyGroup->pEnemyGroup->ubNumTroops * 4 +
                    pEnemyGroup->pEnemyGroup->ubNumElites * 6;
  if (PlayerForceTooStrong(ubSectorID, usOffensePoints, &usDefencePoints)) {
    RequestAttackOnSector(ubSectorID, usDefencePoints);
    return FALSE;
  }
  // For now, automatically attack.
  if (pPlayerGroup->ubNextX) {
    MoveSAIGroupToSector(&pEnemyGroup,
                         (uint8_t)SECTOR(pPlayerGroup->ubNextX, pPlayerGroup->ubNextY), DIRECT,
                         PURSUIT);
  } else {
    MoveSAIGroupToSector(&pEnemyGroup,
                         (uint8_t)SECTOR(pPlayerGroup->ubSectorX, pPlayerGroup->ubSectorY), DIRECT,
                         PURSUIT);
  }
  return TRUE;
}

static void ConvertGroupTroopsToComposition(GROUP *pGroup, int32_t iCompositionID);
static void RemoveSoldiersFromGarrisonBasedOnComposition(int32_t iGarrisonID, uint8_t ubSize);

static void HandlePlayerGroupNoticedByGarrison(const GROUP *const pPlayerGroup,
                                               const uint8_t ubSectorID) {
  SECTORINFO *pSector;
  GROUP *pGroup;
  int32_t iReinforcementsApproved;
  uint16_t usOffensePoints, usDefencePoints;
  uint8_t ubEnemies;
  pSector = &SectorInfo[ubSectorID];
  // First check to see if the player is at his final destination.
  if (!GroupAtFinalDestination(pPlayerGroup)) {
    return;
  }
  usOffensePoints = pSector->ubNumAdmins * 2 + pSector->ubNumTroops * 4 + pSector->ubNumElites * 6;
  if (PlayerForceTooStrong(ubSectorID, usOffensePoints, &usDefencePoints)) {
    RequestAttackOnSector(ubSectorID, usDefencePoints);
    return;
  }

  if (pSector->ubGarrisonID != NO_GARRISON) {
    // Decide whether or not they will attack them with some of the troops.
    ubEnemies = (uint8_t)(pSector->ubNumAdmins + pSector->ubNumTroops + pSector->ubNumElites);
    iReinforcementsApproved =
        (ubEnemies -
         gArmyComp[gGarrisonGroup[pSector->ubGarrisonID].ubComposition].bDesiredPopulation / 2);
    if (iReinforcementsApproved * 2 > pPlayerGroup->ubGroupSize * 3 &&
        iReinforcementsApproved >
            gubMinEnemyGroupSize) {  // Then enemy's available outnumber the
                                     // player by at least 3:2, so attack them.
      pGroup = CreateNewEnemyGroupDepartingFromSector(ubSectorID, 0,
                                                      (uint8_t)iReinforcementsApproved, 0);

      ConvertGroupTroopsToComposition(pGroup, gGarrisonGroup[pSector->ubGarrisonID].ubComposition);

      MoveSAIGroupToSector(&pGroup,
                           (uint8_t)SECTOR(pPlayerGroup->ubSectorX, pPlayerGroup->ubSectorY),
                           DIRECT, REINFORCEMENTS);

      RemoveSoldiersFromGarrisonBasedOnComposition(pSector->ubGarrisonID, pGroup->ubGroupSize);
    }
  }
}

static BOOLEAN HandleMilitiaNoticedByPatrolGroup(uint8_t ubSectorID, GROUP *pEnemyGroup) {
  // For now, automatically attack.
  uint16_t usOffensePoints, usDefencePoints;
  uint8_t ubSectorX = (uint8_t)(ubSectorID % 16) + 1;
  uint8_t ubSectorY = (uint8_t)(ubSectorID / 16) + 1;
  usOffensePoints = pEnemyGroup->pEnemyGroup->ubNumAdmins * 2 +
                    pEnemyGroup->pEnemyGroup->ubNumTroops * 4 +
                    pEnemyGroup->pEnemyGroup->ubNumElites * 6;
  if (PlayerForceTooStrong(ubSectorID, usOffensePoints, &usDefencePoints)) {
    RequestAttackOnSector(ubSectorID, usDefencePoints);
    return FALSE;
  }

  MoveSAIGroupToSector(&pEnemyGroup, (uint8_t)SECTOR(ubSectorX, ubSectorY), DIRECT, REINFORCEMENTS);
  return FALSE;
}

static BOOLEAN AttemptToNoticeEmptySectorSucceeds() {
  if (gubNumAwareBattles || gfAutoAIAware) {  // The queen is in high-alert and is searching for
                                              // players.  All adjacent checks will automatically
                                              // succeed.
    return TRUE;
  }
  if (DayTime()) {  // Day time chances are normal
    if (Chance(giArmyAlertness)) {
      giArmyAlertness -= giArmyAlertnessDecay;
      // Minimum alertness should always be at least 0.
      giArmyAlertness = std::max(0, giArmyAlertness);
      return TRUE;
    }
    giArmyAlertness++;
    return FALSE;
  }
  // Night time chances are one third of normal.
  if (Chance(giArmyAlertness / 3)) {
    giArmyAlertness -= giArmyAlertnessDecay;
    // Minimum alertness should always be at least 0.
    giArmyAlertness = std::max(0, giArmyAlertness);
    return TRUE;
  }
  if (Chance(33)) {
    giArmyAlertness++;
  }
  return FALSE;
}

// Calling the function assumes that a player group is found to be adjacent to
// an enemy group. This uses the alertness rating to emulate the chance that the
// group will notice.  If it does notice, then the alertness drops accordingly to
// simulate a period of time where the enemy would not notice as much.  If it
// fails, the alertness gradually increases until it succeeds.
static BOOLEAN AttemptToNoticeAdjacentGroupSucceeds() {
  if (gubNumAwareBattles || gfAutoAIAware) {  // The queen is in high-alert and is searching for
                                              // players.  All adjacent checks will automatically
                                              // succeed.
    return TRUE;
  }
  if (DayTime()) {  // Day time chances are normal
    if (Chance(giArmyAlertness)) {
      giArmyAlertness -= giArmyAlertnessDecay;
      // Minimum alertness should always be at least 0.
      giArmyAlertness = std::max(0, giArmyAlertness);
      return TRUE;
    }
    giArmyAlertness++;
    return FALSE;
  }
  // Night time chances are one third of normal.
  if (Chance(giArmyAlertness / 3)) {
    giArmyAlertness -= giArmyAlertnessDecay;
    // Minimum alertness should always be at least 0.
    giArmyAlertness = std::max(0, giArmyAlertness);
    return TRUE;
  }
  if (Chance(33)) {
    giArmyAlertness++;
  }
  return FALSE;
}

static BOOLEAN HandleEmptySectorNoticedByPatrolGroup(GROUP *pGroup, uint8_t ubEmptySectorID) {
  uint8_t ubGarrisonID;
  uint8_t ubSectorX = (uint8_t)(ubEmptySectorID % 16) + 1;
  uint8_t ubSectorY = (uint8_t)(ubEmptySectorID / 16) + 1;

  ubGarrisonID = SectorInfo[ubEmptySectorID].ubGarrisonID;
  if (ubGarrisonID != NO_GARRISON) {
    if (gGarrisonGroup[ubGarrisonID].ubPendingGroupID) {
      return FALSE;
    }
  } else {
    return FALSE;
  }

  // Clear the patrol group's previous orders.
  RemoveGroupFromStrategicAILists(*pGroup);

  gGarrisonGroup[ubGarrisonID].ubPendingGroupID = pGroup->ubGroupID;
  MoveSAIGroupToSector(&pGroup, (uint8_t)SECTOR(ubSectorX, ubSectorY), DIRECT, REINFORCEMENTS);

  return TRUE;
}

static void HandleEmptySectorNoticedByGarrison(uint8_t ubGarrisonSectorID,
                                               uint8_t ubEmptySectorID) {
  SECTORINFO *pSector;
  GROUP *pGroup;
  uint8_t ubAvailableTroops;
  uint8_t ubSrcGarrisonID = 255, ubDstGarrisonID = 255;

  // Make sure that the destination sector doesn't already have a pending group.
  pSector = &SectorInfo[ubEmptySectorID];

  ubSrcGarrisonID = SectorInfo[ubGarrisonSectorID].ubGarrisonID;
  ubDstGarrisonID = SectorInfo[ubEmptySectorID].ubGarrisonID;

  if (ubSrcGarrisonID == NO_GARRISON || ubDstGarrisonID == NO_GARRISON) {  // Bad logic
    return;
  }

  if (gGarrisonGroup[ubDstGarrisonID].ubPendingGroupID) {  // A group is already on-route, so don't
                                                           // send anybody from here.
    return;
  }

  // An opportunity has arisen, where the enemy has noticed an important sector
  // that is undefended.
  pSector = &SectorInfo[ubGarrisonSectorID];
  ubAvailableTroops = pSector->ubNumTroops + pSector->ubNumElites + pSector->ubNumAdmins;

  if (ubAvailableTroops >= gubMinEnemyGroupSize * 2) {  // split group into two groups, and move one
                                                        // of the groups to the next sector.
    pGroup = CreateNewEnemyGroupDepartingFromSector(ubGarrisonSectorID, 0,
                                                    (uint8_t)(ubAvailableTroops / 2), 0);
    ConvertGroupTroopsToComposition(pGroup, gGarrisonGroup[ubDstGarrisonID].ubComposition);
    RemoveSoldiersFromGarrisonBasedOnComposition(ubSrcGarrisonID, pGroup->ubGroupSize);
    gGarrisonGroup[ubDstGarrisonID].ubPendingGroupID = pGroup->ubGroupID;
    MoveSAIGroupToSector(&pGroup, ubEmptySectorID, DIRECT, REINFORCEMENTS);
  }
}

static BOOLEAN ReinforcementsApproved(int32_t iGarrisonID, uint16_t *pusDefencePoints) {
  SECTORINFO *pSector;
  uint16_t usOffensePoints;
  uint8_t ubSectorX, ubSectorY;

  pSector = &SectorInfo[gGarrisonGroup[iGarrisonID].ubSectorID];
  ubSectorX = (uint8_t)SECTORX(gGarrisonGroup[iGarrisonID].ubSectorID);
  ubSectorY = (uint8_t)SECTORY(gGarrisonGroup[iGarrisonID].ubSectorID);

  *pusDefencePoints = pSector->ubNumberOfCivsAtLevel[GREEN_MILITIA] * 1 +
                      pSector->ubNumberOfCivsAtLevel[REGULAR_MILITIA] * 2 +
                      pSector->ubNumberOfCivsAtLevel[ELITE_MILITIA] * 3 +
                      PlayerMercsInSector(ubSectorX, ubSectorY, 0) * 4;
  usOffensePoints = gArmyComp[gGarrisonGroup[iGarrisonID].ubComposition].bAdminPercentage * 2 +
                    gArmyComp[gGarrisonGroup[iGarrisonID].ubComposition].bTroopPercentage * 3 +
                    gArmyComp[gGarrisonGroup[iGarrisonID].ubComposition].bElitePercentage * 4 +
                    gubGarrisonReinforcementsDenied[iGarrisonID];
  usOffensePoints = usOffensePoints *
                    gArmyComp[gGarrisonGroup[iGarrisonID].ubComposition].bDesiredPopulation / 100;

  if (usOffensePoints > *pusDefencePoints) {
    return TRUE;
  }
  // Before returning false, determine if reinforcements have been denied
  // repeatedly.  If so, then we might send an augmented force to take it back.
  if (gubGarrisonReinforcementsDenied[iGarrisonID] + usOffensePoints > *pusDefencePoints) {
    return TRUE;
  }
  // Reinforcements will have to wait.  For now, increase the reinforcements
  // denied.  The amount increase is 20 percent of the garrison's priority.
  gubGarrisonReinforcementsDenied[iGarrisonID] +=
      (uint8_t)(gArmyComp[gGarrisonGroup[iGarrisonID].ubComposition].bPriority / 2);

  return FALSE;
}

static void EliminateSurplusTroopsForGarrison(GROUP *pGroup, SECTORINFO *pSector);
static void RecalculateGarrisonWeight(int32_t iGarrisonID);
static void RecalculatePatrolWeight(PATROL_GROUP &);

// if the group has arrived in a sector, and doesn't have any particular orders,
// then send him back where they came from. RETURNS TRUE if the group is deleted
// or told to move somewhere else. This is important as the calling function will
// need to abort processing of the group for obvious reasons.
static BOOLEAN EvaluateGroupSituation(GROUP *pGroup) {
  SECTORINFO *pSector;
  GROUP *pPatrolGroup;
  int32_t i;

  ValidateWeights(2);

  if (!gfQueenAIAwake) {
    return FALSE;
  }
  Assert(!pGroup->fPlayer);
  if (pGroup->pEnemyGroup->ubIntention == PURSUIT) {  // Lost the player group that he was going to
                                                      // attack.  Return to original position.
    ReassignAIGroup(&pGroup);
    return TRUE;
  } else if (pGroup->pEnemyGroup->ubIntention ==
             REINFORCEMENTS) {  // The group has arrived at the location where he
                                // is supposed to reinforce.
    // Step 1 -- Check for matching garrison location
    for (i = 0; i < giGarrisonArraySize; i++) {
      if (gGarrisonGroup[i].ubSectorID == SECTOR(pGroup->ubSectorX, pGroup->ubSectorY) &&
          gGarrisonGroup[i].ubPendingGroupID == pGroup->ubGroupID) {
        pSector = &SectorInfo[SECTOR(pGroup->ubSectorX, pGroup->ubSectorY)];

        if (gGarrisonGroup[i].ubSectorID != SEC_P3) {
          EliminateSurplusTroopsForGarrison(pGroup, pSector);
          pSector->ubNumAdmins = (uint8_t)(pSector->ubNumAdmins + pGroup->pEnemyGroup->ubNumAdmins);
          pSector->ubNumTroops = (uint8_t)(pSector->ubNumTroops + pGroup->pEnemyGroup->ubNumTroops);
          pSector->ubNumElites = (uint8_t)(pSector->ubNumElites + pGroup->pEnemyGroup->ubNumElites);

          if (IsThisSectorASAMSector(pGroup->ubSectorX, pGroup->ubSectorY, 0)) {
            StrategicMap[pGroup->ubSectorX + pGroup->ubSectorY * MAP_WORLD_X].bSAMCondition = 100;
            UpdateSAMDoneRepair(pGroup->ubSectorX, pGroup->ubSectorY, 0);
          }
        } else {  // The group was sent back to the queen's palace (probably
                  // because they couldn't be reassigned
          // anywhere else, but it is possible that the queen's sector is
          // requesting the reinforcements.  In any case, if the queen's sector
          // is less than full strength, fill it up first, then simply add the
          // rest to the global pool.
          if (pSector->ubNumElites < MAX_STRATEGIC_TEAM_SIZE) {
            if (pSector->ubNumElites + pGroup->ubGroupSize >=
                MAX_STRATEGIC_TEAM_SIZE) {  // Fill up the queen's guards, then
                                            // apply the rest to the reinforcement
                                            // pool
              giReinforcementPool += MAX_STRATEGIC_TEAM_SIZE - pSector->ubNumElites;
              pSector->ubNumElites = MAX_STRATEGIC_TEAM_SIZE;
            } else {  // Add all the troops to the queen's guard.
              pSector->ubNumElites += pGroup->ubGroupSize;
            }
          } else {  // Add all the troops to the reinforcement pool as the
                    // queen's guard is at full strength.
            giReinforcementPool += pGroup->ubGroupSize;
          }
        }

        SetThisSectorAsEnemyControlled(pGroup->ubSectorX, pGroup->ubSectorY, 0);
        RemoveGroup(*pGroup);
        RecalculateGarrisonWeight(i);

        return TRUE;
      }
    }
    // Step 2 -- Check for Patrol groups matching waypoint index.
    for (i = 0; i < giPatrolArraySize; i++) {
      if (gPatrolGroup[i].ubSectorID[1] == SECTOR(pGroup->ubSectorX, pGroup->ubSectorY) &&
          gPatrolGroup[i].ubPendingGroupID == pGroup->ubGroupID) {
        gPatrolGroup[i].ubPendingGroupID = 0;
        if (gPatrolGroup[i].ubGroupID &&
            gPatrolGroup[i].ubGroupID != pGroup->ubGroupID) {  // cheat, and warp our reinforcements
                                                               // to them!
          pPatrolGroup = GetGroup(gPatrolGroup[i].ubGroupID);
          pPatrolGroup->pEnemyGroup->ubNumTroops += pGroup->pEnemyGroup->ubNumTroops;
          pPatrolGroup->pEnemyGroup->ubNumElites += pGroup->pEnemyGroup->ubNumElites;
          pPatrolGroup->pEnemyGroup->ubNumAdmins += pGroup->pEnemyGroup->ubNumAdmins;
          pPatrolGroup->ubGroupSize +=
              (uint8_t)(pGroup->pEnemyGroup->ubNumTroops + pGroup->pEnemyGroup->ubNumElites +
                        pGroup->pEnemyGroup->ubNumAdmins);
          if (pPatrolGroup->ubGroupSize > MAX_STRATEGIC_TEAM_SIZE) {
            uint8_t ubCut;
            // truncate the group size.
            ubCut = pPatrolGroup->ubGroupSize - MAX_STRATEGIC_TEAM_SIZE;
            while (ubCut--) {
              if (pGroup->pEnemyGroup->ubNumAdmins) {
                pGroup->pEnemyGroup->ubNumAdmins--;
                pPatrolGroup->pEnemyGroup->ubNumAdmins--;
              } else if (pGroup->pEnemyGroup->ubNumTroops) {
                pGroup->pEnemyGroup->ubNumTroops--;
                pPatrolGroup->pEnemyGroup->ubNumTroops--;
              } else if (pGroup->pEnemyGroup->ubNumElites) {
                pGroup->pEnemyGroup->ubNumElites--;
                pPatrolGroup->pEnemyGroup->ubNumElites--;
              }
            }
            pPatrolGroup->ubGroupSize = MAX_STRATEGIC_TEAM_SIZE;
            Assert(pPatrolGroup->pEnemyGroup->ubNumAdmins + pPatrolGroup->pEnemyGroup->ubNumTroops +
                       pPatrolGroup->pEnemyGroup->ubNumElites ==
                   MAX_STRATEGIC_TEAM_SIZE);
          }
          RemoveGroup(*pGroup);
          RecalculatePatrolWeight(gPatrolGroup[i]);
          ValidateLargeGroup(pPatrolGroup);
        } else {  // the reinforcements have become the new patrol group (even if
                  // same group)
          gPatrolGroup[i].ubGroupID = pGroup->ubGroupID;
          pGroup->pEnemyGroup->ubIntention = PATROL;
          pGroup->ubMoveType = ENDTOEND_FORWARDS;
          RemoveGroupWaypoints(*pGroup);
          AddWaypointIDToPGroup(pGroup, gPatrolGroup[i].ubSectorID[0]);
          AddWaypointIDToPGroup(pGroup, gPatrolGroup[i].ubSectorID[1]);
          if (gPatrolGroup[i].ubSectorID[2]) {  // Add optional waypoints if included.
            AddWaypointIDToPGroup(pGroup, gPatrolGroup[i].ubSectorID[2]);
            if (gPatrolGroup[i].ubSectorID[3])
              AddWaypointIDToPGroup(pGroup, gPatrolGroup[i].ubSectorID[3]);
          }
          RecalculatePatrolWeight(gPatrolGroup[i]);
        }
        return TRUE;
      }
    }
  } else {  // This is a floating group at his final destination...
    if (pGroup->pEnemyGroup->ubIntention != STAGING &&
        pGroup->pEnemyGroup->ubIntention != REINFORCEMENTS) {
      ReassignAIGroup(&pGroup);
      return TRUE;
    }
  }
  ValidateWeights(3);
  return FALSE;
}

static bool EnemyNoticesPlayerArrival(GROUP const &pg, uint8_t const x, uint8_t const y) {
  GROUP *const eg = FindEnemyMovementGroupInSector(x, y);
  if (eg && AttemptToNoticeAdjacentGroupSucceeds()) {
    HandlePlayerGroupNoticedByPatrolGroup(&pg, eg);
    return true;
  }

  SECTORINFO const &s = SectorInfo[SECTOR(x, y)];
  uint8_t const n_enemies = s.ubNumAdmins + s.ubNumTroops + s.ubNumElites;
  if (n_enemies && s.ubGarrisonID != NO_GARRISON && AttemptToNoticeAdjacentGroupSucceeds()) {
    HandlePlayerGroupNoticedByGarrison(&pg, SECTOR(x, y));
    return true;
  }

  return false;
}

static void SendGroupToPool(GROUP **pGroup);

// returns TRUE if the group was deleted.
BOOLEAN StrategicAILookForAdjacentGroups(GROUP *pGroup) {
  uint8_t ubSectorID;
  if (!gfQueenAIAwake) {  // The queen isn't aware the player's presence yet, so
                          // she is oblivious to any situations.

    if (!pGroup->fPlayer) {
      // Exception case!
      // In the beginning of the game, a group is sent to A9 after the first
      // battle.  If you leave A9, when they arrive, they will stay there
      // indefinately because the AI isn't awake.  What we do, is if this is a
      // group in A9, then send them home.
      if (GroupAtFinalDestination(pGroup)) {
        // Wake up the queen now, if she hasn't woken up already.
        WakeUpQueen();
        if ((pGroup->ubSectorX == 9 && pGroup->ubSectorY == 1) ||
            (pGroup->ubSectorX == 3 && pGroup->ubSectorY == 16)) {
          SendGroupToPool(&pGroup);
          if (!pGroup) {  // Group was transferred to the pool
            return TRUE;
          }
        }
      }
    }

    if (!gfQueenAIAwake) {
      return FALSE;
    }
  }
  if (!pGroup->fPlayer) {  // The enemy group has arrived at a new sector and now
                           // controls it.
    // Look in each of the four directions, and the alertness rating will
    // determine the chance to detect any players that may exist in that sector.
    GROUP *pEnemyGroup = pGroup;
    if (GroupAtFinalDestination(pEnemyGroup)) {
      return EvaluateGroupSituation(pEnemyGroup);
    }
    ubSectorID = (uint8_t)SECTOR(pEnemyGroup->ubSectorX, pEnemyGroup->ubSectorY);
    if (pEnemyGroup && pEnemyGroup->ubSectorY > 1 &&
        EnemyPermittedToAttackSector(&pEnemyGroup, (uint8_t)(ubSectorID - 16))) {
      GROUP *const pPlayerGroup =
          FindPlayerMovementGroupInSector(pEnemyGroup->ubSectorX, pEnemyGroup->ubSectorY - 1);
      if (pPlayerGroup && AttemptToNoticeAdjacentGroupSucceeds()) {
        return HandlePlayerGroupNoticedByPatrolGroup(pPlayerGroup, pEnemyGroup);
      } else if (CountAllMilitiaInSector(pEnemyGroup->ubSectorX,
                                         (uint8_t)(pEnemyGroup->ubSectorY - 1)) &&
                 AttemptToNoticeAdjacentGroupSucceeds()) {
        return HandleMilitiaNoticedByPatrolGroup(
            (uint8_t)SECTOR(pEnemyGroup->ubSectorX, pEnemyGroup->ubSectorY - 1), pEnemyGroup);
      } else if (AdjacentSectorIsImportantAndUndefended((uint8_t)(ubSectorID - 16)) &&
                 AttemptToNoticeEmptySectorSucceeds()) {
        return HandleEmptySectorNoticedByPatrolGroup(pEnemyGroup, (uint8_t)(ubSectorID - 16));
      }
    }
    if (pEnemyGroup && pEnemyGroup->ubSectorX > 1 &&
        EnemyPermittedToAttackSector(&pEnemyGroup, (uint8_t)(ubSectorID - 1))) {
      GROUP *const pPlayerGroup =
          FindPlayerMovementGroupInSector(pEnemyGroup->ubSectorX - 1, pEnemyGroup->ubSectorY);
      if (pPlayerGroup && AttemptToNoticeAdjacentGroupSucceeds()) {
        return HandlePlayerGroupNoticedByPatrolGroup(pPlayerGroup, pEnemyGroup);
      } else if (CountAllMilitiaInSector((uint8_t)(pEnemyGroup->ubSectorX - 1),
                                         pEnemyGroup->ubSectorY) &&
                 AttemptToNoticeAdjacentGroupSucceeds()) {
        return HandleMilitiaNoticedByPatrolGroup(
            (uint8_t)SECTOR(pEnemyGroup->ubSectorX - 1, pEnemyGroup->ubSectorY), pEnemyGroup);
      } else if (AdjacentSectorIsImportantAndUndefended((uint8_t)(ubSectorID - 1)) &&
                 AttemptToNoticeEmptySectorSucceeds()) {
        return HandleEmptySectorNoticedByPatrolGroup(pEnemyGroup, (uint8_t)(ubSectorID - 1));
      }
    }
    if (pEnemyGroup && pEnemyGroup->ubSectorY < 16 &&
        EnemyPermittedToAttackSector(&pEnemyGroup, (uint8_t)(ubSectorID + 16))) {
      GROUP *const pPlayerGroup =
          FindPlayerMovementGroupInSector(pEnemyGroup->ubSectorX, pEnemyGroup->ubSectorY + 1);
      if (pPlayerGroup && AttemptToNoticeAdjacentGroupSucceeds()) {
        return HandlePlayerGroupNoticedByPatrolGroup(pPlayerGroup, pEnemyGroup);
      } else if (CountAllMilitiaInSector(pEnemyGroup->ubSectorX,
                                         (uint8_t)(pEnemyGroup->ubSectorY + 1)) &&
                 AttemptToNoticeAdjacentGroupSucceeds()) {
        return HandleMilitiaNoticedByPatrolGroup(
            (uint8_t)SECTOR(pEnemyGroup->ubSectorX, pEnemyGroup->ubSectorY + 1), pEnemyGroup);
      } else if (AdjacentSectorIsImportantAndUndefended((uint8_t)(ubSectorID + 16)) &&
                 AttemptToNoticeEmptySectorSucceeds()) {
        return HandleEmptySectorNoticedByPatrolGroup(pEnemyGroup, (uint8_t)(ubSectorID + 16));
      }
    }
    if (pEnemyGroup && pEnemyGroup->ubSectorX < 16 &&
        EnemyPermittedToAttackSector(&pEnemyGroup, (uint8_t)(ubSectorID + 1))) {
      GROUP *const pPlayerGroup =
          FindPlayerMovementGroupInSector(pEnemyGroup->ubSectorX + 1, pEnemyGroup->ubSectorY);
      if (pPlayerGroup && AttemptToNoticeAdjacentGroupSucceeds()) {
        return HandlePlayerGroupNoticedByPatrolGroup(pPlayerGroup, pEnemyGroup);
      } else if (CountAllMilitiaInSector((uint8_t)(pEnemyGroup->ubSectorX + 1),
                                         pEnemyGroup->ubSectorY) &&
                 AttemptToNoticeAdjacentGroupSucceeds()) {
        return HandleMilitiaNoticedByPatrolGroup(
            (uint8_t)SECTOR(pEnemyGroup->ubSectorX + 1, pEnemyGroup->ubSectorY), pEnemyGroup);
      } else if (AdjacentSectorIsImportantAndUndefended((uint8_t)(ubSectorID + 1)) &&
                 AttemptToNoticeEmptySectorSucceeds()) {
        return HandleEmptySectorNoticedByPatrolGroup(pEnemyGroup, (uint8_t)(ubSectorID + 1));
      }
    }
    if (!pEnemyGroup) {  // group deleted.
      return TRUE;
    }
  } else { /* The player group has arrived at a new sector and now controls it.
            * Look in each of the four directions, and the enemy alertness
            * rating will determine if the enemy notices that the player is
            * here.  Additionally, there are also stationary enemy groups that
            * may also notice the player's new presence. NOTE: Always returns
            * false because it is the player group that we are handling.  We
            * don't mess with the player group here! */
    GROUP const &pg = *pGroup;
    if (pg.ubSectorZ != 0) return FALSE;
    uint8_t const x = pg.ubSectorX;
    uint8_t const y = pg.ubSectorY;
    if (!EnemyPermittedToAttackSector(0, SECTOR(x, y))) return FALSE;
    if (y > 1 && EnemyNoticesPlayerArrival(pg, x, y - 1)) return FALSE;
    if (x < 16 && EnemyNoticesPlayerArrival(pg, x + 1, y)) return FALSE;
    if (y < 16 && EnemyNoticesPlayerArrival(pg, x, y + 1)) return FALSE;
    if (x > 1 && EnemyNoticesPlayerArrival(pg, x - 1, y)) return FALSE;
  }
  return FALSE;
}

// This is called periodically for each enemy occupied sector containing
// garrisons.
void CheckEnemyControlledSector(uint8_t ubSectorID) {
  SECTORINFO *pSector;
  uint8_t ubSectorX, ubSectorY;
  if (!gfQueenAIAwake) {
    return;
  }
  // First, determine if the sector is still owned by the enemy.
  pSector = &SectorInfo[ubSectorID];
  if (pSector->ubGarrisonID != NO_GARRISON) {
    if (gGarrisonGroup[pSector->ubGarrisonID].ubPendingGroupID) {  // Look for a staging group.
      GROUP *pGroup;
      pGroup = GetGroup(gGarrisonGroup[pSector->ubGarrisonID].ubPendingGroupID);
      if (pGroup) {  // We have a staging group
        if (GroupAtFinalDestination(pGroup)) {
          if (pGroup->pEnemyGroup->ubPendingReinforcements) {
            if (pGroup->pEnemyGroup->ubPendingReinforcements > 4) {
              uint8_t ubNum = (uint8_t)(3 + Random(3));
              pGroup->pEnemyGroup->ubNumTroops += ubNum;
              pGroup->ubGroupSize += ubNum;
              pGroup->pEnemyGroup->ubPendingReinforcements -= ubNum;
              RecalculateGroupWeight(*pGroup);
              ValidateLargeGroup(pGroup);
            } else {
              pGroup->pEnemyGroup->ubNumTroops += pGroup->pEnemyGroup->ubPendingReinforcements;
              pGroup->ubGroupSize += pGroup->pEnemyGroup->ubPendingReinforcements;
              pGroup->pEnemyGroup->ubPendingReinforcements = 0;
              ValidateLargeGroup(pGroup);
            }
          } else if (SECTOR(pGroup->ubSectorX, pGroup->ubSectorY) !=
                     gGarrisonGroup[pSector->ubGarrisonID].ubSectorID) {
            MoveSAIGroupToSector(&pGroup, gGarrisonGroup[pSector->ubGarrisonID].ubSectorID, DIRECT,
                                 pGroup->pEnemyGroup->ubIntention);
          }
        }
        // else the group is on route to stage hopefully...
      }
    }
  }
  if (pSector->ubNumAdmins + pSector->ubNumTroops + pSector->ubNumElites) {
    // The sector is still controlled, so look around to see if there are any
    // players nearby.
    ubSectorX = (uint8_t)SECTORX(ubSectorID);
    ubSectorY = (uint8_t)SECTORY(ubSectorID);
    if (ubSectorY > 1 && EnemyPermittedToAttackSector(NULL, (uint8_t)(ubSectorID - 16))) {
      /*
      pPlayerGroup = FindPlayerMovementGroupInSector(bSectorX, ubSectorY - 1);
      if( pPlayerGroup && AttemptToNoticeAdjacentGroupSucceeds() )
      {
              HandlePlayerGroupNoticedByGarrison( pPlayerGroup, ubSectorID );
              return;
      }
      else
      */
      if (AdjacentSectorIsImportantAndUndefended((uint8_t)(ubSectorID - 16)) &&
          AttemptToNoticeEmptySectorSucceeds()) {
        HandleEmptySectorNoticedByGarrison(ubSectorID, (uint8_t)(ubSectorID - 16));
        return;
      }
    }
    if (ubSectorX < 16 && EnemyPermittedToAttackSector(NULL, (uint8_t)(ubSectorID + 1))) {
      /*
      pPlayerGroup = FindPlayerMovementGroupInSector(ubSectorX + 1, ubSectorY);
      if( pPlayerGroup && AttemptToNoticeAdjacentGroupSucceeds() )
      {
              HandlePlayerGroupNoticedByGarrison( pPlayerGroup, ubSectorID );
              return;
      }
      else
      */
      if (AdjacentSectorIsImportantAndUndefended((uint8_t)(ubSectorID + 1)) &&
          AttemptToNoticeEmptySectorSucceeds()) {
        HandleEmptySectorNoticedByGarrison(ubSectorID, (uint8_t)(ubSectorID + 1));
        return;
      }
    }
    if (ubSectorY < 16 && EnemyPermittedToAttackSector(NULL, (uint8_t)(ubSectorID + 16))) {
      /*
      pPlayerGroup = FindPlayerMovementGroupInSector(ubSectorX, ubSectorY + 1);
      if( pPlayerGroup && AttemptToNoticeAdjacentGroupSucceeds() )
      {
              HandlePlayerGroupNoticedByGarrison( pPlayerGroup, ubSectorID );
              return;
      }
      else
      */
      if (AdjacentSectorIsImportantAndUndefended((uint8_t)(ubSectorID + 16)) &&
          AttemptToNoticeEmptySectorSucceeds()) {
        HandleEmptySectorNoticedByGarrison(ubSectorID, (uint8_t)(ubSectorID + 16));
        return;
      }
    }
    if (ubSectorX > 1 && EnemyPermittedToAttackSector(NULL, (uint8_t)(ubSectorID - 1))) {
      /*
      pPlayerGroup = FindPlayerMovementGroupInSector(ubSectorX - 1, ubSectorY);
      if( pPlayerGroup && AttemptToNoticeAdjacentGroupSucceeds() )
      {
              HandlePlayerGroupNoticedByGarrison( pPlayerGroup, ubSectorID );
              return;
      }
      else
      */
      if (AdjacentSectorIsImportantAndUndefended((uint8_t)(ubSectorID - 1)) &&
          AttemptToNoticeEmptySectorSucceeds()) {
        HandleEmptySectorNoticedByGarrison(ubSectorID, (uint8_t)(ubSectorID - 1));
        return;
      }
    }
  }
}

void RemoveGroupFromStrategicAILists(GROUP const &g) {
  uint8_t const group_id = g.ubGroupID;
  for (int32_t i = 0; i < giPatrolArraySize; ++i) {
    PATROL_GROUP &pg = gPatrolGroup[i];
    if (pg.ubGroupID == group_id) {  // Patrol group was destroyed
      pg.ubGroupID = 0;
      RecalculatePatrolWeight(pg);
      return;
    }
    if (pg.ubPendingGroupID == group_id) {  // Group never arrived to reinforce
      pg.ubPendingGroupID = 0;
      return;
    }
  }
  for (int32_t i = 0; i < giGarrisonArraySize; ++i) {
    GARRISON_GROUP &gg = gGarrisonGroup[i];
    if (gg.ubPendingGroupID == group_id) {  // Group never arrived to reinforce
      gg.ubPendingGroupID = 0;
      return;
    }
  }
}

/* Recalculates a group's weight based on any changes.
 * @@@Alex, this is possibly missing in some areas. It is hard to ensure it is
 * everywhere with all the changes I've made. I'm sure you could probably find
 * some missing calls. */
static void RecalculatePatrolWeight(PATROL_GROUP &p) {
  ValidateWeights(4);

  // First, remove the previous weight from the applicable field
  int32_t const prev_weight = p.bWeight;
  if (prev_weight > 0) giRequestPoints -= prev_weight;

  int32_t need_population;
  if (p.ubGroupID != 0) {
    need_population = p.bSize - GetGroup(p.ubGroupID)->ubGroupSize;
    if (need_population < 0) {
      p.bWeight = 0;
      ValidateWeights(27);
      return;
    }
  } else {
    need_population = p.bSize;
  }
  int32_t weight = need_population * 3 * p.bPriority / 96;
  weight = std::min(weight, 2);
  p.bWeight = weight;
  giRequestPoints += weight;

  ValidateWeights(5);
}

// Recalculates a group's weight based on any changes.
//@@@Alex, this is possibly missing in some areas.  It is hard to ensure it is
// everywhere with all the changes I've made.  I'm sure you could probably find
// some missing calls.
static void RecalculateGarrisonWeight(int32_t iGarrisonID) {
  SECTORINFO *pSector;
  int32_t iWeight, iPrevWeight;
  int32_t iDesiredPop, iCurrentPop, iPriority;

  ValidateWeights(6);

  pSector = &SectorInfo[gGarrisonGroup[iGarrisonID].ubSectorID];
  iDesiredPop = gArmyComp[gGarrisonGroup[iGarrisonID].ubComposition].bDesiredPopulation;
  iCurrentPop = pSector->ubNumAdmins + pSector->ubNumTroops + pSector->ubNumElites;
  iPriority = gArmyComp[gGarrisonGroup[iGarrisonID].ubComposition].bPriority;

  // First, remove the previous weight from the applicable field.
  iPrevWeight = gGarrisonGroup[iGarrisonID].bWeight;
  if (iPrevWeight > 0)
    giRequestPoints -= iPrevWeight;
  else if (iPrevWeight < 0)
    giReinforcementPoints += iPrevWeight;

  // Calculate weight (range is -20 to +20 before multiplier).
  // The multiplier of 3 brings it to a range of -96 to +96 which is
  // close enough to a plus/minus 100%.  The resultant percentage is then
  // converted based on the priority.
  iWeight = (iDesiredPop - iCurrentPop) * 3;
  if (iWeight > 0) {  // modify it by it's priority.
    // generates a value between 2 and 100
    iWeight = iWeight * iPriority / 96;
    iWeight = std::max(iWeight, 2);
    giRequestPoints += iWeight;
  } else if (iWeight < 0) {  // modify it by it's reverse priority
    // generates a value between -2 and -100
    iWeight = iWeight * (100 - iPriority) / 96;
    iWeight = std::min(iWeight, -2);
    giReinforcementPoints -= (int8_t)iWeight;
  }

  gGarrisonGroup[iGarrisonID].bWeight = (int8_t)iWeight;

  ValidateWeights(7);
}

void RecalculateSectorWeight(uint8_t ubSectorID) {
  int32_t i;
  for (i = 0; i < giGarrisonArraySize; i++) {
    if (gGarrisonGroup[i].ubSectorID == ubSectorID) {
      RecalculateGarrisonWeight(i);
      return;
    }
  }
}

static void TagSAIGroupWithGracePeriod(GROUP const &);

void RecalculateGroupWeight(GROUP const &g) {
  for (int32_t i = 0; i != giPatrolArraySize; ++i) {
    PATROL_GROUP &p = gPatrolGroup[i];
    if (p.ubGroupID != g.ubGroupID) continue;
    if (g.ubGroupSize == 0) {
      TagSAIGroupWithGracePeriod(g);
      p.ubGroupID = 0;
    }
    RecalculatePatrolWeight(p);
    return;
  }
}

static BOOLEAN GarrisonCanProvideMinimumReinforcements(int32_t iGarrisonID);

static int32_t ChooseSuitableGarrisonToProvideReinforcements(int32_t iDstGarrisonID,
                                                             int32_t iReinforcementsRequested) {
  int32_t iSrcGarrisonID, iBestGarrisonID = NO_GARRISON;
  int32_t iReinforcementsAvailable;
  int32_t i, iRandom, iWeight;
  int8_t bBestWeight;
  uint8_t ubSectorID;

  // Check to see if we could send reinforcements from Alma.  Only
  // Drassen/Cambria get preferred service from Alma, due to it's proximity and
  // Alma's purpose as a forward military base.
  ubSectorID = gGarrisonGroup[iDstGarrisonID].ubSectorID;
  switch (ubSectorID) {
    case SEC_B13:
    case SEC_C13:
    case SEC_D13:
    case SEC_D15:  // Drassen + nearby SAM site
    case SEC_F8:
    case SEC_F9:
    case SEC_G8:
    case SEC_G9:
    case SEC_H8:  // Cambria
      // reinforcements will be primarily sent from Alma whenever possible.

      // find which the first sector that contains Alma soldiers.
      for (i = 0; i < giGarrisonArraySize; i++) {
        if (gGarrisonGroup[i].ubComposition == ALMA_DEFENCE) break;
      }
      iSrcGarrisonID = i;
      // which of these 4 Alma garrisons have the most reinforcements available?
      // It is possible that none of these garrisons can provide any
      // reinforcements.
      bBestWeight = 0;
      for (i = iSrcGarrisonID; i < iSrcGarrisonID + 4; i++) {
        RecalculateGarrisonWeight(i);
        if (bBestWeight > gGarrisonGroup[i].bWeight && GarrisonCanProvideMinimumReinforcements(i)) {
          bBestWeight = gGarrisonGroup[i].bWeight;
          iBestGarrisonID = i;
        }
      }
      // If we can provide reinforcements from Alma, then make sure that it can
      // provide at least 67% of the requested reinforcements.
      if (bBestWeight < 0) {
        iReinforcementsAvailable = ReinforcementsAvailable(iBestGarrisonID);
        if (iReinforcementsAvailable * 100 >=
            iReinforcementsRequested *
                67) {  // This is the approved group to provide the reinforcements.
          return iBestGarrisonID;
        }
      }
      break;
  }

  // The Alma case either wasn't applicable or failed to have the right
  // reinforcements.  Do a general weighted search.
  iRandom = Random(giReinforcementPoints);
  for (iSrcGarrisonID = 0; iSrcGarrisonID < giGarrisonArraySize;
       iSrcGarrisonID++) {  // go through the garrisons
    RecalculateGarrisonWeight(iSrcGarrisonID);
    iWeight = -gGarrisonGroup[iSrcGarrisonID].bWeight;
    if (iWeight > 0) {  // if group is able to provide reinforcements.
      if (iRandom < iWeight && GarrisonCanProvideMinimumReinforcements(iSrcGarrisonID)) {
        iReinforcementsAvailable = ReinforcementsAvailable(iSrcGarrisonID);
        if (iReinforcementsAvailable * 100 >=
            iReinforcementsRequested * 67) {  // This is the approved group to
                                              // provide the reinforcements.
          return iSrcGarrisonID;
        }
      }
      iRandom -= iWeight;
    }
  }

  // So far we have failed on all accounts.  Now, simply process all the
  // garrisons, and return the first garrison that can provide the
  // reinforcements.
  for (iSrcGarrisonID = 0; iSrcGarrisonID < giGarrisonArraySize;
       iSrcGarrisonID++) {  // go through the garrisons
    RecalculateGarrisonWeight(iSrcGarrisonID);
    iWeight = -gGarrisonGroup[iSrcGarrisonID].bWeight;
    if (iWeight > 0 && GarrisonCanProvideMinimumReinforcements(
                           iSrcGarrisonID)) {  // if group is able to provide reinforcements.
      iReinforcementsAvailable = ReinforcementsAvailable(iSrcGarrisonID);
      if (iReinforcementsAvailable * 100 >=
          iReinforcementsRequested *
              67) {  // This is the approved group to provide the reinforcements.
        return iSrcGarrisonID;
      }
    }
  }

  // Well, if we get this far, the queen must be low on troops.  Send whatever
  // we can.
  iRandom = Random(giReinforcementPoints);
  for (iSrcGarrisonID = 0; iSrcGarrisonID < giGarrisonArraySize;
       iSrcGarrisonID++) {  // go through the garrisons
    RecalculateGarrisonWeight(iSrcGarrisonID);
    iWeight = -gGarrisonGroup[iSrcGarrisonID].bWeight;
    if (iWeight > 0 && GarrisonCanProvideMinimumReinforcements(
                           iSrcGarrisonID)) {  // if group is able to provide reinforcements.
      if (iRandom < iWeight) {
        iReinforcementsAvailable = ReinforcementsAvailable(iSrcGarrisonID);
        return iSrcGarrisonID;
      }
      iRandom -= iWeight;
    }
  }

  // Failed completely.
  return -1;
}

static void SendReinforcementsForGarrison(int32_t iDstGarrisonID, uint16_t usDefencePoints,
                                          GROUP **pOptionalGroup) {
  int32_t iChance, iRandom, iSrcGarrisonID;
  int32_t iMaxReinforcementsAllowed, iReinforcementsAvailable, iReinforcementsRequested,
      iReinforcementsApproved;
  GROUP *pGroup;
  uint8_t ubSrcSectorX, ubSrcSectorY, ubDstSectorX, ubDstSectorY;
  uint8_t ubNumExtraReinforcements;
  uint8_t ubGroupSize;
  BOOLEAN fLimitMaxTroopsAllowable = FALSE;

  ValidateWeights(8);

  // Determine how many units the garrison needs.
  iReinforcementsRequested =
      GarrisonReinforcementsRequested(iDstGarrisonID, &ubNumExtraReinforcements);

  // The maximum number of reinforcements can't be offsetted past a certain
  // point based on the priority of the garrison.
  iMaxReinforcementsAllowed =  // from 1 to 3 times the desired size of the
                               // normal force.
      gArmyComp[gGarrisonGroup[iDstGarrisonID].ubComposition].bDesiredPopulation +
      gArmyComp[gGarrisonGroup[iDstGarrisonID].ubComposition].bDesiredPopulation *
          gArmyComp[gGarrisonGroup[iDstGarrisonID].ubComposition].bPriority / 50;

  if (iReinforcementsRequested + ubNumExtraReinforcements >
      iMaxReinforcementsAllowed) {  // adjust the extra reinforcements so that it
                                    // doesn't exceed the maximum allowed.
    fLimitMaxTroopsAllowable = TRUE;
    ubNumExtraReinforcements = (uint8_t)(iMaxReinforcementsAllowed - iReinforcementsRequested);
  }

  iReinforcementsRequested += ubNumExtraReinforcements;

  if (iReinforcementsRequested <= 0) {
    ValidateWeights(9);
    return;
  }

  ubDstSectorX = (uint8_t)SECTORX(gGarrisonGroup[iDstGarrisonID].ubSectorID);
  ubDstSectorY = (uint8_t)SECTORY(gGarrisonGroup[iDstGarrisonID].ubSectorID);

  if (pOptionalGroup && *pOptionalGroup) {  // This group will provide the reinforcements
    pGroup = *pOptionalGroup;

    gGarrisonGroup[iDstGarrisonID].ubPendingGroupID = pGroup->ubGroupID;
    ConvertGroupTroopsToComposition(pGroup, gGarrisonGroup[iDstGarrisonID].ubComposition);
    MoveSAIGroupToSector(pOptionalGroup, gGarrisonGroup[iDstGarrisonID].ubSectorID, STAGE,
                         REINFORCEMENTS);

    ValidateWeights(10);

    return;
  }
  iRandom = Random(giReinforcementPoints + giReinforcementPool);
  if (iRandom < giReinforcementPool) {  // use the pool and send the requested
                                        // amount from SECTOR P3 (queen's palace)
  QUEEN_POOL:

    // KM : Sep 9, 1999
    // If the player owns sector P3, any troops that spawned there were causing
    // serious problems, seeing battle checks were not performed!
    if (!StrategicMap[CALCULATE_STRATEGIC_INDEX(3, 16)]
             .fEnemyControlled) {  // Queen can no longer send reinforcements
                                   // from the palace if she doesn't control it!
      return;
    }

    if (!giReinforcementPool) {
      ValidateWeights(11);
      return;
    }
    iReinforcementsApproved = std::min(iReinforcementsRequested, giReinforcementPool);

    if (iReinforcementsApproved * 3 <
        usDefencePoints) {  // The enemy force that would be sent would likely be
                            // decimated by the player forces.
      gubGarrisonReinforcementsDenied[iDstGarrisonID] +=
          (uint8_t)(gArmyComp[gGarrisonGroup[iDstGarrisonID].ubComposition].bPriority / 2);
      ValidateWeights(12);
      return;
    } else {
      // The force is strong enough to be able to take the sector.
      gubGarrisonReinforcementsDenied[iDstGarrisonID] = 0;
    }

    // The chance she will send them is related with the strength difference
    // between the player's force and the queen's.
    if (ubNumExtraReinforcements && fLimitMaxTroopsAllowable &&
        iReinforcementsApproved == iMaxReinforcementsAllowed) {
      iChance = (iReinforcementsApproved + ubNumExtraReinforcements) * 100 / usDefencePoints;
      if (!Chance(iChance)) {
        ValidateWeights(13);
        return;
      }
    }

    pGroup = CreateNewEnemyGroupDepartingFromSector(SEC_P3, 0, (uint8_t)iReinforcementsApproved, 0);
    ConvertGroupTroopsToComposition(pGroup, gGarrisonGroup[iDstGarrisonID].ubComposition);
    pGroup->ubOriginalSector = (uint8_t)SECTOR(ubDstSectorX, ubDstSectorY);
    giReinforcementPool -= iReinforcementsApproved;
    pGroup->ubMoveType = ONE_WAY;
    gGarrisonGroup[iDstGarrisonID].ubPendingGroupID = pGroup->ubGroupID;

    ubGroupSize = (uint8_t)(pGroup->pEnemyGroup->ubNumTroops + pGroup->pEnemyGroup->ubNumElites +
                            pGroup->pEnemyGroup->ubNumAdmins);

    if (ubNumExtraReinforcements) {
      MoveSAIGroupToSector(&pGroup, gGarrisonGroup[iDstGarrisonID].ubSectorID, STAGE, STAGING);
    } else {
      MoveSAIGroupToSector(&pGroup, gGarrisonGroup[iDstGarrisonID].ubSectorID, STAGE,
                           REINFORCEMENTS);
    }
    ValidateWeights(14);
    return;
  } else {
    iSrcGarrisonID =
        ChooseSuitableGarrisonToProvideReinforcements(iDstGarrisonID, iReinforcementsRequested);
    if (iSrcGarrisonID == -1) {
      ValidateWeights(15);
      goto QUEEN_POOL;
    }

    ubSrcSectorX = (gGarrisonGroup[iSrcGarrisonID].ubSectorID % 16) + 1;
    ubSrcSectorY = (gGarrisonGroup[iSrcGarrisonID].ubSectorID / 16) + 1;
    if (ubSrcSectorX != gWorldSectorX || ubSrcSectorY != gWorldSectorY ||
        gbWorldSectorZ > 0) {  // The reinforcements aren't coming from the
                               // currently loaded sector!
      iReinforcementsAvailable = ReinforcementsAvailable(iSrcGarrisonID);
      if (iReinforcementsAvailable <= 0) {
        SAIReportError(
            L"Attempting to send reinforcements from a garrison "
            L"that doesn't have any! -- KM:0 (with prior saved game "
            L"and strategic decisions.txt)");
        return;
      }
      // Send the lowest of the two:  number requested or number available

      iReinforcementsApproved = std::min(iReinforcementsRequested, iReinforcementsAvailable);
      if (iReinforcementsApproved >
          iMaxReinforcementsAllowed - ubNumExtraReinforcements) {  // The force isn't strong enough,
                                                                   // but the queen isn't willing to
                                                                   // apply extra resources
        iReinforcementsApproved = iMaxReinforcementsAllowed - ubNumExtraReinforcements;
      } else if ((iReinforcementsApproved + ubNumExtraReinforcements) * 3 <
                 usDefencePoints) {  // The enemy force that would be sent would
                                     // likely be decimated by the player forces.
        gubGarrisonReinforcementsDenied[iDstGarrisonID] +=
            (uint8_t)(gArmyComp[gGarrisonGroup[iDstGarrisonID].ubComposition].bPriority / 2);
        ValidateWeights(17);
        return;
      } else {
        // The force is strong enough to be able to take the sector.
        gubGarrisonReinforcementsDenied[iDstGarrisonID] = 0;
      }

      // The chance she will send them is related with the strength difference
      // between the player's force and the queen's.
      if (iReinforcementsApproved + ubNumExtraReinforcements == iMaxReinforcementsAllowed &&
          usDefencePoints) {
        iChance = (iReinforcementsApproved + ubNumExtraReinforcements) * 100 / usDefencePoints;
        if (!Chance(iChance)) {
          ValidateWeights(18);
          return;
        }
      }

      pGroup = CreateNewEnemyGroupDepartingFromSector(gGarrisonGroup[iSrcGarrisonID].ubSectorID, 0,
                                                      (uint8_t)iReinforcementsApproved, 0);
      ConvertGroupTroopsToComposition(pGroup, gGarrisonGroup[iDstGarrisonID].ubComposition);
      RemoveSoldiersFromGarrisonBasedOnComposition(iSrcGarrisonID, pGroup->ubGroupSize);
      pGroup->ubOriginalSector = (uint8_t)SECTOR(ubDstSectorX, ubDstSectorY);
      pGroup->ubMoveType = ONE_WAY;
      gGarrisonGroup[iDstGarrisonID].ubPendingGroupID = pGroup->ubGroupID;
      ubGroupSize = (uint8_t)(pGroup->pEnemyGroup->ubNumTroops + pGroup->pEnemyGroup->ubNumElites +
                              pGroup->pEnemyGroup->ubNumAdmins);

      if (ubNumExtraReinforcements) {
        pGroup->pEnemyGroup->ubPendingReinforcements = ubNumExtraReinforcements;
        MoveSAIGroupToSector(&pGroup, gGarrisonGroup[iDstGarrisonID].ubSectorID, STAGE, STAGING);
      } else {
        MoveSAIGroupToSector(&pGroup, gGarrisonGroup[iDstGarrisonID].ubSectorID, STAGE,
                             REINFORCEMENTS);
      }

      ValidateWeights(19);
      return;
    }
  }
  ValidateWeights(20);
}

static void SendReinforcementsForPatrol(int32_t iPatrolID, GROUP **pOptionalGroup) {
  GROUP *pGroup;
  int32_t iRandom, iSrcGarrisonID, iWeight;
  int32_t iReinforcementsAvailable, iReinforcementsRequested, iReinforcementsApproved;
  uint8_t ubSrcSectorX, ubSrcSectorY;

  ValidateWeights(21);

  PATROL_GROUP *const pg = &gPatrolGroup[iPatrolID];

  // Determine how many units the patrol group needs.
  iReinforcementsRequested = PatrolReinforcementsRequested(pg);

  if (iReinforcementsRequested <= 0) return;

  uint8_t const ubDstSectorX = (pg->ubSectorID[1] % 16) + 1;
  uint8_t const ubDstSectorY = (pg->ubSectorID[1] / 16) + 1;

  if (pOptionalGroup && *pOptionalGroup) {  // This group will provide the reinforcements
    pGroup = *pOptionalGroup;

    pg->ubPendingGroupID = pGroup->ubGroupID;

    MoveSAIGroupToSector(pOptionalGroup, pg->ubSectorID[1], EVASIVE, REINFORCEMENTS);

    ValidateWeights(22);
    return;
  }
  iRandom = Random(giReinforcementPoints + giReinforcementPool);
  if (iRandom < giReinforcementPool) {  // use the pool and send the requested
                                        // amount from SECTOR P3 (queen's palace)
    iReinforcementsApproved = std::min(iReinforcementsRequested, giReinforcementPool);
    if (!iReinforcementsApproved) {
      iReinforcementsApproved = iReinforcementsApproved;
    }
    pGroup = CreateNewEnemyGroupDepartingFromSector(SEC_P3, 0, (uint8_t)iReinforcementsApproved, 0);
    pGroup->ubOriginalSector = (uint8_t)SECTOR(ubDstSectorX, ubDstSectorY);
    giReinforcementPool -= iReinforcementsApproved;

    pg->ubPendingGroupID = pGroup->ubGroupID;

    MoveSAIGroupToSector(&pGroup, pg->ubSectorID[1], EVASIVE, REINFORCEMENTS);

    ValidateWeights(23);
    return;
  } else {
    iRandom -= giReinforcementPool;
    for (iSrcGarrisonID = 0; iSrcGarrisonID < giGarrisonArraySize;
         iSrcGarrisonID++) {  // go through the garrisons
      RecalculateGarrisonWeight(iSrcGarrisonID);
      iWeight = -gGarrisonGroup[iSrcGarrisonID].bWeight;
      if (iWeight > 0) {          // if group is able to provide reinforcements.
        if (iRandom < iWeight) {  // This is the group that gets the reinforcements!
          ubSrcSectorX = (uint8_t)SECTORX(gGarrisonGroup[iSrcGarrisonID].ubSectorID);
          ubSrcSectorY = (uint8_t)SECTORY(gGarrisonGroup[iSrcGarrisonID].ubSectorID);
          if (ubSrcSectorX != gWorldSectorX || ubSrcSectorY != gWorldSectorY ||
              gbWorldSectorZ > 0) {  // The reinforcements aren't coming from the
                                     // currently loaded sector!
            iReinforcementsAvailable = ReinforcementsAvailable(iSrcGarrisonID);
            // Send the lowest of the two:  number requested or number available
            iReinforcementsApproved = std::min(iReinforcementsRequested, iReinforcementsAvailable);
            pGroup = CreateNewEnemyGroupDepartingFromSector(
                gGarrisonGroup[iSrcGarrisonID].ubSectorID, 0, (uint8_t)iReinforcementsApproved, 0);
            pGroup->ubOriginalSector = (uint8_t)SECTOR(ubDstSectorX, ubDstSectorY);
            pg->ubPendingGroupID = pGroup->ubGroupID;

            RemoveSoldiersFromGarrisonBasedOnComposition(iSrcGarrisonID, pGroup->ubGroupSize);

            MoveSAIGroupToSector(&pGroup, pg->ubSectorID[1], EVASIVE, REINFORCEMENTS);

            ValidateWeights(24);

            return;
          }
        }
        iRandom -= iWeight;
      }
    }
  }
  ValidateWeights(25);
}

static void EvolveQueenPriorityPhase(BOOLEAN fForceChange);
static BOOLEAN GarrisonRequestingMinimumReinforcements(int32_t iGarrisonID);
static BOOLEAN PatrolRequestingMinimumReinforcements(int32_t iPatrolID);
static void UpgradeAdminsToTroops();

// Periodically does a general poll and check on each of the groups and
// garrisons, determines reinforcements, new patrol groups, planned assaults,
// etc.
void EvaluateQueenSituation() {
  int32_t i, iRandom;
  int32_t iWeight;
  uint32_t uiOffset;
  uint16_t usDefencePoints;
  int32_t iSumOfAllWeights = 0;

  ValidateWeights(26);

  // figure out how long it shall be before we call this again

  // The more work to do there is (request points the queen's army is asking
  // for), the more often she will make decisions This can increase the decision
  // intervals by up to 500 extra minutes (> 8 hrs)
  uiOffset = std::max(100 - giRequestPoints, 0);
  uiOffset = uiOffset + Random(uiOffset * 4);
  switch (gGameOptions.ubDifficultyLevel) {
    case DIF_LEVEL_EASY:
      uiOffset += EASY_TIME_EVALUATE_IN_MINUTES + Random(EASY_TIME_EVALUATE_VARIANCE);
      break;
    case DIF_LEVEL_MEDIUM:
      uiOffset += NORMAL_TIME_EVALUATE_IN_MINUTES + Random(NORMAL_TIME_EVALUATE_VARIANCE);
      break;
    case DIF_LEVEL_HARD:
      uiOffset += HARD_TIME_EVALUATE_IN_MINUTES + Random(HARD_TIME_EVALUATE_VARIANCE);
      break;
  }

  if (!giReinforcementPool) {  // Queen has run out of reinforcements.  Simulate
                               // recruiting and training new troops
    uiOffset *= 10;
    giReinforcementPool += 30;
    AddStrategicEvent(EVENT_EVALUATE_QUEEN_SITUATION, GetWorldTotalMin() + uiOffset, 0);
    return;
  }

  // Re-post the event
  AddStrategicEvent(EVENT_EVALUATE_QUEEN_SITUATION, GetWorldTotalMin() + uiOffset, 0);

  // if the queen hasn't been alerted to player's presence yet
  if (!gfQueenAIAwake) {  // no decisions can be made yet.
    return;
  }

  // Adjust queen's disposition based on player's progress
  EvolveQueenPriorityPhase(FALSE);

  // Gradually promote any remaining admins into troops
  UpgradeAdminsToTroops();

  if ((giRequestPoints <= 0) || ((giReinforcementPoints <= 0) &&
                                 (giReinforcementPool <= 0))) {  // we either have no reinforcements
                                                                 // or request for reinforcements.
    return;
  }

  // now randomly choose who gets the reinforcements.
  // giRequestPoints is the combined sum of all the individual weights of all
  // garrisons and patrols requesting reinforcements
  iRandom = Random(giRequestPoints);

  // go through garrisons first
  for (i = 0; i < giGarrisonArraySize; i++) {
    RecalculateGarrisonWeight(i);
    iWeight = gGarrisonGroup[i].bWeight;
    if (iWeight > 0) {  // if group is requesting reinforcements.

      iSumOfAllWeights += iWeight;  // debug only!

      if (iRandom < iWeight && !gGarrisonGroup[i].ubPendingGroupID &&
          EnemyPermittedToAttackSector(NULL, gGarrisonGroup[i].ubSectorID) &&
          GarrisonRequestingMinimumReinforcements(
              i)) {  // This is the group that gets the reinforcements!
        if (ReinforcementsApproved(i, &usDefencePoints)) {
          SendReinforcementsForGarrison(i, usDefencePoints, NULL);
        }
        return;
      }
      iRandom -= iWeight;
    }
  }

  // go through the patrol groups
  for (i = 0; i < giPatrolArraySize; i++) {
    RecalculatePatrolWeight(gPatrolGroup[i]);
    iWeight = gPatrolGroup[i].bWeight;
    if (iWeight > 0) {
      iSumOfAllWeights += iWeight;  // debug only!

      if (iRandom < iWeight && !gPatrolGroup[i].ubPendingGroupID &&
          PatrolRequestingMinimumReinforcements(
              i)) {  // This is the group that gets the reinforcements!
        SendReinforcementsForPatrol(i, NULL);
        return;
      }
      iRandom -= iWeight;
    }
  }

  ValidateWeights(27);
}

void SaveStrategicAI(HWFILE const hFile) {
  GARRISON_GROUP gTempGarrisonGroup;
  PATROL_GROUP gTempPatrolGroup;
  ARMY_COMPOSITION gTempArmyComp;
  int32_t i;

  memset(&gTempPatrolGroup, 0, sizeof(PATROL_GROUP));
  memset(&gTempArmyComp, 0, sizeof(ARMY_COMPOSITION));

  FileSeek(hFile, 3, FILE_SEEK_FROM_CURRENT);
  FileWrite(hFile, &gfExtraElites, 1);
  FileWrite(hFile, &giGarrisonArraySize, 4);
  FileWrite(hFile, &giPatrolArraySize, 4);
  FileWrite(hFile, &giReinforcementPool, 4);
  FileWrite(hFile, &giForcePercentage, 4);
  FileWrite(hFile, &giArmyAlertness, 4);
  FileWrite(hFile, &giArmyAlertnessDecay, 4);
  FileWrite(hFile, &gfQueenAIAwake, 1);
  FileWrite(hFile, &giReinforcementPoints, 4);
  FileWrite(hFile, &giRequestPoints, 4);
  FileWrite(hFile, &gubNumAwareBattles, 1);
  FileWrite(hFile, &gubSAIVersion, 1);
  FileWrite(hFile, &gubQueenPriorityPhase, 1);
  FileWrite(hFile, &gfFirstBattleMeanwhileScenePending, 1);
  FileWrite(hFile, &gfMassFortificationOrdered, 1);
  FileWrite(hFile, &gubMinEnemyGroupSize, 1);
  FileWrite(hFile, &gubHoursGracePeriod, 1);
  FileWrite(hFile, &gusPlayerBattleVictories, 2);
  FileWrite(hFile, &gfUseAlternateQueenPosition, 1);
  FileWrite(hFile, gbPadding, SAI_PADDING_BYTES);
  // Save the army composition (which does get modified)
  FileWrite(hFile, gArmyComp, NUM_ARMY_COMPOSITIONS * sizeof(ARMY_COMPOSITION));
  i = SAVED_ARMY_COMPOSITIONS - NUM_ARMY_COMPOSITIONS;
  while (i--) {
    FileWrite(hFile, &gTempArmyComp, sizeof(ARMY_COMPOSITION));
  }
  // Save the patrol group definitions
  if (giPatrolArraySize != 0)
    FileWrite(hFile, gPatrolGroup, giPatrolArraySize * sizeof(PATROL_GROUP));
  i = SAVED_PATROL_GROUPS - giPatrolArraySize;
  while (i--) {
    FileWrite(hFile, &gTempPatrolGroup, sizeof(PATROL_GROUP));
  }
  // Save the garrison information!
  memset(&gTempGarrisonGroup, 0, sizeof(GARRISON_GROUP));
  if (giGarrisonArraySize != 0)
    FileWrite(hFile, gGarrisonGroup, giGarrisonArraySize * sizeof(GARRISON_GROUP));
  i = SAVED_GARRISON_GROUPS - giGarrisonArraySize;
  while (i--) {
    FileWrite(hFile, &gTempGarrisonGroup, sizeof(GARRISON_GROUP));
  }

  FileWrite(hFile, gubPatrolReinforcementsDenied, giPatrolArraySize);

  FileWrite(hFile, gubGarrisonReinforcementsDenied, giGarrisonArraySize);
}

static void ReinitializeUnvisitedGarrisons();

void LoadStrategicAI(HWFILE const hFile) {
  GARRISON_GROUP gTempGarrisonGroup;
  PATROL_GROUP gTempPatrolGroup;
  ARMY_COMPOSITION gTempArmyComp;
  int32_t i;
  uint8_t ubSAIVersion;

  FileSeek(hFile, 3, FILE_SEEK_FROM_CURRENT);
  FileRead(hFile, &gfExtraElites, 1);
  FileRead(hFile, &giGarrisonArraySize, 4);
  FileRead(hFile, &giPatrolArraySize, 4);
  FileRead(hFile, &giReinforcementPool, 4);
  FileRead(hFile, &giForcePercentage, 4);
  FileRead(hFile, &giArmyAlertness, 4);
  FileRead(hFile, &giArmyAlertnessDecay, 4);
  FileRead(hFile, &gfQueenAIAwake, 1);
  FileRead(hFile, &giReinforcementPoints, 4);
  FileRead(hFile, &giRequestPoints, 4);
  FileRead(hFile, &gubNumAwareBattles, 1);
  FileRead(hFile, &ubSAIVersion, 1);
  FileRead(hFile, &gubQueenPriorityPhase, 1);
  FileRead(hFile, &gfFirstBattleMeanwhileScenePending, 1);
  FileRead(hFile, &gfMassFortificationOrdered, 1);
  FileRead(hFile, &gubMinEnemyGroupSize, 1);
  FileRead(hFile, &gubHoursGracePeriod, 1);
  FileRead(hFile, &gusPlayerBattleVictories, 2);
  FileRead(hFile, &gfUseAlternateQueenPosition, 1);
  FileRead(hFile, gbPadding, SAI_PADDING_BYTES);
  // Restore the army composition
  FileRead(hFile, gArmyComp, NUM_ARMY_COMPOSITIONS * sizeof(ARMY_COMPOSITION));
  i = SAVED_ARMY_COMPOSITIONS - NUM_ARMY_COMPOSITIONS;
  while (i--) {
    FileRead(hFile, &gTempArmyComp, sizeof(ARMY_COMPOSITION));
  }

  // Restore the patrol group definitions
  if (gPatrolGroup) {
    MemFree(gPatrolGroup);
  }
  gPatrolGroup = MALLOCN(PATROL_GROUP, giPatrolArraySize);
  FileRead(hFile, gPatrolGroup, giPatrolArraySize * sizeof(PATROL_GROUP));
  i = SAVED_PATROL_GROUPS - giPatrolArraySize;
  while (i--) {
    FileRead(hFile, &gTempPatrolGroup, sizeof(PATROL_GROUP));
  }

  gubSAIVersion = SAI_VERSION;
  // Load the garrison information!
  if (gGarrisonGroup) {
    MemFree(gGarrisonGroup);
  }
  gGarrisonGroup = MALLOCN(GARRISON_GROUP, giGarrisonArraySize);
  FileRead(hFile, gGarrisonGroup, giGarrisonArraySize * sizeof(GARRISON_GROUP));
  i = SAVED_GARRISON_GROUPS - giGarrisonArraySize;
  while (i--) {
    FileRead(hFile, &gTempGarrisonGroup, sizeof(GARRISON_GROUP));
  }

  // Load the list of reinforcement patrol points.
  if (gubPatrolReinforcementsDenied) {
    MemFree(gubPatrolReinforcementsDenied);
    gubPatrolReinforcementsDenied = NULL;
  }
  gubPatrolReinforcementsDenied = MALLOCN(uint8_t, giPatrolArraySize);
  FileRead(hFile, gubPatrolReinforcementsDenied, giPatrolArraySize);

  // Load the list of reinforcement garrison points.
  if (gubGarrisonReinforcementsDenied) {
    MemFree(gubGarrisonReinforcementsDenied);
    gubGarrisonReinforcementsDenied = NULL;
  }
  gubGarrisonReinforcementsDenied = MALLOCN(uint8_t, giGarrisonArraySize);
  FileRead(hFile, gubGarrisonReinforcementsDenied, giGarrisonArraySize);

  if (ubSAIVersion < 6) {  // Reinitialize the costs since they have changed.

    // Recreate the compositions
    memcpy(gArmyComp, gOrigArmyComp, NUM_ARMY_COMPOSITIONS * sizeof(ARMY_COMPOSITION));
    EvolveQueenPriorityPhase(TRUE);

    // Recreate the patrol desired sizes
    for (i = 0; i < giPatrolArraySize; i++) {
      gPatrolGroup[i].bSize = gOrigPatrolGroup[i].bSize;
    }
  }
  if (ubSAIVersion < 7) {
    BuildUndergroundSectorInfoList();
  }
  if (ubSAIVersion < 8) {
    ReinitializeUnvisitedGarrisons();
  }
  if (ubSAIVersion < 10) {
    for (i = 0; i < giPatrolArraySize; i++) {
      if (gPatrolGroup[i].bSize >= 16) {
        gPatrolGroup[i].bSize = 10;
      }
    }
    FOR_EACH_ENEMY_GROUP(pGroup) {
      if (pGroup->ubGroupSize >= 16) {  // accident in patrol groups being too large
        uint8_t ubGetRidOfXTroops = pGroup->ubGroupSize - 10;
        if (gbWorldSectorZ || pGroup->ubSectorX != gWorldSectorX ||
            pGroup->ubSectorY != gWorldSectorY) {  // don't modify groups in the
                                                   // currently loaded sector.
          if (pGroup->pEnemyGroup->ubNumTroops >= ubGetRidOfXTroops) {
            pGroup->pEnemyGroup->ubNumTroops -= ubGetRidOfXTroops;
            pGroup->ubGroupSize -= ubGetRidOfXTroops;
          }
        }
      }
    }
  }
  if (ubSAIVersion < 13) {
    for (i = 0; i < 255; i++) {
      SectorInfo[i].bBloodCatPlacements = 0;
      SectorInfo[i].bBloodCats = -1;
    }
    InitBloodCatSectors();
    // This info is used to clean up the two new codes inserted into the
    // middle of the enumeration for battle codes.
    if (gubEnemyEncounterCode > CREATURE_ATTACK_CODE) {
      gubEnemyEncounterCode += 2;
    }
    if (gubExplicitEnemyEncounterCode > CREATURE_ATTACK_CODE) {
      gubExplicitEnemyEncounterCode += 2;
    }
  }
  if (ubSAIVersion < 14) {
    UNDERGROUND_SECTORINFO *pSector;
    pSector = FindUnderGroundSector(4, 11, 1);
    if (pSector->ubNumTroops + pSector->ubNumElites > 20) {
      pSector->ubNumTroops -= 2;
    }
    pSector = FindUnderGroundSector(3, 15, 1);
    if (pSector->ubNumTroops + pSector->ubNumElites > 20) {
      pSector->ubNumTroops -= 2;
    }
  }
  if (ubSAIVersion < 16) {
    UNDERGROUND_SECTORINFO *pSector;
    pSector = FindUnderGroundSector(3, 15, 1);
    if (pSector) {
      pSector->ubAdjacentSectors |= SOUTH_ADJACENT_SECTOR;
    }
    pSector = FindUnderGroundSector(3, 16, 1);
    if (pSector) {
      pSector->ubAdjacentSectors |= NORTH_ADJACENT_SECTOR;
    }
  }
  if (ubSAIVersion < 17) {  // Patch all groups that have this flag set
    gubNumGroupsArrivedSimultaneously = 0;
    {
      FOR_EACH_GROUP(pGroup) {
        if (pGroup->uiFlags & GROUPFLAG_GROUP_ARRIVED_SIMULTANEOUSLY) {
          pGroup->uiFlags &= ~GROUPFLAG_GROUP_ARRIVED_SIMULTANEOUSLY;
        }
      }
    }
  }
  if (ubSAIVersion < 18) {  // adjust down the number of bloodcats based on
                            // difficulty in the two special bloodcat levels
    switch (gGameOptions.ubDifficultyLevel) {
      case DIF_LEVEL_EASY:  // 50%
        SectorInfo[SEC_I16].bBloodCatPlacements = 14;
        SectorInfo[SEC_N5].bBloodCatPlacements = 13;
        SectorInfo[SEC_I16].bBloodCats = 14;
        SectorInfo[SEC_N5].bBloodCats = 13;
        break;
      case DIF_LEVEL_MEDIUM:  // 75%
        SectorInfo[SEC_I16].bBloodCatPlacements = 19;
        SectorInfo[SEC_N5].bBloodCatPlacements = 18;
        SectorInfo[SEC_I16].bBloodCats = 19;
        SectorInfo[SEC_N5].bBloodCats = 18;
        break;
      case DIF_LEVEL_HARD:  // 100%
        SectorInfo[SEC_I16].bBloodCatPlacements = 26;
        SectorInfo[SEC_N5].bBloodCatPlacements = 25;
        SectorInfo[SEC_I16].bBloodCats = 26;
        SectorInfo[SEC_N5].bBloodCats = 25;
        break;
    }
  }
  if (ubSAIVersion < 19) {
    // Clear the garrison in C5
    gArmyComp[gGarrisonGroup[SectorInfo[SEC_C5].ubGarrisonID].ubComposition].bPriority = 0;
    gArmyComp[gGarrisonGroup[SectorInfo[SEC_C5].ubGarrisonID].ubComposition].bDesiredPopulation = 0;
  }
  if (ubSAIVersion < 20) {
    gArmyComp[QUEEN_DEFENCE].bDesiredPopulation = 32;
    SectorInfo[SEC_P3].ubNumElites = 32;
  }
  if (ubSAIVersion < 21) {
    FOR_EACH_GROUP(pGroup) pGroup->uiFlags = 0;
  }
  if (ubSAIVersion < 22) {  // adjust down the number of bloodcats based on
                            // difficulty in the two special bloodcat levels
    switch (gGameOptions.ubDifficultyLevel) {
      case DIF_LEVEL_EASY:  // 50%
        SectorInfo[SEC_N5].bBloodCatPlacements = 8;
        SectorInfo[SEC_N5].bBloodCats = 10;
        break;
      case DIF_LEVEL_MEDIUM:  // 75%
        SectorInfo[SEC_N5].bBloodCatPlacements = 8;
        SectorInfo[SEC_N5].bBloodCats = 10;
      case DIF_LEVEL_HARD:  // 100%
        SectorInfo[SEC_N5].bBloodCatPlacements = 8;
        SectorInfo[SEC_N5].bBloodCats = 10;
        break;
    }
  }
  if (ubSAIVersion < 23) {
    if (gWorldSectorX != 3 || gWorldSectorY != 16 || !gbWorldSectorZ) {
      SectorInfo[SEC_P3].ubNumElites = 32;
    }
  }
  if (ubSAIVersion < 24) {
    // If the queen has escaped to the basement, do not use the profile
    // insertion info when we finally go down there, otherwise she will end up in
    // the wrong spot, possibly inside the walls.
    if (!gubFact[FACT_QUEEN_DEAD] && gMercProfiles[QUEEN].bSectorZ == 1) {
      if (gbWorldSectorZ != 1 || gWorldSectorX != 16 ||
          gWorldSectorY != 3) {  // We aren't in the basement sector
        gMercProfiles[QUEEN].fUseProfileInsertionInfo = FALSE;
      } else {  // We are in the basement sector, relocate queen to proper
                // position.
        FOR_EACH_IN_TEAM(i, CIV_TEAM) {
          SOLDIERTYPE &s = *i;
          if (s.ubProfile != QUEEN) continue;
          // Found queen, relocate her to 16866
          BumpAnyExistingMerc(16866);
          TeleportSoldier(s, 16866, true);
          break;
        }
      }
    }
  }
  if (ubSAIVersion < 25) {
    if (gubFact[FACT_SKYRIDER_CLOSE_TO_CHOPPER]) {
      gMercProfiles[SKYRIDER].fUseProfileInsertionInfo = FALSE;
    }
  }

  // KM : July 21, 1999 patch fix
  if (ubSAIVersion < 26) {
    int32_t i;
    for (i = 0; i < 255; i++) {
      if (SectorInfo[i].ubNumberOfCivsAtLevel[GREEN_MILITIA] +
              SectorInfo[i].ubNumberOfCivsAtLevel[REGULAR_MILITIA] +
              SectorInfo[i].ubNumberOfCivsAtLevel[ELITE_MILITIA] >
          20) {
        SectorInfo[i].ubNumberOfCivsAtLevel[GREEN_MILITIA] = 0;
        SectorInfo[i].ubNumberOfCivsAtLevel[REGULAR_MILITIA] = 20;
        SectorInfo[i].ubNumberOfCivsAtLevel[ELITE_MILITIA] = 0;
      }
    }
  }

  // KM : August 4, 1999 patch fix
  //     This addresses the problem of not having any soldiers in sector N7 when
  //     playing the game under easy difficulty.
  //		 If captured and interrogated, the player would find no soldiers
  // defending the sector.  This changes the composition
  //     so that it will always be there, and adds the soldiers accordingly if
  //     the sector isn't loaded when the update is made.
  if (ubSAIVersion < 27) {
    if (gGameOptions.ubDifficultyLevel == DIF_LEVEL_EASY) {
      if (gWorldSectorX != 7 || gWorldSectorY != 14 || gbWorldSectorZ) {
        int32_t cnt, iRandom;
        int32_t iEliteChance, iTroopChance, iAdminChance;
        int32_t iStartPop;
        SECTORINFO *pSector = NULL;

        // Change the garrison composition to LEVEL1_DEFENCE from LEVEL2_DEFENCE
        pSector = &SectorInfo[SEC_N7];
        gGarrisonGroup[pSector->ubGarrisonID].ubComposition = LEVEL1_DEFENCE;

        iStartPop = gArmyComp[gGarrisonGroup[pSector->ubGarrisonID].ubComposition].bStartPopulation;
        iEliteChance =
            gArmyComp[gGarrisonGroup[pSector->ubGarrisonID].ubComposition].bElitePercentage;
        iTroopChance =
            gArmyComp[gGarrisonGroup[pSector->ubGarrisonID].ubComposition].bTroopPercentage +
            iEliteChance;
        iAdminChance =
            gArmyComp[gGarrisonGroup[pSector->ubGarrisonID].ubComposition].bAdminPercentage;

        if (iStartPop) {
          // if population is less than maximum
          if (iStartPop != MAX_STRATEGIC_TEAM_SIZE) {
            // then vary it a bit (+/- 25%)
            iStartPop = iStartPop * (100 + (Random(51) - 25)) / 100;
          }

          iStartPop =
              std::max((int32_t)gubMinEnemyGroupSize, std::min(MAX_STRATEGIC_TEAM_SIZE, iStartPop));
          cnt = iStartPop;

          if (iAdminChance) {
            pSector->ubNumAdmins = iAdminChance * iStartPop / 100;
          } else
            while (cnt--) {  // for each person, randomly determine the types of
                             // each soldier.
              {
                iRandom = Random(100);
                if (iRandom < iEliteChance) {
                  pSector->ubNumElites++;
                } else if (iRandom < iTroopChance) {
                  pSector->ubNumTroops++;
                }
              }
            }
        }
      }
    }
  }

  if (ubSAIVersion < 28) {
    if (!StrategicMap[CALCULATE_STRATEGIC_INDEX(3, 16)]
             .fEnemyControlled) {  // Eliminate all enemy groups in this sector,
                                   // because the player owns the sector, and it
                                   // is not
      // possible for them to spawn there!
      FOR_EACH_GROUP_SAFE(i) {
        GROUP &g = *i;
        if (g.fPlayer) continue;
        if (g.ubSectorX != 3) continue;
        if (g.ubSectorY != 16) continue;
        if (g.ubPrevX != 0) continue;
        if (g.ubPrevY != 0) continue;
        RemoveGroupFromStrategicAILists(g);
        RemoveGroup(g);
      }
    }
  }
  if (ubSAIVersion < 29) {
    InitStrategicMovementCosts();
  }

  // KM : Aug 11, 1999 -- Patch fix:  Blindly update the airspace control. There
  // is a bug somewhere 		 that is failing to keep this information up to date, and
  // I failed to find it.  At least this 		 will patch saves.
  UpdateAirspaceControl();

  EvolveQueenPriorityPhase(TRUE);

  // Count and correct the floating groups
  FOR_EACH_GROUP_SAFE(pGroup) {
    if (!pGroup->fPlayer) {
      if (!pGroup->fBetweenSectors) {
        if (pGroup->ubSectorX != gWorldSectorX || pGroup->ubSectorY != gWorldSectorY ||
            gbWorldSectorZ) {
          RepollSAIGroup(pGroup);
          ValidateGroup(pGroup);
        }
      }
    }
  }

  // Update the version number to the most current.
  gubSAIVersion = SAI_VERSION;

  ValidateWeights(28);
  ValidatePendingGroups();
}

// As the player's progress changes in the game, the queen will adjust her
// priorities accordingly. Basically, increasing priorities and numbers for
// sectors she owns, and lowering them.
//@@@Alex, this is tweakable.  My philosophies could be incorrect.  It might be
// better if instead of lowering priorities and numbers for towns the queen has
// lost, to instead lower the priority but increase the numbers so she would send
// larger attack forces.  This is questionable.
static void EvolveQueenPriorityPhase(BOOLEAN fForceChange) {
  int32_t i, index, num, iFactor;
  int32_t iChange, iNew, iNumSoldiers, iNumPromotions;
  SECTORINFO *pSector;
  uint8_t ubOwned[NUM_ARMY_COMPOSITIONS];
  uint8_t ubTotal[NUM_ARMY_COMPOSITIONS];
  uint8_t ubNewPhase;
  ubNewPhase = CurrentPlayerProgressPercentage() / 10;

  if (!fForceChange && ubNewPhase == gubQueenPriorityPhase) {
    return;
  }

  gubQueenPriorityPhase = ubNewPhase;

  // The phase value refers to the deviation percentage she will apply to
  // original garrison values. All sector values are evaluated to see how many of
  // those sectors are enemy controlled.  If they are controlled by her, the
  // desired number will be increased as well as the priority.  On the other
  // hand, if she doesn't own those sectors, the values will be decreased
  // instead.  All values are based off of the originals.
  memset(ubOwned, 0, NUM_ARMY_COMPOSITIONS);
  memset(ubTotal, 0, NUM_ARMY_COMPOSITIONS);

  // Record the values required to calculate the percentage of each composition
  // type that the queen controls.
  for (i = 0; i < giGarrisonArraySize; i++) {
    index = gGarrisonGroup[i].ubComposition;
    if (StrategicMap[SECTOR_INFO_TO_STRATEGIC_INDEX(gGarrisonGroup[i].ubSectorID)]
            .fEnemyControlled) {
      ubOwned[index]++;
    }
    ubTotal[index]++;
  }

  // Go through the *majority* of compositions and modify the priority/desired
  // values.
  for (i = 0; i < NUM_ARMY_COMPOSITIONS; i++) {
    switch (i) {
      case QUEEN_DEFENCE:
      case MEDUNA_DEFENCE:
      case MEDUNA_SAMSITE:
      case LEVEL1_DEFENCE:
      case LEVEL2_DEFENCE:
      case LEVEL3_DEFENCE:
      case OMERTA_WELCOME_WAGON:
      case ROADBLOCK:
        // case SANMONA_SMALL:
        // don't consider these compositions
        continue;
    }
    // If the queen owns them ALL, then she gets the maximum defensive bonus. If
    // she owns NONE, then she gets a maximum defensive penalty.  Everything else
    // lies in the middle.  The legal range is +-50.
    if (ubTotal[i]) {
      iFactor = (ubOwned[i] * 100 / ubTotal[i]) - 50;
    } else {
      iFactor = -50;
    }
    iFactor = iFactor * gubQueenPriorityPhase / 10;

    // modify priority by + or - 25% of original
    if (gArmyComp[i].bPriority) {
      num = gOrigArmyComp[i].bPriority + iFactor / 2;
      num = std::min(std::max(0, num), 100);
      gArmyComp[i].bPriority = (int8_t)num;
    }

    // modify desired population by + or - 50% of original population
    num = gOrigArmyComp[i].bDesiredPopulation * (100 + iFactor) / 100;
    num = std::min(std::max(6, num), MAX_STRATEGIC_TEAM_SIZE);
    gArmyComp[i].bDesiredPopulation = (int8_t)num;

    // if gfExtraElites is set, then augment the composition sizes
    if (gfExtraElites && iFactor >= 15 && gArmyComp[i].bElitePercentage) {
      iChange = gGameOptions.ubDifficultyLevel * 5;

      // increase elite % (max 100)
      iNew = gArmyComp[i].bElitePercentage + iChange;
      iNew = (int8_t)std::min(100, iNew);
      gArmyComp[i].bElitePercentage = (int8_t)iNew;

      // decrease troop % (min 0)
      iNew = gArmyComp[i].bTroopPercentage - iChange;
      iNew = (int8_t)std::max(0, iNew);
      gArmyComp[i].bTroopPercentage = (int8_t)iNew;
    }
  }
  if (gfExtraElites) {
    // Turn off the flag so that this doesn't happen everytime this function is
    // called!
    gfExtraElites = FALSE;

    for (i = 0; i < giGarrisonArraySize; i++) {
      // if we are dealing with extra elites, then augment elite compositions
      // (but only if they exist in the sector). If the queen still owns the town
      // by more than 65% (iFactor >= 15), then upgrade troops to elites in those
      // sectors.
      index = gGarrisonGroup[i].ubComposition;
      switch (index) {
        case QUEEN_DEFENCE:
        case MEDUNA_DEFENCE:
        case MEDUNA_SAMSITE:
        case LEVEL1_DEFENCE:
        case LEVEL2_DEFENCE:
        case LEVEL3_DEFENCE:
        case OMERTA_WELCOME_WAGON:
        case ROADBLOCK:
          // case SANMONA_SMALL:
          // don't consider these compositions
          continue;
      }
      pSector = &SectorInfo[gGarrisonGroup[i].ubSectorID];
      if (ubTotal[index]) {
        iFactor = (ubOwned[index] * 100 / ubTotal[index]) - 50;
      } else {
        iFactor = -50;
      }
      if (iFactor >= 15) {  // Make the actual elites in sector match the new
                            // garrison percentage
        if (!gfWorldLoaded || gbWorldSectorZ ||
            gWorldSectorX != SECTORX(gGarrisonGroup[i].ubSectorID) ||
            gWorldSectorY != SECTORY(gGarrisonGroup[i].ubSectorID)) {  // Also make sure the sector
                                                                       // isn't currently loaded!
          iNumSoldiers = pSector->ubNumAdmins + pSector->ubNumTroops + pSector->ubNumElites;
          iNumPromotions =
              gArmyComp[index].bElitePercentage * iNumSoldiers / 100 - pSector->ubNumElites;

          if (iNumPromotions > 0) {
            while (iNumPromotions--) {
              if (pSector->ubNumAdmins) {
                pSector->ubNumAdmins--;
              } else if (pSector->ubNumTroops) {
                pSector->ubNumTroops--;
              } else {
                Assert(0);
              }
              pSector->ubNumElites++;
            }
            Assert(iNumSoldiers ==
                   pSector->ubNumAdmins + pSector->ubNumTroops + pSector->ubNumElites);
          }
        }
      }
    }
  }
  // Recalculate all of the weights.
  for (i = 0; i < giGarrisonArraySize; i++) {
    RecalculateGarrisonWeight(i);
  }
}

static void RequestHighPriorityGarrisonReinforcements(int32_t iGarrisonID,
                                                      uint8_t ubSoldiersRequested);

void ExecuteStrategicAIAction(uint16_t usActionCode, int16_t sSectorX, int16_t sSectorY) {
  GROUP *pGroup, *pPendingGroup = NULL;
  SECTORINFO *pSector;
  uint8_t ubSectorID;
  uint8_t ubNumSoldiers;
  switch (usActionCode) {
    case STRATEGIC_AI_ACTION_WAKE_QUEEN:
      WakeUpQueen();
      break;

    case STRATEGIC_AI_ACTION_QUEEN_DEAD:
      gfQueenAIAwake = FALSE;
      break;

    case STRATEGIC_AI_ACTION_KINGPIN_DEAD:
      // Immediate send a small garrison to C5 (to discourage access to Tony the
      // dealer)
      /*
      for( i = 0; i < giGarrisonArraySize; i++ )
      {
              if( gGarrisonGroup[ i ].ubComposition == SANMONA_SMALL )
              {
                      //Setup the composition so from now on the queen will
      consider this an important sector
                      //to hold.
                      gArmyComp[ gGarrisonGroup[ i ].ubComposition ].bPriority =
      65; gArmyComp[ gGarrisonGroup[ i ].ubComposition ].bTroopPercentage = 100;
                      gArmyComp[ gGarrisonGroup[ i ].ubComposition
      ].bDesiredPopulation = 5; RequestHighPriorityGarrisonReinforcements( i,
      (uint8_t)(2 + Random( 4 )) ); //send 2-5 soldiers now. break;
              }
      }
      */
      break;
    case NPC_ACTION_SEND_SOLDIERS_TO_DRASSEN:
      // Send 6, 9, or 12 troops (based on difficulty) one of the Drassen sectors.
      // If nobody is there when they arrive, those troops will get reassigned.

      if (Chance(50)) {
        ubSectorID = SEC_D13;
      } else if (Chance(60)) {
        ubSectorID = SEC_B13;
      } else {
        ubSectorID = SEC_C13;
      }
      ubNumSoldiers = (uint8_t)(3 + gGameOptions.ubDifficultyLevel * 3);
      pGroup = CreateNewEnemyGroupDepartingFromSector(SEC_P3, 0, ubNumSoldiers, 0);

      if (!gGarrisonGroup[SectorInfo[ubSectorID].ubGarrisonID].ubPendingGroupID) {
        pGroup->pEnemyGroup->ubIntention = STAGE;
        gGarrisonGroup[SectorInfo[ubSectorID].ubGarrisonID].ubPendingGroupID = pGroup->ubGroupID;
      } else {  // this should never happen (but if it did, then this is the best
                // way to deal with it).
        pGroup->pEnemyGroup->ubIntention = PURSUIT;
      }
      giReinforcementPool -= ubNumSoldiers;
      giReinforcementPool = std::max(giReinforcementPool, 0);

      MoveSAIGroupToSector(&pGroup, ubSectorID, EVASIVE, pGroup->pEnemyGroup->ubIntention);

      break;
    case NPC_ACTION_SEND_SOLDIERS_TO_BATTLE_LOCATION:

      // Send 4, 8, or 12 troops (based on difficulty) to the location of the
      // first battle.  If nobody is there when they arrive, those troops will get
      // reassigned.
      ubSectorID = (uint8_t)STRATEGIC_INDEX_TO_SECTOR_INFO(sWorldSectorLocationOfFirstBattle);
      pSector = &SectorInfo[ubSectorID];
      ubNumSoldiers = (uint8_t)(gGameOptions.ubDifficultyLevel * 4);
      pGroup = CreateNewEnemyGroupDepartingFromSector(SEC_P3, 0, ubNumSoldiers, 0);
      giReinforcementPool -= ubNumSoldiers;
      giReinforcementPool = std::max(giReinforcementPool, 0);

      // Determine if the battle location actually has a garrison assignment.  If
      // so, and the following checks succeed, the enemies will be sent to attack
      // and reinforce that sector.  Otherwise, the enemies will simply check it
      // out, then leave.
      if (pSector->ubGarrisonID != NO_GARRISON) {  // sector has a garrison
        if (!NumEnemiesInSector((int16_t)SECTORX(ubSectorID),
                                (int16_t)SECTORY(ubSectorID))) {  // no enemies are here
          if (gArmyComp[!gGarrisonGroup[pSector->ubGarrisonID].ubComposition]
                  .bPriority) {  // the garrison is important
            if (!gGarrisonGroup[pSector->ubGarrisonID]
                     .ubPendingGroupID) {  // the garrison doesn't have
                                           // reinforcements already on route.
              gGarrisonGroup[pSector->ubGarrisonID].ubPendingGroupID = pGroup->ubGroupID;
              MoveSAIGroupToSector(&pGroup, ubSectorID, STAGE, REINFORCEMENTS);
              break;
            }
          }
        }
      } else {
        MoveSAIGroupToSector(&pGroup, ubSectorID, EVASIVE, PURSUIT);
      }

      break;
    case NPC_ACTION_SEND_SOLDIERS_TO_OMERTA:
      ubNumSoldiers =
          (uint8_t)(gGameOptions.ubDifficultyLevel * 6);  // 6, 12, or 18 based on difficulty.
      pGroup = CreateNewEnemyGroupDepartingFromSector(
          SEC_P3, 0, ubNumSoldiers,
          (uint8_t)(ubNumSoldiers / 7));  // add 1 elite to normal, and 2 for hard
      ubNumSoldiers = (uint8_t)(ubNumSoldiers + ubNumSoldiers / 7);
      giReinforcementPool -= ubNumSoldiers;
      giReinforcementPool = std::max(giReinforcementPool, 0);
      if (PlayerMercsInSector(9, 1, 1) && !PlayerMercsInSector(10, 1, 1) &&
          !PlayerMercsInSector(10, 1,
                               2)) {  // send to A9 (if mercs in A9, but not in A10 or A10 basement)
        ubSectorID = SEC_A9;
      } else {  // send to A10
        ubSectorID = SEC_A10;
      }

      MoveSAIGroupToSector(&pGroup, ubSectorID, EVASIVE, PURSUIT);

      ValidateGroup(pGroup);
      break;
    case NPC_ACTION_SEND_TROOPS_TO_SAM:
      ubSectorID = (uint8_t)SECTOR(sSectorX, sSectorY);
      ubNumSoldiers =
          (uint8_t)(3 + gGameOptions.ubDifficultyLevel + HighestPlayerProgressPercentage() / 15);
      giReinforcementPool -= ubNumSoldiers;
      giReinforcementPool = std::max(giReinforcementPool, 0);
      pGroup = CreateNewEnemyGroupDepartingFromSector(SEC_P3, 0, 0, ubNumSoldiers);
      MoveSAIGroupToSector(&pGroup, ubSectorID, STAGE, REINFORCEMENTS);

      if (gGarrisonGroup[SectorInfo[ubSectorID].ubGarrisonID]
              .ubPendingGroupID) {  // Clear the pending group's assignment.
        pPendingGroup =
            GetGroup(gGarrisonGroup[SectorInfo[ubSectorID].ubGarrisonID].ubPendingGroupID);
        Assert(pPendingGroup);
        RemoveGroupFromStrategicAILists(*pPendingGroup);
      }
      // Assign the elite squad to attack the SAM site
      pGroup->pEnemyGroup->ubIntention = REINFORCEMENTS;
      gGarrisonGroup[SectorInfo[ubSectorID].ubGarrisonID].ubPendingGroupID = pGroup->ubGroupID;

      if (pPendingGroup) {  // Reassign the pending group
        ReassignAIGroup(&pPendingGroup);
      }

      break;
    case NPC_ACTION_ADD_MORE_ELITES:
      gfExtraElites = TRUE;
      EvolveQueenPriorityPhase(TRUE);
      break;
    case NPC_ACTION_GIVE_KNOWLEDGE_OF_ALL_MERCS:
      // temporarily make the queen's forces more aware (high alert)
      switch (gGameOptions.ubDifficultyLevel) {
        case DIF_LEVEL_EASY:
          gubNumAwareBattles = EASY_NUM_AWARE_BATTLES;
          break;
        case DIF_LEVEL_MEDIUM:
          gubNumAwareBattles = NORMAL_NUM_AWARE_BATTLES;
          break;
        case DIF_LEVEL_HARD:
          gubNumAwareBattles = HARD_NUM_AWARE_BATTLES;
          break;
      }
      break;
    default:
      ScreenMsg(FONT_RED, MSG_DEBUG, L"QueenAI failed to handle action code %d.", usActionCode);
      break;
  }
}

static uint8_t RedirectEnemyGroupsMovingThroughSector(uint8_t ubSectorX, uint8_t ubSectorY);

void StrategicHandleQueenLosingControlOfSector(int16_t sSectorX, int16_t sSectorY,
                                               int16_t sSectorZ) {
  SECTORINFO *pSector;
  uint8_t ubSectorID;
  if (sSectorZ) {  // The queen doesn't care about anything happening under the
                   // ground.
    return;
  }

  if (StrategicMap[sSectorX + sSectorY * MAP_WORLD_X]
          .fEnemyControlled) {  // If the sector doesn't belong to the player,
                                // then we shouldn't be calling this function!
    SAIReportError(
        L"StrategicHandleQueenLosingControlOfSector() was called for a sector "
        L"that is internally considered to be enemy controlled.");
    return;
  }

  ubSectorID = SECTOR(sSectorX, sSectorY);
  pSector = &SectorInfo[ubSectorID];

  // Keep track of victories and wake up the queen after x number of battles.
  gusPlayerBattleVictories++;
  if (gusPlayerBattleVictories == 5 - gGameOptions.ubDifficultyLevel) {  // 4 victories for easy, 3
                                                                         // for normal, 2 for hard
    WakeUpQueen();
  }

  if (pSector->ubGarrisonID == NO_GARRISON) {  // Queen doesn't care if the sector lost wasn't a
                                               // garrison sector.
    return;
  } else {  // check to see if there are any pending reinforcements.  If so, then
            // cancel their orders and have them
    // reassigned, so the player doesn't get pestered.  This is a feature that
    // *dumbs* down the AI, and is done for the sake of gameplay.  We don't want
    // the game to be tedious.
    if (!pSector->uiTimeLastPlayerLiberated) {
      pSector->uiTimeLastPlayerLiberated = GetWorldTotalSeconds();
    } else {  // convert hours to seconds and subtract up to half of it randomly
              // "seconds - (hours*3600 / 2)"
      pSector->uiTimeLastPlayerLiberated =
          GetWorldTotalSeconds() - Random(gubHoursGracePeriod * 1800);
    }
    if (gGarrisonGroup[pSector->ubGarrisonID].ubPendingGroupID) {
      GROUP *pGroup;
      pGroup = GetGroup(gGarrisonGroup[pSector->ubGarrisonID].ubPendingGroupID);
      if (pGroup) {
        ReassignAIGroup(&pGroup);
      }
      gGarrisonGroup[pSector->ubGarrisonID].ubPendingGroupID = 0;
    }
  }

  // If there are any enemy groups that will be moving through this sector due,
  // they will have to repath which will cause them to avoid the sector.  Returns
  // the number of redirected groups.
  RedirectEnemyGroupsMovingThroughSector((uint8_t)sSectorX, (uint8_t)sSectorY);
}

static uint8_t SectorDistance(uint8_t ubSectorID1, uint8_t ubSectorID2) {
  uint8_t ubSectorX1, ubSectorX2, ubSectorY1, ubSectorY2;
  uint8_t ubDist;
  ubSectorX1 = (uint8_t)SECTORX(ubSectorID1);
  ubSectorX2 = (uint8_t)SECTORX(ubSectorID2);
  ubSectorY1 = (uint8_t)SECTORY(ubSectorID1);
  ubSectorY2 = (uint8_t)SECTORY(ubSectorID2);

  ubDist = (uint8_t)(abs(ubSectorX1 - ubSectorX2) + abs(ubSectorY1 - ubSectorY2));

  return ubDist;
}

static void RequestHighPriorityGarrisonReinforcements(int32_t iGarrisonID,
                                                      uint8_t ubSoldiersRequested) {
  int32_t i, iBestIndex;
  GROUP *pGroup;
  uint8_t ubBestDist, ubDist;
  uint8_t ubDstSectorX, ubDstSectorY;
  // AssertMsg( giPatrolArraySize == PATROL_GROUPS && giGarrisonArraySize ==
  // GARRISON_GROUPS, "Strategic AI -- Patrol and/or garrison group definition
  // mismatch." );
  ubBestDist = 255;
  iBestIndex = -1;
  for (i = 0; i < giPatrolArraySize; i++) {
    if (gPatrolGroup[i].ubGroupID) {
      pGroup = GetGroup(gPatrolGroup[i].ubGroupID);
      if (pGroup && pGroup->ubGroupSize >= ubSoldiersRequested) {
        ubDist = SectorDistance((uint8_t)SECTOR(pGroup->ubSectorX, pGroup->ubSectorY),
                                gGarrisonGroup[iGarrisonID].ubSectorID);
        if (ubDist < ubBestDist) {
          ubBestDist = ubDist;
          iBestIndex = i;
        }
      }
    }
  }
  ubDstSectorX = (uint8_t)SECTORX(gGarrisonGroup[iGarrisonID].ubSectorID);
  ubDstSectorY = (uint8_t)SECTORY(gGarrisonGroup[iGarrisonID].ubSectorID);
  if (iBestIndex != -1) {  // Send the group to the garrison
    pGroup = GetGroup(gPatrolGroup[iBestIndex].ubGroupID);
    if (pGroup->ubGroupSize > ubSoldiersRequested &&
        pGroup->ubGroupSize - ubSoldiersRequested >=
            gubMinEnemyGroupSize) {  // Split the group, and send to location
      GROUP *pNewGroup;
      pNewGroup = CreateNewEnemyGroupDepartingFromSector(
          (uint8_t)SECTOR(pGroup->ubSectorX, pGroup->ubSectorY), 0, 0, 0);
      // Transfer the troops from group to new group
      if (pGroup->pEnemyGroup->ubNumTroops >= ubSoldiersRequested) {  // All of them are troops, so
                                                                      // do it in one shot.
        pGroup->pEnemyGroup->ubNumTroops -= ubSoldiersRequested;
        pGroup->ubGroupSize -= ubSoldiersRequested;
        pNewGroup->pEnemyGroup->ubNumTroops = ubSoldiersRequested;
        pNewGroup->ubGroupSize += ubSoldiersRequested;
        ValidateLargeGroup(pGroup);
        ValidateLargeGroup(pNewGroup);
      } else
        while (ubSoldiersRequested) {  // There aren't enough troops, so transfer
                                       // other types when we run out of troops,
                                       // prioritizing admins, then elites.
          if (pGroup->pEnemyGroup->ubNumTroops) {
            pGroup->pEnemyGroup->ubNumTroops--;
            pGroup->ubGroupSize--;
            pNewGroup->pEnemyGroup->ubNumTroops++;
            pNewGroup->ubGroupSize++;
            ubSoldiersRequested--;
            ValidateLargeGroup(pGroup);
            ValidateLargeGroup(pNewGroup);
          } else if (pGroup->pEnemyGroup->ubNumAdmins) {
            pGroup->pEnemyGroup->ubNumAdmins--;
            pGroup->ubGroupSize--;
            pNewGroup->pEnemyGroup->ubNumAdmins++;
            pNewGroup->ubGroupSize++;
            ubSoldiersRequested--;
            ValidateLargeGroup(pGroup);
            ValidateLargeGroup(pNewGroup);
          } else if (pGroup->pEnemyGroup->ubNumElites) {
            pGroup->pEnemyGroup->ubNumElites--;
            pGroup->ubGroupSize--;
            pNewGroup->pEnemyGroup->ubNumElites++;
            pNewGroup->ubGroupSize++;
            ubSoldiersRequested--;
            ValidateLargeGroup(pGroup);
            ValidateLargeGroup(pNewGroup);
          } else {
            AssertMsg(0, "Strategic AI group transfer error.  KM : 0");
            return;
          }
        }
      pNewGroup->ubOriginalSector = (uint8_t)SECTOR(ubDstSectorX, ubDstSectorY);
      gGarrisonGroup[iGarrisonID].ubPendingGroupID = pNewGroup->ubGroupID;
      RecalculatePatrolWeight(gPatrolGroup[iBestIndex]);

      MoveSAIGroupToSector(&pNewGroup, gGarrisonGroup[iGarrisonID].ubSectorID, EVASIVE,
                           REINFORCEMENTS);
    } else {  // Send the whole group and kill it's patrol assignment.
      gPatrolGroup[iBestIndex].ubGroupID = 0;
      gGarrisonGroup[iGarrisonID].ubPendingGroupID = pGroup->ubGroupID;
      pGroup->ubOriginalSector = (uint8_t)SECTOR(ubDstSectorX, ubDstSectorY);
      RecalculatePatrolWeight(gPatrolGroup[iBestIndex]);
      // The ONLY case where the group is told to move somewhere else when they
      // could be BETWEEN sectors.  The movegroup functions don't work if this is
      // the case.  Teleporting them to their previous sector is the best and
      // easiest way to deal with this.
      SetEnemyGroupSector(*pGroup, SECTOR(pGroup->ubSectorX, pGroup->ubSectorY));

      MoveSAIGroupToSector(&pGroup, gGarrisonGroup[iGarrisonID].ubSectorID, EVASIVE,
                           REINFORCEMENTS);
      ValidateGroup(pGroup);
    }
  } else {  // There are no groups that have enough troops.  Send a new force
            // from the palace instead.
    pGroup = CreateNewEnemyGroupDepartingFromSector(SEC_P3, 0, ubSoldiersRequested, 0);
    pGroup->ubMoveType = ONE_WAY;
    pGroup->pEnemyGroup->ubIntention = REINFORCEMENTS;
    gGarrisonGroup[iGarrisonID].ubPendingGroupID = pGroup->ubGroupID;
    pGroup->ubOriginalSector = (uint8_t)SECTOR(ubDstSectorX, ubDstSectorY);
    giReinforcementPool -= (int32_t)ubSoldiersRequested;

    MoveSAIGroupToSector(&pGroup, gGarrisonGroup[iGarrisonID].ubSectorID, EVASIVE, REINFORCEMENTS);
    ValidateGroup(pGroup);
  }
}

static void MassFortifyTowns();

void WakeUpQueen() {
  gfQueenAIAwake = TRUE;
  if (!gfMassFortificationOrdered) {
    gfMassFortificationOrdered = TRUE;
    MassFortifyTowns();
  }
}

// Simply orders all garrisons to take troops from the patrol groups and send
// the closest troops from them.  Any garrison, whom there request isn't
// fulfilled (due to lack of troops), will recieve their reinforcements from the
// queen (P3).
static void MassFortifyTowns() {
  int32_t i;
  SECTORINFO *pSector;
  GROUP *pGroup;
  uint8_t ubNumTroops, ubDesiredTroops;
  for (i = 0; i < giGarrisonArraySize; i++) {
    pSector = &SectorInfo[gGarrisonGroup[i].ubSectorID];
    ubNumTroops = pSector->ubNumAdmins + pSector->ubNumTroops + pSector->ubNumElites;
    ubDesiredTroops = (uint8_t)gArmyComp[gGarrisonGroup[i].ubComposition].bDesiredPopulation;
    if (ubNumTroops < ubDesiredTroops) {
      if (!gGarrisonGroup[i].ubPendingGroupID && gGarrisonGroup[i].ubComposition != ROADBLOCK &&
          EnemyPermittedToAttackSector(NULL, gGarrisonGroup[i].ubSectorID)) {
        RequestHighPriorityGarrisonReinforcements(i, (uint8_t)(ubDesiredTroops - ubNumTroops));
      }
    }
  }
  // Convert the garrison sitting in Omerta (if alive), and reassign them
  pSector = &SectorInfo[SEC_A9];
  if (pSector->ubNumTroops) {
    pGroup = CreateNewEnemyGroupDepartingFromSector(SEC_A9, 0, pSector->ubNumTroops, 0);
    pSector->ubNumTroops = 0;
    pGroup->pEnemyGroup->ubIntention = PATROL;
    pGroup->ubMoveType = ONE_WAY;
    ReassignAIGroup(&pGroup);
    ValidateGroup(pGroup);
    RecalculateSectorWeight(SEC_A9);
  }
}

static void RenderAIViewerGarrisonInfo(int32_t x, int32_t y, SECTORINFO *pSector) {
  if (pSector->ubGarrisonID != NO_GARRISON) {
    int32_t iDesired, iSurplus;
    iDesired = gArmyComp[gGarrisonGroup[pSector->ubGarrisonID].ubComposition].bDesiredPopulation;
    iSurplus = pSector->ubNumTroops + pSector->ubNumAdmins + pSector->ubNumElites - iDesired;
    SetFontForeground(FONT_WHITE);
    if (iSurplus >= 0) {
      mprintf(x, y, L"%d desired, %d surplus troops", iDesired, iSurplus);
    } else {
      mprintf(x, y, L"%d desired, %d reinforcements requested", iDesired, -iSurplus);
    }
    if (gGarrisonGroup[pSector->ubGarrisonID].ubPendingGroupID) {
      GROUP *pGroup;
      pGroup = GetGroup(gGarrisonGroup[pSector->ubGarrisonID].ubPendingGroupID);
      mprintf(x, y + 10, L"%d reinforcements on route from group %d in %c%d", pGroup->ubGroupSize,
              pGroup->ubGroupID, pGroup->ubSectorY + 'A' - 1, pGroup->ubSectorX);
    } else {
      MPrint(x, y + 10, L"No pending reinforcements for this sector.");
    }
  } else {
    SetFontForeground(FONT_GRAY2);
    MPrint(x, y, L"No garrison information for this sector.");
  }
}

void StrategicHandleMineThatRanOut(uint8_t ubSectorID) {
  switch (ubSectorID) {
    case SEC_B2:
      gArmyComp[gGarrisonGroup[SectorInfo[SEC_A2].ubGarrisonID].ubComposition].bPriority /= 4;
      gArmyComp[gGarrisonGroup[SectorInfo[SEC_B2].ubGarrisonID].ubComposition].bPriority /= 4;
      break;
    case SEC_D13:
      gArmyComp[gGarrisonGroup[SectorInfo[SEC_B13].ubGarrisonID].ubComposition].bPriority /= 4;
      gArmyComp[gGarrisonGroup[SectorInfo[SEC_C13].ubGarrisonID].ubComposition].bPriority /= 4;
      gArmyComp[gGarrisonGroup[SectorInfo[SEC_D13].ubGarrisonID].ubComposition].bPriority /= 4;
      break;
    case SEC_H8:
      gArmyComp[gGarrisonGroup[SectorInfo[SEC_F8].ubGarrisonID].ubComposition].bPriority /= 4;
      gArmyComp[gGarrisonGroup[SectorInfo[SEC_F9].ubGarrisonID].ubComposition].bPriority /= 4;
      gArmyComp[gGarrisonGroup[SectorInfo[SEC_G8].ubGarrisonID].ubComposition].bPriority /= 4;
      gArmyComp[gGarrisonGroup[SectorInfo[SEC_G9].ubGarrisonID].ubComposition].bPriority /= 4;
      gArmyComp[gGarrisonGroup[SectorInfo[SEC_H8].ubGarrisonID].ubComposition].bPriority /= 4;
      break;
    case SEC_I14:
      gArmyComp[gGarrisonGroup[SectorInfo[SEC_H13].ubGarrisonID].ubComposition].bPriority /= 4;
      gArmyComp[gGarrisonGroup[SectorInfo[SEC_H14].ubGarrisonID].ubComposition].bPriority /= 4;
      gArmyComp[gGarrisonGroup[SectorInfo[SEC_I13].ubGarrisonID].ubComposition].bPriority /= 4;
      gArmyComp[gGarrisonGroup[SectorInfo[SEC_I14].ubGarrisonID].ubComposition].bPriority /= 4;
      break;
  }
}

static BOOLEAN GarrisonCanProvideMinimumReinforcements(int32_t iGarrisonID) {
  int32_t iAvailable;
  int32_t iDesired;
  SECTORINFO *pSector;
  uint8_t ubSectorX, ubSectorY;

  pSector = &SectorInfo[gGarrisonGroup[iGarrisonID].ubSectorID];

  iAvailable = pSector->ubNumAdmins + pSector->ubNumTroops + pSector->ubNumElites;
  iDesired = gArmyComp[gGarrisonGroup[iGarrisonID].ubComposition].bDesiredPopulation;

  if (iAvailable - iDesired >= gubMinEnemyGroupSize) {
    // Do a more expensive check first to determine if there is a player
    // presence here (combat in progress) If so, do not provide reinforcements
    // from here.
    ubSectorX = (uint8_t)SECTORX(gGarrisonGroup[iGarrisonID].ubSectorID);
    ubSectorY = (uint8_t)SECTORY(gGarrisonGroup[iGarrisonID].ubSectorID);
    if (PlayerMercsInSector(ubSectorX, ubSectorY, 0) ||
        CountAllMilitiaInSector(ubSectorX, ubSectorY)) {
      return FALSE;
    }
    return TRUE;
  }
  return FALSE;
}

static BOOLEAN GarrisonRequestingMinimumReinforcements(int32_t iGarrisonID) {
  int32_t iAvailable;
  int32_t iDesired;
  SECTORINFO *pSector;

  if (gGarrisonGroup[iGarrisonID].ubPendingGroupID) {
    return FALSE;
  }

  pSector = &SectorInfo[gGarrisonGroup[iGarrisonID].ubSectorID];
  iAvailable = pSector->ubNumAdmins + pSector->ubNumTroops + pSector->ubNumElites;
  iDesired = gArmyComp[gGarrisonGroup[iGarrisonID].ubComposition].bDesiredPopulation;

  if (iDesired - iAvailable >= gubMinEnemyGroupSize) {
    return TRUE;
  }
  return FALSE;
}

static BOOLEAN PermittedToFillPatrolGroup(int32_t iPatrolID);

static BOOLEAN PatrolRequestingMinimumReinforcements(int32_t iPatrolID) {
  GROUP *pGroup;

  if (gPatrolGroup[iPatrolID].ubPendingGroupID) {
    return FALSE;
  }
  if (!PermittedToFillPatrolGroup(
          iPatrolID)) {  // if the group was defeated, it won't be considered for
                         // reinforcements again for several days
    return FALSE;
  }
  pGroup = GetGroup(gPatrolGroup[iPatrolID].ubGroupID);
  if (pGroup) {
    if (gPatrolGroup[iPatrolID].bSize - pGroup->ubGroupSize >= gubMinEnemyGroupSize) {
      return TRUE;
    }
  }
  return FALSE;
}

static void EliminateSurplusTroopsForGarrison(GROUP *pGroup, SECTORINFO *pSector) {
  int32_t iTotal;
  iTotal = pGroup->pEnemyGroup->ubNumTroops + pGroup->pEnemyGroup->ubNumElites +
           pGroup->pEnemyGroup->ubNumAdmins + pSector->ubNumTroops + pSector->ubNumElites +
           pSector->ubNumAdmins;
  if (iTotal <= MAX_STRATEGIC_TEAM_SIZE) {
    return;
  }
  iTotal -= MAX_STRATEGIC_TEAM_SIZE;
  while (iTotal) {
    if (pGroup->pEnemyGroup->ubNumAdmins) {
      if (pGroup->pEnemyGroup->ubNumAdmins < iTotal) {
        iTotal -= pGroup->pEnemyGroup->ubNumAdmins;
        pGroup->pEnemyGroup->ubNumAdmins = 0;
      } else {
        pGroup->pEnemyGroup->ubNumAdmins -= (uint8_t)iTotal;
        iTotal = 0;
      }
    } else if (pSector->ubNumAdmins) {
      if (pSector->ubNumAdmins < iTotal) {
        iTotal -= pSector->ubNumAdmins;
        pSector->ubNumAdmins = 0;
      } else {
        pSector->ubNumAdmins -= (uint8_t)iTotal;
        iTotal = 0;
      }
    } else if (pGroup->pEnemyGroup->ubNumTroops) {
      if (pGroup->pEnemyGroup->ubNumTroops < iTotal) {
        iTotal -= pGroup->pEnemyGroup->ubNumTroops;
        pGroup->pEnemyGroup->ubNumTroops = 0;
      } else {
        pGroup->pEnemyGroup->ubNumTroops -= (uint8_t)iTotal;
        iTotal = 0;
      }
    } else if (pSector->ubNumTroops) {
      if (pSector->ubNumTroops < iTotal) {
        iTotal -= pSector->ubNumTroops;
        pSector->ubNumTroops = 0;
      } else {
        pSector->ubNumTroops -= (uint8_t)iTotal;
        iTotal = 0;
      }
    } else if (pGroup->pEnemyGroup->ubNumElites) {
      if (pGroup->pEnemyGroup->ubNumElites < iTotal) {
        iTotal -= pGroup->pEnemyGroup->ubNumElites;
        pGroup->pEnemyGroup->ubNumElites = 0;
      } else {
        pGroup->pEnemyGroup->ubNumElites -= (uint8_t)iTotal;
        iTotal = 0;
      }
    } else if (pSector->ubNumElites) {
      if (pSector->ubNumElites < iTotal) {
        iTotal -= pSector->ubNumElites;
        pSector->ubNumElites = 0;
      } else {
        pSector->ubNumElites -= (uint8_t)iTotal;
        iTotal = 0;
      }
    }
  }
}

/* Once Queen is awake, she'll gradually begin replacing admins with regular
 * troops. This is mainly to keep player from fighting many more admins once
 * they are no longer any challenge for him. Eventually all admins will vanish
 * off map. */
static void UpgradeAdminsToTroops() {
  /* On normal, AI evaluates approximately every 10 hrs. There are about
   * 130 administrators seeded on the map. Some of these will be killed by the
   * player. */

  int32_t const min_priority = 100 - 10 * HighestPlayerProgressPercentage();

  // Check all garrisons for administrators.
  uint32_t const cur = GetWorldSector();
  GARRISON_GROUP const *const end = gGarrisonGroup + giGarrisonArraySize;
  for (GARRISON_GROUP const *i = gGarrisonGroup; i != end; ++i) {
    GARRISON_GROUP const &g = *i;

    // Skip sector if it's currently loaded, we'll never upgrade guys in those.
    if (cur == g.ubSectorID) continue;

    SECTORINFO &sector = SectorInfo[g.ubSectorID];
    if (sector.ubNumAdmins == 0) continue;  // No admins in garrison.

    int8_t const priority = gArmyComp[g.ubComposition].bPriority;

    /* Highest priority sectors are upgraded first. Each 1% of progress lowers
     * the priority threshold required to start triggering upgrades by 10%. */
    if (priority <= min_priority) continue;

    for (uint8_t n_to_check = sector.ubNumAdmins; n_to_check != 0; --n_to_check) {
      /* Chance to upgrade at each check is random and is dependent on the
       * garrison's priority. */
      if (!Chance(priority)) continue;
      --sector.ubNumAdmins;
      ++sector.ubNumTroops;
    }
  }

  // Check all moving enemy groups for administrators.
  FOR_EACH_ENEMY_GROUP(i) {
    GROUP const &g = *i;
    if (g.ubGroupSize == 0) continue;
    if (g.fVehicle) continue;

    // Skip sector if it's currently loaded, we'll never upgrade guys in those.
    if (g.ubSectorX == gWorldSectorX && g.ubSectorY == gWorldSectorY) continue;

    Assert(g.pEnemyGroup);
    ENEMYGROUP &eg = *g.pEnemyGroup;
    if (eg.ubNumAdmins == 0) continue;  // No admins in group.

    int8_t priority;
    if (eg.ubIntention == PATROL) {  // Use that patrol's priority.
      int16_t const patrol_id = FindPatrolGroupIndexForGroupID(g.ubGroupID);
      Assert(patrol_id != -1);
      priority = gPatrolGroup[patrol_id].bPriority;
    } else {  // Use a default priority.
      priority = 50;
    }

    /* Highest priority groups are upgraded first. Each 1% of progress lowers
     * the priority threshold required to start triggering upgrades by 10%. */
    if (priority <= min_priority) continue;

    for (uint8_t n_to_check = eg.ubNumAdmins; n_to_check != 0; --n_to_check) {
      /* Chance to upgrade at each check is random and is dependent on the
       * group's priority. */
      if (!Chance(priority)) continue;
      --eg.ubNumAdmins;
      ++eg.ubNumTroops;
    }
  }
}

int16_t FindPatrolGroupIndexForGroupID(uint8_t ubGroupID) {
  int16_t sPatrolIndex;

  for (sPatrolIndex = 0; sPatrolIndex < giPatrolArraySize; sPatrolIndex++) {
    if (gPatrolGroup[sPatrolIndex].ubGroupID == ubGroupID) {
      // found it
      return (sPatrolIndex);
    }
  }

  // not there!
  return (-1);
}

int16_t FindPatrolGroupIndexForGroupIDPending(uint8_t ubGroupID) {
  int16_t sPatrolIndex;

  for (sPatrolIndex = 0; sPatrolIndex < giPatrolArraySize; sPatrolIndex++) {
    if (gPatrolGroup[sPatrolIndex].ubPendingGroupID == ubGroupID) {
      // found it
      return (sPatrolIndex);
    }
  }

  // not there!
  return (-1);
}

int16_t FindGarrisonIndexForGroupIDPending(uint8_t ubGroupID) {
  int16_t sGarrisonIndex;

  for (sGarrisonIndex = 0; sGarrisonIndex < giGarrisonArraySize; sGarrisonIndex++) {
    if (gGarrisonGroup[sGarrisonIndex].ubPendingGroupID == ubGroupID) {
      // found it
      return (sGarrisonIndex);
    }
  }

  // not there!
  return (-1);
}

static void TransferGroupToPool(GROUP **pGroup) {
  giReinforcementPool += (*pGroup)->ubGroupSize;
  RemoveGroup(**pGroup);
  *pGroup = NULL;
}

// NOTE:  Make sure you call SetEnemyGroupSector() first if the group is between
// sectors!!  See example in ReassignAIGroup()...
static void SendGroupToPool(GROUP **pGroup) {
  if ((*pGroup)->ubSectorX == 3 && (*pGroup)->ubSectorY == 16) {
    TransferGroupToPool(pGroup);
  } else {
    (*pGroup)->ubSectorIDOfLastReassignment =
        (uint8_t)SECTOR((*pGroup)->ubSectorX, (*pGroup)->ubSectorY);
    MoveSAIGroupToSector(pGroup, SEC_P3, EVASIVE, REINFORCEMENTS);
  }
}

static void ReassignAIGroup(GROUP **pGroup) {
  int32_t i, iRandom;
  int32_t iWeight;
  uint16_t usDefencePoints;
  int32_t iReloopLastIndex = -1;
  uint8_t ubSectorID;

  ubSectorID = (uint8_t)SECTOR((*pGroup)->ubSectorX, (*pGroup)->ubSectorY);

  (*pGroup)->ubSectorIDOfLastReassignment = ubSectorID;

  RemoveGroupFromStrategicAILists(**pGroup);

  // First thing to do, is teleport the group to be AT the sector he is
  // currently moving from.  Otherwise, the strategic pathing can break if the
  // group is between sectors upon reassignment.
  SetEnemyGroupSector(**pGroup, ubSectorID);

  if (giRequestPoints <= 0) {  // we have no request for reinforcements, so send the group to Meduna
                               // for reassignment in the pool.
    SendGroupToPool(pGroup);
    return;
  }

  // now randomly choose who gets the reinforcements.
  // giRequestPoints is the combined sum of all the individual weights of all
  // garrisons and patrols requesting reinforcements
  iRandom = Random(giRequestPoints);

  // go through garrisons first and begin considering where the random value
  // dictates.  If that garrison doesn't require reinforcements, it'll continue
  // on considering all subsequent garrisons till the end of the array.  If it
  // fails at that point, it'll restart the loop at zero, and consider all
  // garrisons to the index that was first considered by the random value.
  for (i = 0; i < giGarrisonArraySize; i++) {
    RecalculateGarrisonWeight(i);
    iWeight = gGarrisonGroup[i].bWeight;
    if (iWeight > 0) {  // if group is requesting reinforcements.
      if (iRandom < iWeight) {
        if (!gGarrisonGroup[i].ubPendingGroupID &&
            EnemyPermittedToAttackSector(NULL, gGarrisonGroup[i].ubSectorID) &&
            GarrisonRequestingMinimumReinforcements(
                i)) {  // This is the group that gets the reinforcements!
          if (ReinforcementsApproved(i, &usDefencePoints)) {
            SendReinforcementsForGarrison(i, usDefencePoints, pGroup);
            return;
          }
        }
        if (iReloopLastIndex == -1) {  // go to the next garrison and clear the iRandom value so it
                                       // attempts to use all subsequent groups.
          iReloopLastIndex = i - 1;
          iRandom = 0;
        }
      }
      // Decrease the iRandom value until it hits 0.  When that happens, all
      // garrisons will get considered until we either have a match or process
      // all of the garrisons.
      iRandom -= iWeight;
    }
  }
  if (iReloopLastIndex >= 0) {  // Process the loop again to the point where the original random
                                // slot started considering, and consider
    // all of the garrisons.  If this fails, all patrol groups will be
    // considered next.
    for (i = 0; i <= iReloopLastIndex; i++) {
      RecalculateGarrisonWeight(i);
      iWeight = gGarrisonGroup[i].bWeight;
      if (iWeight > 0) {  // if group is requesting reinforcements.
        if (!gGarrisonGroup[i].ubPendingGroupID &&
            EnemyPermittedToAttackSector(NULL, gGarrisonGroup[i].ubSectorID) &&
            GarrisonRequestingMinimumReinforcements(
                i)) {  // This is the group that gets the reinforcements!
          if (ReinforcementsApproved(i, &usDefencePoints)) {
            SendReinforcementsForGarrison(i, usDefencePoints, pGroup);
            return;
          }
        }
      }
    }
  }
  if (iReloopLastIndex == -1) {
    // go through the patrol groups
    for (i = 0; i < giPatrolArraySize; i++) {
      RecalculatePatrolWeight(gPatrolGroup[i]);
      iWeight = gPatrolGroup[i].bWeight;
      if (iWeight > 0) {
        if (iRandom < iWeight) {
          if (!gPatrolGroup[i].ubPendingGroupID &&
              PatrolRequestingMinimumReinforcements(
                  i)) {  // This is the group that gets the reinforcements!
            SendReinforcementsForPatrol(i, pGroup);
            return;
          }
        }
        if (iReloopLastIndex == -1) {
          iReloopLastIndex = i - 1;
          iRandom = 0;
        }
        iRandom -= iWeight;
      }
    }
  } else {
    iReloopLastIndex = giPatrolArraySize - 1;
  }

  for (i = 0; i <= iReloopLastIndex; i++) {
    RecalculatePatrolWeight(gPatrolGroup[i]);
    iWeight = gPatrolGroup[i].bWeight;
    if (iWeight > 0) {
      if (!gPatrolGroup[i].ubPendingGroupID &&
          PatrolRequestingMinimumReinforcements(
              i)) {  // This is the group that gets the reinforcements!
        SendReinforcementsForPatrol(i, pGroup);
        return;
      }
    }
  }
  TransferGroupToPool(pGroup);
}

/* When an enemy AI group is eliminated by the player, apply a grace period in
 * which the group isn't allowed to be filled for several days. */
static void TagSAIGroupWithGracePeriod(GROUP const &g) {
  int32_t const patrol_id = FindPatrolGroupIndexForGroupID(g.ubGroupID);
  if (patrol_id == -1) return;

  uint32_t grace_period;
  switch (gGameOptions.ubDifficultyLevel) {
    case DIF_LEVEL_EASY:
      grace_period = EASY_PATROL_GRACE_PERIOD_IN_DAYS;
      break;
    case DIF_LEVEL_MEDIUM:
      grace_period = NORMAL_PATROL_GRACE_PERIOD_IN_DAYS;
      break;
    case DIF_LEVEL_HARD:
      grace_period = HARD_PATROL_GRACE_PERIOD_IN_DAYS;
      break;
    default:
      return;
  }
  gPatrolGroup[patrol_id].bFillPermittedAfterDayMod100 = (GetWorldDay() + grace_period) % 100;
}

static BOOLEAN PermittedToFillPatrolGroup(int32_t iPatrolID) {
  int32_t iDay;
  int32_t iDayAllowed;
  iDay = GetWorldDay();
  iDayAllowed = gPatrolGroup[iPatrolID].bFillPermittedAfterDayMod100 + (iDay / 100) * 100;
  return iDay >= iDayAllowed;
}

void RepollSAIGroup(GROUP *pGroup) {
  int32_t i;
  Assert(!pGroup->fPlayer);
  if (GroupAtFinalDestination(pGroup)) {
    EvaluateGroupSituation(pGroup);
    return;
  }
  for (i = 0; i < giPatrolArraySize; i++) {
    if (gPatrolGroup[i].ubGroupID == pGroup->ubGroupID) {
      RecalculatePatrolWeight(gPatrolGroup[i]);  // in case there are any dead enemies
      CalculateNextMoveIntention(pGroup);
      return;
    }
  }
  for (i = 0; i < giGarrisonArraySize; i++) {
    // KM : August 6, 1999 Patch fix
    //     Ack, wasn't checking for the matching group to garrison
    if (gGarrisonGroup[i].ubPendingGroupID == pGroup->ubGroupID)
    // end
    {
      RecalculateGarrisonWeight(i);  // in case there are any dead enemies
      CalculateNextMoveIntention(pGroup);
      return;
    }
  }
}

static void CalcNumTroopsBasedOnComposition(uint8_t *pubNumTroops, uint8_t *pubNumElites,
                                            uint8_t ubTotal, int32_t iCompositionID) {
  *pubNumTroops = gArmyComp[iCompositionID].bTroopPercentage * ubTotal / 100;
  *pubNumElites = gArmyComp[iCompositionID].bElitePercentage * ubTotal / 100;

  // Due to low roundoff, it is highly possible that we will be short one
  // soldier.
  while (*pubNumTroops + *pubNumElites < ubTotal) {
    if (Chance(gArmyComp[iCompositionID].bTroopPercentage)) {
      (*pubNumTroops)++;
    } else {
      (*pubNumElites)++;
    }
  }
  Assert(*pubNumTroops + *pubNumElites == ubTotal);
}

static void ConvertGroupTroopsToComposition(GROUP *pGroup, int32_t iCompositionID) {
  Assert(pGroup);
  Assert(!pGroup->fPlayer);
  CalcNumTroopsBasedOnComposition(&pGroup->pEnemyGroup->ubNumTroops,
                                  &pGroup->pEnemyGroup->ubNumElites, pGroup->ubGroupSize,
                                  iCompositionID);
  pGroup->pEnemyGroup->ubNumAdmins = 0;
  pGroup->ubGroupSize = pGroup->pEnemyGroup->ubNumTroops + pGroup->pEnemyGroup->ubNumElites;
  ValidateLargeGroup(pGroup);
}

static void RemoveSoldiersFromGarrisonBasedOnComposition(int32_t iGarrisonID, uint8_t ubSize) {
  SECTORINFO *pSector;
  int32_t iCompositionID;
  uint8_t ubNumTroops, ubNumElites;

  iCompositionID = gGarrisonGroup[iGarrisonID].ubComposition;

  CalcNumTroopsBasedOnComposition(&ubNumTroops, &ubNumElites, ubSize, iCompositionID);
  pSector = &SectorInfo[gGarrisonGroup[iGarrisonID].ubSectorID];
  // if there are administrators in this sector, remove them first.

  while (ubSize && pSector->ubNumAdmins) {
    pSector->ubNumAdmins--;
    ubSize--;
    if (ubNumTroops) {
      ubNumTroops--;
    } else {
      ubNumElites--;
    }
  }
  // No administrators are left.

  // Eliminate the troops
  while (ubNumTroops) {
    if (pSector->ubNumTroops) {
      pSector->ubNumTroops--;
    } else if (pSector->ubNumElites) {
      pSector->ubNumElites--;
    } else {
      Assert(0);
    }
    ubNumTroops--;
  }

  // Eliminate the elites
  while (ubNumElites) {
    if (pSector->ubNumElites) {
      pSector->ubNumElites--;
    } else if (pSector->ubNumTroops) {
      pSector->ubNumTroops--;
    } else {
      Assert(0);
    }
    ubNumElites--;
  }

  RecalculateGarrisonWeight(iGarrisonID);
}

static void MoveSAIGroupToSector(GROUP **const pGroup, uint8_t const sector,
                                 SAIMOVECODE const move_code, uint8_t const intention) {
  uint8_t const dst_x = SECTORX(sector);
  uint8_t const dst_y = SECTORY(sector);
  GROUP &g = **pGroup;

  if (g.fBetweenSectors) SetEnemyGroupSector(g, SECTOR(g.ubSectorX, g.ubSectorY));

  g.pEnemyGroup->ubIntention = intention;
  g.ubMoveType = ONE_WAY;

  /* Make sure that the group isn't moving into a garrison sector. These sectors
   * should be using ASSAULT intentions! */
  Assert(intention != PURSUIT || SectorInfo[sector].ubGarrisonID == NO_GARRISON);

  /* If the destination sector is the current location. Instead of causing code
   * logic problems, simply process them as if they just arrived. */
  if (g.ubSectorX == dst_x && g.ubSectorY == dst_y &&
      EvaluateGroupSituation(&g)) {  // The group was deleted.
    *pGroup = 0;
    return;
  }

  uint8_t const x = g.ubSectorX;
  uint8_t const y = g.ubSectorY;
  switch (move_code) {
    case STAGE:
      MoveGroupFromSectorToSectorButAvoidPlayerInfluencedSectorsAndStopOneSectorBeforeEnd(
          g, x, y, dst_x, dst_y);
      break;
    case EVASIVE:
      MoveGroupFromSectorToSectorButAvoidPlayerInfluencedSectors(g, x, y, dst_x, dst_y);
      break;
    case DIRECT:
      MoveGroupFromSectorToSector(g, x, y, dst_x, dst_y);
      break;
  }
  /* Make sure that the group is moving. If this fails, then the pathing may
   * have failed for some reason. */
  ValidateGroup(&g);
}

// If there are any enemy groups that will be moving through this sector due,
// they will have to repath which will cause them to avoid the sector.  Returns
// the number of redirected groups.
static uint8_t RedirectEnemyGroupsMovingThroughSector(uint8_t ubSectorX, uint8_t ubSectorY) {
  uint8_t ubNumGroupsRedirected = 0;
  WAYPOINT *pWaypoint;
  uint8_t ubDestSectorID;
  FOR_EACH_ENEMY_GROUP(pGroup) {
    if (pGroup->ubMoveType == ONE_WAY) {  // check the waypoint list
      if (GroupWillMoveThroughSector(pGroup, ubSectorX, ubSectorY)) {
        // extract the group's destination.
        pWaypoint = GetFinalWaypoint(pGroup);
        Assert(pWaypoint);
        ubDestSectorID = (uint8_t)SECTOR(pWaypoint->x, pWaypoint->y);
        SetEnemyGroupSector(*pGroup, SECTOR(pGroup->ubSectorX, pGroup->ubSectorY));
        MoveSAIGroupToSector(&pGroup, ubDestSectorID, EVASIVE, pGroup->pEnemyGroup->ubIntention);
        ubNumGroupsRedirected++;
      }
    }
  }
  if (ubNumGroupsRedirected) {
    ScreenMsg(FONT_LTBLUE, MSG_BETAVERSION,
              L"Test message for new feature:  %d enemy groups were redirected "
              L"away from moving through sector %c%d.  Please don't report "
              L"unless this number is greater than 5.",
              ubNumGroupsRedirected, ubSectorY + 'A' - 1, ubSectorX);
  }
  return ubNumGroupsRedirected;
}

// when the SAI compositions change, it is necessary to call this function upon
// version load, to reflect the changes of the compositions to the sector that
// haven't been visited yet.
static void ReinitializeUnvisitedGarrisons() {
  SECTORINFO *pSector;
  ARMY_COMPOSITION *pArmyComp;
  GROUP *pGroup;
  int32_t i, cnt, iEliteChance, iAdminChance;

  // Recreate the compositions
  memcpy(gArmyComp, gOrigArmyComp, NUM_ARMY_COMPOSITIONS * sizeof(ARMY_COMPOSITION));
  EvolveQueenPriorityPhase(TRUE);

  // Go through each unvisited sector and recreate the garrison forces based on
  // the desired population.
  for (i = 0; i < giGarrisonArraySize; i++) {
    if (gGarrisonGroup[i].ubComposition >= LEVEL1_DEFENCE &&
        gGarrisonGroup[i].ubComposition <= LEVEL3_DEFENCE) {  // These 3 compositions make up the
                                                              // perimeter around Meduna.  The
                                                              // existance of these are based on the
      // difficulty level, and we don't want to reset these anyways, due to the
      // fact that many of the reinforcements come from these sectors, and it
      // could potentially add upwards of 150 extra troops which would seriously
      // unbalance the difficulty.
      continue;
    }
    pSector = &SectorInfo[gGarrisonGroup[i].ubSectorID];
    pArmyComp = &gArmyComp[gGarrisonGroup[i].ubComposition];
    if (!(pSector->uiFlags & SF_ALREADY_VISITED)) {
      pSector->ubNumAdmins = 0;
      pSector->ubNumTroops = 0;
      pSector->ubNumElites = 0;
      if (gfQueenAIAwake) {
        cnt = pArmyComp->bDesiredPopulation;
      } else {
        cnt = pArmyComp->bStartPopulation;
      }

      if (gGarrisonGroup[i].ubPendingGroupID) {  // if the garrison has reinforcements on
                                                 // route, then subtract the number of
        // reinforcements from the value we reset the size of the garrison. This
        // is to prevent overfilling the group.
        pGroup = GetGroup(gGarrisonGroup[i].ubPendingGroupID);
        if (pGroup) {
          cnt -= pGroup->ubGroupSize;
          cnt = std::max(cnt, 0);
        }
      }

      iEliteChance = pArmyComp->bElitePercentage;
      iAdminChance = pArmyComp->bAdminPercentage;
      if (iAdminChance && !gfQueenAIAwake && cnt) {
        pSector->ubNumAdmins = iAdminChance * cnt / 100;
      } else
        while (cnt--) {  // for each person, randomly determine the types of each
                         // soldier.
          if (Chance(iEliteChance)) {
            pSector->ubNumElites++;
          } else {
            pSector->ubNumTroops++;
          }
        }
    }
  }
}

#include "gtest/gtest.h"

TEST(StrategicAI, asserts) {
  EXPECT_EQ(sizeof(ARMY_COMPOSITION), 20);
  EXPECT_EQ(sizeof(PATROL_GROUP), 20);
  EXPECT_EQ(sizeof(GARRISON_GROUP), 14);
}
