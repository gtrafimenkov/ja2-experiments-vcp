// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#include "Utils/MercTextBox.h"

#include <stdexcept>

#include "Directories.h"
#include "Local.h"
#include "SGP/Font.h"
#include "SGP/HImage.h"
#include "SGP/MemMan.h"
#include "SGP/PODObj.h"
#include "SGP/VObject.h"
#include "SGP/VSurface.h"
#include "SGP/Video.h"
#include "TileEngine/RenderDirty.h"
#include "Utils/FontControl.h"
#include "Utils/WordWrap.h"

#define TEXT_POPUP_GAP_BN_LINES 10
#define TEXT_POPUP_FONT FONT12ARIAL
#define TEXT_POPUP_COLOR FONT_MCOLOR_WHITE

#define MERC_TEXT_FONT FONT12ARIAL
#define MERC_TEXT_COLOR FONT_MCOLOR_WHITE

#define MERC_TEXT_MIN_WIDTH 10
#define MERC_TEXT_POPUP_WINDOW_TEXT_OFFSET_X 10
#define MERC_TEXT_POPUP_WINDOW_TEXT_OFFSET_Y 10

#define MERC_BACKGROUND_WIDTH 350
#define MERC_BACKGROUND_HEIGHT 200

// both of the below are index by the enum for thier types - background and
// border in MercTextBox.h

// filenames for border popup .sti's
static char const *const zMercBorderPopupFilenames[] = {
    INTERFACEDIR "/tactpopup.sti", INTERFACEDIR "/tactredpopup.sti",
    INTERFACEDIR "/tactbluepopup.sti", INTERFACEDIR "/tactpopupmain.sti",
    INTERFACEDIR "/laptoppopup.sti"};

// filenames for background popup .pcx's
static char const *const zMercBackgroundPopupFilenames[] = {
    INTERFACEDIR "/tactpopupbackground.pcx",     INTERFACEDIR "/tactpopupwhitebackground.pcx",
    INTERFACEDIR "/tactpopupgreybackground.pcx", INTERFACEDIR "/tactpopupbackgroundmain.pcx",
    INTERFACEDIR "/laptoppopupbackground.pcx",   INTERFACEDIR "/imp_popup_background.pcx"};

struct MercPopUpBox {
  SGPVSurface *uiSourceBufferIndex;
  uint8_t ubBackgroundIndex;
  uint8_t ubBorderIndex;
  SGPVSurface *uiMercTextPopUpBackground;
  SGPVObject *uiMercTextPopUpBorder;
};

// the flags
static SGPVObject *guiBoxIcons;
static SGPVObject *guiSkullIcons;

void InitMercPopupBox() {
  guiBoxIcons = AddVideoObjectFromFile(INTERFACEDIR "/msgboxicons.sti");        // stop icon
  guiSkullIcons = AddVideoObjectFromFile(INTERFACEDIR "/msgboxiconskull.sti");  // skull icon
}

// Tactical Popup
static void LoadTextMercPopupImages(MercPopUpBox *const box,
                                    MercPopUpBackground const ubBackgroundIndex,
                                    MercPopUpBorder const ubBorderIndex) {
  // this function will load the graphics associated with the background and
  // border index values
  box->uiMercTextPopUpBackground =
      AddVideoSurfaceFromFile(zMercBackgroundPopupFilenames[ubBackgroundIndex]);
  box->uiMercTextPopUpBorder = AddVideoObjectFromFile(zMercBorderPopupFilenames[ubBorderIndex]);

  // so far so good, return successful
  box->ubBackgroundIndex = ubBackgroundIndex;
  box->ubBorderIndex = ubBorderIndex;
}

static void RemoveTextMercPopupImages(MercPopUpBox *const box) {
  DeleteVideoSurface(box->uiMercTextPopUpBackground);
  DeleteVideoObject(box->uiMercTextPopUpBorder);
}

void RenderMercPopUpBox(MercPopUpBox const *const box, int16_t const sDestX, int16_t const sDestY,
                        SGPVSurface *const buffer) {
  if (!box) return;

  // will render/transfer the image from the buffer in the data structure to the
  // buffer specified by user
  SGPVSurface *const vs = box->uiSourceBufferIndex;
  BltVideoSurface(buffer, vs, sDestX, sDestY, NULL);

  // Invalidate!
  if (buffer == FRAME_BUFFER) {
    InvalidateRegion(sDestX, sDestY, sDestX + vs->Width(), sDestY + vs->Height());
  }
}

