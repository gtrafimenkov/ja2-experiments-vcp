// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#include "Laptop/AIMArchives.h"

#include <algorithm>
#include <string.h>

#include "Directories.h"
#include "Laptop/AIM.h"
#include "Laptop/Laptop.h"
#include "Macro.h"
#include "SGP/ButtonSystem.h"
#include "SGP/Font.h"
#include "SGP/VObject.h"
#include "SGP/VSurface.h"
#include "SGP/Video.h"
#include "Utils/Cursors.h"
#include "Utils/EncryptedFile.h"
#include "Utils/FontControl.h"
#include "Utils/Text.h"
#include "Utils/WordWrap.h"

#define AIM_ALUMNI_NAME_FILE BINARYDATADIR "/alumname.edt"
#define AIM_ALUMNI_FILE BINARYDATADIR "/alumni.edt"

#define AIM_ALUMNI_TITLE_FONT FONT14ARIAL
#define AIM_ALUMNI_TITLE_COLOR AIM_GREEN

#define AIM_ALUMNI_POPUP_FONT FONT10ARIAL
#define AIM_ALUMNI_POPUP_COLOR FONT_MCOLOR_WHITE

#define AIM_ALUMNI_POPUP_NAME_FONT FONT12ARIAL
#define AIM_ALUMNI_POPUP_NAME_COLOR FONT_MCOLOR_WHITE

#define AIM_ALUMNI_NAME_FONT FONT12ARIAL
#define AIM_ALUMNI_NAME_COLOR FONT_MCOLOR_WHITE
#define AIM_ALUMNI_PAGE_FONT FONT14ARIAL
#define AIM_ALUMNI_PAGE_COLOR_UP FONT_MCOLOR_DKWHITE
#define AIM_ALUMNI_PAGE_COLOR_DOWN 138

#define OLD_MERCS_COUNT 51
#define AIM_ALUMNI_NUM_FACE_COLS 5
#define AIM_ALUMNI_NUM_FACE_ROWS 4
#define MAX_NUMBER_OLD_MERCS_ON_PAGE (AIM_ALUMNI_NUM_FACE_ROWS * AIM_ALUMNI_NUM_FACE_COLS)

#define AIM_ALUMNI_START_GRID_X LAPTOP_SCREEN_UL_X + 37
#define AIM_ALUMNI_START_GRID_Y LAPTOP_SCREEN_WEB_UL_Y + 68

#define AIM_ALUMNI_GRID_OFFSET_X 90
#define AIM_ALUMNI_GRID_OFFSET_Y 72

#define AIM_ALUMNI_ALUMNI_FRAME_WIDTH 66
#define AIM_ALUMNI_ALUMNI_FRAME_HEIGHT 64

#define AIM_ALUMNI_ALUMNI_FACE_WIDTH 56
#define AIM_ALUMNI_ALUMNI_FACE_HEIGHT 50

#define AIM_ALUMNI_NAME_OFFSET_X 5
#define AIM_ALUMNI_NAME_OFFSET_Y 55
#define AIM_ALUMNI_NAME_WIDTH AIM_ALUMNI_ALUMNI_FRAME_WIDTH - AIM_ALUMNI_NAME_OFFSET_X * 2

#define AIM_ALUMNI_PAGE1_X LAPTOP_SCREEN_UL_X + 100
#define AIM_ALUMNI_PAGE1_Y LAPTOP_SCREEN_WEB_UL_Y + 357
#define AIM_ALUMNI_PAGE_GAP BOTTOM_BUTTON_START_WIDTH + 25

#define AIM_ALUMNI_TITLE_X IMAGE_OFFSET_X + 149
#define AIM_ALUMNI_TITLE_Y AIM_SYMBOL_Y + AIM_SYMBOL_SIZE_Y  // + 2
#define AIM_ALUMNI_TITLE_WIDTH AIM_SYMBOL_WIDTH

#define AIM_POPUP_WIDTH 309
#define AIM_POPUP_TEXT_WIDTH 296
#define AIM_POPUP_SECTION_HEIGHT 9

