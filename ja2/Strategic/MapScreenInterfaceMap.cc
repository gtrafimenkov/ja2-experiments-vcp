// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#include "Strategic/MapScreenInterfaceMap.h"

#include <stdexcept>
#include <stdio.h>
#include <string.h>
#include <wchar.h>

#include "Directories.h"
#include "Laptop/Finances.h"
#include "Local.h"
#include "Macro.h"
#include "SGP/ButtonSystem.h"
#include "SGP/Debug.h"
#include "SGP/Font.h"
#include "SGP/HImage.h"
#include "SGP/Line.h"
#include "SGP/MemMan.h"
#include "SGP/VObject.h"
#include "SGP/VObjectBlitters.h"
#include "SGP/VSurface.h"
#include "SGP/Video.h"
#include "Strategic/Assignments.h"
#include "Strategic/CampaignTypes.h"
#include "Strategic/GameClock.h"
#include "Strategic/MapScreen.h"
#include "Strategic/MapScreenHelicopter.h"
#include "Strategic/MapScreenInterface.h"
#include "Strategic/MapScreenInterfaceBorder.h"
#include "Strategic/PlayerCommand.h"
#include "Strategic/PreBattleInterface.h"
#include "Strategic/QueenCommand.h"
#include "Strategic/StrategicMap.h"
#include "Strategic/StrategicMines.h"
#include "Strategic/StrategicMovement.h"
#include "Strategic/StrategicPathing.h"
#include "Strategic/StrategicTownLoyalty.h"
#include "Strategic/TownMilitia.h"
#include "Tactical/AirRaid.h"
#include "Tactical/Interface.h"
#include "Tactical/MapInformation.h"
#include "Tactical/MercHiring.h"
#include "Tactical/MilitiaControl.h"
#include "Tactical/Overhead.h"
#include "Tactical/SoldierProfile.h"
#include "Tactical/Squads.h"
#include "Tactical/TacticalSave.h"
#include "Tactical/Vehicles.h"
#include "TileEngine/RenderDirty.h"
#include "TileEngine/SysUtil.h"
#include "Utils/FontControl.h"
#include "Utils/Message.h"
#include "Utils/Text.h"
#include "Utils/TimerControl.h"

// zoom x and y coords for map scrolling
int32_t iZoomX = 0;
int32_t iZoomY = 0;

// Scroll region width
#define SCROLL_REGION 4

// The Map/Mouse Scroll defines
#define EAST_DIR 0
#define WEST_DIR 1
#define NORTH_DIR 2
#define SOUTH_DIR 3
#define TOP_NORTH 2
#define TOP_SOUTH 13
#define RIGHT_WEST 250
#define RIGHT_EAST 260
#define LEFT_EAST 640
#define LEFT_WEST 630
#define BOTTOM_NORTH 320
#define BOTTOM_SOUTH 330

// Map Scroll Defines
#define SCROLL_EAST 0
#define SCROLL_WEST 1
#define SCROLL_NORTH 2
#define SCROLL_SOUTH 3
#define SCROLL_DELAY 50
#define HORT_SCROLL 14
#define VERT_SCROLL 10

// the pop up for helicopter stuff
#define MAP_HELICOPTER_ETA_POPUP_X 400
#define MAP_HELICOPTER_ETA_POPUP_Y 250
#define MAP_HELICOPTER_UPPER_ETA_POPUP_Y 50
#define MAP_HELICOPTER_ETA_POPUP_WIDTH 120
#define MAP_HELICOPTER_ETA_POPUP_HEIGHT 68

#define MAP_LEVEL_STRING_X 432
#define MAP_LEVEL_STRING_Y 305

// font
#define MAP_FONT BLOCKFONT2

// index color
#define MAP_INDEX_COLOR 32 * 4 - 9

// max number of sectors viewable
#define MAX_VIEW_SECTORS 16

// Map Location index regions

// x start of hort index
#define MAP_HORT_INDEX_X 292

// y position of hort index
#define MAP_HORT_INDEX_Y 10

// height of hort index
#define MAP_HORT_HEIGHT GetFontHeight(MAP_FONT)

// vert index start x
#define MAP_VERT_INDEX_X 273

// vert index start y
#define MAP_VERT_INDEX_Y 31

// vert width
#define MAP_VERT_WIDTH GetFontHeight(MAP_FONT)

// "Boxes" Icons
#define SMALL_YELLOW_BOX 0
#define BIG_YELLOW_BOX 1
#define SMALL_DULL_YELLOW_BOX 2
#define BIG_DULL_YELLOW_BOX 3
#define SMALL_WHITE_BOX 4
#define BIG_WHITE_BOX 5
#define SMALL_RED_BOX 6
#define BIG_RED_BOX 7
#define SMALL_QUESTION_MARK 8
#define BIG_QUESTION_MARK 9

#define MERC_ICONS_PER_LINE 6
#define ROWS_PER_SECTOR 5

#define MAP_X_ICON_OFFSET 2
#define MAP_Y_ICON_OFFSET 1

// Arrow Offsets
#define UP_X 13
#define UP_Y 7
#define DOWN_X 0
#define DOWN_Y -2
#define RIGHT_X -2
#define RIGHT_Y 11
#define LEFT_X 2
#define LEFT_Y 5

// The Path Lines
#define NORTH_LINE 1
#define SOUTH_LINE 0
#define WEST_LINE 3
#define EAST_LINE 2
#define N_TO_E_LINE 4
#define E_TO_S_LINE 5
#define W_TO_N_LINE 6
#define S_TO_W_LINE 7
#define W_TO_S_LINE 8
#define N_TO_W_LINE 9
#define S_TO_E_LINE 10
#define E_TO_N_LINE 11
#define W_TO_E_LINE 12
#define N_TO_S_LINE 13
#define E_TO_W_LINE 14
#define S_TO_N_LINE 15
#define W_TO_E_PART1_LINE 16
#define W_TO_E_PART2_LINE 17
#define E_TO_W_PART1_LINE 18
#define E_TO_W_PART2_LINE 19
#define N_TO_S_PART1_LINE 20
#define N_TO_S_PART2_LINE 21
#define S_TO_N_PART1_LINE 22
#define S_TO_N_PART2_LINE 23
#define GREEN_X_WEST 36
#define GREEN_X_EAST 37
#define GREEN_X_NORTH 38
#define GREEN_X_SOUTH 39
#define RED_X_WEST 40
#define RED_X_EAST 41
#define RED_X_NORTH 42
#define RED_X_SOUTH 43

// The arrows
#define Y_NORTH_ARROW 24
#define Y_SOUTH_ARROW 25
#define Y_EAST_ARROW 26
#define Y_WEST_ARROW 27
#define W_NORTH_ARROW 28
#define W_SOUTH_ARROW 29
#define W_EAST_ARROW 30
#define W_WEST_ARROW 31
#define NORTH_ARROW 32
#define SOUTH_ARROW 33
#define EAST_ARROW 34
#define WEST_ARROW 35

#define ZOOM_Y_NORTH_ARROW 68
#define ZOOM_Y_SOUTH_ARROW 69
#define ZOOM_Y_EAST_ARROW 70
#define ZOOM_Y_WEST_ARROW 71
#define ZOOM_W_NORTH_ARROW 72
#define ZOOM_W_SOUTH_ARROW 73
#define ZOOM_W_EAST_ARROW 74
#define ZOOM_W_WEST_ARROW 75
#define ZOOM_NORTH_ARROW 76
#define ZOOM_SOUTH_ARROW 77
#define ZOOM_EAST_ARROW 78
#define ZOOM_WEST_ARROW 79
#define ARROW_DELAY 20
#define PAUSE_DELAY 1000

// The zoomed in path lines
#define SOUTH_ZOOM_LINE 44
#define NORTH_ZOOM_LINE 45
#define EAST_ZOOM_LINE 46
#define WEST_ZOOM_LINE 47
#define N_TO_E_ZOOM_LINE 48
#define E_TO_S_ZOOM_LINE 49
#define W_TO_N_ZOOM_LINE 50
#define S_TO_W_ZOOM_LINE 51
#define W_TO_S_ZOOM_LINE 52
#define N_TO_W_ZOOM_LINE 53
#define S_TO_E_ZOOM_LINE 54
#define E_TO_N_ZOOM_LINE 55
#define W_TO_E_ZOOM_LINE 56
#define N_TO_S_ZOOM_LINE 57
#define E_TO_W_ZOOM_LINE 58
#define S_TO_N_ZOOM_LINE 59
#define ZOOM_GREEN_X_WEST 80
#define ZOOM_GREEN_X_EAST 81
#define ZOOM_GREEN_X_NORTH 82
#define ZOOM_GREEN_X_SOUTH 83
#define ZOOM_RED_X_WEST 84
#define ZOOM_RED_X_EAST 85
#define ZOOM_RED_X_NORTH 86
#define ZOOM_RED_X_SOUTH 87

#define CHAR_FONT_COLOR 32 * 4 - 9

// Arrow Offsets
#define EAST_OFFSET_X 11
#define EAST_OFFSET_Y 0
#define NORTH_OFFSET_X 9
#define NORTH_OFFSET_Y -9
#define SOUTH_OFFSET_X -9
#define SOUTH_OFFSET_Y 9
#define WEST_OFFSET_X -11
#define WEST_OFFSET_Y 0
#define WEST_TO_SOUTH_OFFSET_Y 0
#define EAST_TO_NORTH_OFFSET_Y 0
#define RED_WEST_OFF_X -MAP_GRID_X
#define RED_EAST_OFF_X MAP_GRID_X
#define RED_NORTH_OFF_Y -21
#define RED_SOUTH_OFF_Y 21

// the font use on the mvt icons for mapscreen
#define MAP_MVT_ICON_FONT SMALLCOMPFONT

// map shading colors

enum {
  MAP_SHADE_BLACK = 0,
  MAP_SHADE_LT_GREEN,
  MAP_SHADE_DK_GREEN,
  MAP_SHADE_LT_RED,
  MAP_SHADE_DK_RED,
};
// the big map .pcx
static SGPVSurface *guiBIGMAP;

// orta .sti icon
static SGPVObject *guiORTAICON;
static SGPVObject *guiTIXAICON;

// boxes for characters on the map
static SGPVObject *guiCHARICONS;

// the merc arrival sector landing zone icon
static SGPVObject *guiBULLSEYE;

// the max allowable towns militia in a sector
#define MAP_MILITIA_MAP_X 4
#define MAP_MILITIA_MAP_Y 20
#define MAP_MILITIA_LOWER_ROW_Y 142
#define NUMBER_OF_MILITIA_ICONS_PER_LOWER_ROW 25
#define MILITIA_BOX_ROWS 3
#define MILITIA_BOX_BOX_HEIGHT 36
#define MILITIA_BOX_BOX_WIDTH 42
#define MAP_MILITIA_BOX_POS_X 400
#define MAP_MILITIA_BOX_POS_Y 125

#define POPUP_MILITIA_ICONS_PER_ROW 5  // max 6 rows gives the limit of 30 militia
#define MEDIUM_MILITIA_ICON_SPACING 5
#define LARGE_MILITIA_ICON_SPACING 6

#define MILITIA_BTN_OFFSET_X 26
#define MILITIA_BTN_HEIGHT 11
#define MILITIA_BOX_WIDTH 133
#define MILITIA_BOX_TEXT_OFFSET_Y 4
#define MILITIA_BOX_UNASSIGNED_TEXT_OFFSET_Y 132
#define MILITIA_BOX_TEXT_TITLE_HEIGHT 13

#define MAP_MILITIA_BOX_AUTO_BOX_X 4
#define MAP_MILITIA_BOX_AUTO_BOX_Y 167
#define MAP_MILITIA_BOX_DONE_BOX_X 67

#define HELI_ICON 0
#define HELI_SHADOW_ICON 1

#define HELI_ICON_WIDTH 20
#define HELI_ICON_HEIGHT 10
#define HELI_SHADOW_ICON_WIDTH 19
#define HELI_SHADOW_ICON_HEIGHT 11

// the militia box buttons
static GUIButtonRef giMapMilitiaButton[5];

static int16_t const gsMilitiaSectorButtonColors[] = {FONT_LTGREEN, FONT_LTBLUE, 16};

// track number of townspeople picked up
int16_t sGreensOnCursor = 0;
int16_t sRegularsOnCursor = 0;
int16_t sElitesOnCursor = 0;

// the current militia town id
int16_t sSelectedMilitiaTown = 0;

// sublevel graphics
static SGPVObject *guiSubLevel1;
static SGPVObject *guiSubLevel2;
static SGPVObject *guiSubLevel3;

// the between sector icons
static SGPVObject *guiCHARBETWEENSECTORICONS;
static SGPVObject *guiCHARBETWEENSECTORICONSCLOSE;

// tixa found
BOOLEAN fFoundTixa = FALSE;

// selected sector
uint16_t sSelMapX = 9;
uint16_t sSelMapY = 1;

// highlighted sector
int16_t gsHighlightSectorX = -1;
int16_t gsHighlightSectorY = -1;

// the current sector Z value of the map being displayed
int32_t iCurrentMapSectorZ = 0;

// the palettes
static uint16_t *pMapLTRedPalette;
static uint16_t *pMapDKRedPalette;
static uint16_t *pMapLTGreenPalette;
static uint16_t *pMapDKGreenPalette;

// heli pop up
static SGPVObject *guiMapBorderHeliSectors;

static uint16_t const sBaseSectorList[] = {
    // NOTE: These co-ordinates must match the top left corner of the 3x3 town
    // tiles cutouts in interface/militiamaps.sti!
    SECTOR(9, 1),    // Omerta
    SECTOR(13, 2),   // Drassen
    SECTOR(13, 8),   // Alma
    SECTOR(1, 7),    // Grumm
    SECTOR(8, 9),    // Tixa
    SECTOR(8, 6),    // Cambria
    SECTOR(4, 2),    // San Mona
    SECTOR(5, 8),    // Estoni
    SECTOR(3, 10),   // Orta
    SECTOR(11, 11),  // Balime
    SECTOR(3, 14),   // Meduna
    SECTOR(2, 1),    // Chitzena
};

// position of town names on the map
// these are no longer PIXELS, but 10 * the X,Y position in SECTORS (fractions
// possible) to the X-CENTER of the town
static SGPPoint const pTownPoints[] = {
    {0, 0},     {90, 10},  // Omerta
    {125, 40},             // Drassen
    {130, 90},             // Alma
    {15, 80},              // Grumm
    {85, 100},             // Tixa
    {95, 70},              // Cambria
    {45, 40},              // San Mona
    {55, 90},              // Estoni
    {35, 110},             // Orta
    {110, 120},            // Balime
    {45, 150},             // Meduna
    {15, 20},              // Chitzena
};

// map region
SGPRect MapScreenRect = {(MAP_VIEW_START_X + MAP_GRID_X - 2), (MAP_VIEW_START_Y + MAP_GRID_Y - 1),
                         MAP_VIEW_START_X + MAP_VIEW_WIDTH - 1 + MAP_GRID_X,
                         MAP_VIEW_START_Y + MAP_VIEW_HEIGHT - 10 + MAP_GRID_Y};

static SGPRect gOldClipRect;

// screen region
static SGPRect FullScreenRect = {0, 0, SCREEN_WIDTH, SCREEN_HEIGHT};

// temp helicopter path
PathSt *pTempHelicopterPath = NULL;

// character temp path
PathSt *pTempCharacterPath = NULL;

// draw temp path?
BOOLEAN fDrawTempHeliPath = FALSE;

// the map arrows graphics
static SGPVObject *guiMAPCURSORS;

// destination plotting character
int8_t bSelectedDestChar = -1;

// assignment selection character
int8_t bSelectedAssignChar = -1;

// current contract char
int8_t bSelectedContractChar = -1;

// has the temp path for character or helicopter been already drawn?
BOOLEAN fTempPathAlreadyDrawn = FALSE;

// the regions for the mapscreen militia box
static MOUSE_REGION gMapScreenMilitiaBoxRegions[9];

// the mine icon
static SGPVObject *guiMINEICON;

// militia graphics
static SGPVObject *guiMilitia;
static SGPVObject *guiMilitiaMaps;
static SGPVObject *guiMilitiaSectorHighLight;
static SGPVObject *guiMilitiaSectorOutline;

// the sector that is highlighted on the militia map
static int16_t sSectorMilitiaMapSector = -1;
static bool fMilitiaMapButtonsCreated = false;
static int16_t sSectorMilitiaMapSectorOutline = -1;

// have any nodes in the current path list been deleted?
BOOLEAN fDeletedNode = FALSE;

static uint16_t gusUndergroundNearBlack;

BOOLEAN gfMilitiaPopupCreated = FALSE;

int32_t giAnimateRouteBaseTime = 0;
int32_t giPotHeliPathBaseTime = 0;

// sam and mine icons
static SGPVObject *guiSAMICON;

// helicopter icon
static SGPVObject *guiHelicopterIcon;

void DrawMapIndexBigMap(BOOLEAN fSelectedCursorIsYellow) {
  // this procedure will draw the coord indexes on the zoomed out map
  SetFontDestBuffer(FRAME_BUFFER);
  SetFont(MAP_FONT);
  SetFontBackground(FONT_MCOLOR_BLACK);

  bool const draw_cursors = CanDrawSectorCursor();
  bool const sel_candidate = bSelectedDestChar == -1 && !fPlotForHelicopter;
  uint8_t const sel_colour = fSelectedCursorIsYellow ? FONT_YELLOW : FONT_WHITE;
  for (int32_t i = 1; i <= MAX_VIEW_SECTORS; ++i) {
    int16_t usX;
    int16_t usY;

    uint8_t const colour_x = !draw_cursors                    ? MAP_INDEX_COLOR
                             : i == sSelMapX && sel_candidate ? sel_colour
                             : i == gsHighlightSectorX        ? FONT_WHITE
                                                              : MAP_INDEX_COLOR;
    SetFontForeground(colour_x);
    FindFontCenterCoordinates(MAP_HORT_INDEX_X + (i - 1) * MAP_GRID_X, MAP_HORT_INDEX_Y, MAP_GRID_X,
                              MAP_HORT_HEIGHT, pMapHortIndex[i], MAP_FONT, &usX, &usY);
    MPrint(usX, usY, pMapHortIndex[i]);

    uint8_t const colour_y = !draw_cursors                    ? MAP_INDEX_COLOR
                             : i == sSelMapY && sel_candidate ? sel_colour
                             : i == gsHighlightSectorY        ? FONT_WHITE
                                                              : MAP_INDEX_COLOR;
    SetFontForeground(colour_y);
    FindFontCenterCoordinates(MAP_VERT_INDEX_X, MAP_VERT_INDEX_Y + (i - 1) * MAP_GRID_Y,
                              MAP_HORT_HEIGHT, MAP_GRID_Y, pMapVertIndex[i], MAP_FONT, &usX, &usY);
    MPrint(usX, usY, pMapVertIndex[i]);
  }

  InvalidateRegion(MAP_VERT_INDEX_X, MAP_VERT_INDEX_Y, MAP_VERT_INDEX_X + MAP_HORT_HEIGHT,
                   MAP_VERT_INDEX_Y + MAX_VIEW_SECTORS * MAP_GRID_Y);
  InvalidateRegion(MAP_HORT_INDEX_X, MAP_HORT_INDEX_Y,
                   MAP_HORT_INDEX_X + MAX_VIEW_SECTORS * MAP_GRID_X,
                   MAP_HORT_INDEX_Y + MAP_HORT_HEIGHT);
}

/*
void DrawMapIndexSmallMap( BOOLEAN fSelectedCursorIsYellow )
{
        // this procedure will draw the coord indexes on the zoomed in map
        int16_t usX, usY;
        int32_t iCount=0;
        BOOLEAN fDrawCursors;


        SetFontDestBuffer(FRAME_BUFFER, MAP_HORT_INDEX_X, MAP_HORT_INDEX_Y,
MAP_HORT_INDEX_X + MAX_VIEW_SECTORS * MAP_GRID_X, MAP_HORT_INDEX_Y +
MAP_GRID_Y); SetFontAttributes(MAP_FONT, MAP_INDEX_COLOR);

        fDrawCursors = CanDrawSectorCursor( );

        for(iCount=1; iCount <= MAX_VIEW_SECTORS; iCount++)
        {
                if (fDrawCursors && iCount == sSelMapX && bSelectedDestChar ==
-1 && !fPlotForHelicopter) SetFontForeground(fSelectedCursorIsYellow ?
FONT_YELLOW : FONT_WHITE); else if( fDrawCursors && ( iCount ==
gsHighlightSectorX ) ) SetFontForeground(FONT_WHITE); else
    SetFontForeground(MAP_INDEX_COLOR);

   FindFontCenterCoordinates(MAP_HORT_INDEX_X + iCount * MAP_GRID_X * 2 -
iZoomX, MAP_HORT_INDEX_Y, MAP_GRID_X * 2, MAP_HORT_HEIGHT,
pMapHortIndex[iCount], MAP_FONT, &usX, &usY); MPrint(usX, usY,
pMapHortIndex[iCount]);
  }
        SetFontDestBuffer(FRAME_BUFFER, MAP_VERT_INDEX_X, MAP_VERT_INDEX_Y,
MAP_VERT_INDEX_X + MAP_GRID_X, MAP_VERT_INDEX_Y + MAX_VIEW_SECTORS *
MAP_GRID_Y);

        for(iCount=1; iCount <= MAX_VIEW_SECTORS; iCount++)
        {
                if (fDrawCursors && iCount == sSelMapY && bSelectedDestChar ==
-1 && !fPlotForHelicopter) SetFontForeground(fSelectedCursorIsYellow ?
FONT_YELLOW : FONT_WHITE); else if( fDrawCursors && ( iCount ==
gsHighlightSectorY ) ) SetFontForeground(FONT_WHITE); else
    SetFontForeground(MAP_INDEX_COLOR);

         FindFontCenterCoordinates(MAP_VERT_INDEX_X, MAP_VERT_INDEX_Y + iCount *
MAP_GRID_Y * 2 - iZoomY, MAP_HORT_HEIGHT, MAP_GRID_Y * 2, pMapVertIndex[iCount],
MAP_FONT, &usX, &usY); MPrint(usX, usY, pMapVertIndex[iCount]);
        }

  InvalidateRegion(MAP_VERT_INDEX_X,
MAP_VERT_INDEX_Y,MAP_VERT_INDEX_X+MAP_HORT_HEIGHT,
MAP_VERT_INDEX_Y+iCount*MAP_GRID_Y ); InvalidateRegion(MAP_HORT_INDEX_X,
MAP_HORT_INDEX_Y,MAP_HORT_INDEX_X+iCount*MAP_GRID_X,  MAP_HORT_INDEX_Y+
MAP_HORT_HEIGHT); SetFontDestBuffer(FRAME_BUFFER);
}
*/

static void HandleShowingOfEnemyForcesInSector(int16_t sSectorX, int16_t sSectorY, int8_t bSectorZ,
                                               uint8_t ubIconPosition);

static void HandleShowingOfEnemiesWithMilitiaOn() {
  for (int16_t sX = 1; sX < MAP_WORLD_X - 1; ++sX) {
    for (int16_t sY = 1; sY < MAP_WORLD_Y - 1; ++sY) {
      HandleShowingOfEnemyForcesInSector(sX, sY, iCurrentMapSectorZ,
                                         CountAllMilitiaInSector(sX, sY));
    }
  }
}

static void BlitMineGridMarkers();
static void BlitMineIcon(int16_t sMapX, int16_t sMapY);
static void BlitMineText(uint8_t mine_idx, int16_t sMapX, int16_t sMapY);
static void BlitTownGridMarkers();
static void DisplayLevelString();
static void DrawBullseye();
static void DrawOrta();
static void DrawTixa();
static void DrawTownMilitiaForcesOnMap();
static void HandleLowerLevelMapBlit();
static void ShadeMapElem(int16_t sMapX, int16_t sMapY, int32_t iColor);
static void ShowItemsOnMap();
static void ShowSAMSitesOnStrategicMap();
static void ShowTeamAndVehicles();
static void ShowTownText();

