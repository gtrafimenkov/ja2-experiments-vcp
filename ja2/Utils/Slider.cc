// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#include "Utils/Slider.h"

#include <algorithm>
#include <stdexcept>

#include "Directories.h"
#include "Local.h"
#include "Macro.h"
#include "SGP/Debug.h"
#include "SGP/HImage.h"
#include "SGP/Input.h"
#include "SGP/Line.h"
#include "SGP/MemMan.h"
#include "SGP/MouseSystem.h"
#include "SGP/Types.h"
#include "SGP/VObject.h"
#include "SGP/VSurface.h"
#include "SGP/Video.h"
#include "TileEngine/RenderDirty.h"
#include "TileEngine/SysUtil.h"

#define WHEEL_MOVE_FRACTION 32

#define SLIDER_VERTICAL 0x00000001

#define DEFUALT_SLIDER_SIZE 7

#define STEEL_SLIDER_WIDTH 42
#define STEEL_SLIDER_HEIGHT 25

struct SLIDER {
  uint16_t usPosX;
  uint16_t usPosY;
  uint16_t usWidth;
  uint16_t usHeight;
  uint16_t usNumberOfIncrements;
  SLIDER_CHANGE_CALLBACK SliderChangeCallback;

  uint16_t usCurrentIncrement;

  uint16_t usBackGroundColor;

  MOUSE_REGION ScrollAreaMouseRegion;

  uint16_t usCurrentSliderBoxPosition;

  SGPRect LastRect;

  uint32_t uiFlags;

  uint8_t ubSliderWidth;
  uint8_t ubSliderHeight;

  SLIDER *pNext;
  SLIDER *pPrev;
};

static SLIDER *pSliderHead = NULL;
static SLIDER *gpCurrentSlider = NULL;

static SGPVObject *guiSliderBoxImage;

void InitSlider() {
  // load Slider Box Graphic graphic and add it
  guiSliderBoxImage = AddVideoObjectFromFile(INTERFACEDIR "/sliderbox.sti");
}

void ShutDownSlider() {
  AssertMsg(guiSliderBoxImage != NULL,
            "Trying to ShutDown the Slider System when it was never inited");

  // Do a cehck to see if there are still active nodes
  for (SLIDER *i = pSliderHead; i != NULL;) {
    SLIDER *const remove = i;
    i = i->pNext;
    RemoveSliderBar(remove);

    // Report an error
  }

  // if so report an errror
  DeleteVideoObject(guiSliderBoxImage);
  guiSliderBoxImage = NULL;
}

static void CalculateNewSliderBoxPosition(SLIDER *pSlider);
static void SelectedSliderButtonCallBack(MOUSE_REGION *r, int32_t iReason);
static void SelectedSliderMovementCallBack(MOUSE_REGION *r, int32_t reason);

SLIDER *AddSlider(uint8_t ubStyle, uint16_t usCursor, uint16_t usPosX, uint16_t usPosY,
                  uint16_t usWidth, uint16_t usNumberOfIncrements, int8_t sPriority,
                  SLIDER_CHANGE_CALLBACK SliderChangeCallback) {
  AssertMsg(guiSliderBoxImage != NULL,
            "Trying to Add a Slider Bar when the Slider System was never inited");

  if (ubStyle >= NUM_SLIDER_STYLES) throw std::logic_error("Invalid slider style");

  SLIDER *const s = MALLOCZ(SLIDER);
  // Assign the settings to the current slider
  s->usPosX = usPosX;
  s->usPosY = usPosY;
  s->usNumberOfIncrements = usNumberOfIncrements;
  s->SliderChangeCallback = SliderChangeCallback;
  s->usCurrentIncrement = 0;
  s->usBackGroundColor = rgb32_to_rgb565(FROMRGB(255, 255, 255));

  uint16_t x = usPosX;
  uint16_t y = usPosY;
  uint16_t w;
  uint16_t h;
  switch (ubStyle) {
    case SLIDER_VERTICAL_STEEL:
      s->uiFlags = SLIDER_VERTICAL;
      s->usWidth = STEEL_SLIDER_WIDTH;
      s->usHeight = usWidth;
      s->ubSliderWidth = STEEL_SLIDER_WIDTH;
      s->ubSliderHeight = STEEL_SLIDER_HEIGHT;
      x -= s->usWidth / 2;
      w = s->usWidth;
      h = s->usHeight;
      break;

    default:
    case SLIDER_DEFAULT_STYLE:
      s->uiFlags = 0;
      s->usWidth = usWidth;
      s->usHeight = DEFUALT_SLIDER_SIZE;
      y -= DEFUALT_SLIDER_SIZE;
      w = s->usWidth;
      h = DEFUALT_SLIDER_SIZE * 2;
      break;
  }

  MOUSE_REGION *const r = &s->ScrollAreaMouseRegion;
  MSYS_DefineRegion(r, x, y, x + w, y + h, sPriority, usCursor, SelectedSliderMovementCallBack,
                    SelectedSliderButtonCallBack);
  r->SetUserPtr(s);

  // Add the slider into the list
  if (pSliderHead == NULL) {
    pSliderHead = s;
    s->pNext = NULL;
  } else {
    SLIDER *i = pSliderHead;
    while (i->pNext != NULL) i = i->pNext;

    i->pNext = s;
    s->pPrev = i;
    s->pNext = NULL;
  }

  CalculateNewSliderBoxPosition(s);

  return s;
}

