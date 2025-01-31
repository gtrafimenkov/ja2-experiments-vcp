// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#include "Laptop/IMPPersonalityQuiz.h"

#include <stdio.h>
#include <string.h>
#include <wchar.h>

#include "Directories.h"
#include "Laptop/CharProfile.h"
#include "Laptop/IMPCompileCharacter.h"
#include "Laptop/IMPTextSystem.h"
#include "Laptop/IMPVideoObjects.h"
#include "Laptop/Laptop.h"
#include "Macro.h"
#include "SGP/ButtonSystem.h"
#include "SGP/Debug.h"
#include "SGP/Font.h"
#include "SGP/Input.h"
#include "Tactical/SoldierProfileType.h"
#include "TileEngine/RenderDirty.h"
#include "Utils/Cursors.h"
#include "Utils/FontControl.h"
#include "Utils/Text.h"

static BUTTON_PICS *giIMPPersonalityQuizButtonImage[2];
static GUIButtonRef giIMPPersonalityQuizButton[2];

// these are the buttons for the current question
static BUTTON_PICS *giIMPPersonalityQuizAnswerButtonImage[8];
static GUIButtonRef giIMPPersonalityQuizAnswerButton[8];

static GUIButtonRef giPreviousQuestionButton;
static GUIButtonRef giNextQuestionButton;

static BUTTON_PICS *giPreviousQuestionButtonImage;
static BUTTON_PICS *giNextQuestionButtonImage;

// this the currently highlighted answer
int32_t iCurrentAnswer = -1;

// the current quiz question
int32_t giCurrentPersonalityQuizQuestion = 0;
static int32_t giPreviousPersonalityQuizQuestion = -1;
int32_t giMaxPersonalityQuizQuestion = 0;

// start over flag
BOOLEAN fStartOverFlag = FALSE;

#define BTN_FIRST_COLUMN_X 15
#define BTN_SECOND_COLUMN_X 256

#define INDENT_OFFSET 55

// number of IMP questions
#define MAX_NUMBER_OF_IMP_QUESTIONS 16

// answer list
int32_t iQuizAnswerList[MAX_NUMBER_OF_IMP_QUESTIONS];
// current number of buttons being shown
int32_t iNumberOfPersonaButtons = 0;

static void CreateIMPPersonalityQuizAnswerButtons();
static void CreateIMPPersonalityQuizButtons();
static void PrintQuizQuestionNumber();
static void ResetQuizAnswerButtons();

void EnterIMPPersonalityQuiz() {
  // void answers out the quiz
  memset(&iQuizAnswerList, -1, sizeof(int32_t) * MAX_NUMBER_OF_IMP_QUESTIONS);

  // if we are entering for first time, reset
  if (giCurrentPersonalityQuizQuestion == MAX_NUMBER_OF_IMP_QUESTIONS) {
    giCurrentPersonalityQuizQuestion = 0;
  }
  // reset previous
  giPreviousPersonalityQuizQuestion = -1;

  // reset skills, attributes and personality
  ResetSkillsAttributesAndPersonality();

  // create/destroy buttons for  questions, if needed
  CreateIMPPersonalityQuizAnswerButtons();

  // now reset them
  ResetQuizAnswerButtons();

  // create other buttons
  CreateIMPPersonalityQuizButtons();
}

void RenderIMPPersonalityQuiz() {
  // the background
  RenderProfileBackGround();

  // highlight answer
  PrintImpText();

  // indent for current and last page numbers
  // RenderAttrib2IndentFrame(BTN_FIRST_COLUMN_X + 2, 365 );

  // the current and last question numbers
  PrintQuizQuestionNumber();
}

static void DestroyIMPersonalityQuizButtons();
static void DestroyPersonalityQuizButtons();

void ExitIMPPersonalityQuiz() {
  // set previous to current, we want it's buttons gone!
  giPreviousPersonalityQuizQuestion = giCurrentPersonalityQuizQuestion;

  // destroy regular quiz buttons: the done and start over buttons
  DestroyIMPersonalityQuizButtons();

  // destroy the buttons used for answers
  DestroyPersonalityQuizButtons();

  if (fStartOverFlag) {
    fStartOverFlag = FALSE;
    giCurrentPersonalityQuizQuestion = 0;
  }
}

static void HandleIMPQuizKeyBoard();

