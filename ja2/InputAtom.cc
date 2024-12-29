// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#include "InputAtom.h"

#include "SGP/Input.h"

bool InputAtom::isKeyDown() const { return this->usEvent == KEY_DOWN; }

JInput_VirtualKey InputAtom::getKey() const { return this->key; }
