#include "jplatform_audio.h"

#include <stdio.h>

#include "SDL_audio.h"

JAudio_DeviceID JAudio_Open(JAudio_Callback callback) {
  SDL_AudioSpec want, have;

  want.freq = 22050;
  want.format = AUDIO_S16SYS;
  want.channels = 2;
  want.samples = 1024;
  want.callback = callback;
  want.userdata = NULL;

  SDL_AudioDeviceID dev = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);

  if (dev == 0) {
    fprintf(stderr, "Failed to open audio device: %s\n", SDL_GetError());
    return false;
  }

  if (have.freq != want.freq || have.format != want.format || have.channels != want.channels) {
    fprintf(stderr, "Warning: Actual audio specifications differ from requested.\n");
    fprintf(stderr, "Requested: freq=%d, format=%d, channels=%d\n", want.freq, want.format,
            want.channels);
    fprintf(stderr, "Actual:   freq=%d, format=%d, channels=%d\n", have.freq, have.format,
            have.channels);
  }

  return dev;
}

void JAudio_PauseAudio(JAudio_DeviceID dev, bool pause_on) {
  SDL_PauseAudioDevice(dev, pause_on ? 1 : 0);
}
