// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#include "SGP/Input.h"

#include <map>

#include "Local.h"
#include "Macro.h"
#include "SGP/English.h"
#include "SGP/MemMan.h"
#include "SGP/Timer.h"
#include "SGP/Types.h"
#include "SGP/UTF8String.h"
#include "SGP/Video.h"

#include "SDL_events.h"
#include "SDL_keycode.h"

// The pressedKeys table is used to track which of the keys is up or down at any
// one time. This is used while polling the interface.
static std::map<uint32_t, bool> pressedKeys;  // true = Pressed, false = Not Pressed

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

static void QueueKeyEvent(uint16_t ubInputEvent, SDL_Keycode Key, SDL_Keymod Mod, wchar_t Char) {
  // Can we queue up one more event, if not, the event is lost forever
  if (gusQueueCount == lengthof(gEventQueue)) return;

  uint16_t ModifierState = 0;
  if (Mod & KMOD_SHIFT) ModifierState |= SHIFT_DOWN;
  if (Mod & KMOD_CTRL) ModifierState |= CTRL_DOWN;
  if (Mod & KMOD_ALT) ModifierState |= ALT_DOWN;
  gEventQueue[gusTailIndex].usKeyState = ModifierState;
  gEventQueue[gusTailIndex].usEvent = ubInputEvent;
  gEventQueue[gusTailIndex].usParam = Key;
  gEventQueue[gusTailIndex].Char = Char;

  gusQueueCount++;

  gusTailIndex = (gusTailIndex + 1) % lengthof(gEventQueue);
}

