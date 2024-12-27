// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#ifndef __TYPES_
#define __TYPES_

#include <stdint.h>

template <typename T>
static inline void Swap(T &a, T &b) {
  T t(a);
  a = b;
  b = t;
}

typedef unsigned char BOOLEAN;
typedef char STRING512[512];

typedef char SGPFILENAME[100];

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#define BAD_INDEX -1

#define PI 3.1415926

#ifndef NULL
#define NULL 0
#endif

typedef uint32_t COLORVAL;

struct AuxObjectData;
struct ETRLEObject;
struct RelTileLoc;
struct SGPImage;

class SGPVObject;
typedef SGPVObject *HVOBJECT;
typedef SGPVObject *Font;

class SGPVSurface;

struct BUTTON_PICS;

struct SGPFile;
typedef SGPFile *HWFILE;

#define SGP_TRANSPARENT ((uint16_t)0)

#ifdef __cplusplus
#define ENUM_BITSET(type)                                                                \
  static inline type operator~(type a) { return (type) ~(int)a; }                        \
  static inline type operator&(type a, type b) { return (type)((int)a & (int)b); }       \
  static inline type operator&=(type &a, type b) { return a = (type)((int)a & (int)b); } \
  static inline type operator|(type a, type b) { return (type)((int)a | (int)b); }       \
  static inline type operator|=(type &a, type b) { return a = (type)((int)a | (int)b); }
#else
#define ENUM_BITSET(type)
#endif

#endif
