// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#define NOMINMAX

#include "SGP/Video.h"

#include <algorithm>
#include <cstring>
#include <ctime>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdexcept>
#include <stdio.h>

#include "FadeScreen.h"
#include "Local.h"
#include "Macro.h"
#include "SGP/Debug.h"
#include "SGP/FileMan.h"
#include "SGP/Font.h"
#include "SGP/HImage.h"
#include "SGP/Input.h"
#include "SGP/MemMan.h"
#include "SGP/PlatformIO.h"
#include "SGP/Types.h"
#include "SGP/VObjectBlitters.h"
#include "SGP/VSurface.h"
#include "TileEngine/RenderDirty.h"
#include "TileEngine/RenderWorld.h"
#include "Utils/TimerControl.h"
#include "jplatform_time.h"
#include "jplatform_video.h"

#define BUFFER_READY 0x00
#define BUFFER_DIRTY 0x02

#define MAX_CURSOR_WIDTH 64
#define MAX_CURSOR_HEIGHT 64

#define MAX_DIRTY_REGIONS 128

#define VIDEO_OFF 0x00
#define VIDEO_ON 0x01
#define VIDEO_SUSPENDED 0x04

#define MAX_NUM_FRAMES 25

static BOOLEAN gfVideoCapture = FALSE;
static uint32_t guiFramePeriod = 1000 / 15;
static uint32_t guiLastFrame;
static uint16_t *gpFrameData[MAX_NUM_FRAMES];
static int32_t giNumFrames = 0;

// Globals for mouse cursor
static uint16_t gusMouseCursorWidth;
static uint16_t gusMouseCursorHeight;
static int16_t gsMouseCursorXOffset;
static int16_t gsMouseCursorYOffset;

static JRect MouseBackground = {0, 0, 0, 0};

// Refresh thread based variables
static uint32_t guiFrameBufferState;   // BUFFER_READY, BUFFER_DIRTY
static uint32_t guiVideoManagerState;  // VIDEO_ON, VIDEO_OFF, VIDEO_SUSPENDED

// Dirty rectangle management variables
static JRect DirtyRegions[MAX_DIRTY_REGIONS];
static uint32_t guiDirtyRegionCount;
static BOOLEAN gfForceFullScreenRefresh;

static JRect DirtyRegionsEx[MAX_DIRTY_REGIONS];
static uint32_t guiDirtyRegionExCount;

// Screen output stuff
static BOOLEAN gfPrintFrameBuffer;
static uint32_t guiPrintFrameBufferIndex;

static struct JSurface *MouseCursor;
static struct JSurface *FrameBuffer;
static struct JSurface *ScreenBuffer;

struct JVideoState *g_videoState = NULL;

static void RecreateBackBuffer();
static void DeletePrimaryVideoSurfaces();

void VideoToggleFullScreen() { JVideo_ToggleFullScreen(g_videoState); }

void InitializeVideoManager() {
  g_videoState = JVideo_Init(APPLICATION_NAME, SCREEN_WIDTH, SCREEN_HEIGHT);

  SGPRect_set(&ClippingRect, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);

  ScreenBuffer = JSurface_Create16bpp(SCREEN_WIDTH, SCREEN_HEIGHT);

  if (ScreenBuffer == NULL) {
    // SLOGE(TAG, "JSurface_Create16bpp for ScreenBuffer failed: %s\n",
  }

  FrameBuffer = JSurface_Create16bpp(SCREEN_WIDTH, SCREEN_HEIGHT);

  if (FrameBuffer == NULL) {
    // SLOGE(TAG, "JSurface_Create16bpp for FrameBuffer failed: %s\n",
  }

  MouseCursor = JSurface_Create16bpp(MAX_CURSOR_WIDTH, MAX_CURSOR_HEIGHT);
  JSurface_SetColorKey(MouseCursor, 0);

  if (MouseCursor == NULL) {
    // SLOGE(TAG, "JSurface_Create16bpp for MouseCursor failed: %s\n",
  }

  JVideo_HideCursor();

  // Initialize state variables
  guiFrameBufferState = BUFFER_DIRTY;
  guiVideoManagerState = VIDEO_ON;
  guiDirtyRegionCount = 0;
  gfForceFullScreenRefresh = TRUE;
  gfPrintFrameBuffer = FALSE;
  guiPrintFrameBufferIndex = 0;
}

