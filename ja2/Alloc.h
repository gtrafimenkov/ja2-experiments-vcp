#ifndef __ALLOC_H
#define __ALLOC_H

#include <stddef.h>
#include <stdint.h>

void* zmalloc(size_t size);

void* MemAlloc(size_t size);
void MemFree(void* ptr);
void* MemRealloc(void* ptr, size_t size);

#endif
