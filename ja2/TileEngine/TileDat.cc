// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#include "TileEngine/TileDat.h"

#include "Macro.h"
#include "TileEngine/TileDef.h"
#include "TileEngine/WorldDef.h"

int16_t const gOpenDoorList[] = {
    FIRSTDOOR1,  SECONDDOOR1,  THIRDDOOR1,  FOURTHDOOR1,  FIRSTDOOR6,  SECONDDOOR6,
    THIRDDOOR6,  FOURTHDOOR6,  FIRSTDOOR11, SECONDDOOR11, THIRDDOOR11, FOURTHDOOR11,
    FIRSTDOOR16, SECONDDOOR16, THIRDDOOR16, FOURTHDOOR16, -1};

static int16_t const gOpenDoorShadowList[] = {FIRSTDOORSH1,
                                              SECONDDOORSH1,
                                              THIRDDOORSH1,
                                              FOURTHDOORSH1,
                                              FIRSTDOORSH6,
                                              SECONDDOORSH6,
                                              THIRDDOORSH6,
                                              FOURTHDOORSH6,
                                              FIRSTDOORSH11,
                                              SECONDDOORSH11,
                                              THIRDDOORSH11,
                                              FOURTHDOORSH11,
                                              FIRSTDOORSH16,
                                              SECONDDOORSH16,
                                              THIRDDOORSH16,
                                              FOURTHDOORSH16,
                                              -1};

int16_t const gClosedDoorList[] = {
    FIRSTDOOR5,  SECONDDOOR5,  THIRDDOOR5,  FOURTHDOOR5,  FIRSTDOOR10, SECONDDOOR10,
    THIRDDOOR10, FOURTHDOOR10, FIRSTDOOR15, SECONDDOOR15, THIRDDOOR15, FOURTHDOOR15,
    FIRSTDOOR20, SECONDDOOR20, THIRDDOOR20, FOURTHDOOR20, -1};

static int16_t const gClosedDoorShadowList[] = {FIRSTDOORSH5,
                                                SECONDDOORSH5,
                                                THIRDDOORSH5,
                                                FOURTHDOORSH5,
                                                FIRSTDOORSH10,
                                                SECONDDOORSH10,
                                                THIRDDOORSH10,
                                                FOURTHDOORSH10,
                                                FIRSTDOORSH15,
                                                SECONDDOORSH15,
                                                THIRDDOORSH15,
                                                FOURTHDOORSH15,
                                                FIRSTDOORSH20,
                                                SECONDDOORSH20,
                                                THIRDDOORSH20,
                                                FOURTHDOORSH20,
                                                -1};

struct ShadowBuddies {
  int16_t structure;
  int16_t shadow;
  int16_t first_structure;
  int16_t first_shadow;
};