#define AIM_POPUP_X LAPTOP_SCREEN_UL_X + (500 - AIM_POPUP_WIDTH) / 2
#define AIM_POPUP_Y 120 + LAPTOP_SCREEN_WEB_DELTA_Y

#define AIM_POPUP_SHADOW_GAP 4

#define AIM_ALUMNI_FACE_PANEL_X AIM_POPUP_X + 6
#define AIM_ALUMNI_FACE_PANEL_Y AIM_POPUP_Y + 6
#define AIM_ALUMNI_FACE_PANEL_WIDTH 58
#define AIM_ALUMNI_FACE_PANEL_HEIGHT 52

#define AIM_ALUMNI_POPUP_NAME_X AIM_ALUMNI_FACE_PANEL_X + AIM_ALUMNI_FACE_PANEL_WIDTH + 10
#define AIM_ALUMNI_POPUP_NAME_Y AIM_ALUMNI_FACE_PANEL_Y + 20

#define AIM_ALUMNI_POPUP_DESC_X AIM_POPUP_X + 8
#define AIM_ALUMNI_POPUP_DESC_Y AIM_ALUMNI_FACE_PANEL_Y + AIM_ALUMNI_FACE_PANEL_HEIGHT + 5

#define AIM_ALUMNI_DONE_X AIM_POPUP_X + AIM_POPUP_WIDTH - AIM_ALUMNI_DONE_WIDTH - 7
#define AIM_ALUMNI_DONE_WIDTH 36
#define AIM_ALUMNI_DONE_HEIGHT 16

#define AIM_ALUMNI_NAME_SIZE 80
#define AIM_ALUMNI_DECRIPTION_SIZE 80 * 7
#define AIM_ALUMNI_FILE_RECORD_SIZE 80 * 8
#define AIM_ALUMNI_FULL_NAME_SIZE 80

static SGPVObject *guiAlumniFrame;
static SGPVObject *guiOldAim;
static SGPVObject *guiAlumniPopUp;
static SGPVObject *guiPopUpPic;
static SGPVObject *guiDoneButton;

static uint8_t gubPageNum;
static uint8_t gubDrawOldMerc;
static uint8_t gfDrawPopUpBox = FALSE;
static BOOLEAN gfDestroyPopUpBox;
static BOOLEAN gfFaceMouseRegionsActive;
static BOOLEAN gfReDrawScreen = FALSE;

static BOOLEAN AimArchivesSubPagesVisitedFlag[3] = {0, 0, 0};

// Face regions
static MOUSE_REGION gMercAlumniFaceMouseRegions[MAX_NUMBER_OLD_MERCS_ON_PAGE];

// Done region
static MOUSE_REGION gDoneRegion;

// Previous Button
static BUTTON_PICS *guiAlumniPageButtonImage;
static GUIButtonRef guiAlumniPageButton[3];

void EnterInitAimArchives() {
  gfDrawPopUpBox = FALSE;
  gfDestroyPopUpBox = FALSE;

  memset(&AimArchivesSubPagesVisitedFlag, 0, 3);
  AimArchivesSubPagesVisitedFlag[0] = TRUE;
}

static void BtnAlumniPageButtonCallback(GUI_BUTTON *btn, int32_t reason);
static void DisableAimArchiveButton();
static void InitAlumniFaceRegions();

