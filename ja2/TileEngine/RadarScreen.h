// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#ifndef __RADAR_SCREEN_H
#define __RADAR_SCREEN_H

#include "SGP/Types.h"

void LoadRadarScreenBitmap(const char *filename);

// RADAR WINDOW DEFINES
#define RADAR_WINDOW_X 543
#define RADAR_WINDOW_TM_Y INTERFACE_START_Y + 13
#define RADAR_WINDOW_SM_Y INV_INTERFACE_START_Y + 13
#define RADAR_WINDOW_WIDTH 88
#define RADAR_WINDOW_HEIGHT 44

void InitRadarScreen();
void RenderRadarScreen();
void MoveRadarScreen();

// toggle rendering flag of radar screen
void ToggleRadarScreenRender();

// clear out the video object for the radar map
void ClearOutRadarMapImage();

// do we render the radar screen?..or the squad list?
extern BOOLEAN fRenderRadarScreen;

#endif