// Bidirectional mapping between structs and shadows
static ShadowBuddies const g_shadow_buddies[] = {
    {FIRSTCLIFF, FIRSTCLIFFSHADOW, FIRSTCLIFF1, FIRSTCLIFFSHADOW1},

    {FIRSTOSTRUCT, FIRSTSHADOW, FIRSTOSTRUCT1, FIRSTSHADOW1},
    {SECONDOSTRUCT, SECONDSHADOW, SECONDOSTRUCT1, SECONDSHADOW1},
    {THIRDOSTRUCT, THIRDSHADOW, THIRDOSTRUCT1, THIRDSHADOW1},
    {FOURTHOSTRUCT, FOURTHSHADOW, FOURTHOSTRUCT1, FOURTHSHADOW1},
    {FIFTHOSTRUCT, FIFTHSHADOW, FIFTHOSTRUCT1, FIFTHSHADOW1},
    {SIXTHOSTRUCT, SIXTHSHADOW, SIXTHOSTRUCT1, SIXTHSHADOW1},
    {SEVENTHOSTRUCT, SEVENTHSHADOW, SEVENTHOSTRUCT1, SEVENTHSHADOW1},
    {EIGHTOSTRUCT, EIGHTSHADOW, EIGHTOSTRUCT1, EIGHTSHADOW1},

    {FIRSTFULLSTRUCT, FIRSTFULLSHADOW, FIRSTFULLSTRUCT1, FIRSTFULLSHADOW1},
    {SECONDFULLSTRUCT, SECONDFULLSHADOW, SECONDFULLSTRUCT1, SECONDFULLSHADOW1},
    {THIRDFULLSTRUCT, THIRDFULLSHADOW, THIRDFULLSTRUCT1, THIRDFULLSHADOW1},
    {FOURTHFULLSTRUCT, FOURTHFULLSHADOW, FOURTHFULLSTRUCT1, FOURTHFULLSHADOW1},

    {FIRSTDOOR, FIRSTDOORSHADOW, FIRSTDOOR1, FIRSTDOORSH1},
    {SECONDDOOR, SECONDDOORSHADOW, SECONDDOOR1, SECONDDOORSH1},
    {THIRDDOOR, THIRDDOORSHADOW, THIRDDOOR1, THIRDDOORSH1},
    {FOURTHDOOR, FOURTHDOORSHADOW, FOURTHDOOR1, FOURTHDOORSH1},

    // Fence
    {FENCESTRUCT, FENCESHADOW, FENCESTRUCT1, FENCESHADOW1},

    // Vehicles
    {FIRSTVEHICLE, FIRSTVEHICLESHADOW, FIRSTVEHICLE1, FIRSTVEHICLESHADOW1},
    {SECONDVEHICLE, SECONDVEHICLESHADOW, SECONDVEHICLE1, SECONDVEHICLESHADOW1},

    // DebrisSTRUCT
    {FIRSTDEBRISSTRUCT, FIRSTDEBRISSTRUCTSHADOW, FIRSTDEBRISSTRUCT1, FIRSTDEBRISSTRUCTSHADOW1},
    {SECONDDEBRISSTRUCT, SECONDDEBRISSTRUCTSHADOW, SECONDDEBRISSTRUCT1, SECONDDEBRISSTRUCTSHADOW1},

    {NINTHOSTRUCT, NINTHOSTRUCTSHADOW, NINTHOSTRUCT1, NINTHOSTRUCTSHADOW1},
    {TENTHOSTRUCT, TENTHOSTRUCTSHADOW, TENTHOSTRUCT1, TENTHOSTRUCTSHADOW1},

    {FIRSTLARGEEXPDEBRIS, FIRSTLARGEEXPDEBRISSHADOW, FIRSTLARGEEXPDEBRIS1,
     FIRSTLARGEEXPDEBRISSHADOW1},
    {SECONDLARGEEXPDEBRIS, SECONDLARGEEXPDEBRISSHADOW, SECONDLARGEEXPDEBRIS1,
     SECONDLARGEEXPDEBRISSHADOW1},
};

// Global variable used to initialize tile database with full tile spec
static uint8_t const gFullBaseTileValues[] = {
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  // First Texture
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  // Second Texture
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  // Third Texture
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  // Forth Texture
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  // Fifth Texture
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,

    1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  // Sixth Texture
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  // Seventh Texture
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,

    1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  // Water1 Texture
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,

    1, 1, 1, 1, 1, 1, 1, 1, 1, 1  // Water2 Texture
};

