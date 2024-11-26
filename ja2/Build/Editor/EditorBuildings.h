#ifndef __EDITORBUILDINGS_H
#define __EDITORBUILDINGS_H

#include "SGP/Types.h"

extern BOOLEAN fBuildingShowRoofs, fBuildingShowWalls, fBuildingShowRoomInfo;
extern UINT8 gubCurrRoomNumber;
extern UINT8 gubMaxRoomNumber;

void SetupTextInputForBuildings();
void ExtractAndUpdateBuildingInfo();

// Initialization routines
void GameInitEditorBuildingInfo();

// Selection method callbacks
// Building utility functions
void UpdateBuildingsInfo();
void KillBuilding(UINT32 iMapIndex);

struct BUILDINGLAYOUTNODE {
  BUILDINGLAYOUTNODE *next;
  INT16 sGridNo;
};

extern BUILDINGLAYOUTNODE *gpBuildingLayoutList;
extern INT16 gsBuildingLayoutAnchorGridNo;

// The first step is copying a building.  After that, it either must be pasted
// or moved.
void CopyBuilding(INT32 iMapIndex);
void MoveBuilding(INT32 iMapIndex);
void PasteBuilding(INT32 iMapIndex);
void DeleteBuildingLayout();

void ReplaceBuildingWithNewRoof(INT32 iMapIndex);
void UpdateWallsView();
void UpdateRoofsView();

void InitDoorEditing(INT32 iMapIndex);
void ExtractAndUpdateDoorInfo();
void KillDoorEditing();
void RenderDoorEditingWindow();

void AddLockedDoorCursors();
void RemoveLockedDoorCursors();
void FindNextLockedDoor();

extern BOOLEAN gfEditingDoor;

extern UINT16 usCurrentMode;

#endif
