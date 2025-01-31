// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#include "TileEngine/PhysMath.h"

#include <math.h>

vector_3 VAdd(vector_3 *a, vector_3 *b) {
  vector_3 c;

  c.x = a->x + b->x;
  c.y = a->y + b->y;
  c.z = a->z + b->z;

  return (c);
}

vector_3 VMultScalar(vector_3 *a, real b) {
  vector_3 c;

  c.x = a->x * b;
  c.y = a->y * b;
  c.z = a->z * b;

  return (c);
}

real VDotProduct(vector_3 *a, vector_3 *b) {
  return ((a->x * b->x) + (a->y * b->y) + (a->z * b->z));
}

vector_3 VGetNormal(vector_3 *a) {
  vector_3 c;
  const float length = VDotProduct(a, a);
  if (length == 0) {
    c.x = 0;
    c.y = 0;
    c.z = 0;
  } else {
    const float OneOverLength = 1 / sqrt(length);
    c.x = OneOverLength * a->x;
    c.y = OneOverLength * a->y;
    c.z = OneOverLength * a->z;
  }
  return (c);
}
