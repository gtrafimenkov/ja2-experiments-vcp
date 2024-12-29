#ifndef JA2_SGP_BOX_H
#define JA2_SGP_BOX_H

#include <stdint.h>

struct SGPBox {
  int32_t x;
  int32_t y;
  int32_t w;
  int32_t h;
};

inline void SGPBox_set(struct SGPBox *box, int32_t x, int32_t y, int32_t w, int32_t h) {
  box->x = x;
  box->y = y;
  box->w = w;
  box->h = h;
}

#endif  // JA2_SGP_BOX_H