void ShutdownVideoManager() {
  DebugMsg(TOPIC_VIDEO, DBG_LEVEL_0, "Shutting down the video manager");

  guiVideoManagerState = VIDEO_OFF;

  // ATE: Release mouse cursor!
  FreeMouseCursor();
}

void SuspendVideoManager() { guiVideoManagerState = VIDEO_SUSPENDED; }

BOOLEAN RestoreVideoManager() {
#if 1  // XXX TODO
  UNIMPLEMENTED;
  return false;
#else
  // Make sure the video manager is indeed suspended before moving on

  if (guiVideoManagerState == VIDEO_SUSPENDED) {
    // Set the video state to VIDEO_ON

    guiFrameBufferState = BUFFER_DIRTY;
    gfForceFullScreenRefresh = TRUE;
    guiVideoManagerState = VIDEO_ON;
    return TRUE;
  } else {
    return FALSE;
  }
#endif
}

void InvalidateRegion(int32_t iLeft, int32_t iTop, int32_t iRight, int32_t iBottom) {
  if (gfForceFullScreenRefresh) {
    // There's no point in going on since we are forcing a full screen refresh
    return;
  }

  if (guiDirtyRegionCount < MAX_DIRTY_REGIONS) {
    // Well we haven't broken the MAX_DIRTY_REGIONS limit yet, so we register
    // the new region

    // DO SOME PREMIMARY CHECKS FOR VALID RECTS
    if (iLeft < 0) iLeft = 0;
    if (iTop < 0) iTop = 0;

    if (iRight > SCREEN_WIDTH) iRight = SCREEN_WIDTH;
    if (iBottom > SCREEN_HEIGHT) iBottom = SCREEN_HEIGHT;

    if (iRight - iLeft <= 0) return;
    if (iBottom - iTop <= 0) return;

    DirtyRegions[guiDirtyRegionCount].x = iLeft;
    DirtyRegions[guiDirtyRegionCount].y = iTop;
    DirtyRegions[guiDirtyRegionCount].w = iRight - iLeft;
    DirtyRegions[guiDirtyRegionCount].h = iBottom - iTop;
    guiDirtyRegionCount++;
  } else {
    // The MAX_DIRTY_REGIONS limit has been exceeded. Therefore we arbitrarely
    // invalidate the entire screen and force a full screen refresh
    guiDirtyRegionExCount = 0;
    guiDirtyRegionCount = 0;
    gfForceFullScreenRefresh = TRUE;
  }
}

static void AddRegionEx(int32_t iLeft, int32_t iTop, int32_t iRight, int32_t iBottom);

void InvalidateRegionEx(int32_t iLeft, int32_t iTop, int32_t iRight, int32_t iBottom) {
  // Check if we are spanning the rectangle - if so slit it up!
  if (iTop <= gsVIEWPORT_WINDOW_END_Y && iBottom > gsVIEWPORT_WINDOW_END_Y) {
    // Add new top region
    AddRegionEx(iLeft, iTop, iRight, gsVIEWPORT_WINDOW_END_Y);

    // Add new bottom region
    AddRegionEx(iLeft, gsVIEWPORT_WINDOW_END_Y, iRight, iBottom);
  } else {
    AddRegionEx(iLeft, iTop, iRight, iBottom);
  }
}