void DrawMap() {
  if (!iCurrentMapSectorZ) {
    if (fZoomFlag) {
      if (iZoomX < WEST_ZOOM_BOUND) iZoomX = WEST_ZOOM_BOUND;
      if (iZoomX > EAST_ZOOM_BOUND) iZoomX = EAST_ZOOM_BOUND;
      if (iZoomY < NORTH_ZOOM_BOUND + 1) iZoomY = NORTH_ZOOM_BOUND;
      if (iZoomY > SOUTH_ZOOM_BOUND) iZoomY = SOUTH_ZOOM_BOUND;

      int16_t const x = iZoomX - 2;
      int16_t const y = iZoomY - 3;
      int16_t w = MAP_VIEW_WIDTH + 2;
      int16_t h = MAP_VIEW_HEIGHT - 1;
      uint16_t const src_w = guiBIGMAP->Width();
      uint16_t const src_h = guiBIGMAP->Height();
      if (w > src_w - x) w = src_w - x;
      if (h > src_h - x) h = src_h - y;
      SGPBox const clip = {x, y, w, h};
      BltVideoSurface(guiSAVEBUFFER, guiBIGMAP, MAP_VIEW_START_X + MAP_GRID_X,
                      MAP_VIEW_START_Y + MAP_GRID_Y - 2, &clip);
    } else {
      BltVideoSurfaceHalf(guiSAVEBUFFER, guiBIGMAP, MAP_VIEW_START_X + 1, MAP_VIEW_START_Y, NULL);
    }

    // shade map sectors (must be done after Tixa/Orta/Mine icons have been
    // blitted, but before icons!)
    for (int16_t cnt = 1; cnt < MAP_WORLD_X - 1; ++cnt) {
      for (int16_t cnt2 = 1; cnt2 < MAP_WORLD_Y - 1; ++cnt2) {
        if (!GetSectorFlagStatus(cnt, cnt2, iCurrentMapSectorZ, SF_ALREADY_VISITED)) {
          int32_t color;
          if (fShowAircraftFlag) {
            if (!StrategicMap[cnt + cnt2 * WORLD_MAP_X].fEnemyAirControlled) {
              // sector not visited, not air controlled
              color = MAP_SHADE_DK_GREEN;
            } else {
              // sector not visited, controlled and air not
              color = MAP_SHADE_DK_RED;
            }
          } else {
            // not visited
            color = MAP_SHADE_BLACK;
          }
          ShadeMapElem(cnt, cnt2, color);
        } else {
          if (fShowAircraftFlag) {
            int32_t color;
            if (!StrategicMap[cnt + cnt2 * WORLD_MAP_X].fEnemyAirControlled) {
              // sector visited and air controlled
              color = MAP_SHADE_LT_GREEN;
            } else {
              // sector visited but not air controlled
              color = MAP_SHADE_LT_RED;
            }
            ShadeMapElem(cnt, cnt2, color);
          }
        }
      }
    }

    /* unfortunately, we can't shade these icons as part of shading the map,
     * because for airspace, the shade function doesn't merely shade the
     * existing map surface, but instead grabs the original graphics from
     * bigmap, and changes their palette.  blitting icons prior to shading would
     * mean they don't show up in airspace view at all. */

    if (fFoundOrta) DrawOrta();
    if (fFoundTixa) DrawTixa();

    ShowSAMSitesOnStrategicMap();

    // draw mine icons and descriptive text
    for (int32_t i = 0; i < MAX_NUMBER_OF_MINES; ++i) {
      uint8_t const sector = gMineLocation[i].sector;
      int16_t const x = SECTORX(sector);
      int16_t const y = SECTORY(sector);
      BlitMineIcon(x, y);
      // show mine name/production text
      if (fShowMineFlag) BlitMineText(i, x, y);
    }

    // draw towns names & loyalty ratings, and grey town limit borders
    if (fShowTownFlag) {
      BlitTownGridMarkers();
      ShowTownText();
    }

    if (fShowMilitia) DrawTownMilitiaForcesOnMap();

    if (fShowAircraftFlag && !gfInChangeArrivalSectorMode) DrawBullseye();
  } else {
    HandleLowerLevelMapBlit();
  }

  /* show mine outlines even when viewing underground sublevels - they indicate
   * where the mine entrances are */
  if (fShowMineFlag) BlitMineGridMarkers();

  if (fShowTeamFlag) {
    ShowTeamAndVehicles();
  } else if (fShowMilitia) {
    HandleShowingOfEnemiesWithMilitiaOn();
  }

  if (fShowItemsFlag) ShowItemsOnMap();

  DisplayLevelString();
}

void GetScreenXYFromMapXY(int16_t sMapX, int16_t sMapY, int16_t *psX, int16_t *psY) {
  int16_t sXTempOff = 1;
  int16_t sYTempOff = 1;
  if (fZoomFlag) {
    *psX = ((sMapX / 2 + sXTempOff) * MAP_GRID_ZOOM_X) + MAP_VIEW_START_X;
    *psY = ((sMapY / 2 + sYTempOff) * MAP_GRID_ZOOM_Y) + MAP_VIEW_START_Y;
  } else {
    *psX = (sMapX * MAP_GRID_X) + MAP_VIEW_START_X;
    *psY = (sMapY * MAP_GRID_Y) + MAP_VIEW_START_Y;
  }
}

static void GetScreenXYFromMapXYStationary(int16_t sMapX, int16_t sMapY, int16_t *psX,
                                           int16_t *psY) {
  int16_t sXTempOff = 1;
  int16_t sYTempOff = 1;
  //(MAP_VIEW_START_X+((iCount+1)*MAP_GRID_X)*2-iZoomX));
  *psX = ((sMapX + sXTempOff) * MAP_GRID_X) * 2 - ((int16_t)iZoomX) + MAP_VIEW_START_X;
  *psY = ((sMapY + sYTempOff) * MAP_GRID_Y) * 2 - ((int16_t)iZoomY) + MAP_VIEW_START_Y;
}

// display the town names and loyalty on the screen
static void ShowTownText() {
  SetFont(MAP_FONT);
  SetFontBackground(FONT_MCOLOR_BLACK);
  SetFontDestBuffer(guiSAVEBUFFER, MapScreenRect.iLeft + 2, MapScreenRect.iTop,
                    MapScreenRect.iRight, MapScreenRect.iBottom);
  ClipBlitsToMapViewRegion();

  for (int8_t town = FIRST_TOWN; town < NUM_TOWNS; ++town) {
    // skip Orta/Tixa until found
    if (town == ORTA && !fFoundOrta) continue;
    if (town == TIXA && !fFoundTixa) continue;

    uint16_t x = MAP_VIEW_START_X + MAP_GRID_X;
    uint16_t y = MAP_VIEW_START_Y + MAP_GRID_Y;
    if (!fZoomFlag) {
      x += pTownPoints[town].iX * MAP_GRID_X / 10;
      y += pTownPoints[town].iY * MAP_GRID_Y / 10 + 1;
    } else {
      x += MAP_GRID_ZOOM_X - iZoomX + pTownPoints[town].iX * MAP_GRID_ZOOM_X / 10;
      y += MAP_GRID_ZOOM_Y - iZoomY + pTownPoints[town].iY * MAP_GRID_ZOOM_Y / 10 + 1;
    }

    // if within view region...render, else don't
    if (x < MAP_VIEW_START_X || MAP_VIEW_START_X + MAP_VIEW_WIDTH < x) continue;
    if (y < MAP_VIEW_START_Y || MAP_VIEW_START_Y + MAP_VIEW_HEIGHT < y) continue;

    // don't show loyalty string until loyalty tracking for that town has been
    // started
    if (gTownLoyalty[town].fStarted && gfTownUsesLoyalty[town]) {
      // if loyalty is too low to train militia, and militia training is allowed
      // here
      uint8_t const colour = gTownLoyalty[town].ubRating < MIN_RATING_TO_TRAIN_TOWN &&
                                     MilitiaTrainingAllowedInTown(town)
                                 ? FONT_MCOLOR_RED
                                 : FONT_MCOLOR_LTGREEN;
      SetFontForeground(colour);

      wchar_t loyalty_str[32];
      swprintf(loyalty_str, lengthof(loyalty_str), gsLoyalString, gTownLoyalty[town].ubRating);

      int16_t loyalty_x = x - StringPixLength(loyalty_str, MAP_FONT) / 2;
      if (!fZoomFlag)  // make sure we don't go past left edge (Grumm)
      {
        int16_t const min_x = MAP_VIEW_START_X + 23;
        if (loyalty_x < min_x) loyalty_x = min_x;
      }

      GDirtyPrint(loyalty_x, y + GetFontHeight(MAP_FONT), loyalty_str);
    } else {
      SetFontForeground(FONT_MCOLOR_LTGREEN);
    }

    wchar_t const *const name = pTownNames[town];
    int16_t const name_x = x - StringPixLength(name, MAP_FONT) / 2;
    GDirtyPrint(name_x, y, name);
  }

  RestoreClipRegionToFullScreen();
}

static void DrawMapBoxIcon(HVOBJECT, uint16_t icon, int16_t sec_x, int16_t sec_y, uint8_t icon_pos);

// "on duty" includes mercs inside vehicles
static int32_t ShowOnDutyTeam(int16_t const x, int16_t const y) {
  uint8_t icon_pos = 0;
  CFOR_EACH_IN_CHAR_LIST(c) {
    SOLDIERTYPE const &s = *c->merc;
    if (s.uiStatusFlags & SOLDIER_VEHICLE) continue;
    if (s.sSectorX != x) continue;
    if (s.sSectorY != y) continue;
    if (s.bSectorZ != iCurrentMapSectorZ) continue;
    if (s.bLife <= 0) continue;
    if (s.bAssignment >= ON_DUTY && s.bAssignment != VEHICLE) continue;
    if (InHelicopter(s)) continue;
    if (PlayerIDGroupInMotion(s.ubGroupID)) continue;
    DrawMapBoxIcon(guiCHARICONS, SMALL_YELLOW_BOX, x, y, icon_pos++);
  }
  return icon_pos;
}

static int32_t ShowAssignedTeam(int16_t const x, int16_t const y, int32_t icon_pos) {
  CFOR_EACH_IN_CHAR_LIST(c) {
    SOLDIERTYPE const &s = *c->merc;
    // given number of on duty members, find number of assigned chars
    // start at beginning of list, look for people who are in sector and
    // assigned
    if (s.uiStatusFlags & SOLDIER_VEHICLE) continue;
    if (s.sSectorX != x) continue;
    if (s.sSectorY != y) continue;
    if (s.bSectorZ != iCurrentMapSectorZ) continue;
    if (s.bAssignment < ON_DUTY) continue;
    if (s.bAssignment == VEHICLE) continue;
    if (s.bAssignment == IN_TRANSIT) continue;
    if (s.bAssignment == ASSIGNMENT_POW) continue;
    if (s.bLife <= 0) continue;
    if (PlayerIDGroupInMotion(s.ubGroupID)) continue;

    DrawMapBoxIcon(guiCHARICONS, SMALL_DULL_YELLOW_BOX, x, y, icon_pos++);
  }
  return icon_pos;
}

static int32_t ShowVehicles(int16_t const x, int16_t const y, int32_t icon_pos) {
  CFOR_EACH_VEHICLE(i) {
    VEHICLETYPE const &v = *i;
    // skip the chopper, it has its own icon and displays in airspace mode
    if (IsHelicopter(v)) continue;
    if (v.sSectorX != x) continue;
    if (v.sSectorY != y) continue;
    if (v.sSectorZ != iCurrentMapSectorZ) continue;
    if (PlayerIDGroupInMotion(v.ubMovementGroup)) continue;

    SOLDIERTYPE const &vs = GetSoldierStructureForVehicle(v);
    if (vs.bTeam != OUR_TEAM) continue;

    DrawMapBoxIcon(guiCHARICONS, SMALL_WHITE_BOX, x, y, icon_pos++);
  }
  return icon_pos;
}

static void ShowEnemiesInSector(int16_t const x, int16_t const y, int16_t n_enemies,
                                uint8_t icon_pos) {
  while (n_enemies-- != 0) {
    DrawMapBoxIcon(guiCHARICONS, SMALL_RED_BOX, x, y, icon_pos++);
  }
}

static void ShowUncertainNumberEnemiesInSector(int16_t const sec_x, int16_t const sec_y) {
  if (!fZoomFlag) {
    int16_t const x = MAP_VIEW_START_X + sec_x * MAP_GRID_X + MAP_X_ICON_OFFSET;
    int16_t const y = MAP_VIEW_START_Y + sec_y * MAP_GRID_Y - 1;
    BltVideoObject(guiSAVEBUFFER, guiCHARICONS, SMALL_QUESTION_MARK, x, y);
    InvalidateRegion(x, y, x + DMAP_GRID_X, y + DMAP_GRID_Y);
  }
#if 0  // XXX was commented out
	else
	{
		int16_t sX;
		int16_t sY;
		GetScreenXYFromMapXYStationary(sec_x, sec_y, &sX, &sY);
		int16_t const x = sX - MAP_GRID_X + MAP_X_ICON_OFFSET;
 	 	int16_t const y = sY - MAP_GRID_Y - 1;

		ClipBlitsToMapViewRegion();
		BltVideoObject(guiSAVEBUFFER, guiCHARICONS, BIG_QUESTION_MARK, x, y);
		RestoreClipRegionToFullScreen();
		InvalidateRegion(x, y, x + DMAP_GRID_ZOOM_X, y + DMAP_GRID_ZOOM_Y);
	}
#endif
}

static void ShowPeopleInMotion(int16_t sX, int16_t sY);

static void ShowTeamAndVehicles() {
  // Go through each sector, display the on duty, assigned, and vehicles
  int32_t icon_pos = 0;
  GROUP const *const g = gfDisplayPotentialRetreatPaths ? gpBattleGroup : 0;
  for (int16_t x = 1; x != MAP_WORLD_X - 1; ++x) {
    for (int16_t y = 1; y != MAP_WORLD_Y - 1; ++y) {
      /* Don't show mercs/vehicles currently in this sector if player is
       * contemplating retreating from THIS sector */
      if (!g || x != g->ubSectorX || y != g->ubSectorY) {
        icon_pos = ShowOnDutyTeam(x, y);
        icon_pos = ShowAssignedTeam(x, y, icon_pos);
        icon_pos = ShowVehicles(x, y, icon_pos);
      }

      HandleShowingOfEnemyForcesInSector(x, y, iCurrentMapSectorZ, icon_pos);
      ShowPeopleInMotion(x, y);
    }
  }
}

static void ShadeMapElemZoomIn(int16_t sMapX, int16_t sMapY, int32_t iColor);

static void ShadeMapElem(const int16_t sMapX, const int16_t sMapY, const int32_t iColor) {
  if (fZoomFlag) {
    ShadeMapElemZoomIn(sMapX, sMapY, iColor);
  } else {
    int16_t sScreenX;
    int16_t sScreenY;
    GetScreenXYFromMapXY(sMapX, sMapY, &sScreenX, &sScreenY);

    // compensate for original BIG_MAP blit being done at MAP_VIEW_START_X + 1
    sScreenX += 1;

    SGPBox const clip = {2 * (sScreenX - (MAP_VIEW_START_X + 1)), 2 * (sScreenY - MAP_VIEW_START_Y),
                         2 * MAP_GRID_X, 2 * MAP_GRID_Y};

    // non-airspace
    if (iColor == MAP_SHADE_BLACK) sScreenY -= 1;

    uint16_t *pal;
    switch (iColor) {
      case MAP_SHADE_BLACK:
        // simply shade darker
        guiSAVEBUFFER->ShadowRect(sScreenX, sScreenY, sScreenX + MAP_GRID_X - 1,
                                  sScreenY + MAP_GRID_Y - 1);
        return;

      case MAP_SHADE_LT_GREEN:
        pal = pMapLTGreenPalette;
        break;
      case MAP_SHADE_DK_GREEN:
        pal = pMapDKGreenPalette;
        break;
      case MAP_SHADE_LT_RED:
        pal = pMapLTRedPalette;
        break;
      case MAP_SHADE_DK_RED:
        pal = pMapDKRedPalette;
        break;

      default:
        return;
    }

    // get original video surface palette
    SGPVSurface *const map = guiBIGMAP;
    // SGPVSurface* const mine = guiMINEICON;
    // SGPVSurface* const sam  = guiSAMICON;

    uint16_t *const org_pal = map->p16BPPPalette;
    map->p16BPPPalette = pal;
    // mine->p16BPPPalette = pal;
    // sam->p16BPPPalette  = pal;
    BltVideoSurfaceHalf(guiSAVEBUFFER, guiBIGMAP, sScreenX, sScreenY, &clip);
    map->p16BPPPalette = org_pal;
    // mine->p16BPPPalette = org_pal;
    // sam->p16BPPPalette  = org_pal;
  }
}

static void ShadeMapElemZoomIn(const int16_t sMapX, const int16_t sMapY, int32_t iColor) {
  // trabslate to screen co-ords for zoomed
  int16_t sScreenX;
  int16_t sScreenY;
  GetScreenXYFromMapXYStationary(sMapX, sMapY, &sScreenX, &sScreenY);

  // shift left by one sector
  const int32_t iX = sScreenX - MAP_GRID_X;
  const int32_t iY = sScreenY - MAP_GRID_Y;

  if (MapScreenRect.iLeft - MAP_GRID_X * 2 < iX && iX < MapScreenRect.iRight &&
      MapScreenRect.iTop - MAP_GRID_Y * 2 < iY && iY < MapScreenRect.iBottom) {
    sScreenX = iX;
    sScreenY = iY;

    SGPRect clip;
    if (iColor == MAP_SHADE_BLACK) {
      clip.iLeft = sScreenX + 1;
      clip.iTop = sScreenY;
      clip.iRight = sScreenX + MAP_GRID_X * 2 - 1;
      clip.iBottom = sScreenY + MAP_GRID_Y * 2 - 1;
    } else {
      clip.iLeft = iZoomX + sScreenX - MAP_VIEW_START_X - MAP_GRID_X;
      clip.iTop = iZoomY + sScreenY - MAP_VIEW_START_Y - MAP_GRID_Y;
      clip.iRight = clip.iLeft + MAP_GRID_X * 2;
      clip.iBottom = clip.iTop + MAP_GRID_Y * 2;

      if (sScreenY <= MapScreenRect.iTop + 10) {
        clip.iTop -= 5;
        sScreenY -= 5;
      }

      sScreenX += 1;
      if (sMapX == 1) {
        clip.iLeft -= 5;
        sScreenX -= 5;
      }
    }

    if (sScreenX >= MapScreenRect.iRight - 2 * MAP_GRID_X) ++clip.iRight;
    if (sScreenY >= MapScreenRect.iBottom - 2 * MAP_GRID_X) ++clip.iBottom;

    sScreenX += 1;
    sScreenY += 1;

    if (sScreenX > MapScreenRect.iRight || sScreenY > MapScreenRect.iBottom) return;

    uint16_t *pal;
    switch (iColor) {
      case MAP_SHADE_BLACK:
        // simply shade darker
        guiSAVEBUFFER->ShadowRect(clip.iLeft, clip.iTop, clip.iRight, clip.iBottom);
        return;

      case MAP_SHADE_LT_GREEN:
        pal = pMapLTGreenPalette;
        break;
      case MAP_SHADE_DK_GREEN:
        pal = pMapDKGreenPalette;
        break;
      case MAP_SHADE_LT_RED:
        pal = pMapLTRedPalette;
        break;
      case MAP_SHADE_DK_RED:
        pal = pMapDKRedPalette;
        break;

      default:
        return;
    }

    SGPBox const r = {clip.iLeft, clip.iTop, clip.iRight - clip.iLeft, clip.iBottom - clip.iTop};
    SGPVSurface *const map = guiBIGMAP;
    uint16_t *const org_pal = map->p16BPPPalette;
    map->p16BPPPalette = pal;
    BltVideoSurface(guiSAVEBUFFER, guiBIGMAP, sScreenX, sScreenY, &r);
    map->p16BPPPalette = org_pal;
  }
}

void InitializePalettesForMap() {
  SGP::AutoPtr<SGPVSurfaceAuto> uiTempMap(AddVideoSurfaceFromFile(INTERFACEDIR "/b_map.pcx"));

  SGPPaletteEntry const *const pal = uiTempMap->GetPalette();

  pMapLTRedPalette = Create16BPPPaletteShaded(pal, 400, 0, 0, TRUE);
  pMapDKRedPalette = Create16BPPPaletteShaded(pal, 200, 0, 0, TRUE);
  pMapLTGreenPalette = Create16BPPPaletteShaded(pal, 0, 400, 0, TRUE);
  pMapDKGreenPalette = Create16BPPPaletteShaded(pal, 0, 200, 0, TRUE);
}

void ShutDownPalettesForMap() {
  MemFree(pMapLTRedPalette);
  MemFree(pMapDKRedPalette);
  MemFree(pMapLTGreenPalette);
  MemFree(pMapDKGreenPalette);

  pMapLTRedPalette = NULL;
  pMapDKRedPalette = NULL;
  pMapLTGreenPalette = NULL;
  pMapDKGreenPalette = NULL;
}

static void CopyPathToCharactersSquadIfInOne(SOLDIERTYPE *pCharacter);

void PlotPathForCharacter(SOLDIERTYPE &s, int16_t const x, int16_t const y,
                          bool const tactical_traversal) {
  // Don't build path, if cursor isn't allowed here
  if (!IsTheCursorAllowedToHighLightThisSector(x, y)) return;
  // Leave if the character is in transit
  if (s.bAssignment == IN_TRANSIT) return;

  if (s.bSectorZ != 0) { /* Not on the surface, character won't move until they
                          * reach surface, inform player of this fact */
    wchar_t const *const who =
        s.bAssignment >= ON_DUTY ? s.name : pLongAssignmentStrings[s.bAssignment];
    MapScreenMessage(FONT_MCOLOR_DKRED, MSG_INTERFACE, L"%ls %ls", who, gsUndergroundString);
    return;
  }

  bool const vehicle = s.bAssignment == VEHICLE || s.uiStatusFlags & SOLDIER_VEHICLE;
  if (vehicle) SetUpMvtGroupForVehicle(&s);

  /* Plot a path from current position to x, y: Get last sector in characters
   * list, build new path, remove tail section, and append onto old list */
  int16_t const start = GetLastSectorIdInCharactersPath(&s);
  int16_t const end = x + y * MAP_WORLD_X;
  PathSt *const path = BuildAStrategicPath(start, end, *GetSoldierGroup(s), tactical_traversal);
  s.pMercPath = AppendStrategicPath(path, s.pMercPath);

  if (vehicle) {
    MoveCharactersPathToVehicle(&s);
  } else {
    CopyPathToCharactersSquadIfInOne(&s);
  }
}

void PlotATemporaryPathForCharacter(const SOLDIERTYPE *const pCharacter, const int16_t sX,
                                    const int16_t sY) {
  // clear old temp path
  pTempCharacterPath = ClearStrategicPathList(pTempCharacterPath, -1);

  // is cursor allowed here?..if not..don't build temp path
  if (!IsTheCursorAllowedToHighLightThisSector(sX, sY)) {
    return;
  }

  pTempCharacterPath =
      BuildAStrategicPath(GetLastSectorIdInCharactersPath(pCharacter), sX + sY * MAP_WORLD_X,
                          *GetSoldierGroup(*pCharacter), FALSE);
}

