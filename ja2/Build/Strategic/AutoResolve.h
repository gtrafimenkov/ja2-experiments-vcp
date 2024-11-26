#ifndef __AUTO_RESOLVE_H
#define __AUTO_RESOLVE_H

#include "JA2Types.h"
#include "ScreenIDs.h"
#include "Tactical/ItemTypes.h"

void EnterAutoResolveMode(UINT8 ubSectorX, UINT8 ubSectorY);

// is the autoresolve active?
BOOLEAN IsAutoResolveActive();

void EliminateAllEnemies(UINT8 ubSectorX, UINT8 ubSectorY);

void ConvertTacticalBattleIntoStrategicAutoResolveBattle();

UINT8 GetAutoResolveSectorID();

extern BOOLEAN gfTransferTacticalOppositionToAutoResolve;

// Returns TRUE if autoresolve is active or a sector is loaded.
BOOLEAN GetCurrentBattleSectorXYZ(INT16 *psSectorX, INT16 *psSectorY, INT16 *psSectorZ);

UINT32 VirtualSoldierDressWound(SOLDIERTYPE *pSoldier, SOLDIERTYPE *pVictim, OBJECTTYPE *pKit,
                                INT16 sKitPts, INT16 sStatus);

// Returns TRUE if a battle is happening ONLY
BOOLEAN GetCurrentBattleSectorXYZAndReturnTRUEIfThereIsABattle(INT16 *psSectorX, INT16 *psSectorY,
                                                               INT16 *psSectorZ);

ScreenID AutoResolveScreenHandle();

#endif