static void AddRegionEx(int32_t iLeft, int32_t iTop, int32_t iRight, int32_t iBottom) {
  if (guiDirtyRegionExCount < MAX_DIRTY_REGIONS) {
    // DO SOME PRELIMINARY CHECKS FOR VALID RECTS
    if (iLeft < 0) iLeft = 0;
    if (iTop < 0) iTop = 0;

    if (iRight > SCREEN_WIDTH) iRight = SCREEN_WIDTH;
    if (iBottom > SCREEN_HEIGHT) iBottom = SCREEN_HEIGHT;

    if (iRight - iLeft <= 0) return;
    if (iBottom - iTop <= 0) return;

    DirtyRegionsEx[guiDirtyRegionExCount].x = iLeft;
    DirtyRegionsEx[guiDirtyRegionExCount].y = iTop;
    DirtyRegionsEx[guiDirtyRegionExCount].w = iRight - iLeft;
    DirtyRegionsEx[guiDirtyRegionExCount].h = iBottom - iTop;
    guiDirtyRegionExCount++;
  } else {
    guiDirtyRegionExCount = 0;
    guiDirtyRegionCount = 0;
    gfForceFullScreenRefresh = TRUE;
  }
}

void InvalidateScreen() {
  // W A R N I N G ---- W A R N I N G ---- W A R N I N G ---- W A R N I N G ----
  // W A R N I N G ----
  //
  // This function is intended to be called by a thread which has already locked
  // the FRAME_BUFFER_MUTEX mutual exclusion section. Anything else will cause
  // the application to yack

  guiDirtyRegionCount = 0;
  guiDirtyRegionExCount = 0;
  gfForceFullScreenRefresh = TRUE;
  guiFrameBufferState = BUFFER_DIRTY;
}

static void ScrollJA2Background(int16_t sScrollXIncrement, int16_t sScrollYIncrement) {
  struct JSurface *Source =
      JSurface_Create16bpp(JSurface_Width(ScreenBuffer), JSurface_Height(ScreenBuffer));
  JRect SrcRect;
  JRect DstRect;
  JRect StripRegions[2];
  uint16_t NumStrips = 0;

  const uint16_t usWidth = SCREEN_WIDTH;
  const uint16_t usHeight = gsVIEWPORT_WINDOW_END_Y - gsVIEWPORT_WINDOW_START_Y;

  JSurface_Blit(ScreenBuffer, Source);

  if (sScrollXIncrement < 0) {
    SrcRect.x = 0;
    SrcRect.w = usWidth + sScrollXIncrement;
    DstRect.x = -sScrollXIncrement;
    StripRegions[0].x = gsVIEWPORT_START_X;
    StripRegions[0].y = gsVIEWPORT_WINDOW_START_Y;
    StripRegions[0].w = -sScrollXIncrement;
    StripRegions[0].h = usHeight;
    ++NumStrips;
  } else if (sScrollXIncrement > 0) {
    SrcRect.x = sScrollXIncrement;
    SrcRect.w = usWidth - sScrollXIncrement;
    DstRect.x = 0;
    StripRegions[0].x = gsVIEWPORT_END_X - sScrollXIncrement;
    StripRegions[0].y = gsVIEWPORT_WINDOW_START_Y;
    StripRegions[0].w = sScrollXIncrement;
    StripRegions[0].h = usHeight;
    ++NumStrips;
  } else {
    SrcRect.x = 0;
    SrcRect.w = usWidth;
    DstRect.x = 0;
  }

  if (sScrollYIncrement < 0) {
    SrcRect.y = gsVIEWPORT_WINDOW_START_Y;
    SrcRect.h = usHeight + sScrollYIncrement;
    DstRect.y = gsVIEWPORT_WINDOW_START_Y - sScrollYIncrement;
    StripRegions[NumStrips].x = DstRect.x;
    StripRegions[NumStrips].y = gsVIEWPORT_WINDOW_START_Y;
    StripRegions[NumStrips].w = SrcRect.w;
    StripRegions[NumStrips].h = -sScrollYIncrement;
    ++NumStrips;
  } else if (sScrollYIncrement > 0) {
    SrcRect.y = gsVIEWPORT_WINDOW_START_Y + sScrollYIncrement;
    SrcRect.h = usHeight - sScrollYIncrement;
    DstRect.y = gsVIEWPORT_WINDOW_START_Y;
    StripRegions[NumStrips].x = DstRect.x;
    StripRegions[NumStrips].y = gsVIEWPORT_WINDOW_END_Y - sScrollYIncrement;
    StripRegions[NumStrips].w = SrcRect.w;
    StripRegions[NumStrips].h = sScrollYIncrement;
    ++NumStrips;
  } else {
    SrcRect.y = gsVIEWPORT_WINDOW_START_Y;
    SrcRect.h = usHeight;
    DstRect.y = gsVIEWPORT_WINDOW_START_Y;
  }

  JSurface_BlitRectToRect(Source, ScreenBuffer, &SrcRect, &DstRect);
#if defined __GNUC__ && defined i386
  __asm__ __volatile__("cld");  // XXX HACK000D
#endif

  for (uint32_t i = 0; i < NumStrips; i++) {
    uint32_t x = StripRegions[i].x;
    uint32_t y = StripRegions[i].y;
    uint32_t w = StripRegions[i].w;
    uint32_t h = StripRegions[i].h;
    for (uint32_t j = y; j < y + h; ++j) {
      memset(gpZBuffer + j * SCREEN_WIDTH + x, 0, w * sizeof(*gpZBuffer));
    }

    RenderStaticWorldRect(x, y, x + w, y + h, TRUE);
    JSurface_BlitRectToRect(FrameBuffer, ScreenBuffer, &StripRegions[i], &StripRegions[i]);
  }

  // RESTORE SHIFTED
  RestoreShiftedVideoOverlays(sScrollXIncrement, sScrollYIncrement);

  // SAVE NEW
  SaveVideoOverlaysArea(BACKBUFFER);

  // BLIT NEW
  ExecuteVideoOverlaysToAlternateBuffer(BACKBUFFER);

  JRect r;
  r.x = gsVIEWPORT_START_X;
  r.y = gsVIEWPORT_WINDOW_START_Y;
  r.w = gsVIEWPORT_END_X - gsVIEWPORT_START_X;
  r.h = gsVIEWPORT_WINDOW_END_Y - gsVIEWPORT_WINDOW_START_Y;
  JVideo_RenderSufaceCopy(g_videoState, ScreenBuffer, &r);

  JSurface_Free(Source);
}