void HandleIMPPersonalityQuiz() {
  // create/destroy buttons for  questions, if needed
  CreateIMPPersonalityQuizAnswerButtons();

  // handle keyboard input
  HandleIMPQuizKeyBoard();

  if (iCurrentAnswer == -1) {
    DisableButton(giIMPPersonalityQuizButton[0]);
  }
}

static GUIButtonRef MakeButton(BUTTON_PICS *const img, const wchar_t *const text, const int16_t x,
                               const int16_t y, const GUI_CALLBACK click) {
  const int16_t text_col = FONT_WHITE;
  const int16_t shadow_col = DEFAULT_SHADOW;
  GUIButtonRef const btn =
      CreateIconAndTextButton(img, text, FONT12ARIAL, text_col, shadow_col, text_col, shadow_col, x,
                              y, MSYS_PRIORITY_HIGH, click);
  btn->SetCursor(CURSOR_WWW);
  return btn;
}

static void BtnIMPPersonalityQuizAnswerConfirmCallback(GUI_BUTTON *btn, int32_t reason);
static void BtnIMPPersonalityQuizStartOverCallback(GUI_BUTTON *btn, int32_t reason);
static void PreviousQuestionButtonCallback(GUI_BUTTON *btn, int32_t iReason);
static void NextQuestionButtonCallback(GUI_BUTTON *btn, int32_t iReason);

static void CreateIMPPersonalityQuizButtons() {
  // this function will create the buttons needed for the IMP personality quiz
  // Page
  const int16_t dx = LAPTOP_SCREEN_UL_X;
  const int16_t dy = LAPTOP_SCREEN_WEB_UL_Y;

  // ths Done button
  giIMPPersonalityQuizButtonImage[0] = LoadButtonImage(LAPTOPDIR "/button_7.sti", 0, 1);
  giIMPPersonalityQuizButton[0] =
      MakeButton(giIMPPersonalityQuizButtonImage[0], pImpButtonText[8], dx + 197, dy + 302,
                 BtnIMPPersonalityQuizAnswerConfirmCallback);

  // start over
  giIMPPersonalityQuizButtonImage[1] = LoadButtonImage(LAPTOPDIR "/button_5.sti", 0, 1);
  giIMPPersonalityQuizButton[1] =
      MakeButton(giIMPPersonalityQuizButtonImage[1], pImpButtonText[7], dx + BTN_FIRST_COLUMN_X,
                 dy + 302, BtnIMPPersonalityQuizStartOverCallback);

  giPreviousQuestionButtonImage = LoadButtonImage(LAPTOPDIR "/button_3.sti", 0, 1);
  giPreviousQuestionButton = MakeButton(giPreviousQuestionButtonImage, pImpButtonText[12], dx + 197,
                                        dy + 361, PreviousQuestionButtonCallback);

  giNextQuestionButtonImage = LoadButtonImage(LAPTOPDIR "/button_3.sti", 0, 1);
  giNextQuestionButton = MakeButton(giNextQuestionButtonImage, pImpButtonText[13], dx + 417,
                                    dy + 361, NextQuestionButtonCallback);

  giNextQuestionButton->SpecifyTextSubOffsets(0, -1, FALSE);
  giPreviousQuestionButton->SpecifyTextSubOffsets(0, -1, FALSE);

  DisableButton(giPreviousQuestionButton);
  DisableButton(giNextQuestionButton);
}

static void DestroyIMPersonalityQuizButtons() {
  // this function will destroy the buttons needed for the IMP personality quiz
  // page

  // the done button
  RemoveButton(giIMPPersonalityQuizButton[0]);
  UnloadButtonImage(giIMPPersonalityQuizButtonImage[0]);

  // the start over button
  RemoveButton(giIMPPersonalityQuizButton[1]);
  UnloadButtonImage(giIMPPersonalityQuizButtonImage[1]);

  // previosu button
  RemoveButton(giPreviousQuestionButton);
  UnloadButtonImage(giPreviousQuestionButtonImage);

  // next button
  RemoveButton(giNextQuestionButton);
  UnloadButtonImage(giNextQuestionButtonImage);
}

static void AddIMPPersonalityQuizAnswerButtons(int32_t iNumberOfButtons);
static void ToggleQuestionNumberButtonOn(int32_t iAnswerNumber);

