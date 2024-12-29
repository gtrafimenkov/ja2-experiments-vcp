#ifndef JA2_JPLATFORM_INPUT_H
#define JA2_JPLATFORM_INPUT_H

#include <stdbool.h>
#include <stdint.h>

struct JInput_KeyMod {
  bool num;
  bool shift;
  bool ctrl;
  bool alt;
};

// Unicode codepoint or one of the values of JInput_SpecialVirtualKey
typedef uint32_t JInput_VirtualKey;

// Special virtual keys
enum JInput_SpecialVirtualKey {
  JIK_UNKNOWN = 0,

  JIK_RETURN = '\r',
  JIK_ESCAPE = '\x1B',
  JIK_BACKSPACE = '\b',
  JIK_TAB = '\t',
  JIK_SPACE = ' ',

  JIK__SPECIAL_START = 0xF0000,  // 0xF0000-0xFFFFF - private use area of unicode codepoints
  JIK_DELETE,
  JIK_DOWN,
  JIK_END,
  JIK_F1,
  JIK_F2,
  JIK_F3,
  JIK_F4,
  JIK_F5,
  JIK_F6,
  JIK_F7,
  JIK_F8,
  JIK_F9,
  JIK_F10,
  JIK_F11,
  JIK_F12,
  JIK_HOME,
  JIK_INSERT,
  JIK_KP_0,
  JIK_KP_1,
  JIK_KP_2,
  JIK_KP_3,
  JIK_KP_4,
  JIK_KP_5,
  JIK_KP_6,
  JIK_KP_7,
  JIK_KP_8,
  JIK_KP_9,
  JIK_KP_DIVIDE,
  JIK_KP_ENTER,
  JIK_KP_MINUS,
  JIK_KP_MULTIPLY,
  JIK_KP_PERIOD,
  JIK_KP_PLUS,
  JIK_LALT,
  JIK_LCTRL,
  JIK_LEFT,
  JIK_LSHIFT,
  JIK_PAGEDOWN,
  JIK_PAGEUP,
  JIK_PAUSE,
  JIK_PRINTSCREEN,
  JIK_RALT,
  JIK_RCTRL,
  JIK_RIGHT,
  JIK_RSHIFT,
  JIK_SCROLLLOCK,
  JIK_UP,
};

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

void JInput_StartTextInput();
void JInput_StopTextInput();

#ifdef __cplusplus
}  // extern "C"
#endif  // __cplusplus

#endif  // JA2_JPLATFORM_INPUT_H