char const *const gTileSurfaceName[NUMBEROFTILETYPES] = {
    "TEXTURE1", "TEXTURE2", "TEXTURE3", "TEXTURE4", "TEXTURE5", "TEXTURE6", "TEXTURE7", "WATER1",
    "DEEP WATER", "FIRSTCLIFFHANG", "FIRSTCLIFF", "FIRSTCLIFFSHADOW", "OSTRUCT1", "OSTRUCT2",
    "OSTRUCT3", "OSTRUCT4", "OSTRUCT5", "OSTRUCT6", "OSTRUCT7", "OSTRUCT8", "OFSTRUCT1",
    "OFSTRUCT2", "PLACEHOLDER1", "PLACEHOLDER2",

    "SHADOW1", "SHADOW2", "SHADOW3", "SHADOW4", "SHADOW5", "SHADOW6", "SHADOW7", "SHADOW8",
    "FSHADOW1", "FSHADOW2", "PLACEHOLDER3", "PLACEHOLDER4",

    "WALL1", "WALL2", "WALL3", "WALL4", "DOOR1", "DOOR2", "DOOR3", "DOOR4", "DOORSH1", "DOORSH2",
    "DOORSH3", "DOORSH4", "SLANTFLATPEICE", "ANOTHERDEBRIS", "ROADPIECES", "WINDOW4",
    "DECORATIONS1", "DECORATIONS2", "DECORATIONS3", "DECORATIONS4", "WALLDECAL1", "WALLDECAL2",
    "WALLDECAL3", "WALLDECAL4", "FLOOR1", "FLOOR2", "FLOOR3", "FLOOR4", "ROOF1", "ROOF2", "ROOF3",
    "ROOF4", "SROOF1", "SROOF2", "ONROOF1", "ONROOF2", "MOCKF1",

    "ISTRUCT1", "ISTRUCT2", "ISTRUCT3", "ISTRUCT4",

    "FIRSTCISTRCUT",

    "FIRSTROAD",

    "ROCKS", "WOOD", "WEEDS", "GRASS", "SAND", "MISC",

    "ANIOSTRUCT", "FENCESTRUCT", "FENCESHADOW",

    "FIRSTVEHICLE", "SECONDVEHICLE", "FIRSTVEHICLESHADOW", "SECONDVEHICLESHADOW", "MISC2",
    "FIRSTDEBRISSTRUCT", "SECONDDEBRISSTRUCT", "FIRSTDEBRISSTRUCTSHADOW",
    "SECONDDEBRISSTRUCTSHADOW", "NINTHOSTRUCT", "TENTHOSTRUCT", "NINTHOSTRUCTSHADOW",
    "TENTHOSTRUCTSHADOW",

    "FIRSTEXPLODEDEBRIS", "SECONDEXPLODEDEBRIS", "FIRSTLARGEEXPLODEDEBRIS",
    "SECONDLARGEEXPLODEDEBRIS", "FIRSTLARGEEXPLODEDEBRISSHADOW", "SECONDLARGEEXPLODEDEBRISSHADOW",

    "FIFTHISTRUCT", "SIXTHISTRUCT", "SEVENTHISTRUCT", "EIGHTISTRUCT",

    "FIRSTHIGHROOF", "SECONDHIGHROOF",

    "WALLDECAL5", "WALLDECAL6", "WALLDECAL7", "WALLDECAL8",

    "HUMANBLOOD", "CREATUREBLOOD", "FIRSTSWITCHES",

    // ABSOLUTELY NO STUFF PAST HERE!
    // CAN BE SAVED IN THE MAP DIRECTLY!
    "REVEALEDSLANTROOF", "1stREVEALEDHIGHROOF", "2ndREVEALEDHIGHROOF",

    "GUNS", "ITEMS", "ITEMS2",

    "GLASSSHATTER", "ITEMS3", "BODYBLOW",

    "EXITTEXTURE", "FOOTPRINTS", "POINTERS", "POINTERS2", "POINTERS3",

    "GOODRUN", "GOODWALK", "GOODSWAT", "GOODSCOOT", "CONFIRMMOVE", "VEHICLEMOVE", "ACTIONTWO",
    "BADMARKER", "GRING", "ROTATINGKEY", "SELRING", "SPECIAL", "BULLET", "MISS1", "MISS2", "MISS3",
    "WIREFRAME"};

