#pragma once

/* Warning, this file is autogenerated by cbindgen. Don't modify this manually. */

#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

struct SGPRect {
  int32_t iLeft;
  int32_t iTop;
  int32_t iRight;
  int32_t iBottom;
};

struct SGPPoint {
  int32_t iX;
  int32_t iY;
};

struct SGPBox {
  int32_t x;
  int32_t y;
  int32_t w;
  int32_t h;
};

/**
 * Copy of SGPRect from the C codebase.
 */
struct GRect {
  int32_t iLeft;
  int32_t iTop;
  int32_t iRight;
  int32_t iBottom;
};

/**
 * Copy of Rect from the C codebase.
 */
struct Rect {
  int32_t left;
  int32_t top;
  int32_t right;
  int32_t bottom;
};

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

struct SGPRect SGPRect_new(int32_t left, int32_t top, int32_t right, int32_t bottom);

void SGPRect_set(struct SGPRect *r, int32_t left, int32_t top, int32_t right, int32_t bottom);

struct SGPPoint SGPPoint_new(int32_t x, int32_t y);

void SGPPoint_set(struct SGPPoint *r, int32_t x, int32_t y);

void SGPBox_set(struct SGPBox *r, int32_t x, int32_t y, int32_t w, int32_t h);

/**
 * Create new GRect structure
 */
struct GRect NewGRect(int32_t left, int32_t top, int32_t width, int32_t height);

/**
 * Create new Rect structure
 */
struct Rect NewRect(int32_t left, int32_t top, int32_t width, int32_t height);

#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus
