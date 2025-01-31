// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#include "Tactical/Boxing.h"

#include <algorithm>

#include "Laptop/History.h"
#include "Macro.h"
#include "SGP/Random.h"
#include "Strategic/GameClock.h"
#include "Strategic/StrategicMap.h"
#include "Tactical/AnimationData.h"
#include "Tactical/HandleUI.h"
#include "Tactical/Interface.h"
#include "Tactical/InterfaceDialogue.h"
#include "Tactical/Items.h"
#include "Tactical/OppList.h"
#include "Tactical/Overhead.h"
#include "Tactical/Points.h"
#include "Tactical/SoldierControl.h"
#include "Tactical/SoldierProfile.h"
#include "Tactical/TeamTurns.h"
#include "TacticalAI/AI.h"
#include "TacticalAI/NPC.h"
#include "TileEngine/RenderFun.h"
#include "TileEngine/WorldMan.h"
#include "Utils/FontControl.h"
#include "Utils/MusicControl.h"
#include "Utils/TimerControl.h"

int16_t gsBoxerGridNo[NUM_BOXERS] = {11393, 11233, 11073};
SOLDIERTYPE *gBoxer[NUM_BOXERS];
BOOLEAN gfBoxerFought[NUM_BOXERS] = {FALSE, FALSE, FALSE};
BOOLEAN gfLastBoxingMatchWonByPlayer = FALSE;
uint8_t gubBoxingMatchesWon = 0;
uint8_t gubBoxersRests = 0;
BOOLEAN gfBoxersResting = FALSE;

void ExitBoxing() {
  // find boxers and turn them neutral again

  // first time through loop, look for AI guy, then for PC guy.... for stupid
  // oppcnt/alert status reasons
  for (uint8_t ubPass = 0; ubPass < 2; ++ubPass) {
    // because boxer could die, loop through all soldier ptrs
    FOR_EACH_SOLDIER(s) {
      if (!(s->uiStatusFlags & SOLDIER_BOXER)) continue;
      if (GetRoom(s->sGridNo) != BOXING_RING) continue;

      if (s->uiStatusFlags & SOLDIER_PC) {
        if (ubPass == 0) continue;  // pass 0, only handle AI
        // put guy under AI control temporarily
        s->uiStatusFlags |= SOLDIER_PCUNDERAICONTROL;
      } else {
        if (ubPass == 1) continue;  // pass 1, only handle PCs
        // reset AI boxer to neutral
        SetSoldierNeutral(s);
        RecalculateOppCntsDueToBecomingNeutral(s);
      }
      CancelAIAction(s);
      s->bAlertStatus = STATUS_GREEN;
      s->bUnderFire = 0;

      // if necessary, revive boxer so he can leave ring
      if (s->bLife > 0 && (s->bLife < OKLIFE || s->bBreath < OKBREATH)) {
        s->bLife = std::max((int8_t)(OKLIFE * 2), s->bLife);
        if (s->bBreath < 100) {
          // deduct -ve BPs to grant some BPs back (properly)
          DeductPoints(s, 0, -100 * (100 - s->bBreath));
        }
        BeginSoldierGetup(s);
      }
    }
  }

  DeleteTalkingMenu();
  EndAllAITurns();

  if (CheckForEndOfCombatMode(FALSE)) {
    EndTopMessage();
    SetMusicMode(MUSIC_TACTICAL_NOTHING);
    // Lock UI until we get out of the ring
    guiPendingOverrideEvent = LU_BEGINUILOCK;
  }
}

// in both these cases we're going to want the AI to take over and move the
// boxers out of the ring!
void EndBoxingMatch(SOLDIERTYPE *pLoser) {
  if (pLoser->bTeam == OUR_TEAM) {
    SetBoxingState(LOST_ROUND);
  } else {
    SetBoxingState(WON_ROUND);
    gfLastBoxingMatchWonByPlayer = TRUE;
    gubBoxingMatchesWon++;
  }
  TriggerNPCRecord(DARREN, 22);
}

void BoxingPlayerDisqualified(SOLDIERTYPE *pOffender, int8_t bReason) {
  if (bReason == BOXER_OUT_OF_RING || bReason == NON_BOXER_IN_RING) {
    EVENT_StopMerc(pOffender);
  }
  SetBoxingState(DISQUALIFIED);
  TriggerNPCRecord(DARREN, 21);
  // ExitBoxing();
}

