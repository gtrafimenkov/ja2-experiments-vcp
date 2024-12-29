#include "jplatform_video.h"

#include <string.h>

#include "SDL.h"
#include "SDL_hints.h"
#include "SDL_render.h"
#include "SDL_surface.h"
#include "SDL_video.h"

// JSurface contains only SDL_Surface.  That makes pointers to JSurface and SDL_Surface
// interchangeable.
struct JSurface {
  struct SDL_Surface s;
};

struct JVideoState {
  SDL_Renderer *renderer;
  SDL_Window *window;
  SDL_Texture *ScreenTexture;
  uint16_t initialWindowWidth;
  uint16_t initialWindowHeight;
};

static inline SDL_Surface *sdlsurf(struct JSurface *s) { return (SDL_Surface *)s; }

static void *zmalloc(size_t size) {
  void *p = malloc(size);
  memset(p, 0, size);
  return p;
}

struct JVideoState *JVideo_Init(const char *title, uint16_t width, uint16_t height) {
  struct JVideoState *v = zmalloc(sizeof(struct JVideoState));

  v->initialWindowWidth = width;
  v->initialWindowHeight = height;

  SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "best");
  v->window = SDL_CreateWindow(title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width,
                               height, SDL_WINDOW_RESIZABLE);
  v->renderer = SDL_CreateRenderer(v->window, -1, 0);
  SDL_RenderSetLogicalSize(v->renderer, width, height);

  v->ScreenTexture = SDL_CreateTexture(v->renderer, SDL_PIXELFORMAT_RGB565,
                                       SDL_TEXTUREACCESS_STREAMING, width, height);

  if (v->ScreenTexture == NULL) {
    // SLOGE(TAG, "SDL_CreateTexture for ScreenTexture failed: %s\n",
    // SDL_GetError());
  }

  return v;
}

void JVideo_ToggleFullScreen(struct JVideoState *v) {
  if (SDL_GetWindowFlags(v->window) & SDL_WINDOW_FULLSCREEN) {
    SDL_SetWindowFullscreen(v->window, 0);
  } else {
    SDL_SetWindowFullscreen(v->window, SDL_WINDOW_FULLSCREEN);
  }
}

void JVideo_HideCursor() { SDL_ShowCursor(SDL_DISABLE); }

void JVideo_RenderSufaceCopy(struct JVideoState *v, struct JSurface *s, struct JRect *r) {
  SDL_Texture *t = SDL_CreateTextureFromSurface(v->renderer, sdlsurf(s));
  SDL_Rect rect = {.x = r->x, .y = r->y, .w = r->w, .h = r->h};
  SDL_RenderCopy(v->renderer, t, &rect, &rect);
  SDL_DestroyTexture(t);
}

void JVideo_RenderAndPresent(struct JVideoState *v, struct JSurface *s) {
  SDL_UpdateTexture(v->ScreenTexture, NULL, JSurface_GetPixels(s), JSurface_Pitch(s));
  SDL_RenderClear(v->renderer);
  SDL_RenderCopy(v->renderer, v->ScreenTexture, NULL, NULL);
  SDL_RenderPresent(v->renderer);
}

void JVideo_SimulateMouseMovement(struct JVideoState *v, uint32_t newPosX, uint32_t newPosY) {
  int windowWidth, windowHeight;
  SDL_GetWindowSize(v->window, &windowWidth, &windowHeight);

  double windowWidthD = windowWidth;
  double windowHeightD = windowHeight;
  double screenWidthD = v->initialWindowWidth;
  double screenHeightD = v->initialWindowHeight;

  double scaleFactorX = windowWidthD / screenWidthD;
  double scaleFactorY = windowHeightD / screenHeightD;
  double scaleFactor = windowWidth > windowHeight ? scaleFactorY : scaleFactorX;

  double scaledWindowWidth = scaleFactor * screenWidthD;
  double scaledWindowHeight = scaleFactor * screenHeightD;

  double paddingX = (windowWidthD - scaledWindowWidth) / 2.0;
  double paddingY = (windowHeight - scaledWindowHeight) / 2.0;
  int windowPositionX = paddingX + (double)newPosX * scaledWindowWidth / screenWidthD;
  int windowPositionY = paddingY + (double)newPosY * scaledWindowHeight / screenHeightD;

  SDL_WarpMouseInWindow(v->window, windowPositionX, windowPositionY);
}

void JVideo_ToggleMouseGrab(struct JVideoState *v) {
  SDL_SetWindowGrab(v->window, SDL_GetWindowGrab(v->window) == SDL_FALSE ? SDL_TRUE : SDL_FALSE);
}

bool JSurface_Lock(struct JSurface *s) { return SDL_LockSurface(sdlsurf(s)) == 0; }

void JSurface_Unlock(struct JSurface *s) { SDL_UnlockSurface(sdlsurf(s)); }

int JSurface_Pitch(struct JSurface *s) { return sdlsurf(s)->pitch; }

void *JSurface_GetPixels(struct JSurface *s) { return sdlsurf(s)->pixels; };

struct JSurface *JSurface_Create8bpp(uint16_t width, uint16_t height) {
  return (struct JSurface *)SDL_CreateRGBSurface(0, width, height, 8, 0, 0, 0, 0);
}