void EnterAimArchives() {
  gfDrawPopUpBox = FALSE;
  gfDestroyPopUpBox = FALSE;

  InitAimDefaults();
  InitAimMenuBar();

  gubPageNum = (uint8_t)giCurrentSubPage;

  guiAlumniFrame = AddVideoObjectFromFile(LAPTOPDIR "/alumniframe.sti");
  guiOldAim = AddVideoObjectFromFile(LAPTOPDIR "/old_aim.sti");  // 1st set of faces
  guiPopUpPic = AddVideoObjectFromFile(LAPTOPDIR "/popuppicframe.sti");
  guiAlumniPopUp = AddVideoObjectFromFile(LAPTOPDIR "/alumnipopup.sti");
  guiDoneButton = AddVideoObjectFromFile(LAPTOPDIR "/donebutton.sti");

  InitAlumniFaceRegions();

  guiAlumniPageButtonImage = LoadButtonImage(LAPTOPDIR "/bottombuttons2.sti", 0, 1);

  int16_t x = AIM_ALUMNI_PAGE1_X;
  int16_t const y = AIM_ALUMNI_PAGE1_Y;
  for (int32_t i = 0; i < 3; ++i) {
    GUIButtonRef const btn = CreateIconAndTextButton(
        guiAlumniPageButtonImage, AimAlumniText[i], AIM_ALUMNI_PAGE_FONT, AIM_ALUMNI_PAGE_COLOR_UP,
        DEFAULT_SHADOW, AIM_ALUMNI_PAGE_COLOR_DOWN, DEFAULT_SHADOW, x, y, MSYS_PRIORITY_HIGH,
        BtnAlumniPageButtonCallback);
    guiAlumniPageButton[i] = btn;
    btn->SetCursor(CURSOR_WWW);
    btn->SetUserData(i);
    x += AIM_ALUMNI_PAGE_GAP;
  }

  DisableAimArchiveButton();
  RenderAimArchives();
}

static void CreateDestroyDoneMouseRegion(uint16_t usPosY);
static void RemoveAimAlumniFaceRegion();

void ExitAimArchives() {
  DeleteVideoObject(guiAlumniFrame);
  DeleteVideoObject(guiOldAim);
  DeleteVideoObject(guiAlumniPopUp);
  DeleteVideoObject(guiPopUpPic);
  DeleteVideoObject(guiDoneButton);

  RemoveAimAlumniFaceRegion();

  for (size_t i = 0; i < 3; ++i) RemoveButton(guiAlumniPageButton[i]);
  UnloadButtonImage(guiAlumniPageButtonImage);

  RemoveAimDefaults();
  ExitAimMenuBar();
  giCurrentSubPage = gubPageNum;

  CreateDestroyDoneMouseRegion(0);
  gfDestroyPopUpBox = FALSE;
  gfDrawPopUpBox = FALSE;
}

void HandleAimArchives() {
  if (gfReDrawScreen) {
    //		RenderAimArchives();
    fPausedReDrawScreenFlag = TRUE;

    gfReDrawScreen = FALSE;
  }
  if (gfDestroyPopUpBox) {
    gfDestroyPopUpBox = FALSE;

    CreateDestroyDoneMouseRegion(0);
    InitAlumniFaceRegions();
    gfDestroyPopUpBox = FALSE;
  }
}

static void DisplayAlumniOldMercPopUp();

void RenderAimArchives() {
  DrawAimDefaults();
  DisableAimButton();

  // Draw Link Title
  DrawTextToScreen(AimAlumniText[AIM_ALUMNI_ALUMNI], AIM_ALUMNI_TITLE_X, AIM_ALUMNI_TITLE_Y,
                   AIM_ALUMNI_TITLE_WIDTH, AIM_ALUMNI_TITLE_FONT, AIM_ALUMNI_TITLE_COLOR,
                   FONT_MCOLOR_BLACK, CENTER_JUSTIFIED);

  // Draw the mug shot border and face
  int32_t const start = gubPageNum * MAX_NUMBER_OLD_MERCS_ON_PAGE;
  int32_t const n_faces = std::min(OLD_MERCS_COUNT - start, MAX_NUMBER_OLD_MERCS_ON_PAGE);
  int32_t face_idx = start;
  for (int32_t i = 0; i != n_faces; ++i, ++face_idx) {
    int32_t const x =
        i % AIM_ALUMNI_NUM_FACE_COLS * AIM_ALUMNI_GRID_OFFSET_X + AIM_ALUMNI_START_GRID_X;
    int32_t const y =
        i / AIM_ALUMNI_NUM_FACE_COLS * AIM_ALUMNI_GRID_OFFSET_Y + AIM_ALUMNI_START_GRID_Y;

    BltVideoObject(FRAME_BUFFER, guiOldAim, face_idx, x + 4,
                   y + 4);  // Blt face to screen
    BltVideoObject(FRAME_BUFFER, guiAlumniFrame, 0, x,
                   y);  // Blt the alumni frame background

    // Display the merc's name
    wchar_t sText[AIM_ALUMNI_NAME_SIZE];
    LoadEncryptedDataFromFile(AIM_ALUMNI_NAME_FILE, sText, AIM_ALUMNI_NAME_SIZE * face_idx,
                              AIM_ALUMNI_NAME_SIZE);
    DrawTextToScreen(sText, x + AIM_ALUMNI_NAME_OFFSET_X, y + AIM_ALUMNI_NAME_OFFSET_Y,
                     AIM_ALUMNI_NAME_WIDTH, AIM_ALUMNI_NAME_FONT, AIM_ALUMNI_NAME_COLOR,
                     FONT_MCOLOR_BLACK, CENTER_JUSTIFIED);
  }

  if (gfDrawPopUpBox) {
    DisplayAlumniOldMercPopUp();
    RemoveAimAlumniFaceRegion();
  }

  MarkButtonsDirty();
  RenderWWWProgramTitleBar();
  InvalidateRegion(LAPTOP_SCREEN_UL_X, LAPTOP_SCREEN_WEB_UL_Y, LAPTOP_SCREEN_LR_X,
                   LAPTOP_SCREEN_WEB_LR_Y);
}

