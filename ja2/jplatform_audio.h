#ifndef JA2_JPLATFORM_AUDIO_H
#define JA2_JPLATFORM_AUDIO_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

typedef uint32_t JAudio_DeviceID;
typedef void (*JAudio_Callback)(void *userdata, uint8_t *stream, int len);

JAudio_DeviceID JAudio_Open(JAudio_Callback callback);
void JAudio_PauseAudio(JAudio_DeviceID dev, bool pause_on);

#ifdef __cplusplus
}  // extern "C"
#endif  // __cplusplus

#endif  // JA2_JPLATFORM_AUDIO_H
