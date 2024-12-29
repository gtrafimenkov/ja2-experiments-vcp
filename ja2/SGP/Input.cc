// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#include "SGP/Input.h"

#include <map>

#include "Local.h"
#include "Macro.h"
#include "SGP/English.h"
#include "SGP/MemMan.h"
#include "SGP/Types.h"
#include "SGP/UTF8String.h"
#include "SGP/Video.h"
#include "jplatform_events.h"
#include "jplatform_input.h"
#include "jplatform_time.h"
#include "jplatform_video.h"

extern struct JVideoState *g_videoState;

// The pressedKeys table is used to track which of the keys is up or down at any
// one time. This is used while polling the interface.
static std::map<JInput_VirtualKey, bool> pressedKeys;  // true = Pressed, false = Not Pressed

static BOOLEAN fCursorWasClipped = FALSE;
static SGPRect gCursorClipRect;

// These data structure are used to track the mouse while polling

static uint32_t guiSingleClickTimer;

static uint32_t guiLeftButtonRepeatTimer;
static uint32_t guiRightButtonRepeatTimer;

BOOLEAN gfLeftButtonState;   // TRUE = Pressed, FALSE = Not Pressed
BOOLEAN gfRightButtonState;  // TRUE = Pressed, FALSE = Not Pressed
uint16_t gusMouseXPos;       // X position of the mouse on screen
uint16_t gusMouseYPos;       // y position of the mouse on screen

// The queue structures are used to track input events using queued events

static InputAtom gEventQueue[256];
static uint16_t gusQueueCount;
static uint16_t gusHeadIndex;
static uint16_t gusTailIndex;

static void QueueMouseEvent(uint16_t ubInputEvent) {
  // Can we queue up one more event, if not, the event is lost forever
  if (gusQueueCount == lengthof(gEventQueue)) return;

  gEventQueue[gusTailIndex].usEvent = ubInputEvent;

  gusQueueCount++;

  gusTailIndex = (gusTailIndex + 1) % lengthof(gEventQueue);
}

static void QueueKeyEvent(uint16_t ubInputEvent, JInput_VirtualKey key, JInput_KeyMod mod,
                          wchar_t Char) {
  // Can we queue up one more event, if not, the event is lost forever
  if (gusQueueCount == lengthof(gEventQueue)) return;

  gEventQueue[gusTailIndex].shift = mod.shift;
  gEventQueue[gusTailIndex].ctrl = mod.ctrl;
  gEventQueue[gusTailIndex].alt = mod.alt;
  gEventQueue[gusTailIndex].usEvent = ubInputEvent;
  gEventQueue[gusTailIndex].key = key;
  gEventQueue[gusTailIndex].Char = Char;

  gusQueueCount++;

  gusTailIndex = (gusTailIndex + 1) % lengthof(gEventQueue);
}

void SetSafeMousePosition(int32_t x, int32_t y) {
  if (x < 0) x = 0;
  if (y < 0) y = 0;
  if (x > SCREEN_WIDTH) x = SCREEN_WIDTH;
  if (y > SCREEN_HEIGHT) y = SCREEN_HEIGHT;

  gusMouseXPos = x;
  gusMouseYPos = y;
}

BOOLEAN DequeueSpecificEvent(InputAtom *Event, uint32_t uiMaskFlags) {
  // Is there an event to dequeue
  if (gusQueueCount > 0) {
    // Check if it has the masks!
    if (gEventQueue[gusHeadIndex].usEvent & uiMaskFlags) {
      return DequeueEvent(Event);
    }
  }

  return FALSE;
}

static void HandleSingleClicksAndButtonRepeats();

BOOLEAN DequeueEvent(InputAtom *Event) {
  HandleSingleClicksAndButtonRepeats();

  if (gusQueueCount == 0) return FALSE;

  *Event = gEventQueue[gusHeadIndex];
  gusHeadIndex = (gusHeadIndex + 1) % lengthof(gEventQueue);
  gusQueueCount--;
  return TRUE;
}

void MouseButtonDown(const struct JEvent_MouseButtonPress *e) {
  SetSafeMousePosition(e->x, e->y);
  if (e->left) {
    guiLeftButtonRepeatTimer = JTime_GetTicks() + BUTTON_REPEAT_TIMEOUT;
    gfLeftButtonState = TRUE;
    QueueMouseEvent(LEFT_BUTTON_DOWN);
  }

  if (e->right) {
    guiRightButtonRepeatTimer = JTime_GetTicks() + BUTTON_REPEAT_TIMEOUT;
    gfRightButtonState = TRUE;
    QueueMouseEvent(RIGHT_BUTTON_DOWN);
  }
}