void TriggerEndOfBoxingRecord(SOLDIERTYPE *pSoldier) {
  // unlock UI
  guiPendingOverrideEvent = LU_ENDUILOCK;

  if (pSoldier) {
    switch (gTacticalStatus.bBoxingState) {
      case WON_ROUND:
        AddHistoryToPlayersLog(HISTORY_WON_BOXING, pSoldier->ubProfile, GetWorldTotalMin(),
                               gWorldSectorX, gWorldSectorY);
        TriggerNPCRecord(DARREN, 23);
        break;
      case LOST_ROUND:
        // log as lost
        AddHistoryToPlayersLog(HISTORY_LOST_BOXING, pSoldier->ubProfile, GetWorldTotalMin(),
                               gWorldSectorX, gWorldSectorY);
        TriggerNPCRecord(DARREN, 24);
        break;
      case DISQUALIFIED:
        AddHistoryToPlayersLog(HISTORY_DISQUALIFIED_BOXING, pSoldier->ubProfile, GetWorldTotalMin(),
                               gWorldSectorX, gWorldSectorY);
        break;
    }
  }

  SetBoxingState(NOT_BOXING);
}

uint8_t CountPeopleInBoxingRing() {
  uint8_t ubTotalInRing = 0;

  FOR_EACH_MERC(i) {
    if (GetRoom((*i)->sGridNo) == BOXING_RING) {
      ++ubTotalInRing;
    }
  }

  return (ubTotalInRing);
}

static void PickABoxer();

static void CountPeopleInBoxingRingAndDoActions() {
  uint8_t ubTotalInRing = 0;
  uint8_t ubPlayersInRing = 0;
  SOLDIERTYPE *pInRing[2] = {NULL, NULL};
  SOLDIERTYPE *pNonBoxingPlayer = NULL;

  FOR_EACH_MERC(i) {
    SOLDIERTYPE *const s = *i;
    if (GetRoom(s->sGridNo) != BOXING_RING) continue;

    if (ubTotalInRing < 2) pInRing[ubTotalInRing] = s;
    ++ubTotalInRing;

    if (s->uiStatusFlags & SOLDIER_PC) {
      ++ubPlayersInRing;
      if (!pNonBoxingPlayer && !(s->uiStatusFlags & SOLDIER_BOXER)) {
        pNonBoxingPlayer = s;
      }
    }
  }

  if (ubPlayersInRing > 1) {
    // boxing match just became invalid!
    if (gTacticalStatus.bBoxingState <= PRE_BOXING) {
      BoxingPlayerDisqualified(pNonBoxingPlayer, NON_BOXER_IN_RING);
      // set to not in boxing or it won't be handled otherwise
      SetBoxingState(NOT_BOXING);
    } else {
      BoxingPlayerDisqualified(pNonBoxingPlayer, NON_BOXER_IN_RING);
    }

    return;
  }

  if (gTacticalStatus.bBoxingState == BOXING_WAITING_FOR_PLAYER) {
    if (ubTotalInRing == 1 && ubPlayersInRing == 1) {
      // time to go to pre-boxing
      SetBoxingState(PRE_BOXING);
      PickABoxer();
    }
  } else
    // if pre-boxing, check for two people (from different teams!) in the ring
    if (gTacticalStatus.bBoxingState == PRE_BOXING) {
      if (ubTotalInRing == 2 && ubPlayersInRing == 1) {
        // ladieees and gennleman, we have a fight!
        for (uint32_t uiLoop = 0; uiLoop < 2; ++uiLoop) {
          if (!(pInRing[uiLoop]->uiStatusFlags & SOLDIER_BOXER)) {
            // set as boxer!
            pInRing[uiLoop]->uiStatusFlags |= SOLDIER_BOXER;
          }
        }
        // start match!
        SetBoxingState(BOXING);
        gfLastBoxingMatchWonByPlayer = FALSE;

        // give the first turn to a randomly chosen boxer
        EnterCombatMode(pInRing[Random(2)]->bTeam);
      }
    }
  /*
  else
  {
          // check to see if the player has more than one person in the ring
          if ( ubPlayersInRing > 1 )
          {
                  // boxing match just became invalid!
                  BoxingPlayerDisqualified( pNonBoxingPlayer, NON_BOXER_IN_RING
  ); return;
          }
  }
  */
}

bool CheckOnBoxers() {
  // repick boxer IDs every time
  if (!gBoxer[0]) {
    // get boxer soldier IDs!
    for (uint32_t i = 0; i != NUM_BOXERS; ++i) {
      SOLDIERTYPE *const s = WhoIsThere2(gsBoxerGridNo[i], 0);
      if (!s || FindObjClass(s, IC_WEAPON) != NO_SLOT) continue;
      // No weapon so this guy is a boxer
      gBoxer[i] = s;
    }
  }

  return gBoxer[0] || gBoxer[1] || gBoxer[2];
}

bool BoxerExists() {
  FOR_EACH(GridNo const, i, gsBoxerGridNo) {
    if (WhoIsThere2(*i, 0)) return true;
  }
  return false;
}