static void CreateIMPPersonalityQuizAnswerButtons() {
  // this function will create the buttons for the personality quiz answer
  // selections

  if (IMP_PERSONALITY_QUIZ != iCurrentImpPage) {
    // not valid pagre, get out
    return;
  }

  if (giCurrentPersonalityQuizQuestion == giPreviousPersonalityQuizQuestion) {
    // mode has not changed, return;
    return;
  }

  // destroy old screens buttons
  DestroyPersonalityQuizButtons();

  // re-render screen
  RenderProfileBackGround();

  switch (giCurrentPersonalityQuizQuestion) {
    case -1:
      break;  // do nothing
    case 0:
      iNumberOfPersonaButtons = 6;
      break;
    case 3:
      iNumberOfPersonaButtons = 5;
      break;
    case 5:
      iNumberOfPersonaButtons = 5;
      break;
    case 10:
      iNumberOfPersonaButtons = 5;
      break;
    case 11:
      iNumberOfPersonaButtons = 8;
      break;
    default:
      iNumberOfPersonaButtons = 4;
      break;
  }

  AddIMPPersonalityQuizAnswerButtons(iNumberOfPersonaButtons);

  ToggleQuestionNumberButtonOn(iQuizAnswerList[giCurrentPersonalityQuizQuestion]);

  // re render text
  PrintImpText();

  // the current and last question numbers
  PrintQuizQuestionNumber();

  // title bar
  RenderWWWProgramTitleBar();
}

static void DestroyIMPPersonalityQuizAnswerButtons(int32_t iNumberOfButtons);

static void DestroyPersonalityQuizButtons() {
  // this function will destroy the buttons used in the previous personality
  // question
  // destroy old buttons
  uint32_t ButtonCount;
  switch (giPreviousPersonalityQuizQuestion) {
    case -1:
      return;  // do nothing
    case 0:
      ButtonCount = 6;
      break;
    case 3:
      ButtonCount = 5;
      break;
    case 5:
      ButtonCount = 5;
      break;
    case 10:
      ButtonCount = 5;
      break;
    case 11:
      ButtonCount = 8;
      break;
    default:
      ButtonCount = 4;
      break;
  }
  DestroyIMPPersonalityQuizAnswerButtons(ButtonCount);
}

static void BtnQuizAnswerCallback(GUI_BUTTON *, int32_t reason);

static void AddIMPPersonalityQuizAnswerButtons(int32_t iNumberOfButtons) {
  // will add iNumberofbuttons to the answer button list
  for (uint32_t i = 0; i < iNumberOfButtons; i++) {
    int32_t XLoc = LAPTOP_SCREEN_UL_X + (i < 4 ? BTN_FIRST_COLUMN_X : BTN_SECOND_COLUMN_X);
    int32_t YLoc = LAPTOP_SCREEN_WEB_UL_Y + 97 + i % 4 * 50;
    BUTTON_PICS *const Image = LoadButtonImage(LAPTOPDIR "/button_6.sti", 0, 1);
    giIMPPersonalityQuizAnswerButtonImage[i] = Image;
    GUIButtonRef const Button = QuickCreateButtonNoMove(
        Image, XLoc, YLoc, MSYS_PRIORITY_HIGHEST - 3, BtnQuizAnswerCallback);
    giIMPPersonalityQuizAnswerButton[i] = Button;
    Button->SetUserData(i);
    Button->SpecifyTextOffsets(23, 12, TRUE);
    wchar_t sString[32];
    swprintf(sString, lengthof(sString), L"%d", i + 1);
    Button->SpecifyGeneralTextAttributes(sString, FONT12ARIAL, FONT_WHITE, FONT_BLACK);
    Button->SetCursor(CURSOR_WWW);
  }

  // previous is current
  giPreviousPersonalityQuizQuestion = giCurrentPersonalityQuizQuestion;
}

static void DestroyIMPPersonalityQuizAnswerButtons(int32_t iNumberOfButtons) {
  int32_t iCounter = 0;
  for (iCounter = 0; iCounter < iNumberOfButtons; iCounter++) {
    RemoveButton(giIMPPersonalityQuizAnswerButton[iCounter]);
    UnloadButtonImage(giIMPPersonalityQuizAnswerButtonImage[iCounter]);
  }
}

static void CheckStateOfTheConfirmButton();