uint16_t const gNumTilesPerType[NUMBEROFTILETYPES] = {
    FIRSTTEXTURE35 - FIRSTTEXTURE1 + 1, SECONDTEXTURE35 - SECONDTEXTURE1 + 1,
    THIRDTEXTURE35 - THIRDTEXTURE1 + 1, FOURTHTEXTURE35 - FOURTHTEXTURE1 + 1,
    FIFTHTEXTURE35 - FIFTHTEXTURE1 + 1, SIXTHTEXTURE37 - SIXTHTEXTURE1 + 1,
    SEVENTHTEXTURE49 - SEVENTHTEXTURE1 + 1, REGWATERTEXTURE50 - REGWATERTEXTURE1 + 1,
    DEEPWATERTEXTURE37 - DEEPWATERTEXTURE1 + 1, FIRSTCLIFFHANG17 - FIRSTCLIFFHANG1 + 1,
    FIRSTCLIFF17 - FIRSTCLIFF1 + 1,
    FIRSTCLIFFSHADOW17 - FIRSTCLIFFSHADOW1 + 1,  // Med hill

    FIRSTOSTRUCT12 - FIRSTOSTRUCT1 + 1, SECONDOSTRUCT12 - SECONDOSTRUCT1 + 1,
    THIRDOSTRUCT12 - THIRDOSTRUCT1 + 1,
    FOURTHOSTRUCT12 - FOURTHOSTRUCT1 + 1,    // Fourth OSTRUCT
    FIFTHOSTRUCT12 - FIFTHOSTRUCT1 + 1,      // Fifth OSTRUCT
    SIXTHOSTRUCT12 - SIXTHOSTRUCT1 + 1,      // Sixth OSTRUCT
    SEVENTHOSTRUCT12 - SEVENTHOSTRUCT1 + 1,  // Seventh OSTRUCT
    EIGHTOSTRUCT12 - EIGHTOSTRUCT1 + 1,      // Eigth OSTRUCT
    FIRSTFULLSTRUCT12 - FIRSTFULLSTRUCT1 + 1, SECONDFULLSTRUCT12 - SECONDFULLSTRUCT1 + 1,
    THIRDFULLSTRUCT2 - THIRDFULLSTRUCT1 + 1, FOURTHFULLSTRUCT2 - FOURTHFULLSTRUCT1 + 1,

    FIRSTSHADOW12 - FIRSTSHADOW1 + 1, SECONDSHADOW12 - SECONDSHADOW1 + 1,
    THIRDSHADOW12 - THIRDSHADOW1 + 1, FOURTHSHADOW12 - FOURTHSHADOW1 + 1,
    FIFTHSHADOW12 - FIFTHSHADOW1 + 1, SIXTHSHADOW12 - SIXTHSHADOW1 + 1,
    SEVENTHSHADOW12 - SEVENTHSHADOW1 + 1, EIGHTSHADOW12 - EIGHTSHADOW1 + 1,
    FIRSTFULLSHADOW12 - FIRSTFULLSHADOW1 + 1, SECONDFULLSHADOW12 - SECONDFULLSHADOW1 + 1,
    THIRDFULLSHADOW2 - THIRDFULLSHADOW1 + 1, FOURTHFULLSHADOW2 - FOURTHFULLSHADOW1 + 1,

    FIRSTWALL65 - FIRSTWALL1 + 1, SECONDWALL65 - SECONDWALL1 + 1, THIRDWALL65 - THIRDWALL1 + 1,
    FOURTHWALL65 - FOURTHWALL1 + 1,

    FIRSTDOOR20 - FIRSTDOOR1 + 1, SECONDDOOR20 - SECONDDOOR1 + 1, THIRDDOOR20 - THIRDDOOR1 + 1,
    FOURTHDOOR20 - FOURTHDOOR1 + 1,

    FIRSTDOORSH20 - FIRSTDOORSH1 + 1, SECONDDOORSH20 - SECONDDOORSH1 + 1,
    THIRDDOORSH20 - THIRDDOORSH1 + 1, FOURTHDOORSH20 - FOURTHDOORSH1 + 1,

    SLANTROOFCEILING2 - SLANTROOFCEILING1 + 1, ANOTHERDEBRIS10 - ANOTHERDEBRIS1 + 1,
    ROADPIECES400 - ROADPIECES001 + 1, FOURTHWINDOW2 - FOURTHWINDOW1 + 1,

    FIRSTDECORATIONS10 - FIRSTDECORATIONS1 + 1, SECONDDECORATIONS10 - SECONDDECORATIONS1 + 1,
    THIRDDECORATIONS10 - THIRDDECORATIONS1 + 1, FOURTHDECORATIONS10 - FOURTHDECORATIONS1 + 1,

    FIRSTWALLDECAL10 - FIRSTWALLDECAL1 + 1, SECONDWALLDECAL10 - SECONDWALLDECAL1 + 1,
    THIRDWALLDECAL10 - THIRDWALLDECAL1 + 1, FOURTHWALLDECAL10 - FOURTHWALLDECAL1 + 1,

    FIRSTFLOOR8 - FIRSTFLOOR1 + 1, SECONDFLOOR8 - SECONDFLOOR1 + 1, THIRDFLOOR8 - THIRDFLOOR1 + 1,
    FOURTHFLOOR8 - FOURTHFLOOR1 + 1,

    FIRSTROOF14 - FIRSTROOF1 + 1, SECONDROOF14 - SECONDROOF1 + 1, THIRDROOF14 - THIRDROOF1 + 1,
    FOURTHROOF14 - FOURTHROOF1 + 1, FIRSTSLANTROOF20 - FIRSTSLANTROOF1 + 1,
    SECONDSLANTROOF20 - SECONDSLANTROOF1 + 1,

    FIRSTONROOF12 - FIRSTONROOF1 + 1, SECONDONROOF12 - SECONDONROOF1 + 1,

    1,

    FIRSTISTRUCT24 - FIRSTISTRUCT1 + 1, SECONDISTRUCT24 - SECONDISTRUCT1 + 1,
    THIRDISTRUCT24 - THIRDISTRUCT1 + 1, FOURTHISTRUCT24 - FOURTHISTRUCT1 + 1,
    FIRSTCISTRUCT24 - FIRSTCISTRUCT1 + 1,

    FIRSTROAD35 - FIRSTROAD1 + 1,

    DEBRISROCKS10 - DEBRISROCKS1 + 1, DEBRISWOOD10 - DEBRISWOOD1 + 1,
    DEBRISWEEDS10 - DEBRISWEEDS1 + 1, DEBRISGRASS10 - DEBRISGRASS1 + 1,
    DEBRISSAND10 - DEBRISSAND1 + 1, DEBRISMISC10 - DEBRISMISC1 + 1,

    ANIOSTRUCT20 - ANIOSTRUCT1 + 1, FENCESTRUCT23 - FENCESTRUCT1 + 1,
    FENCESHADOW23 - FENCESHADOW1 + 1, FIRSTVEHICLE12 - FIRSTVEHICLE1 + 1,
    SECONDVEHICLE12 - SECONDVEHICLE1 + 1, FIRSTVEHICLESHADOW12 - FIRSTVEHICLESHADOW1 + 1,
    SECONDVEHICLESHADOW12 - SECONDVEHICLESHADOW1 + 1, DEBRIS2MISC10 - DEBRIS2MISC1 + 1,

    FIRSTDEBRISSTRUCT10 - FIRSTDEBRISSTRUCT1 + 1, SECONDDEBRISSTRUCT10 - SECONDDEBRISSTRUCT1 + 1,
    FIRSTDEBRISSTRUCTSHADOW10 - FIRSTDEBRISSTRUCTSHADOW1 + 1,
    SECONDDEBRISSTRUCTSHADOW10 - SECONDDEBRISSTRUCTSHADOW1 + 1,

    NINTHOSTRUCT12 - NINTHOSTRUCT1 + 1, TENTHOSTRUCT12 - TENTHOSTRUCT1 + 1,
    NINTHOSTRUCTSHADOW12 - NINTHOSTRUCTSHADOW1 + 1, TENTHOSTRUCTSHADOW12 - TENTHOSTRUCTSHADOW1 + 1,

    FIRSTEXPLDEBRIS40 - FIRSTEXPLDEBRIS1 + 1, SECONDEXPLDEBRIS40 - SECONDEXPLDEBRIS1 + 1,
    FIRSTLARGEEXPDEBRIS10 - FIRSTLARGEEXPDEBRIS1 + 1,
    SECONDLARGEEXPDEBRIS10 - SECONDLARGEEXPDEBRIS1 + 1,
    FIRSTLARGEEXPDEBRISSHADOW10 - FIRSTLARGEEXPDEBRISSHADOW1 + 1,
    SECONDLARGEEXPDEBRISSHADOW10 - SECONDLARGEEXPDEBRISSHADOW1 + 1,

    FIFTHISTRUCT24 - FIFTHISTRUCT1 + 1, SIXTHISTRUCT24 - SIXTHISTRUCT1 + 1,
    SEVENTHISTRUCT24 - SEVENTHISTRUCT1 + 1, EIGHTISTRUCT24 - EIGHTISTRUCT1 + 1,

    FIRSTHIGHROOF15 - FIRSTHIGHROOF1 + 1, SECONDHIGHROOF15 - SECONDHIGHROOF1 + 1,

    FIFTHWALLDECAL10 - FIFTHWALLDECAL1 + 1, SIXTHWALLDECAL10 - SIXTHWALLDECAL1 + 1,
    SEVENTHWALLDECAL10 - SEVENTHWALLDECAL1 + 1, EIGTHWALLDECAL10 - EIGTHWALLDECAL1 + 1,

    HUMANBLOOD16 - HUMANBLOOD1 + 1, CREATUREBLOOD16 - CREATUREBLOOD1 + 1,
    FIRSTSWITCHES21 - FIRSTSWITCHES1 + 1,

    // NO SAVED STUFF AFTER HERE!
    REVEALEDSLANTROOFS8 - REVEALEDSLANTROOFS1 + 1,
    FIRSTREVEALEDHIGHROOFS11 - FIRSTREVEALEDHIGHROOFS1 + 1,
    SECONDREVEALEDHIGHROOFS11 - SECONDREVEALEDHIGHROOFS1 + 1,

    GUN60 - GUN1 + 1, P1ITEM149 - P1ITEM1 + 1, P2ITEM45 - P2ITEM1 + 1,

    WINDOWSHATTER20 - WINDOWSHATTER1 + 1, P3ITEM16 - P3ITEM1 + 1,
    BODYEXPLOSION15 - BODYEXPLOSION1 + 1,

    EXITTEXTURE35 - EXITTEXTURE1 + 1, FOOTPRINTS80 - FOOTPRINTS1 + 1,

    FIRSTPOINTERS24 - FIRSTPOINTERS1 + 1, SECONDPOINTERS8 - SECONDPOINTERS1 + 1,
    THIRDPOINTERS3 - THIRDPOINTERS1 + 1,

    GOODRUN11 - GOODRUN1 + 1, GOODWALK11 - GOODWALK1 + 1, GOODSWAT11 - GOODSWAT1 + 1,
    GOODPRONE11 - GOODPRONE1 + 1, CONFIRMMOVE11 - CONFIRMMOVE1 + 1,
    VEHICLEMOVE10 - VEHICLEMOVE1 + 1, ACTIONTWO11 - ACTIONTWO1 + 1, BADMARKER11 - BADMARKER1 + 1,
    GOODRING10 - GOODRING1 + 1, ROTATINGKEY8 - ROTATINGKEY1 + 1, SELRING10 - SELRING1 + 1,
    SPECIALTILE_COVER_5 - SPECIALTILE_MAPEXIT + 1, BULLETTILE2 - BULLETTILE1 + 1,

    FIRSTMISS5 - FIRSTMISS1 + 1, SECONDMISS5 - SECONDMISS1 + 1, THIRDMISS14 - THIRDMISS1 + 1,
    WIREFRAMES15 - WIREFRAMES1 + 1};