static void SelectAlumniFaceRegionCallBack(MOUSE_REGION *pRegion, int32_t iReason) {
  if (iReason & MSYS_CALLBACK_REASON_LBUTTON_UP) {
    gfDrawPopUpBox = TRUE;
    gfReDrawScreen = TRUE;

    gubDrawOldMerc = (uint8_t)MSYS_GetRegionUserData(pRegion, 0);
  }
}

static void ChangingAimArchiveSubPage(uint8_t ubSubPageNumber);
static void ResetAimArchiveButtons();

static void BtnAlumniPageButtonCallback(GUI_BUTTON *btn, int32_t reason) {
  uint8_t const ubRetValue = btn->GetUserData();

  if (reason & MSYS_CALLBACK_REASON_LBUTTON_UP) {
    RemoveAimAlumniFaceRegion();
    ChangingAimArchiveSubPage(ubRetValue);
    gubPageNum = ubRetValue;
    gfReDrawScreen = TRUE;
    gfDestroyPopUpBox = TRUE;
    ResetAimArchiveButtons();
    DisableAimArchiveButton();
    gfDrawPopUpBox = FALSE;
  }
}

static void ResetAimArchiveButtons() {
  int i = 0;

  for (i = 0; i < 3; i++) {
    guiAlumniPageButton[i]->uiFlags &= ~BUTTON_CLICKED_ON;
  }
}

static void DisableAimArchiveButton() {
  if (gubPageNum < 3) {
    guiAlumniPageButton[gubPageNum]->uiFlags |= BUTTON_CLICKED_ON;
  }
}