static void BtnQuizAnswerCallback(GUI_BUTTON *const btn, int32_t const reason) {
  if (reason & MSYS_CALLBACK_REASON_LBUTTON_DWN) {
    ResetQuizAnswerButtons();
    btn->uiFlags |= BUTTON_CLICKED_ON;
    CheckStateOfTheConfirmButton();
    iCurrentAnswer = btn->GetUserData();
    PrintImpText();
    PrintQuizQuestionNumber();
    fReDrawCharProfile = TRUE;
  }
}

static void CheckAndUpdateNextPreviousIMPQuestionButtonStates();
static void CompileQuestionsInStatsAndWhatNot();

static void BtnIMPPersonalityQuizAnswerConfirmCallback(GUI_BUTTON *btn, int32_t reason) {
  if (reason & MSYS_CALLBACK_REASON_LBUTTON_UP) {
    if (iCurrentAnswer != -1) {
      ResetQuizAnswerButtons();
      iQuizAnswerList[giCurrentPersonalityQuizQuestion] = iCurrentAnswer;
      iCurrentAnswer = -1;

      // next question, JOHNNY!
      if (giCurrentPersonalityQuizQuestion == giMaxPersonalityQuizQuestion) {
        giMaxPersonalityQuizQuestion++;
      }

      giCurrentPersonalityQuizQuestion++;
      CheckAndUpdateNextPreviousIMPQuestionButtonStates();

      // OPPS!, done..time to finish up
      if (giCurrentPersonalityQuizQuestion > 15) {
        iCurrentImpPage = IMP_PERSONALITY_FINISH;
        CompileQuestionsInStatsAndWhatNot();
      }
    }
  }
}

static void BtnIMPPersonalityQuizStartOverCallback(GUI_BUTTON *btn, int32_t reason) {
  if (reason & MSYS_CALLBACK_REASON_LBUTTON_UP) {
    giPreviousPersonalityQuizQuestion = giCurrentPersonalityQuizQuestion;
    giMaxPersonalityQuizQuestion = 0;
    fStartOverFlag = TRUE;

    iCurrentImpPage = IMP_PERSONALITY;
    fButtonPendingFlag = TRUE;
    iCurrentAnswer = -1;
  }
}

static void ResetQuizAnswerButtons() {
  // how many buttons to reset?
  int32_t iCounter;
  switch (giCurrentPersonalityQuizQuestion) {
    case -1:
      return;  // do nothing
    case 0:
      iCounter = 6;
      break;
    case 3:
      iCounter = 5;
      break;
    case 5:
      iCounter = 5;
      break;
    case 10:
      iCounter = 5;
      break;
    case 11:
      iCounter = 8;
      break;
    default:
      iCounter = 4;
      break;
  }

  // now run through and reset the buttons
  for (int32_t i = 0; i < iCounter; i++) {
    giIMPPersonalityQuizAnswerButton[i]->uiFlags &= ~BUTTON_CLICKED_ON;
  }
}

