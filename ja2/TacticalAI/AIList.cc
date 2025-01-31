// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#include "TacticalAI/AIList.h"

#include <stdexcept>
#include <string.h>

#include "Macro.h"
#include "SGP/Debug.h"
#include "Tactical/AnimationData.h"
#include "Tactical/Interface.h"
#include "Tactical/OppList.h"
#include "Tactical/Overhead.h"
#include "TacticalAI/AI.h"
#include "TacticalAI/AIInternals.h"

#define MAX_AI_PRIORITY 100

struct AILIST {
  SOLDIERTYPE *soldier;
  int8_t bPriority;
  AILIST *pNext;
};

static AILIST gAIList[TOTAL_SOLDIERS];
static AILIST *gpFirstAIListEntry;

static void DeleteAIListEntry(AILIST *pEntry) {
  pEntry->soldier = NULL;
  pEntry->bPriority = 0;
  pEntry->pNext = NULL;
}

static void ClearAIList() {
  memset(gAIList, 0, sizeof(gAIList));
  gpFirstAIListEntry = 0;
}

static AILIST *CreateNewAIListEntry(SOLDIERTYPE *const s, int8_t const priority) {
  FOR_EACH(AILIST, i, gAIList) {
    AILIST &e = *i;
    if (e.soldier) continue;
    e.soldier = s;
    e.bPriority = priority;
    e.pNext = 0;
    return &e;
  }
  throw std::runtime_error("out of AI list entries");
}

static bool SatisfiesAIListConditions(SOLDIERTYPE &, uint8_t *n_done, bool do_random_checks);

SOLDIERTYPE *RemoveFirstAIListEntry() {
  while (AILIST *const e = gpFirstAIListEntry) {
    gpFirstAIListEntry = e->pNext;
    SOLDIERTYPE &s = *e->soldier;
    DeleteAIListEntry(e);
    // Make sure conditions still met
    if (SatisfiesAIListConditions(s, 0, false)) return &s;
  }
  return 0;
}

static void RemoveAIListEntryForID(SOLDIERTYPE const *const s) {
  for (AILIST **anchor = &gpFirstAIListEntry; *anchor; anchor = &(*anchor)->pNext) {
    AILIST *const e = *anchor;
    if (e->soldier != s) continue;
    *anchor = e->pNext;
    DeleteAIListEntry(e);
    return;
  }
  // none found, that's okay
}

static void InsertIntoAIList(SOLDIERTYPE *const s, int8_t const priority) {
  AILIST *const e = CreateNewAIListEntry(s, priority);
  // Look through the list to see where to insert the entry
  for (AILIST **anchor = &gpFirstAIListEntry;; anchor = &(*anchor)->pNext) {
    if (*anchor && priority <= (*anchor)->bPriority) continue;
    e->pNext = *anchor;
    *anchor = e;
    return;
  }
}

static bool SatisfiesAIListConditions(SOLDIERTYPE &s, uint8_t *const n_done,
                                      bool const do_random_checks) {
  if (gTacticalStatus.bBoxingState == BOXING && !(s.uiStatusFlags & SOLDIER_BOXER)) return false;
  if (!s.bActive) return false;
  if (!s.bInSector) return false;
  if (s.bLife < OKLIFE) return false;
  if (s.bBreath < OKBREATH) return false;

  if (s.bMoved) {
    if (s.bActionPoints <= 1 && n_done) ++*n_done;
    return false;
  }

  // Do we need to re-evaluate alert status here?
  DecideAlertStatus(&s);

  /* If we are dealing with the civ team and this person hasn't heard any
   * gunfire, handle only 1 time in 10 */
  if (IsOnCivTeam(&s)) {
    // Don't handle cows and crows in TB!
    if (s.ubBodyType == CROW || s.ubBodyType == COW) return false;

    /* If someone in a civ group is neutral but the civ group is non-neutral,
     * should be handled all the time */
    if (s.bNeutral && (s.ubCivilianGroup == NON_CIV_GROUP ||
                       gTacticalStatus.fCivGroupHostile[s.ubCivilianGroup] == CIV_GROUP_NEUTRAL)) {
      if (s.bAlertStatus < STATUS_RED) {  // Unalerted, barely handle
        if (do_random_checks && PreRandom(10) != 0 && !s.ubQuoteRecord) return false;
      } else {  // Heard gunshots
        if (s.uiStatusFlags & SOLDIER_COWERING) {
          if (!s.bVisible) return false;  // never handle
          // If have profile, don't handle, don't want them running around
          if (s.ubProfile != NO_PROFILE) return false;
          // Else don't handle much
          if (do_random_checks && PreRandom(3) != 0) return false;
        } else if (s.ubBodyType == CRIPPLECIV) {  // Don't handle much
          if (do_random_checks && PreRandom(3) != 0) return false;
        }
      }
    }
    // non-neutral civs should be handled all the time, right?

    // Reset last action if cowering
    if (s.uiStatusFlags & SOLDIER_COWERING) s.bLastAction = AI_ACTION_NONE;
  }

  return true;
}

bool MoveToFrontOfAIList(SOLDIERTYPE *const s) {
  if (!SatisfiesAIListConditions(*s, 0, false)) return false;  // Cannot do that

  RemoveAIListEntryForID(s);

  /* We have to fake this guy's alert status (in the list) to be the same as the
   * current front of the list */
  AILIST *const head = gpFirstAIListEntry;
  int8_t const priority = head ? head->bPriority : MAX_AI_PRIORITY;
  AILIST *const e = CreateNewAIListEntry(s, priority);
  e->pNext = head;
  gpFirstAIListEntry = e;
  return true;
}

bool BuildAIListForTeam(int8_t const team) {
  // This team is being given control so reset their muzzle flashes
  TurnOffTeamsMuzzleFlashes(team);

  ClearAIList();

  // Create a new list
  uint8_t n = 0;
  uint8_t n_done = 0;
  FOR_EACH_MERC(i) {
    SOLDIERTYPE &s = **i;
    // non-null merc slot ensures active
    if (s.bTeam != team) continue;
    if (!SatisfiesAIListConditions(s, &n_done, true)) continue;

    int8_t priority = s.bAlertStatus;
    if (s.bVisible == TRUE) priority += 3;

    InsertIntoAIList(&s, priority);
    ++n;
  }

  if (n > 0) InitEnemyUIBar(n, n_done);
  return n > 0;
}