static void CalculateNewSliderIncrement(SLIDER *s, uint16_t usPos);
static void RenderSelectedSliderBar(SLIDER *s);

void RenderAllSliderBars() {
  // set the currently selectd slider bar
  if (gfLeftButtonState && gpCurrentSlider != NULL) {
    SLIDER *const s = gpCurrentSlider;
    const uint16_t pos = gusMouseYPos < s->usPosY ? 0 : gusMouseYPos - s->usPosY;
    CalculateNewSliderIncrement(s, pos);
  } else {
    gpCurrentSlider = NULL;
  }

  for (SLIDER *i = pSliderHead; i != NULL; i = i->pNext) {
    RenderSelectedSliderBar(i);
  }
}

static void OptDisplayLine(uint16_t usStartX, uint16_t usStartY, uint16_t EndX, uint16_t EndY,
                           int16_t iColor);
static void RenderSliderBox(SLIDER *s);

static void RenderSelectedSliderBar(SLIDER *s) {
  if (!(s->uiFlags & SLIDER_VERTICAL)) {
    // Display the background (the bar)
    const uint16_t x = s->usPosX;
    const uint16_t y = s->usPosY;
    const uint16_t w = s->usWidth;
    const uint16_t c = s->usBackGroundColor;
    OptDisplayLine(x + 1, y - 1, x + w - 1, y - 1, c);
    OptDisplayLine(x, y, x + w, y, c);
    OptDisplayLine(x + 1, y + 1, x + w - 1, y + 1, c);

    InvalidateRegion(x, y - 2, x + w + 1, y + 2);
  }

  RenderSliderBox(s);
}

static void RenderSliderBox(SLIDER *s) {
  SGPRect DestRect;
  if (s->uiFlags & SLIDER_VERTICAL) {
    // fill out the settings for the current dest rect
    DestRect.iLeft = s->usPosX - s->ubSliderWidth / 2;
    DestRect.iTop = s->usCurrentSliderBoxPosition - s->ubSliderHeight / 2;
    DestRect.iRight = DestRect.iLeft + s->ubSliderWidth;
    DestRect.iBottom = DestRect.iTop + s->ubSliderHeight;

    if (s->LastRect.iTop == DestRect.iTop) return;

    // Restore the old rect
    BlitBufferToBuffer(guiSAVEBUFFER, FRAME_BUFFER, s->LastRect.iLeft, s->LastRect.iTop,
                       s->ubSliderWidth, s->ubSliderHeight);

    // invalidate the old area
    InvalidateRegion(s->LastRect.iLeft, s->LastRect.iTop, s->LastRect.iRight, s->LastRect.iBottom);

    BltVideoObject(FRAME_BUFFER, guiSliderBoxImage, 0, DestRect.iLeft, DestRect.iTop);

    // invalidate the area
    InvalidateRegion(DestRect.iLeft, DestRect.iTop, DestRect.iRight, DestRect.iBottom);
  } else {
    // fill out the settings for the current dest rect
    DestRect.iLeft = s->usCurrentSliderBoxPosition;
    DestRect.iTop = s->usPosY - DEFUALT_SLIDER_SIZE;
    DestRect.iRight = DestRect.iLeft + s->ubSliderWidth;
    DestRect.iBottom = DestRect.iTop + s->ubSliderHeight;

    if (s->LastRect.iLeft == DestRect.iLeft) return;

    // Restore the old rect
    BlitBufferToBuffer(guiSAVEBUFFER, FRAME_BUFFER, s->LastRect.iLeft, s->LastRect.iTop, 8, 15);

    BltVideoObject(FRAME_BUFFER, guiSliderBoxImage, 0, DestRect.iLeft, DestRect.iTop);

    // invalidate the area
    InvalidateRegion(DestRect.iLeft, DestRect.iTop, s->usCurrentSliderBoxPosition + 9,
                     s->usPosY + DEFUALT_SLIDER_SIZE);
  }

  // Save the new rect location
  s->LastRect = DestRect;
}

void RemoveSliderBar(SLIDER *s) {
  if (s == pSliderHead) pSliderHead = pSliderHead->pNext;

  if (s->pNext) s->pNext->pPrev = s->pPrev;
  if (s->pPrev) s->pPrev->pNext = s->pNext;

  MSYS_RemoveRegion(&s->ScrollAreaMouseRegion);
  MemFree(s);
}

