// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#include "Tactical/Morale.h"

#include <algorithm>

#include "Macro.h"
#include "SGP/Debug.h"
#include "Strategic/Assignments.h"
#include "Strategic/MapScreen.h"
#include "Strategic/StrategicMap.h"
#include "Strategic/StrategicMovement.h"
#include "Strategic/StrategicStatus.h"
#include "Tactical/Campaign.h"
#include "Tactical/DialogueControl.h"
#include "Tactical/DrugsAndAlcohol.h"
#include "Tactical/Overhead.h"
#include "Tactical/SkillCheck.h"
#include "Tactical/SoldierMacros.h"
#include "Tactical/SoldierProfile.h"
#include "Tactical/Squads.h"
#include "TacticalAI/AI.h"
#include "Utils/FontControl.h"
#include "Utils/Message.h"

#define MORALE_MOD_MAX \
  50  // morale *mod* range is -50 to 50, if you change this, check the decay
      // formulas!

#define DRUG_EFFECT_MORALE_MOD 150
#define ALCOHOL_EFFECT_MORALE_MOD 160

#define HOURS_BETWEEN_STRATEGIC_DECAY 3

#define PHOBIC_LIMIT -20

// macros
#define SOLDIER_IN_SECTOR(s, x, y, z) \
  (!(s)->fBetweenSectors && (s)->sSectorX == (x) && (s)->sSectorY == (y) && (s)->bSectorZ == (z))

MoraleEvent gbMoraleEvent[NUM_MORALE_EVENTS] = {
    // TACTICAL = Short Term Effect, STRATEGIC = Long Term Effect
    {TACTICAL_MORALE_EVENT, +4},  //	MORALE_KILLED_ENEMY
    {TACTICAL_MORALE_EVENT,
     -5},                           //	MORALE_SQUADMATE_DIED,		// in same sector (not really
                                    // squad)... IN ADDITION to strategic loss of morale
    {TACTICAL_MORALE_EVENT, -1},    //	MORALE_SUPPRESSED,				//
                                    // up to 4 times per turn
    {TACTICAL_MORALE_EVENT, -2},    //	MORALE_AIRSTRIKE,
    {TACTICAL_MORALE_EVENT, +2},    //	MORALE_DID_LOTS_OF_DAMAGE,
    {TACTICAL_MORALE_EVENT, -3},    //	MORALE_TOOK_LOTS_OF_DAMAGE,
    {STRATEGIC_MORALE_EVENT, -5},   //	MORALE_KILLED_CIVILIAN,
    {STRATEGIC_MORALE_EVENT, +4},   //	MORALE_BATTLE_WON,
    {STRATEGIC_MORALE_EVENT, -5},   //	MORALE_RAN_AWAY,
    {STRATEGIC_MORALE_EVENT, +2},   //	MORALE_HEARD_BATTLE_WON,
    {STRATEGIC_MORALE_EVENT, -2},   //	MORALE_HEARD_BATTLE_LOST,
    {STRATEGIC_MORALE_EVENT, +5},   //	MORALE_TOWN_LIBERATED,
    {STRATEGIC_MORALE_EVENT, -5},   //	MORALE_TOWN_LOST,
    {STRATEGIC_MORALE_EVENT, +8},   //	MORALE_MINE_LIBERATED,
    {STRATEGIC_MORALE_EVENT, -8},   //	MORALE_MINE_LOST,
    {STRATEGIC_MORALE_EVENT, +3},   //	MORALE_SAM_SITE_LIBERATED,
    {STRATEGIC_MORALE_EVENT, -3},   //	MORALE_SAM_SITE_LOST,
    {STRATEGIC_MORALE_EVENT, -15},  //	MORALE_BUDDY_DIED,
    {STRATEGIC_MORALE_EVENT, +5},   //	MORALE_HATED_DIED,
    {STRATEGIC_MORALE_EVENT,
     -5},                           //	MORALE_TEAMMATE_DIED,			// not in same sector
    {STRATEGIC_MORALE_EVENT, +5},   //	MORALE_LOW_DEATHRATE,
    {STRATEGIC_MORALE_EVENT, -5},   //	MORALE_HIGH_DEATHRATE,
    {STRATEGIC_MORALE_EVENT, +2},   //	MORALE_GREAT_MORALE,
    {STRATEGIC_MORALE_EVENT, -2},   //	MORALE_POOR_MORALE,
    {TACTICAL_MORALE_EVENT, -10},   //  MORALE_DRUGS_CRASH
    {TACTICAL_MORALE_EVENT, -10},   //  MORALE_ALCOHOL_CRASH
    {STRATEGIC_MORALE_EVENT, +15},  //  MORALE_MONSTER_QUEEN_KILLED
    {STRATEGIC_MORALE_EVENT, +25},  //  MORALE_DEIDRANNA_KILLED
    {TACTICAL_MORALE_EVENT, -1},    //	MORALE_CLAUSTROPHOBE_UNDERGROUND,
    {TACTICAL_MORALE_EVENT, -5},    //	MORALE_INSECT_PHOBIC_SEES_CREATURE,
    {TACTICAL_MORALE_EVENT, -1},    //	MORALE_NERVOUS_ALONE,
    {STRATEGIC_MORALE_EVENT, -5},   //	MORALE_MERC_CAPTURED,
    {STRATEGIC_MORALE_EVENT, -5},   //	MORALE_MERC_MARRIED,
    {STRATEGIC_MORALE_EVENT, +8},   //	MORALE_QUEEN_BATTLE_WON,
    {STRATEGIC_MORALE_EVENT, +5},   //  MORALE_SEX,
};