static void DisplayAlumniOldMercPopUp() {
  uint8_t i, ubNumLines = 11;  // 17
  uint16_t usPosY;
  uint8_t ubNumDescLines;
  uint32_t uiStartLoc;
  uint16_t usStringPixLength;

  // Load the description
  wchar_t sDesc[AIM_ALUMNI_DECRIPTION_SIZE];
  uiStartLoc = AIM_ALUMNI_FILE_RECORD_SIZE * gubDrawOldMerc + AIM_ALUMNI_FULL_NAME_SIZE;
  LoadEncryptedDataFromFile(AIM_ALUMNI_FILE, sDesc, uiStartLoc, AIM_ALUMNI_DECRIPTION_SIZE);

  usStringPixLength = StringPixLength(sDesc, AIM_ALUMNI_POPUP_FONT);
  ubNumDescLines = (uint8_t)(usStringPixLength / AIM_POPUP_TEXT_WIDTH);

  ubNumLines += ubNumDescLines;

  usPosY = AIM_POPUP_Y;

  // draw top line of the popup background
  FRAME_BUFFER->ShadowRect(AIM_POPUP_X + AIM_POPUP_SHADOW_GAP, usPosY + AIM_POPUP_SHADOW_GAP,
                           AIM_POPUP_X + AIM_POPUP_WIDTH + AIM_POPUP_SHADOW_GAP,
                           usPosY + AIM_POPUP_SECTION_HEIGHT + AIM_POPUP_SHADOW_GAP - 1);
  BltVideoObject(FRAME_BUFFER, guiAlumniPopUp, 0, AIM_POPUP_X, usPosY);
  // draw mid section of the popup background
  usPosY += AIM_POPUP_SECTION_HEIGHT;
  for (i = 0; i < ubNumLines; i++) {
    FRAME_BUFFER->ShadowRect(AIM_POPUP_X + AIM_POPUP_SHADOW_GAP, usPosY + AIM_POPUP_SHADOW_GAP,
                             AIM_POPUP_X + AIM_POPUP_WIDTH + AIM_POPUP_SHADOW_GAP,
                             usPosY + AIM_POPUP_SECTION_HEIGHT + AIM_POPUP_SHADOW_GAP - 1);
    BltVideoObject(FRAME_BUFFER, guiAlumniPopUp, 1, AIM_POPUP_X, usPosY);
    usPosY += AIM_POPUP_SECTION_HEIGHT;
  }
  // draw the bottom line and done button
  FRAME_BUFFER->ShadowRect(AIM_POPUP_X + AIM_POPUP_SHADOW_GAP, usPosY + AIM_POPUP_SHADOW_GAP,
                           AIM_POPUP_X + AIM_POPUP_WIDTH + AIM_POPUP_SHADOW_GAP,
                           usPosY + AIM_POPUP_SECTION_HEIGHT + AIM_POPUP_SHADOW_GAP - 1);
  BltVideoObject(FRAME_BUFFER, guiAlumniPopUp, 2, AIM_POPUP_X, usPosY);
  BltVideoObject(FRAME_BUFFER, guiDoneButton, 0, AIM_ALUMNI_DONE_X,
                 usPosY - AIM_ALUMNI_DONE_HEIGHT);
  DrawTextToScreen(AimAlumniText[AIM_ALUMNI_DONE], AIM_ALUMNI_DONE_X + 1,
                   usPosY - AIM_ALUMNI_DONE_HEIGHT + 3, AIM_ALUMNI_DONE_WIDTH,
                   AIM_ALUMNI_POPUP_NAME_FONT, AIM_ALUMNI_POPUP_NAME_COLOR, FONT_MCOLOR_BLACK,
                   CENTER_JUSTIFIED);

  CreateDestroyDoneMouseRegion(usPosY);

  /// blt face panale and the mecs fce
  BltVideoObject(FRAME_BUFFER, guiPopUpPic, 0, AIM_ALUMNI_FACE_PANEL_X, AIM_ALUMNI_FACE_PANEL_Y);
  BltVideoObject(FRAME_BUFFER, guiOldAim, gubDrawOldMerc, AIM_ALUMNI_FACE_PANEL_X + 1,
                 AIM_ALUMNI_FACE_PANEL_Y + 1);

  // Load and display the name
  wchar_t sName[AIM_ALUMNI_FULL_NAME_SIZE];
  uiStartLoc = AIM_ALUMNI_FILE_RECORD_SIZE * gubDrawOldMerc;
  LoadEncryptedDataFromFile(AIM_ALUMNI_FILE, sName, uiStartLoc, AIM_ALUMNI_FULL_NAME_SIZE);

  DrawTextToScreen(sName, AIM_ALUMNI_POPUP_NAME_X, AIM_ALUMNI_POPUP_NAME_Y, 0,
                   AIM_ALUMNI_POPUP_NAME_FONT, AIM_ALUMNI_POPUP_NAME_COLOR, FONT_MCOLOR_BLACK,
                   LEFT_JUSTIFIED);

  // Display the description
  DisplayWrappedString(AIM_ALUMNI_POPUP_DESC_X, AIM_ALUMNI_POPUP_DESC_Y, AIM_POPUP_TEXT_WIDTH, 2,
                       AIM_ALUMNI_POPUP_FONT, AIM_ALUMNI_POPUP_COLOR, sDesc, FONT_MCOLOR_BLACK,
                       LEFT_JUSTIFIED);

  InvalidateRegion(LAPTOP_SCREEN_UL_X, LAPTOP_SCREEN_WEB_UL_Y, LAPTOP_SCREEN_LR_X,
                   LAPTOP_SCREEN_WEB_LR_Y);
}

