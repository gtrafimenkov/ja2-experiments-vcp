// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#ifndef JA2_INPUT_ATOM_H
#define JA2_INPUT_ATOM_H

#include <stdbool.h>
#include <stdint.h>
#include <wchar.h>

#include "jplatform_input.h"

struct InputAtom {
  bool alt;
  bool shift;
  bool ctrl;
  uint16_t usEvent;
  JInput_VirtualKey key;
  wchar_t Char;

  bool isKeyDown() const;
  JInput_VirtualKey getKey() const;
};

#endif
