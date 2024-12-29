#ifndef JA2_JPLATFORM_H
#define JA2_JPLATFORM_H

#include <stdbool.h>
#include <stdint.h>

#include "jplatform_events.h"

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

typedef void(JEventHandler)(enum JEventType eventType, struct JEventData* data);

void JPlatform_MainLoop(JEventHandler* eventHandler);

void JPlatform_Init();
void JPlatform_Exit();

// Call this function if you want to stop the main loop and exit the game.
void JPlatform_RequestExit();

#ifdef __cplusplus
}  // extern "C"
#endif  // __cplusplus

#endif  // JA2_JPLATFORM_H
