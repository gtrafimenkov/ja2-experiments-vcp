// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#include "Laptop/IMPBeginScreen.h"

#include <string.h>
#include <wchar.h>

#include "Directories.h"
#include "Laptop/CharProfile.h"
#include "Laptop/IMPAttributeSelection.h"
#include "Laptop/IMPFinish.h"
#include "Laptop/IMPHomePage.h"
#include "Laptop/IMPMainPage.h"
#include "Laptop/IMPPortraits.h"
#include "Laptop/IMPVideoObjects.h"
#include "Laptop/Laptop.h"
#include "Laptop/LaptopSave.h"
#include "Local.h"
#include "Macro.h"
#include "MessageBoxScreen.h"
#include "SGP/ButtonSystem.h"
#include "SGP/English.h"
#include "SGP/Font.h"
#include "SGP/HImage.h"
#include "SGP/Line.h"
#include "SGP/VSurface.h"
#include "SGP/Video.h"
#include "ScreenIDs.h"
#include "Strategic/GameClock.h"
#include "Tactical/MercHiring.h"
#include "Tactical/SoldierProfileType.h"
#include "TileEngine/RenderDirty.h"
#include "Utils/Cursors.h"
#include "Utils/FontControl.h"
#include "Utils/Text.h"
#include "Utils/TextInput.h"
#include "Utils/TimerControl.h"
#include "jplatform_input.h"

#define FULL_NAME_CURSOR_Y LAPTOP_SCREEN_WEB_UL_Y + 138
#define NICK_NAME_CURSOR_Y LAPTOP_SCREEN_WEB_UL_Y + 195
#define MALE_BOX_X 2 + 192 + LAPTOP_SCREEN_UL_X
#define MALE_BOX_Y 254 + LAPTOP_SCREEN_WEB_UL_Y
#define MALE_BOX_WIDTH 24 - 2
#define MALE_BOX_HEIGHT 24 - 2
#define FEMALE_BOX_X 2 + 302 + LAPTOP_SCREEN_UL_X
#define FULL_NAME_REGION_WIDTH 230
#define NICK_NAME_REGION_WIDTH 100

// genders
enum {
  IMP_FEMALE = 0,
  IMP_MALE,
};

// TextEnterMode .. whether user is entering full name or nick name, or gender
// selection
enum {
  FULL_NAME_MODE,
  NICK_NAME_MODE,
  MALE_GENDER_SELECT,
  FEMALE_GENDER_SELECT,
};

// beginning character stats
wchar_t pFullNameString[NAME_LENGTH];
wchar_t pNickNameString[NICKNAME_LENGTH];

// positions in name strings
uint32_t uiFullNameCharacterPosition = 0;
uint32_t uiNickNameCharacterPosition = 0;

// non gender
int8_t bGenderFlag = -1;

// IMP begin page buttons
static BUTTON_PICS *giIMPBeginScreenButtonImage[1];
GUIButtonRef giIMPBeginScreenButton[1];

// current mode of entering text we are in, ie FULL or Nick name?
uint8_t ubTextEnterMode = 0;

// cursor position
uint32_t uiNickNameCursorPosition = 196 + LAPTOP_SCREEN_UL_X;
uint32_t uiFullNameCursorPosition = 196 + LAPTOP_SCREEN_UL_X;

// whther a new char has been entered ( to force redraw)
BOOLEAN fNewCharInString = FALSE;

static MOUSE_REGION gIMPBeginScreenMouseRegions[4];

static void CreateIMPBeginScreenButtons();
static void CreateIMPBeginScreenMouseRegions();

void EnterIMPBeginScreen() {
  // reset all variables

  memset(pFullNameString, 0, sizeof(pFullNameString));
  memset(pNickNameString, 0, sizeof(pNickNameString));

  // if we are not restarting...then copy over name, set cursor and array
  // positions
  if (iCurrentProfileMode != 0) {
    wcscpy(pFullNameString, pFullName);
    wcscpy(pNickNameString, pNickName);
    uiFullNameCharacterPosition = (uint32_t)wcslen(pFullNameString);
    uiNickNameCharacterPosition = (uint32_t)wcslen(pNickNameString);
    uiFullNameCursorPosition =
        196 + LAPTOP_SCREEN_UL_X + StringPixLength(pFullNameString, FONT14ARIAL);
    uiNickNameCursorPosition =
        196 + LAPTOP_SCREEN_UL_X + StringPixLength(pNickNameString, FONT14ARIAL);

    // set gender too
    bGenderFlag = fCharacterIsMale;

  } else {
    uiNickNameCursorPosition = 196 + LAPTOP_SCREEN_UL_X;
    uiFullNameCursorPosition = 196 + LAPTOP_SCREEN_UL_X;
    uiFullNameCharacterPosition = 0;
    uiNickNameCharacterPosition = 0;
    bGenderFlag = -1;
  }

  ubTextEnterMode = 0;

  // draw name if any
  fNewCharInString = TRUE;

  // render the screen on entry
  RenderIMPBeginScreen();

  if (fFinishedCharGeneration) {
    ubTextEnterMode = 5;
  } else {
    fFirstIMPAttribTime = TRUE;
  }

  // create mouse regions
  CreateIMPBeginScreenMouseRegions();

  // create buttons needed for begin screen
  CreateIMPBeginScreenButtons();
}

