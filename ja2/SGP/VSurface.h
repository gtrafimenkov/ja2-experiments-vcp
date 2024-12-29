// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#ifndef VSURFACE_H
#define VSURFACE_H

#include "SGP/AutoObj.h"
#include "SGP/AutoPtr.h"
#include "SGP/Buffer.h"
#include "SGP/Types.h"
#include "jplatform_video.h"

#define BACKBUFFER g_back_buffer
#define FRAME_BUFFER g_frame_buffer
#define MOUSE_BUFFER g_mouse_buffer

class SGPVSurface;
class SGPVSurfaceAuto;
class SGPVSurface;

extern SGPVSurface *g_back_buffer;
extern SGPVSurfaceAuto *g_frame_buffer;
extern SGPVSurfaceAuto *g_mouse_buffer;

/** Utility wrapper around struct JSurface. */
class SGPVSurface {
 public:
  SGPVSurface(struct JSurface *);

 protected:
  SGPVSurface(uint16_t w, uint16_t h, uint8_t bpp);

 public:
  virtual ~SGPVSurface();

  uint16_t Width() const;
  uint16_t Height() const;
  uint8_t BPP() const;

  // Set palette, also sets 16BPP palette
  void SetPalette(const struct JColor *src_pal);

  // Get the RGB palette entry values
  struct JColor const *GetPalette() const { return palette_; }

  void SetTransparency(COLORVAL);

  /* Fill an entire surface with a colour */
  void Fill(uint16_t colour);

  void ShadowRect(int32_t x1, int32_t y1, int32_t x2, int32_t y2);
  void ShadowRectUsingLowPercentTable(int32_t x1, int32_t y1, int32_t x2, int32_t y2);

  /* Fills an rectangular area with a specified color value. */
  friend void ColorFillVideoSurfaceArea(SGPVSurface *, int32_t iDestX1, int32_t iDestY1,
                                        int32_t iDestX2, int32_t iDestY2, uint16_t Color16BPP);

  // Blits a video Surface to another video Surface
  friend void BltVideoSurface(SGPVSurface *dst, SGPVSurface *src, int32_t iDestX, int32_t iDestY,
                              SGPBox const *src_rect);

  /* This function will stretch the source image to the size of the dest rect.
   * If the 2 images are not 16 Bpp, it returns false. */
  friend void BltStretchVideoSurface(SGPVSurface *dst, SGPVSurface const *src,
                                     SGPBox const *src_rect, SGPBox const *dst_rect);

 protected:
  struct JSurface *surface_;
  SGP::Buffer<struct JColor> palette_;

 public:
  uint16_t *p16BPPPalette;  // A 16BPP palette used for 8->16 blits
  SGPVSurface *next_;

 private:
  class LockBase {
   public:
    explicit LockBase(struct JSurface *const s) : surface_(s) {}

    template <typename T>
    T *Buffer() {
      return static_cast<T *>(JSurface_GetPixels(surface_));
    }

    uint32_t Pitch() { return JSurface_Pitch(surface_); }

   protected:
    struct JSurface *surface_;
  };

 public:
  class Lock : public LockBase {
   public:
    explicit Lock(SGPVSurface *const vs) : LockBase(vs->surface_) { JSurface_Lock(surface_); }

    ~Lock() { JSurface_Unlock(surface_); }
  };

  class Lockable : public LockBase {
   public:
    explicit Lockable() : LockBase(0) {}

    ~Lockable() {
      if (surface_) JSurface_Unlock(surface_);
    }

    void Lock(SGPVSurface *const vs) {
      if (surface_) JSurface_Unlock(surface_);
      surface_ = vs->surface_;
      if (surface_) JSurface_Lock(surface_);
    }
  };
};

/**
 * Utility wrapper around struct JSurface which automatically
 * frees struct JSurface when the object is destroyed. */
class SGPVSurfaceAuto : public SGPVSurface {
 public:
  SGPVSurfaceAuto(uint16_t w, uint16_t h, uint8_t bpp);
  SGPVSurfaceAuto(struct JSurface *);

  virtual ~SGPVSurfaceAuto();
};

SGPVSurfaceAuto *AddVideoSurface(uint16_t Width, uint16_t Height, uint8_t BitDepth);
SGPVSurfaceAuto *AddVideoSurfaceFromFile(const char *Filename);

/* Blits a video surface in half size to another video surface.
 * If SrcRect is NULL the entire source surface is blitted.
 * Only blitting from 8bbp surfaces to 16bpp surfaces is supported. */
void BltVideoSurfaceHalf(SGPVSurface *dst, SGPVSurface *src, int32_t DestX, int32_t DestY,
                         SGPBox const *src_rect);

// Deletes all data, including palettes
static inline void DeleteVideoSurface(SGPVSurface *const vs) { delete vs; }

void BltVideoSurfaceOnce(SGPVSurface *dst, const char *filename, int32_t x, int32_t y);

/** Draw image on the video surface stretching the image if necessary. */
void BltVideoSurfaceOnceWithStretch(SGPVSurface *const dst, const char *const filename);

/** Fill video surface with another one with stretch. */
void FillVideoSurfaceWithStretch(SGPVSurface *const dst, SGPVSurface *const src);

#endif