void SetSafeMousePosition(int x, int y) {
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

static void UpdateMousePos(const SDL_MouseButtonEvent *BtnEv) {
  SetSafeMousePosition(BtnEv->x, BtnEv->y);
}

#if defined(WITH_MAEMO) || defined __APPLE__
static BOOLEAN g_down_right;
#endif

void MouseButtonDown(const SDL_MouseButtonEvent *BtnEv) {
  UpdateMousePos(BtnEv);
  switch (BtnEv->button) {
    case SDL_BUTTON_LEFT: {
#if defined WITH_MAEMO
      /* If the menu button (mapped to F4) is pressed, then treat the event as
       * right click */
      const Uint8 *const key_state = SDL_GetKeyboardState(NULL);
      g_down_right = key_state[SDL_SCANCODE_F4];
      if (g_down_right) goto right_button;
#endif
#if defined(__APPLE__)
      const Uint8 *const key_state = SDL_GetKeyboardState(NULL);
      g_down_right = key_state[SDL_SCANCODE_LGUI] || key_state[SDL_SCANCODE_RGUI];
      if (g_down_right) goto right_button;
#endif
      guiLeftButtonRepeatTimer = GetClock() + BUTTON_REPEAT_TIMEOUT;
      gfLeftButtonState = TRUE;
      QueueMouseEvent(LEFT_BUTTON_DOWN);
      break;
    }

    case SDL_BUTTON_RIGHT:
#if defined(WITH_MAEMO) || defined(__APPLE__)
    right_button:
#endif
      guiRightButtonRepeatTimer = GetClock() + BUTTON_REPEAT_TIMEOUT;
      gfRightButtonState = TRUE;
      QueueMouseEvent(RIGHT_BUTTON_DOWN);
      break;
  }
}

void MouseButtonUp(const SDL_MouseButtonEvent *BtnEv) {
  UpdateMousePos(BtnEv);
  switch (BtnEv->button) {
    case SDL_BUTTON_LEFT: {
#if defined(WITH_MAEMO) || defined(__APPLE__)
      if (g_down_right) goto right_button;
#endif
      guiLeftButtonRepeatTimer = 0;
      gfLeftButtonState = FALSE;
      QueueMouseEvent(LEFT_BUTTON_UP);
      uint32_t uiTimer = GetClock();
      if (uiTimer - guiSingleClickTimer < DBL_CLK_TIME) {
        QueueMouseEvent(LEFT_BUTTON_DBL_CLK);
      } else {
        guiSingleClickTimer = uiTimer;
      }
      break;
    }

    case SDL_BUTTON_RIGHT:
#if defined WITH_MAEMO || defined(__APPLE__)
    right_button:
#endif
      guiRightButtonRepeatTimer = 0;
      gfRightButtonState = FALSE;
      QueueMouseEvent(RIGHT_BUTTON_UP);
      break;
  }
}

void MouseWheelScroll(const SDL_MouseWheelEvent *WheelEv) {
  if (WheelEv->y > 0) {
    QueueMouseEvent(MOUSE_WHEEL_UP);
  } else {
    QueueMouseEvent(MOUSE_WHEEL_DOWN);
  }
}

static void KeyChange(SDL_Keysym const *const key_sym, bool const pressed) {
  SDL_Keycode key = key_sym->sym;
  SDL_Keymod const mod = (SDL_Keymod)key_sym->mod;
  bool const num = mod & KMOD_NUM;
  switch (key) {
#if defined WITH_MAEMO
    /* Use the menu button (mapped to F4) as modifier for right click */
    case SDLK_F4:
      return;
#endif

    case SDLK_KP_0:
      key = num ? SDLK_0 : SDLK_INSERT;
      break;
    case SDLK_KP_1:
      key = num ? SDLK_1 : SDLK_END;
      break;
    case SDLK_KP_2:
      key = num ? SDLK_2 : SDLK_DOWN;
      break;
    case SDLK_KP_3:
      key = num ? SDLK_3 : SDLK_PAGEDOWN;
      break;
    case SDLK_KP_4:
      key = num ? SDLK_4 : SDLK_LEFT;
      break;
    case SDLK_KP_5:
      if (!num) return;
      key = SDLK_5;
      break;
    case SDLK_KP_6:
      key = num ? SDLK_6 : SDLK_RIGHT;
      break;
    case SDLK_KP_7:
      key = num ? SDLK_7 : SDLK_HOME;
      break;
    case SDLK_KP_8:
      key = num ? SDLK_8 : SDLK_UP;
      break;
    case SDLK_KP_9:
      key = num ? SDLK_9 : SDLK_PAGEUP;
      break;
    case SDLK_KP_PERIOD:
      key = num ? SDLK_PERIOD : SDLK_DELETE;
      break;
    case SDLK_KP_DIVIDE:
      key = SDLK_SLASH;
      break;
    case SDLK_KP_MULTIPLY:
      key = SDLK_ASTERISK;
      break;
    case SDLK_KP_MINUS:
      key = SDLK_MINUS;
      break;
    case SDLK_KP_PLUS:
      key = SDLK_PLUS;
      break;
    case SDLK_KP_ENTER:
      key = SDLK_RETURN;
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

void KeyDown(const SDL_Keysym *KeySym) {
  switch (KeySym->sym) {
    case SDLK_LSHIFT:
    case SDLK_RSHIFT:
      pressedKeys[SHIFT] = true;
      break;

    case SDLK_LCTRL:
    case SDLK_RCTRL:
      pressedKeys[CTRL] = true;
      break;

    case SDLK_LALT:
    case SDLK_RALT:
      pressedKeys[ALT] = true;
      break;

    case SDLK_PRINTSCREEN:
    case SDLK_SCROLLLOCK:
      break;

    default:
      KeyChange(KeySym, true);
      break;
  }
}

void KeyUp(const SDL_Keysym *KeySym) {
  switch (KeySym->sym) {
    case SDLK_LSHIFT:
    case SDLK_RSHIFT:
      pressedKeys[SHIFT] = false;
      break;

    case SDLK_LCTRL:
    case SDLK_RCTRL:
      pressedKeys[CTRL] = false;
      break;

    case SDLK_LALT:
    case SDLK_RALT:
      pressedKeys[ALT] = false;
      break;

    case SDLK_PRINTSCREEN:
      if (KeySym->mod & KMOD_CTRL)
        VideoCaptureToggle();
      else
        PrintScreen();
      break;

    case SDLK_SCROLLLOCK:
      SDL_SetWindowGrab(GAME_WINDOW,
                        SDL_GetWindowGrab(GAME_WINDOW) == SDL_FALSE ? SDL_TRUE : SDL_FALSE);
      break;

    case SDLK_RETURN:
      if (IsKeyDown(ALT)) {
        VideoToggleFullScreen();
        break;
      }
      /* FALLTHROUGH */

    default:
      KeyChange(KeySym, false);
      break;
  }
}

void TextInput(const SDL_TextInputEvent *TextEv) {
  UTF8String utf8String = UTF8String(TextEv->text);
  QueueKeyEvent(TEXT_INPUT, SDLK_UNKNOWN, KMOD_NONE, utf8String.getUTF16()[0]);
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
  int windowWidth, windowHeight;
  SDL_GetWindowSize(GAME_WINDOW, &windowWidth, &windowHeight);

  double windowWidthD = windowWidth;
  double windowHeightD = windowHeight;
  double screenWidthD = SCREEN_WIDTH;
  double screenHeightD = SCREEN_HEIGHT;

  double scaleFactorX = windowWidthD / screenWidthD;
  double scaleFactorY = windowHeightD / screenHeightD;
  double scaleFactor = windowWidth > windowHeight ? scaleFactorY : scaleFactorX;

  double scaledWindowWidth = scaleFactor * screenWidthD;
  double scaledWindowHeight = scaleFactor * screenHeightD;

  double paddingX = (windowWidthD - scaledWindowWidth) / 2.0;
  double paddingY = (windowHeight - scaledWindowHeight) / 2.0;
  int windowPositionX = paddingX + (double)uiNewXPos * scaledWindowWidth / screenWidthD;
  int windowPositionY = paddingY + (double)uiNewYPos * scaledWindowHeight / screenHeightD;

  SDL_WarpMouseInWindow(GAME_WINDOW, windowPositionX, windowPositionY);
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
  uint32_t uiTimer = GetClock();

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

BOOLEAN IsKeyDown(int a) { return pressedKeys.count(a) > 0 ? pressedKeys[a] : false; }
