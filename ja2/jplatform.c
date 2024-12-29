#include "jplatform.h"

#include "SDL.h"
#include "SDL_keycode.h"

void JPlatform_Init() { SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_AUDIO); }

void JPlatform_Exit() { SDL_Quit(); }

void JPlatform_RequestExit() {
  SDL_Event event;
  event.type = SDL_QUIT;
  SDL_PushEvent(&event);
}

// Convert SDL2 virtual key (SDL_Keycode.sym) into JInput virtual key.
static JInput_VirtualKey convertSDLKey(SDL_Keycode sdl_keycode) {
  if (sdl_keycode < 0) {
    return JIK_UNKNOWN;
  }

  switch (sdl_keycode) {
    case SDLK_RETURN:
      return JIK_RETURN;
    case SDLK_ESCAPE:
      return JIK_ESCAPE;
    case SDLK_BACKSPACE:
      return JIK_BACKSPACE;
    case SDLK_TAB:
      return JIK_TAB;

    case SDLK_DELETE:
      return JIK_DELETE;
    case SDLK_DOWN:
      return JIK_DOWN;
    case SDLK_END:
      return JIK_END;
    case SDLK_F1:
      return JIK_F1;
    case SDLK_F10:
      return JIK_F10;
    case SDLK_F11:
      return JIK_F11;
    case SDLK_F12:
      return JIK_F12;
    case SDLK_F2:
      return JIK_F2;
    case SDLK_F3:
      return JIK_F3;
    case SDLK_F4:
      return JIK_F4;
    case SDLK_F5:
      return JIK_F5;
    case SDLK_F6:
      return JIK_F6;
    case SDLK_F7:
      return JIK_F7;
    case SDLK_F8:
      return JIK_F8;
    case SDLK_F9:
      return JIK_F9;
    case SDLK_HOME:
      return JIK_HOME;
    case SDLK_INSERT:
      return JIK_INSERT;
    case SDLK_KP_0:
      return JIK_KP_0;
    case SDLK_KP_1:
      return JIK_KP_1;
    case SDLK_KP_2:
      return JIK_KP_2;
    case SDLK_KP_3:
      return JIK_KP_3;
    case SDLK_KP_4:
      return JIK_KP_4;
    case SDLK_KP_5:
      return JIK_KP_5;
    case SDLK_KP_6:
      return JIK_KP_6;
    case SDLK_KP_7:
      return JIK_KP_7;
    case SDLK_KP_8:
      return JIK_KP_8;
    case SDLK_KP_9:
      return JIK_KP_9;
    case SDLK_KP_DIVIDE:
      return JIK_KP_DIVIDE;
    case SDLK_KP_ENTER:
      return JIK_KP_ENTER;
    case SDLK_KP_MINUS:
      return JIK_KP_MINUS;
    case SDLK_KP_MULTIPLY:
      return JIK_KP_MULTIPLY;
    case SDLK_KP_PERIOD:
      return JIK_KP_PERIOD;
    case SDLK_KP_PLUS:
      return JIK_KP_PLUS;
    case SDLK_LALT:
      return JIK_LALT;
    case SDLK_LCTRL:
      return JIK_LCTRL;
    case SDLK_LEFT:
      return JIK_LEFT;
    case SDLK_LSHIFT:
      return JIK_LSHIFT;
    case SDLK_PAGEDOWN:
      return JIK_PAGEDOWN;
    case SDLK_PAGEUP:
      return JIK_PAGEUP;
    case SDLK_PAUSE:
      return JIK_PAUSE;
    case SDLK_PRINTSCREEN:
      return JIK_PRINTSCREEN;
    case SDLK_RALT:
      return JIK_RALT;
    case SDLK_RCTRL:
      return JIK_RCTRL;
    case SDLK_RIGHT:
      return JIK_RIGHT;
    case SDLK_RSHIFT:
      return JIK_RSHIFT;
    case SDLK_SCROLLLOCK:
      return JIK_SCROLLLOCK;
    case SDLK_UP:
      return JIK_UP;
  }

  // if outside the valid unicode range and not the special keys
  if (sdl_keycode > 0x10FFFF) {
    return JIK_UNKNOWN;
  }

  // normal unicode codepoint
  return sdl_keycode;
}