BOOLEAN gfSomeoneSaidMoraleQuote = FALSE;

int8_t GetMoraleModifier(SOLDIERTYPE *pSoldier) {
  if (pSoldier->uiStatusFlags & SOLDIER_PC) {
    if (pSoldier->bMorale > 50) {
      // give +1 at 55, +3 at 65, up to +5 at 95 and above
      return ((pSoldier->bMorale - 45) / 10);
    } else {
      // give penalties down to -20 at 0 (-2 at 45, -4 by 40...)
      return ((pSoldier->bMorale - 50) * 2 / 5);
    }
  } else {
    // use AI morale
    switch (pSoldier->bAIMorale) {
      case MORALE_HOPELESS:
        return (-15);
      case MORALE_WORRIED:
        return (-7);
      case MORALE_CONFIDENT:
        return (2);
      case MORALE_FEARLESS:
        return (5);
      default:
        return (0);
    }
  }
}

static void DecayTacticalMorale(SOLDIERTYPE *pSoldier) {
  // decay the tactical morale modifier
  if (pSoldier->bTacticalMoraleMod != 0) {
    // decay the modifier!
    if (pSoldier->bTacticalMoraleMod > 0) {
      pSoldier->bTacticalMoraleMod =
          std::max(0, pSoldier->bTacticalMoraleMod - (8 - pSoldier->bTacticalMoraleMod / 10));
    } else {
      pSoldier->bTacticalMoraleMod =
          std::min(0, pSoldier->bTacticalMoraleMod + (6 + pSoldier->bTacticalMoraleMod / 10));
    }
  }
}

static void DecayStrategicMorale(SOLDIERTYPE *pSoldier) {
  // decay the modifier!
  if (pSoldier->bStrategicMoraleMod > 0) {
    pSoldier->bStrategicMoraleMod =
        std::max(0, pSoldier->bStrategicMoraleMod - (8 - pSoldier->bStrategicMoraleMod / 10));
  } else {
    pSoldier->bStrategicMoraleMod =
        std::min(0, pSoldier->bStrategicMoraleMod + (6 + pSoldier->bStrategicMoraleMod / 10));
  }
}

