#ifndef _CHEATS__H_
#define _CHEATS__H_

#include "Types.h"

extern	UINT8			gubCheatLevel;

/** Reset cheat level to initial value. */
extern void resetCheatLevelToInitialValue();

/** Get cheat code. */
extern const char * getCheatCode();

  // ATE: remove cheats unless we're doing a debug build
	#define						INFORMATION_CHEAT_LEVEL( )			( gubCheatLevel >= (isGermanVersion() ? 5 : 3) )
	#define						CHEATER_CHEAT_LEVEL( )					( gubCheatLevel >= (isGermanVersion() ? 6 : 5) )
	#define						DEBUG_CHEAT_LEVEL( )					  ( gubCheatLevel >= (isGermanVersion() ? 7 : 6) )
	#define						RESET_CHEAT_LEVEL( )						( gubCheatLevel = 0 )

#endif