static void CompileQuestionsInStatsAndWhatNot() {
  switch (iQuizAnswerList[0]) {
    case 0:
      AddSkillToSkillList(fCharacterIsMale ? MARTIALARTS : AMBIDEXT);
      break;
    case 1:
      AddAnAttitudeToAttitudeList(ATT_LONER);
      break;
    case 2:
      AddSkillToSkillList(HANDTOHAND);
      break;
    case 3:
      AddSkillToSkillList(LOCKPICKING);
      break;
    case 4:
      AddSkillToSkillList(THROWING);
      break;
    case 5:
      AddAnAttitudeToAttitudeList(ATT_OPTIMIST);
      break;
  }

  switch (iQuizAnswerList[1]) {
    case 0:
      AddSkillToSkillList(TEACHING);
      break;
    case 1:
      AddSkillToSkillList(STEALTHY);
      break;
    case 2:
      AddAPersonalityToPersonalityList(PSYCHO);
      break;
    case 3:
      AddAnAttitudeToAttitudeList(ATT_FRIENDLY);
      break;
  }

  switch (iQuizAnswerList[2]) {
    case 0:
      AddSkillToSkillList(LOCKPICKING);
      break;
    case 1:
      AddAnAttitudeToAttitudeList(ATT_ARROGANT);
      break;
    case 2:
      AddSkillToSkillList(STEALTHY);
      break;
    case 3:
      AddAnAttitudeToAttitudeList(ATT_NORMAL);
      break;
  }

  switch (iQuizAnswerList[3]) {
    case 0:
      AddSkillToSkillList(AUTO_WEAPS);
      break;
    case 1:
      AddAnAttitudeToAttitudeList(ATT_FRIENDLY);
      break;
    case 2:
      AddAnAttitudeToAttitudeList(ATT_NORMAL);
      break;
    case 3:
      AddAnAttitudeToAttitudeList(ATT_ASSHOLE);
      break;
    case 4:
      AddAnAttitudeToAttitudeList(ATT_LONER);
      break;
  }

  switch (iQuizAnswerList[4]) {
    // XXX TODO0006 the effects seems odd. answer 3 is even more aggressive than
    // answer 2
    case 0:
      AddAnAttitudeToAttitudeList(ATT_COWARD);
      break;
    case 1:
      break;  // none
    case 2:
      AddAnAttitudeToAttitudeList(ATT_AGGRESSIVE);
      break;
    case 3:
      break;  // none
  }

  switch (iQuizAnswerList[5]) {
    case 0:
      AddAnAttitudeToAttitudeList(ATT_COWARD);
      break;
    case 1:
      AddSkillToSkillList(NIGHTOPS);
      break;
    case 2:
      AddAPersonalityToPersonalityList(CLAUSTROPHOBIC);
      break;
    case 3:
      break;  // none
    case 4:
      break;  // none
  }

  switch (iQuizAnswerList[6]) {
    case 0:
      AddSkillToSkillList(ELECTRONICS);
      break;
    case 1:
      AddSkillToSkillList(KNIFING);
      break;
    case 2:
      AddSkillToSkillList(NIGHTOPS);
      break;
    case 3:
      break;  // none
  }

  switch (iQuizAnswerList[7]) {
    case 0:
      AddSkillToSkillList(AMBIDEXT);
      break;
    case 1:
      break;  // none
    case 2:
      AddAnAttitudeToAttitudeList(ATT_OPTIMIST);
      break;
    case 3:
      AddAPersonalityToPersonalityList(PSYCHO);
      break;
  }

  switch (iQuizAnswerList[8]) {
    case 0:
      AddAPersonalityToPersonalityList(FORGETFUL);
      break;
    case 1:  // none // XXX TODO0006 fallthrough? code and comment disagree
    case 2:
      AddAnAttitudeToAttitudeList(ATT_PESSIMIST);
      break;
    case 3:
      AddAPersonalityToPersonalityList(NERVOUS);
      break;
  }

  switch (iQuizAnswerList[9]) {
    case 0:
      break;  // none
    case 1:
      AddAnAttitudeToAttitudeList(ATT_PESSIMIST);
      break;
    case 2:
      AddAnAttitudeToAttitudeList(ATT_ASSHOLE);
      break;
    case 3:
      AddAPersonalityToPersonalityList(NERVOUS);
      break;
  }

  switch (iQuizAnswerList[10]) {
    case 0:
      break;  // none
    case 1:
      AddSkillToSkillList(TEACHING);
      break;
    case 2:
      AddAnAttitudeToAttitudeList(ATT_AGGRESSIVE);
      break;
    case 3:
      AddAnAttitudeToAttitudeList(ATT_NORMAL);
      break;
    case 4:
      break;  // none
  }

  switch (iQuizAnswerList[11]) {
    case 0:
      AddSkillToSkillList(fCharacterIsMale ? MARTIALARTS : AMBIDEXT);
      break;
    case 1:
      AddSkillToSkillList(KNIFING);
      break;
    case 2:
      break;  // none
    case 3:
      AddSkillToSkillList(AUTO_WEAPS);
      break;
    case 4:
      AddSkillToSkillList(HANDTOHAND);
      break;
    case 5:
      AddSkillToSkillList(ELECTRONICS);
      break;
    case 6:
      break;  // asshole // XXX TODO0006 code and commend disagree
    case 7:
      break;  // none
  }

  switch (iQuizAnswerList[12]) {
    case 0:
      AddAPersonalityToPersonalityList(FORGETFUL);
      break;
    case 1:
      AddAnAttitudeToAttitudeList(ATT_NORMAL);
      break;
    case 2:
      AddAnAttitudeToAttitudeList(ATT_NORMAL);
      break;
    case 3:
      AddAPersonalityToPersonalityList(HEAT_INTOLERANT);
      break;
  }

  switch (iQuizAnswerList[13]) {
    case 0:
      AddAPersonalityToPersonalityList(CLAUSTROPHOBIC);
      break;
    case 1:
      AddAnAttitudeToAttitudeList(ATT_NORMAL);
      break;
    case 2:
      AddAPersonalityToPersonalityList(HEAT_INTOLERANT);
      break;
    case 3:
      break;  // none
  }

  switch (iQuizAnswerList[14]) {
    case 0:
      AddSkillToSkillList(THROWING);
      break;
    case 1:
      AddSkillToSkillList(AMBIDEXT);
      break;
    // XXX TODO0006 cases 2 and 3 probably interchanged
    case 3:
      break;  // none
    case 2:
      AddAnAttitudeToAttitudeList(ATT_ARROGANT);
      break;
  }

  switch (iQuizAnswerList[15]) {
    // XXX TODO0006 this question has no effect
    case 0:
      break;  // none !
    case 1:
      break;  // none !
    case 2:
      break;  // none !
    case 3:
      break;  // none !
  }
}

