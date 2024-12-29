#include "Alloc.h"

#include <malloc.h>
#include <string.h>

void* zmalloc(size_t size) {
  void* p = malloc(size);
  memset(p, 0, size);
  return p;
}

void* MemAlloc(size_t size) { return malloc(size); }
void MemFree(void* ptr) { free(ptr); }
void* MemRealloc(void* ptr, size_t size) { return realloc(ptr, size); }