static void SelectedSliderMovementCallBack(MOUSE_REGION *r, int32_t reason) {
  // if we already have an anchored slider bar
  if (gpCurrentSlider != NULL) return;

  if (!gfLeftButtonState) return;

  SLIDER *s;
  if (reason & MSYS_CALLBACK_REASON_LOST_MOUSE) {
    s = r->GetUserPtr<SLIDER>();

    // set the currently selectd slider bar
    gpCurrentSlider = s;
  } else if (reason & MSYS_CALLBACK_REASON_GAIN_MOUSE || reason & MSYS_CALLBACK_REASON_MOVE) {
    s = r->GetUserPtr<SLIDER>();
  } else {
    return;
  }

  const uint16_t pos = s->uiFlags & SLIDER_VERTICAL ? r->RelativeYPos : r->RelativeXPos;
  CalculateNewSliderIncrement(s, pos);
}

static void SetSliderPos(SLIDER *s, int32_t pos) {
  if (pos == s->usCurrentIncrement) return;
  s->usCurrentIncrement = pos;
  CalculateNewSliderBoxPosition(s);
  if (s->uiFlags & SLIDER_VERTICAL) pos = s->usNumberOfIncrements - pos;
  s->SliderChangeCallback(pos);
}

static void SelectedSliderButtonCallBack(MOUSE_REGION *r, int32_t iReason) {
  // if we already have an anchored slider bar
  if (gpCurrentSlider != NULL) return;

  if (iReason & MSYS_CALLBACK_REASON_LBUTTON_DWN || iReason & MSYS_CALLBACK_REASON_LBUTTON_REPEAT) {
    SLIDER *const s = r->GetUserPtr<SLIDER>();
    const uint16_t pos = s->uiFlags & SLIDER_VERTICAL ? r->RelativeYPos : r->RelativeXPos;
    CalculateNewSliderIncrement(s, pos);
  } else if (iReason & MSYS_CALLBACK_REASON_WHEEL_UP) {
    SLIDER *const s = r->GetUserPtr<SLIDER>();
    const int32_t step = (s->usNumberOfIncrements + WHEEL_MOVE_FRACTION - 1) / WHEEL_MOVE_FRACTION;
    int32_t pos = s->usCurrentIncrement - step;
    pos = std::max(0, pos);
    SetSliderPos(s, pos);
  } else if (iReason & MSYS_CALLBACK_REASON_WHEEL_DOWN) {
    SLIDER *const s = r->GetUserPtr<SLIDER>();
    const int32_t step = (s->usNumberOfIncrements + WHEEL_MOVE_FRACTION - 1) / WHEEL_MOVE_FRACTION;
    int32_t pos = s->usCurrentIncrement + step;
    pos = std::min(pos, (int32_t)s->usNumberOfIncrements);
    SetSliderPos(s, pos);
  }
}

static void CalculateNewSliderIncrement(SLIDER *s, uint16_t usPos) {
  int32_t pos;
  if (s->uiFlags & SLIDER_VERTICAL) {
    if (usPos <= s->usHeight) {
      pos = s->usNumberOfIncrements * usPos / s->usHeight;
    } else {
      pos = s->usNumberOfIncrements;
    }
  } else {
    pos = s->usNumberOfIncrements * usPos / s->usWidth;
  }

  SetSliderPos(s, pos);
}

static void OptDisplayLine(uint16_t usStartX, uint16_t usStartY, uint16_t EndX, uint16_t EndY,
                           int16_t iColor) {
  SGPVSurface::Lock l(FRAME_BUFFER);
  SetClippingRegionAndImageWidth(l.Pitch(), 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
  LineDraw(FALSE, usStartX, usStartY, EndX, EndY, iColor, l.Buffer<uint16_t>());
}

static void CalculateNewSliderBoxPosition(SLIDER *s) {
  uint16_t pos;
  uint16_t usMaxPos;
  if (s->uiFlags & SLIDER_VERTICAL) {
    // if the box is in the last position
    if (s->usCurrentIncrement >= s->usNumberOfIncrements) {
      pos = s->usPosY + s->usHeight;
    } else {
      pos = s->usPosY + s->usHeight * s->usCurrentIncrement / s->usNumberOfIncrements;
    }
    usMaxPos = s->usPosY + s->usHeight;
  } else {
    // if the box is in the last position
    if (s->usCurrentIncrement == s->usNumberOfIncrements) {
      pos = s->usPosX + s->usWidth - 8 + 1;  // - minus box width
    } else {
      pos = s->usPosX + s->usWidth * s->usCurrentIncrement / s->usNumberOfIncrements;
    }
    usMaxPos = s->usPosX + s->usWidth - 8 + 1;
  }

  // if the box is past the edge, move it back
  s->usCurrentSliderBoxPosition = std::min(pos, usMaxPos);
}

void SetSliderValue(SLIDER *s, uint32_t uiNewValue) {
  if (uiNewValue >= s->usNumberOfIncrements) return;

  if (s->uiFlags & SLIDER_VERTICAL) {
    s->usCurrentIncrement = s->usNumberOfIncrements - uiNewValue;
  } else {
    s->usCurrentIncrement = uiNewValue;
  }

  CalculateNewSliderBoxPosition(s);
}