// clear out character path list, after and including this sector
uint32_t ClearPathAfterThisSectorForCharacter(SOLDIERTYPE *pCharacter, int16_t sX, int16_t sY) {
  int32_t iOrigLength = 0;
  VEHICLETYPE *pVehicle = NULL;

  iOrigLength = GetLengthOfMercPath(pCharacter);

  if (!iOrigLength) {
    // no previous path, nothing to do
    return (ABORT_PLOTTING);
  }

  // if we're clearing everything beyond the current sector, that's quite
  // different.  Since we're basically cancelling his movement completely, we
  // must also make sure his next X,Y are changed and he officially "returns" to
  // his sector
  if ((sX == pCharacter->sSectorX) && (sY == pCharacter->sSectorY)) {
    // if we're in confirm map move mode, cancel that (before new UI messages
    // are issued)
    EndConfirmMapMoveMode();

    CancelPathsOfAllSelectedCharacters();
    return (PATH_CLEARED);
  } else  // click not in the current sector
  {
    // if the clicked sector is along current route, this will repath only as
    // far as it.  If not, the entire path will be canceled.

    // if a vehicle
    if (pCharacter->uiStatusFlags & SOLDIER_VEHICLE) {
      pVehicle = &(pVehicleList[pCharacter->bVehicleID]);
    }
    // or in a vehicle
    else if (pCharacter->bAssignment == VEHICLE) {
      pVehicle = &(pVehicleList[pCharacter->iVehicleId]);
    } else {
      // foot soldier
      pCharacter->pMercPath = ClearStrategicPathListAfterThisSector(pCharacter->pMercPath, sX, sY,
                                                                    pCharacter->ubGroupID);
    }

    // if there's an associated vehicle structure
    if (pVehicle != NULL) {
      // do it for the vehicle, too
      pVehicle->pMercPath = ClearStrategicPathListAfterThisSector(pVehicle->pMercPath, sX, sY,
                                                                  pVehicle->ubMovementGroup);
    }

    if (GetLengthOfMercPath(pCharacter) < iOrigLength) {
      CopyPathToAllSelectedCharacters(pCharacter->pMercPath);
      // path WAS actually shortened
      return (PATH_SHORTENED);
    } else {
      // same path as before - it's not any shorter
      return (ABORT_PLOTTING);
    }
  }
}

void CancelPathForCharacter(SOLDIERTYPE *pCharacter) {
  // clear out character's entire path list, he and his squad will stay/return
  // to his current sector.
  pCharacter->pMercPath = ClearStrategicPathList(pCharacter->pMercPath, pCharacter->ubGroupID);
  // NOTE: This automatically calls RemoveGroupWaypoints() internally for valid
  // movement groups

  // This causes the group to effectively reverse directions (even if they've
  // never actually left), so handle that. They are going to return to their
  // current X,Y sector.
  RebuildWayPointsForGroupPath(pCharacter->pMercPath, *GetGroup(pCharacter->ubGroupID));
  //	GroupReversingDirectionsBetweenSectors( GetGroup( pCharacter->ubGroupID
  //), ( uint8_t )( pCharacter->sSectorX ), ( uint8_t )( pCharacter->sSectorY ),
  // FALSE );

  // if he's in a vehicle, clear out the vehicle, too
  if (pCharacter->bAssignment == VEHICLE) {
    CancelPathForVehicle(pVehicleList[pCharacter->iVehicleId], TRUE);
  } else {
    // display "travel route canceled" message
    BeginMapUIMessage(0, pMapPlotStrings[3]);
  }

  CopyPathToCharactersSquadIfInOne(pCharacter);

  fMapPanelDirty = TRUE;
  fTeamPanelDirty = TRUE;
  fCharacterInfoPanelDirty = TRUE;  // to update ETA
}

void CancelPathForVehicle(VEHICLETYPE &v, BOOLEAN const fAlreadyReversed) {
  // we're clearing everything beyond the *current* sector, that's quite
  // different.  Since we're basically cancelling his movement completely, we
  // must also make sure his next X,Y are changed and he officially "returns" to
  // his sector
  v.pMercPath = ClearStrategicPathList(v.pMercPath, v.ubMovementGroup);
  // NOTE: This automatically calls RemoveGroupWaypoints() internally for valid
  // movement groups

  // if we already reversed one of the passengers, flag will be TRUE,
  // don't do it again or we're headed back where we came from!
  if (!fAlreadyReversed) {
    // This causes the group to effectively reverse directions (even if they've
    // never actually left), so handle that. They are going to return to their
    // current X,Y sector.
    RebuildWayPointsForGroupPath(v.pMercPath, *GetGroup(v.ubMovementGroup));
  }

  // display "travel route canceled" message
  BeginMapUIMessage(0, pMapPlotStrings[3]);

  // turn the helicopter flag off here, this prevents the "route aborted" msg
  // from coming up
  fPlotForHelicopter = FALSE;

  fTeamPanelDirty = TRUE;
  fMapPanelDirty = TRUE;
  fCharacterInfoPanelDirty = TRUE;  // to update ETA
}

static void CopyPathToCharactersSquadIfInOne(SOLDIERTYPE *const s) {
  // check if on a squad, if so, do same thing for all characters

  // check to see if character is on a squad, if so, copy path to squad
  if (s->bAssignment < ON_DUTY) {
    CopyPathOfCharacterToSquad(s, s->bAssignment);
  }
}

static void AnimateRoute(PathSt *pPath);
static void TracePathRoute(PathSt *);

void DisplaySoldierPath(SOLDIERTYPE *pCharacter) {
  PathSt *const pPath = GetSoldierMercPathPtr(pCharacter);
  // trace real route
  TracePathRoute(pPath);
  AnimateRoute(pPath);
}

void DisplaySoldierTempPath() {
  // now render temp route
  TracePathRoute(pTempCharacterPath);
}

void DisplayHelicopterPath() {
  // clip to map
  ClipBlitsToMapViewRegion();

  VEHICLETYPE const &v = GetHelicopter();
  // trace both lists..temp is conditional if cursor has sat in same sector grid
  // long enough
  TracePathRoute(v.pMercPath);
  AnimateRoute(v.pMercPath);

  // restore
  RestoreClipRegionToFullScreen();
}

void DisplayHelicopterTempPath() {
  // should we draw temp path?
  if (fDrawTempHeliPath) TracePathRoute(pTempHelicopterPath);
}

void PlotPathForHelicopter(const int16_t sX, const int16_t sY) {
  // will plot the path for the helicopter

  // no heli...go back
  if (!fShowAircraftFlag || iHelicopterVehicleId == -1) return;

  // is cursor allowed here?..if not..don't build path
  if (!IsTheCursorAllowedToHighLightThisSector(sX, sY)) return;

  // set up mvt group for helicopter
  SetUpHelicopterForMovement();

  VEHICLETYPE &v = GetHelicopter();
  // will plot a path from current position to sX, sY
  // get last sector in helicopters list, build new path, remove tail section,
  // move to beginning of list, and append onto old list
  v.pMercPath = AppendStrategicPath(
      BuildAStrategicPath(GetLastSectorOfHelicoptersPath(), (int16_t)(sX + sY * MAP_WORLD_X),
                          *GetGroup(v.ubMovementGroup), FALSE),
      v.pMercPath);

  fMapPanelDirty = TRUE;
}

void PlotATemporaryPathForHelicopter(int16_t sX, int16_t sY) {
  // clear old temp path
  pTempHelicopterPath = ClearStrategicPathList(pTempHelicopterPath, 0);

  // is cursor allowed here?..if not..don't build temp path
  if (!IsTheCursorAllowedToHighLightThisSector(sX, sY)) {
    return;
  }

  // build path
  pTempHelicopterPath = BuildAStrategicPath(GetLastSectorOfHelicoptersPath(), sX + sY * MAP_WORLD_X,
                                            *GetGroup(GetHelicopter().ubMovementGroup), FALSE);
}

// clear out helicopter path list, after and including this sector
uint32_t ClearPathAfterThisSectorForHelicopter(int16_t sX, int16_t sY) {
  int32_t iOrigLength = 0;

  // clear out helicopter path list, after and including this sector
  if (iHelicopterVehicleId == -1 || !CanHelicopterFly()) {
    // abort plotting, shouldn't even be here
    return (ABORT_PLOTTING);
  }

  VEHICLETYPE &v = GetHelicopter();

  iOrigLength = GetLengthOfPath(v.pMercPath);
  if (!iOrigLength) {
    // no previous path, nothing to do, and we didn't shorten it
    return (ABORT_PLOTTING);
  }

  // are we clearing everything beyond the helicopter's CURRENT sector?
  if (sX == v.sSectorX && sY == v.sSectorY) {
    // if we're in confirm map move mode, cancel that (before new UI messages
    // are issued)
    EndConfirmMapMoveMode();

    CancelPathForVehicle(v, FALSE);
    return (PATH_CLEARED);
  } else  // click not in the current sector
  {
    // if the clicked sector is along current route, this will repath only as
    // far as it.  If not, the entire path will be canceled.
    v.pMercPath = ClearStrategicPathListAfterThisSector(v.pMercPath, sX, sY, v.ubMovementGroup);

    if (GetLengthOfPath(v.pMercPath) < iOrigLength) {
      // really shortened!
      return (PATH_SHORTENED);
    } else {
      // same path as before - it's not any shorter
      return (ABORT_PLOTTING);
    }
  }
}

int16_t GetLastSectorOfHelicoptersPath() {
  VEHICLETYPE const &v = GetHelicopter();
  // will return the last sector of the helicopter's current path
  int16_t sLastSector = v.sSectorX + v.sSectorY * MAP_WORLD_X;
  PathSt *pNode = v.pMercPath;

  while (pNode) {
    sLastSector = (int16_t)(pNode->uiSectorId);
    pNode = pNode->pNext;
  }

  return sLastSector;
}

// trace a route for a passed path...doesn't require dest char - most more
// general
static void TracePathRoute(PathSt *const pPath) {
  if (pPath == NULL) return;
  Assert(pPath->pPrev == NULL);

  int32_t iDirection = -1;
  int32_t iArrow = -1;
  const PathSt *prev = NULL;
  const PathSt *next;
  for (const PathSt *node = pPath; node != NULL; prev = node, node = next) {
    next = node->pNext;

    BOOLEAN fUTurnFlag = FALSE;
    int32_t iArrowX;
    int32_t iArrowY;
    int32_t iX;
    int32_t iY;
    if (prev && next) {
      const int32_t iDeltaA = (int16_t)node->uiSectorId - (int16_t)prev->uiSectorId;
      const int32_t iDeltaB = (int16_t)node->uiSectorId - (int16_t)next->uiSectorId;
      if (iDeltaA == 0) return;
      if (!fZoomFlag) {
        iX = node->uiSectorId % MAP_WORLD_X;
        iY = node->uiSectorId / MAP_WORLD_X;
        iX = iX * MAP_GRID_X + MAP_VIEW_START_X;
        iY = iY * MAP_GRID_Y + MAP_VIEW_START_Y;
      } else {
        int16_t sX;
        int16_t sY;
        GetScreenXYFromMapXYStationary(node->uiSectorId % MAP_WORLD_X,
                                       node->uiSectorId / MAP_WORLD_X, &sX, &sY);
        iY = sY - MAP_GRID_Y;
        iX = sX - MAP_GRID_X;
      }
      iArrowX = iX;
      iArrowY = iY;
      if (prev->pPrev && next->pNext) {
        // check to see if out-of sector U-turn
        // for placement of arrows
        const int32_t iDeltaB1 = next->uiSectorId - next->pNext->uiSectorId;
        fUTurnFlag = (iDeltaB1 == -WORLD_MAP_X && iDeltaA == -WORLD_MAP_X && iDeltaB == -1) ||
                     (iDeltaB1 == -WORLD_MAP_X && iDeltaA == -WORLD_MAP_X && iDeltaB == 1) ||
                     (iDeltaB1 == WORLD_MAP_X && iDeltaA == WORLD_MAP_X && iDeltaB == 1) ||
                     (iDeltaB1 == -WORLD_MAP_X && iDeltaA == -WORLD_MAP_X && iDeltaB == 1) ||
                     (iDeltaB1 == -1 && iDeltaA == -1 && iDeltaB == -WORLD_MAP_X) ||
                     (iDeltaB1 == -1 && iDeltaA == -1 && iDeltaB == WORLD_MAP_X) ||
                     (iDeltaB1 == 1 && iDeltaA == 1 && iDeltaB == -WORLD_MAP_X) ||
                     (iDeltaB1 == 1 && iDeltaA == 1 && iDeltaB == WORLD_MAP_X);
      }

      if (prev->uiSectorId == next->uiSectorId) {
        if (prev->uiSectorId + WORLD_MAP_X == node->uiSectorId) {
          if (fZoomFlag) {
            iDirection = S_TO_N_ZOOM_LINE;
            iArrow = ZOOM_NORTH_ARROW;
            iArrowX += NORTH_OFFSET_X * 2;
            iArrowY += NORTH_OFFSET_Y * 2;
          } else {
            iDirection = S_TO_N_LINE;
            iArrow = NORTH_ARROW;
            iArrowX += NORTH_OFFSET_X;
            iArrowY += NORTH_OFFSET_Y;
          }
        } else if (prev->uiSectorId - WORLD_MAP_X == node->uiSectorId) {
          if (fZoomFlag) {
            iDirection = N_TO_S_ZOOM_LINE;
            iArrow = ZOOM_SOUTH_ARROW;
            iArrowX += SOUTH_OFFSET_X * 2;
            iArrowY += SOUTH_OFFSET_Y * 2;
          } else {
            iDirection = N_TO_S_LINE;
            iArrow = SOUTH_ARROW;
            iArrowX += SOUTH_OFFSET_X;
            iArrowY += SOUTH_OFFSET_Y;
          }
        } else if (prev->uiSectorId + 1 == node->uiSectorId) {
          if (fZoomFlag) {
            iDirection = E_TO_W_ZOOM_LINE;
            iArrow = ZOOM_WEST_ARROW;
            iArrowX += WEST_OFFSET_X * 2;
            iArrowY += WEST_OFFSET_Y * 2;
          } else {
            iDirection = E_TO_W_LINE;
            iArrow = WEST_ARROW;
            iArrowX += WEST_OFFSET_X;
            iArrowY += WEST_OFFSET_Y;
          }
        } else {
          if (fZoomFlag) {
            iDirection = W_TO_E_ZOOM_LINE;
            iArrow = ZOOM_EAST_ARROW;
            iArrowX += EAST_OFFSET_X * 2;
            iArrowY += EAST_OFFSET_Y * 2;
          } else {
            iDirection = W_TO_E_LINE;
            iArrow = EAST_ARROW;
            iArrowX += EAST_OFFSET_X;
            iArrowY += EAST_OFFSET_Y;
          }
        }
      } else {
        if (iDeltaA == -1 && iDeltaB == 1) {
          if (fZoomFlag) {
            iDirection = WEST_ZOOM_LINE;
            iArrow = ZOOM_WEST_ARROW;
            iArrowX += WEST_OFFSET_X * 2;
            iArrowY += WEST_OFFSET_Y * 2;
          } else {
            iDirection = WEST_LINE;
            iArrow = WEST_ARROW;
            iArrowX += WEST_OFFSET_X;
            iArrowY += WEST_OFFSET_Y;
          }
        } else if (iDeltaA == 1 && iDeltaB == -1) {
          if (fZoomFlag) {
            iDirection = EAST_ZOOM_LINE;
            iArrow = ZOOM_EAST_ARROW;
            iArrowX += EAST_OFFSET_X * 2;
            iArrowY += EAST_OFFSET_Y * 2;
          } else {
            iDirection = EAST_LINE;
            iArrow = EAST_ARROW;
            iArrowX += EAST_OFFSET_X;
            iArrowY += EAST_OFFSET_Y;
          }
        } else if (iDeltaA == -WORLD_MAP_X && iDeltaB == WORLD_MAP_X) {
          if (fZoomFlag) {
            iDirection = NORTH_ZOOM_LINE;
            iArrow = ZOOM_NORTH_ARROW;
            iArrowX += NORTH_OFFSET_X * 2;
            iArrowY += NORTH_OFFSET_Y * 2;
          } else {
            iDirection = NORTH_LINE;
            iArrow = NORTH_ARROW;
            iArrowX += NORTH_OFFSET_X;
            iArrowY += NORTH_OFFSET_Y;
          }
        } else if (iDeltaA == WORLD_MAP_X && iDeltaB == -WORLD_MAP_X) {
          if (fZoomFlag) {
            iDirection = SOUTH_ZOOM_LINE;
            iArrow = ZOOM_SOUTH_ARROW;
            iArrowX += SOUTH_OFFSET_X * 2;
            iArrowY += SOUTH_OFFSET_Y * 2;
          } else {
            iDirection = SOUTH_LINE;
            iArrow = SOUTH_ARROW;
            iArrowX += SOUTH_OFFSET_X;
            iArrowY += SOUTH_OFFSET_Y;
          }
        } else if (iDeltaA == -WORLD_MAP_X && iDeltaB == -1) {
          if (fZoomFlag) {
            iDirection = N_TO_E_ZOOM_LINE;
            iArrow = ZOOM_EAST_ARROW;
            iArrowX += EAST_OFFSET_X * 2;
            iArrowY += EAST_OFFSET_Y * 2;
          } else {
            iDirection = N_TO_E_LINE;
            iArrow = EAST_ARROW;
            iArrowX += EAST_OFFSET_X;
            iArrowY += EAST_OFFSET_Y;
          }
        } else if (iDeltaA == WORLD_MAP_X && iDeltaB == 1) {
          if (fZoomFlag) {
            iDirection = S_TO_W_ZOOM_LINE;
            iArrow = ZOOM_WEST_ARROW;
            iArrowX += WEST_OFFSET_X * 2;
            iArrowY += WEST_OFFSET_Y * 2;
          } else {
            iDirection = S_TO_W_LINE;
            iArrow = WEST_ARROW;
            iArrowX += WEST_OFFSET_X;
            iArrowY += WEST_OFFSET_Y;
          }
        } else if (iDeltaA == 1 && iDeltaB == -WORLD_MAP_X) {
          if (fZoomFlag) {
            iDirection = E_TO_S_ZOOM_LINE;
            iArrow = ZOOM_SOUTH_ARROW;
            iArrowX += SOUTH_OFFSET_X * 2;
            iArrowY += SOUTH_OFFSET_Y * 2;
          } else {
            iDirection = E_TO_S_LINE;
            iArrow = SOUTH_ARROW;
            iArrowX += SOUTH_OFFSET_X;
            iArrowY += SOUTH_OFFSET_Y;
          }
        } else if (iDeltaA == -1 && iDeltaB == WORLD_MAP_X) {
          if (fZoomFlag) {
            iDirection = W_TO_N_ZOOM_LINE;
            iArrow = ZOOM_NORTH_ARROW;
            iArrowX += NORTH_OFFSET_X * 2;
            iArrowY += NORTH_OFFSET_Y * 2;
          } else {
            iDirection = W_TO_N_LINE;
            iArrow = NORTH_ARROW;
            iArrowX += NORTH_OFFSET_X;
            iArrowY += NORTH_OFFSET_Y;
          }
        } else if (iDeltaA == -1 && iDeltaB == -WORLD_MAP_X) {
          if (fZoomFlag) {
            iDirection = W_TO_S_ZOOM_LINE;
            iArrow = ZOOM_SOUTH_ARROW;
            iArrowX += SOUTH_OFFSET_X * 2;
            iArrowY += (SOUTH_OFFSET_Y + WEST_TO_SOUTH_OFFSET_Y) * 2;
          } else {
            iDirection = W_TO_S_LINE;
            iArrow = SOUTH_ARROW;
            iArrowX += SOUTH_OFFSET_X;
            iArrowY += SOUTH_OFFSET_Y + WEST_TO_SOUTH_OFFSET_Y;
          }
        } else if (iDeltaA == -WORLD_MAP_X && iDeltaB == 1) {
          if (fZoomFlag) {
            iDirection = N_TO_W_ZOOM_LINE;
            iArrow = ZOOM_WEST_ARROW;
            iArrowX += WEST_OFFSET_X * 2;
            iArrowY += WEST_OFFSET_Y * 2;
          } else {
            iDirection = N_TO_W_LINE;
            iArrow = WEST_ARROW;
            iArrowX += WEST_OFFSET_X;
            iArrowY += WEST_OFFSET_Y;
          }
        } else if (iDeltaA == WORLD_MAP_X && iDeltaB == -1) {
          if (fZoomFlag) {
            iDirection = S_TO_E_ZOOM_LINE;
            iArrow = ZOOM_EAST_ARROW;
            iArrowX += EAST_OFFSET_X * 2;
            iArrowY += EAST_OFFSET_Y * 2;
          } else {
            iDirection = S_TO_E_LINE;
            iArrow = EAST_ARROW;
            iArrowX += EAST_OFFSET_X;
            iArrowY += EAST_OFFSET_Y;
          }
        } else if (iDeltaA == 1 && iDeltaB == WORLD_MAP_X) {
          if (fZoomFlag) {
            iDirection = E_TO_N_ZOOM_LINE;
            iArrow = ZOOM_NORTH_ARROW;
            iArrowX += NORTH_OFFSET_X * 2;
            iArrowY += (NORTH_OFFSET_Y + EAST_TO_NORTH_OFFSET_Y) * 2;
          } else {
            iDirection = E_TO_N_LINE;
            iArrow = NORTH_ARROW;
            iArrowX += NORTH_OFFSET_X;
            iArrowY += NORTH_OFFSET_Y + EAST_TO_NORTH_OFFSET_Y;
          }
        }
      }
    } else {
      if (!fZoomFlag) {
        iX = node->uiSectorId % MAP_WORLD_X;
        iY = node->uiSectorId / MAP_WORLD_X;
        iX = iX * MAP_GRID_X + MAP_VIEW_START_X;
        iY = iY * MAP_GRID_Y + MAP_VIEW_START_Y;
      } else {
        int16_t sX;
        int16_t sY;
        GetScreenXYFromMapXYStationary(node->uiSectorId % MAP_WORLD_X,
                                       node->uiSectorId / MAP_WORLD_X, &sX, &sY);
        iY = sY - MAP_GRID_Y;
        iX = sX - MAP_GRID_X;
      }
      iArrowX = iX;
      iArrowY = iY;
      // display enter and exit 'X's
      if (prev) {
        fUTurnFlag = TRUE;
        const int32_t iDeltaA = (int16_t)node->uiSectorId - (int16_t)prev->uiSectorId;
        if (iDeltaA == -1) {
          iDirection = fZoomFlag ? ZOOM_RED_X_WEST : RED_X_WEST;
        } else if (iDeltaA == 1) {
          iDirection = fZoomFlag ? ZOOM_RED_X_EAST : RED_X_EAST;
        } else if (iDeltaA == -WORLD_MAP_X) {
          iDirection = fZoomFlag ? ZOOM_RED_X_NORTH : RED_X_NORTH;
        } else {
          iDirection = fZoomFlag ? ZOOM_RED_X_SOUTH : RED_X_SOUTH;
        }
      }
      if (next) {
        fUTurnFlag = FALSE;
        const int32_t iDeltaB = (int16_t)node->uiSectorId - (int16_t)next->uiSectorId;
        if (iDeltaB == -1) {
          if (fZoomFlag) {
            iDirection = ZOOM_GREEN_X_EAST;
            iArrow = ZOOM_EAST_ARROW;
            iArrowX += EAST_OFFSET_X * 2;
            iArrowY += EAST_OFFSET_Y * 2;
          } else {
            iDirection = GREEN_X_EAST;
            iArrow = EAST_ARROW;
            iArrowX += EAST_OFFSET_X;
            iArrowY += EAST_OFFSET_Y;
          }
        } else if (iDeltaB == 1) {
          if (fZoomFlag) {
            iDirection = ZOOM_GREEN_X_WEST;
            iArrow = ZOOM_WEST_ARROW;
            iArrowX += WEST_OFFSET_X * 2;
            iArrowY += WEST_OFFSET_Y * 2;
          } else {
            iDirection = GREEN_X_WEST;
            iArrow = WEST_ARROW;
            iArrowX += WEST_OFFSET_X;
            iArrowY += WEST_OFFSET_Y;
          }
        } else if (iDeltaB == WORLD_MAP_X) {
          if (fZoomFlag) {
            iDirection = ZOOM_GREEN_X_NORTH;
            iArrow = ZOOM_NORTH_ARROW;
            iArrowX += NORTH_OFFSET_X * 2;
            iArrowY += NORTH_OFFSET_Y * 2;
          } else {
            iDirection = GREEN_X_NORTH;
            iArrow = NORTH_ARROW;
            iArrowX += NORTH_OFFSET_X;
            iArrowY += NORTH_OFFSET_Y;
          }
        } else {
          if (fZoomFlag) {
            iDirection = ZOOM_GREEN_X_SOUTH;
            iArrow = ZOOM_SOUTH_ARROW;
            iArrowX += SOUTH_OFFSET_X * 2;
            iArrowY += SOUTH_OFFSET_Y * 2;
          } else {
            iDirection = GREEN_X_SOUTH;
            iArrow = SOUTH_ARROW;
            iArrowX += SOUTH_OFFSET_X;
            iArrowY += SOUTH_OFFSET_Y;
          }
        }
      }
    }

    if (iDirection == -1) continue;

    if (!fZoomFlag || (MAP_VIEW_START_X < iX && iX < SCREEN_WIDTH - MAP_GRID_X * 2 &&
                       MAP_VIEW_START_Y < iY && iY < MAP_VIEW_START_Y + MAP_VIEW_HEIGHT)) {
      BltVideoObject(FRAME_BUFFER, guiMAPCURSORS, iDirection, iX, iY);

      if (!fUTurnFlag) {
        BltVideoObject(FRAME_BUFFER, guiMAPCURSORS, (uint16_t)iArrow, iArrowX, iArrowY);
        InvalidateRegion(iArrowX, iArrowY, iArrowX + 2 * MAP_GRID_X, iArrowY + 2 * MAP_GRID_Y);
      }

      InvalidateRegion(iX, iY, iX + 2 * MAP_GRID_X, iY + 2 * MAP_GRID_Y);
    }
  }
}

