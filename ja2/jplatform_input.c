#include "jplatform_input.h"

#include "SDL_keyboard.h"

void JInput_StartTextInput() { SDL_StartTextInput(); }
void JInput_StopTextInput() { SDL_StopTextInput(); }
