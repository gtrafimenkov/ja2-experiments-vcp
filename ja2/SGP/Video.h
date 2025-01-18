// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#ifndef VIDEO_H
#define VIDEO_H

#include "SGP/Types.h"

#define VIDEO_NO_CURSOR 0xFFFF

void VideoSetFullScreen(BOOLEAN enable);
void InitializeVideoManager();
void ShutdownVideoManager();
void SuspendVideoManager();
BOOLEAN RestoreVideoManager();
void InvalidateRegion(int32_t iLeft, int32_t iTop, int32_t iRight, int32_t iBottom);
void InvalidateScreen();
void EndFrameBufferRender();
void PrintScreen();

/* Toggle between fullscreen and window mode after initialising the video
 * manager */
void VideoToggleFullScreen();

void SetMouseCursorProperties(int16_t sOffsetX, int16_t sOffsetY, uint16_t usCursorHeight,
                              uint16_t usCursorWidth);

void VideoCaptureToggle();

void InvalidateRegionEx(int32_t iLeft, int32_t iTop, int32_t iRight, int32_t iBottom);

void RefreshScreen();

// Creates a list to contain video Surfaces
void InitializeVideoSurfaceManager();

// Deletes any video Surface placed into list
void ShutdownVideoSurfaceManager();

#endif
