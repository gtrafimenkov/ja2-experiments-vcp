// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#include "Laptop/IMPAttributeEntrance.h"

#include "Directories.h"
#include "Laptop/CharProfile.h"
#include "Laptop/IMPVideoObjects.h"
#include "Laptop/Laptop.h"
#include "SGP/ButtonSystem.h"
#include "SGP/Font.h"
#include "TileEngine/RenderDirty.h"
#include "Utils/Cursors.h"
#include "Utils/FontControl.h"
#include "Utils/Text.h"

// the buttons
static BUTTON_PICS *giIMPAttributeEntranceButtonImage[1];
static GUIButtonRef giIMPAttributeEntranceButton[1];

static void BtnIMPAttributeBeginCallback(GUI_BUTTON *btn, int32_t reason);

static void CreateIMPAttributeEntranceButtons();

void EnterIMPAttributeEntrance() { CreateIMPAttributeEntranceButtons(); }

void RenderIMPAttributeEntrance() {
  // the background
  RenderProfileBackGround();

  // avg merc indent
  RenderAvgMercIndentFrame(90, 40);
}

static void DestroyIMPAttributeEntranceButtons();

void ExitIMPAttributeEntrance() {
  // destroy the finish buttons
  DestroyIMPAttributeEntranceButtons();
}

void HandleIMPAttributeEntrance() {}

static void CreateIMPAttributeEntranceButtons() {
  // the begin button
  giIMPAttributeEntranceButtonImage[0] = LoadButtonImage(LAPTOPDIR "/button_2.sti", 0, 1);
  giIMPAttributeEntranceButton[0] = CreateIconAndTextButton(
      giIMPAttributeEntranceButtonImage[0], pImpButtonText[13], FONT12ARIAL, FONT_WHITE,
      DEFAULT_SHADOW, FONT_WHITE, DEFAULT_SHADOW, LAPTOP_SCREEN_UL_X + 136,
      LAPTOP_SCREEN_WEB_UL_Y + 314, MSYS_PRIORITY_HIGH, BtnIMPAttributeBeginCallback);

  giIMPAttributeEntranceButton[0]->SetCursor(CURSOR_WWW);
}

static void DestroyIMPAttributeEntranceButtons() {
  // this function will destroy the buttons needed for the IMP attrib enter page

  // the begin  button
  RemoveButton(giIMPAttributeEntranceButton[0]);
  UnloadButtonImage(giIMPAttributeEntranceButtonImage[0]);
}

static void BtnIMPAttributeBeginCallback(GUI_BUTTON *btn, int32_t reason) {
  if (reason & MSYS_CALLBACK_REASON_LBUTTON_UP) {
    iCurrentImpPage = IMP_ATTRIBUTE_PAGE;
    fButtonPendingFlag = TRUE;
  }
}
