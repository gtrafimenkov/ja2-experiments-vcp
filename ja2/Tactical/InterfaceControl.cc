// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#include "Tactical/InterfaceControl.h"

#include "JAScreens.h"
#include "Local.h"
#include "Macro.h"
#include "SGP/ButtonSystem.h"
#include "SGP/Font.h"
#include "SGP/HImage.h"
#include "SGP/Input.h"
#include "SGP/Line.h"
#include "SGP/MouseSystem.h"
#include "SGP/VObject.h"
#include "SGP/VObjectBlitters.h"
#include "SGP/VSurface.h"
#include "Strategic/Assignments.h"
#include "Strategic/GameClock.h"
#include "Strategic/MapScreen.h"
#include "Strategic/MapScreenInterface.h"
#include "Tactical/AnimationData.h"
#include "Tactical/CivQuotes.h"
#include "Tactical/Faces.h"
#include "Tactical/HandleItems.h"
#include "Tactical/HandleUI.h"
#include "Tactical/Interface.h"
#include "Tactical/InterfaceCursors.h"
#include "Tactical/InterfaceDialogue.h"
#include "Tactical/InterfaceItems.h"
#include "Tactical/InterfacePanels.h"
#include "Tactical/Overhead.h"
#include "Tactical/PathAI.h"
#include "Tactical/SoldierFind.h"
#include "Tactical/SpreadBurst.h"
#include "Tactical/Squads.h"
#include "Tactical/StrategicExitGUI.h"
#include "TileEngine/InteractiveTiles.h"
#include "TileEngine/IsometricUtils.h"
#include "TileEngine/RadarScreen.h"
#include "TileEngine/RenderDirty.h"
#include "TileEngine/RenderWorld.h"
#include "TileEngine/SysUtil.h"
#include "TileEngine/TileDat.h"
#include "TileEngine/WorldMan.h"
#include "Utils/Cursors.h"
#include "Utils/FontControl.h"
#include "Utils/PopUpBox.h"
#include "Utils/TimerControl.h"

static SGPRect gOldClippingRect;
static SGPRect gOldDirtyClippingRect;

static uint16_t gusUICurIntTileEffectIndex;
static int16_t gsUICurIntTileEffectGridNo;
static uint8_t gsUICurIntTileOldShade;

BOOLEAN gfRerenderInterfaceFromHelpText = FALSE;

DirtyLevel gfPausedTacticalRenderInterfaceFlags = DIRTYLEVEL0;
BOOLEAN gfPausedTacticalRenderFlags = FALSE;

static bool g_switch_panel = false;
static SOLDIERTYPE *g_new_panel_soldier = 0;

void SetNewPanel(SOLDIERTYPE *const s) {
  g_switch_panel = true;
  g_new_panel_soldier = s;
}

void HandleTacticalPanelSwitch() {
  if (!g_switch_panel) return;
  g_switch_panel = false;

  SOLDIERTYPE *const s = g_new_panel_soldier;
  SetCurrentInterfacePanel(s ? SM_PANEL : TEAM_PANEL);
  SetCurrentTacticalPanelCurrentMerc(s);

  if (guiCurrentScreen != SHOPKEEPER_SCREEN)  // XXX necessary?
  {
    RenderButtons();
  }
}

static void RenderPanel() {
  switch (gsCurInterfacePanel) {
    case SM_PANEL:
      RenderSMPanel(&fInterfacePanelDirty);
      break;
    case TEAM_PANEL:
      RenderTEAMPanel(fInterfacePanelDirty);
      break;
    default:
      break;
  }
}

static void HandlePausedTacticalRender();

void RenderTacticalInterface() {
  // handle paused render of tactical
  HandlePausedTacticalRender();

  if (!fInMapMode)  // XXX necessary?
  {
    HandleFlashingItems();

    HandleMultiPurposeLocator();
  }

  // Handle degrading new items...
  DegradeNewlyAddedItems();

  RenderPanel();

  // Handle faces
  if (guiCurrentScreen != SHOPKEEPER_SCREEN) HandleAutoFaces();
}

/* handle paused render of tactical panel, if flag set, OR it in with tactical
 * render flags then reset */