void BltAnswerIndents(int32_t iNumberOfIndents) {
  int32_t iCounter = 0;

  // the question indent
  RenderQtnIndentFrame(15, 20);

  // the answers

  for (iCounter = 0; iCounter < iNumberOfIndents; iCounter++) {
    switch (iCounter) {
      case (0):
        if (iNumberOfIndents < 5) {
          RenderQtnLongIndentFrame(BTN_FIRST_COLUMN_X + INDENT_OFFSET, 93);

          if (iCurrentAnswer == iCounter) {
            RenderQtnLongIndentHighFrame(BTN_FIRST_COLUMN_X + INDENT_OFFSET, 93);
          }
        } else {
          RenderQtnShortIndentFrame(BTN_FIRST_COLUMN_X + INDENT_OFFSET, 93);

          if (iCurrentAnswer == iCounter) {
            RenderQtnShortIndentHighFrame(BTN_FIRST_COLUMN_X + INDENT_OFFSET, 93);
          }
        }
        break;
      case (1):
        if (iNumberOfIndents < 5) {
          RenderQtnLongIndentFrame(BTN_FIRST_COLUMN_X + INDENT_OFFSET, 143);

          if (iCurrentAnswer == iCounter) {
            RenderQtnLongIndentHighFrame(BTN_FIRST_COLUMN_X + INDENT_OFFSET, 143);
          }
        } else {
          RenderQtnShortIndentFrame(BTN_FIRST_COLUMN_X + INDENT_OFFSET, 143);

          if (iCurrentAnswer == iCounter) {
            RenderQtnShortIndentHighFrame(BTN_FIRST_COLUMN_X + INDENT_OFFSET, 143);
          }
        }
        break;
      case (2):
        if (iNumberOfIndents < 5) {
          RenderQtnLongIndentFrame(BTN_FIRST_COLUMN_X + INDENT_OFFSET, 193);

          if (iCurrentAnswer == iCounter) {
            RenderQtnLongIndentHighFrame(BTN_FIRST_COLUMN_X + INDENT_OFFSET, 193);
          }
        } else {
          RenderQtnShortIndentFrame(BTN_FIRST_COLUMN_X + INDENT_OFFSET, 193);

          if (iCurrentAnswer == iCounter) {
            RenderQtnShortIndentHighFrame(BTN_FIRST_COLUMN_X + INDENT_OFFSET, 193);
          }
        }
        break;
      case (3):

        // is this question # 6 ..if so, need longer answer box
        if (giCurrentPersonalityQuizQuestion == 5) {
          // render longer frame
          RenderQtnShort2IndentFrame(BTN_FIRST_COLUMN_X + INDENT_OFFSET, 243);

          // is this answer currently selected?
          if (iCurrentAnswer == iCounter) {
            // need to highlight
            RenderQtnShort2IndentHighFrame(BTN_FIRST_COLUMN_X + INDENT_OFFSET, 243);
          }
          // done
          break;
        }

        if (iNumberOfIndents < 5) {
          RenderQtnLongIndentFrame(BTN_FIRST_COLUMN_X + INDENT_OFFSET, 243);

          if (iCurrentAnswer == iCounter) {
            RenderQtnLongIndentHighFrame(BTN_FIRST_COLUMN_X + INDENT_OFFSET, 243);
          }
        } else {
          RenderQtnShortIndentFrame(BTN_FIRST_COLUMN_X + INDENT_OFFSET, 243);

          if (iCurrentAnswer == iCounter) {
            RenderQtnShortIndentHighFrame(BTN_FIRST_COLUMN_X + INDENT_OFFSET, 243);
          }
        }
        break;
      case (4):

        // is this question # 14 or 21?..if so, need longer answer box
        if ((giCurrentPersonalityQuizQuestion == 10) || (giCurrentPersonalityQuizQuestion == 5)) {
          // render longer frame
          RenderQtnShort2IndentFrame(BTN_SECOND_COLUMN_X + INDENT_OFFSET, 93);

          // is this answer currently selected?
          if (iCurrentAnswer == iCounter) {
            // need to highlight
            RenderQtnShort2IndentHighFrame(BTN_SECOND_COLUMN_X + INDENT_OFFSET, 93);
          }
          // done
          break;
        }

        RenderQtnShortIndentFrame(BTN_SECOND_COLUMN_X + INDENT_OFFSET, 93);

        if (iCurrentAnswer == iCounter) {
          RenderQtnShortIndentHighFrame(BTN_SECOND_COLUMN_X + INDENT_OFFSET, 93);
        }
        break;
      case (5):

        // special case?..longer frame needed if so
        if (giCurrentPersonalityQuizQuestion == 19) {
          // render longer frame
          RenderQtnShort2IndentFrame(BTN_SECOND_COLUMN_X + INDENT_OFFSET, 143);

          // is this answer currently selected?
          if (iCurrentAnswer == iCounter) {
            // need to highlight
            RenderQtnShort2IndentHighFrame(BTN_SECOND_COLUMN_X + INDENT_OFFSET, 143);
          }
          // done
          break;
        }
        RenderQtnShortIndentFrame(BTN_SECOND_COLUMN_X + INDENT_OFFSET, 143);
        if (iCurrentAnswer == iCounter) {
          RenderQtnShortIndentHighFrame(BTN_SECOND_COLUMN_X + INDENT_OFFSET, 143);
        }
        break;
      case (6):
        RenderQtnShortIndentFrame(BTN_SECOND_COLUMN_X + INDENT_OFFSET, 193);
        if (iCurrentAnswer == iCounter) {
          RenderQtnShortIndentHighFrame(BTN_SECOND_COLUMN_X + INDENT_OFFSET, 193);
        }
        break;
      case (7):
        RenderQtnShortIndentFrame(BTN_SECOND_COLUMN_X + INDENT_OFFSET, 243);
        if (iCurrentAnswer == iCounter) {
          RenderQtnShortIndentHighFrame(BTN_SECOND_COLUMN_X + INDENT_OFFSET, 243);
        }
        break;
      case (8):
        break;
    }
  }
}

