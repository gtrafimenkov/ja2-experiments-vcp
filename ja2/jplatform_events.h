#ifndef JA2_JPLATFORM_EVENTS_H
#define JA2_JPLATFORM_EVENTS_H

#include <stdbool.h>
#include <stdint.h>

#include "jplatform_input.h"

struct JEvent_MouseButtonPress {
  bool left;
  bool right;
  int32_t x;
  int32_t y;
};

struct JEvent_MouseMotion {
  int32_t x;
  int32_t y;
};

struct JEvent_MouseWheel {
  int32_t y;
};

struct JEvent_TextInput {
  const char* text;
};

struct JEvent_KeyInput {
  JInput_VirtualKey key;
  struct JInput_KeyMod mod;
};

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

enum JEventType {
  JEVENT_KEYDOWN,
  JEVENT_KEYUP,
  JEVENT_TEXTINPUT,
  JEVENT_MOUSEBUTTONDOWN,
  JEVENT_MOUSEBUTTONUP,
  JEVENT_MOUSEMOTION,
  JEVENT_MOUSEWHEEL,
  JEVENT_QUIT,
  JEVENT_NOTHING,
};

struct JEventData {
  struct JEvent_MouseButtonPress mouseButtonPress;
  struct JEvent_MouseMotion mouseMotion;
  struct JEvent_MouseWheel mouseWheel;
  struct JEvent_TextInput textInput;
  struct JEvent_KeyInput keyInput;
};

#ifdef __cplusplus
}  // extern "C"
#endif  // __cplusplus

#endif  // JA2_JPLATFORM_EVENTS_H