#define ALPHA_MASK 0

struct JSurface *JSurface_Create16bpp(uint16_t width, uint16_t height) {
  // SDL_PixelFormat const *f = SDL_AllocFormat(SDL_PIXELFORMAT_RGB565);
  // return (struct JSurface *)SDL_CreateRGBSurface(0, width, height, 16, f->Rmask, f->Gmask,
  // f->Bmask, f->Amask);

  return (struct JSurface *)SDL_CreateRGBSurface(0, width, height, 16, RGB565_RED_MASK,
                                                 RGB565_GREEN_MASK, RGB565_BLUE_MASK, ALPHA_MASK);
}

uint16_t JSurface_Width(struct JSurface *s) { return sdlsurf(s)->w; }
uint16_t JSurface_Height(struct JSurface *s) { return sdlsurf(s)->h; }
uint8_t JSurface_BPP(struct JSurface *s) { return sdlsurf(s)->format->BitsPerPixel; }

void JSurface_SetColorKey(struct JSurface *s, uint32_t key) {
  SDL_SetColorKey(sdlsurf(s), SDL_TRUE, key);
}

bool JSurface_IsColorKeySet(struct JSurface *s) {
  // GT: This condition is very strange.
  // It was used in BltStretchVideoSurface.
  return sdlsurf(s)->flags & SDL_TRUE;
}

void JSurface_Fill(struct JSurface *s, uint16_t colour) { SDL_FillRect(sdlsurf(s), NULL, colour); }
void JSurface_FillRect(struct JSurface *s, struct JRect *rect, uint16_t colour) {
  SDL_Rect r = {.x = rect->x, .y = rect->y, .w = rect->w, .h = rect->h};
  SDL_FillRect(sdlsurf(s), &r, colour);
}

void JSurface_Free(struct JSurface *s) { SDL_FreeSurface(sdlsurf(s)); }

void JSurface_Blit(struct JSurface *src, struct JSurface *dst) {
  SDL_BlitSurface(sdlsurf(src), NULL, sdlsurf(dst), NULL);
}

void JSurface_BlitToPoint(struct JSurface *src, struct JSurface *dst, int32_t destX,
                          int32_t destY) {
  SDL_Rect dstrect;
  dstrect.x = destX;
  dstrect.y = destY;
  SDL_BlitSurface(sdlsurf(src), NULL, sdlsurf(dst), &dstrect);
}

void JSurface_BlitRectToPoint(struct JSurface *src, struct JSurface *dst,
                              struct JRect const *srcBox, int32_t destX, int32_t destY) {
  SDL_Rect srcRect = {.x = srcBox->x, .y = srcBox->y, .w = srcBox->w, .h = srcBox->h};
  SDL_Rect dstRect = {.x = destX, .y = destY, .w = 0, .h = 0};
  SDL_BlitSurface(sdlsurf(src), &srcRect, sdlsurf(dst), &dstRect);
}

void JSurface_BlitRectToRect(struct JSurface *src, struct JSurface *dst, struct JRect const *srcBox,
                             struct JRect const *destBox) {
  SDL_Rect srcRect = {.x = srcBox->x, .y = srcBox->y, .w = srcBox->w, .h = srcBox->h};
  SDL_Rect dstRect = {.x = destBox->x, .y = destBox->y, .w = destBox->w, .h = destBox->h};
  SDL_BlitSurface(sdlsurf(src), &srcRect, sdlsurf(dst), &dstRect);
}

#define SGPGetRValue(rgb) ((uint8_t)(rgb))
#define SGPGetBValue(rgb) ((uint8_t)((rgb) >> 16))
#define SGPGetGValue(rgb) ((uint8_t)(((uint16_t)(rgb)) >> 8))

#define FROMRGB(r, g, b) \
  ((uint32_t)(((uint8_t)(r) | ((uint16_t)(g) << 8)) | (((uint32_t)(uint8_t)(b)) << 16)))

#define BLACK_SUBSTITUTE 0x0001

// Convert from RGB32 to RGB565 16 bit value
uint16_t rgb32_to_rgb565(uint32_t RGBValue) {
  uint16_t r = SGPGetRValue(RGBValue);
  uint16_t g = SGPGetGValue(RGBValue);
  uint16_t b = SGPGetBValue(RGBValue);

  uint16_t usColor =
      ((r << 8) & RGB565_RED_MASK) | ((g << 3) & RGB565_GREEN_MASK) | ((b >> 3) & RGB565_BLUE_MASK);

  // if our color worked out to absolute black, and the original wasn't
  // absolute black, convert it to a VERY dark grey to avoid transparency
  // problems
  if (usColor == 0 && RGBValue != 0) usColor = BLACK_SUBSTITUTE;

  return usColor;
}

// Convert from RGB565 to RGB32
uint32_t rgb565_to_rgb32(uint16_t Value16BPP) {
  uint8_t r = (Value16BPP & RGB565_RED_MASK) >> 8;
  uint8_t g = (Value16BPP & RGB565_GREEN_MASK) >> 3;
  uint8_t b = (Value16BPP & RGB565_BLUE_MASK) << 3;

  uint32_t val = FROMRGB(r, g, b);
  return val;
}
