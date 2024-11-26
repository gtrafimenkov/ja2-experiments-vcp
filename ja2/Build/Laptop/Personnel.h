#ifndef __PERSONNEL_H
#define __PERSONNEL_H

#include "JA2Types.h"

void GameInitPersonnel();
void EnterPersonnel();
void ExitPersonnel();
void HandlePersonnel();
void RenderPersonnel();

// add character to:

// leaving for odd reasons
void AddCharacterToOtherList(SOLDIERTYPE *pSoldier);

// killed and removed
void AddCharacterToDeadList(SOLDIERTYPE *pSoldier);

// simply fired...but alive
void AddCharacterToFiredList(SOLDIERTYPE *pSoldier);

BOOLEAN RemoveNewlyHiredMercFromPersonnelDepartedList(uint8_t ubProfile);

#endif