void DecayTacticalMoraleModifiers() {
  FOR_EACH_IN_TEAM(s, OUR_TEAM) {
    // if the merc is in Arulco
    // CJC: decay modifiers while asleep! or POW!
    if (s->ubProfile == NO_PROFILE) continue;
    if (s->bAssignment == IN_TRANSIT) continue;
    if (s->bAssignment == ASSIGNMENT_DEAD) continue;

    // only let morale mod decay if it is positive while merc is a POW
    if (s->bAssignment == ASSIGNMENT_POW && s->bTacticalMoraleMod < 0) continue;

    switch (GetProfile(s->ubProfile).bPersonalityTrait) {
      case CLAUSTROPHOBIC:
        if (s->bSectorZ > 0) {
          // underground, no recovery... in fact, if tact morale is high, decay
          if (s->bTacticalMoraleMod > PHOBIC_LIMIT) {
            HandleMoraleEvent(s, MORALE_CLAUSTROPHOBE_UNDERGROUND, s->sSectorX, s->sSectorY,
                              s->bSectorZ);
          }
          continue;
        }
        break;

      case NERVOUS:
        if (s->bMorale < 50) {
          BOOLEAN handle_nervous;
          if (s->ubGroupID != 0 && PlayerIDGroupInMotion(s->ubGroupID)) {
            handle_nervous = NumberOfPeopleInSquad(s->bAssignment) == 1;
          } else if (s->bInSector) {
            handle_nervous = DistanceToClosestFriend(s) > NERVOUS_RADIUS;
          } else {
            // look for anyone else in same sector
            handle_nervous = TRUE;
            CFOR_EACH_IN_TEAM(other, OUR_TEAM) {
              if (other != s && other->sSectorX == s->sSectorX && other->sSectorY == s->sSectorY &&
                  other->bSectorZ == s->bSectorZ) {
                // found someone!
                handle_nervous = FALSE;
                break;
              }
            }
          }

          if (handle_nervous) {
            if (s->bTacticalMoraleMod == PHOBIC_LIMIT) {
              // don't change morale
              continue;
            }

            // alone, no recovery... in fact, if tact morale is high, decay
            if (!(s->usQuoteSaidFlags & SOLDIER_QUOTE_SAID_PERSONALITY)) {
              TacticalCharacterDialogue(s, QUOTE_PERSONALITY_TRAIT);
              s->usQuoteSaidFlags |= SOLDIER_QUOTE_SAID_PERSONALITY;
            }
            HandleMoraleEvent(s, MORALE_NERVOUS_ALONE, s->sSectorX, s->sSectorY, s->bSectorZ);
            continue;
          }
        }
        break;
    }

    DecayTacticalMorale(s);
    RefreshSoldierMorale(s);
  }
}

static void DecayStrategicMoraleModifiers() {
  FOR_EACH_IN_TEAM(s, OUR_TEAM) {
    // if the merc is active, in Arulco
    // CJC: decay modifiers while asleep! or POW!
    if (s->ubProfile != NO_PROFILE && s->bAssignment != IN_TRANSIT &&
        s->bAssignment != ASSIGNMENT_DEAD) {
      // only let morale mod decay if it is positive while merc is a POW
      if (s->bAssignment == ASSIGNMENT_POW && s->bStrategicMoraleMod < 0) {
        continue;
      }

      DecayStrategicMorale(s);
      RefreshSoldierMorale(s);
    }
  }
}

void RefreshSoldierMorale(SOLDIERTYPE *pSoldier) {
  int32_t iActualMorale;

  if (pSoldier->fMercAsleep) {
    // delay this till later!
    return;
  }

  // CJC, April 19, 1999: added up to 20% morale boost according to progress
  iActualMorale = DEFAULT_MORALE + (int32_t)pSoldier->bTeamMoraleMod +
                  (int32_t)pSoldier->bTacticalMoraleMod + (int32_t)pSoldier->bStrategicMoraleMod +
                  (int32_t)(CurrentPlayerProgressPercentage() / 5);

  // ATE: Modify morale based on drugs....
  iActualMorale += ((pSoldier->bDrugEffect[DRUG_TYPE_ADRENALINE] * DRUG_EFFECT_MORALE_MOD) / 100);
  iActualMorale += ((pSoldier->bDrugEffect[DRUG_TYPE_ALCOHOL] * ALCOHOL_EFFECT_MORALE_MOD) / 100);

  iActualMorale = std::min(100, iActualMorale);
  iActualMorale = std::max(0, iActualMorale);
  pSoldier->bMorale = (int8_t)iActualMorale;

  // update mapscreen as needed
  fCharacterInfoPanelDirty = TRUE;
}

