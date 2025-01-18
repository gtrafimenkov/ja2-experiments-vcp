// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#ifndef _HANDLE_UI_
#define _HANDLE_UI_

#include "JA2Types.h"
#include "SGP/Input.h"
#include "SGP/MouseSystem.h"
#include "ScreenIDs.h"
#include "Tactical/InterfaceCursors.h"
#include "Tactical/SoldierFind.h"
#include "rust_geometry.h"

#define UIEVENT_SINGLEEVENT 0x00000002
#define UIEVENT_SNAPMOUSE 0x00000008

#define NO_GUY_SELECTION 0
#define SELECTED_GUY_SELECTION 1
#define NONSELECTED_GUY_SELECTION 2
#define ENEMY_GUY_SELECTION 3

enum MouseMoveState { MOUSE_STATIONARY, MOUSE_MOVING_IN_TILE, MOUSE_MOVING_NEW_TILE };

enum MoveUITarget {
  MOVEUI_TARGET_NONE = 0,
  MOVEUI_TARGET_INTTILES = 1,
  MOVEUI_TARGET_ITEMS = 2,
  MOVEUI_TARGET_MERCS = 3,
  MOVEUI_TARGET_MERCSFORAID = 5,
  MOVEUI_TARGET_WIREFENCE = 6,
  MOVEUI_TARGET_BOMB = 7,
  MOVEUI_TARGET_STEAL = 8,
  MOVEUI_TARGET_REPAIR = 9,
  MOVEUI_TARGET_JAR = 10,
  MOVEUI_TARGET_CAN = 11,
  MOVEUI_TARGET_REFUEL = 12
};

#define MOVEUI_RETURN_ON_TARGET_MERC 1

enum UI_MODE {
  DONT_CHANGEMODE,
  IDLE_MODE,
  MOVE_MODE,
  ACTION_MODE,
  MENU_MODE,
  CONFIRM_MOVE_MODE,
  ADJUST_STANCE_MODE,
  CONFIRM_ACTION_MODE,
  HANDCURSOR_MODE,
  GETTINGITEM_MODE,
  ENEMYS_TURN_MODE,
  LOOKCURSOR_MODE,
  TALKINGMENU_MODE,
  TALKCURSOR_MODE,
  LOCKUI_MODE,
  OPENDOOR_MENU_MODE,
  LOCKOURTURN_UI_MODE,
  EXITSECTORMENU_MODE,
  RUBBERBAND_MODE,
  JUMPOVER_MODE,
};

struct UI_EVENT {
  typedef ScreenID (*UI_HANDLEFNC)(UI_EVENT *);

  uint32_t uiFlags;
  UI_MODE ChangeToUIMode;
  UI_HANDLEFNC HandleEvent;
  BOOLEAN fFirstTime;
  BOOLEAN fDoneMenu;
  UI_MODE uiMenuPreviousMode;
  uint32_t uiParams[3];
};

// EVENT ENUMERATION
enum UIEventKind {
  I_DO_NOTHING,
  I_NEW_MERC,
  I_NEW_BADMERC,
  I_SELECT_MERC,
  I_ENTER_EDIT_MODE,
  I_ENDTURN,
  I_TESTHIT,
  I_CHANGELEVEL,
  I_ON_TERRAIN,
  I_CHANGE_TO_IDLE,
  I_SOLDIERDEBUG,
  I_LOSDEBUG,
  I_LEVELNODEDEBUG,

  ET_ON_TERRAIN,
  ET_ENDENEMYS_TURN,

  M_ON_TERRAIN,
  M_CHANGE_TO_ACTION,
  M_CHANGE_TO_HANDMODE,
  M_CYCLE_MOVEMENT,
  M_CYCLE_MOVE_ALL,
  M_CHANGE_TO_ADJPOS_MODE,

  A_ON_TERRAIN,
  A_CHANGE_TO_MOVE,
  A_CHANGE_TO_CONFIM_ACTION,
  A_END_ACTION,
  U_MOVEMENT_MENU,
  U_POSITION_MENU,

