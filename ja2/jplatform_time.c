#include "jplatform_time.h"

#include "SDL_timer.h"

uint32_t JTime_GetTicks() { return SDL_GetTicks(); }

void JTime_Delay(uint32_t ms) { SDL_Delay(ms); }

JTime_TimerID JTime_AddTimer(uint32_t interval, JTime_TimerCallback callback, void *param) {
  return SDL_AddTimer(interval, callback, param);
}

bool JTime_RemoveTimer(JTime_TimerID id) { return SDL_RemoveTimer(id); }