static void UpdateSoldierMorale(SOLDIERTYPE *pSoldier, uint8_t ubType, int8_t bMoraleMod) {
  int32_t iMoraleModTotal;

  if (!pSoldier->bActive) return;
  if (pSoldier->bLife < CONSCIOUSNESS) return;
  if (IsMechanical(*pSoldier)) return;
  if (AM_AN_EPC(pSoldier)) return;

  if ((pSoldier->bAssignment == ASSIGNMENT_DEAD) || (pSoldier->bAssignment == ASSIGNMENT_POW) ||
      (pSoldier->bAssignment == IN_TRANSIT)) {
    return;
  }

  if (pSoldier->ubProfile == NO_PROFILE) {
    return;
  }

  MERCPROFILESTRUCT const &p = GetProfile(pSoldier->ubProfile);

  if (bMoraleMod > 0) {
    switch (p.bAttitude) {
      case ATT_OPTIMIST:
      case ATT_AGGRESSIVE:
        bMoraleMod += 1;
        break;
      case ATT_PESSIMIST:
        bMoraleMod -= 1;
        break;
      default:
        break;
    }
    if (bMoraleMod < 0) {
      // can't change a positive event into a negative one!
      bMoraleMod = 0;
    }
  } else {
    switch (p.bAttitude) {
      case ATT_OPTIMIST:
        bMoraleMod += 1;
        break;
      case ATT_PESSIMIST:
        bMoraleMod -= 1;
        break;
      case ATT_COWARD:
        bMoraleMod -= 2;
      default:
        break;
    }
    if (pSoldier->bLevel == 1) {
      bMoraleMod--;
    } else if (pSoldier->bLevel > 5) {
      bMoraleMod++;
    }
    if (bMoraleMod > 0) {
      // can't change a negative event into a positive one!
      bMoraleMod = 0;
    }
  }
  // apply change!
  if (ubType == TACTICAL_MORALE_EVENT) {
    iMoraleModTotal = (int32_t)pSoldier->bTacticalMoraleMod + (int32_t)bMoraleMod;
    iMoraleModTotal = std::min(iMoraleModTotal, MORALE_MOD_MAX);
    iMoraleModTotal = std::max(iMoraleModTotal, -MORALE_MOD_MAX);
    pSoldier->bTacticalMoraleMod = (int8_t)iMoraleModTotal;
  } else if (gTacticalStatus.fEnemyInSector && !pSoldier->bInSector)  // delayed strategic
  {
    iMoraleModTotal = (int32_t)pSoldier->bDelayedStrategicMoraleMod + (int32_t)bMoraleMod;
    iMoraleModTotal = std::min(iMoraleModTotal, MORALE_MOD_MAX);
    iMoraleModTotal = std::max(iMoraleModTotal, -MORALE_MOD_MAX);
    pSoldier->bDelayedStrategicMoraleMod = (int8_t)iMoraleModTotal;
  } else  // strategic
  {
    iMoraleModTotal = (int32_t)pSoldier->bStrategicMoraleMod + (int32_t)bMoraleMod;
    iMoraleModTotal = std::min(iMoraleModTotal, MORALE_MOD_MAX);
    iMoraleModTotal = std::max(iMoraleModTotal, -MORALE_MOD_MAX);
    pSoldier->bStrategicMoraleMod = (int8_t)iMoraleModTotal;
  }

  RefreshSoldierMorale(pSoldier);

  if (!pSoldier->fMercAsleep) {
    if (!gfSomeoneSaidMoraleQuote) {
      // Check if we're below a certain value and warn
      if (pSoldier->bMorale < 35) {
        // Have we said this quote yet?
        if (!(pSoldier->usQuoteSaidFlags & SOLDIER_QUOTE_SAID_LOW_MORAL)) {
          gfSomeoneSaidMoraleQuote = TRUE;

          // ATE: Amde it a DELAYED QUOTE - will be delayed by the dialogue Q
          // until it's our turn...
          DelayedTacticalCharacterDialogue(pSoldier, QUOTE_STARTING_TO_WHINE);
          pSoldier->usQuoteSaidFlags |= SOLDIER_QUOTE_SAID_LOW_MORAL;
        }
      }
    }
  }

  // Reset flag!
  if (pSoldier->bMorale > 65) {
    pSoldier->usQuoteSaidFlags &= (~SOLDIER_QUOTE_SAID_LOW_MORAL);
  }
}

static void HandleMoraleEventForSoldier(SOLDIERTYPE *pSoldier, int8_t bMoraleEvent) {
  UpdateSoldierMorale(pSoldier, gbMoraleEvent[bMoraleEvent].ubType,
                      gbMoraleEvent[bMoraleEvent].bChange);
}