static void WriteTGAHeader(FILE *const f) {
  /*
   *  0 byte ID length
   *  1 byte colour map type
   *  2 byte targa type
   *  3 word colour map origin
   *  5 word colour map length
   *  7 byte colour map entry size
   *  8 word origin x
   * 10 word origin y
   * 12 word image width
   * 14 word image height
   * 16 byte bits per pixel
   * 17 byte image descriptor
   */
  static const uint8_t data[] = {0,
                                 0,
                                 2,
                                 0,
                                 0,
                                 0,
                                 0,
                                 0,
                                 0,
                                 0,
                                 0,
                                 0,
                                 SCREEN_WIDTH % 256,
                                 SCREEN_WIDTH / 256,
                                 SCREEN_HEIGHT % 256,
                                 SCREEN_HEIGHT / 256,
                                 PIXEL_DEPTH,
                                 0};
  fwrite(data, sizeof(data), 1, f);
}

/* Create a file for a screenshot, which is guaranteed not to exist yet. */
static FILE *CreateScreenshotFile() {
  const char *const exec_dir = FileMan::getExeFolderPath().c_str();
  do {
    char filename[2048];
    sprintf(filename, "%s/SCREEN%03d.TGA", exec_dir, guiPrintFrameBufferIndex++);
#ifndef _WIN32
#define O_BINARY 0
#endif
    int const fd = open3(filename, O_WRONLY | O_CREAT | O_EXCL | O_BINARY, 0644);
    if (fd >= 0) {
      FILE *const f = fdopen(fd, "wb");
      if (f == NULL) close(fd);
      return f;
    }
  } while (errno == EEXIST);
  return NULL;
}

static void TakeScreenshot() {
  FILE *const f = CreateScreenshotFile();
  if (!f) return;

  WriteTGAHeader(f);

  // If not 5/5/5, create buffer
  uint16_t *buf = MALLOCN(uint16_t, SCREEN_WIDTH);

  uint16_t const *src = static_cast<uint16_t const *>(JSurface_GetPixels(ScreenBuffer));
  for (int32_t y = SCREEN_HEIGHT - 1; y >= 0; --y) {
    if (buf) {  // ATE: Fix this such that it converts pixel format to 5/5/5
      memcpy(buf, src + y * SCREEN_WIDTH, SCREEN_WIDTH * sizeof(*buf));
      ConvertRGBDistribution565To555(buf, SCREEN_WIDTH);
      fwrite(buf, sizeof(*buf), SCREEN_WIDTH, f);
    } else {
      fwrite(src + y * SCREEN_WIDTH, SCREEN_WIDTH * 2, 1, f);
    }
  }

  if (buf) MemFree(buf);

  fclose(f);
}

