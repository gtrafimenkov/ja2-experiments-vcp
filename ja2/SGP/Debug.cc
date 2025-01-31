// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#include "SGP/Debug.h"

#include <SDL.h>
#include <stdarg.h>
#include <stdio.h>

#include "Macro.h"
#include "SGP/Timer.h"

#if defined(SGP_DEBUG) || defined(FORCE_ASSERTS_ON)

static BOOLEAN gfRecordToFile = FALSE;
static BOOLEAN gfRecordToDebugger = TRUE;

static STRING512 gpcDebugLogFileName;

#ifndef _NO_DEBUG_TXT
static BOOLEAN DbgGetLogFileName(STRING512 pcName) {
  strcpy(pcName, "debug.txt");
  return TRUE;
}
#endif

BOOLEAN InitializeDebugManager() {
  gfRecordToFile = TRUE;
  gfRecordToDebugger = TRUE;

#ifndef _NO_DEBUG_TXT
  if (!DbgGetLogFileName(gpcDebugLogFileName)) return FALSE;
  remove(gpcDebugLogFileName);
#endif

  return (TRUE);
}

void DebugMsg(TopicID uiTopicId, DebugLevel uiDebugLevel, const char *strMessage) {
  fprintf(stderr, "%s\n", strMessage);

// add _NO_DEBUG_TXT to your SGP preprocessor definitions to avoid this f**king
// huge file from slowly growing behind the scenes!!!!
#ifndef _NO_DEBUG_TXT
  FILE *OutFile = fopen(gpcDebugLogFileName, "a+");
  if (OutFile != NULL) {
    fprintf(OutFile, "%s\n", strMessage);
    fclose(OutFile);
  }
#endif
}

static void _DebugRecordToFile(BOOLEAN gfState) { gfRecordToFile = gfState; }

static void _DebugRecordToDebugger(BOOLEAN gfState) { gfRecordToDebugger = gfState; }

void _DebugMessage(const char *pString, uint32_t uiLineNum, const char *pSourceFile) {
  char ubOutputString[512];
  sprintf(ubOutputString, "{ %ld } %s [Line %d in %s]\n", GetClock(), pString, uiLineNum,
          pSourceFile);

  if (gfRecordToDebugger) {
    fputs(ubOutputString, stderr);
  }

#ifndef _NO_DEBUG_TXT
  if (gfRecordToFile) {
    FILE *DebugFile = fopen(gpcDebugLogFileName, "a+");
    if (DebugFile != NULL) {
      fputs(ubOutputString, DebugFile);
      fclose(DebugFile);
    }
  }
#endif
}

void _FailMessage(const char *pString, uint32_t uiLineNum, const char *pSourceFile) {
  char ubOutputString[512];
  if (pString != NULL)
    sprintf(ubOutputString, "{ %ld } Assertion Failure [Line %d in %s]: %s\n", GetClock(),
            uiLineNum, pSourceFile, pString);
  else
    sprintf(ubOutputString, "{ %ld } Assertion Failure [Line %d in %s]\n", GetClock(), uiLineNum,
            pSourceFile);

  // Output to debugger
  if (gfRecordToDebugger) fputs(ubOutputString, stderr);

    // Record to file if required
#ifndef _NO_DEBUG_TXT
  if (gfRecordToFile) {
    FILE *DebugFile = fopen(gpcDebugLogFileName, "a+");
    if (DebugFile != NULL) {
      fputs(ubOutputString, DebugFile);
      fclose(DebugFile);
    }
  }
#endif

  SDL_Quit();
  abort();
}

const char *String(const char *const fmt, ...) {
  static char TmpDebugString[8][512];
  static uint32_t StringIndex = 0;

  // Record string index. This index is used since we live in a multitasking
  // environment. It is still not bulletproof, but it's better than a single
  // string
  char *ResultString = TmpDebugString[StringIndex];
  StringIndex = (StringIndex + 1) % lengthof(TmpDebugString);

  va_list ArgPtr;
  va_start(ArgPtr, fmt);
  vsprintf(ResultString, fmt, ArgPtr);
  va_end(ArgPtr);

  return ResultString;
}

#endif
