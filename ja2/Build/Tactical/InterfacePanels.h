#ifndef __INTERFACE_PANELS
#define __INTERFACE_PANELS

#include "JA2Types.h"
#include "SGP/ButtonSystem.h"
#include "SGP/MouseSystem.h"
#include "Tactical/Interface.h"

enum {
  STANCEUP_BUTTON = 0,
  UPDOWN_BUTTON,
  CLIMB_BUTTON,
  STANCEDOWN_BUTTON,
  HANDCURSOR_BUTTON,
  PREVMERC_BUTTON,
  NEXTMERC_BUTTON,
  OPTIONS_BUTTON,
  BURSTMODE_BUTTON,
  LOOK_BUTTON,
  TALK_BUTTON,
  MUTE_BUTTON,
  SM_DONE_BUTTON,
  SM_MAP_SCREEN_BUTTON,
  NUM_SM_BUTTONS
};

enum { TEAM_DONE_BUTTON = 0, TEAM_MAP_SCREEN_BUTTON, CHANGE_SQUAD_BUTTON, NUM_TEAM_BUTTONS };

#define NEW_ITEM_CYCLE_COUNT 19
#define NEW_ITEM_CYCLES 4
#define NUM_TEAM_SLOTS 6

#define PASSING_ITEM_DISTANCE_OKLIFE 3
#define PASSING_ITEM_DISTANCE_NOTOKLIFE 2

#define SHOW_LOCATOR_NORMAL 1
#define SHOW_LOCATOR_FAST 2

void CreateSMPanelButtons();
void RemoveSMPanelButtons();
void InitializeSMPanel();
void ShutdownSMPanel();
void RenderSMPanel(DirtyLevel *);
void EnableSMPanelButtons(BOOLEAN fEnable, BOOLEAN fFromItemPickup);

void CreateTEAMPanelButtons();
void RemoveTEAMPanelButtons();
void InitializeTEAMPanel();
void ShutdownTEAMPanel();
void RenderTEAMPanel(DirtyLevel);

void SetSMPanelCurrentMerc(SOLDIERTYPE *s);
void SetTEAMPanelCurrentMerc();

void InitTEAMSlots();
SOLDIERTYPE *GetPlayerFromInterfaceTeamSlot(UINT8 ubPanelSlot);
void RemoveAllPlayersFromSlot();
BOOLEAN RemovePlayerFromTeamSlot(const SOLDIERTYPE *s);
void CheckForAndAddMercToTeamPanel(SOLDIERTYPE *s);

void DisableTacticalTeamPanelButtons(BOOLEAN fDisable);
void RenderTownIDString();

void KeyRingItemPanelButtonCallback(MOUSE_REGION *pRegion, INT32 iReason);
void KeyRingSlotInvClickCallback(MOUSE_REGION *pRegion, INT32 iReason);

void ShowRadioLocator(SOLDIERTYPE *s, UINT8 ubLocatorSpeed);
void EndRadioLocator(SOLDIERTYPE *s);

extern MOUSE_REGION gSMPanelRegion;

extern BOOLEAN gfDisableTacticalPanelButtons;

// Used when the shop keeper interface is active
void DisableSMPpanelButtonsWhenInShopKeeperInterface();

void ReEvaluateDisabledINVPanelButtons();

void CheckForDisabledForGiveItem();
void ReevaluateItemHatches(SOLDIERTYPE *s, BOOLEAN fEnable);

void HandlePanelFaceAnimations(SOLDIERTYPE *s);

void GoToMapScreenFromTactical();

void HandleTacticalEffectsOfEquipmentChange(SOLDIERTYPE *s, UINT32 uiInvPos, UINT16 usOldItem,
                                            UINT16 usNewItem);

void FinishAnySkullPanelAnimations();

SOLDIERTYPE *FindNextMercInTeamPanel(SOLDIERTYPE *prev);

void BeginKeyPanelFromKeyShortcut();

void UpdateForContOverPortrait(SOLDIERTYPE *s, BOOLEAN fOn);

void HandleLocateSelectMerc(SOLDIERTYPE *, bool force_select);

BOOLEAN HandleNailsVestFetish(const SOLDIERTYPE *pSoldier, UINT32 uiHandPos, UINT16 usReplaceItem);

extern SOLDIERTYPE *gpSMCurrentMerc;
extern GUIButtonRef iSMPanelButtons[NUM_SM_BUTTONS];
extern GUIButtonRef iTEAMPanelButtons[NUM_TEAM_BUTTONS];
extern GUIButtonRef giSMStealthButton;
extern SOLDIERTYPE *gSelectSMPanelToMerc;
extern MOUSE_REGION gSM_SELMERCMoneyRegion;
extern UINT8 gubHandPos;
extern UINT16 gusOldItemIndex;
extern UINT16 gusNewItemIndex;
extern BOOLEAN gfDeductPoints;
extern BOOLEAN gfSMDisableForItems;

void LoadInterfacePanelGraphics();
void DeleteInterfacePanelGraphics();

#endif