  C_WAIT_FOR_CONFIRM,
  C_MOVE_MERC,
  C_ON_TERRAIN,

  PADJ_ADJUST_STANCE,

  CA_ON_TERRAIN,
  CA_MERC_SHOOT,
  CA_END_CONFIRM_ACTION,

  HC_ON_TERRAIN,

  G_GETTINGITEM,

  LC_ON_TERRAIN,
  LC_CHANGE_TO_LOOK,
  LC_LOOK,

  TA_TALKINGMENU,

  T_ON_TERRAIN,
  T_CHANGE_TO_TALKING,

  LU_ON_TERRAIN,
  LU_BEGINUILOCK,
  LU_ENDUILOCK,

  OP_OPENDOORMENU,

  LA_ON_TERRAIN,
  LA_BEGINUIOURTURNLOCK,
  LA_ENDUIOUTURNLOCK,

  EX_EXITSECTORMENU,

  RB_ON_TERRAIN,

  JP_ON_TERRAIN,
  JP_JUMP,

  NUM_UI_EVENTS
};

typedef BOOLEAN (*UIKEYBOARD_HOOK)(InputAtom *pInputEvent);

// GLOBAL STATUS VARS
extern UI_MODE gCurrentUIMode;
extern UIEventKind guiCurrentEvent;
extern UICursorID guiCurrentUICursor;
extern int16_t gsSelectedLevel;
extern BOOLEAN gfPlotNewMovement;
extern UIEventKind guiPendingOverrideEvent;

// GLOBALS
extern BOOLEAN gfUIDisplayActionPoints;
extern BOOLEAN gfUIDisplayActionPointsInvalid;
extern BOOLEAN gfUIDisplayActionPointsBlack;
extern BOOLEAN gfUIDisplayActionPointsCenter;
extern int16_t gUIDisplayActionPointsOffY;
extern int16_t gUIDisplayActionPointsOffX;
extern uint32_t guiShowUPDownArrows;
extern BOOLEAN gfUIHandleSelection;
extern int16_t gsSelectedGridNo;
extern SOLDIERTYPE *gSelectedGuy;

extern BOOLEAN gfUIMouseOnValidCatcher;
extern const SOLDIERTYPE *gUIValidCatcher;
extern BOOLEAN gUIUseReverse;

extern BOOLEAN gfUIHandleShowMoveGrid;
extern uint16_t gsUIHandleShowMoveGridLocation;

extern BOOLEAN gUITargetShotWaiting;

extern BOOLEAN gfUIWaitingForUserSpeechAdvance;

extern BOOLEAN gfUIAllMoveOn;
extern BOOLEAN gfUICanBeginAllMoveCycle;

extern BOOLEAN gfUIRefreshArrows;

extern BOOLEAN gfUIHandlePhysicsTrajectory;

// GLOBALS FOR FAST LOOKUP FOR FINDING MERCS FROM THE MOUSE
extern SOLDIERTYPE *gUIFullTarget;
extern SoldierFindFlags guiUIFullTargetFlags;

extern BOOLEAN gfUIConfirmExitArrows;
extern int16_t gsJumpOverGridNo;

ScreenID HandleTacticalUI();
ScreenID UIHandleEndTurn(UI_EVENT *);

extern BOOLEAN gfUIShowCurIntTile;

extern SGPRect gRubberBandRect;
extern BOOLEAN gRubberBandActive;

void EndMenuEvent(uint32_t uiEvent);
void SetUIKeyboardHook(UIKEYBOARD_HOOK KeyboardHookFnc);

extern BOOLEAN gfUIForceReExamineCursorData;

// FUNCTIONS IN INPUT MODULES
void GetKeyboardInput(UIEventKind *puiNewEvent);
void GetPolledKeyboardInput(UIEventKind *puiNewEvent);