static void SnapshotSmall();

void RefreshScreen() {
  if (guiVideoManagerState != VIDEO_ON) return;

#if DEBUG_PRINT_FPS
  {
    static int32_t prevSecond = 0;
    static int32_t fps = 0;

    int32_t currentSecond = time(NULL);
    if (currentSecond != prevSecond) {
      printf("fps: %d\n", fps);
      fps = 0;
      prevSecond = currentSecond;
    } else {
      fps++;
    }
  }
#endif

  JSurface_BlitRectToRect(FrameBuffer, ScreenBuffer, &MouseBackground, &MouseBackground);

  const BOOLEAN scrolling = (gsScrollXIncrement != 0 || gsScrollYIncrement != 0);

  if (guiFrameBufferState == BUFFER_DIRTY) {
    if (gfFadeInitialized && gfFadeInVideo) {
      gFadeFunction();
    } else {
      if (gfForceFullScreenRefresh) {
        JSurface_Blit(FrameBuffer, ScreenBuffer);
      } else {
        for (uint32_t i = 0; i < guiDirtyRegionCount; i++) {
          JSurface_BlitRectToRect(FrameBuffer, ScreenBuffer, &DirtyRegions[i], &DirtyRegions[i]);
        }

        for (uint32_t i = 0; i < guiDirtyRegionExCount; i++) {
          JRect *r = &DirtyRegionsEx[i];
          if (scrolling) {
            // Check if we are completely out of bounds
            if (r->y <= gsVIEWPORT_WINDOW_END_Y && r->y + r->h <= gsVIEWPORT_WINDOW_END_Y) {
              continue;
            }
          }
          JSurface_BlitRectToRect(FrameBuffer, ScreenBuffer, r, r);
        }
      }
    }
    if (scrolling) {
      ScrollJA2Background(gsScrollXIncrement, gsScrollYIncrement);
      gsScrollXIncrement = 0;
      gsScrollYIncrement = 0;
    }
    gfIgnoreScrollDueToCenterAdjust = FALSE;
    guiFrameBufferState = BUFFER_READY;
  }

  if (gfVideoCapture) {
    uint32_t uiTime = JTime_GetTicks();
    if (uiTime < guiLastFrame || uiTime > guiLastFrame + guiFramePeriod) {
      SnapshotSmall();
      guiLastFrame = uiTime;
    }
  }

  if (gfPrintFrameBuffer) {
    TakeScreenshot();
    gfPrintFrameBuffer = FALSE;
  }

  SGPPoint MousePos;
  GetMousePos(&MousePos);
  JRect src;
  src.x = 0;
  src.y = 0;
  src.w = gusMouseCursorWidth;
  src.h = gusMouseCursorHeight;
  JRect_set(&MouseBackground, MousePos.iX - gsMouseCursorXOffset,
            MousePos.iY - gsMouseCursorYOffset, gusMouseCursorWidth, gusMouseCursorHeight);
  JSurface_BlitRectToRect(MouseCursor, ScreenBuffer, &src, &MouseBackground);

  JVideo_RenderAndPresent(g_videoState, ScreenBuffer);

  gfForceFullScreenRefresh = FALSE;
  guiDirtyRegionCount = 0;
  guiDirtyRegionExCount = 0;
}

void SetMouseCursorProperties(int16_t sOffsetX, int16_t sOffsetY, uint16_t usCursorHeight,
                              uint16_t usCursorWidth) {
  gsMouseCursorXOffset = sOffsetX;
  gsMouseCursorYOffset = sOffsetY;
  gusMouseCursorWidth = usCursorWidth;
  gusMouseCursorHeight = usCursorHeight;
}

