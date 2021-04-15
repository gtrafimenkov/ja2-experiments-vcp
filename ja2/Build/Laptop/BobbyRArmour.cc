#include "Laptop/BobbyRArmour.h"

#include "Directories.h"
#include "Laptop/BobbyR.h"
#include "Laptop/BobbyRGuns.h"
#include "Laptop/Laptop.h"
#include "SGP/ButtonSystem.h"
#include "SGP/VObject.h"
#include "SGP/VSurface.h"
#include "SGP/Video.h"
#include "Utils/Text.h"

static SGPVObject *guiArmourBackground;
static SGPVObject *guiArmourGrid;

void EnterBobbyRArmour() {
  // load the background graphic and add it
  guiArmourBackground = AddVideoObjectFromFile(LAPTOPDIR "/armourbackground.sti");

  // load the gunsgrid graphic and add it
  guiArmourGrid = AddVideoObjectFromFile(LAPTOPDIR "/armourgrid.sti");

  InitBobbyBrTitle();
  // Draw menu bar
  InitBobbyMenuBar();

  SetFirstLastPagesForNew(IC_ARMOUR);

  RenderBobbyRArmour();
}

void ExitBobbyRArmour() {
  DeleteVideoObject(guiArmourBackground);
  DeleteVideoObject(guiArmourGrid);
  DeleteBobbyMenuBar();

  DeleteBobbyBrTitle();
  DeleteMouseRegionForBigImage();

  giCurrentSubPage = gusCurWeaponIndex;
  guiLastBobbyRayPage = LAPTOP_MODE_BOBBY_R_ARMOR;
}

void RenderBobbyRArmour() {
  WebPageTileBackground(BOBBYR_NUM_HORIZONTAL_TILES, BOBBYR_NUM_VERTICAL_TILES,
                        BOBBYR_BACKGROUND_WIDTH, BOBBYR_BACKGROUND_HEIGHT, guiArmourBackground);

  // Display title at top of page
  DisplayBobbyRBrTitle();

  BltVideoObject(FRAME_BUFFER, guiArmourGrid, 0, BOBBYR_GRIDLOC_X, BOBBYR_GRIDLOC_Y);

  DisplayItemInfo(IC_ARMOUR);

  UpdateButtonText(guiCurrentLaptopMode);
  MarkButtonsDirty();
  RenderWWWProgramTitleBar();
  InvalidateRegion(LAPTOP_SCREEN_UL_X, LAPTOP_SCREEN_WEB_UL_Y, LAPTOP_SCREEN_LR_X,
                   LAPTOP_SCREEN_WEB_LR_Y);
}