void GetTBMouseButtonInput(UIEventKind *puiNewEvent);
void GetTBMousePositionInput(UIEventKind *puiNewEvent);
void HandleStanceChangeFromUIKeys(uint8_t ubAnimHeight);
void HandleKeyInputOnEnemyTurn();

BOOLEAN SelectedMercCanAffordAttack();
BOOLEAN SelectedMercCanAffordMove();

void ToggleHandCursorMode(UIEventKind *puiNewEvent);
void ToggleTalkCursorMode(UIEventKind *puiNewEvent);
void ToggleLookCursorMode();

void UIHandleSoldierStanceChange(SOLDIERTYPE *s, int8_t bNewStance);
MouseMoveState GetCursorMovementFlags();

BOOLEAN HandleUIMovementCursor(SOLDIERTYPE *, MouseMoveState, uint16_t usMapPos, MoveUITarget);
bool UIMouseOnValidAttackLocation(SOLDIERTYPE *);

BOOLEAN UIOkForItemPickup(SOLDIERTYPE *pSoldier, int16_t sGridNo);

SOLDIERTYPE *GetValidTalkableNPCFromMouse(BOOLEAN fGive, BOOLEAN fAllowMercs,
                                          BOOLEAN fCheckCollapsed);
BOOLEAN IsValidTalkableNPC(const SOLDIERTYPE *s, BOOLEAN fGive, BOOLEAN fAllowMercs,
                           BOOLEAN fCheckCollapsed);

BOOLEAN HandleTalkInit();

BOOLEAN HandleCheckForExitArrowsInput(BOOLEAN fAdjustForConfirm);

void SetUIBusy(const SOLDIERTYPE *s);
void UnSetUIBusy(const SOLDIERTYPE *s);

ScreenID UIHandleLUIEndLock(UI_EVENT *);

void BeginDisplayTimedCursor(UICursorID, uint32_t uiDelay);

void HandleHandCursorClick(uint16_t usMapPos, UIEventKind *puiNewEvent);

ScreenID UIHandleChangeLevel(UI_EVENT *);
BOOLEAN UIHandleOnMerc(BOOLEAN fMovementMode);

void ChangeInterfaceLevel(int16_t sLevel);

void EndRubberBanding();
void ResetMultiSelection();
void EndMultiSoldierSelection(BOOLEAN fAcknowledge);
void StopRubberBandedMercFromMoving();

BOOLEAN SelectedGuyInBusyAnimation();

void GotoLowerStance(SOLDIERTYPE *);
void GotoHigherStance(SOLDIERTYPE *);

BOOLEAN IsValidJumpLocation(const SOLDIERTYPE *pSoldier, int16_t sGridNo, BOOLEAN fCheckForPath);

void PopupAssignmentMenuInTactical();

extern GridNo gfUIOverItemPoolGridNo;
extern int16_t gsCurrentActionPoints;
extern BOOLEAN fRightButtonDown;
extern BOOLEAN fLeftButtonDown;
extern BOOLEAN fIgnoreLeftUp;
extern BOOLEAN gfIgnoreOnSelectedGuy;
extern BOOLEAN gUIActionModeChangeDueToMouseOver;
extern MOUSE_REGION gDisableRegion;
extern BOOLEAN gfDisableRegionActive;
extern MOUSE_REGION gUserTurnRegion;
extern BOOLEAN gfUserTurnRegionActive;
extern UIKEYBOARD_HOOK gUIKeyboardHook;
extern BOOLEAN gfResetUIMovementOptimization;

extern BOOLEAN gfUIShowExitEast;
extern BOOLEAN gfUIShowExitWest;
extern BOOLEAN gfUIShowExitNorth;
extern BOOLEAN gfUIShowExitSouth;

BOOLEAN ValidQuickExchangePosition();

void CheckForDisabledRegionRemove();

void HandleTacticalUILoseCursorFromOtherScreen();

void SetInterfaceHeightLevel();

#endif