void JPlatform_MainLoop(JEventHandler* eventHandler) {
  bool s_doGameCycles = true;

  while (true) {
    // cycle until SDL_Quit is received

    SDL_Event event;
    if (SDL_PollEvent(&event)) {
      switch (event.type) {
        case SDL_APP_WILLENTERBACKGROUND:
          s_doGameCycles = false;
          break;

        case SDL_APP_WILLENTERFOREGROUND:
          s_doGameCycles = true;
          break;

        case SDL_KEYDOWN: {
          struct JInput_KeyMod mod;
          mod.num = event.key.keysym.mod & KMOD_NUM;
          mod.shift = event.key.keysym.mod & KMOD_SHIFT;
          mod.ctrl = event.key.keysym.mod & KMOD_CTRL;
          mod.alt = event.key.keysym.mod & KMOD_ALT;

          struct JEvent_KeyInput data;
          data.key = convertSDLKey(event.key.keysym.sym);
          data.mod = mod;

          struct JEventData e;
          e.keyInput = data;

          eventHandler(JEVENT_KEYDOWN, &e);
        } break;
        case SDL_KEYUP: {
          struct JInput_KeyMod mod;
          mod.num = event.key.keysym.mod & KMOD_NUM;
          mod.shift = event.key.keysym.mod & KMOD_SHIFT;
          mod.ctrl = event.key.keysym.mod & KMOD_CTRL;
          mod.alt = event.key.keysym.mod & KMOD_ALT;

          struct JEvent_KeyInput data;
          data.key = convertSDLKey(event.key.keysym.sym);
          data.mod = mod;

          struct JEventData e;
          e.keyInput = data;

          eventHandler(JEVENT_KEYUP, &e);
        } break;
        case SDL_TEXTINPUT: {
          struct JEvent_TextInput data;
          data.text = event.text.text;
          struct JEventData e;
          e.textInput = data;
          eventHandler(JEVENT_TEXTINPUT, &e);
        } break;

        case SDL_MOUSEBUTTONDOWN: {
          struct JEvent_MouseButtonPress data;
          data.left = event.button.button == SDL_BUTTON_LEFT;
          data.right = event.button.button == SDL_BUTTON_RIGHT;
          data.x = event.button.x;
          data.y = event.button.y;
          struct JEventData e;
          e.mouseButtonPress = data;
          eventHandler(JEVENT_MOUSEBUTTONDOWN, &e);
        } break;

        case SDL_MOUSEBUTTONUP: {
          struct JEvent_MouseButtonPress data;
          data.left = event.button.button == SDL_BUTTON_LEFT;
          data.right = event.button.button == SDL_BUTTON_RIGHT;
          data.x = event.button.x;
          data.y = event.button.y;

          struct JEventData e;
          e.mouseButtonPress = data;
          eventHandler(JEVENT_MOUSEBUTTONUP, &e);
        } break;

        case SDL_MOUSEMOTION: {
          struct JEvent_MouseMotion data;
          data.x = event.motion.x;
          data.y = event.motion.y;

          struct JEventData e;
          e.mouseMotion = data;
          eventHandler(JEVENT_MOUSEMOTION, &e);
        } break;

        case SDL_MOUSEWHEEL: {
          struct JEvent_MouseWheel data;
          data.y = event.wheel.y;

          struct JEventData e;
          e.mouseWheel = data;

          eventHandler(JEVENT_MOUSEWHEEL, &e);
        } break;

        case SDL_QUIT: {
          struct JEventData e;
          eventHandler(JEVENT_QUIT, &e);
        } break;
      }
    } else {
      if (s_doGameCycles) {
        struct JEventData e;
        eventHandler(JEVENT_NOTHING, &e);
      } else {
        SDL_WaitEvent(NULL);
      }
    }
  }
}