static BOOLEAN TraceCharAnimatedRoute(PathSt *pPath, BOOLEAN fForceUpDate);

static void AnimateRoute(PathSt *pPath) {
  SetFontDestBuffer(FRAME_BUFFER);

  // the animated path
  if (TraceCharAnimatedRoute(pPath, FALSE)) {
    // ARM? Huh?  Why the same thing twice more?
    TraceCharAnimatedRoute(pPath, TRUE);
    TraceCharAnimatedRoute(pPath, TRUE);
  }
}

static BOOLEAN TraceCharAnimatedRoute(PathSt *const pPath, const BOOLEAN fForceUpDate) {
  static PathSt *pCurrentNode = NULL;
  static BOOLEAN fUpDateFlag = FALSE;
  static BOOLEAN fPauseFlag = TRUE;
  static uint8_t ubCounter = 1;

  int32_t iDifference = 0;
  int32_t iArrow = -1;
  int32_t iX = 0, iY = 0;
  int32_t iPastX, iPastY;
  int16_t sX = 0, sY = 0;
  int32_t iArrowX, iArrowY;
  int32_t iDeltaA, iDeltaB, iDeltaB1;
  int32_t iDirection = -1;
  BOOLEAN fUTurnFlag = FALSE;
  BOOLEAN fNextNode = FALSE;
  PathSt *pTempNode = NULL;
  PathSt *pNode = NULL;
  PathSt *pPastNode = NULL;
  PathSt *pNextNode = NULL;

  // must be plotting movement
  if (bSelectedDestChar == -1 && !fPlotForHelicopter) {
    return FALSE;
  }

  // if any nodes have been deleted, reset current node to beginning of the list
  if (fDeletedNode) {
    fDeletedNode = FALSE;
    pCurrentNode = NULL;
  }

  // Valid path?
  if (pPath == NULL) {
    return FALSE;
  } else {
    if (pCurrentNode == NULL) {
      pCurrentNode = pPath;
    }
  }

  // Check Timer
  if (giAnimateRouteBaseTime == 0) {
    giAnimateRouteBaseTime = GetJA2Clock();
    return FALSE;
  }

  // check difference in time
  iDifference = GetJA2Clock() - giAnimateRouteBaseTime;

  // if pause flag, check time, if time passed, reset, continue on, else return
  if (fPauseFlag) {
    if (iDifference < PAUSE_DELAY) {
      return FALSE;
    } else {
      fPauseFlag = FALSE;
      giAnimateRouteBaseTime = GetJA2Clock();
    }
  }

  // if is checkflag and change in status, return TRUE;
  if (!fForceUpDate) {
    if (iDifference < ARROW_DELAY) {
      if (!fUpDateFlag) return FALSE;
    } else {
      // sufficient time, update base time
      giAnimateRouteBaseTime = GetJA2Clock();
      fUpDateFlag = !fUpDateFlag;

      fNextNode = TRUE;
    }
  }

  // check to see if Current node has not been deleted
  pTempNode = pPath;

  while (pTempNode) {
    if (pTempNode == pCurrentNode) {
      // not deleted
      // reset pause flag
      break;
    } else
      pTempNode = pTempNode->pNext;
  }

  // if deleted, restart at beginnning
  if (pTempNode == NULL) {
    pCurrentNode = pPath;

    // set pause flag
    if (!pCurrentNode) return FALSE;
  }

  // Handle drawing of arrow
  pNode = pCurrentNode;
  if ((!pNode->pPrev) && (ubCounter == 1) && (fForceUpDate)) {
    ubCounter = 0;
    return FALSE;
  } else if ((ubCounter == 1) && (fForceUpDate)) {
    pNode = pCurrentNode->pPrev;
  }
  if (pNode->pNext)
    pNextNode = pNode->pNext;
  else
    pNextNode = NULL;

  if (pNode->pPrev)
    pPastNode = pNode->pPrev;
  else
    pPastNode = NULL;

  // go through characters list and display arrows for path
  fUTurnFlag = FALSE;
  if ((pPastNode) && (pNextNode)) {
    iDeltaA = (int16_t)pNode->uiSectorId - (int16_t)pPastNode->uiSectorId;
    iDeltaB = (int16_t)pNode->uiSectorId - (int16_t)pNextNode->uiSectorId;
    if (iDeltaA == 0) return FALSE;
    if (!fZoomFlag) {
      iX = (pNode->uiSectorId % MAP_WORLD_X);
      iY = (pNode->uiSectorId / MAP_WORLD_X);
      iX = (iX * MAP_GRID_X) + MAP_VIEW_START_X;
      iY = (iY * MAP_GRID_Y) + MAP_VIEW_START_Y;
    } else {
      GetScreenXYFromMapXYStationary(((uint16_t)(pNode->uiSectorId % MAP_WORLD_X)),
                                     ((uint16_t)(pNode->uiSectorId / MAP_WORLD_X)), &sX, &sY);
      iY = sY - MAP_GRID_Y;
      iX = sX - MAP_GRID_X;
    }
    iArrowX = iX;
    iArrowY = iY;
    if ((pPastNode->pPrev) && (pNextNode->pNext)) {
      fUTurnFlag = FALSE;
      // check to see if out-of sector U-turn
      // for placement of arrows
      iDeltaB1 = pNextNode->uiSectorId - pNextNode->pNext->uiSectorId;
      if ((iDeltaB1 == -WORLD_MAP_X) && (iDeltaA == -WORLD_MAP_X) && (iDeltaB == -1)) {
        fUTurnFlag = TRUE;
      } else if ((iDeltaB1 == -WORLD_MAP_X) && (iDeltaA == -WORLD_MAP_X) && (iDeltaB == 1)) {
        fUTurnFlag = TRUE;
      } else if ((iDeltaB1 == WORLD_MAP_X) && (iDeltaA == WORLD_MAP_X) && (iDeltaB == 1)) {
        fUTurnFlag = TRUE;
      } else if ((iDeltaB1 == -WORLD_MAP_X) && (iDeltaA == -WORLD_MAP_X) && (iDeltaB == 1)) {
        fUTurnFlag = TRUE;
      } else if ((iDeltaB1 == -1) && (iDeltaA == -1) && (iDeltaB == -WORLD_MAP_X)) {
        fUTurnFlag = TRUE;
      } else if ((iDeltaB1 == -1) && (iDeltaA == -1) && (iDeltaB == WORLD_MAP_X)) {
        fUTurnFlag = TRUE;
      } else if ((iDeltaB1 == 1) && (iDeltaA == 1) && (iDeltaB == -WORLD_MAP_X)) {
        fUTurnFlag = TRUE;
      } else if ((iDeltaB1 == 1) && (iDeltaA == 1) && (iDeltaB == WORLD_MAP_X)) {
        fUTurnFlag = TRUE;
      } else
        fUTurnFlag = FALSE;
    }

    if (pPastNode->uiSectorId == pNextNode->uiSectorId) {
      if (pPastNode->uiSectorId + WORLD_MAP_X == pNode->uiSectorId) {
        if (fZoomFlag) {
          iDirection = S_TO_N_ZOOM_LINE;
          if (!ubCounter)
            iArrow = ZOOM_W_NORTH_ARROW;
          else
            iArrow = ZOOM_NORTH_ARROW;
          iArrowX += NORTH_OFFSET_X * 2;
          iArrowY += NORTH_OFFSET_Y * 2;
        } else {
          iDirection = S_TO_N_LINE;
          if (!ubCounter)
            iArrow = W_NORTH_ARROW;
          else
            iArrow = NORTH_ARROW;

          iArrowX += NORTH_OFFSET_X;
          iArrowY += NORTH_OFFSET_Y;
        }
      } else if (pPastNode->uiSectorId - WORLD_MAP_X == pNode->uiSectorId) {
        if (fZoomFlag) {
          iDirection = N_TO_S_ZOOM_LINE;
          if (!ubCounter)
            iArrow = ZOOM_W_SOUTH_ARROW;
          else
            iArrow = ZOOM_SOUTH_ARROW;
          iArrowX += SOUTH_OFFSET_X * 2;
          iArrowY += SOUTH_OFFSET_Y * 2;
        } else {
          iDirection = N_TO_S_LINE;
          if (!ubCounter)
            iArrow = W_SOUTH_ARROW;
          else
            iArrow = SOUTH_ARROW;
          iArrowX += SOUTH_OFFSET_X;
          iArrowY += SOUTH_OFFSET_Y;
        }
      } else if (pPastNode->uiSectorId + 1 == pNode->uiSectorId) {
        if (fZoomFlag) {
          iDirection = E_TO_W_ZOOM_LINE;
          if (!ubCounter)
            iArrow = ZOOM_W_WEST_ARROW;
          else
            iArrow = ZOOM_WEST_ARROW;
          iArrowX += WEST_OFFSET_X * 2;
          iArrowY += WEST_OFFSET_Y * 2;
        } else {
          iDirection = E_TO_W_LINE;
          if (!ubCounter)
            iArrow = W_WEST_ARROW;
          else
            iArrow = WEST_ARROW;
          iArrowX += WEST_OFFSET_X;
          iArrowY += WEST_OFFSET_Y;
        }
      } else {
        if (fZoomFlag) {
          iDirection = W_TO_E_ZOOM_LINE;
          if (!ubCounter)
            iArrow = ZOOM_W_EAST_ARROW;
          else
            iArrow = ZOOM_EAST_ARROW;
          iArrowX += EAST_OFFSET_X * 2;
          iArrowY += EAST_OFFSET_Y * 2;
        } else {
          iDirection = W_TO_E_LINE;
          if (!ubCounter)
            iArrow = W_EAST_ARROW;
          else
            iArrow = EAST_ARROW;
          iArrowX += EAST_OFFSET_X;
          iArrowY += EAST_OFFSET_Y;
        }
      }
    } else {
      if ((iDeltaA == -1) && (iDeltaB == 1)) {
        if (fZoomFlag) {
          iDirection = WEST_ZOOM_LINE;
          if (!ubCounter)
            iArrow = ZOOM_W_WEST_ARROW;
          else
            iArrow = ZOOM_WEST_ARROW;

          iArrowX += WEST_OFFSET_X * 2;
          iArrowY += WEST_OFFSET_Y * 2;
        } else {
          iDirection = WEST_LINE;
          if (!ubCounter)
            iArrow = W_WEST_ARROW;
          else
            iArrow = WEST_ARROW;

          iArrowX += WEST_OFFSET_X;
          iArrowY += WEST_OFFSET_Y;
        }
      } else if ((iDeltaA == 1) && (iDeltaB == -1)) {
        if (fZoomFlag) {
          iDirection = EAST_ZOOM_LINE;
          if (!ubCounter)
            iArrow = ZOOM_W_EAST_ARROW;
          else
            iArrow = ZOOM_EAST_ARROW;

          iArrowX += EAST_OFFSET_X * 2;
          iArrowY += EAST_OFFSET_Y * 2;
        } else {
          iDirection = EAST_LINE;
          if (!ubCounter)
            iArrow = W_EAST_ARROW;
          else
            iArrow = EAST_ARROW;

          iArrowX += EAST_OFFSET_X;
          iArrowY += EAST_OFFSET_Y;
        }
      } else if ((iDeltaA == -WORLD_MAP_X) && (iDeltaB == WORLD_MAP_X)) {
        if (fZoomFlag) {
          iDirection = NORTH_ZOOM_LINE;
          if (!ubCounter)
            iArrow = ZOOM_W_NORTH_ARROW;
          else
            iArrow = ZOOM_NORTH_ARROW;

          iArrowX += NORTH_OFFSET_X * 2;
          iArrowY += NORTH_OFFSET_Y * 2;
        } else {
          iDirection = NORTH_LINE;
          if (!ubCounter)
            iArrow = W_NORTH_ARROW;
          else
            iArrow = NORTH_ARROW;

          iArrowX += NORTH_OFFSET_X;
          iArrowY += NORTH_OFFSET_Y;
        }
      } else if ((iDeltaA == WORLD_MAP_X) && (iDeltaB == -WORLD_MAP_X)) {
        if (fZoomFlag) {
          iDirection = SOUTH_ZOOM_LINE;
          if (!ubCounter)
            iArrow = ZOOM_W_SOUTH_ARROW;
          else
            iArrow = ZOOM_SOUTH_ARROW;

          iArrowX += SOUTH_OFFSET_X * 2;
          iArrowY += SOUTH_OFFSET_Y * 2;
        } else {
          iDirection = SOUTH_LINE;
          if (!ubCounter)
            iArrow = W_SOUTH_ARROW;
          else
            iArrow = SOUTH_ARROW;

          iArrowX += SOUTH_OFFSET_X;
          iArrowY += SOUTH_OFFSET_Y;
        }
      } else if ((iDeltaA == -WORLD_MAP_X) && (iDeltaB == -1)) {
        if (fZoomFlag) {
          iDirection = N_TO_E_ZOOM_LINE;
          if (!ubCounter)
            iArrow = ZOOM_W_EAST_ARROW;
          else
            iArrow = ZOOM_EAST_ARROW;

          iArrowX += EAST_OFFSET_X * 2;
          iArrowY += EAST_OFFSET_Y * 2;
        } else {
          iDirection = N_TO_E_LINE;
          if (!ubCounter)
            iArrow = W_EAST_ARROW;
          else
            iArrow = EAST_ARROW;

          iArrowX += EAST_OFFSET_X;
          iArrowY += EAST_OFFSET_Y;
        }
      } else if ((iDeltaA == WORLD_MAP_X) && (iDeltaB == 1)) {
        if (fZoomFlag) {
          iDirection = S_TO_W_ZOOM_LINE;
          if (!ubCounter)
            iArrow = ZOOM_W_WEST_ARROW;
          else
            iArrow = ZOOM_WEST_ARROW;

          iArrowX += WEST_OFFSET_X * 2;
          iArrowY += WEST_OFFSET_Y * 2;
        } else {
          iDirection = S_TO_W_LINE;
          if (!ubCounter)
            iArrow = W_WEST_ARROW;
          else
            iArrow = WEST_ARROW;

          iArrowX += WEST_OFFSET_X;
          iArrowY += WEST_OFFSET_Y;
        }
      } else if ((iDeltaA == 1) && (iDeltaB == -WORLD_MAP_X)) {
        if (fZoomFlag) {
          iDirection = E_TO_S_ZOOM_LINE;
          if (!ubCounter)
            iArrow = ZOOM_W_SOUTH_ARROW;
          else
            iArrow = ZOOM_SOUTH_ARROW;

          iArrowX += SOUTH_OFFSET_X * 2;
          iArrowY += SOUTH_OFFSET_Y * 2;
        } else {
          iDirection = E_TO_S_LINE;
          if (!ubCounter)
            iArrow = W_SOUTH_ARROW;
          else
            iArrow = SOUTH_ARROW;

          iArrowX += SOUTH_OFFSET_X;
          iArrowY += SOUTH_OFFSET_Y;
        }
      } else if ((iDeltaA == -1) && (iDeltaB == WORLD_MAP_X)) {
        if (fZoomFlag) {
          iDirection = W_TO_N_ZOOM_LINE;
          if (!ubCounter)
            iArrow = ZOOM_W_NORTH_ARROW;
          else
            iArrow = ZOOM_NORTH_ARROW;

          iArrowX += NORTH_OFFSET_X * 2;
          iArrowY += NORTH_OFFSET_Y * 2;
        } else {
          iDirection = W_TO_N_LINE;
          if (!ubCounter)
            iArrow = W_NORTH_ARROW;
          else
            iArrow = NORTH_ARROW;

          iArrowX += NORTH_OFFSET_X;
          iArrowY += NORTH_OFFSET_Y;
        }
      } else if ((iDeltaA == -1) && (iDeltaB == -WORLD_MAP_X)) {
        if (fZoomFlag) {
          iDirection = W_TO_S_ZOOM_LINE;
          if (!ubCounter)
            iArrow = ZOOM_W_SOUTH_ARROW;
          else
            iArrow = ZOOM_SOUTH_ARROW;

          iArrowX += SOUTH_OFFSET_X * 2;
          iArrowY += (SOUTH_OFFSET_Y + WEST_TO_SOUTH_OFFSET_Y) * 2;
        } else {
          iDirection = W_TO_S_LINE;
          if (!ubCounter)
            iArrow = W_SOUTH_ARROW;
          else
            iArrow = SOUTH_ARROW;
          iArrowX += SOUTH_OFFSET_X;
          iArrowY += (SOUTH_OFFSET_Y + WEST_TO_SOUTH_OFFSET_Y);
        }
      } else if ((iDeltaA == -WORLD_MAP_X) && (iDeltaB == 1)) {
        if (fZoomFlag) {
          iDirection = N_TO_W_ZOOM_LINE;
          if (!ubCounter)
            iArrow = ZOOM_W_WEST_ARROW;
          else
            iArrow = ZOOM_WEST_ARROW;

          iArrowX += WEST_OFFSET_X * 2;
          iArrowY += WEST_OFFSET_Y * 2;
        } else {
          iDirection = N_TO_W_LINE;
          if (!ubCounter)
            iArrow = W_WEST_ARROW;
          else
            iArrow = WEST_ARROW;

          iArrowX += WEST_OFFSET_X;
          iArrowY += WEST_OFFSET_Y;
        }
      } else if ((iDeltaA == WORLD_MAP_X) && (iDeltaB == -1)) {
        if (fZoomFlag) {
          iDirection = S_TO_E_ZOOM_LINE;
          if (!ubCounter)
            iArrow = ZOOM_W_EAST_ARROW;
          else
            iArrow = ZOOM_EAST_ARROW;
          iArrowX += EAST_OFFSET_X * 2;
          iArrowY += EAST_OFFSET_Y * 2;
        } else {
          iDirection = S_TO_E_LINE;
          if (!ubCounter)
            iArrow = W_EAST_ARROW;
          else
            iArrow = EAST_ARROW;
          iArrowX += EAST_OFFSET_X;
          iArrowY += EAST_OFFSET_Y;
        }
      } else if ((iDeltaA == 1) && (iDeltaB == WORLD_MAP_X)) {
        if (fZoomFlag) {
          iDirection = E_TO_N_ZOOM_LINE;
          if (!ubCounter)
            iArrow = ZOOM_W_NORTH_ARROW;
          else
            iArrow = ZOOM_NORTH_ARROW;
          iArrowX += (NORTH_OFFSET_X * 2);
          iArrowY += (NORTH_OFFSET_Y + EAST_TO_NORTH_OFFSET_Y) * 2;
        } else {
          iDirection = E_TO_N_LINE;
          if (!ubCounter)
            iArrow = W_NORTH_ARROW;
          else
            iArrow = NORTH_ARROW;

          iArrowX += NORTH_OFFSET_X;
          iArrowY += NORTH_OFFSET_Y + EAST_TO_NORTH_OFFSET_Y;
        }
      }
    }

  }

  else {
    iX = (pNode->uiSectorId % MAP_WORLD_X);
    iY = (pNode->uiSectorId / MAP_WORLD_X);
    iX = (iX * MAP_GRID_X) + MAP_VIEW_START_X;
    iY = (iY * MAP_GRID_Y) + MAP_VIEW_START_Y;
    if (pPastNode) {
      iPastX = (pPastNode->uiSectorId % MAP_WORLD_X);
      iPastY = (pPastNode->uiSectorId / MAP_WORLD_X);
      iPastX = (iPastX * MAP_GRID_X) + MAP_VIEW_START_X;
      iPastY = (iPastY * MAP_GRID_Y) + MAP_VIEW_START_Y;
    }
    iArrowX = iX;
    iArrowY = iY;
    // display enter and exit 'X's
    if (pPastNode) {
      // red 'X'
      fUTurnFlag = TRUE;
      iDeltaA = (int16_t)pNode->uiSectorId - (int16_t)pPastNode->uiSectorId;
      if (iDeltaA == -1) {
        iDirection = RED_X_WEST;
        // iX+=RED_WEST_OFF_X;
      } else if (iDeltaA == 1) {
        iDirection = RED_X_EAST;
        // iX+=RED_EAST_OFF_X;
      } else if (iDeltaA == -WORLD_MAP_X) {
        iDirection = RED_X_NORTH;
        // iY+=RED_NORTH_OFF_Y;
      } else {
        iDirection = RED_X_SOUTH;
        //	iY+=RED_SOUTH_OFF_Y;
      }
    }
    if (pNextNode) {
      fUTurnFlag = FALSE;
      iDeltaB = (int16_t)pNode->uiSectorId - (int16_t)pNextNode->uiSectorId;
      if (iDeltaB == -1) {
        iDirection = GREEN_X_EAST;
        if (!ubCounter)
          iArrow = W_EAST_ARROW;
        else
          iArrow = EAST_ARROW;

        iArrowX += EAST_OFFSET_X;
        iArrowY += EAST_OFFSET_Y;
        // iX+=RED_EAST_OFF_X;
      } else if (iDeltaB == 1) {
        iDirection = GREEN_X_WEST;
        if (!ubCounter)
          iArrow = W_WEST_ARROW;
        else
          iArrow = WEST_ARROW;

        iArrowX += WEST_OFFSET_X;
        iArrowY += WEST_OFFSET_Y;
        // iX+=RED_WEST_OFF_X;
      } else if (iDeltaB == WORLD_MAP_X) {
        iDirection = GREEN_X_NORTH;
        if (!ubCounter)
          iArrow = W_NORTH_ARROW;
        else
          iArrow = NORTH_ARROW;

        iArrowX += NORTH_OFFSET_X;
        iArrowY += NORTH_OFFSET_Y;
        // iY+=RED_NORTH_OFF_Y;
      } else {
        iDirection = GREEN_X_SOUTH;
        if (!ubCounter)
          iArrow = W_SOUTH_ARROW;
        else
          iArrow = SOUTH_ARROW;
        iArrowX += SOUTH_OFFSET_X;
        iArrowY += SOUTH_OFFSET_Y;
        // iY+=RED_SOUTH_OFF_Y;
      }
    }
  }
  if (fNextNode) {
    if (!ubCounter) {
      pCurrentNode = pCurrentNode->pNext;
      if (!pCurrentNode) fPauseFlag = TRUE;
    }
  }
  if ((iDirection != -1) && (iArrow != -1)) {
    if (!fUTurnFlag) {
      if (!fZoomFlag || (MAP_VIEW_START_X < iX && iX < SCREEN_WIDTH - MAP_GRID_X * 2 &&
                         MAP_VIEW_START_Y < iY && iY < MAP_VIEW_START_Y + MAP_VIEW_HEIGHT)) {
        // if(!fZoomFlag)
        // RestoreExternBackgroundRect(((int16_t)iArrowX),((int16_t)iArrowY),DMAP_GRID_X,
        // DMAP_GRID_Y); else
        // RestoreExternBackgroundRect(((int16_t)iArrowX),
        // ((int16_t)iArrowY),DMAP_GRID_ZOOM_X, DMAP_GRID_ZOOM_Y);
        if (pNode != pPath) {
          BltVideoObject(FRAME_BUFFER, guiMAPCURSORS, (uint16_t)iArrow, iArrowX, iArrowY);
          InvalidateRegion(iArrowX, iArrowY, iArrowX + 2 * MAP_GRID_X, iArrowY + 2 * MAP_GRID_Y);
        }
      }
      if (ubCounter == 1)
        ubCounter = 0;
      else
        ubCounter = 1;
      return TRUE;
    }
    if (ubCounter == 1)
      ubCounter = 0;
    else
      ubCounter = 1;
  }
  // move to next arrow

  // ARM who knows what it should return here?
  return FALSE;
}

