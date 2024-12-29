#ifndef JA2_JPLATFORM_TIME_H
#define JA2_JPLATFORM_TIME_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

uint32_t JTime_GetTicks();
void JTime_Delay(uint32_t ms);

typedef int JTime_TimerID;

typedef uint32_t (*JTime_TimerCallback)(uint32_t interval, void *param);

JTime_TimerID JTime_AddTimer(uint32_t interval, JTime_TimerCallback callback, void *param);
bool JTime_RemoveTimer(JTime_TimerID id);

#ifdef __cplusplus
}  // extern "C"
#endif  // __cplusplus

#endif  // JA2_JPLATFORM_TIME_H