static void HandlePausedTacticalRender() {
  // for a one frame paused render of tactical
  if (gfPausedTacticalRenderFlags) {
    SetRenderFlags(RENDER_FLAG_FULL);
    gfPausedTacticalRenderFlags = FALSE;
  }

  if (gfPausedTacticalRenderInterfaceFlags != DIRTYLEVEL0) {
    fInterfacePanelDirty = gfPausedTacticalRenderInterfaceFlags;
    gfPausedTacticalRenderInterfaceFlags = DIRTYLEVEL0;
  }
}

void RenderTacticalInterfaceWhileScrolling() {
  RenderButtons();
  RenderPanel();
  HandleAutoFaces();
}

void SetUpInterface() {
  if (fInMapMode) return;  // XXX necessary?

  DrawUICursor();
  SetupPhysicsTrajectoryUI();

  if (g_ui_message_overlay && GetJA2Clock() - guiUIMessageTime > guiUIMessageTimeDelay) {
    EndUIMessage();
  }

  if (gCurrentUIMode == OPENDOOR_MENU_MODE) HandleOpenDoorMenu();

  HandleTalkingMenu();

  if (gCurrentUIMode == EXITSECTORMENU_MODE) HandleSectorExitMenu();

  // For the most part - shutdown interface when it's the enemy's turn
  if (gTacticalStatus.ubCurrentTeam != OUR_TEAM) return;

  HandleInterfaceBackgrounds();

  switch (gfUIHandleSelection) {
    uint16_t idx;

    case NONSELECTED_GUY_SELECTION:
      idx = GOODRING1;
      goto add_node;
    // case SELECTED_GUY_SELECTION:    idx = SELRING1;       goto add_node;
    case SELECTED_GUY_SELECTION:
      idx = FIRSTPOINTERS2;
      goto add_node;
    case ENEMY_GUY_SELECTION:
      idx = FIRSTPOINTERS2;
      goto add_node;
    default:
      break;

    add_node:
      LEVELNODE *(&add)(uint32_t, uint16_t) =
          gsSelectedLevel == 0 ? AddObjectToHead : AddRoofToHead;
      LEVELNODE *const n = add(gsSelectedGridNo, idx);
      n->ubShadeLevel = DEFAULT_SHADE_LEVEL;
      n->ubNaturalShadeLevel = DEFAULT_SHADE_LEVEL;
      break;
  }

  if (gfUIHandleShowMoveGrid) {
    SOLDIERTYPE const *const sel = GetSelectedMan();
    if (sel && sel->sGridNo != gsUIHandleShowMoveGridLocation) {
      uint16_t const idx = gfUIHandleShowMoveGrid == 2 ? FIRSTPOINTERS4
                           : sel->bStealthMode         ? FIRSTPOINTERS9
                                                       : FIRSTPOINTERS2;
      LEVELNODE *(&add)(uint32_t, uint16_t) =
          gsInterfaceLevel == 0 ? AddTopmostToHead : AddOnRoofToHead;
      LEVELNODE *const n = add(gsUIHandleShowMoveGridLocation, GetSnapCursorIndex(idx));
      n->ubShadeLevel = DEFAULT_SHADE_LEVEL;
      n->ubNaturalShadeLevel = DEFAULT_SHADE_LEVEL;
    }
  }

  if (gfUIShowCurIntTile) {  // Check if we are over an interactive tile
    if (LEVELNODE *const int_tile = GetCurInteractiveTileGridNo(&gsUICurIntTileEffectGridNo)) {
      gusUICurIntTileEffectIndex = int_tile->usIndex;
      // Shade green
      gsUICurIntTileOldShade = int_tile->ubShadeLevel;
      int_tile->ubShadeLevel = 0;
      int_tile->uiFlags |= LEVELNODE_DYNAMIC;
    }
  }
}