// display potential path, yes or no?
void DisplayThePotentialPathForHelicopter(int16_t sMapX, int16_t sMapY) {
  // simply check if we want to refresh the screen to display path
  static BOOLEAN fOldShowAirCraft = FALSE;
  static int16_t sOldMapX, sOldMapY;
  int32_t iDifference = 0;

  if (fOldShowAirCraft != fShowAircraftFlag) {
    fOldShowAirCraft = fShowAircraftFlag;
    giPotHeliPathBaseTime = GetJA2Clock();

    sOldMapX = sMapX;
    sOldMapY = sMapY;
    fTempPathAlreadyDrawn = FALSE;
    fDrawTempHeliPath = FALSE;
  }

  if ((sMapX != sOldMapX) || (sMapY != sOldMapY)) {
    giPotHeliPathBaseTime = GetJA2Clock();

    sOldMapX = sMapX;
    sOldMapY = sMapY;

    // path was plotted and we moved, re draw map..to clean up mess
    if (fTempPathAlreadyDrawn) {
      fMapPanelDirty = TRUE;
    }

    fTempPathAlreadyDrawn = FALSE;
    fDrawTempHeliPath = FALSE;
  }

  iDifference = GetJA2Clock() - giPotHeliPathBaseTime;

  if (fTempPathAlreadyDrawn) {
    return;
  }

  if (iDifference > MIN_WAIT_TIME_FOR_TEMP_PATH) {
    fDrawTempHeliPath = TRUE;
    giPotHeliPathBaseTime = GetJA2Clock();
    fTempPathAlreadyDrawn = TRUE;
  }
}

bool IsTheCursorAllowedToHighLightThisSector(int16_t const x, int16_t const y) {
  return SectorInfo[SECTOR(x, y)].ubTraversability[THROUGH_STRATEGIC_MOVE] != EDGEOFWORLD;
}

void RestoreBackgroundForMapGrid(int16_t sMapX, int16_t sMapY) {
  int16_t sX, sY;

  if (!fZoomFlag) {
    // screen values
    sX = (sMapX * MAP_GRID_X) + MAP_VIEW_START_X;
    sY = (sMapY * MAP_GRID_Y) + MAP_VIEW_START_Y;

    // restore background
    RestoreExternBackgroundRect(sX, sY, DMAP_GRID_X, DMAP_GRID_Y);
  } else {
    // get screen coords from map values
    GetScreenXYFromMapXYStationary(sMapX, sMapY, &sX, &sY);

    // is this on the screen?
    if ((sX > MapScreenRect.iLeft) && (sX < MapScreenRect.iRight) && (sY > MapScreenRect.iTop) &&
        (sY < MapScreenRect.iBottom)) {
      // offset
      sY = sY - MAP_GRID_Y;
      sX = sX - MAP_GRID_X;

      // restore
      RestoreExternBackgroundRect(sX, sY, DMAP_GRID_ZOOM_X, DMAP_GRID_ZOOM_Y);
    }
  }
}

void ClipBlitsToMapViewRegion() {
  // the standard mapscreen rectangle doesn't work for clipping while zoomed...
  SGPRect ZoomedMapScreenClipRect = {MAP_VIEW_START_X + MAP_GRID_X,
                                     MAP_VIEW_START_Y + MAP_GRID_Y - 1,
                                     MAP_VIEW_START_X + MAP_VIEW_WIDTH + MAP_GRID_X,
                                     MAP_VIEW_START_Y + MAP_VIEW_HEIGHT + MAP_GRID_Y - 10};
  SGPRect *pRectToUse;

  if (fZoomFlag)
    pRectToUse = &ZoomedMapScreenClipRect;
  else
    pRectToUse = &MapScreenRect;

  SetClippingRect(pRectToUse);
  gOldClipRect = gDirtyClipRect;
  gDirtyClipRect = *pRectToUse;
}

void RestoreClipRegionToFullScreen() {
  SetClippingRect(&FullScreenRect);
  gDirtyClipRect = gOldClipRect;
}

void ClipBlitsToMapViewRegionForRectangleAndABit(uint32_t uiDestPitchBYTES) {
  // clip blits to map view region
  // because MC's map coordinates system is so screwy, these had to be
  // hand-tuned to work right...  ARM
  if (fZoomFlag)
    SetClippingRegionAndImageWidth(uiDestPitchBYTES, MapScreenRect.iLeft + 2, MapScreenRect.iTop,
                                   MapScreenRect.iRight - MapScreenRect.iLeft,
                                   MapScreenRect.iBottom - MapScreenRect.iTop);
  else
    SetClippingRegionAndImageWidth(uiDestPitchBYTES, MapScreenRect.iLeft - 1,
                                   MapScreenRect.iTop - 1,
                                   MapScreenRect.iRight - MapScreenRect.iLeft + 3,
                                   MapScreenRect.iBottom - MapScreenRect.iTop + 2);
}

