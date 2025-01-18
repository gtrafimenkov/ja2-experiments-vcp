// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#include "rust_geometry.h"
#include "Editor/MessageBox.h"

#include "Directories.h"
#include "Local.h"
#include "SGP/ButtonSystem.h"
#include "SGP/English.h"
#include "SGP/Font.h"
#include "SGP/Input.h"
#include "SGP/Video.h"
#include "Utils/FontControl.h"
#include "jplatform_input.h"

GUIButtonRef iMsgBoxBgrnd;
GUIButtonRef iMsgBoxOk;
GUIButtonRef iMsgBoxCancel;

BOOLEAN gfMessageBoxResult = FALSE;
uint8_t gubMessageBoxStatus = MESSAGEBOX_NONE;

static void MsgBoxCnclClkCallback(GUI_BUTTON *butn, int32_t reason);
static void MsgBoxOkClkCallback(GUI_BUTTON *butn, int32_t reason);

void CreateMessageBox(wchar_t const *const msg) {
  Font const font = gpLargeFontType1;
  int16_t w = StringPixLength(msg, font) + 10;
  int16_t const h = 96;
  if (w > 600) w = 600;

  int16_t const x = (SCREEN_WIDTH - w) / 2;
  int16_t const y = (SCREEN_HEIGHT - h) / 2;

  // Fake button for background with text
  iMsgBoxBgrnd =
      CreateLabel(msg, font, FONT_LTKHAKI, FONT_DKKHAKI, x, y, w, h, MSYS_PRIORITY_HIGHEST - 2);

  int16_t const bx = x + w / 2;
  int16_t const by = y + 58;
  iMsgBoxOk = QuickCreateButtonImg(EDITORDIR "/ok.sti", 0, 1, 2, 3, 4, bx - 35, by,
                                   MSYS_PRIORITY_HIGHEST - 1, MsgBoxOkClkCallback);
  iMsgBoxCancel = QuickCreateButtonImg(EDITORDIR "/cancel.sti", 0, 1, 2, 3, 4, bx + 5, by,
                                       MSYS_PRIORITY_HIGHEST - 1, MsgBoxCnclClkCallback);

  SGPRect msg_box_rect;
  msg_box_rect.iLeft = x;
  msg_box_rect.iTop = y;
  msg_box_rect.iRight = x + w;
  msg_box_rect.iBottom = y + h;
  RestrictMouseCursor(&msg_box_rect);

  gfMessageBoxResult = FALSE;
  gubMessageBoxStatus = MESSAGEBOX_WAIT;
}

BOOLEAN MessageBoxHandled() {
  InputAtom DummyEvent;

  while (DequeueEvent(&DummyEvent)) {
    if (DummyEvent.isKeyDown()) {
      switch (DummyEvent.getKey()) {
        case JIK_RETURN:
        case 'y':
          gubMessageBoxStatus = MESSAGEBOX_DONE;
          gfMessageBoxResult = TRUE;
          break;

        case JIK_ESCAPE:
        case 'n':
          gubMessageBoxStatus = MESSAGEBOX_DONE;
          gfMessageBoxResult = FALSE;
          break;
      }
    }
  }

  if (gubMessageBoxStatus == MESSAGEBOX_DONE) {
    while (DequeueEvent(&DummyEvent)) continue;
  }
  MarkButtonsDirty();
  RenderButtons();
  //	InvalidateScreen( );
  //	ExecuteBaseDirtyRectQueue();
  EndFrameBufferRender();
  return gubMessageBoxStatus == MESSAGEBOX_DONE;
}

void RemoveMessageBox() {
  FreeMouseCursor();
  RemoveButton(iMsgBoxCancel);
  RemoveButton(iMsgBoxOk);
  RemoveButton(iMsgBoxBgrnd);
  gubMessageBoxStatus = MESSAGEBOX_NONE;
}

static void MsgBoxOkClkCallback(GUI_BUTTON *butn, int32_t reason) {
  if (reason & MSYS_CALLBACK_REASON_LBUTTON_UP) {
    gubMessageBoxStatus = MESSAGEBOX_DONE;
    gfMessageBoxResult = TRUE;
  }
}

static void MsgBoxCnclClkCallback(GUI_BUTTON *butn, int32_t reason) {
  if (reason & MSYS_CALLBACK_REASON_LBUTTON_UP) {
    gubMessageBoxStatus = MESSAGEBOX_DONE;
    gfMessageBoxResult = FALSE;
  }
}