void ResetInterface() {
  if (fInMapMode) return;  // XXX necessary?

  // Find out if we need to show any menus
  DetermineWhichAssignmentMenusCanBeShown();
  CreateDestroyAssignmentPopUpBoxes();

  HideUICursor();
  ResetPhysicsTrajectoryUI();

  if (gfUIHandleSelection) {
    BOOLEAN(&remove)
    (uint32_t, uint16_t) = gsSelectedLevel == 0 ? RemoveObject : RemoveRoof;
    GridNo const gridno = gsSelectedGridNo;
    remove(gridno, GOODRING1);
    remove(gridno, FIRSTPOINTERS2);
  }

  if (gfUIHandleShowMoveGrid) {
    BOOLEAN(&remove)
    (uint32_t, uint16_t) = gsInterfaceLevel == 0 ? RemoveTopmost : RemoveOnRoof;
    GridNo const gridno = gsUIHandleShowMoveGridLocation;
    remove(gridno, FIRSTPOINTERS2);
    remove(gridno, FIRSTPOINTERS4);
    remove(gridno, FIRSTPOINTERS9);
    remove(gridno, FIRSTPOINTERS13);
    remove(gridno, FIRSTPOINTERS15);
    remove(gridno, FIRSTPOINTERS19);
    remove(gridno, FIRSTPOINTERS20);
  }

  fInterfacePanelDirty = DIRTYLEVEL0;

  // Reset int tile cursor stuff
  if (gfUIShowCurIntTile && gsUICurIntTileEffectGridNo != NOWHERE) {
    for (LEVELNODE *i = gpWorldLevelData[gsUICurIntTileEffectGridNo].pStructHead; i; i = i->pNext) {
      LEVELNODE &n = *i;
      if (n.usIndex != gusUICurIntTileEffectIndex) continue;
      n.ubShadeLevel = gsUICurIntTileOldShade;
      n.uiFlags &= ~LEVELNODE_DYNAMIC;
      break;
    }
  }
}

static uint32_t const guiColors[] = {
    FROMRGB(198, 163, 0), FROMRGB(185, 150, 0), FROMRGB(172, 136, 0), FROMRGB(159, 123, 0),
    FROMRGB(146, 110, 0), FROMRGB(133, 96, 0),  FROMRGB(120, 83, 0),  FROMRGB(133, 96, 0),
    FROMRGB(146, 110, 0), FROMRGB(159, 123, 0), FROMRGB(172, 136, 0), FROMRGB(185, 150, 0)};

static void RenderRubberBanding() {
  static int32_t flash_colour = 0;
  static int32_t time_of_last_update = 0;

  if (!gRubberBandActive) return;

  int16_t l = gRubberBandRect.iLeft;
  int16_t r = gRubberBandRect.iRight;
  int16_t t = gRubberBandRect.iTop;
  int16_t b = gRubberBandRect.iBottom;

  if (l == r && t == b) return;

  uint32_t const now = GetJA2Clock();
  if (now - time_of_last_update > 60) {
    time_of_last_update = now;
    if (++flash_colour == lengthof(guiColors)) flash_colour = 0;
  }
  uint16_t const colour = Get16BPPColor(guiColors[flash_colour]);

  // Draw rectangle.....
  SGPVSurface::Lock lock(FRAME_BUFFER);
  uint16_t *const pDestBuf = lock.Buffer<uint16_t>();
  SetClippingRegionAndImageWidth(lock.Pitch(), 0, 0, gsVIEWPORT_END_X, gsVIEWPORT_WINDOW_END_Y);

  if (l != r) {
    if (l > r) Swap(l, r);
    LineDraw(TRUE, l, t, r, t, colour, pDestBuf);
    RegisterBackgroundRectSingleFilled(l, t, r - l + 1, 1);
    LineDraw(TRUE, l, b, r, b, colour, pDestBuf);
    RegisterBackgroundRectSingleFilled(l, b, r - l + 1, 1);
  }

  if (t != b) {
    if (t > b) Swap(t, b);
    LineDraw(TRUE, l, t, l, b, colour, pDestBuf);
    RegisterBackgroundRectSingleFilled(l, t, 1, b - t + 1);
    LineDraw(TRUE, r, t, r, b, colour, pDestBuf);
    RegisterBackgroundRectSingleFilled(r, t, 1, b - t + 1);
  }
}

static void EndViewportOverlays();
static void StartViewportOverlays();