void EndFrameBufferRender() { guiFrameBufferState = BUFFER_DIRTY; }

void PrintScreen() { gfPrintFrameBuffer = TRUE; }

/*******************************************************************************
 * SnapshotSmall
 *
 * Grabs a screen from the primary surface, and stuffs it into a 16-bit
 * (RGB 5,5,5), uncompressed Targa file. Each time the routine is called, it
 * increments the file number by one. The files are create in the current
 * directory, usually the EXE directory. This routine produces 1/4 sized images.
 *
 ******************************************************************************/

static void RefreshMovieCache();

static void SnapshotSmall() {
  // Get the write pointer
  const uint16_t *pVideo = (uint16_t *)JSurface_GetPixels(ScreenBuffer);

  uint16_t *pDest = gpFrameData[giNumFrames];

  for (int32_t iCountY = SCREEN_HEIGHT - 1; iCountY >= 0; iCountY--) {
    for (int32_t iCountX = 0; iCountX < SCREEN_WIDTH; iCountX++) {
      pDest[iCountY * SCREEN_WIDTH + iCountX] = pVideo[iCountY * SCREEN_WIDTH + iCountX];
    }
  }

  giNumFrames++;

  if (giNumFrames == MAX_NUM_FRAMES) RefreshMovieCache();
}

void VideoCaptureToggle() {}

static void RefreshMovieCache() {
  static uint32_t uiPicNum = 0;

  PauseTime(TRUE);

  const char *ExecDir = FileMan::getExeFolderPath().c_str();

  for (int32_t cnt = 0; cnt < giNumFrames; cnt++) {
    char cFilename[2048];
    sprintf(cFilename, "%s/JA%5.5d.TGA", ExecDir, uiPicNum++);

    FILE *disk = fopen(cFilename, "wb");
    if (disk == NULL) return;

    WriteTGAHeader(disk);

    uint16_t *pDest = gpFrameData[cnt];

    for (int32_t iCountY = SCREEN_HEIGHT - 1; iCountY >= 0; iCountY -= 1) {
      for (int32_t iCountX = 0; iCountX < SCREEN_WIDTH; iCountX++) {
        fwrite(pDest + iCountY * SCREEN_WIDTH + iCountX, sizeof(uint16_t), 1, disk);
      }
    }

    fclose(disk);
  }

  PauseTime(FALSE);

  giNumFrames = 0;
}

static void RecreateBackBuffer() {
  SGPVSurface *newBackbuffer = new SGPVSurface(ScreenBuffer);

  if (g_back_buffer != NULL) {
    ReplaceFontBackBuffer(g_back_buffer, newBackbuffer);

    delete g_back_buffer;
    g_back_buffer = NULL;
  }

  g_back_buffer = newBackbuffer;
}

static void SetPrimaryVideoSurfaces() {
  // Delete surfaces if they exist
  DeletePrimaryVideoSurfaces();

  RecreateBackBuffer();

  g_mouse_buffer = new SGPVSurfaceAuto(MouseCursor);
  g_frame_buffer = new SGPVSurfaceAuto(FrameBuffer);
}

static void DeletePrimaryVideoSurfaces() {
  delete g_back_buffer;
  g_back_buffer = NULL;

  delete g_frame_buffer;
  g_frame_buffer = NULL;

  delete g_mouse_buffer;
  g_mouse_buffer = NULL;
}

SGPVSurface *gpVSurfaceHead = 0;

void InitializeVideoSurfaceManager() {
  // Shouldn't be calling this if the video surface manager already exists.
  // Call shutdown first...
  Assert(gpVSurfaceHead == NULL);
  gpVSurfaceHead = NULL;

  // Create primary and backbuffer from globals
  SetPrimaryVideoSurfaces();
}

void ShutdownVideoSurfaceManager() {
  DebugMsg(TOPIC_VIDEOSURFACE, DBG_LEVEL_0, "Shutting down the Video Surface manager");

  // Delete primary viedeo surfaces
  DeletePrimaryVideoSurfaces();

  while (gpVSurfaceHead) {
    delete gpVSurfaceHead;
  }
}
