#ifndef JA2_JPLATFORM_VIDEO_H
#define JA2_JPLATFORM_VIDEO_H

// Two types of video surfaces are supported:
// - 8 bits per pixel - paletted image
// - 16 bits per pixel in RGB565 encoding

#include <stdbool.h>
#include <stdint.h>

struct JRect {
  int32_t x;
  int32_t y;
  int32_t w;
  int32_t h;
};

inline void JRect_set(struct JRect *r, int32_t x, int32_t y, int32_t w, int32_t h) {
  r->x = x;
  r->y = y;
  r->w = w;
  r->h = h;
}

// Square collection of pixels
struct JSurface;

// Internal video state
struct JVideoState;

#define RGB565_RED_MASK 0xF800
#define RGB565_GREEN_MASK 0x07E0
#define RGB565_BLUE_MASK 0x001F

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

struct JVideoState *JVideo_Init(const char *title, uint16_t width, uint16_t height);
void JVideo_ToggleFullScreen(struct JVideoState *v);
void JVideo_ToggleMouseGrab(struct JVideoState *v);
void JVideo_HideCursor();

void JVideo_RenderSufaceCopy(struct JVideoState *v, struct JSurface *s, struct JRect *r);
void JVideo_RenderAndPresent(struct JVideoState *v, struct JSurface *s);

void JVideo_SimulateMouseMovement(struct JVideoState *v, uint32_t newPosX, uint32_t newPosY);

// Lock the surface to get access to raw pixels.
bool JSurface_Lock(struct JSurface *s);

// Unlock the previously locked surface.
void JSurface_Unlock(struct JSurface *s);

int JSurface_Pitch(struct JSurface *s);

void *JSurface_GetPixels(struct JSurface *s);

struct JSurface *JSurface_Create8bpp(uint16_t width, uint16_t height);

struct JSurface *JSurface_Create16bpp(uint16_t width, uint16_t height);

uint16_t JSurface_Width(struct JSurface *s);
uint16_t JSurface_Height(struct JSurface *s);
uint8_t JSurface_BPP(struct JSurface *s);

void JSurface_SetColorKey(struct JSurface *s, uint32_t key);
bool JSurface_IsColorKeySet(struct JSurface *s);
void JSurface_Fill(struct JSurface *s, uint16_t colour);
void JSurface_FillRect(struct JSurface *s, struct JRect *rect, uint16_t colour);

void JSurface_Free(struct JSurface *s);

void JSurface_Blit(struct JSurface *src, struct JSurface *dst);
void JSurface_BlitToPoint(struct JSurface *src, struct JSurface *dst, int32_t destX, int32_t destY);
void JSurface_BlitRectToPoint(struct JSurface *src, struct JSurface *dst,
                              struct JRect const *srcBox, int32_t destX, int32_t destY);
void JSurface_BlitRectToRect(struct JSurface *src, struct JSurface *dst, struct JRect const *srcBox,
                             struct JRect const *destBox);

#ifdef __cplusplus
}  // extern "C"
#endif  // __cplusplus

#endif  // JA2_JPLATFORM_VIDEO_H