void RenderTopmostTacticalInterface() {
  if (gfRerenderInterfaceFromHelpText) {
    fInterfacePanelDirty = DIRTYLEVEL2;
    RenderPanel();
    gfRerenderInterfaceFromHelpText = FALSE;
  }

  if (fInMapMode)  // XXX necessary?
  {
    // If we want to rederaw whole screen, dirty all buttons!
    if (fInterfacePanelDirty == DIRTYLEVEL2) MarkButtonsDirty();
    RenderButtons();
    return;
  }

  if (InItemStackPopup()) {
    RenderItemStackPopup(fInterfacePanelDirty == DIRTYLEVEL2);
  }

  if (InKeyRingPopup() && !InItemDescriptionBox()) {
    RenderKeyRingPopup(fInterfacePanelDirty == DIRTYLEVEL2);
  }

  if (gfInMovementMenu) {
    RenderMovementMenu();
  }

  // Syncronize for upcoming soldier counters
  SYNCTIMECOUNTER();

  // Setup system for video overlay (text and blitting) Sets clipping rects, etc
  StartViewportOverlays();

  RenderTopmostFlashingItems();
  RenderTopmostMultiPurposeLocator();
  RenderAccumulatedBurstLocations();

  FOR_EACH_MERC(i) {
    SOLDIERTYPE &s = **i;
    DrawSelectedUIAboveGuy(s);

    if (!s.fDisplayDamage) continue;
    if (s.sGridNo == NOWHERE) continue;
    if (s.bVisible == -1) continue;

    int16_t sMercScreenX;
    int16_t sMercScreenY;
    GetSoldierTRUEScreenPos(&s, &sMercScreenX, &sMercScreenY);

    int16_t x = sMercScreenX + s.sDamageX;
    int16_t y = sMercScreenY + s.sDamageY;
    if (s.ubBodyType == QUEENMONSTER) {
      x += 25;
      y += 10;
    } else {
      x += 2 * 30 / 3;
      y += -5;

      if (y < gsVIEWPORT_WINDOW_START_Y) {
        y = sMercScreenY - s.sBoundingBoxOffsetY;
      }
    }

    SetFontAttributes(TINYFONT1, FONT_MCOLOR_WHITE);
    GDirtyPrintF(x, y, L"-%d", s.sDamage);
  }

  // FOR THE MOST PART, DISABLE INTERFACE STUFF WHEN IT'S ENEMY'S TURN
  if (gTacticalStatus.ubCurrentTeam == OUR_TEAM) {
    RenderArrows();
  }

  EndViewportOverlays();
  RenderRubberBanding();

  if (!gfInItemPickupMenu && !gpItemPointer) {
    HandleAnyMercInSquadHasCompatibleStuff(NULL);
  }

  // CHECK IF OUR CURSOR IS OVER AN INV POOL
  GridNo const usMapPos = GetMouseMapPos();
  SOLDIERTYPE *const sel = GetSelectedMan();
  if (usMapPos != NOWHERE && gfUIOverItemPoolGridNo != NOWHERE && sel) {
    // Check if we are over an item pool
    int8_t level = sel->bLevel;
    ITEM_POOL const *item_pool = GetItemPool(gfUIOverItemPoolGridNo, level);
    if (!item_pool) {
      // ATE: Allow to see list if a different level....
      level = (level == 0 ? 1 : 0);
      item_pool = GetItemPool(gfUIOverItemPoolGridNo, level);
    }

    if (item_pool) {
      STRUCTURE *pStructure;
      int16_t sIntTileGridNo;
      int16_t const sActionGridNo =
          ConditionalGetCurInteractiveTileGridNoAndStructure(&sIntTileGridNo, &pStructure, FALSE)
              ? sIntTileGridNo
              : usMapPos;

      int8_t const bZLevel = GetZLevelOfItemPoolGivenStructure(sActionGridNo, level, pStructure);
      if (AnyItemsVisibleOnLevel(item_pool, bZLevel)) {
        DrawItemPoolList(item_pool, bZLevel, gusMouseXPos, gusMouseYPos);
        // ATE: If over items, remove locator....
        RemoveFlashItemSlot(item_pool);
      }
    }
  }

  switch (gCurrentUIMode) {
    case GETTINGITEM_MODE:
      SetItemPickupMenuDirty(DIRTYLEVEL2);
      RenderItemPickupMenu();
      break;

    case OPENDOOR_MENU_MODE:
      RenderOpenDoorMenu();
      break;
    default:
      break;
  }

  if (gfInTalkPanel) RenderTalkingMenu();

  if (gfInSectorExitMenu) RenderSectorExitMenu();

  if (fRenderRadarScreen) {
    RenderClock();
    RenderTownIDString();
    CreateMouseRegionForPauseOfClock();
  } else {
    RemoveMouseRegionForPauseOfClock();
  }

  // If we want to rederaw whole screen, dirty all buttons!
  if (fInterfacePanelDirty == DIRTYLEVEL2) MarkButtonsDirty();

  RenderButtons();
  RenderPausedGameBox();

  MarkAllBoxesAsAltered();

  HandleShowingOfTacticalInterfaceFastHelpText();
  HandleShadingOfLinesForAssignmentMenus();
  DetermineBoxPositions();
  DisplayBoxes(FRAME_BUFFER);
}