static void InitAlumniFaceRegions() {
  if (gfFaceMouseRegionsActive) return;
  gfFaceMouseRegionsActive = TRUE;

  int32_t const start = gubPageNum * MAX_NUMBER_OLD_MERCS_ON_PAGE;
  int32_t const n_faces = std::min(OLD_MERCS_COUNT - start, MAX_NUMBER_OLD_MERCS_ON_PAGE);
  int32_t face_idx = start;
  uint16_t const w = AIM_ALUMNI_ALUMNI_FACE_WIDTH;
  uint16_t const h = AIM_ALUMNI_ALUMNI_FACE_HEIGHT;
  for (size_t i = 0; i != n_faces; ++i, ++face_idx) {
    uint16_t const x =
        i % AIM_ALUMNI_NUM_FACE_COLS * AIM_ALUMNI_GRID_OFFSET_X + AIM_ALUMNI_START_GRID_X;
    uint16_t const y =
        i / AIM_ALUMNI_NUM_FACE_COLS * AIM_ALUMNI_GRID_OFFSET_Y + AIM_ALUMNI_START_GRID_Y;
    MOUSE_REGION *const r = &gMercAlumniFaceMouseRegions[i];
    MSYS_DefineRegion(r, x, y, x + w, y + h, MSYS_PRIORITY_HIGH, CURSOR_WWW, MSYS_NO_CALLBACK,
                      SelectAlumniFaceRegionCallBack);
    MSYS_SetRegionUserData(r, 0, face_idx);
  }
}

static void RemoveAimAlumniFaceRegion() {
  if (!gfFaceMouseRegionsActive) return;
  gfFaceMouseRegionsActive = FALSE;

  size_t const start = gubPageNum * MAX_NUMBER_OLD_MERCS_ON_PAGE;
  size_t const n_faces =
      std::min((size_t)(OLD_MERCS_COUNT - start), (size_t)MAX_NUMBER_OLD_MERCS_ON_PAGE);
  for (size_t i = 0; i < n_faces; ++i) {
    MSYS_RemoveRegion(&gMercAlumniFaceMouseRegions[i]);
  }
}

static void SelectAlumniDoneRegionCallBack(MOUSE_REGION *pRegion, int32_t iReason);

static void CreateDestroyDoneMouseRegion(uint16_t usPosY) {
  static BOOLEAN DoneRegionCreated = FALSE;

  if ((!DoneRegionCreated) && (usPosY != 0)) {
    usPosY -= AIM_ALUMNI_DONE_HEIGHT;
    MSYS_DefineRegion(&gDoneRegion, AIM_ALUMNI_DONE_X - 2, usPosY,
                      (AIM_ALUMNI_DONE_X - 2 + AIM_ALUMNI_DONE_WIDTH),
                      (int16_t)(usPosY + AIM_ALUMNI_DONE_HEIGHT), MSYS_PRIORITY_HIGH, CURSOR_WWW,
                      MSYS_NO_CALLBACK, SelectAlumniDoneRegionCallBack);
    DoneRegionCreated = TRUE;
  }

  if (DoneRegionCreated && usPosY == 0) {
    MSYS_RemoveRegion(&gDoneRegion);
    DoneRegionCreated = FALSE;
  }
}

static void SelectAlumniDoneRegionCallBack(MOUSE_REGION *pRegion, int32_t iReason) {
  if (iReason & MSYS_CALLBACK_REASON_LBUTTON_UP) {
    gfDestroyPopUpBox = TRUE;
    gfDrawPopUpBox = FALSE;
    gfReDrawScreen = TRUE;
  }
}

static void ChangingAimArchiveSubPage(uint8_t ubSubPageNumber) {
  fLoadPendingFlag = TRUE;

  if (!AimArchivesSubPagesVisitedFlag[ubSubPageNumber]) {
    fConnectingToSubPage = TRUE;
    fFastLoadFlag = FALSE;

    AimArchivesSubPagesVisitedFlag[ubSubPageNumber] = TRUE;
  } else {
    fConnectingToSubPage = TRUE;
    fFastLoadFlag = TRUE;
  }
}