uint8_t gTileTypeLogicalHeight[NUMBEROFTILETYPES] = {
    2,   // First texture
    2,   // Second texture
    2,   // Third texture
    2,   // Forth texture
    2,   // Fifth texture
    2,   // Sixth texture
    2,   // Seventh texture
    10,  // First water
    10   // Second water
};

static void SetAnimData(uint16_t const database_elem, TILE_ELEMENT &te, const bool minus_plus) {
  AllocateAnimTileData(&te, 5);

  TILE_ANIMATION_DATA &a = *te.pAnimData;
  a.bCurrentFrame = 0;
  for (uint8_t k = 0; k != a.ubNumFrames; ++k) {
    a.pusFrames[k] = minus_plus ? database_elem + k : database_elem - k;
  }
}

static void SetAnimDataIfInList(int16_t const *const list, uint16_t const database_elem,
                                TILE_ELEMENT &te, const bool minus_plus) {
  for (int16_t const *i = list; *i != -1; ++i) {
    // If we are a shadow type
    if (database_elem != *i) continue;
    SetAnimData(database_elem, te, minus_plus);
  }
}

void SetSpecificDatabaseValues(uint16_t const type, uint16_t const database_elem, TILE_ELEMENT &te,
                               bool const use_raised_object_type) {
  // Setup buddies for structures and shadows
  FOR_EACH(ShadowBuddies const, i, g_shadow_buddies) {
    if (type == i->shadow) {
      te.sBuddyNum = database_elem - i->first_shadow + i->first_structure;
      // Check flags and made the same, take from buddy's
      te.uiFlags |= gTileDatabase[te.sBuddyNum].uiFlags;
      break;
    } else if (type == i->structure) {
      te.sBuddyNum = database_elem - i->first_structure + i->first_shadow;
      // Set flag indicating such
      te.uiFlags |= HAS_SHADOW_BUDDY;
      break;
    }
  }

  if (FIRSTDOOR1 <= database_elem && database_elem <= FOURTHDOORSH20) {
    // Door anims
    SetAnimDataIfInList(gOpenDoorList, database_elem, te, true);  // Open
    SetAnimDataIfInList(gOpenDoorShadowList, database_elem, te,
                        true);                                       // Open shadow
    SetAnimDataIfInList(gClosedDoorList, database_elem, te, false);  // Closed
    SetAnimDataIfInList(gClosedDoorShadowList, database_elem, te,
                        false);  // Closed shadow
  }

  if (database_elem == FIRSTMISS1) {
    SetAnimData(database_elem, te, true);
  }

  if (FIRSTMISS1 <= database_elem && database_elem <= FIRSTMISS5) {
    te.uiFlags |= DYNAMIC_TILE;
  }

  if (database_elem == WINDOWSHATTER1 || database_elem == WINDOWSHATTER6 ||
      database_elem == WINDOWSHATTER11 || database_elem == WINDOWSHATTER16) {
    SetAnimData(database_elem, te, true);
  }

  if (WINDOWSHATTER1 <= database_elem && database_elem <= WINDOWSHATTER20) {
    te.uiFlags |= DYNAMIC_TILE;
  }

  if (type == BODYEXPLOSION) {
    te.uiFlags |= DYNAMIC_TILE;
  }

  // Set flags for objects pieces which use proper z
  if (use_raised_object_type) {
    te.uiFlags |= OBJECTLAYER_USEZHEIGHT;
  }

  // Set flag for full 3d tiles, as well as the dynamic flag for the folliage
  if (FIRSTFULLSTRUCT1 <= database_elem && database_elem <= SECONDFULLSTRUCT12) {
    int16_t const idx_diff = database_elem - gTileTypeStartIndex[type];
    switch (idx_diff % 3) {
      case 0:
        te.uiFlags |= FULL3D_TILE;
        break;  // Set every first as full tile
      case 1:
        te.uiFlags |= DYNAMIC_TILE;
        break;  // Every second set as dynamic
    }
  }

  // Ignore height for cliffs ie: if we rasie the land, do not offset the cliff
  if (FIRSTCLIFFHANG1 <= database_elem && database_elem <= FIRSTCLIFFSHADOW17) {
    if (type == FIRSTCLIFFHANG) te.uiFlags |= CLIFFHANG_TILE;
    te.uiFlags |= IGNORE_WORLD_HEIGHT;
  }

  if (FIRSTWALL1 <= database_elem && database_elem <= FOURTHWALL65) {
    te.uiFlags |= WALL_TILE;
  }

  // Set a-frames heigher!
  if (FIRSTWALL1 <= database_elem &&
      database_elem <= FOURTHWALL47) {  // Set these ones higher (for roof pieces)
    uint16_t const start = gTileTypeStartIndex[type];
    if (start + WALL_AFRAME_START <= database_elem && database_elem <= start + WALL_AFRAME_END) {
      te.uiFlags |= AFRAME_TILE;
    }
  }

  // Set UI Elements to be dynamic
  if (FOOTPRINTS1 <= database_elem && database_elem <= THIRDPOINTERS2) {
    te.uiFlags |= DYNAMIC_TILE | OBJECTLAYER_USEZHEIGHT;
  }

  // Set UI Elements to use object z level
  if (FOOTPRINTS <= type && type <= LASTUIELEM) {
    te.uiFlags |= OBJECTLAYER_USEZHEIGHT;
  }

  // Set UI Elements to use object z level
  if (HUMANBLOOD <= type && type <= CREATUREBLOOD) {
    te.uiFlags |= OBJECTLAYER_USEZHEIGHT;
  }

  // Set UI Elements to use object z level
  if (GUNS <= type && type <= P2ITEMS) {
    te.uiFlags |= OBJECTLAYER_USEZHEIGHT;
  }
  if (type == P3ITEMS) {
    te.uiFlags |= OBJECTLAYER_USEZHEIGHT;
  }

  if (FIRSTEXPLDEBRIS <= type && type <= SECONDEXPLDEBRIS) {
    te.uiFlags |= OBJECTLAYER_USEZHEIGHT;
  }

  // Set UI Elements to be dynamic
  if (database_elem == MOCKFLOOR1) {
    te.uiFlags |= DYNAMIC_TILE;
  }

  if (type == BULLETTILE) {
    te.uiFlags |= DYNAMIC_TILE;
  }

  // Set full tile flag for floors
  if (FIRSTFLOOR1 <= database_elem && database_elem <= FOURTHFLOOR8) {
    te.ubFullTile = 1;
  }

  if (/*FIRSTTEXTURE1 <= database_elem &&*/ database_elem <=
      DEEPWATERTEXTURE10) {  // Set tile 'fullness' attribute
    te.ubFullTile = gFullBaseTileValues[database_elem];
  }

  if ((REGWATERTEXTURE18 <= database_elem && database_elem <= REGWATERTEXTURE50) ||
      database_elem == REGWATERTEXTURE || database_elem == REGWATERTEXTURE12 ||
      database_elem == REGWATERTEXTURE14 || database_elem == REGWATERTEXTURE16) {
    te.ubTerrainID = FLAT_GROUND;
  }

  if ((FIRSTROOF <= type && type <= SECONDSLANTROOF) || type == FIRSTHIGHROOF ||
      type == SECONDHIGHROOF) {
    te.uiFlags |= ROOF_TILE;
  }
}