static void StartViewportOverlays() {
  // Set Clipping Rect to be the viewscreen
  // Save old one
  gOldClippingRect = ClippingRect;

  // Save old dirty clipping rect
  gOldDirtyClippingRect = ClippingRect;

  // Set bottom clipping value for blitter clipping rect
  ClippingRect.iLeft = INTERFACE_START_X;
  ClippingRect.iTop = gsVIEWPORT_WINDOW_START_Y;
  ClippingRect.iRight = SCREEN_WIDTH;
  ClippingRect.iBottom = gsVIEWPORT_WINDOW_END_Y;

  // Set values for dirty rect clipping rect
  gDirtyClipRect.iLeft = INTERFACE_START_X;
  gDirtyClipRect.iTop = gsVIEWPORT_WINDOW_START_Y;
  gDirtyClipRect.iRight = SCREEN_WIDTH;
  gDirtyClipRect.iBottom = gsVIEWPORT_WINDOW_END_Y;

  SaveFontSettings();
  SetFontDestBuffer(FRAME_BUFFER, 0, gsVIEWPORT_WINDOW_START_Y, SCREEN_WIDTH,
                    gsVIEWPORT_WINDOW_END_Y);
}

static void EndViewportOverlays() {
  // Reset clipping rect
  ClippingRect = gOldClippingRect;
  gDirtyClipRect = gOldDirtyClippingRect;
  RestoreFontSettings();
}

void EraseInterfaceMenus(BOOLEAN fIgnoreUIUnLock) {
  // ATE: If we are currently talking, setup this flag so that the
  // automatic handler in handledialogue doesn't adjust the UI setting
  if ((gTacticalStatus.uiFlags & ENGAGED_IN_CONV) && fIgnoreUIUnLock) {
    gTacticalStatus.uiFlags |= IGNORE_ENGAGED_IN_CONV_UI_UNLOCK;
  }

  // Remove item pointer if one active
  CancelItemPointer();

  ShutDownQuoteBoxIfActive();
  PopDownMovementMenu();
  PopDownOpenDoorMenu();
  DeleteTalkingMenu();
}

bool AreWeInAUIMenu() {
  return gfInMovementMenu || gfInOpenDoorMenu || gfInItemPickupMenu || gfInSectorExitMenu ||
         gfInTalkPanel;
}

void ResetInterfaceAndUI() {
  // Erase menus
  EraseInterfaceMenus(FALSE);

  EraseRenderArrows();

  EndRubberBanding();

  // ResetMultiSelection( );

  if (g_ui_message_overlay != NULL) {
    RemoveVideoOverlay(g_ui_message_overlay);
    g_ui_message_overlay = NULL;
  }

  // Set UI back to movement...
  guiPendingOverrideEvent = M_ON_TERRAIN;
  HandleTacticalUI();
}

bool InterfaceOKForMeanwhilePopup() { return !g_switch_panel; }