static void GetMercPopupBoxFontColor(uint8_t ubBackgroundIndex, uint8_t *pubFontColor,
                                     uint8_t *pubFontShadowColor);

MercPopUpBox *PrepareMercPopupBox(MercPopUpBox *box, MercPopUpBackground const ubBackgroundIndex,
                                  MercPopUpBorder const ubBorderIndex, wchar_t const *const pString,
                                  uint16_t usWidth, uint16_t const usMarginX,
                                  uint16_t const usMarginTopY, uint16_t const usMarginBottomY,
                                  uint16_t *const pActualWidth, uint16_t *const pActualHeight,
                                  MercPopupBoxFlags const flags) {
  if (usWidth >= SCREEN_WIDTH) {
    throw std::logic_error("Tried to create too wide merc popup box");
  }

  if (usWidth <= MERC_TEXT_MIN_WIDTH) usWidth = MERC_TEXT_MIN_WIDTH;

  SGP::PODObj<MercPopUpBox> new_box(0);
  // check id value, if -1, box has not been inited yet
  if (!box) {
    // no box yet
    box = new_box.Allocate();
    LoadTextMercPopupImages(box, ubBackgroundIndex, ubBorderIndex);
  } else {
    // has been created already,
    // Check if these images are different
    if (ubBackgroundIndex != box->ubBackgroundIndex || ubBorderIndex != box->ubBorderIndex) {
      // Remove old, set new
      RemoveTextMercPopupImages(box);
      LoadTextMercPopupImages(box, ubBackgroundIndex, ubBorderIndex);
    }
  }

  uint16_t const usStringPixLength = StringPixLength(pString, TEXT_POPUP_FONT);
  uint16_t usTextWidth;
  if (usStringPixLength < usWidth - MERC_TEXT_POPUP_WINDOW_TEXT_OFFSET_X * 2) {
    usWidth = usStringPixLength + MERC_TEXT_POPUP_WINDOW_TEXT_OFFSET_X * 2;
    usTextWidth = usWidth - MERC_TEXT_POPUP_WINDOW_TEXT_OFFSET_X * 2 + 1;
  } else {
    usTextWidth = usWidth - MERC_TEXT_POPUP_WINDOW_TEXT_OFFSET_X * 2 + 1 - usMarginX;
  }

  uint16_t const usNumberVerticalPixels =
      IanWrappedStringHeight(usTextWidth, 2, TEXT_POPUP_FONT, pString);
  uint16_t usHeight = usNumberVerticalPixels + MERC_TEXT_POPUP_WINDOW_TEXT_OFFSET_Y * 2;

  // Add height for margins
  usHeight += usMarginTopY + usMarginBottomY;

  // Add width for margins
  usWidth += usMarginX * 2;

  // Add width for iconic...
  if (flags & (MERC_POPUP_PREPARE_FLAGS_STOPICON | MERC_POPUP_PREPARE_FLAGS_SKULLICON)) {
    // Make minimun height for box...
    if (usHeight < 45) usHeight = 45;
    usWidth += 35;
  }

  if (usWidth >= MERC_BACKGROUND_WIDTH) usWidth = MERC_BACKGROUND_WIDTH - 1;

  // make sure the area isnt bigger than the background texture
  if (usHeight >= MERC_BACKGROUND_HEIGHT) {
    throw std::logic_error("Tried to create too high merc popup box");
  }

  // Create a background video surface to blt the face onto
  SGPVSurface *const vs = AddVideoSurface(usWidth, usHeight, PIXEL_DEPTH);
  box->uiSourceBufferIndex = vs;

  *pActualWidth = usWidth;
  *pActualHeight = usHeight;

  if (flags & MERC_POPUP_PREPARE_FLAGS_TRANS_BACK) {
    // Zero with yellow,
    // Set source transparcenty
    vs->SetTransparency(FROMRGB(255, 255, 0));
    vs->Fill(rgb32_to_rgb565(FROMRGB(255, 255, 0)));
  } else {
    SGPBox const DestRect = {0, 0, usWidth, usHeight};
    BltVideoSurface(vs, box->uiMercTextPopUpBackground, 0, 0, &DestRect);
  }

  const SGPVObject *const hImageHandle = box->uiMercTextPopUpBorder;

  uint16_t usPosY = 0;
  // blit top row of images
  for (uint16_t i = TEXT_POPUP_GAP_BN_LINES; i < usWidth - TEXT_POPUP_GAP_BN_LINES;
       i += TEXT_POPUP_GAP_BN_LINES) {
    // TOP ROW
    BltVideoObject(vs, hImageHandle, 1, i, usPosY);
    // BOTTOM ROW
    BltVideoObject(vs, hImageHandle, 6, i, usHeight - TEXT_POPUP_GAP_BN_LINES + 6);
  }

  // blit the left and right row of images
  uint16_t usPosX = 0;
  for (uint16_t i = TEXT_POPUP_GAP_BN_LINES; i < usHeight - TEXT_POPUP_GAP_BN_LINES;
       i += TEXT_POPUP_GAP_BN_LINES) {
    BltVideoObject(vs, hImageHandle, 3, usPosX, i);
    BltVideoObject(vs, hImageHandle, 4, usPosX + usWidth - 4, i);
  }

  // blt the corner images for the row
  // top left
  BltVideoObject(vs, hImageHandle, 0, 0, usPosY);
  // top right
  BltVideoObject(vs, hImageHandle, 2, usWidth - TEXT_POPUP_GAP_BN_LINES, usPosY);
  // bottom left
  BltVideoObject(vs, hImageHandle, 5, 0, usHeight - TEXT_POPUP_GAP_BN_LINES);
  // bottom right
  BltVideoObject(vs, hImageHandle, 7, usWidth - TEXT_POPUP_GAP_BN_LINES,
                 usHeight - TEXT_POPUP_GAP_BN_LINES);

  // Icon if ness....
  if (flags & MERC_POPUP_PREPARE_FLAGS_STOPICON) {
    BltVideoObject(vs, guiBoxIcons, 0, 5, 4);
  }
  if (flags & MERC_POPUP_PREPARE_FLAGS_SKULLICON) {
    BltVideoObject(vs, guiSkullIcons, 0, 9, 4);
  }

  // Get the font and shadow colors
  uint8_t ubFontColor;
  uint8_t ubFontShadowColor;
  GetMercPopupBoxFontColor(ubBackgroundIndex, &ubFontColor, &ubFontShadowColor);

  SetFontShadow(ubFontShadowColor);
  SetFontDestBuffer(vs);

  // Display the text
  int16_t sDispTextXPos = MERC_TEXT_POPUP_WINDOW_TEXT_OFFSET_X + usMarginX;

  if (flags & (MERC_POPUP_PREPARE_FLAGS_STOPICON | MERC_POPUP_PREPARE_FLAGS_SKULLICON)) {
    sDispTextXPos += 30;
  }

  DisplayWrappedString(sDispTextXPos, MERC_TEXT_POPUP_WINDOW_TEXT_OFFSET_Y + usMarginTopY,
                       usTextWidth, 2, MERC_TEXT_FONT, ubFontColor, pString, FONT_MCOLOR_BLACK,
                       LEFT_JUSTIFIED);

  SetFontDestBuffer(FRAME_BUFFER);
  SetFontShadow(DEFAULT_SHADOW);

  new_box.Release();
  return box;
}

