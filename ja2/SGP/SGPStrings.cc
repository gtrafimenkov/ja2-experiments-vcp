// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#include "SGP/SGPStrings.h"

#include <stdio.h>

void ReplacePath(char *const buf, size_t const size, char const *path, char const *const filename,
                 char const *const ext) {
  char const *base = filename;
  char const *old_ext = 0;
  for (char const *i = filename;; ++i) {
    switch (*i) {
      case '.':
        old_ext = i;
        break;

      case '/':
      case '\\':
        base = i + 1;
        old_ext = 0;
        break;

      case '\0':
        if (!path) {
          base = filename;
          path = "";
        }
        int const n = (int)((old_ext ? old_ext : i) - base);
        snprintf(buf, size, "%s%.*s%s", path, n, base, ext);
        return;
    }
  }
}