static void PrintQuizQuestionNumber() {
  // this function will print the number of the current question and the numebr
  // of questions

  SetFontAttributes(FONT12ARIAL, FONT_WHITE);

  // print current question number
  mprintf(LAPTOP_SCREEN_UL_X + 345, LAPTOP_SCREEN_WEB_UL_Y + 370, L"%d",
          giCurrentPersonalityQuizQuestion + 1);

  // total number of questions
  MPrint(LAPTOP_SCREEN_UL_X + 383, LAPTOP_SCREEN_WEB_UL_Y + 370, L"16");
}

static void CheckStateOfTheConfirmButton() {
  // will check the state of the confirm button, should it be enabled or
  // disabled?
  if (iCurrentAnswer == -1) {
    // was disabled, enable
    EnableButton(giIMPPersonalityQuizButton[0]);
  }
}

static void MoveAheadAQuestion();
static void MoveBackAQuestion();

static void HandleIMPQuizKeyBoard() {
  InputAtom InputEvent;
  BOOLEAN fSkipFrame = FALSE;

  SGPPoint MousePos;
  GetMousePos(&MousePos);

  while (DequeueEvent(&InputEvent)) {
    if (!fSkipFrame) {
      // HOOK INTO MOUSE HOOKS

      /*
      if( (InputEvent.usEvent == KEY_DOWN ) && ( InputEvent.usParam >= '1' ) &&
      ( InputEvent.usParam <= '9') )
      {
              if( ( uint16_t )( iNumberOfPersonaButtons ) >= InputEvent.usParam -
      '0' )
              {
                      // reset buttons
                      ResetQuizAnswerButtons( );

                      // ok, check to see if button was disabled, if so, re
      enable CheckStateOfTheConfirmButton( );

                      // toggle this button on
                      giIMPPersonalityQuizAnswerButton[InputEvent.usParam -
      '1']->uiFlags |= BUTTON_CLICKED_ON;

                      iCurrentAnswer = InputEvent.usParam - '1';

                      PrintImpText( );

                      // the current and last question numbers
                      PrintQuizQuestionNumber( );

                      fReDrawCharProfile = TRUE;
                      fSkipFrame = TRUE;
              }
      }
      else if (iCurrentAnswer != -1 && InputEvent.usEvent == KEY_DOWN &&
      InputEvent.usParam == SDLK_RETURN)
      {
              // reset all the buttons
              ResetQuizAnswerButtons( );

              // copy the answer into the list
              iQuizAnswerList[ giCurrentPersonalityQuizQuestion ] =
      iCurrentAnswer;

              // reset answer for next question
              iCurrentAnswer = -1;

              // next question, JOHNNY!
              giCurrentPersonalityQuizQuestion++;
              giMaxPersonalityQuizQuestion++;


              // OPPS!, done..time to finish up
              if( giCurrentPersonalityQuizQuestion > 15)
              {
                      iCurrentImpPage = IMP_PERSONALITY_FINISH;
                      // process
                      CompileQuestionsInStatsAndWhatNot( );
              }

              fSkipFrame = TRUE;
      }
      else if( ( InputEvent.usEvent == KEY_DOWN ) && ( InputEvent.usParam == '='
      ) )
      {
              MoveAheadAQuestion( );
              fSkipFrame = TRUE;
      }
      else if( ( InputEvent.usEvent == KEY_DOWN ) && ( InputEvent.usParam == '-'
      ) )
      {
              MoveBackAQuestion( );
              fSkipFrame = TRUE;
      }
      else
*/
      {
        MouseSystemHook(InputEvent.usEvent, MousePos.iX, MousePos.iY);
        HandleKeyBoardShortCutsForLapTop(InputEvent.usEvent, InputEvent.usParam,
                                         InputEvent.usKeyState);
      }
    }
  }
}