// Deletes the surface thats contains the border, background and the text.
void RemoveMercPopupBox(MercPopUpBox *const box) {
  if (!box) return;

  DeleteVideoSurface(box->uiSourceBufferIndex);

  // DEF Added 5/26
  // Delete the background and the border
  RemoveTextMercPopupImages(box);

  MemFree(box);
}

// Pass in the background index, and pointers to the font and shadow color
static void GetMercPopupBoxFontColor(uint8_t ubBackgroundIndex, uint8_t *pubFontColor,
                                     uint8_t *pubFontShadowColor) {
  switch (ubBackgroundIndex) {
    case BASIC_MERC_POPUP_BACKGROUND:
      *pubFontColor = TEXT_POPUP_COLOR;
      *pubFontShadowColor = DEFAULT_SHADOW;
      break;

    case WHITE_MERC_POPUP_BACKGROUND:
      *pubFontColor = 2;
      *pubFontShadowColor = FONT_MCOLOR_WHITE;
      break;

    case GREY_MERC_POPUP_BACKGROUND:
      *pubFontColor = 2;
      *pubFontShadowColor = NO_SHADOW;
      break;

    case LAPTOP_POPUP_BACKGROUND:
      *pubFontColor = TEXT_POPUP_COLOR;
      *pubFontShadowColor = DEFAULT_SHADOW;
      break;

    default:
      *pubFontColor = TEXT_POPUP_COLOR;
      *pubFontShadowColor = DEFAULT_SHADOW;
      break;
  }
}