static void DisplayPlayerNameStrings();
static void Print8CharacterOnlyString();
static void RenderGender();

void RenderIMPBeginScreen() {
  // the background
  RenderProfileBackGround();

  // fourth button image 3X
  RenderButton4Image(64, 118);
  RenderButton4Image(64, 178);
  RenderButton4Image(64, 238);

  // the begin screen indents
  RenderBeginIndent(105, 58);

  // full name indent
  RenderNameIndent(194, 132);

  // nick name
  RenderNickNameIndent(194, 192);

  // render warning string
  Print8CharacterOnlyString();

  DisplayPlayerNameStrings();
  RenderGender();
}

static void DestroyIMPBeginScreenMouseRegions();
static void RemoveIMPBeginScreenButtons();

void ExitIMPBeginScreen() {
  // remove buttons
  RemoveIMPBeginScreenButtons();

  // remove mouse regions
  DestroyIMPBeginScreenMouseRegions();

  wcscpy(pFullName, pFullNameString);

  // is nick name too long?..shorten
  if (wcslen(pNickNameString) > 8) {
    // null out char 9
    pNickNameString[8] = 0;
  }

  wcscpy(pNickName, pNickNameString);

  // set gender
  fCharacterIsMale = bGenderFlag;
}

static void DisplayFemaleGlowCursor();
static void DisplayFullNameStringCursor();
static void DisplayMaleGlowCursor();
static void DisplayNickNameStringCursor();
static void GetPlayerKeyBoardInputForIMPBeginScreen();

void HandleIMPBeginScreen() {
  GetPlayerKeyBoardInputForIMPBeginScreen();

  // has a new char been added to activation string

  // render the cursor
  switch (ubTextEnterMode) {
    case FULL_NAME_MODE:
      DisplayFullNameStringCursor();
      break;
    case NICK_NAME_MODE:
      DisplayNickNameStringCursor();
      break;
    case MALE_GENDER_SELECT:
      DisplayMaleGlowCursor();
      break;
    case FEMALE_GENDER_SELECT:
      DisplayFemaleGlowCursor();
      break;
  }

  if (fNewCharInString) {
    DisplayPlayerNameStrings();
    RenderGender();
  }
}

static void BtnIMPBeginScreenDoneCallback(GUI_BUTTON *btn, int32_t reason);

static void CreateIMPBeginScreenButtons() {
  // this procedure will create the buttons needed for the IMP BeginScreen

  // ths done button
  giIMPBeginScreenButtonImage[0] = LoadButtonImage(LAPTOPDIR "/button_2.sti", 0, 1);
  giIMPBeginScreenButton[0] = CreateIconAndTextButton(
      giIMPBeginScreenButtonImage[0], pImpButtonText[6], FONT12ARIAL, FONT_WHITE, DEFAULT_SHADOW,
      FONT_WHITE, DEFAULT_SHADOW, LAPTOP_SCREEN_UL_X + 134, LAPTOP_SCREEN_WEB_UL_Y + 314,
      MSYS_PRIORITY_HIGH, BtnIMPBeginScreenDoneCallback);

  giIMPBeginScreenButton[0]->SetCursor(CURSOR_WWW);
}

static void RemoveIMPBeginScreenButtons() {
  // this procedure will destroy the already created buttosn for the IMP
  // BeginScreen

  // the done button
  RemoveButton(giIMPBeginScreenButton[0]);
  UnloadButtonImage(giIMPBeginScreenButtonImage[0]);
}

static BOOLEAN CheckCharacterInputForEgg();
static void CopyFirstNameIntoNickName();