void HandleMoraleEvent(SOLDIERTYPE *pSoldier, int8_t bMoraleEvent, int16_t sMapX, int16_t sMapY,
                       int8_t bMapZ) {
  gfSomeoneSaidMoraleQuote = FALSE;

  // NOTE: Many morale events are NOT attached to a specific player soldier at
  // all! Those that do need it have Asserts on a case by case basis below
  if (pSoldier == NULL) {
    DebugMsg(
        TOPIC_JA2, DBG_LEVEL_3,
        String("Handling morale event %d at X=%d, Y=%d,Z=%d", bMoraleEvent, sMapX, sMapY, bMapZ));
  } else {
    DebugMsg(TOPIC_JA2, DBG_LEVEL_3,
             String("Handling morale event %d for %ls at X=%d, Y=%d, Z=%d", bMoraleEvent,
                    pSoldier->name, sMapX, sMapY, bMapZ));
  }

  switch (bMoraleEvent) {
    case MORALE_KILLED_ENEMY:
    case MORALE_DID_LOTS_OF_DAMAGE:
    case MORALE_DRUGS_CRASH:
    case MORALE_ALCOHOL_CRASH:
    case MORALE_SUPPRESSED:
    case MORALE_TOOK_LOTS_OF_DAMAGE:
    case MORALE_HIGH_DEATHRATE:
    case MORALE_SEX:
      // needs specific soldier!
      Assert(pSoldier);
      // affects the soldier only
      HandleMoraleEventForSoldier(pSoldier, bMoraleEvent);
      break;

    case MORALE_CLAUSTROPHOBE_UNDERGROUND:
    case MORALE_INSECT_PHOBIC_SEES_CREATURE:
    case MORALE_NERVOUS_ALONE:
      // needs specific soldier!
      Assert(pSoldier);
      // affects the soldier only, should be ignored if tactical morale mod is -20
      // or less
      if (pSoldier->bTacticalMoraleMod > PHOBIC_LIMIT) {
        HandleMoraleEventForSoldier(pSoldier, bMoraleEvent);
      }
      break;

    case MORALE_BATTLE_WON:
      // affects everyone to varying degrees
      FOR_EACH_IN_TEAM(s, OUR_TEAM) {
        if (SOLDIER_IN_SECTOR(s, sMapX, sMapY, bMapZ)) {
          HandleMoraleEventForSoldier(s, MORALE_BATTLE_WON);
        } else {
          HandleMoraleEventForSoldier(s, MORALE_HEARD_BATTLE_WON);
        }
      }
      break;

    case MORALE_RAN_AWAY:
      // affects everyone to varying degrees
      FOR_EACH_IN_TEAM(i, OUR_TEAM) {
        SOLDIERTYPE &s = *i;
        /* CJC: adding to SOLDIER_IN_SECTOR check special stuff because the old
         * sector values might be appropriate (because in transit going out of
         * that sector!) */
        if (SOLDIER_IN_SECTOR(&s, sMapX, sMapY, bMapZ) ||
            (s.fBetweenSectors && s.ubPrevSectorID % 16 + 1 == sMapX &&
             s.ubPrevSectorID / 16 + 1 == sMapY && s.bSectorZ == bMapZ)) {
          switch (GetProfile(s.ubProfile).bAttitude) {
            case ATT_AGGRESSIVE:
              // Double the penalty - these guys REALLY hate running away
              HandleMoraleEventForSoldier(&s, MORALE_RAN_AWAY);
              /* FALLTHROUGH */
            default:
              HandleMoraleEventForSoldier(&s, MORALE_RAN_AWAY);
              break;

            case ATT_COWARD:
              // No penalty - cowards are perfectly happy to avoid fights
              break;
          }
        } else {
          HandleMoraleEventForSoldier(&s, MORALE_HEARD_BATTLE_LOST);
        }
      }
      break;

    case MORALE_TOWN_LIBERATED:
    case MORALE_TOWN_LOST:
    case MORALE_MINE_LIBERATED:
    case MORALE_MINE_LOST:
    case MORALE_SAM_SITE_LIBERATED:
    case MORALE_SAM_SITE_LOST:
    case MORALE_KILLED_CIVILIAN:
    case MORALE_LOW_DEATHRATE:
    case MORALE_HEARD_BATTLE_WON:
    case MORALE_HEARD_BATTLE_LOST:
    case MORALE_MONSTER_QUEEN_KILLED:
    case MORALE_DEIDRANNA_KILLED:
      // affects everyone, everywhere
      FOR_EACH_IN_TEAM(s, OUR_TEAM) { HandleMoraleEventForSoldier(s, bMoraleEvent); }
      break;

    case MORALE_POOR_MORALE:
    case MORALE_GREAT_MORALE:
    case MORALE_AIRSTRIKE:
      // affects every in sector
      FOR_EACH_IN_TEAM(s, OUR_TEAM) {
        if (SOLDIER_IN_SECTOR(s, sMapX, sMapY, bMapZ)) {
          HandleMoraleEventForSoldier(s, bMoraleEvent);
        }
      }
      break;

    case MORALE_MERC_CAPTURED:
      // needs specific soldier! (for reputation, not here)
      Assert(pSoldier);

      // affects everyone
      FOR_EACH_IN_TEAM(s, OUR_TEAM) { HandleMoraleEventForSoldier(s, bMoraleEvent); }
      break;

    case MORALE_TEAMMATE_DIED:
      // needs specific soldier!
      Assert(pSoldier);

      // affects everyone, in sector differently than not, extra bonuses if it's a
      // buddy or hated merc
      FOR_EACH_IN_TEAM(i, OUR_TEAM) {
        SOLDIERTYPE &other = *i;
        if (other.ubProfile == NO_PROFILE) continue;
        MERCPROFILESTRUCT const &p = GetProfile(other.ubProfile);

        if (HATED_MERC(p, pSoldier->ubProfile)) {
          HandleMoraleEventForSoldier(&other, MORALE_HATED_DIED);
        } else {
          if (SOLDIER_IN_SECTOR(&other, sMapX, sMapY,
                                bMapZ)) {  // Mate died in my sector! tactical morale mod
            HandleMoraleEventForSoldier(&other, MORALE_SQUADMATE_DIED);
          }

          /* This is handled for everyone even if in sector, as it's a strategic
           * morale mod */
          HandleMoraleEventForSoldier(&other, MORALE_TEAMMATE_DIED);

          if (BUDDY_MERC(p, pSoldier->ubProfile)) {
            HandleMoraleEventForSoldier(&other, MORALE_BUDDY_DIED);
          }
        }
      }
      break;

    case MORALE_MERC_MARRIED:
      /* Female mercs get unhappy based on how sexist they are (=hate men),
       * gentlemen males get unhappy too */
      FOR_EACH_IN_TEAM(i, OUR_TEAM) {
        SOLDIERTYPE &other = *i;
        if (other.ubProfile == NO_PROFILE) continue;

        // We hate 'em anyways
        if (WhichHated(other.ubProfile, pSoldier->ubProfile) != -1) continue;

        MERCPROFILESTRUCT const &p = GetProfile(other.ubProfile);
        if (p.bSex == FEMALE) {
          switch (p.bSexist) {
            case VERY_SEXIST:  // handle twice
              HandleMoraleEventForSoldier(&other, MORALE_MERC_MARRIED);
              /* FALLTHROUGH */
            case SOMEWHAT_SEXIST:
              HandleMoraleEventForSoldier(&other, MORALE_MERC_MARRIED);
              break;
          }
        } else {
          switch (p.bSexist) {
            case GENTLEMAN:
              HandleMoraleEventForSoldier(&other, MORALE_MERC_MARRIED);
              break;
          }
        }
      }
      break;

    default:
      // debug message
      ScreenMsg(MSG_FONT_RED, MSG_BETAVERSION, L"Invalid morale event type = %d.  AM/CC-1",
                bMoraleEvent);
      break;
  }

  // some morale events also impact the player's reputation with the mercs back
  // home
  switch (bMoraleEvent) {
    case MORALE_HIGH_DEATHRATE:
      ModifyPlayerReputation(REPUTATION_HIGH_DEATHRATE);
      break;
    case MORALE_LOW_DEATHRATE:
      ModifyPlayerReputation(REPUTATION_LOW_DEATHRATE);
      break;
    case MORALE_POOR_MORALE:
      ModifyPlayerReputation(REPUTATION_POOR_MORALE);
      break;
    case MORALE_GREAT_MORALE:
      ModifyPlayerReputation(REPUTATION_GREAT_MORALE);
      break;
    case MORALE_BATTLE_WON:
      ModifyPlayerReputation(REPUTATION_BATTLE_WON);
      break;
    case MORALE_RAN_AWAY:
    case MORALE_HEARD_BATTLE_LOST:
      ModifyPlayerReputation(REPUTATION_BATTLE_LOST);
      break;
    case MORALE_TOWN_LIBERATED:
      ModifyPlayerReputation(REPUTATION_TOWN_WON);
      break;
    case MORALE_TOWN_LOST:
      ModifyPlayerReputation(REPUTATION_TOWN_LOST);
      break;
    case MORALE_TEAMMATE_DIED:
      // impact depends on that dude's level of experience
      ModifyPlayerReputation((uint8_t)(pSoldier->bExpLevel * REPUTATION_SOLDIER_DIED));
      break;
    case MORALE_MERC_CAPTURED:
      // impact depends on that dude's level of experience
      ModifyPlayerReputation((uint8_t)(pSoldier->bExpLevel * REPUTATION_SOLDIER_CAPTURED));
      break;
    case MORALE_KILLED_CIVILIAN:
      ModifyPlayerReputation(REPUTATION_KILLED_CIVILIAN);
      break;
    case MORALE_MONSTER_QUEEN_KILLED:
      ModifyPlayerReputation(REPUTATION_KILLED_MONSTER_QUEEN);
      break;
    case MORALE_DEIDRANNA_KILLED:
      ModifyPlayerReputation(REPUTATION_KILLED_DEIDRANNA);
      break;

    default:
      // no reputation impact
      break;
  }
}