void MouseButtonUp(const struct JEvent_MouseButtonPress *e) {
  SetSafeMousePosition(e->x, e->y);
  if (e->left) {
    guiLeftButtonRepeatTimer = 0;
    gfLeftButtonState = FALSE;
    QueueMouseEvent(LEFT_BUTTON_UP);
    uint32_t uiTimer = JTime_GetTicks();
    if (uiTimer - guiSingleClickTimer < DBL_CLK_TIME) {
      QueueMouseEvent(LEFT_BUTTON_DBL_CLK);
    } else {
      guiSingleClickTimer = uiTimer;
    }
  }

  if (e->right) {
    guiRightButtonRepeatTimer = 0;
    gfRightButtonState = FALSE;
    QueueMouseEvent(RIGHT_BUTTON_UP);
  }
}

void MouseWheelScroll(const struct JEvent_MouseWheel *e) {
  if (e->y > 0) {
    QueueMouseEvent(MOUSE_WHEEL_UP);
  } else {
    QueueMouseEvent(MOUSE_WHEEL_DOWN);
  }
}

static void KeyChange(JInput_VirtualKey key, JInput_KeyMod mod, bool const pressed) {
  switch (key) {
    case JIK_KP_0:
      key = mod.num ? '0' : JIK_INSERT;
      break;
    case JIK_KP_1:
      key = mod.num ? '1' : JIK_END;
      break;
    case JIK_KP_2:
      key = mod.num ? '2' : JIK_DOWN;
      break;
    case JIK_KP_3:
      key = mod.num ? '3' : JIK_PAGEDOWN;
      break;
    case JIK_KP_4:
      key = mod.num ? '4' : JIK_LEFT;
      break;
    case JIK_KP_5:
      if (!mod.num) return;
      key = '5';
      break;
    case JIK_KP_6:
      key = mod.num ? '6' : JIK_RIGHT;
      break;
    case JIK_KP_7:
      key = mod.num ? '7' : JIK_HOME;
      break;
    case JIK_KP_8:
      key = mod.num ? '8' : JIK_UP;
      break;
    case JIK_KP_9:
      key = mod.num ? '9' : JIK_PAGEUP;
      break;
    case JIK_KP_PERIOD:
      key = mod.num ? '.' : JIK_DELETE;
      break;
    case JIK_KP_DIVIDE:
      key = '/';
      break;
    case JIK_KP_MULTIPLY:
      key = '*';
      break;
    case JIK_KP_MINUS:
      key = '-';
      break;
    case JIK_KP_PLUS:
      key = '+';
      break;
    case JIK_KP_ENTER:
      key = JIK_RETURN;
      break;
  }

  uint32_t event_type;
  bool keyPressed = IsKeyDown(key);
  if (pressed) {
    // key down
    event_type = keyPressed ? KEY_REPEAT : KEY_DOWN;
  } else {
    // key up
    if (!keyPressed) return;
    event_type = KEY_UP;
  }
  pressedKeys[key] = pressed;

  QueueKeyEvent(event_type, key, mod, '\0');
}

void KeyDown(JInput_VirtualKey key, JInput_KeyMod mod) {
  switch (key) {
    case JIK_LSHIFT:
    case JIK_RSHIFT:
      pressedKeys[SHIFT] = true;
      break;

    case JIK_LCTRL:
    case JIK_RCTRL:
      pressedKeys[CTRL] = true;
      break;

    case JIK_LALT:
    case JIK_RALT:
      pressedKeys[ALT] = true;
      break;

    case JIK_PRINTSCREEN:
    case JIK_SCROLLLOCK:
      break;

    default:
      KeyChange(key, mod, true);
      break;
  }
}

void KeyUp(JInput_VirtualKey key, JInput_KeyMod mod) {
  switch (key) {
    case JIK_LSHIFT:
    case JIK_RSHIFT:
      pressedKeys[SHIFT] = false;
      break;

    case JIK_LCTRL:
    case JIK_RCTRL:
      pressedKeys[CTRL] = false;
      break;

    case JIK_LALT:
    case JIK_RALT:
      pressedKeys[ALT] = false;
      break;

    case JIK_PRINTSCREEN:
      if (mod.ctrl)
        VideoCaptureToggle();
      else
        PrintScreen();
      break;

    case JIK_SCROLLLOCK:
      JVideo_ToggleMouseGrab(g_videoState);
      break;

    case JIK_RETURN:
      if (IsKeyDown(ALT)) {
        VideoToggleFullScreen();
        break;
      }
      /* FALLTHROUGH */

    default:
      KeyChange(key, mod, false);
      break;
  }
}

