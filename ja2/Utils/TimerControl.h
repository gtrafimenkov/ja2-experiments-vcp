// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#ifndef __TIMER_CONTROL_H
#define __TIMER_CONTROL_H

#include "SGP/Types.h"

typedef void (*CUSTOMIZABLE_TIMER_CALLBACK)();

// TIMER DEFINES
enum {
  TOVERHEAD = 0,              // Overhead time slice
  NEXTSCROLL,                 // Scroll Speed timer
  STARTSCROLL,                // Scroll Start timer
  ANIMATETILES,               // Animate tiles timer
  FPSCOUNTER,                 // FPS value
  PATHFINDCOUNTER,            // PATH FIND COUNTER
  CURSORCOUNTER,              // ANIMATED CURSOR
  RMOUSECLICK_DELAY_COUNTER,  // RIGHT BUTTON CLICK DELAY
  LMOUSECLICK_DELAY_COUNTER,  // LEFT	 BUTTON CLICK DELAY
  TARGETREFINE,               // TARGET REFINE
  CURSORFLASH,                // Cursor/AP flash
  PHYSICSUPDATE,              // PHYSICS UPDATE.
  GLOW_ENEMYS,
  STRATEGIC_OVERHEAD,    // STRATEGIC OVERHEAD
  CYCLERENDERITEMCOLOR,  // CYCLE COLORS
  NONGUNTARGETREFINE,    // TARGET REFINE
  CURSORFLASHUPDATE,     //
  INVALID_AP_HOLD,       // TIME TO HOLD INVALID AP
  RADAR_MAP_BLINK,       // BLINK DELAY FOR RADAR MAP
  MUSICOVERHEAD,         // MUSIC TIMER
  NUMTIMERS
};

// Base resultion of callback timer
#define BASETIMESLICE 10

extern const int32_t giTimerIntervals[NUMTIMERS];
extern int32_t giTimerCounters[NUMTIMERS];

// GLOBAL SYNC TEMP TIME
extern int32_t giClockTimer;

extern int32_t giTimerDiag;

extern int32_t giTimerTeamTurnUpdate;

void InitializeJA2Clock();
void ShutdownJA2Clock();

uint32_t GetJA2Clock();

void PauseTime(BOOLEAN fPaused);

void SetCustomizableTimerCallbackAndDelay(int32_t iDelay, CUSTOMIZABLE_TIMER_CALLBACK pCallback,
                                          BOOLEAN fReplace);
void CheckCustomizableTimer();

// Don't modify this value
extern uint32_t guiBaseJA2Clock;
extern CUSTOMIZABLE_TIMER_CALLBACK gpCustomizableTimerCallback;

#define UPDATECOUNTER(c)                                                \
  ((giTimerCounters[c] - BASETIMESLICE) < 0) ? (giTimerCounters[c] = 0) \
                                             : (giTimerCounters[c] -= BASETIMESLICE)
#define RESETCOUNTER(c) (giTimerCounters[c] = giTimerIntervals[c])
#define COUNTERDONE(c) (giTimerCounters[c] == 0) ? TRUE : FALSE

#define UPDATETIMECOUNTER(c) ((c - BASETIMESLICE) < 0) ? (c = 0) : (c -= BASETIMESLICE)
#define RESETTIMECOUNTER(c, d) (c = d)

#define TIMECOUNTERDONE(c, d) (c == 0)

#define ZEROTIMECOUNTER(c) (c = 0)

/* whenever guiBaseJA2Clock changes, we must reset all the timer variables that
 * use it as a reference */
void ResetJA2ClockGlobalTimers();

#endif