void HourlyMoraleUpdate() {
  static int8_t strategic_morale_update_counter = 0;

  // loop through all mercs to calculate their morale
  FOR_EACH_IN_TEAM(s, OUR_TEAM) {
    // if the merc is active, in Arulco, and conscious, not POW
    if (s->ubProfile == NO_PROFILE) continue;
    if (s->fMercAsleep) continue;
    if (s->bAssignment == IN_TRANSIT) continue;
    if (s->bAssignment == ASSIGNMENT_DEAD) continue;
    if (s->bAssignment == ASSIGNMENT_POW) continue;

    // calculate the guy's opinion of the people he is with
    MERCPROFILESTRUCT const &p = GetProfile(s->ubProfile);

    // If we're moving, we only check our opinions of people in our squad
    bool const same_group_only = s->ubGroupID != 0 && PlayerIDGroupInMotion(s->ubGroupID);
    bool found_hated = false;

    // Counts to calculate average opinion
    int32_t sum_opinions = 0;
    int8_t n_team_members = 0;

    // Let people with high leadership affect their own morale
    int8_t highest_team_leadership = EffectiveLeadership(s);

    // loop through all other mercs
    CFOR_EACH_IN_TEAM(other, OUR_TEAM) {
      // skip past ourselves and all inactive mercs
      if (other == s) continue;
      if (other->ubProfile == NO_PROFILE) continue;
      if (other->fMercAsleep) continue;
      if (other->bAssignment == IN_TRANSIT) continue;
      if (other->bAssignment == ASSIGNMENT_DEAD) continue;
      if (other->bAssignment == ASSIGNMENT_POW) continue;

      if (same_group_only) {  // All we have to check is the group ID
        if (s->ubGroupID != other->ubGroupID) continue;
      } else {
        // Check to see if the location is the same
        if (other->sSectorX != s->sSectorX) continue;
        if (other->sSectorY != s->sSectorY) continue;
        if (other->bSectorZ != s->bSectorZ) continue;

        // If the OTHER soldier is in motion then we don't do anything!
        if (other->ubGroupID != 0 && PlayerIDGroupInMotion(other->ubGroupID)) continue;
      }

      int8_t opinion = p.bMercOpinion[other->ubProfile];
      if (opinion == HATED_OPINION) {
        int8_t const hated = WhichHated(s->ubProfile, other->ubProfile);
        if (hated >= 2 ||  // Learn to hate which has become full-blown hatred,
                           // full strength
            p.bHatedCount[hated] <=
                p.bHatedTime[hated] / 2) {  // We're teamed with someone we hate! We HATE this!
                                            // Ignore everyone else!
          found_hated = true;
          break;
        }
        // Otherwise just mix this opinion in with everyone else

        // Scale according to how close to we are to snapping
        // KM : Divide by 0 error found.  Wrapped into an if statement.
        int8_t const hated_time = p.bHatedTime[hated];
        if (hated_time != 0) {
          opinion = (int32_t)opinion * (hated_time - p.bHatedCount[hated]) / hated_time;
        }
      }
      sum_opinions += opinion;
      ++n_team_members;
      int8_t const other_leadership = EffectiveLeadership(other);
      if (highest_team_leadership < other_leadership) highest_team_leadership = other_leadership;
    }

    int8_t actual_team_opinion;
    if (found_hated) {
      // If teamed with someone we hated, team opinion is automatically minimum
      actual_team_opinion = HATED_OPINION;
    } else if (n_team_members > 0) {
      actual_team_opinion = sum_opinions / n_team_members;
      // give bonus/penalty for highest leadership value on team
      actual_team_opinion += (highest_team_leadership - 50) / 10;
    } else  // alone
    {
      actual_team_opinion = 0;
    }

    // reduce to a range of HATED through BUDDY
    if (actual_team_opinion > BUDDY_OPINION) {
      actual_team_opinion = BUDDY_OPINION;
    } else if (actual_team_opinion < HATED_OPINION) {
      actual_team_opinion = HATED_OPINION;
    }

    // Shift morale from team by ~10%

    // This should range between -75 and +75
    int8_t const team_morale_mod_diff = actual_team_opinion - s->bTeamMoraleMod;
    int8_t const team_morale_mod_change = team_morale_mod_diff > 0 ? 1 + team_morale_mod_diff / 10
                                          : team_morale_mod_diff < 0
                                              ? -1 + team_morale_mod_diff / 10
                                              : 0;
    s->bTeamMoraleMod += team_morale_mod_change;
    s->bTeamMoraleMod = std::min(s->bTeamMoraleMod, (int8_t)MORALE_MOD_MAX);
    s->bTeamMoraleMod = std::max(s->bTeamMoraleMod, (int8_t)(-MORALE_MOD_MAX));

    // New, December 3rd, 1998, by CJC --
    // If delayed strategic modifier exists then incorporate it in strategic mod
    if (s->bDelayedStrategicMoraleMod != 0) {
      s->bStrategicMoraleMod += s->bDelayedStrategicMoraleMod;
      s->bStrategicMoraleMod = std::min(s->bStrategicMoraleMod, (int8_t)MORALE_MOD_MAX);
      s->bStrategicMoraleMod = std::max(s->bStrategicMoraleMod, (int8_t)(-MORALE_MOD_MAX));
      s->bDelayedStrategicMoraleMod = 0;
    }

    /* Refresh the morale value for the soldier based on the recalculated team
     * modifier */
    RefreshSoldierMorale(s);
  }

  if (++strategic_morale_update_counter == HOURS_BETWEEN_STRATEGIC_DECAY) {
    strategic_morale_update_counter = 0;
    DecayStrategicMoraleModifiers();
  }
}

