#ifndef SGPSTRINGS_H
#define SGPSTRINGS_H

#include <cwchar>
#include <stdio.h>
#include <string.h>

#include "PlatformStrings.h"


#if defined(__linux__) || defined(_WIN32)

size_t wcslcpy(wchar_t* dst, const wchar_t* src, size_t size);
size_t strlcpy(char* dst, const char* src, size_t size);

#endif

#endif