void TextInput(const char *text) {
  UTF8String utf8String = UTF8String(text);
  struct JInput_KeyMod mod = {};
  QueueKeyEvent(TEXT_INPUT, JIK_UNKNOWN, mod, utf8String.getUTF16()[0]);
}

void GetMousePos(SGPPoint *Point) {
  Point->iX = gusMouseXPos;
  Point->iY = gusMouseYPos;
}

void RestrictMouseToXYXY(uint16_t usX1, uint16_t usY1, uint16_t usX2, uint16_t usY2) {
  SGPRect TempRect;
  TempRect.iLeft = usX1;
  TempRect.iTop = usY1;
  TempRect.iRight = usX2;
  TempRect.iBottom = usY2;
  RestrictMouseCursor(&TempRect);
}

void RestrictMouseCursor(const SGPRect *pRectangle) {
  // Make a copy of our rect....
  gCursorClipRect = *pRectangle;
#if 1  // XXX TODO0000 Should probably removed completly. Confining the mouse
       // cursor is The Wrong Thing(tm)
#else
  ClipCursor((RECT *)pRectangle);
#endif
  fCursorWasClipped = TRUE;
}

void FreeMouseCursor() {
#if 1  // XXX TODO0000
#else
  ClipCursor(NULL);
#endif
  fCursorWasClipped = FALSE;
}

void RestoreCursorClipRect() {
#if 1  // XXX TODO0000
  UNIMPLEMENTED
#else
  if (fCursorWasClipped) {
    ClipCursor(&gCursorClipRect);
  }
#endif
}

void GetRestrictedClipCursor(SGPRect *pRectangle) {
#if 1  // XXX TODO0000
  pRectangle->iLeft = 0;
  pRectangle->iTop = 0;
  pRectangle->iRight = SCREEN_WIDTH;
  pRectangle->iBottom = SCREEN_HEIGHT;
#else
  GetClipCursor((RECT *)pRectangle);
#endif
}

BOOLEAN IsCursorRestricted() { return fCursorWasClipped; }

void SimulateMouseMovement(uint32_t uiNewXPos, uint32_t uiNewYPos) {
  JVideo_SimulateMouseMovement(g_videoState, uiNewXPos, uiNewYPos);
}

void DequeueAllKeyBoardEvents() {
#if 1  // XXX TODO
  FIXME
#else
  // dequeue all the events waiting in the windows queue
  MSG KeyMessage;
  while (PeekMessage(&KeyMessage, ghWindow, WM_KEYFIRST, WM_KEYLAST, PM_REMOVE));

  // Deque all the events waiting in the SGP queue
  InputAtom InputEvent;
  while (DequeueEvent(&InputEvent)) {
    // dont do anything
  }
#endif
}

static void HandleSingleClicksAndButtonRepeats() {
  uint32_t uiTimer = JTime_GetTicks();

  // Is there a LEFT mouse button repeat
  if (gfLeftButtonState) {
    if ((guiLeftButtonRepeatTimer > 0) && (guiLeftButtonRepeatTimer <= uiTimer)) {
      QueueMouseEvent(LEFT_BUTTON_REPEAT);
      guiLeftButtonRepeatTimer = uiTimer + BUTTON_REPEAT_TIME;
    }
  } else {
    guiLeftButtonRepeatTimer = 0;
  }

  // Is there a RIGHT mouse button repeat
  if (gfRightButtonState) {
    if ((guiRightButtonRepeatTimer > 0) && (guiRightButtonRepeatTimer <= uiTimer)) {
      QueueMouseEvent(RIGHT_BUTTON_REPEAT);
      guiRightButtonRepeatTimer = uiTimer + BUTTON_REPEAT_TIME;
    }
  } else {
    guiRightButtonRepeatTimer = 0;
  }
}

BOOLEAN IsKeyDown(JInput_VirtualKey key) {
  return pressedKeys.count(key) > 0 ? pressedKeys[key] : false;
}