void DailyMoraleUpdate(SOLDIERTYPE *pSoldier) {
  if (pSoldier->ubProfile == NO_PROFILE) {
    return;
  }

  // CJC: made per hour now
  /*
          // decay the merc's strategic morale modifier
          if (pSoldier->bStrategicMoraleMod != 0)
          {
                  // decay the modifier!
                  DecayStrategicMorale( pSoldier );

                  // refresh the morale value for the soldier based on the
     recalculated modifier RefreshSoldierMorale( pSoldier );
          }
  */

  // check death rate vs. merc's tolerance once/day (ignores buddies!)
  if (MercThinksDeathRateTooHigh(GetProfile(pSoldier->ubProfile))) {
    // too high, morale takes a hit
    HandleMoraleEvent(pSoldier, MORALE_HIGH_DEATHRATE, pSoldier->sSectorX, pSoldier->sSectorY,
                      pSoldier->bSectorZ);
  }

  // check his morale vs. his morale tolerance once/day (ignores buddies!)
  if (MercThinksHisMoraleIsTooLow(pSoldier)) {
    // too low, morale sinks further (merc's in a funk and things aren't getting
    // better)
    HandleMoraleEvent(pSoldier, MORALE_POOR_MORALE, pSoldier->sSectorX, pSoldier->sSectorY,
                      pSoldier->bSectorZ);
  } else if (pSoldier->bMorale >= 75) {
    // very high morale, merc is cheerleading others
    HandleMoraleEvent(pSoldier, MORALE_GREAT_MORALE, pSoldier->sSectorX, pSoldier->sSectorY,
                      pSoldier->bSectorZ);
  }
}