static void PickABoxer() {
  for (uint32_t i = 0; i != NUM_BOXERS; ++i) {
    SOLDIERTYPE *const boxer = gBoxer[i];
    if (!boxer) continue;

    if (gfBoxerFought[i]) {  // pathetic attempt to prevent multiple AI boxers
      boxer->uiStatusFlags &= ~SOLDIER_BOXER;
    } else if (boxer->bActive && boxer->bInSector && boxer->bLife >= OKLIFE) {  // Pick this boxer
      boxer->uiStatusFlags |= SOLDIER_BOXER;
      SetSoldierNonNeutral(boxer);
      RecalculateOppCntsDueToNoLongerNeutral(boxer);
      CancelAIAction(boxer);
      RESETTIMECOUNTER(boxer->AICounter, 0);
      gfBoxerFought[i] = TRUE;
      // Improve stats based on the # of rests these guys have had
      boxer->bStrength = std::min(100, boxer->bStrength + gubBoxersRests * 5);
      boxer->bDexterity = std::min(100, boxer->bDexterity + gubBoxersRests * 5);
      boxer->bAgility = std::min(100, boxer->bAgility + gubBoxersRests * 5);
      boxer->bLifeMax = std::min(100, boxer->bLifeMax + gubBoxersRests * 5);
      // Give the 3rd boxer martial arts
      if (i == NUM_BOXERS - 1 && boxer->ubBodyType == REGMALE) {
        boxer->ubSkillTrait1 = MARTIALARTS;
      }
      break;
    }
  }
}

bool BoxerAvailable() {
  // No way around this, BoxerAvailable will have to go find boxer IDs if they
  // aren't set.
  if (!CheckOnBoxers()) return false;

  for (uint8_t i = 0; i != NUM_BOXERS; ++i) {
    if (gBoxer[i] && !gfBoxerFought[i]) return true;
  }
  return false;
}

// NOTE THIS IS NOW BROKEN BECAUSE NPC.C ASSUMES THAT BOXERSAVAILABLE < 3 IS A
// SEQUEL FIGHT.   Maybe we could check Kingpin's location instead!
static uint8_t BoxersAvailable() {
  uint8_t ubLoop;
  uint8_t ubCount = 0;

  for (ubLoop = 0; ubLoop < NUM_BOXERS; ubLoop++) {
    if (gBoxer[ubLoop] != NULL && !gfBoxerFought[ubLoop]) ubCount++;
  }

  return (ubCount);
}

BOOLEAN AnotherFightPossible() {
  // Check that and a boxer is still available and
  // a player has at least OKLIFE + 5 life

  // and at least one fight HAS occurred
  uint8_t ubAvailable;

  ubAvailable = BoxersAvailable();

  if (ubAvailable == NUM_BOXERS || ubAvailable == 0) {
    return (FALSE);
  }

  CFOR_EACH_IN_TEAM(s, OUR_TEAM) {
    if (s->bInSector && s->bLife > OKLIFE + 5 && !s->bCollapsed) {
      return TRUE;
    }
  }

  return (FALSE);
}

void BoxingMovementCheck(SOLDIERTYPE *pSoldier) {
  if (GetRoom(pSoldier->sGridNo) == BOXING_RING) {
    // someone moving in/into the ring
    CountPeopleInBoxingRingAndDoActions();
  } else if ((gTacticalStatus.bBoxingState == BOXING) &&
             (pSoldier->uiStatusFlags & SOLDIER_BOXER)) {
    // boxer stepped out of the ring!
    BoxingPlayerDisqualified(pSoldier, BOXER_OUT_OF_RING);
    // add the history record here.
    AddHistoryToPlayersLog(HISTORY_DISQUALIFIED_BOXING, pSoldier->ubProfile, GetWorldTotalMin(),
                           gWorldSectorX, gWorldSectorY);
    // make not a boxer any more
    pSoldier->uiStatusFlags &= ~(SOLDIER_BOXER);
    pSoldier->uiStatusFlags &= (~SOLDIER_PCUNDERAICONTROL);
  }
}

void SetBoxingState(int8_t bNewState) {
  if (gTacticalStatus.bBoxingState == NOT_BOXING) {
    if (bNewState != NOT_BOXING) {
      // pause time
      PauseGame();
    }

  } else {
    if (bNewState == NOT_BOXING) {
      // unpause time
      UnPauseGame();

      if (BoxersAvailable() == NUM_BOXERS) {
        // set one boxer to be set as boxed so that the game will allow another
        // fight to occur
        gfBoxerFought[0] = TRUE;
      }
    }
  }
  gTacticalStatus.bBoxingState = bNewState;
}

void ClearAllBoxerFlags() { FOR_EACH_MERC(i)(*i)->uiStatusFlags &= ~SOLDIER_BOXER; }