void RestoreClipRegionToFullScreenForRectangle(uint32_t uiDestPitchBYTES) {
  // clip blits to map view region
  SetClippingRegionAndImageWidth(uiDestPitchBYTES, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
}

// mvt icon offset defines
#define SOUTH_Y_MVT_OFFSET +10
#define SOUTH_X_MVT_OFFSET 0
#define NORTH_Y_MVT_OFFSET -10
#define NORTH_X_MVT_OFFSET +10
#define NORTH_SOUTH_CENTER_OFFSET +5

#define SOUTH_Y_MVT_OFFSET_ZOOM +30
#define SOUTH_X_MVT_OFFSET_ZOOM +5
#define NORTH_Y_MVT_OFFSET_ZOOM -8
#define NORTH_X_MVT_OFFSET_ZOOM +25
#define NORTH_SOUTH_CENTER_OFFSET_ZOOM +15

#define EAST_Y_MVT_OFFSET +8
#define EAST_X_MVT_OFFSET 15
#define WEST_Y_MVT_OFFSET -2
#define WEST_X_MVT_OFFSET -8
#define EAST_WEST_CENTER_OFFSET +2

#define EAST_Y_MVT_OFFSET_ZOOM +24
#define EAST_X_MVT_OFFSET_ZOOM +36
#define WEST_Y_MVT_OFFSET_ZOOM +5
#define WEST_X_MVT_OFFSET_ZOOM -10
#define EAST_WEST_CENTER_OFFSET_ZOOM +12

#define NORTH_TEXT_X_OFFSET +1
#define NORTH_TEXT_Y_OFFSET +4
#define SOUTH_TEXT_X_OFFSET +1
#define SOUTH_TEXT_Y_OFFSET +2

#define EAST_TEXT_X_OFFSET +2
#define EAST_TEXT_Y_OFFSET 0
#define WEST_TEXT_X_OFFSET +4
#define WEST_TEXT_Y_OFFSET 0

#define ICON_WIDTH 8

// show the icons for people in motion
static void ShowPeopleInMotion(int16_t const sX, int16_t const sY) {
  if (iCurrentMapSectorZ != 0) return;

  // show the icons for people in motion from this sector to the next guy over
  int16_t const sSource = CALCULATE_STRATEGIC_INDEX(sX, sY);
  for (int32_t dir = 0; dir != 4;
       ++dir) {  // find how many people are coming and going in this sector
    int16_t sDest = sSource;
    switch (dir) {
      case 0:
        if (sY <= 1) continue;
        sDest += NORTH_MOVE;
        break;
      case 1:
        if (sX >= MAP_WORLD_X - 2) continue;
        sDest += EAST_MOVE;
        break;
      case 2:
        if (sY >= MAP_WORLD_Y - 2) continue;
        sDest += SOUTH_MOVE;
        break;
      case 3:
        if (sX <= 1) continue;
        sDest += WEST_MOVE;
        break;

      default:
        abort();
    }

    int32_t sExiting;
    int32_t sEntering;
    BOOLEAN fAboutToEnter;
    int16_t const sec_src = SECTOR(sSource % MAP_WORLD_X, sSource / MAP_WORLD_X);
    int16_t const sec_dst = SECTOR(sDest % MAP_WORLD_X, sDest / MAP_WORLD_X);
    if (!PlayersBetweenTheseSectors(sec_src, sec_dst, &sExiting, &sEntering, &fAboutToEnter))
      continue;
    // someone is leaving

    // now find position
    int16_t sOffsetX;
    int16_t sOffsetY;
    if (dir % 2 == 0) {     // guys going north or south
      if (sEntering > 0) {  // more than one coming in, offset from middle
        sOffsetX = dir == 0 ? (!fZoomFlag ? NORTH_X_MVT_OFFSET : NORTH_X_MVT_OFFSET_ZOOM)
                            : (!fZoomFlag ? SOUTH_X_MVT_OFFSET : SOUTH_X_MVT_OFFSET_ZOOM);
      } else {
        sOffsetX = !fZoomFlag ? NORTH_SOUTH_CENTER_OFFSET : NORTH_SOUTH_CENTER_OFFSET_ZOOM;
      }

      if (dir == 0) {  // going north
        sOffsetY = !fZoomFlag ? NORTH_Y_MVT_OFFSET : NORTH_Y_MVT_OFFSET_ZOOM;
      } else {  // going south
        sOffsetY = !fZoomFlag ? SOUTH_Y_MVT_OFFSET : SOUTH_Y_MVT_OFFSET_ZOOM;
      }
    } else {                // going east/west
      if (sEntering > 0) {  // people also entering, offset from middle
        sOffsetY = dir == 1 ? (!fZoomFlag ? EAST_Y_MVT_OFFSET : EAST_Y_MVT_OFFSET_ZOOM)
                            : (!fZoomFlag ? WEST_Y_MVT_OFFSET : WEST_Y_MVT_OFFSET_ZOOM);
      } else {
        sOffsetY = !fZoomFlag ? EAST_WEST_CENTER_OFFSET : EAST_WEST_CENTER_OFFSET_ZOOM;
      }

      if (dir == 1) {  // going east
        sOffsetX = !fZoomFlag ? EAST_X_MVT_OFFSET : EAST_X_MVT_OFFSET_ZOOM;
      } else {  // going west
        sOffsetX = !fZoomFlag ? WEST_X_MVT_OFFSET : WEST_X_MVT_OFFSET_ZOOM;
      }
    }

    int16_t sTextXOffset;
    int16_t sTextYOffset;
    switch (dir) {
      case 0:
        sTextXOffset = NORTH_TEXT_X_OFFSET;
        sTextYOffset = NORTH_TEXT_Y_OFFSET;
        break;
      case 1:
        sTextXOffset = EAST_TEXT_X_OFFSET;
        sTextYOffset = EAST_TEXT_Y_OFFSET;
        break;
      case 2:
        sTextXOffset = SOUTH_TEXT_X_OFFSET;
        sTextYOffset = SOUTH_TEXT_Y_OFFSET;
        break;
      case 3:
        sTextXOffset = WEST_TEXT_X_OFFSET;
        sTextYOffset = WEST_TEXT_Y_OFFSET;
        break;

      default:
        abort();
    }

    // if about to enter, draw yellow arrows, blue otherwise
    SGPVObject const *const hIconHandle =
        fAboutToEnter ? guiCHARBETWEENSECTORICONSCLOSE : guiCHARBETWEENSECTORICONS;

    int16_t iX;
    int16_t iY;
    // zoomed in or not?
    if (!fZoomFlag) {
      iX = MAP_VIEW_START_X + sX * MAP_GRID_X + sOffsetX;
      iY = MAP_Y_ICON_OFFSET + MAP_VIEW_START_Y + sY * MAP_GRID_Y + sOffsetY;
      BltVideoObject(guiSAVEBUFFER, hIconHandle, dir, iX, iY);
    } else {
      int16_t sXPosition;
      int16_t sYPosition;
      GetScreenXYFromMapXYStationary(sX, sY, &sXPosition, &sYPosition);
      iY = sYPosition - MAP_GRID_Y + sOffsetY;
      iX = sXPosition - MAP_GRID_X + sOffsetX;

      ClipBlitsToMapViewRegion();
      BltVideoObject(guiSAVEBUFFER, hIconHandle, dir, iX, iY);
      RestoreClipRegionToFullScreen();
    }

    // blit the text
    uint8_t const foreground = fAboutToEnter ? FONT_BLACK : FONT_WHITE;
    SetFontAttributes(MAP_MVT_ICON_FONT, foreground);
    SetFontDestBuffer(guiSAVEBUFFER);

    wchar_t buf[32];
    swprintf(buf, lengthof(buf), L"%d", sExiting);

    int16_t usX;
    int16_t usY;
    FindFontCenterCoordinates(iX + sTextXOffset, 0, ICON_WIDTH, 0, buf, MAP_FONT, &usX, &usY);
    MPrint(usX, iY + sTextYOffset, buf);

    int32_t iWidth;
    int32_t iHeight;
    if (dir % 2 == 0) {  // north south
      iWidth = 10;
      iHeight = 12;
    } else {  // east west
      iWidth = 12;
      iHeight = 7;
    }

    // error correction for scrolling with people on the move
    if (iX < 0) {
      iWidth += iX;
      iX = 0;
    }

    if (iY < 0) {
      iHeight += iY;
      iY = 0;
    }

    if (iWidth > 0 && iHeight > 0) {
      RestoreExternBackgroundRect(iX, iY, (int16_t)iWidth, (int16_t)iHeight);
    }
  }

  // restore buffer
  SetFontDestBuffer(FRAME_BUFFER);
}

/* calculate the distance travelled, the proposed distance, and total distance
 * one can go and display these on screen */
void DisplayDistancesForHelicopter() {
  static int16_t sOldYPosition = 0;

  int16_t sMapX;
  int16_t sMapY;
  int16_t const sYPosition = !fZoomFlag && GetMouseMapXY(&sMapX, &sMapY) && sMapY >= 13
                                 ? MAP_HELICOPTER_UPPER_ETA_POPUP_Y
                                 : MAP_HELICOPTER_ETA_POPUP_Y;

  if (sOldYPosition != 0 && sOldYPosition != sYPosition) {
    RestoreExternBackgroundRect(MAP_HELICOPTER_ETA_POPUP_X, sOldYPosition,
                                MAP_HELICOPTER_ETA_POPUP_WIDTH + 20,
                                MAP_HELICOPTER_ETA_POPUP_HEIGHT);
  }
  sOldYPosition = sYPosition;

  BltVideoObject(FRAME_BUFFER, guiMapBorderHeliSectors, 0, MAP_HELICOPTER_ETA_POPUP_X, sYPosition);

  SetFontAttributes(MAP_FONT, FONT_LTGREEN);

  int32_t const x = MAP_HELICOPTER_ETA_POPUP_X + 5;
  int32_t y = sYPosition + 5;
  int32_t const w = MAP_HELICOPTER_ETA_POPUP_WIDTH;
  int32_t const h = GetFontHeight(MAP_FONT);
  wchar_t sString[32];
  int16_t sX;
  int16_t sY;

  MPrint(x, y, pHelicopterEtaStrings[0]);
  int32_t const total_distance = DistanceOfIntendedHelicopterPath();
  swprintf(sString, lengthof(sString), L"%d", total_distance);
  FindFontRightCoordinates(x, y, w, 0, sString, MAP_FONT, &sX, &sY);
  MPrint(sX, y, sString);
  y += h;

  MPrint(x, y, pHelicopterEtaStrings[1]);
  int16_t const n_safe_sectors = GetNumSafeSectorsInPath();
  swprintf(sString, lengthof(sString), L"%d", n_safe_sectors);
  FindFontRightCoordinates(x, y, w, 0, sString, MAP_FONT, &sX, &sY);
  MPrint(sX, y, sString);
  y += h;

  MPrint(x, y, pHelicopterEtaStrings[2]);
  int16_t const n_unsafe_sectors = GetNumUnSafeSectorsInPath();
  swprintf(sString, lengthof(sString), L"%d", n_unsafe_sectors);
  FindFontRightCoordinates(x, y, w, 0, sString, MAP_FONT, &sX, &sY);
  MPrint(sX, y, sString);
  y += h;

  // calculate the cost of the trip based on the number of safe and unsafe
  // sectors it will pass through
  MPrint(x, y, pHelicopterEtaStrings[3]);
  uint32_t const uiTripCost =
      n_safe_sectors * COST_AIRSPACE_SAFE + n_unsafe_sectors * COST_AIRSPACE_UNSAFE;
  SPrintMoney(sString, uiTripCost);
  FindFontRightCoordinates(x, y, w, 0, sString, MAP_FONT, &sX, &sY);
  MPrint(sX, y, sString);
  y += h;

  MPrint(x, y, pHelicopterEtaStrings[4]);
  // get travel time for the last path segment
  int32_t iTime = GetPathTravelTimeDuringPlotting(pTempHelicopterPath);
  // add travel time for any prior path segments (stored in the helicopter's
  // mercpath, but waypoints aren't built)
  iTime += GetPathTravelTimeDuringPlotting(GetHelicopter().pMercPath);
  swprintf(sString, lengthof(sString), L"%d%ls %d%ls", iTime / 60, gsTimeStrings[0], iTime % 60,
           gsTimeStrings[1]);
  FindFontRightCoordinates(x, y, w, 0, sString, MAP_FONT, &sX, &sY);
  MPrint(sX, y, sString);
  y += h;

  // show # of passengers aboard the chopper
  MPrint(x, y, pHelicopterEtaStrings[6]);
  swprintf(sString, lengthof(sString), L"%d", GetNumberOfPassengersInHelicopter());
  FindFontRightCoordinates(x, y, w, 0, sString, MAP_FONT, &sX, &sY);
  MPrint(sX, y, sString);

  InvalidateRegion(MAP_HELICOPTER_ETA_POPUP_X, sOldYPosition,
                   MAP_HELICOPTER_ETA_POPUP_X + MAP_HELICOPTER_ETA_POPUP_WIDTH + 20,
                   sOldYPosition + MAP_HELICOPTER_ETA_POPUP_HEIGHT);
}

static void DisplayDestinationOfHelicopter();

// grab position of helicopter and blt to screen
void DisplayPositionOfHelicopter() {
  static int16_t sOldMapX = 0, sOldMapY = 0;
  //	int16_t sX =0, sY = 0;
  float flRatio = 0.0;
  uint32_t x, y;
  uint16_t minX, minY, maxX, maxY;

  AssertMsg(0 <= sOldMapX && sOldMapX < SCREEN_WIDTH,
            String("DisplayPositionOfHelicopter: Invalid sOldMapX = %d", sOldMapX));
  AssertMsg(0 <= sOldMapY && sOldMapY < SCREEN_HEIGHT,
            String("DisplayPositionOfHelicopter: Invalid sOldMapY = %d", sOldMapY));

  // restore background on map where it is
  if (sOldMapX != 0) {
    RestoreExternBackgroundRect(sOldMapX, sOldMapY, HELI_ICON_WIDTH, HELI_ICON_HEIGHT);
    sOldMapX = 0;
  }

  if (iHelicopterVehicleId != -1) {
    // draw the destination icon first, so when they overlap, the real one is on
    // top!
    DisplayDestinationOfHelicopter();

    VEHICLETYPE const &v = GetHelicopter();
    // check if mvt group
    if (v.ubMovementGroup != 0) {
      const GROUP *const pGroup = GetGroup(v.ubMovementGroup);

      // this came up in one bug report!
      Assert(pGroup->uiTraverseTime != -1);

      if (pGroup->uiTraverseTime > 0 && pGroup->uiTraverseTime != TRAVERSE_TIME_IMPOSSIBLE) {
        flRatio = ((pGroup->uiTraverseTime + GetWorldTotalMin()) - pGroup->uiArrivalTime) /
                  (float)pGroup->uiTraverseTime;
      }

      /*
                              AssertMsg( ( flRatio >= 0 ) && ( flRatio <= 100 ),
         String( "DisplayPositionOfHelicopter: Invalid flRatio = %6.2f, trav %d,
         arr %d, time %d", flRatio, pGroup->uiTraverseTime,
         pGroup->uiArrivalTime, GetWorldTotalMin() ) );
      */

      if (flRatio < 0) {
        flRatio = 0;
      } else if (flRatio > 100) {
        flRatio = 100;
      }

      //			if( !fZoomFlag )
      {
        // grab min and max locations to interpolate sub sector position
        minX = MAP_VIEW_START_X + MAP_GRID_X * (pGroup->ubSectorX);
        maxX = MAP_VIEW_START_X + MAP_GRID_X * (pGroup->ubNextX);
        minY = MAP_VIEW_START_Y + MAP_GRID_Y * (pGroup->ubSectorY);
        maxY = MAP_VIEW_START_Y + MAP_GRID_Y * (pGroup->ubNextY);
      }
      /*
                              else
                              {

                                      // grab coords for nextx,y and current x,y

                                      // zoomed in, takes a little more work
                                      GetScreenXYFromMapXYStationary(
         ((uint16_t)(pGroup->ubSectorX)),((uint16_t)(pGroup->ubSectorY)) , &sX, &sY
         ); sY=sY-MAP_GRID_Y; sX=sX-MAP_GRID_X;

                                      minX = ( sX );
                                      minY = ( sY );

                                      GetScreenXYFromMapXYStationary(
         ((uint16_t)(pGroup->ubNextX)),((uint16_t)(pGroup->ubNextY)) , &sX, &sY );
                                      sY=sY-MAP_GRID_Y;
                                      sX=sX-MAP_GRID_X;

                                      maxX = ( sX );
                                      maxY = ( sY );
                              }
      */

      AssertMsg(minX < 640, String("DisplayPositionOfHelicopter: Invalid minX = %d", minX));
      AssertMsg(maxX < 640, String("DisplayPositionOfHelicopter: Invalid maxX = %d", maxX));
      AssertMsg(minY < 640, String("DisplayPositionOfHelicopter: Invalid minY = %d", minY));
      AssertMsg(maxY < 640, String("DisplayPositionOfHelicopter: Invalid maxY = %d", maxY));

      // IMPORTANT: Since min can easily be larger than max, we gotta cast to as
      // signed value
      x = (uint32_t)(minX + flRatio * ((int16_t)maxX - (int16_t)minX));
      y = (uint32_t)(minY + flRatio * ((int16_t)maxY - (int16_t)minY));

      /*
                              if( fZoomFlag )
                              {
                                      x += 13;
                                      y += 8;
                              }
                              else
      */
      {
        x += 1;
        y += 3;
      }

      AssertMsg(0 <= x && x < SCREEN_WIDTH,
                String("DisplayPositionOfHelicopter: Invalid x = %d.  At "
                       "%d,%d.  Next %d,%d.  Min/Max X = %d/%d",
                       x, pGroup->ubSectorX, pGroup->ubSectorY, pGroup->ubNextX, pGroup->ubNextY,
                       minX, maxX));

      AssertMsg(0 <= y && y < SCREEN_HEIGHT,
                String("DisplayPositionOfHelicopter: Invalid y = %d.  At "
                       "%d,%d.  Next %d,%d.  Min/Max Y = %d/%d",
                       y, pGroup->ubSectorX, pGroup->ubSectorY, pGroup->ubNextX, pGroup->ubNextY,
                       minY, maxY));

      // clip blits to mapscreen region
      ClipBlitsToMapViewRegion();

      BltVideoObject(FRAME_BUFFER, guiHelicopterIcon, HELI_ICON, x, y);

      SetFontAttributes(MAP_MVT_ICON_FONT, FONT_WHITE);
      mprintf(x + 5, y + 1, L"%d", GetNumberOfPassengersInHelicopter());

      InvalidateRegion(x, y, x + HELI_ICON_WIDTH, y + HELI_ICON_HEIGHT);

      RestoreClipRegionToFullScreen();

      // now store the old stuff
      sOldMapX = (int16_t)x;
      sOldMapY = (int16_t)y;
    }
  }
}

static void DisplayDestinationOfHelicopter() {
  static int16_t sOldMapX = 0, sOldMapY = 0;
  int16_t sMapX, sMapY;
  uint32_t x, y;

  AssertMsg(0 <= sOldMapX && sOldMapX < SCREEN_WIDTH,
            String("DisplayDestinationOfHelicopter: Invalid sOldMapX = %d", sOldMapX));
  AssertMsg(0 <= sOldMapY && sOldMapY < SCREEN_HEIGHT,
            String("DisplayDestinationOfHelicopter: Invalid sOldMapY = %d", sOldMapY));

  // restore background on map where it is
  if (sOldMapX != 0) {
    RestoreExternBackgroundRect(sOldMapX, sOldMapY, HELI_SHADOW_ICON_WIDTH,
                                HELI_SHADOW_ICON_HEIGHT);
    sOldMapX = 0;
  }

  // if helicopter is going somewhere
  if (GetLengthOfPath(GetHelicopter().pMercPath) > 1) {
    // get destination
    const int16_t sSector = GetLastSectorOfHelicoptersPath();
    sMapX = sSector % MAP_WORLD_X;
    sMapY = sSector / MAP_WORLD_X;

    x = MAP_VIEW_START_X + (MAP_GRID_X * sMapX) + 1;
    y = MAP_VIEW_START_Y + (MAP_GRID_Y * sMapY) + 3;

    AssertMsg(
        0 <= x && x < SCREEN_WIDTH,
        String("DisplayDestinationOfHelicopter: Invalid x = %d.  Dest %d,%d", x, sMapX, sMapY));
    AssertMsg(
        0 <= y && y < SCREEN_HEIGHT,
        String("DisplayDestinationOfHelicopter: Invalid y = %d.  Dest %d,%d", y, sMapX, sMapY));

    // clip blits to mapscreen region
    ClipBlitsToMapViewRegion();

    BltVideoObject(FRAME_BUFFER, guiHelicopterIcon, HELI_SHADOW_ICON, x, y);
    InvalidateRegion(x, y, x + HELI_SHADOW_ICON_WIDTH, y + HELI_SHADOW_ICON_HEIGHT);

    RestoreClipRegionToFullScreen();

    // now store the old stuff
    sOldMapX = (int16_t)x;
    sOldMapY = (int16_t)y;
  }
}

BOOLEAN CheckForClickOverHelicopterIcon(int16_t sClickedSectorX, int16_t sClickedSectorY) {
  BOOLEAN fHelicopterOverNextSector = FALSE;
  float flRatio = 0.0;
  int16_t sSectorX;
  int16_t sSectorY;

  if (!fShowAircraftFlag) return FALSE;
  if (iHelicopterVehicleId == -1) return FALSE;

  VEHICLETYPE const &v = GetHelicopter();

  // figure out over which sector the helicopter APPEARS to be to the player
  // (because we slide it smoothly across the map, unlike groups travelling on
  // the ground, it can appear over its next sector while it's not there yet.
  GROUP const *const pGroup = GetGroup(v.ubMovementGroup);
  Assert(pGroup);

  if (pGroup->fBetweenSectors) {
    // this came up in one bug report!
    Assert(pGroup->uiTraverseTime != -1);

    if (pGroup->uiTraverseTime > 0 && pGroup->uiTraverseTime != TRAVERSE_TIME_IMPOSSIBLE) {
      flRatio = (pGroup->uiTraverseTime - pGroup->uiArrivalTime + GetWorldTotalMin()) /
                (float)pGroup->uiTraverseTime;
    }

    // if more than halfway there, the chopper appears more over the next
    // sector, not over its current one(!)
    if (flRatio > 0.5) {
      fHelicopterOverNextSector = TRUE;
    }
  }

  if (fHelicopterOverNextSector) {
    // use the next sector's coordinates
    sSectorX = pGroup->ubNextX;
    sSectorY = pGroup->ubNextY;
  } else {
    // use current sector's coordinates
    sSectorX = v.sSectorX;
    sSectorY = v.sSectorY;
  }

  // check if helicopter appears where he clicked
  if ((sSectorX != sClickedSectorX) || (sSectorY != sClickedSectorY)) {
    return (FALSE);
  }

  return (TRUE);
}

static void DrawSite(const int16_t sector_x, const int16_t sector_y, const SGPVObject *const icon) {
  int16_t x;
  int16_t y;
  uint16_t max_w;
  uint16_t max_h;
  uint8_t vo_idx;
  if (fZoomFlag) {
    {
      SGPVSurface::Lock l(guiSAVEBUFFER);
      SetClippingRegionAndImageWidth(l.Pitch(), MAP_VIEW_START_X + MAP_GRID_X - 1,
                                     MAP_VIEW_START_Y + MAP_GRID_Y - 1, MAP_VIEW_WIDTH + 1,
                                     MAP_VIEW_HEIGHT - 9);
    }

    GetScreenXYFromMapXYStationary(sector_x, sector_y, &x, &y);
    x -= MAP_GRID_X - 1;
    y -= MAP_GRID_Y;
    max_w = MAP_GRID_ZOOM_X - 1;
    max_h = MAP_GRID_ZOOM_Y - 1;
    vo_idx = 0;
  } else {
    GetScreenXYFromMapXY(sector_x, sector_y, &x, &y);
    ++x;
    max_w = MAP_GRID_X - 1;
    max_h = MAP_GRID_Y - 1;
    vo_idx = 1;
  }

  ETRLEObject const &ETRLEProps = icon->SubregionProperties(vo_idx);
  uint16_t const w = ETRLEProps.usWidth;
  uint16_t const h = ETRLEProps.usHeight;
  x += (max_w - w) / 2;
  /* If the icon is higher than a map cell, align with the bottom of the cell */
  y += (h > max_h ? max_h - h : (max_h - h) / 2);

  BltVideoObject(guiSAVEBUFFER, icon, vo_idx, x, y);
}

static void BlitMineIcon(int16_t sMapX, int16_t sMapY) { DrawSite(sMapX, sMapY, guiMINEICON); }

static void PrintStringCenteredBoxed(int32_t x, const int32_t y, const wchar_t *const string) {
  x -= StringPixLength(string, MAP_FONT) / 2;
  if (!fZoomFlag)  // it's ok to cut strings off in zoomed mode
  {
    if (x < MAP_VIEW_START_X + 23) x = MAP_VIEW_START_X + 23;
  }
  MPrint(x, y, string);
}

static void BlitMineText(uint8_t const mine_idx, int16_t const sMapX, int16_t const sMapY) {
  // set coordinates for start of mine text
  int16_t sScreenX;
  int16_t sScreenY;
  if (fZoomFlag) {
    GetScreenXYFromMapXYStationary(sMapX, sMapY, &sScreenX, &sScreenY);
    sScreenY += MAP_GRID_ZOOM_Y / 2 + 1;  // slightly below
  } else {
    GetScreenXYFromMapXY(sMapX, sMapY, &sScreenX, &sScreenY);
    sScreenX += MAP_GRID_X / 2;  // centered around middle of mine square
    sScreenY += MAP_GRID_Y + 1;  // slightly below
  }

  // show detailed mine info (name, production rate, daily production)

  SetFontDestBuffer(guiSAVEBUFFER, MAP_VIEW_START_X, MAP_VIEW_START_Y,
                    MAP_VIEW_START_X + MAP_VIEW_WIDTH + MAP_GRID_X,
                    MAP_VIEW_START_Y + MAP_VIEW_HEIGHT + 7);
  SetFontAttributes(MAP_FONT, FONT_LTGREEN);

  int32_t const x = sScreenX;
  int32_t y = sScreenY;
  int32_t const h = GetFontHeight(MAP_FONT);
  wchar_t buf[32];

  // display associated town name, followed by "mine"
  swprintf(buf, lengthof(buf), L"%ls %ls", pTownNames[GetTownAssociatedWithMine(mine_idx)],
           pwMineStrings[0]);
  PrintStringCenteredBoxed(x, y, buf);
  y += h;

  // check if mine is empty (abandoned) or running out
  if (gMineStatus[mine_idx].fEmpty) {
    PrintStringCenteredBoxed(x, y, pwMineStrings[5]);
    y += h;
  } else if (gMineStatus[mine_idx].fShutDown) {
    PrintStringCenteredBoxed(x, y, pwMineStrings[6]);
    y += h;
  } else if (gMineStatus[mine_idx].fRunningOut) {
    PrintStringCenteredBoxed(x, y, pwMineStrings[7]);
    y += h;
  }

  // only show production if player controls it and it's actually producing
  if (PlayerControlsMine(mine_idx) && !gMineStatus[mine_idx].fEmpty) {
    // show current production
    SPrintMoney(buf, PredictDailyIncomeFromAMine(mine_idx));

    // if potential is not nil, show percentage of the two
    if (GetMaxPeriodicRemovalFromMine(mine_idx) > 0) {
      wchar_t wSubString[32];
      swprintf(wSubString, lengthof(wSubString), L" (%d%%)",
               PredictDailyIncomeFromAMine(mine_idx) * 100 / GetMaxDailyRemovalFromMine(mine_idx));
      wcscat(buf, wSubString);
    }

    PrintStringCenteredBoxed(x, y, buf);
  }

  SetFontDestBuffer(FRAME_BUFFER, MAP_VIEW_START_X, MAP_VIEW_START_Y,
                    MAP_VIEW_START_X + MAP_VIEW_WIDTH + MAP_GRID_X,
                    MAP_VIEW_START_Y + MAP_VIEW_HEIGHT + 7);
}

static void BlitTownGridMarkers() {
  SGPVSurface::Lock l(guiSAVEBUFFER);
  uint16_t *const buf = l.Buffer<uint16_t>();
  uint32_t const pitch = l.Pitch();

  ClipBlitsToMapViewRegionForRectangleAndABit(pitch);

  // Go through list of towns and place on screen
  uint16_t const color = Get16BPPColor(FROMRGB(100, 100, 100));
  FOR_EACH_TOWN_SECTOR(i) {
    // skip Orta/Tixa until found
    switch (i->town) {
      case ORTA:
        if (!fFoundOrta) continue;
        break;
      case TIXA:
        if (!fFoundTixa) continue;
        break;
    }

    int32_t const sector = i->sector;
    int16_t x;
    int16_t y;
    int16_t w;
    int16_t h;
    if (fZoomFlag) {
      GetScreenXYFromMapXYStationary(SECTORX(sector), SECTORY(sector), &x, &y);
      x -= MAP_GRID_X - 1;
      y -= MAP_GRID_Y;
      w = 2 * MAP_GRID_X;
      h = 2 * MAP_GRID_Y;
    } else {  // Get location on screen
      GetScreenXYFromMapXY(SECTORX(sector), SECTORY(sector), &x, &y);
      w = MAP_GRID_X - 1;
      h = MAP_GRID_Y;
      x += 2;
    }

    int32_t const loc = SECTOR_INFO_TO_STRATEGIC_INDEX(sector);
    if (StrategicMap[loc - MAP_WORLD_X].bNameId == BLANK_SECTOR) {
      LineDraw(TRUE, x - 1, y - 1, x + w - 1, y - 1, color, buf);
    }

    if (StrategicMap[loc + MAP_WORLD_X].bNameId == BLANK_SECTOR) {
      LineDraw(TRUE, x - 1, y + h - 1, x + w - 1, y + h - 1, color, buf);
    }

    if (StrategicMap[loc - 1].bNameId == BLANK_SECTOR) {
      LineDraw(TRUE, x - 2, y - 1, x - 2, y + h - 1, color, buf);
    }

    if (StrategicMap[loc + 1].bNameId == BLANK_SECTOR) {
      LineDraw(TRUE, x + w - 1, y - 1, x + w - 1, y + h - 1, color, buf);
    }
  }

  RestoreClipRegionToFullScreenForRectangle(pitch);
}

static void BlitMineGridMarkers() {
  SGPVSurface::Lock l(guiSAVEBUFFER);
  uint32_t const pitch = l.Pitch();

  ClipBlitsToMapViewRegionForRectangleAndABit(pitch);

  uint16_t const color = Get16BPPColor(FROMRGB(100, 100, 100));
  FOR_EACH(MINE_LOCATION_TYPE const, i, gMineLocation) {
    int16_t x;
    int16_t y;
    int16_t w;
    int16_t h;
    MINE_LOCATION_TYPE const &m = *i;
    int16_t const mx = SECTORX(m.sector);
    int16_t const my = SECTORY(m.sector);
    if (fZoomFlag) {
      GetScreenXYFromMapXYStationary(mx, my, &x, &y);
      x -= MAP_GRID_X;
      y -= MAP_GRID_Y;
      w = 2 * MAP_GRID_X;
      h = 2 * MAP_GRID_Y;
    } else {  // Get location on screen
      GetScreenXYFromMapXY(mx, my, &x, &y);
      w = MAP_GRID_X;
      h = MAP_GRID_Y;
    }

    RectangleDraw(TRUE, x, y - 1, x + w, y + h - 1, color, l.Buffer<uint16_t>());
  }

  RestoreClipRegionToFullScreenForRectangle(pitch);
}

static void DisplayLevelString() {
  // given the current level being displayed on the map, show a sub level
  // message

  // at the surface
  if (!iCurrentMapSectorZ) {
    return;
  }

  // otherwise we will have to display the string with the level number

  SetFontDestBuffer(guiSAVEBUFFER, MAP_VIEW_START_X, MAP_VIEW_START_Y,
                    MAP_VIEW_START_X + MAP_VIEW_WIDTH + MAP_GRID_X,
                    MAP_VIEW_START_Y + MAP_VIEW_HEIGHT + 7);
  SetFontAttributes(MAP_FONT, MAP_INDEX_COLOR);
  mprintf(MAP_LEVEL_STRING_X, MAP_LEVEL_STRING_Y, L"%ls %d", sMapLevelString, iCurrentMapSectorZ);
  SetFontDestBuffer(FRAME_BUFFER);
}

static int16_t &GetPickedUpMilitia(uint8_t const type) {
  switch (type) {
    case GREEN_MILITIA:
      return sGreensOnCursor;
    case REGULAR_MILITIA:
      return sRegularsOnCursor;
    case ELITE_MILITIA:
      return sElitesOnCursor;
    default:
      throw std::logic_error("invalid militia type");
  }
}

// function to manipulate the number of towns people on the cursor
static void PickUpATownPersonFromSector(uint8_t const type, uint8_t const sector) {
  // Are they in the same town as they were picked up from?
  if (GetTownIdForSector(sector) != sSelectedMilitiaTown) return;

  if (!SectorOursAndPeaceful(SECTORX(sector), SECTORY(sector), 0)) return;

  uint8_t &n_type = SectorInfo[sector].ubNumberOfCivsAtLevel[type];
  // See if there are any militia of this type in this sector
  if (n_type == 0) return;

  --n_type;                    // Reduce number in this sector
  ++GetPickedUpMilitia(type);  // Pick this guy up
  fMapPanelDirty = TRUE;
  if (sector == GetWorldSector()) gfStrategicMilitiaChangesMade = TRUE;
}

static void DropAPersonInASector(uint8_t const type, uint8_t const sector) {
  // Are they in the same town as they were picked up from?
  if (GetTownIdForSector(sector) != sSelectedMilitiaTown) return;

  if (!SectorOursAndPeaceful(SECTORX(sector), SECTORY(sector), 0)) return;

  uint8_t(&n_milita)[MAX_MILITIA_LEVELS] = SectorInfo[sector].ubNumberOfCivsAtLevel;
  if (n_milita[GREEN_MILITIA] + n_milita[REGULAR_MILITIA] + n_milita[ELITE_MILITIA] >=
      MAX_ALLOWABLE_MILITIA_PER_SECTOR)
    return;

  // Drop the guy into this sector
  int16_t &n_type = GetPickedUpMilitia(type);
  if (n_type == 0) return;

  --n_type;
  ++n_milita[type];  // Up the number in this sector of this type of militia
  fMapPanelDirty = TRUE;
  if (sector == GetWorldSector()) gfStrategicMilitiaChangesMade = TRUE;
}

void LoadMapScreenInterfaceMapGraphics() {
  guiBIGMAP = AddVideoSurfaceFromFile(INTERFACEDIR "/b_map.pcx");
  guiBULLSEYE = AddVideoObjectFromFile(INTERFACEDIR "/bullseye.sti");
  guiSAMICON = AddVideoObjectFromFile(INTERFACEDIR "/sam.sti");
  guiCHARBETWEENSECTORICONS = AddVideoObjectFromFile(INTERFACEDIR "/merc_between_sector_icons.sti");
  guiCHARBETWEENSECTORICONSCLOSE =
      AddVideoObjectFromFile(INTERFACEDIR "/merc_mvt_green_arrows.sti");
  guiCHARICONS = AddVideoObjectFromFile(INTERFACEDIR "/boxes.sti");
  guiHelicopterIcon = AddVideoObjectFromFile(INTERFACEDIR "/helicop.sti");
  guiMAPCURSORS = AddVideoObjectFromFile(INTERFACEDIR "/mapcursr.sti");
  guiMINEICON = AddVideoObjectFromFile(INTERFACEDIR "/mine.sti");
  guiMapBorderHeliSectors = AddVideoObjectFromFile(INTERFACEDIR "/pos2.sti");
  guiMilitia = AddVideoObjectFromFile(INTERFACEDIR "/militia.sti");
  guiMilitiaMaps = AddVideoObjectFromFile(INTERFACEDIR "/militiamaps.sti");
  guiMilitiaSectorHighLight = AddVideoObjectFromFile(INTERFACEDIR "/militiamapsectoroutline2.sti");
  guiMilitiaSectorOutline = AddVideoObjectFromFile(INTERFACEDIR "/militiamapsectoroutline.sti");
  guiORTAICON = AddVideoObjectFromFile(INTERFACEDIR "/map_item.sti");
  guiSubLevel1 = AddVideoObjectFromFile(INTERFACEDIR "/mine_1.sti");
  guiSubLevel2 = AddVideoObjectFromFile(INTERFACEDIR "/mine_2.sti");
  guiSubLevel3 = AddVideoObjectFromFile(INTERFACEDIR "/mine_3.sti");
  guiTIXAICON = AddVideoObjectFromFile(INTERFACEDIR "/prison.sti");
}

void DeleteMapScreenInterfaceMapGraphics() {
  DeleteVideoSurface(guiBIGMAP);
  DeleteVideoObject(guiBULLSEYE);
  DeleteVideoObject(guiSAMICON);
  DeleteVideoObject(guiCHARBETWEENSECTORICONS);
  DeleteVideoObject(guiCHARBETWEENSECTORICONSCLOSE);
  DeleteVideoObject(guiCHARICONS);
  DeleteVideoObject(guiHelicopterIcon);
  DeleteVideoObject(guiMAPCURSORS);
  DeleteVideoObject(guiMINEICON);
  DeleteVideoObject(guiMapBorderHeliSectors);
  DeleteVideoObject(guiMilitia);
  DeleteVideoObject(guiMilitiaMaps);
  DeleteVideoObject(guiMilitiaSectorHighLight);
  DeleteVideoObject(guiMilitiaSectorOutline);
  DeleteVideoObject(guiORTAICON);
  DeleteVideoObject(guiSubLevel1);
  DeleteVideoObject(guiSubLevel2);
  DeleteVideoObject(guiSubLevel3);
  DeleteVideoObject(guiTIXAICON);
}

static void CheckAndUpdateStatesOfSelectedMilitiaSectorButtons();
static void DisplayUnallocatedMilitia();
static void DrawTownMilitiaName();
static void RenderIconsPerSectorForSelectedTown();
static void RenderShadingForUnControlledSectors();
static void SetMilitiaMapButtonsText();
static void ShowHighLightedSectorOnMilitiaMap();

void DrawMilitiaPopUpBox() {
  if (!fShowMilitia) {
    sSelectedMilitiaTown = 0;
  }

  // create buttons
  CreateDestroyMilitiaSectorButtons();

  // create mouse regions if we need to
  CreateDestroyMilitiaPopUPRegions();

  if (!sSelectedMilitiaTown) return;

  // update states of militia selected sector buttons
  CheckAndUpdateStatesOfSelectedMilitiaSectorButtons();

  BltVideoObject(FRAME_BUFFER, guiMilitia, 0, MAP_MILITIA_BOX_POS_X, MAP_MILITIA_BOX_POS_Y);
  BltVideoObject(FRAME_BUFFER, guiMilitiaMaps, sSelectedMilitiaTown - 1,
                 MAP_MILITIA_BOX_POS_X + MAP_MILITIA_MAP_X,
                 MAP_MILITIA_BOX_POS_Y + MAP_MILITIA_MAP_Y);

  // set font color for labels and "total militia" counts
  SetFontForeground(FONT_WHITE);

  // draw name of town, and the "unassigned" label
  DrawTownMilitiaName();

  // render the icons for each sector in the town
  RenderIconsPerSectorForSelectedTown();

  // shade any sectors not under our control
  RenderShadingForUnControlledSectors();

  // display anyone picked up
  DisplayUnallocatedMilitia();

  // draw the highlight last
  ShowHighLightedSectorOnMilitiaMap();

  ETRLEObject const &pTrav = guiMilitia->SubregionProperties(0);
  InvalidateRegion(MAP_MILITIA_BOX_POS_X, MAP_MILITIA_BOX_POS_Y,
                   MAP_MILITIA_BOX_POS_X + pTrav.usWidth, MAP_MILITIA_BOX_POS_Y + pTrav.usHeight);

  // set the text for the militia map sector info buttons
  SetMilitiaMapButtonsText();

  // render buttons
  MarkButtonsDirty();
}

static void CreateMilitiaPanelBottomButton();
static void DeleteMilitiaPanelBottomButton();
static void HandleShutDownOfMilitiaPanelIfPeopleOnTheCursor(int16_t sTownValue);
static void MilitiaRegionClickCallback(MOUSE_REGION *, int32_t reason);
static void MilitiaRegionMoveCallback(MOUSE_REGION *, int32_t reason);

void CreateDestroyMilitiaPopUPRegions() {
  static int16_t sOldTown = 0;

  // create destroy militia pop up regions for mapscreen militia pop up box
  if (sSelectedMilitiaTown != 0) {
    sOldTown = sSelectedMilitiaTown;
  }

  if (fShowMilitia && sSelectedMilitiaTown && !gfMilitiaPopupCreated) {
    for (int32_t i = 0; i < 9; ++i) {
      MOUSE_REGION *const r = &gMapScreenMilitiaBoxRegions[i];
      uint16_t const x =
          MAP_MILITIA_BOX_POS_X + MAP_MILITIA_MAP_X + i % MILITIA_BOX_ROWS * MILITIA_BOX_BOX_WIDTH;
      uint16_t const y =
          MAP_MILITIA_BOX_POS_Y + MAP_MILITIA_MAP_Y + i / MILITIA_BOX_ROWS * MILITIA_BOX_BOX_HEIGHT;
      MSYS_DefineRegion(r, x, y, x + MILITIA_BOX_BOX_WIDTH, y + MILITIA_BOX_BOX_HEIGHT,
                        MSYS_PRIORITY_HIGHEST - 3, MSYS_NO_CURSOR, MilitiaRegionMoveCallback,
                        MilitiaRegionClickCallback);
      MSYS_SetRegionUserData(r, 0, i);
    }

    CreateMilitiaPanelBottomButton();
    gfMilitiaPopupCreated = TRUE;
  } else if (gfMilitiaPopupCreated && (!fShowMilitia || !sSelectedMilitiaTown)) {
    for (int32_t i = 0; i < 9; ++i) {
      MSYS_RemoveRegion(&gMapScreenMilitiaBoxRegions[i]);
    }

    // handle the shutdown of the panel...there maybe people on the cursor,
    // distribute them evenly over all the sectors
    HandleShutDownOfMilitiaPanelIfPeopleOnTheCursor(sOldTown);

    DeleteMilitiaPanelBottomButton();
    gfMilitiaPopupCreated = FALSE;
  }
}

static int16_t GetBaseSectorForCurrentTown();

static void RenderIconsPerSectorForSelectedTown() {
  // render icons for map
  int16_t const sBaseSectorValue = GetBaseSectorForCurrentTown();
  for (int32_t iCounter = 0; iCounter != 9; ++iCounter) {
    int32_t const dx = iCounter % MILITIA_BOX_ROWS;
    int32_t const dy = iCounter / MILITIA_BOX_ROWS;
    int16_t const sector = sBaseSectorValue + dx + dy * 16;

    // skip sectors not in the selected town (nearby other towns or wilderness
    // SAM Sites)
    if (GetTownIdForSector(sector) != sSelectedMilitiaTown) continue;

    // get number of each
    SECTORINFO const &si = SectorInfo[sector];
    int32_t const n_greens = si.ubNumberOfCivsAtLevel[GREEN_MILITIA];
    int32_t const n_regulars = si.ubNumberOfCivsAtLevel[REGULAR_MILITIA];
    int32_t const n_elites = si.ubNumberOfCivsAtLevel[ELITE_MILITIA];
    int32_t const n_total = n_greens + n_regulars + n_elites;

    StrategicMapElement const &e = StrategicMap[SECTOR_INFO_TO_STRATEGIC_INDEX(sector)];
    if (e.bNameId != BLANK_SECTOR && !e.fEnemyControlled) {
      // print number of troops
      SetFont(FONT10ARIAL);
      wchar_t sString[32];
      swprintf(sString, lengthof(sString), L"%d", n_total);
      int16_t sX;
      int16_t sY;
      int16_t const x = MAP_MILITIA_BOX_POS_X + MAP_MILITIA_MAP_X + dx * MILITIA_BOX_BOX_WIDTH;
      int16_t const y = MAP_MILITIA_BOX_POS_Y + MAP_MILITIA_MAP_Y + dy * MILITIA_BOX_BOX_HEIGHT;
      FindFontRightCoordinates(x, y, MILITIA_BOX_BOX_WIDTH, 0, sString, FONT10ARIAL, &sX, &sY);
      MPrint(sSectorMilitiaMapSector != iCounter ? sX : sX - 15, sY + MILITIA_BOX_BOX_HEIGHT - 5,
             sString);
    }

    // now display
    for (int32_t i = 0; i < n_total; ++i) {
      // get screen x and y coords
      int16_t x = MAP_MILITIA_BOX_POS_X + MAP_MILITIA_MAP_X + dx * MILITIA_BOX_BOX_WIDTH;
      int16_t y = MAP_MILITIA_BOX_POS_Y + MAP_MILITIA_MAP_Y + dy * MILITIA_BOX_BOX_HEIGHT;
      uint16_t icon_base;
      if (sSectorMilitiaMapSector == iCounter) {
        x += i % POPUP_MILITIA_ICONS_PER_ROW * MEDIUM_MILITIA_ICON_SPACING + 2;
        y += i / POPUP_MILITIA_ICONS_PER_ROW * (MEDIUM_MILITIA_ICON_SPACING - 1) + 3;
        icon_base = 5;
      } else {
        x += i % POPUP_MILITIA_ICONS_PER_ROW * MEDIUM_MILITIA_ICON_SPACING + 3;
        y += i / POPUP_MILITIA_ICONS_PER_ROW * MEDIUM_MILITIA_ICON_SPACING + 3;
        icon_base = 8;
      }

      uint16_t const icon = i < n_greens                ? icon_base
                            : i < n_greens + n_regulars ? icon_base + 1
                                                        : icon_base + 2;
      BltVideoObject(FRAME_BUFFER, guiMilitia, icon, x, y);
    }
  }
}

// Get the sector value for the upper left corner
static int16_t GetBaseSectorForCurrentTown() {
  int16_t sBaseSector = 0;

  // is the current town
  if (sSelectedMilitiaTown != 0) {
    sBaseSector = sBaseSectorList[(int16_t)(sSelectedMilitiaTown - 1)];
  }

  // return the current sector value
  return (sBaseSector);
}

static void ShowHighLightedSectorOnMilitiaMap() {
  // show the highlighted sector on the militia map
  int16_t sX = 0, sY = 0;

  if (sSectorMilitiaMapSector != -1) {
    sX = MAP_MILITIA_BOX_POS_X + MAP_MILITIA_MAP_X +
         ((sSectorMilitiaMapSector % MILITIA_BOX_ROWS) * MILITIA_BOX_BOX_WIDTH);
    sY = MAP_MILITIA_BOX_POS_Y + MAP_MILITIA_MAP_Y +
         ((sSectorMilitiaMapSector / MILITIA_BOX_ROWS) * MILITIA_BOX_BOX_HEIGHT);
    BltVideoObject(FRAME_BUFFER, guiMilitiaSectorHighLight, 0, sX, sY);
  }

  if (sSectorMilitiaMapSectorOutline != -1) {
    sX = MAP_MILITIA_BOX_POS_X + MAP_MILITIA_MAP_X +
         ((sSectorMilitiaMapSectorOutline % MILITIA_BOX_ROWS) * MILITIA_BOX_BOX_WIDTH);
    sY = MAP_MILITIA_BOX_POS_Y + MAP_MILITIA_MAP_Y +
         ((sSectorMilitiaMapSectorOutline / MILITIA_BOX_ROWS) * MILITIA_BOX_BOX_HEIGHT);
    BltVideoObject(FRAME_BUFFER, guiMilitiaSectorOutline, 0, sX, sY);
  }
}

static bool IsThisMilitiaTownSectorAllowable(int16_t sSectorIndexValue);

static void MilitiaRegionClickCallback(MOUSE_REGION *const r, int32_t const reason) {
  if (reason & MSYS_CALLBACK_REASON_LBUTTON_UP) {
    int16_t const val = MSYS_GetRegionUserData(r, 0);
    sSectorMilitiaMapSector = !IsThisMilitiaTownSectorAllowable(val) ? -1
                              : sSectorMilitiaMapSector == val       ? -1
                                                                     : val;
  } else if (reason & MSYS_CALLBACK_REASON_RBUTTON_UP) {
    sSectorMilitiaMapSector = -1;
  }
}

static void MilitiaRegionMoveCallback(MOUSE_REGION *const r, int32_t const reason) {
  if (reason & MSYS_CALLBACK_REASON_GAIN_MOUSE) {
    int16_t const val = MSYS_GetRegionUserData(r, 0);
    sSectorMilitiaMapSectorOutline = !IsThisMilitiaTownSectorAllowable(val) ? -1 : val;
  } else if (reason & MSYS_CALLBACK_REASON_LOST_MOUSE) {
    sSectorMilitiaMapSectorOutline = -1;
  }
}

static void MilitiaButtonCallback(GUI_BUTTON *btn, int32_t reason);

void CreateDestroyMilitiaSectorButtons() {
  static int16_t sOldSectorValue = -1;

  if (!fMilitiaMapButtonsCreated && sOldSectorValue == sSectorMilitiaMapSector && fShowMilitia &&
      sSelectedMilitiaTown && sSectorMilitiaMapSector != -1) {
    // given sector..place down the 3 buttons
    int16_t const x = MAP_MILITIA_BOX_POS_X + MAP_MILITIA_MAP_X +
                      sSectorMilitiaMapSector % MILITIA_BOX_ROWS * MILITIA_BOX_BOX_WIDTH +
                      MILITIA_BTN_OFFSET_X;
    int16_t y = MAP_MILITIA_BOX_POS_Y + MAP_MILITIA_MAP_Y +
                sSectorMilitiaMapSector / MILITIA_BOX_ROWS * MILITIA_BOX_BOX_HEIGHT + 2;
    for (int32_t i = 0; i != 3; y += MILITIA_BTN_HEIGHT, ++i) {
      GUIButtonRef b = QuickCreateButtonImg(INTERFACEDIR "/militia.sti", 3, 4, x, y,
                                            MSYS_PRIORITY_HIGHEST - 1, MilitiaButtonCallback);
      giMapMilitiaButton[i] = b;
      b->SetUserData(i);
      b->SpecifyGeneralTextAttributes(0, FONT10ARIAL, gsMilitiaSectorButtonColors[i], FONT_BLACK);
      b->SpecifyTextSubOffsets(0, 0, TRUE);
      b->fShiftText = FALSE;
      b->SetFastHelpText(pMilitiaButtonsHelpText[i]);
    }

    CreateScreenMaskForMoveBox();

    // Set the fact that the buttons are created
    fMilitiaMapButtonsCreated = true;
  } else if (fMilitiaMapButtonsCreated &&
             (sOldSectorValue != sSectorMilitiaMapSector || !fShowMilitia ||
              !sSelectedMilitiaTown || sSectorMilitiaMapSector == -1)) {
    sOldSectorValue = sSectorMilitiaMapSector;

    // get rid of the buttons
    for (int32_t i = 0; i != 3; ++i) {
      RemoveButton(giMapMilitiaButton[i]);
    }

    if (!fShowMilitia || !sSelectedMilitiaTown) {
      sSectorMilitiaMapSector = -1;
      sSelectedMilitiaTown = 0;
    }

    RemoveScreenMaskForMoveBox();

    // Set the fact that the buttons are destroyed
    fMilitiaMapButtonsCreated = false;
  }

  sOldSectorValue = sSectorMilitiaMapSector;
}

static void SetMilitiaMapButtonsText() {
  if (!fMilitiaMapButtonsCreated) return;

  // grab the appropriate global sector value in the world
  int16_t const base_sector = GetBaseSectorForCurrentTown();
  int16_t const sector = base_sector + (sSectorMilitiaMapSector % MILITIA_BOX_ROWS +
                                        sSectorMilitiaMapSector / MILITIA_BOX_ROWS * 16);
  SECTORINFO const &si = SectorInfo[sector];
  wchar_t buf[64];

  // the greens in this sector
  swprintf(buf, lengthof(buf), L"%d", si.ubNumberOfCivsAtLevel[GREEN_MILITIA]);
  giMapMilitiaButton[0]->SpecifyText(buf);

  // the regulars in this sector
  swprintf(buf, lengthof(buf), L"%d", si.ubNumberOfCivsAtLevel[REGULAR_MILITIA]);
  giMapMilitiaButton[1]->SpecifyText(buf);

  // the number of elites in this sector
  swprintf(buf, lengthof(buf), L"%d", si.ubNumberOfCivsAtLevel[ELITE_MILITIA]);
  giMapMilitiaButton[2]->SpecifyText(buf);
}

static void MilitiaButtonCallback(GUI_BUTTON *btn, int32_t reason) {
  int32_t const iValue = btn->GetUserData();

  // get the sector value for the upper left corner
  int16_t sBaseSectorValue = GetBaseSectorForCurrentTown();
  int16_t sGlobalMapSector = sBaseSectorValue + sSectorMilitiaMapSector % MILITIA_BOX_ROWS +
                             sSectorMilitiaMapSector / MILITIA_BOX_ROWS * 16;

  if (reason & MSYS_CALLBACK_REASON_LBUTTON_UP) {
    DropAPersonInASector(iValue, sGlobalMapSector);
  } else if (reason & MSYS_CALLBACK_REASON_RBUTTON_UP) {
    PickUpATownPersonFromSector(iValue, sGlobalMapSector);
  }
}

static void DisplayUnallocatedMilitia() {
  // show the nunber on the cursor
  int32_t iTotalNumberOfTroops = 0, iNumberOfGreens = 0, iNumberOfRegulars = 0, iNumberOfElites = 0,
          iCurrentTroopIcon = 0;
  int32_t iCurrentIcon = 0;
  int16_t sX = 0, sY = 0;

  // get number of each
  iNumberOfGreens = sGreensOnCursor;
  iNumberOfRegulars = sRegularsOnCursor;
  iNumberOfElites = sElitesOnCursor;

  // get total
  iTotalNumberOfTroops = iNumberOfGreens + iNumberOfRegulars + iNumberOfElites;

  // now display
  for (iCurrentTroopIcon = 0; iCurrentTroopIcon < iTotalNumberOfTroops; iCurrentTroopIcon++) {
    // get screen x and y coords
    sX = (iCurrentTroopIcon % NUMBER_OF_MILITIA_ICONS_PER_LOWER_ROW) * MEDIUM_MILITIA_ICON_SPACING +
         MAP_MILITIA_BOX_POS_X + MAP_MILITIA_MAP_X + 1;
    sY = (iCurrentTroopIcon / NUMBER_OF_MILITIA_ICONS_PER_LOWER_ROW) * MEDIUM_MILITIA_ICON_SPACING +
         MAP_MILITIA_BOX_POS_Y + MAP_MILITIA_LOWER_ROW_Y;

    if (iCurrentTroopIcon < iNumberOfGreens) {
      iCurrentIcon = 8;
    } else if (iCurrentTroopIcon < iNumberOfGreens + iNumberOfRegulars) {
      iCurrentIcon = 9;
    } else {
      iCurrentIcon = 10;
    }

    BltVideoObject(FRAME_BUFFER, guiMilitia, iCurrentIcon, sX, sY);
  }
}

// Is this sector allowed to be clicked on?
static bool IsThisMilitiaTownSectorAllowable(int16_t const sSectorIndexValue) {
  int16_t const base_sector = GetBaseSectorForCurrentTown();
  int16_t const sector = base_sector + sSectorIndexValue % MILITIA_BOX_ROWS +
                         sSectorIndexValue / MILITIA_BOX_ROWS * 16;
  int16_t const x = SECTORX(sector);
  int16_t const y = SECTORY(sector);
  return StrategicMap[CALCULATE_STRATEGIC_INDEX(x, y)].bNameId != BLANK_SECTOR &&
         SectorOursAndPeaceful(x, y, 0);
}

static void DrawTownMilitiaName() {
  wchar_t const *const town = pTownNames[sSelectedMilitiaTown];
  int16_t const x = MAP_MILITIA_BOX_POS_X;
  int16_t const y = MAP_MILITIA_BOX_POS_Y;
  int16_t const w = MILITIA_BOX_WIDTH;
  wchar_t buf[64];
  int16_t sX;
  int16_t sY;

  // get the name for the current militia town
  swprintf(buf, lengthof(buf), L"%ls %ls", town, pMilitiaString[0]);
  FindFontCenterCoordinates(x, y + MILITIA_BOX_TEXT_OFFSET_Y, w, MILITIA_BOX_TEXT_TITLE_HEIGHT, buf,
                            FONT10ARIAL, &sX, &sY);
  MPrint(sX, sY, buf);

  // might as well show the unassigned string
  swprintf(buf, lengthof(buf), L"%ls %ls", town, pMilitiaString[1]);
  FindFontCenterCoordinates(x, y + MILITIA_BOX_UNASSIGNED_TEXT_OFFSET_Y, w,
                            GetFontHeight(FONT10ARIAL), buf, FONT10ARIAL, &sX, &sY);
  MPrint(sX, sY, buf);
}

static void HandleShutDownOfMilitiaPanelIfPeopleOnTheCursor(int16_t const town) {
  // check if anyone still on the cursor
  if (sGreensOnCursor == 0 && sRegularsOnCursor == 0 && sElitesOnCursor == 0) return;

  FOR_EACH_SECTOR_IN_TOWN(i, town) {
    int32_t const sector = i->sector;
    if (!SectorOursAndPeaceful(SECTORX(sector), SECTORY(sector), 0)) continue;
    SECTORINFO &si = SectorInfo[sector];
    uint8_t &n_green = si.ubNumberOfCivsAtLevel[GREEN_MILITIA];
    uint8_t &n_regular = si.ubNumberOfCivsAtLevel[REGULAR_MILITIA];
    uint8_t &n_elite = si.ubNumberOfCivsAtLevel[ELITE_MILITIA];
    int32_t n = MAX_ALLOWABLE_MILITIA_PER_SECTOR - n_green - n_regular - n_elite;
    if (n == 0) continue;

    while (sGreensOnCursor != 0 || sRegularsOnCursor != 0 || sElitesOnCursor != 0) {
      if (sGreensOnCursor != 0) {
        ++n_green;
        --sGreensOnCursor;
        if (--n == 0) break;
      }

      if (sRegularsOnCursor != 0) {
        ++n_regular;
        --sRegularsOnCursor;
        if (--n == 0) break;
      }

      if (sElitesOnCursor != 0) {
        ++n_elite;
        --sElitesOnCursor;
        if (--n == 0) break;
      }
    }

    if (sector == GetWorldSector()) gfStrategicMilitiaChangesMade = TRUE;
  }

  Assert(sGreensOnCursor == 0);
  Assert(sRegularsOnCursor == 0);
  Assert(sElitesOnCursor == 0);
}

// Even out troops among the town
static void HandleEveningOutOfTroopsAmongstSectors() {
  int8_t const town = sSelectedMilitiaTown;

  // How many sectors in the selected town do we control?
  int32_t const n_under_control = GetTownSectorsUnderControl(town);
  if (n_under_control == 0) return;  // If none, there's nothing to be done

  int32_t n_green = 0;
  int32_t n_regular = 0;
  int32_t n_elite = 0;
  int16_t const base_sector = GetBaseSectorForCurrentTown();
  for (int32_t i = 0; i != 9; ++i) {
    int16_t const sector = base_sector + i % MILITIA_BOX_ROWS + i / MILITIA_BOX_ROWS * 16;

    // Skip sectors not in the selected town (nearby other towns or wilderness
    // SAM Sites)
    if (GetTownIdForSector(sector) != town) continue;

    if (StrategicMap[SECTOR_INFO_TO_STRATEGIC_INDEX(sector)].fEnemyControlled) continue;

    SECTORINFO const &si = SectorInfo[sector];
    n_green += si.ubNumberOfCivsAtLevel[GREEN_MILITIA];
    n_regular += si.ubNumberOfCivsAtLevel[REGULAR_MILITIA];
    n_elite += si.ubNumberOfCivsAtLevel[ELITE_MILITIA];
  }

  // Grab those on the cursor
  n_green += sGreensOnCursor;
  n_regular += sRegularsOnCursor;
  n_elite += sElitesOnCursor;

  if (n_green + n_regular + n_elite == 0) return;

  int32_t const n_green_per_sector = n_green / n_under_control;
  int32_t const n_regular_per_sector = n_regular / n_under_control;
  int32_t const n_elite_per_sector = n_elite / n_under_control;

  // Get the left overs
  int32_t n_left_over_green = n_green % n_under_control;
  int32_t n_left_over_regular = n_regular % n_under_control;
  int32_t n_left_over_elite = n_elite % n_under_control;

  FOR_EACH_SECTOR_IN_TOWN(i, town) {
    int32_t const sector = i->sector;
    if (StrategicMap[SECTOR_INFO_TO_STRATEGIC_INDEX(sector)].fEnemyControlled) continue;
    if (NumHostilesInSector(SECTORX(sector), SECTORY(sector), 0) != 0) continue;

    SECTORINFO &si = SectorInfo[sector];

    // Distribute here
    si.ubNumberOfCivsAtLevel[GREEN_MILITIA] = n_green_per_sector;
    si.ubNumberOfCivsAtLevel[REGULAR_MILITIA] = n_regular_per_sector;
    si.ubNumberOfCivsAtLevel[ELITE_MILITIA] = n_elite_per_sector;

    // Add leftovers that weren't included in the div operation
    int16_t total_so_far = n_green_per_sector + n_regular_per_sector + n_elite_per_sector;
    if (n_left_over_green != 0 && total_so_far < MAX_ALLOWABLE_MILITIA_PER_SECTOR) {
      ++si.ubNumberOfCivsAtLevel[GREEN_MILITIA];
      ++total_so_far;
      --n_left_over_green;
    }
    if (n_left_over_regular != 0 && total_so_far < MAX_ALLOWABLE_MILITIA_PER_SECTOR) {
      ++si.ubNumberOfCivsAtLevel[REGULAR_MILITIA];
      ++total_so_far;
      --n_left_over_regular;
    }
    if (n_left_over_elite && total_so_far < MAX_ALLOWABLE_MILITIA_PER_SECTOR) {
      ++si.ubNumberOfCivsAtLevel[ELITE_MILITIA];
      ++total_so_far;
      --n_left_over_elite;
    }

    if (sector == GetWorldSector()) gfStrategicMilitiaChangesMade = TRUE;
  }

  // Zero out numbers on the cursor
  sGreensOnCursor = 0;
  sRegularsOnCursor = 0;
  sElitesOnCursor = 0;
}

static void MakeButton(uint32_t idx, int16_t x, GUI_CALLBACK click, const wchar_t *text) {
  GUIButtonRef const btn = QuickCreateButtonImg(INTERFACEDIR "/militia.sti", 1, 2, x,
                                                MAP_MILITIA_BOX_POS_Y + MAP_MILITIA_BOX_AUTO_BOX_Y,
                                                MSYS_PRIORITY_HIGHEST - 1, click);
  giMapMilitiaButton[idx] = btn;
  btn->SpecifyGeneralTextAttributes(text, FONT10ARIAL, FONT_BLACK, FONT_BLACK);
}

static bool CanMilitiaAutoDistribute();
static void MilitiaAutoButtonCallback(GUI_BUTTON *btn, int32_t reason);
static void MilitiaDoneButtonCallback(GUI_BUTTON *btn, int32_t reason);

static void CreateMilitiaPanelBottomButton() {
  MakeButton(3, MAP_MILITIA_BOX_POS_X + MAP_MILITIA_BOX_AUTO_BOX_X, MilitiaAutoButtonCallback,
             pMilitiaButtonString[0]);
  MakeButton(4, MAP_MILITIA_BOX_POS_X + MAP_MILITIA_BOX_DONE_BOX_X, MilitiaDoneButtonCallback,
             pMilitiaButtonString[1]);

  // AUTO button help
  giMapMilitiaButton[3]->SetFastHelpText(pMilitiaButtonsHelpText[3]);

  // if auto-distribution is not possible
  if (!CanMilitiaAutoDistribute()) {
    // disable the AUTO button
    DisableButton(giMapMilitiaButton[3]);
  }
}

static void DeleteMilitiaPanelBottomButton() {
  // delete militia panel bottom
  RemoveButton(giMapMilitiaButton[3]);
  RemoveButton(giMapMilitiaButton[4]);

  if (sSelectedMilitiaTown != 0) {
    HandleShutDownOfMilitiaPanelIfPeopleOnTheCursor(sSelectedMilitiaTown);
  }

  // redraw the map
  fMapPanelDirty = TRUE;
}

static void MilitiaAutoButtonCallback(GUI_BUTTON *btn, int32_t reason) {
  if (reason & MSYS_CALLBACK_REASON_LBUTTON_UP) {
    // distribute troops over all the sectors under control
    HandleEveningOutOfTroopsAmongstSectors();
    fMapPanelDirty = TRUE;
  }
}

static void MilitiaDoneButtonCallback(GUI_BUTTON *btn, int32_t reason) {
  if (reason & MSYS_CALLBACK_REASON_LBUTTON_UP) {
    // reset fact we are in the box
    sSelectedMilitiaTown = 0;
    fMapPanelDirty = TRUE;
  }
}

static void RenderShadingForUnControlledSectors() {
  // now render shading over any uncontrolled sectors
  int16_t const sBaseSectorValue = GetBaseSectorForCurrentTown();
  for (int32_t dy = 0; dy != 3; ++dy) {
    for (int32_t dx = 0; dx != 3; ++dx) {
      int32_t const x = SECTORX(sBaseSectorValue) + dx;
      int32_t const y = SECTORY(sBaseSectorValue) + dy;

      StrategicMapElement const &e = StrategicMap[CALCULATE_STRATEGIC_INDEX(x, y)];
      if (e.bNameId == BLANK_SECTOR) continue;
      if (!e.fEnemyControlled && NumHostilesInSector(x, y, 0) == 0) continue;

      // shade this sector, not under our control
      int16_t const sX = MAP_MILITIA_BOX_POS_X + MAP_MILITIA_MAP_X + dx * MILITIA_BOX_BOX_WIDTH;
      int16_t const sY = MAP_MILITIA_BOX_POS_Y + MAP_MILITIA_MAP_Y + dy * MILITIA_BOX_BOX_HEIGHT;
      FRAME_BUFFER->ShadowRect(sX, sY, sX + MILITIA_BOX_BOX_WIDTH - 1,
                               sY + MILITIA_BOX_BOX_HEIGHT - 1);
    }
  }
}

static void DrawMilitiaForcesForSector(int32_t const sector) {
  int16_t const x = SECTORX(sector);
  int16_t const y = SECTORY(sector);

  if (StrategicMap[CALCULATE_STRATEGIC_INDEX(x, y)].fEnemyControlled) return;

  /* Large/small icon offset in the .sti */
  int32_t const icon = fZoomFlag ? 11 : 5;
  int32_t pos = 0;
  SECTORINFO const &si = SectorInfo[sector];
  for (int32_t i = si.ubNumberOfCivsAtLevel[GREEN_MILITIA]; i != 0; --i) {
    DrawMapBoxIcon(guiMilitia, icon, x, y, pos++);
  }
  for (int32_t i = si.ubNumberOfCivsAtLevel[REGULAR_MILITIA]; i != 0; --i) {
    DrawMapBoxIcon(guiMilitia, icon + 1, x, y, pos++);
  }
  for (int32_t i = si.ubNumberOfCivsAtLevel[ELITE_MILITIA]; i != 0; --i) {
    DrawMapBoxIcon(guiMilitia, icon + 2, x, y, pos++);
  }
}

static void DrawTownMilitiaForcesOnMap() {
  ClipBlitsToMapViewRegion();

  FOR_EACH_TOWN_SECTOR(i) { DrawMilitiaForcesForSector(i->sector); }

  // now handle militia for sam sectors
  FOR_EACH(int16_t const, i, pSamList) { DrawMilitiaForcesForSector(*i); }

  RestoreClipRegionToFullScreen();
}

static void CheckAndUpdateStatesOfSelectedMilitiaSectorButtons() {
  if (!fMilitiaMapButtonsCreated) {
    EnableButton(giMapMilitiaButton[4]);
    return;
  }

  int16_t const base_sector = GetBaseSectorForCurrentTown();
  int16_t const sector = base_sector + (sSectorMilitiaMapSector % MILITIA_BOX_ROWS +
                                        sSectorMilitiaMapSector / MILITIA_BOX_ROWS * 16);
  SECTORINFO const &si = SectorInfo[sector];
  int32_t const n_green = si.ubNumberOfCivsAtLevel[GREEN_MILITIA] + sGreensOnCursor;
  int32_t const n_regular = si.ubNumberOfCivsAtLevel[REGULAR_MILITIA] + sRegularsOnCursor;
  int32_t const n_elite = si.ubNumberOfCivsAtLevel[ELITE_MILITIA] + sElitesOnCursor;
  EnableButton(giMapMilitiaButton[4],
               sGreensOnCursor + sRegularsOnCursor + sElitesOnCursor == 0);  // Done
  EnableButton(giMapMilitiaButton[0], n_green != 0);                         // greens button
  EnableButton(giMapMilitiaButton[1], n_regular != 0);                       // regulars button
  EnableButton(giMapMilitiaButton[2], n_elite != 0);                         // elites button
}

static void HideExistenceOfUndergroundMapSector(uint8_t ubSectorX, uint8_t ubSectorY);

static void ShadeSubLevelsNotVisited() {
  // Obtain the 16-bit version of the same color used in the mine STIs
  gusUndergroundNearBlack = Get16BPPColor(FROMRGB(2, 2, 0));

  // Run through all (real & possible) underground sectors
  for (UNDERGROUND_SECTORINFO const *i = gpUndergroundSectorInfoHead; i; i = i->next) {
    if (i->ubSectorZ != (uint8_t)iCurrentMapSectorZ) continue;
    if (i->uiFlags & SF_ALREADY_VISITED) continue;
    /* The sector is on the currently displayed sublevel and has never been
     * visited.  Remove that portion of the "mine" graphics from view. */
    HideExistenceOfUndergroundMapSector(i->ubSectorX, i->ubSectorY);
  }
}

static void HandleLowerLevelMapBlit() {
  // blits the sub level maps
  const SGPVObject *vo;  // XXX HACK000E
  switch (iCurrentMapSectorZ) {
    case 1:
      vo = guiSubLevel1;
      break;
    case 2:
      vo = guiSubLevel2;
      break;
    case 3:
      vo = guiSubLevel3;
      break;

    default:
      abort();  // HACK000E
  }

  // handle the blt of the sublevel
  BltVideoObject(guiSAVEBUFFER, vo, 0, MAP_VIEW_START_X + 21, MAP_VIEW_START_Y + 17);

  // handle shading of sublevels
  ShadeSubLevelsNotVisited();
}

int32_t GetNumberOfMilitiaInSector(int16_t sSectorX, int16_t sSectorY, int8_t bSectorZ) {
  int32_t iNumberInSector = 0;

  if (!bSectorZ) {
    iNumberInSector =
        SectorInfo[SECTOR(sSectorX, sSectorY)].ubNumberOfCivsAtLevel[GREEN_MILITIA] +
        SectorInfo[SECTOR(sSectorX, sSectorY)].ubNumberOfCivsAtLevel[REGULAR_MILITIA] +
        SectorInfo[SECTOR(sSectorX, sSectorY)].ubNumberOfCivsAtLevel[ELITE_MILITIA];
  }

  return (iNumberInSector);
}

// There is a special case flag used when players encounter enemies in a sector,
// then retreat.  The number of enemies will display on mapscreen until time is
// compressed.  When time is compressed, the flag is cleared, and a question mark
// is displayed to reflect that the player no longer knows.  This is the function
// that clears that flag.
void ClearAnySectorsFlashingNumberOfEnemies() {
  int32_t i;
  for (i = 0; i < 256; i++) {
    SectorInfo[i].uiFlags &= ~SF_PLAYER_KNOWS_ENEMIES_ARE_HERE;
  }

  // redraw map
  fMapPanelDirty = TRUE;
}

static BOOLEAN CanMercsScoutThisSector(int16_t sSectorX, int16_t sSectorY, int8_t bSectorZ);

uint32_t WhatPlayerKnowsAboutEnemiesInSector(int16_t sSectorX, int16_t sSectorY) {
  uint32_t uiSectorFlags = SectorInfo[SECTOR(sSectorX, sSectorY)].uiFlags;

  // if player has militia close enough to scout this sector out, if there are
  // mercs who can scout here, OR
  // Special case flag used when players encounter enemies in a sector, then
  // retreat.  The number of enemies will display on mapscreen until time is
  // compressed.  When time is compressed, the flag is cleared, and a question
  // mark is displayed to reflect that the player no longer knows.
  if (CanMercsScoutThisSector(sSectorX, sSectorY, 0) ||
      CanNearbyMilitiaScoutThisSector(sSectorX, sSectorY) ||
      (uiSectorFlags & SF_PLAYER_KNOWS_ENEMIES_ARE_HERE)) {
    // if the enemies are stationary (i.e. mercs attacking a garrison)
    if (NumStationaryEnemiesInSector(sSectorX, sSectorY) > 0) {
      // inside a garrison - hide their # (show question mark) to match what the
      // PBI is showing
      return KNOWS_THEYRE_THERE;
    } else {
      // other situations - show exactly how many there are
      return KNOWS_HOW_MANY;
    }
  }

  // if the player has visited the sector during this game
  if (GetSectorFlagStatus(sSectorX, sSectorY, 0, SF_ALREADY_VISITED)) {
    // then he always knows about any enemy presence for the remainder of the
    // game, but not exact numbers
    return KNOWS_THEYRE_THERE;
  }

  // if Skyrider noticed the enemis in the sector recently
  if (uiSectorFlags & SF_SKYRIDER_NOTICED_ENEMIES_HERE) {
    // and Skyrider is still in this sector, flying
    if (IsSkyriderIsFlyingInSector(sSectorX, sSectorY)) {
      // player remains aware of them as long as Skyrider remains in the sector
      return KNOWS_THEYRE_THERE;
    } else {
      // Skyrider is gone, reset the flag that he noticed enemies here
      SectorInfo[SECTOR(sSectorX, sSectorY)].uiFlags &= ~SF_SKYRIDER_NOTICED_ENEMIES_HERE;
    }
  }

  // no information available
  return KNOWS_NOTHING;
}

static BOOLEAN CanMercsScoutThisSector(int16_t sSectorX, int16_t sSectorY, int8_t bSectorZ) {
  CFOR_EACH_IN_TEAM(pSoldier, OUR_TEAM) {
    // vehicles can't scout!
    if (pSoldier->uiStatusFlags & SOLDIER_VEHICLE) {
      continue;
    }

    // POWs, dead guys, guys in transit, sleeping, and really hurt guys can't
    // scout!
    if ((pSoldier->bAssignment == IN_TRANSIT) || (pSoldier->bAssignment == ASSIGNMENT_POW) ||
        (pSoldier->bAssignment == ASSIGNMENT_DEAD) || pSoldier->fMercAsleep ||
        (pSoldier->bLife < OKLIFE)) {
      continue;
    }

    // don't count mercs aboard Skyrider
    if (InHelicopter(*pSoldier)) continue;

    // mercs on the move can't scout
    if (pSoldier->fBetweenSectors) {
      continue;
    }

    // is he here?
    if ((pSoldier->sSectorX == sSectorX) && (pSoldier->sSectorY == sSectorY) &&
        (pSoldier->bSectorZ == bSectorZ)) {
      return (TRUE);
    }
  }

  // none here who can scout
  return (FALSE);
}

static void HandleShowingOfEnemyForcesInSector(int16_t const x, int16_t const y, int8_t const z,
                                               uint8_t const icon_pos) {
  // ATE: If game has just started, don't do it
  if (DidGameJustStart()) return;

  // Never display enemies underground - sector info doesn't have support for it
  if (z != 0) return;

  int16_t const n_enemies = NumEnemiesInSector(x, y);
  if (n_enemies == 0) return;  // No enemies here, display nothing

  switch (WhatPlayerKnowsAboutEnemiesInSector(x, y)) {
    case KNOWS_NOTHING:  // Display nothing
      break;

    case KNOWS_THEYRE_THERE:  // Display a question mark
      ShowUncertainNumberEnemiesInSector(x, y);
      break;

    case KNOWS_HOW_MANY:
      /* Display individual icons for each enemy, starting at the received icon
       * position index */
      ShowEnemiesInSector(x, y, n_enemies, icon_pos);
      break;
  }
}

static void BlitSAMGridMarkers();

static void ShowSAMSitesOnStrategicMap() {
  if (fShowAircraftFlag) BlitSAMGridMarkers();

  BOOLEAN const *found = fSamSiteFound;
  FOR_EACH(int16_t const, i, pSamList) {
    // Has the sam site here been found?
    if (!*found++) continue;

    int16_t const sector = *i;
    int16_t const sec_x = SECTORX(sector);
    int16_t const sec_y = SECTORY(sector);

    DrawSite(sec_x, sec_y, guiSAMICON);

    if (fShowAircraftFlag) {  // write "SAM Site" centered underneath
      int16_t x;
      int16_t y;
      if (fZoomFlag) {
        GetScreenXYFromMapXYStationary(sec_x, sec_y, &x, &y);
        x += 1;
        y += 9;
      } else {
        GetScreenXYFromMapXY(sec_x, sec_y, &x, &y);
        x += 11;
        y += 19;
      }

      wchar_t const *const sam_site = pLandTypeStrings[SAM_SITE];

      // Center the first string around x.
      x -= StringPixLength(sam_site, MAP_FONT) / 2;

      if (x < MAP_VIEW_START_X || MAP_VIEW_START_X + MAP_VIEW_WIDTH < x) continue;
      if (y < MAP_VIEW_START_Y || MAP_VIEW_START_Y + MAP_VIEW_HEIGHT < y) continue;
      // Within view, render.

      SetFontDestBuffer(guiSAVEBUFFER, MapScreenRect.iLeft + 2, MapScreenRect.iTop,
                        MapScreenRect.iRight, MapScreenRect.iBottom);

      ClipBlitsToMapViewRegion();

      // Green on green does not contrast well, use yellow.
      SetFontAttributes(MAP_FONT, FONT_MCOLOR_LTYELLOW);
      GDirtyPrint(x, y, sam_site);

      RestoreClipRegionToFullScreen();
    }
  }
}

static void BlitSAMGridMarkers() {
  uint16_t const colour = Get16BPPColor(FROMRGB(100, 100, 100));

  SGPVSurface::Lock l(guiSAVEBUFFER);
  uint32_t const uiDestPitchBYTES = l.Pitch();

  ClipBlitsToMapViewRegionForRectangleAndABit(uiDestPitchBYTES);

  BOOLEAN const *found = fSamSiteFound;
  FOR_EACH(int16_t const, i, pSamList) {
    // Has the sam site here been found?
    if (!*found++) continue;

    int16_t x;
    int16_t y;
    int16_t w;
    int16_t h;
    int16_t const sector = *i;
    if (fZoomFlag) {
      GetScreenXYFromMapXYStationary(SECTORX(sector), SECTORY(sector), &x, &y);
      x -= MAP_GRID_X;
      y -= MAP_GRID_Y;
      w = MAP_GRID_X * 2;
      h = MAP_GRID_Y * 2;
    } else {
      GetScreenXYFromMapXY(SECTORX(sector), SECTORY(sector), &x, &y);
      w = MAP_GRID_X;
      h = MAP_GRID_Y;
    }

    RectangleDraw(TRUE, x, y - 1, x + w, y + h - 1, colour, l.Buffer<uint16_t>());
  }

  RestoreClipRegionToFullScreenForRectangle(uiDestPitchBYTES);
}

static bool CanMilitiaAutoDistribute() {
  int8_t const town = sSelectedMilitiaTown;

  // Cannot auto-distribute, if we don't have a town selected. (this excludes
  // SAM sites)
  if (town == BLANK_SECTOR) return false;

  FOR_EACH_SECTOR_IN_TOWN(i, town) {
    int16_t const sector = i->sector;
    if (StrategicMap[SECTOR_INFO_TO_STRATEGIC_INDEX(sector)].fEnemyControlled) continue;

    uint8_t const(&n_milita)[MAX_MILITIA_LEVELS] = SectorInfo[sector].ubNumberOfCivsAtLevel;
    if (n_milita[GREEN_MILITIA] + n_milita[REGULAR_MILITIA] + n_milita[ELITE_MILITIA] != 0)
      return true;
  }

  // No militia in town, cannot auto-distribute.
  return false;
}

static void ShowItemsOnMap() {
  ClipBlitsToMapViewRegion();
  SetFontDestBuffer(guiSAVEBUFFER, MapScreenRect.iLeft + 2, MapScreenRect.iTop,
                    MapScreenRect.iRight, MapScreenRect.iBottom);
  SetFontAttributes(MAP_FONT, FONT_MCOLOR_LTGREEN);

  // run through sectors
  for (int16_t x = 1; x < MAP_WORLD_X - 1; ++x) {
    for (int16_t y = 1; y < MAP_WORLD_Y - 1; ++y) {
      // to speed this up, only look at sector that player has visited
      if (!GetSectorFlagStatus(x, y, iCurrentMapSectorZ, SF_ALREADY_VISITED)) continue;

      uint32_t const n_items =
          GetNumberOfVisibleWorldItemsFromSectorStructureForSector(x, y, iCurrentMapSectorZ);
      if (n_items == 0) continue;

      int16_t usXPos;
      int16_t usYPos;
      int16_t const sXCorner = MAP_VIEW_START_X + x * MAP_GRID_X;
      int16_t const sYCorner = MAP_VIEW_START_Y + y * MAP_GRID_Y;
      wchar_t sString[10];
      swprintf(sString, lengthof(sString), L"%d", n_items);
      FindFontCenterCoordinates(sXCorner, sYCorner, MAP_GRID_X, MAP_GRID_Y, sString, MAP_FONT,
                                &usXPos, &usYPos);
      GDirtyPrint(usXPos, usYPos, sString);
    }
  }

  RestoreClipRegionToFullScreen();
}

static void DrawMapBoxIcon(HVOBJECT const vo, uint16_t const icon, int16_t const sec_x,
                           int16_t const sec_y, uint8_t const icon_pos) {
  /* Don't show any more icons than will fit into one sector, to keep them from
   * spilling into sector(s) beneath */
  if (icon_pos >= MERC_ICONS_PER_LINE * ROWS_PER_SECTOR) return;

  int32_t const col = icon_pos % MERC_ICONS_PER_LINE;
  int32_t const row = icon_pos / MERC_ICONS_PER_LINE;
  if (!fZoomFlag) {
    int32_t const x = MAP_VIEW_START_X + sec_x * MAP_GRID_X + MAP_X_ICON_OFFSET + 3 * col;
    int32_t const y = MAP_VIEW_START_Y + sec_y * MAP_GRID_Y + MAP_Y_ICON_OFFSET + 3 * row;
    BltVideoObject(guiSAVEBUFFER, vo, icon, x, y);
    InvalidateRegion(x, y, x + DMAP_GRID_X, y + DMAP_GRID_Y);
  }
#if 0  // XXX was commented out
	else
	{
		int32_t sX;
		int32_t sY;
		GetScreenXYFromMapXYStationary((uint16_t)sX, (uint16_t)sY, &sX, &sY); // XXX first two parameters should be sector x/y
		int32_t const x = MAP_X_ICON_OFFSET + sX - MAP_GRID_X + 6 * col + 2;
 	 	int32_t const y = MAP_Y_ICON_OFFSET + sY - MAP_GRID_Y + 6 * row;

		ClipBlitsToMapViewRegion();
		BltVideoObject(guiSAVEBUFFER, vo, BIG_YELLOW_BOX, x, y); // XXX should use icon instead of hardcoding BIG_YELLOW_BOX
		RestoreClipRegionToFullScreen();
		InvalidateRegion(x, y, x + DMAP_GRID_ZOOM_X, y + DMAP_GRID_ZOOM_Y);
	}
#endif
}

static void DrawOrta() { DrawSite(ORTA_SECTOR_X, ORTA_SECTOR_Y, guiORTAICON); }

static void DrawTixa() { DrawSite(TIXA_SECTOR_X, TIXA_SECTOR_Y, guiTIXAICON); }

static void DrawBullseye() {
  int16_t sX, sY;

  GetScreenXYFromMapXY(SECTORX(g_merc_arrive_sector), SECTORY(g_merc_arrive_sector), &sX, &sY);
  sY -= 2;

  BltVideoObject(guiSAVEBUFFER, guiBULLSEYE, 0, sX, sY);
}

static void HideExistenceOfUndergroundMapSector(uint8_t ubSectorX, uint8_t ubSectorY) {
  int16_t sScreenX;
  int16_t sScreenY;

  GetScreenXYFromMapXY(ubSectorX, ubSectorY, &sScreenX, &sScreenY);

  // fill it with near black
  ColorFillVideoSurfaceArea(guiSAVEBUFFER, sScreenX + 1, sScreenY, sScreenX + MAP_GRID_X,
                            sScreenY + MAP_GRID_Y - 1, gusUndergroundNearBlack);
}

void InitMapSecrets() {
  fFoundTixa = FALSE;
  fFoundOrta = FALSE;
  FOR_EACH(BOOLEAN, i, fSamSiteFound) *i = FALSE;
}

BOOLEAN CanRedistributeMilitiaInSector(int16_t sClickedSectorX, int16_t sClickedSectorY,
                                       int8_t bClickedTownId) {
  int32_t iCounter = 0;
  int16_t sBaseSectorValue = 0, sCurrentSectorValue = 0;
  int16_t sSectorX = 0, sSectorY = 0;

  // if no world is loaded, we can't be in combat (PBI/Auto-resolve locks out
  // normal mapscreen interface for this)
  if (!gfWorldLoaded) {
    // ok to redistribute
    return (TRUE);
  }

  // if tactically not in combat, hostile sector, or air-raid
  if (!(gTacticalStatus.uiFlags & INCOMBAT) && !(gTacticalStatus.fEnemyInSector) && !InAirRaid()) {
    // ok to redistribute
    return (TRUE);
  }

  // if the fight is underground
  if (gbWorldSectorZ != 0) {
    // ok to redistribute
    return (TRUE);
  }

  // currently loaded surface sector IS hostile - so we must check if it's also
  // one of the sectors in this "militia map"

  // get the sector value for the upper left corner
  sBaseSectorValue = sBaseSectorList[bClickedTownId - 1];

  // render icons for map
  for (iCounter = 0; iCounter < 9; iCounter++) {
    // grab current sector value
    sCurrentSectorValue =
        sBaseSectorValue + ((iCounter % MILITIA_BOX_ROWS) + (iCounter / MILITIA_BOX_ROWS) * (16));

    sSectorX = SECTORX(sCurrentSectorValue);
    sSectorY = SECTORY(sCurrentSectorValue);

    // not in the same town?
    if (StrategicMap[CALCULATE_STRATEGIC_INDEX(sSectorX, sSectorY)].bNameId != bClickedTownId) {
      continue;
    }

    // if this is the loaded sector that is currently hostile
    if ((sSectorX == gWorldSectorX) && (sSectorY == gWorldSectorY)) {
      // the fight is within this town!  Can't redistribute.
      return (FALSE);
    }
  }

  // the fight is elsewhere - ok to redistribute
  return (TRUE);
}