static void CheckAndUpdateNextPreviousIMPQuestionButtonStates() {
  EnableButton(giNextQuestionButton,
               giCurrentPersonalityQuizQuestion < giMaxPersonalityQuizQuestion);
  EnableButton(giPreviousQuestionButton, giCurrentPersonalityQuizQuestion != 0);
}

static void MoveAheadAQuestion() {
  // move ahead a question in the personality question list
  if (giCurrentPersonalityQuizQuestion < giMaxPersonalityQuizQuestion) {
    giCurrentPersonalityQuizQuestion++;

    iCurrentAnswer = -1;
    CheckStateOfTheConfirmButton();

    iCurrentAnswer = iQuizAnswerList[giCurrentPersonalityQuizQuestion];
  }

  CheckAndUpdateNextPreviousIMPQuestionButtonStates();
}

static void MoveBackAQuestion() {
  if (giCurrentPersonalityQuizQuestion > 0) {
    giCurrentPersonalityQuizQuestion--;

    iCurrentAnswer = -1;
    CheckStateOfTheConfirmButton();

    iCurrentAnswer = iQuizAnswerList[giCurrentPersonalityQuizQuestion];
  }

  EnableButton(giNextQuestionButton);

  CheckAndUpdateNextPreviousIMPQuestionButtonStates();
}

static void ToggleQuestionNumberButtonOn(int32_t iAnswerNumber) {
  if ((giCurrentPersonalityQuizQuestion <= giMaxPersonalityQuizQuestion) && (iAnswerNumber != -1)) {
    // reset buttons
    ResetQuizAnswerButtons();

    // toggle this button on
    giIMPPersonalityQuizAnswerButton[iAnswerNumber]->uiFlags |= BUTTON_CLICKED_ON;
    iCurrentAnswer = iAnswerNumber;
  }
}

static void PreviousQuestionButtonCallback(GUI_BUTTON *btn, int32_t iReason) {
  if (iReason & MSYS_CALLBACK_REASON_LBUTTON_UP) {
    MoveBackAQuestion();
  }
}

static void NextQuestionButtonCallback(GUI_BUTTON *btn, int32_t iReason) {
  if (iReason & MSYS_CALLBACK_REASON_LBUTTON_UP) {
    MoveAheadAQuestion();
  }
}