static void BtnIMPBeginScreenDoneCallback(GUI_BUTTON *btn, int32_t reason) {
  // easter egg check
  BOOLEAN fEggOnYouFace = FALSE;

  if (reason & MSYS_CALLBACK_REASON_LBUTTON_UP) {
    if (fFinishedCharGeneration) {
      // simply reviewing name and gender, exit to finish page
      iCurrentImpPage = IMP_FINISH;
      fButtonPendingFlag = TRUE;
      return;
    } else {
      if (CheckCharacterInputForEgg()) {
        fEggOnYouFace = TRUE;
      }
    }

    // back to mainpage

    // check to see if a name has been selected, if not, do not allow player to
    // proceed with more char generation
    if (pFullNameString[0] != L'\0' && pFullNameString[0] != L' ' && bGenderFlag != -1) {
      // valid full name, check to see if nick name
      if (pNickNameString[0] == '\0' || pNickNameString[0] == L' ') {
        // no nick name
        // copy first name to nick name
        CopyFirstNameIntoNickName();
      }
      // ok, now set back to main page, and set the fact we have completed part
      // 1
      if (iCurrentProfileMode < 1 && bGenderFlag != -1) {
        iCurrentProfileMode = 1;
      } else if (bGenderFlag == -1) {
        iCurrentProfileMode = 0;
      }

      // no easter egg?...then proceed along
      if (!fEggOnYouFace) {
        iCurrentImpPage = IMP_MAIN_PAGE;
        fButtonPendingFlag = TRUE;
      }
    } else {
      // invalid name, reset current mode
      DoLapTopMessageBox(MSG_BOX_IMP_STYLE, pImpPopUpStrings[2], LAPTOP_SCREEN, MSG_BOX_FLAG_OK,
                         NULL);
      iCurrentProfileMode = 0;
    }
  }
}

static void EnterKey(InputAtom const &a, wchar_t *const buf, uint32_t const buf_len,
                     uint32_t &pos_char, uint32_t const max_w, uint32_t &pos_cursor) {
  switch (a.getKey()) {
    case JIK_BACKSPACE:  // Delete char left of cursor
    {
      if (pos_char == 0) break;
      wchar_t const c = buf[--pos_char];
      buf[pos_char] = L'\0';
      pos_cursor -= GetCharWidth(FONT14ARIAL, c);
      fNewCharInString = TRUE;
      break;
    }

    default:  // Append new char, if it is valid and there is space left
    {
      wchar_t const c = a.Char;
      if (IsPrintableChar(c)) {
        if (pos_char >= buf_len) break;
        uint32_t const new_pos_cursor = pos_cursor + GetCharWidth(FONT14ARIAL, c);
        if (new_pos_cursor > LAPTOP_SCREEN_UL_X + 196 + max_w) break;
        pos_cursor = new_pos_cursor;
        buf[pos_char] = c;
        buf[++pos_char] = L'\0';
        fNewCharInString = TRUE;
      }
      break;
    }
  }
}

static void HandleBeginScreenTextEvent(InputAtom const &a) {
  /* this function checks to see if a letter or a backspace was pressed, if so,
   * either put char to screen or delete it */
  switch (ubTextEnterMode) {
    case FULL_NAME_MODE:
      EnterKey(a, pFullNameString, NAME_LENGTH, uiFullNameCharacterPosition, FULL_NAME_REGION_WIDTH,
               uiFullNameCursorPosition);
      break;
    case NICK_NAME_MODE:
      EnterKey(a, pNickNameString, NICKNAME_LENGTH, uiNickNameCharacterPosition,
               NICK_NAME_REGION_WIDTH, uiNickNameCursorPosition);
      break;
  }
}

static void DecrementTextEnterMode();
static void IncrementTextEnterMode();

static void GetPlayerKeyBoardInputForIMPBeginScreen() {
  InputAtom InputEvent;

  // handle input events
  while (DequeueEvent(&InputEvent)) {
    if (!HandleTextInput(&InputEvent) && (InputEvent.usEvent != KEY_UP)) {
      switch (InputEvent.getKey()) {
        case JIK_RETURN:
          // check to see if gender was highlighted..if so, select it
          if (FEMALE_GENDER_SELECT == ubTextEnterMode) {
            bGenderFlag = IMP_FEMALE;
          } else if (MALE_GENDER_SELECT == ubTextEnterMode) {
            bGenderFlag = IMP_MALE;
          }

          // increment to next selection box
          IncrementTextEnterMode();
          fNewCharInString = TRUE;
          break;

        case JIK_SPACE:
          // handle space bar
          if (FEMALE_GENDER_SELECT == ubTextEnterMode) {
            bGenderFlag = IMP_FEMALE;
            DecrementTextEnterMode();
          } else if (MALE_GENDER_SELECT == ubTextEnterMode) {
            bGenderFlag = IMP_MALE;
            IncrementTextEnterMode();
          } else {
            HandleBeginScreenTextEvent(InputEvent);
          }
          fNewCharInString = TRUE;
          break;

        case JIK_ESCAPE:
          HandleLapTopESCKey();
          break;

        case JIK_TAB:
          // tab hit, increment to next selection box
          if (InputEvent.shift) {
            DecrementTextEnterMode();
          } else {
            IncrementTextEnterMode();
          }
          fNewCharInString = TRUE;
          break;

        default:
          HandleBeginScreenTextEvent(InputEvent);
          break;
      }
    }
  }
}

