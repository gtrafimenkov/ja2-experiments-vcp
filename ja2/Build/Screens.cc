#include "Screens.h"

#include "AniViewScreen.h"
#include "Credits.h"
#include "Editor/EditScreen.h"
#include "Editor/LoadScreen.h"
#include "FadeScreen.h"
#include "GameInitOptionsScreen.h"
#include "GameScreen.h"
#include "Intro.h"
#include "JAScreens.h"
#include "Laptop/Laptop.h"
#include "MainMenuScreen.h"
#include "MessageBoxScreen.h"
#include "OptionsScreen.h"
#include "SaveLoadScreen.h"
#include "Strategic/AIViewer.h"
#include "Strategic/AutoResolve.h"
#include "Strategic/MapScreen.h"
#include "Strategic/QuestDebugSystem.h"
#include "Tactical/ShopKeeperInterface.h"
#include "Utils/MapUtility.h"

Screens const GameScreens[MAX_SCREENS] = {
    {NULL, NULL, NULL},//     {EditScreenInit, EditScreenHandle, EditScreenShutdown},
    {NULL, NULL, NULL},
    {NULL, NULL, NULL},
    {NULL, ErrorScreenHandle, NULL},  // Title Screen
    {NULL, InitScreenHandle, NULL},   // Title Screen
    {MainGameScreenInit, MainGameScreenHandle, MainGameScreenShutdown},
    {NULL, NULL, NULL},//     {NULL, AniEditScreenHandle, NULL},
    {NULL, NULL, NULL},//     {NULL, PalEditScreenHandle, NULL},
    {NULL, NULL, NULL},//     {NULL, DebugScreenHandle, NULL},
    {MapScreenInit, MapScreenHandle, MapScreenShutdown},
    {LaptopScreenInit, LaptopScreenHandle, LaptopScreenShutdown},
    {NULL, NULL, NULL},//     {NULL, LoadSaveScreenHandle, NULL},
    {NULL, NULL, NULL},//     {NULL, MapUtilScreenHandle, NULL},
    {NULL, FadeScreenHandle, NULL},
    {NULL, MessageBoxScreenHandle, MessageBoxScreenShutdown},
    {NULL, MainMenuScreenHandle, NULL},
    {NULL, NULL, NULL},//     {NULL, AutoResolveScreenHandle, NULL},
    {NULL, NULL, NULL},//     {NULL, SaveLoadScreenHandle, NULL},
    {NULL, OptionsScreenHandle, NULL},
    {NULL, NULL, NULL},//     {ShopKeeperScreenInit, ShopKeeperScreenHandle, ShopKeeperScreenShutdown},
    {NULL, NULL, NULL},//     {NULL, SexScreenHandle, NULL},
    {NULL, NULL, NULL},//     {NULL, GameInitOptionsScreenHandle, NULL},
    {NULL, NULL, NULL},//     {NULL, NULL, NULL},
    {NULL, NULL, NULL},//     {NULL, IntroScreenHandle, NULL},
    {NULL, NULL, NULL},//     {NULL, CreditScreenHandle, NULL},
    {NULL, NULL, NULL},//     {QuestDebugScreenInit, QuestDebugScreenHandle, NULL}
};

#include "gtest/gtest.h"

TEST(Screens, asserts) { EXPECT_EQ(lengthof(GameScreens), MAX_SCREENS); }