static uint16_t CurrentGlowColour() {
  static uint32_t uiBaseTime = 0;
  static uint32_t iCurrentState = 0;
  static BOOLEAN fIncrement = TRUE;

  if (uiBaseTime == 0) uiBaseTime = GetJA2Clock();

  // get difference
  uint32_t uiDeltaTime = GetJA2Clock() - uiBaseTime;

  // if difference is long enough, rotate colors
  if (uiDeltaTime > MIN_GLOW_DELTA) {
    if (iCurrentState == 10) fIncrement = FALSE;  // start rotating downward
    if (iCurrentState == 0) fIncrement = TRUE;    // rotate colors upward
    iCurrentState = iCurrentState + (fIncrement ? 1 : -1);
    // reset basetime to current clock
    uiBaseTime = GetJA2Clock();
  }

  return rgb32_to_rgb565(GlowColorsList[iCurrentState]);
}

static void DisplayNameStringCursor(int32_t x, int32_t y) {
  SGPVSurface::Lock l(FRAME_BUFFER);
  SetClippingRegionAndImageWidth(l.Pitch(), 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
  LineDraw(TRUE, x, y, x, y + CURSOR_HEIGHT + 1, CurrentGlowColour(), l.Buffer<uint16_t>());
  InvalidateRegion(x, y, x + 1, y + CURSOR_HEIGHT + 2);
}

static void DisplayFullNameStringCursor() {
  DisplayNameStringCursor(uiFullNameCursorPosition, FULL_NAME_CURSOR_Y - 3);
}

static void DisplayNickNameStringCursor() {
  DisplayNameStringCursor(uiNickNameCursorPosition, NICK_NAME_CURSOR_Y);
}

static void DisplayPlayerNameStrings() {
  // player gone too far, move back
  if (uiFullNameCharacterPosition >= NAME_LENGTH) {
    uiFullNameCharacterPosition = NAME_LENGTH;
  }
  if (uiNickNameCharacterPosition >= NICKNAME_LENGTH) {
    uiNickNameCharacterPosition = NICKNAME_LENGTH;
  }

  // restore background
  RenderNameIndent(194, 132);
  RenderNickNameIndent(194, 192);

  SetFontAttributes(FONT14ARIAL, 184);
  MPrint(LAPTOP_SCREEN_UL_X + 196, FULL_NAME_CURSOR_Y + 1, pFullNameString);
  MPrint(LAPTOP_SCREEN_UL_X + 196, NICK_NAME_CURSOR_Y + 4, pNickNameString);

  fNewCharInString = FALSE;
  fReDrawScreenFlag = TRUE;
}

static void DisplayGenderGlowCursor(int32_t x) {
  // this procdure will draw the activation string cursor on the screen at
  // position cursorx cursory
  SGPVSurface::Lock l(FRAME_BUFFER);
  SetClippingRegionAndImageWidth(l.Pitch(), 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
  RectangleDraw(TRUE, x, MALE_BOX_Y, x + MALE_BOX_WIDTH, MALE_BOX_Y + MALE_BOX_HEIGHT,
                CurrentGlowColour(), l.Buffer<uint16_t>());
  InvalidateRegion(x, MALE_BOX_Y, x + MALE_BOX_WIDTH + 1, MALE_BOX_Y + MALE_BOX_HEIGHT + 1);
}

static void DisplayMaleGlowCursor() { DisplayGenderGlowCursor(MALE_BOX_X); }

static void DisplayFemaleGlowCursor() { DisplayGenderGlowCursor(FEMALE_BOX_X); }

static void CopyFirstNameIntoNickName() {
  // this procedure will copy the characters first name in to the nickname for
  // the character
  uint32_t iCounter = 0;
  while ((pFullNameString[iCounter] != L' ') && (iCounter < NICKNAME_LENGTH) &&
         (pFullNameString[iCounter] != 0)) {
    // copy charcters into nick name
    pNickNameString[iCounter] = pFullNameString[iCounter];
    iCounter++;
  }
}

static void IncrementTextEnterMode() {
  // this function will incrment which text enter mode we are in, FULLname,
  // NICKname, IMP_MALE or IMP_FEMALE

  // if at IMP_FEMALE gender selection, reset to full name
  if (FEMALE_GENDER_SELECT == ubTextEnterMode) {
    ubTextEnterMode = FULL_NAME_MODE;
  } else {
    // otherwise, next selection
    ubTextEnterMode++;
  }
}

static void DecrementTextEnterMode() {
  // this function will incrment which text enter mode we are in, FULLname,
  // NICKname, IMP_MALE or IMP_FEMALE

  // if at IMP_FEMALE gender selection, reset to full name
  if (FULL_NAME_MODE == ubTextEnterMode) {
    ubTextEnterMode = FEMALE_GENDER_SELECT;
  } else {
    // otherwise, next selection
    ubTextEnterMode--;
  }
}

static void MvtOnFemaleRegionCallBack(MOUSE_REGION *pRegion, int32_t iReason);
static void MvtOnMaleRegionCallBack(MOUSE_REGION *pRegion, int32_t iReason);
static void SelectFemaleRegionCallBack(MOUSE_REGION *pRegion, int32_t iReason);
static void SelectFullNameRegionCallBack(MOUSE_REGION *pRegion, int32_t iReason);
static void SelectMaleRegionCallBack(MOUSE_REGION *pRegion, int32_t iReason);
static void SelectNickNameRegionCallBack(MOUSE_REGION *pRegion, int32_t iReason);

static void CreateIMPBeginScreenMouseRegions() {
  // this function creates the IMP mouse regions

  // are we only reviewing text?.. if so, do not create regions
  if (ubTextEnterMode == 5) return;

  // full name region
  MSYS_DefineRegion(&gIMPBeginScreenMouseRegions[0], LAPTOP_SCREEN_UL_X + 196,
                    LAPTOP_SCREEN_WEB_UL_Y + 135, LAPTOP_SCREEN_UL_X + 196 + FULL_NAME_REGION_WIDTH,
                    LAPTOP_SCREEN_WEB_UL_Y + 135 + 24, MSYS_PRIORITY_HIGH, CURSOR_WWW,
                    MSYS_NO_CALLBACK, SelectFullNameRegionCallBack);

  // nick name region
  MSYS_DefineRegion(&gIMPBeginScreenMouseRegions[1], LAPTOP_SCREEN_UL_X + 196,
                    LAPTOP_SCREEN_WEB_UL_Y + 195, LAPTOP_SCREEN_UL_X + 196 + NICK_NAME_REGION_WIDTH,
                    LAPTOP_SCREEN_WEB_UL_Y + 195 + 24, MSYS_PRIORITY_HIGH, CURSOR_WWW,
                    MSYS_NO_CALLBACK, SelectNickNameRegionCallBack);

  // IMP_MALE gender area
  MSYS_DefineRegion(&gIMPBeginScreenMouseRegions[2], MALE_BOX_X, MALE_BOX_Y,
                    MALE_BOX_X + MALE_BOX_WIDTH, MALE_BOX_Y + MALE_BOX_HEIGHT, MSYS_PRIORITY_HIGH,
                    CURSOR_WWW, MvtOnMaleRegionCallBack, SelectMaleRegionCallBack);

  // IMP_FEMALE gender region
  MSYS_DefineRegion(&gIMPBeginScreenMouseRegions[3], FEMALE_BOX_X, MALE_BOX_Y,
                    FEMALE_BOX_X + MALE_BOX_WIDTH, MALE_BOX_Y + MALE_BOX_HEIGHT, MSYS_PRIORITY_HIGH,
                    CURSOR_WWW, MvtOnFemaleRegionCallBack, SelectFemaleRegionCallBack);
}

static void DestroyIMPBeginScreenMouseRegions() {
  // Do not remove regions, if only reviewing text.
  if (ubTextEnterMode == 5) return;

  FOR_EACH(MOUSE_REGION, i, gIMPBeginScreenMouseRegions) MSYS_RemoveRegion(&*i);
}

static void SelectFullNameRegionCallBack(MOUSE_REGION *pRegion, int32_t iReason) {
  if (iReason & MSYS_CALLBACK_REASON_LBUTTON_UP) {
    // set current mode to full name type in mode
    ubTextEnterMode = FULL_NAME_MODE;
    fNewCharInString = TRUE;
  }
}

static void SelectNickNameRegionCallBack(MOUSE_REGION *pRegion, int32_t iReason) {
  if (iReason & MSYS_CALLBACK_REASON_LBUTTON_UP) {
    // set mode to nick name type in
    ubTextEnterMode = NICK_NAME_MODE;
    fNewCharInString = TRUE;
  }
}

static void SelectMaleRegionCallBack(MOUSE_REGION *pRegion, int32_t iReason) {
  if (iReason & MSYS_CALLBACK_REASON_LBUTTON_UP) {
    // set mode to nick name type in
    bGenderFlag = IMP_MALE;
    fNewCharInString = TRUE;
  }
}

static void SelectFemaleRegionCallBack(MOUSE_REGION *pRegion, int32_t iReason) {
  if (iReason & MSYS_CALLBACK_REASON_LBUTTON_UP) {
    // set mode to nick name type in
    bGenderFlag = IMP_FEMALE;
    fNewCharInString = TRUE;
  }
}

static void MvtOnFemaleRegionCallBack(MOUSE_REGION *pRegion, int32_t iReason) {
  if (iReason & MSYS_CALLBACK_REASON_LOST_MOUSE) {
    // fNewCharInString = TRUE;
  } else if (iReason & MSYS_CALLBACK_REASON_GAIN_MOUSE) {
    ubTextEnterMode = FEMALE_GENDER_SELECT;
    fNewCharInString = TRUE;
  }
}

static void MvtOnMaleRegionCallBack(MOUSE_REGION *pRegion, int32_t iReason) {
  if (iReason & MSYS_CALLBACK_REASON_LOST_MOUSE) {
    // fNewCharInString = TRUE;
  } else if (iReason & MSYS_CALLBACK_REASON_GAIN_MOUSE) {
    ubTextEnterMode = MALE_GENDER_SELECT;
    fNewCharInString = TRUE;
  }
}

static void RenderGender() {
  // this procedure will render the gender of the character int he appropriate
  // box
  RenderGenderIndent(192, 252);
  RenderGenderIndent(302, 252);

  int32_t x;
  switch (bGenderFlag) {
    case IMP_FEMALE:
      x = FEMALE_BOX_X + 9;
      break;
    case IMP_MALE:
      x = MALE_BOX_X + 9;
      break;
    default:
      return;  // none selected yet
  }
  SetFontBackground(FONT_BLACK);
  SetFontForeground(184);
  MPrint(x, MALE_BOX_Y + 6, L"X");
}

static void Print8CharacterOnlyString() {
  SetFontAttributes(FONT12ARIAL, FONT_BLACK, NO_SHADOW);
  MPrint(430, LAPTOP_SCREEN_WEB_DELTA_Y + 228, pIMPBeginScreenStrings);
  SetFontShadow(DEFAULT_SHADOW);
}

static BOOLEAN CheckCharacterInputForEgg() {
  /*
          if (wcscmp(pFullNameString, L"retraC kraM") == 0 &&
     wcscmp(pNickNameString, L"BdyCnt"))
          {
                  wcscpy(pFullNameString, L"Mark Carter");
                  wcscpy(pNickNameString, L"BodyCount");
                  bGenderFlag             = IMP_MALE;
                  iHealth                 = 99;
                  iAgility                = 99;
                  iStrength               = 99;
                  iDexterity              = 99;
                  iWisdom                 = 35;
                  iLeadership             = 35;
                  iMarksmanship           = 99;
                  iMechanical             =  0;
                  iExplosives             = 99;
                  iMedical                =  0;
                  iSkillA                 = AMBIDEXT;
                  iSkillB                 = AUTO_WEAPS;
                  iPersonality            = NO_PERSONALITYTRAIT;
                  iAttitude               = ATT_ASSHOLE;
                  LaptopSaveInfo.iVoiceId = 1;
                  iPortraitNumber         = 2;
          }
          else if (wcscmp(pFullNameString, L"hcnerF evaD") == 0 &&
     wcscmp(pNickNameString, L"Feral") == 0)
          {
                  wcscpy(pFullNameString, L"Dave French");
                  wcscpy(pNickNameString, L"Feral");
                  bGenderFlag             = IMP_MALE;
                  iHealth                 = 90;
                  iAgility                = 95;
                  iStrength               = 90;
                  iDexterity              = 95;
                  iWisdom                 = 60;
                  iLeadership             = 60;
                  iMarksmanship           = 89;
                  iMechanical             = 40;
                  iExplosives             = 25;
                  iMedical                = 60;
                  iSkillA                 = MARTIALARTS;
                  iSkillB                 = AUTO_WEAPS;
                  iPersonality            = NO_PERSONALITYTRAIT;
                  iAttitude               = ATT_FRIENDLY;
                  LaptopSaveInfo.iVoiceId = 1;
                  iPortraitNumber         = 1;
          }
          else if (wcscmp(pFullNameString, L"Marnes") == 0 &&
     wcscmp(pNickNameString, L"Marnes") == 0)
          {
                  wcscpy(pFullNameString, L"Kris Marnes");
                  wcscpy(pNickNameString, L"Marnes");
                  bGenderFlag             = IMP_MALE;
                  iHealth                 = 100;
                  iAgility                = 100;
                  iStrength               = 100;
                  iDexterity              = 100;
                  iWisdom                 = 100;
                  iLeadership             = 100;
                  iMarksmanship           = 100;
                  iMechanical             = 100;
                  iExplosives             = 100;
                  iMedical                = 100;
                  iSkillA                 = MARTIALARTS;
                  iSkillB                 = AUTO_WEAPS;
                  iPersonality            = NO_PERSONALITYTRAIT;
                  iAttitude               = ATT_FRIENDLY;
                  LaptopSaveInfo.iVoiceId = 2;
                  iPortraitNumber         = 1;
          }
          else if (wcscmp(pFullNameString, L"neslO namroN") == 0 &&
     wcscmp(pNickNameString, L"N.R.G") == 0)
          {
                  wcscpy(pFullNameString, L"Norman Olsen");
                  wcscpy(pNickNameString, L"N.R.G");
                  bGenderFlag             = IMP_MALE;
                  iHealth                 = 99;
                  iAgility                = 99;
                  iStrength               = 99;
                  iDexterity              = 90;
                  iWisdom                 = 70;
                  iLeadership             = 35;
                  iMarksmanship           = 70;
                  iMechanical             = 65;
                  iExplosives             = 99;
                  iMedical                = 75;
                  iSkillA                 = STEALTHY;
                  iSkillB                 = MARTIALARTS;
                  iPersonality            = NO_PERSONALITYTRAIT;
                  iAttitude               = ATT_AGGRESSIVE;
                  LaptopSaveInfo.iVoiceId = 1;
                  iPortraitNumber         = 4;
          }
          else if (wcscmp(pFullNameString, L"snommE werdnA") == 0 &&
     wcscmp(pNickNameString, L"Bubba") == 0)
          {
                  wcscpy(pFullNameString, L"Andrew Emmons");
                  wcscpy(pNickNameString, L"Bubba");
                  bGenderFlag             = IMP_MALE;
                  iHealth                 = 97;
                  iAgility                = 98;
                  iStrength               = 80;
                  iDexterity              = 80;
                  iWisdom                 = 99;
                  iLeadership             = 99;
                  iMarksmanship           = 99;
                  iMechanical             =  0;
                  iExplosives             =  0;
                  iMedical                = 99;
                  iSkillA                 = AUTO_WEAPS;
                  iSkillB                 = AMBIDEXT;
                  iPersonality            = NO_PERSONALITYTRAIT;
                  iAttitude               = ATT_ARROGANT;
                  LaptopSaveInfo.iVoiceId = 1;
                  iPortraitNumber         = 2;
          }
          else if (wcscmp(pFullNameString, L"nalehW yeoJ") == 0 &&
     wcscmp(pNickNameString, L"Joeker") == 0)
          {
                  wcscpy(pFullNameString, L"Joey Whelan");
                  wcscpy(pNickNameString, L"Joeker");
                  bGenderFlag             =  0;
                  iHealth                 = 99;
                  iAgility                = 99;
                  iStrength               = 99;
                  iDexterity              = 99;
                  iWisdom                 = 70;
                  iLeadership             = 80;
                  iMarksmanship           = 99;
                  iMechanical             = 35;
                  iExplosives             = 99;
                  iMedical                = 35;
                  iSkillA                 = AUTO_WEAPS;
                  iSkillB                 = MARTIALARTS;
                  iPersonality            = NO_PERSONALITYTRAIT;
                  iAttitude               = ATT_AGGRESSIVE;
                  LaptopSaveInfo.iVoiceId = 1;
                  iPortraitNumber         = 4;
          }
          else if (wcscmp(pFullNameString, L"gnehC cirE") == 0 &&
     wcscmp(pNickNameString, L"BlakAddr") == 0)
          {
                  wcscpy(pFullNameString, L"Eric Cheng");
                  wcscpy(pNickNameString, L"BlakAddr");
                  bGenderFlag             = IMP_MALE;
                  iHealth                 = 99;
                  iAgility                = 99;
                  iStrength               = 99;
                  iDexterity              = 99;
                  iWisdom                 = 99;
                  iLeadership             = 70;
                  iMarksmanship           = 99;
                  iMechanical             = 50;
                  iExplosives             = 99;
                  iMedical                =  0;
                  iSkillA                 = AUTO_WEAPS;
                  iSkillB                 = MARTIALARTS;
                  iPersonality            = NO_PERSONALITYTRAIT;
                  iAttitude               = ATT_LONER;
                  LaptopSaveInfo.iVoiceId = 1;
                  iPortraitNumber         = 3;
          }
          else if (wcscmp(pFullNameString, L"Karters Killer Kru") == 0 &&
     wcscmp(pNickNameString, L"Bitchin") == 0)
          {
                  wcscpy(pFullNameString, L"Mark Carter");
                  wcscpy(pNickNameString, L"BodyCount");
                  bGenderFlag             = IMP_MALE;
                  iHealth                 = 99;
                  iAgility                = 99;
                  iStrength               = 99;
                  iDexterity              = 99;
                  iWisdom                 = 35;
                  iLeadership             = 35;
                  iMarksmanship           = 99;
                  iMechanical             =  0;
                  iExplosives             = 99;
                  iMedical                =  0;
                  iSkillA                 = AMBIDEXT;
                  iSkillB                 = AUTO_WEAPS;
                  iPersonality            = PSYCHO;
                  iAttitude               = ATT_ASSHOLE;
                  LaptopSaveInfo.iVoiceId = 1;
                  iPortraitNumber         = 2;

                  MERC_HIRE_STRUCT HireMercStruct;
                  HireMercStruct.sSectorX                  =
     SECTORX(g_merc_arrive_sector); HireMercStruct.sSectorY                  =
     SECTORY(g_merc_arrive_sector); HireMercStruct.bSectorZ                  =
     0; HireMercStruct.fUseLandingZoneForArrival = TRUE;
                  HireMercStruct.ubInsertionCode           =
     INSERTION_CODE_ARRIVING_GAME; HireMercStruct.fCopyProfileItemsOver     =
     TRUE; HireMercStruct.iTotalContractLength      = 365;
                  HireMercStruct.uiTimeTillMercArrives     =
     GetMercArrivalTimeOfDay();

                  HireMercStruct.ubProfileID = REAPER;
                  HireMerc(HireMercStruct);

                  HireMercStruct.ubProfileID = SHADOW;
                  HireMerc(HireMercStruct);

                  HireMercStruct.ubProfileID = RAVEN;
                  HireMerc(HireMercStruct);

                  HireMercStruct.ubProfileID = LYNX;
                  HireMerc(HireMercStruct);

                  HireMercStruct.ubProfileID = CLIFF;
                  HireMerc(HireMercStruct);
          }
          else if (wcscmp(pFullNameString, L"dleifmaC sirhC") == 0 &&
     wcscmp(pNickNameString, L"SSR") == 0)
          {
                  wcscpy(pFullNameString, L"James Bolivar DiGriz");
                  wcscpy(pNickNameString, L"DiGriz");
                  bGenderFlag             = IMP_MALE;
                  iHealth                 = 99;
                  iAgility                = 80;
                  iStrength               = 80;
                  iDexterity              = 99;
                  iWisdom                 = 70;
                  iLeadership             = 70;
                  iMarksmanship           = 99;
                  iMechanical             = 99;
                  iExplosives             = 99;
                  iMedical                = 60;
                  iSkillA                 = ELECTRONICS;
                  iSkillB                 = LOCKPICKING;
                  iPersonality            = NO_PERSONALITYTRAIT;
                  iAttitude               = ATT_LONER;
                  LaptopSaveInfo.iVoiceId = 1;
                  iPortraitNumber         = 3;
          }
          else if (wcscmp(pFullNameString, L"Test Female") == 0 &&
     wcscmp(pNickNameString, L"Test") == 0)
          {
                  wcscpy(pFullNameString, L"Test Female");
                  wcscpy(pNickNameString, L"Test");
                  bGenderFlag             = IMP_FEMALE;
                  iHealth                 = 55;
                  iAgility                = 55;
                  iStrength               = 55;
                  iDexterity              = 55;
                  iWisdom                 = 55;
                  iLeadership             = 55;
                  iMarksmanship           = 55;
                  iMechanical             = 55;
                  iExplosives             = 55;
                  iMedical                = 55;
                  iSkillA                 = NO_SKILLTRAIT;
                  iSkillB                 = NO_SKILLTRAIT;
                  iPersonality            = NO_PERSONALITYTRAIT;
                  iAttitude               = ATT_LONER;
                  LaptopSaveInfo.iVoiceId = 1;
                  iPortraitNumber         = 5;
          }
          else
          {
                  return FALSE;
          }

          iCurrentImpPage = IMP_FINISH;
          return TRUE;
  */
  return FALSE;
}
