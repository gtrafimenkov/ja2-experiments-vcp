// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#include "TacticalAI/FindLocations.h"

#include <algorithm>
#include <string.h>

#include "Macro.h"
#include "SGP/Debug.h"
#include "SGP/Video.h"
#include "Strategic/StrategicMap.h"
#include "Tactical/AnimationControl.h"
#include "Tactical/AnimationData.h"
#include "Tactical/Boxing.h"
#include "Tactical/HandleItems.h"
#include "Tactical/Items.h"
#include "Tactical/LOS.h"
#include "Tactical/OppList.h"
#include "Tactical/PathAI.h"
#include "Tactical/PathAIDebug.h"
#include "Tactical/Points.h"
#include "Tactical/SoldierProfile.h"
#include "Tactical/StructureWrap.h"
#include "Tactical/Weapons.h"
#include "Tactical/WorldItems.h"
#include "TacticalAI/AI.h"
#include "TacticalAI/AIInternals.h"
#include "TileEngine/Environment.h"
#include "TileEngine/IsometricUtils.h"
#include "TileEngine/Lighting.h"
#include "TileEngine/MapEdgepoints.h"
#include "TileEngine/RenderFun.h"
#include "TileEngine/RenderWorld.h"
#include "TileEngine/Structure.h"
#include "TileEngine/WorldMan.h"
#include "Utils/Text.h"

#ifdef _DEBUG
int16_t gsCoverValue[WORLD_MAX];
int16_t gsBestCover;
#ifndef PATHAI_VISIBLE_DEBUG
// NB Change this to true to get visible cover debug -- CJC
BOOLEAN gfDisplayCoverValues = FALSE;
#endif
#endif

int8_t gubAIPathCosts[19][19];

static int32_t CalcPercentBetter(int32_t iOldValue, int32_t iNewValue, int32_t iOldScale,
                                 int32_t iNewScale) {
  int32_t iValueChange, iScaleSum, iPercentBetter;  //,loopCnt,tempInt;

  // calcalate how much better the new cover would be than the current cover
  iValueChange = iNewValue - iOldValue;

  // here, the change in cover HAS to be an improvement over current cover
  if (iValueChange <= 0) {
#ifdef BETAVERSION
    sprintf(tempstr, "CalcPercentBetter: ERROR - invalid valueChange = %d", valueChange);
    PopMessage(tempstr);
#endif

    return (NOWHERE);
  }

  iScaleSum = iOldScale + iNewScale;

  // here, the change in cover HAS to be an improvement over current cover
  if (iScaleSum <= 0) {
#ifdef BETAVERSION
    sprintf(tempstr, "CalcPercentBetter: ERROR - invalid scaleSum = %d", iScaleSum);
    PopMessage(tempstr);
#endif

    return (NOWHERE);
  }

  iPercentBetter = (iValueChange * 100) / iScaleSum;

#ifdef DEBUGCOVER
  DebugAI(
      String("CalcPercentBetter: %%Better %ld, old %ld, new %ld, change "
             "%ld\n\t\toldScale %ld, newScale %ld, scaleSum %ld\n",
             iPercentBetter, iOldValue, iNewValue, iValueChange, iOldScale, iNewScale, iScaleSum));
#endif

  return (iPercentBetter);
}

static void AICenterXY(int16_t sGridNo, float *pdX, float *pdY) {
  int16_t sXPos, sYPos;

  sXPos = sGridNo % WORLD_COLS;
  sYPos = sGridNo / WORLD_COLS;

  *pdX = (float)(sXPos * CELL_X_SIZE + CELL_X_SIZE / 2);
  *pdY = (float)(sYPos * CELL_Y_SIZE + CELL_Y_SIZE / 2);
}

static int8_t CalcWorstCTGTForPosition(SOLDIERTYPE *const pSoldier,
                                       const SOLDIERTYPE *const opponent, const int16_t sOppGridNo,
                                       const int8_t bLevel, const int32_t iMyAPsLeft) {
  // When considering a gridno for cover, we want to take into account cover if
  // we lie down, so we return the LOWEST chance to get through for that
  // location.
  int8_t bCubeLevel, bThisCTGT, bWorstCTGT = 100;

  for (bCubeLevel = 1; bCubeLevel <= 3; bCubeLevel++) {
    switch (bCubeLevel) {
      case 1:
        if (iMyAPsLeft < AP_CROUCH + AP_PRONE) {
          continue;
        }
        break;
      case 2:
        if (iMyAPsLeft < AP_CROUCH) {
          continue;
        }
        break;
      default:
        break;
    }

    bThisCTGT =
        SoldierToLocationChanceToGetThrough(pSoldier, sOppGridNo, bLevel, bCubeLevel, opponent);
    if (bThisCTGT < bWorstCTGT) {
      bWorstCTGT = bThisCTGT;
      // if there is perfect cover
      if (bWorstCTGT == 0)
        // then bail from the for loop, it can't possible get any better
        break;
    }
  }
  return (bWorstCTGT);
}

static int8_t CalcAverageCTGTForPosition(SOLDIERTYPE *const pSoldier,
                                         const SOLDIERTYPE *const opponent,
                                         const int16_t sOppGridNo, const int8_t bLevel,
                                         const int32_t iMyAPsLeft) {
  // When considering a gridno for cover, we want to take into account cover if
  // we lie down, so we return the LOWEST chance to get through for that
  // location.
  int8_t bCubeLevel;
  int32_t iTotalCTGT = 0;
  int32_t bValidCubeLevels = 0;

  for (bCubeLevel = 1; bCubeLevel <= 3; bCubeLevel++) {
    switch (bCubeLevel) {
      case 1:
        if (iMyAPsLeft < AP_CROUCH + AP_PRONE) {
          continue;
        }
        break;
      case 2:
        if (iMyAPsLeft < AP_CROUCH) {
          continue;
        }
        break;
      default:
        break;
    }
    iTotalCTGT +=
        SoldierToLocationChanceToGetThrough(pSoldier, sOppGridNo, bLevel, bCubeLevel, opponent);
    bValidCubeLevels++;
  }
  iTotalCTGT /= bValidCubeLevels;
  return ((int8_t)iTotalCTGT);
}

static int8_t CalcBestCTGT(SOLDIERTYPE *const pSoldier, const SOLDIERTYPE *const opponent,
                           const int16_t sOppGridNo, const int8_t bLevel,
                           const int32_t iMyAPsLeft) {
  // NOTE: CTGT stands for "ChanceToGetThrough..."

  // using only ints for maximum execution speed here
  // CJC: Well, so much for THAT idea!
  int16_t sCentralGridNo, sAdjSpot, sNorthGridNo, sSouthGridNo, sDir, sCheckSpot;

  int8_t bThisCTGT, bBestCTGT = 0;

  sCheckSpot = -1;

  sCentralGridNo = pSoldier->sGridNo;

  // precalculate these for speed
  // what was struct for?
  sNorthGridNo = NewGridNo(sCentralGridNo, DirectionInc(NORTH));
  sSouthGridNo = NewGridNo(sCentralGridNo, DirectionInc(SOUTH));

  // look into all 8 adjacent tiles & determine where the cover is the worst
  for (sDir = 1; sDir <= 8; sDir++) {
    // get the gridno of the adjacent spot lying in that direction
    sAdjSpot = NewGridNo(sCentralGridNo, DirectionInc(sDir));

    // if it wasn't out of bounds
    if (sAdjSpot != sCentralGridNo) {
      // if the adjacent spot can we walked on and isn't in water or gas
      if ((NewOKDestination(pSoldier, sAdjSpot, IGNOREPEOPLE, bLevel) > 0) &&
          !InWaterOrGas(pSoldier, sAdjSpot)) {
        switch (sDir) {
          case NORTH:
          case EAST:
          case SOUTH:
          case WEST:
            sCheckSpot = sAdjSpot;
            break;
          case NORTHEAST:
          case NORTHWEST:
            // spot to the NORTH is guaranteed to be in bounds since NE/NW was
            sCheckSpot = sNorthGridNo;
            break;
          case SOUTHEAST:
          case SOUTHWEST:
            // spot to the SOUTH is guaranteed to be in bounds since SE/SW was
            sCheckSpot = sSouthGridNo;
            break;
        }

        // ATE: OLD STUFF
        // if the adjacent gridno is reachable from the starting spot
        if (NewOKDestination(pSoldier, sCheckSpot, FALSE, bLevel)) {
          // the dude could move to this adjacent gridno, so put him there
          // "virtually" so we can calculate what our cover is from there

          // NOTE: GOTTA SET THESE 3 FIELDS *BACK* AFTER USING THIS FUNCTION!!!
          pSoldier->sGridNo = sAdjSpot;  // pretend he's standing at 'sAdjSpot'
          AICenterXY(sAdjSpot, &(pSoldier->dXPos), &(pSoldier->dYPos));
          bThisCTGT = CalcWorstCTGTForPosition(pSoldier, opponent, sOppGridNo, bLevel, iMyAPsLeft);
          if (bThisCTGT > bBestCTGT) {
            bBestCTGT = bThisCTGT;
            // if there is no cover
            if (bBestCTGT == 100)
              // then bail from the for loop, it can't possible get any better
              break;
          }
        }
      }
    }
  }

  return (bBestCTGT);
}

static int32_t CalcCoverValue(SOLDIERTYPE *pMe, int16_t sMyGridNo, int32_t iMyThreat,
                              int32_t iMyAPsLeft, uint32_t uiThreatIndex, int32_t iRange,
                              int32_t morale, int32_t *iTotalScale) {
  // all 32-bit integers for max. speed
  int32_t iMyPosValue, iHisPosValue, iCoverValue;
  int32_t iReductionFactor, iThisScale;
  int16_t sHisGridNo, sMyRealGridNo = NOWHERE, sHisRealGridNo = NOWHERE;
  int16_t sTempX, sTempY;
  float dMyX, dMyY, dHisX, dHisY;
  int8_t bHisBestCTGT, bHisActualCTGT, bHisCTGT, bMyCTGT;
  int32_t iRangeChange, iRangeFactor, iRangeFactorMultiplier;
  SOLDIERTYPE *pHim;

  dMyX = dMyY = dHisX = dHisY = -1.0;

  pHim = Threat[uiThreatIndex].pOpponent;
  sHisGridNo = Threat[uiThreatIndex].sGridNo;

  // THE FOLLOWING STUFF IS *VEERRRY SCAARRRY*, BUT SHOULD WORK.  IF YOU REALLY
  // HATE IT, THEN CHANGE ChanceToGetThrough() TO WORK FROM A GRIDNO TO GRIDNO

  // if this is theoretical, and I'm not actually at sMyGridNo right now
  if (pMe->sGridNo != sMyGridNo) {
    sMyRealGridNo = pMe->sGridNo;  // remember where I REALLY am
    dMyX = pMe->dXPos;
    dMyY = pMe->dYPos;

    pMe->sGridNo = sMyGridNo;  // but pretend I'm standing at sMyGridNo
    ConvertGridNoToCenterCellXY(sMyGridNo, &sTempX, &sTempY);
    pMe->dXPos = (float)sTempX;
    pMe->dYPos = (float)sTempY;
  }

  // if this is theoretical, and he's not actually at hisGrid right now
  if (pHim->sGridNo != sHisGridNo) {
    sHisRealGridNo = pHim->sGridNo;  // remember where he REALLY is
    dHisX = pHim->dXPos;
    dHisY = pHim->dYPos;

    pHim->sGridNo = sHisGridNo;  // but pretend he's standing at sHisGridNo
    ConvertGridNoToCenterCellXY(sHisGridNo, &sTempX, &sTempY);
    pHim->dXPos = (float)sTempX;
    pHim->dYPos = (float)sTempY;
  }

  if (InWaterOrGas(pHim, sHisGridNo)) {
    bHisActualCTGT = 0;
  } else {
    // optimistically assume we'll be behind the best cover available at this
    // spot

    // bHisActualCTGT =
    // ChanceToGetThrough(pHim,sMyGridNo,FAKE,ACTUAL,TESTWALLS,9999,M9PISTOL,NOT_FOR_LOS);
    // // assume a gunshot
    bHisActualCTGT = CalcWorstCTGTForPosition(pHim, pMe, sMyGridNo, pMe->bLevel, iMyAPsLeft);
  }

  // normally, that will be the cover I'll use, unless worst case over-rides it
  bHisCTGT = bHisActualCTGT;

  // only calculate his best case CTGT if there is room for improvement!
  if (bHisActualCTGT < 100) {
    // if we didn't remember his real gridno earlier up above, we got to now,
    // because calculating worst case is about to play with it in a big way!
    if (sHisRealGridNo == NOWHERE) {
      sHisRealGridNo = pHim->sGridNo;  // remember where he REALLY is
      dHisX = pHim->dXPos;
      dHisY = pHim->dYPos;
    }

    // calculate where my cover is worst if opponent moves just 1 tile over
    bHisBestCTGT = CalcBestCTGT(pHim, pMe, sMyGridNo, pMe->bLevel, iMyAPsLeft);

    // if he can actually improve his CTGT by moving to a nearby gridno
    if (bHisBestCTGT > bHisActualCTGT) {
      // he may not take advantage of his best case, so take only 2/3 of best
      bHisCTGT = ((2 * bHisBestCTGT) + bHisActualCTGT) / 3;
    }
  }

  // if my intended gridno is in water or gas, I can't attack at all from there
  // here, for smoke, consider bad
  if (InWaterGasOrSmoke(pMe, sMyGridNo)) {
    bMyCTGT = 0;
  } else {
    // put him at sHisGridNo if necessary!
    if (pHim->sGridNo != sHisGridNo) {
      pHim->sGridNo = sHisGridNo;
      ConvertGridNoToCenterCellXY(sHisGridNo, &sTempX, &sTempY);
      pHim->dXPos = (float)sTempX;
      pHim->dYPos = (float)sTempY;
    }
    // bMyCTGT =
    // ChanceToGetThrough(pMe,sHisGridNo,FAKE,ACTUAL,TESTWALLS,9999,M9PISTOL,NOT_FOR_LOS);
    // // assume a gunshot bMyCTGT = SoldierToLocationChanceToGetThrough( pMe,
    // sHisGridNo, pMe->bTargetLevel, pMe->bTargetCubeLevel );

    // let's not assume anything about the stance the enemy might take, so take
    // an average value... no cover give a higher value than partial cover
    bMyCTGT = CalcAverageCTGTForPosition(pMe, pHim, sHisGridNo, pHim->bLevel, iMyAPsLeft);

    // since NPCs are too dumb to shoot "blind", ie. at opponents that they
    // themselves can't see (mercs can, using another as a spotter!), if the
    // cover is below the "see_thru" threshold, it's equivalent to perfect
    // cover!
    if (bMyCTGT < SEE_THRU_COVER_THRESHOLD) {
      bMyCTGT = 0;
    }
  }

  // UNDO ANY TEMPORARY "DAMAGE" DONE ABOVE
  if (sMyRealGridNo != NOWHERE) {
    pMe->sGridNo = sMyRealGridNo;  // put me back where I belong!
    pMe->dXPos = dMyX;             // also change the 'x'
    pMe->dYPos = dMyY;             // and the 'y'
  }

  if (sHisRealGridNo != NOWHERE) {
    pHim->sGridNo = sHisRealGridNo;  // put HIM back where HE belongs!
    pHim->dXPos = dHisX;             // also change the 'x'
    pHim->dYPos = dHisY;             // and the 'y'
  }

  // these value should be < 1 million each
  iHisPosValue = bHisCTGT * Threat[uiThreatIndex].iValue * Threat[uiThreatIndex].iAPs;
  iMyPosValue = bMyCTGT * iMyThreat * iMyAPsLeft;

  // try to account for who outnumbers who: the side with the advantage thus
  // (hopefully) values offense more, while those in trouble will play defense
  if (pHim->bOppCnt > 1) {
    iHisPosValue /= pHim->bOppCnt;
  }

  if (pMe->bOppCnt > 1) {
    iMyPosValue /= pMe->bOppCnt;
  }

  // if my positional value is worth something at all here
  if (iMyPosValue > 0) {
    // if I CAN'T crouch when I get there, that makes it significantly less
    // appealing a spot (how much depends on range), so that's a penalty to me
    if (iMyAPsLeft < AP_CROUCH)
      // subtract another 1 % penalty for NOT being able to crouch per tile
      // the farther away we are, the bigger a difference crouching will make!
      iMyPosValue -= ((iMyPosValue * (AIM_PENALTY_TARGET_CROUCHED + (iRange / CELL_X_SIZE))) / 100);
  }

  // high morale prefers decreasing the range (positive factor), while very
  // low morale (HOPELESS) prefers increasing it

  //	if (bHisCTGT < 100 || (morale - 1 < 0))
  {
    iRangeFactorMultiplier = RangeChangeDesire(pMe);

    if (iRangeFactorMultiplier) {
      iRangeChange = Threat[uiThreatIndex].iOrigRange - iRange;

      if (iRangeChange) {
        // iRangeFactor = (iRangeChange * (morale - 1)) / 4;
        iRangeFactor = (iRangeChange * iRangeFactorMultiplier) / 2;

#ifdef DEBUGCOVER
        DebugAI(String("CalcCoverValue: iRangeChange %d, iRangeFactor %d\n", iRangeChange,
                       iRangeFactor));
#endif

        // aggression booster for stupider enemies
        iMyPosValue += 100 * iRangeFactor * (5 - SoldierDifficultyLevel(pMe)) / 5;

        // if factor is positive increase positional value, else decrease it
        // change both values, since one or the other could be 0
        if (iRangeFactor > 0) {
          iMyPosValue = (iMyPosValue * (100 + iRangeFactor)) / 100;
          iHisPosValue = (100 * iHisPosValue) / (100 + iRangeFactor);
        } else if (iRangeFactor < 0) {
          iMyPosValue = (100 * iMyPosValue) / (100 - iRangeFactor);
          iHisPosValue = (iHisPosValue * (100 - iRangeFactor)) / 100;
        }
      }
    }
  }

  // the farther apart we are, the less important the cover differences are
  // the less certain his position, the less important cover differences are
  iReductionFactor =
      ((MAX_THREAT_RANGE - iRange) * Threat[uiThreatIndex].iCertainty) / MAX_THREAT_RANGE;

  // divide by a 100 to make the numbers more managable and avoid 32-bit limit
  iThisScale = std::max(iMyPosValue, iHisPosValue) / 100;
  iThisScale = (iThisScale * iReductionFactor) / 100;
  *iTotalScale += iThisScale;
  // this helps to decide the percent improvement later

  // POSITIVE COVER VALUE INDICATES THE COVER BENEFITS ME, NEGATIVE RESULT
  // MEANS IT BENEFITS THE OTHER GUY.
  // divide by a 100 to make the numbers more managable and avoid 32-bit limit
  iCoverValue = (iMyPosValue - iHisPosValue) / 100;
  iCoverValue = (iCoverValue * iReductionFactor) / 100;

#ifdef DEBUGCOVER
  DebugAI(
      String("CalcCoverValue: iCoverValue %d, sMyGridNo %d, sHisGrid %d, "
             "iRange %d, morale %d\n",
             iCoverValue, sMyGridNo, sHisGridNo, iRange, morale));
  DebugAI(String("CalcCoverValue: iCertainty %d, his bOppCnt %d, my bOppCnt %d\n",
                 Threat[uiThreatIndex].iCertainty, pHim->bOppCnt, pMe->bOppCnt));
  DebugAI(String("CalcCoverValue: bHisCTGT = %d, hisThreat = %d, hisFullAPs = %d\n", bHisCTGT,
                 Threat[uiThreatIndex].iValue, Threat[uiThreatIndex].iAPs));
  DebugAI(String("CalcCoverValue: bMyCTGT = %d,  iMyThreat = %d,  iMyAPsLeft = %d\n", bMyCTGT,
                 iMyThreat, iMyAPsLeft));
  DebugAI(String("CalcCoverValue: hisPosValue = %d, myPosValue = %d\n", iHisPosValue, iMyPosValue));
  DebugAI(
      String("CalcCoverValue: iThisScale = %d, iTotalScale = %d, "
             "iReductionFactor %d\n\n",
             iThisScale, *iTotalScale, iReductionFactor));
#endif

  return (iCoverValue);
}

static uint8_t NumberOfTeamMatesAdjacent(SOLDIERTYPE *pSoldier, int16_t sGridNo) {
  int16_t sTempGridNo;

  uint8_t ubCount = 0;

  for (uint8_t ubLoop = 0; ubLoop < NUM_WORLD_DIRECTIONS; ++ubLoop) {
    sTempGridNo = NewGridNo(sGridNo, DirectionInc(ubLoop));
    if (sTempGridNo != sGridNo) {
      const SOLDIERTYPE *const tgt = WhoIsThere2(sGridNo, pSoldier->bLevel);
      if (tgt != NULL && tgt != pSoldier && tgt->bTeam == pSoldier->bTeam) {
        ubCount++;
      }
    }
  }

  return (ubCount);
}

int16_t FindBestNearbyCover(SOLDIERTYPE *pSoldier, int32_t morale, int32_t *piPercentBetter) {
  // all 32-bit integers for max. speed
  int32_t iCurrentCoverValue, iCoverValue, iBestCoverValue;
  int32_t iCurrentScale, iCoverScale;
  int32_t iDistFromOrigin, iDistCoverFromOrigin, iThreatCertainty;
  int16_t sGridNo, sBestCover = NOWHERE;
  int32_t iPathCost;
  int32_t iThreatRange, iClosestThreatRange = 1500;
  //	int16_t sClosestThreatGridno = NOWHERE;
  int32_t iMyThreatValue;
  int16_t sThreatLoc;
  int32_t iMaxThreatRange;
  uint32_t uiThreatCnt = 0;
  int32_t iMaxMoveTilesLeft, iSearchRange, iRoamRange;
  int16_t sMaxLeft, sMaxRight, sMaxUp, sMaxDown, sXOffset, sYOffset;
  int16_t sOrigin;  // has to be a short, need a pointer
  int16_t *pusLastLoc;
  int8_t *pbPersOL;
  int8_t *pbPublOL;

  uint8_t ubBackgroundLightLevel;
  uint8_t ubBackgroundLightPercent = 0;
  uint8_t ubLightPercentDifference;
  BOOLEAN fNight;

  int32_t iBestCoverScale = 0;  // XXX HACK000E

  bool const fHasGasMask = IsWearingHeadGear(*pSoldier, GASMASK);

  if (gbWorldSectorZ > 0) {
    fNight = FALSE;
  } else {
    ubBackgroundLightLevel = GetTimeOfDayAmbientLightLevel();

    if (ubBackgroundLightLevel < NORMAL_LIGHTLEVEL_DAY + 2) {
      fNight = FALSE;
    } else {
      fNight = TRUE;
      ubBackgroundLightPercent = gbLightSighting[0][ubBackgroundLightLevel];
    }
  }

  iBestCoverValue = -1;

#if defined(_DEBUG) && !defined(PATHAI_VISIBLE_DEBUG)
  if (gfDisplayCoverValues) {
    memset(gsCoverValue, 0x7F, sizeof(int16_t) * WORLD_MAX);
  }
#endif

  // NameMessage(pSoldier,"looking for some cover...");

  // BUILD A LIST OF THREATENING GRID #s FROM PERSONAL & PUBLIC opplists

  pusLastLoc = &(gsLastKnownOppLoc[pSoldier->ubID][0]);

  // hang a pointer into personal opplist
  pbPersOL = &(pSoldier->bOppList[0]);
  // hang a pointer into public opplist
  pbPublOL = &(gbPublicOpplist[pSoldier->bTeam][0]);

  // decide how far we're gonna be looking
  iSearchRange = gbDiff[DIFF_MAX_COVER_RANGE][SoldierDifficultyLevel(pSoldier)];

  /*
          switch (pSoldier->bAttitude)
          {
                  case DEFENSIVE:		iSearchRange += 2; break;
                  case BRAVESOLO:		iSearchRange -= 4; break;
                  case BRAVEAID:		iSearchRange -= 4; break;
                  case CUNNINGSOLO:	iSearchRange += 4; break;
                  case CUNNINGAID:	iSearchRange += 4; break;
                  case AGGRESSIVE:	iSearchRange -= 2; break;
          }*/

  // maximum search range is 1 tile / 8 pts of wisdom
  if (iSearchRange > (pSoldier->bWisdom / 8)) {
    iSearchRange = (pSoldier->bWisdom / 8);
  }

  if (!gfTurnBasedAI) {
    // don't search so far in realtime
    iSearchRange /= 2;
  }

  if (pSoldier->bAlertStatus >= STATUS_RED)  // if already in battle
  {
    uint16_t const usMovementMode = DetermineMovementMode(pSoldier, AI_ACTION_TAKE_COVER);

    // must be able to reach the cover, so it can't possibly be more than
    // action points left (rounded down) tiles away, since minimum
    // cost to move per tile is 1 points.
    iMaxMoveTilesLeft =
        std::max(0, pSoldier->bActionPoints - MinAPsToStartMovement(pSoldier, usMovementMode));
    // NumMessage("In BLACK, maximum tiles to move left = ",maxMoveTilesLeft);

    // if we can't go as far as the usual full search range
    if (iMaxMoveTilesLeft < iSearchRange) {
      // then limit the search range to only as far as we CAN go
      iSearchRange = iMaxMoveTilesLeft;
    }
  }

  if (iSearchRange <= 0) {
    return (NOWHERE);
  }

  // those within 20 tiles of any tile we'll CONSIDER as cover are important
  iMaxThreatRange = MAX_THREAT_RANGE + (CELL_X_SIZE * iSearchRange);

  // calculate OUR OWN general threat value (not from any specific location)
  iMyThreatValue = CalcManThreatValue(pSoldier, NOWHERE, FALSE, pSoldier);

  // look through all opponents for those we know of
  FOR_EACH_MERC(i) {
    SOLDIERTYPE *const pOpponent = *i;

    // if this merc is inactive, at base, on assignment, dead, unconscious
    if (pOpponent->bLife < OKLIFE) continue;  // next merc

    // if this man is neutral / on the same side, he's not an opponent
    if (CONSIDERED_NEUTRAL(pSoldier, pOpponent) || (pSoldier->bSide == pOpponent->bSide)) {
      continue;  // next merc
    }

    pbPersOL = pSoldier->bOppList + pOpponent->ubID;
    pbPublOL = gbPublicOpplist[pSoldier->bTeam] + pOpponent->ubID;
    pusLastLoc = gsLastKnownOppLoc[pSoldier->ubID] + pOpponent->ubID;

    // if this opponent is unknown personally and publicly
    if ((*pbPersOL == NOT_HEARD_OR_SEEN) && (*pbPublOL == NOT_HEARD_OR_SEEN)) {
      continue;  // next merc
    }

    // Special stuff for Carmen the bounty hunter
    if (pSoldier->bAttitude == ATTACKSLAYONLY && pOpponent->ubProfile != SLAY) {
      continue;  // next opponent
    }

    // if personal knowledge is more up to date or at least equal
    if ((gubKnowledgeValue[*pbPublOL - OLDEST_HEARD_VALUE][*pbPersOL - OLDEST_HEARD_VALUE] > 0) ||
        (*pbPersOL == *pbPublOL)) {
      // using personal knowledge, obtain opponent's "best guess" gridno
      sThreatLoc = *pusLastLoc;
      iThreatCertainty = ThreatPercent[*pbPersOL - OLDEST_HEARD_VALUE];
    } else {
      // using public knowledge, obtain opponent's "best guess" gridno
      sThreatLoc = gsPublicLastKnownOppLoc[pSoldier->bTeam][pOpponent->ubID];
      iThreatCertainty = ThreatPercent[*pbPublOL - OLDEST_HEARD_VALUE];
    }

    // calculate how far away this threat is (in adjusted pixels)
    // iThreatRange =
    // AdjPixelsAway(CenterX(pSoldier->sGridNo),CenterY(pSoldier->sGridNo),CenterX(sThreatLoc),CenterY(sThreatLoc));
    iThreatRange = GetRangeInCellCoordsFromGridNoDiff(pSoldier->sGridNo, sThreatLoc);

    // NumMessage("Threat Range = ",iThreatRange);

#ifdef DEBUGCOVER
//		DebugAI( String( "FBNC: Opponent %d believed to be at gridno %d
//(mine %d, public
//%d)\n",iLoop,sThreatLoc,*pusLastLoc,PublicLastKnownOppLoc[pSoldier->bTeam][iLoop]
//) );
#endif

    // if this opponent is believed to be too far away to really be a threat
    if (iThreatRange > iMaxThreatRange) {
#ifdef DEBUGCOVER
//			AINameMessage(pOpponent,"is too far away to be a
// threat",1000);
#endif

      continue;  // check next opponent
    }

    // remember this opponent as a current threat, but DON'T REDUCE FOR COVER!
    Threat[uiThreatCnt].iValue = CalcManThreatValue(pOpponent, pSoldier->sGridNo, FALSE, pSoldier);

    // if the opponent is no threat at all for some reason
    if (Threat[uiThreatCnt].iValue == -999) {
      // NameMessage(pOpponent,"is thought to be no threat");
      continue;  // check next opponent
    }

    // NameMessage(pOpponent,"added to the list of threats");
    // NumMessage("His/Her threat value = ",threatValue[uiThreatCnt]);

    Threat[uiThreatCnt].pOpponent = pOpponent;
    Threat[uiThreatCnt].sGridNo = sThreatLoc;
    Threat[uiThreatCnt].iCertainty = iThreatCertainty;
    Threat[uiThreatCnt].iOrigRange = iThreatRange;

    // calculate how many APs he will have at the start of the next turn
    Threat[uiThreatCnt].iAPs = CalcActionPoints(pOpponent);

    if (iThreatRange < iClosestThreatRange) {
      iClosestThreatRange = iThreatRange;
      // NumMessage("New Closest Threat Range = ",iClosestThreatRange);
      //			sClosestThreatGridNo = sThreatLoc;
      // NumMessage("Closest Threat Gridno = ",sClosestThreatGridNo);
    }

    uiThreatCnt++;
  }

  // if no known opponents were found to threaten us, can't worry about cover
  if (!uiThreatCnt) {
    // NameMessage(pSoldier,"has no threats - WON'T take cover");
    return (sBestCover);
  }

  // calculate our current cover value in the place we are now, since the
  // cover we are searching for must be better than what we have now!
  iCurrentCoverValue = 0;
  iCurrentScale = 0;

  // for every opponent that threatens, consider this spot's cover vs. him
  for (uint32_t uiLoop = 0; uiLoop < uiThreatCnt; ++uiLoop) {
    // if this threat is CURRENTLY within 20 tiles
    if (Threat[uiLoop].iOrigRange <= MAX_THREAT_RANGE) {
      // add this opponent's cover value to our current total cover value
      iCurrentCoverValue +=
          CalcCoverValue(pSoldier, pSoldier->sGridNo, iMyThreatValue, pSoldier->bActionPoints,
                         uiLoop, Threat[uiLoop].iOrigRange, morale, &iCurrentScale);
    }
    // sprintf(tempstr,"iCurrentCoverValue after opponent %d is now
    // %d",iLoop,iCurrentCoverValue); PopMessage(tempstr);
  }

  iCurrentCoverValue -=
      (iCurrentCoverValue / 10) * NumberOfTeamMatesAdjacent(pSoldier, pSoldier->sGridNo);

#ifdef DEBUGCOVER
//	AINumMessage("Search Range = ",iSearchRange);
#endif

  // determine maximum horizontal limits
  sMaxLeft = std::min(iSearchRange, (pSoldier->sGridNo % MAXCOL));
  // NumMessage("sMaxLeft = ",sMaxLeft);
  sMaxRight = std::min(iSearchRange, MAXCOL - ((pSoldier->sGridNo % MAXCOL) + 1));
  // NumMessage("sMaxRight = ",sMaxRight);

  // determine maximum vertical limits
  sMaxUp = std::min(iSearchRange, (pSoldier->sGridNo / MAXROW));
  // NumMessage("sMaxUp = ",sMaxUp);
  sMaxDown = std::min(iSearchRange, MAXROW - ((pSoldier->sGridNo / MAXROW) + 1));
  // NumMessage("sMaxDown = ",sMaxDown);

  iRoamRange = RoamingRange(pSoldier, &sOrigin);

  // if status isn't black (life & death combat), and roaming range is limited
  if ((pSoldier->bAlertStatus != STATUS_BLACK) && (iRoamRange < MAX_ROAMING_RANGE) &&
      (sOrigin != NOWHERE)) {
    // must try to stay within or return to the point of origin
    iDistFromOrigin = SpacesAway(sOrigin, pSoldier->sGridNo);
  } else {
    // don't care how far from origin we go
    iDistFromOrigin = -1;
  }

#ifdef DEBUGCOVER
  DebugAI(
      String("FBNC: iRoamRange %d, sMaxLeft %d, sMaxRight %d, sMaxUp %d, "
             "sMaxDown %d\n",
             iRoamRange, sMaxLeft, sMaxRight, sMaxUp, sMaxDown));
#endif

  // the initial cover value to beat is our current cover value
  iBestCoverValue = iCurrentCoverValue;

#ifdef DEBUGDECISIONS
  DebugAI(String("FBNC: CURRENT iCoverValue = %d\n", iCurrentCoverValue));
#endif

  if (pSoldier->bAlertStatus >= STATUS_RED)  // if already in battle
  {
    // to speed this up, tell PathAI to cancel any paths beyond our AP reach!
    gubNPCAPBudget = pSoldier->bActionPoints;
  } else {
    // even if not under pressure, limit to 1 turn's travelling distance
    // hope this isn't too expensive...
    gubNPCAPBudget = CalcActionPoints(pSoldier);
    // gubNPCAPBudget = pSoldier->bInitialAPs;
  }

  // Call FindBestPath to set flags in all locations that we can
  // walk into within range.  We have to set some things up first...

  // set the distance limit of the square region
  gubNPCDistLimit = (uint8_t)iSearchRange;

  // reset the "reachable" flags in the region we're looking at
  for (sYOffset = -sMaxUp; sYOffset <= sMaxDown; sYOffset++) {
    for (sXOffset = -sMaxLeft; sXOffset <= sMaxRight; sXOffset++) {
      sGridNo = pSoldier->sGridNo + sXOffset + (MAXCOL * sYOffset);
      if (!(sGridNo >= 0 && sGridNo < WORLD_MAX)) {
        continue;
      }
      gpWorldLevelData[sGridNo].uiFlags &= ~(MAPELEMENT_REACHABLE);
    }
  }

  FindBestPath(pSoldier, NOWHERE, pSoldier->bLevel,
               DetermineMovementMode(pSoldier, AI_ACTION_TAKE_COVER), COPYREACHABLE_AND_APS, 0);

  // Turn off the "reachable" flag for his current location
  // so we don't consider it
  gpWorldLevelData[pSoldier->sGridNo].uiFlags &= ~(MAPELEMENT_REACHABLE);

  // SET UP double-LOOP TO STEP THROUGH POTENTIAL GRID #s
  for (sYOffset = -sMaxUp; sYOffset <= sMaxDown; sYOffset++) {
    for (sXOffset = -sMaxLeft; sXOffset <= sMaxRight; sXOffset++) {
      // HandleMyMouseCursor(KEYBOARDALSO);

      // calculate the next potential gridno
      sGridNo = pSoldier->sGridNo + sXOffset + (MAXCOL * sYOffset);
      if (!(sGridNo >= 0 && sGridNo < WORLD_MAX)) {
        continue;
      }

      // NumMessage("Testing gridno #",sGridNo);

      // if we are limited to staying/returning near our place of origin
      if (iDistFromOrigin != -1) {
        iDistCoverFromOrigin = SpacesAway(sOrigin, sGridNo);

        // if this is outside roaming range, and doesn't get us closer to it
        if ((iDistCoverFromOrigin > iRoamRange) && (iDistFromOrigin <= iDistCoverFromOrigin)) {
          continue;  // then we can't go there
        }
      }

      if (!(gpWorldLevelData[sGridNo].uiFlags & MAPELEMENT_REACHABLE)) {
        continue;
      }

      if (!fHasGasMask) {
        if (InGas(pSoldier, sGridNo)) {
          continue;
        }
      }

      // ignore blacklisted spot
      if (sGridNo == pSoldier->sBlackList) {
        continue;
      }

      iPathCost = gubAIPathCosts[AI_PATHCOST_RADIUS + sXOffset][AI_PATHCOST_RADIUS + sYOffset];
      /*
      // water is OK, if the only good hiding place requires us to get wet, OK
      iPathCost =
      LegalNPCDestination(pSoldier,sGridNo,ENSURE_PATH_COST,WATEROK);

      if (!iPathCost)
      {
              continue;        // skip on to the next potential grid
      }

      // CJC: This should be a redundent check because the path code is given an
      // AP limit to begin with!
      if (pSoldier->bAlertStatus == STATUS_BLACK)      // in battle
      {
              // must be able to afford the APs to get to this cover this turn
              if (iPathCost > pSoldier->bActionPoints)
              {
                      //NumMessage("In BLACK, and can't afford to get there,
      cost = ",iPathCost); continue;      // skip on to the next potential grid
              }
      }
      */

      // OK, this place shows potential.  How useful is it as cover?
      // EVALUATE EACH GRID #, remembering the BEST PROTECTED ONE
      iCoverValue = 0;
      iCoverScale = 0;

      // for every opponent that threatens, consider this spot's cover vs. him
      for (uint32_t uiLoop = 0; uiLoop < uiThreatCnt; ++uiLoop) {
        // calculate the range we would be at from this opponent
        iThreatRange = GetRangeInCellCoordsFromGridNoDiff(sGridNo, Threat[uiLoop].sGridNo);
        // if this threat would be within 20 tiles, count it
        if (iThreatRange <= MAX_THREAT_RANGE) {
          iCoverValue += CalcCoverValue(pSoldier, sGridNo, iMyThreatValue,
                                        (pSoldier->bActionPoints - iPathCost), uiLoop, iThreatRange,
                                        morale, &iCoverScale);
        }

        // sprintf(tempstr,"iCoverValue after opponent %d is now
        // %d",iLoop,iCoverValue); PopMessage(tempstr);
      }

      // reduce cover for each person adjacent to this gridno who is on our
      // team, by 10% (so locations next to several people will be very much
      // frowned upon
      if (iCoverValue >= 0) {
        iCoverValue -= (iCoverValue / 10) * NumberOfTeamMatesAdjacent(pSoldier, sGridNo);
      } else {
        // when negative, must add a negative to decrease the total
        iCoverValue += (iCoverValue / 10) * NumberOfTeamMatesAdjacent(pSoldier, sGridNo);
      }

      if (fNight && GetRoom(sGridNo) == NO_ROOM)  // ignore in buildings in case placed there
      {
        // reduce cover at nighttime based on how bright the light is at that
        // location using the difference in sighting distance between the
        // background and the light for this tile
        ubLightPercentDifference = (gbLightSighting[0][LightTrueLevel(sGridNo, pSoldier->bLevel)] -
                                    ubBackgroundLightPercent);
        if (iCoverValue >= 0) {
          iCoverValue -= (iCoverValue / 100) * ubLightPercentDifference;
        } else {
          iCoverValue += (iCoverValue / 100) * ubLightPercentDifference;
        }
      }

#ifdef DEBUGCOVER
      // if there ARE multiple opponents
      if (uiThreatCnt > 1) {
        DebugAI(String("FBNC: Total iCoverValue at gridno %d is %d\n\n", sGridNo, iCoverValue));
      }
#endif

#if defined(_DEBUG) && !defined(PATHAI_VISIBLE_DEBUG)
      if (gfDisplayCoverValues) {
        gsCoverValue[sGridNo] = (int16_t)(iCoverValue / 100);
      }
#endif

      // if this is better than the best place found so far

      if (iCoverValue > iBestCoverValue) {
#ifdef DEBUGDECISIONS
        DebugAI(String("FBNC: NEW BEST iCoverValue at gridno %d is %d\n", sGridNo, iCoverValue));
#endif
        // remember it instead
        sBestCover = sGridNo;
        iBestCoverValue = iCoverValue;
        iBestCoverScale = iCoverScale;
      }
    }
  }

  gubNPCAPBudget = 0;
  gubNPCDistLimit = 0;

#if defined(_DEBUG) && !defined(PATHAI_VISIBLE_DEBUG)
  if (gfDisplayCoverValues) {
    // do a locate?
    LocateSoldier(pSoldier, SETLOCATORFAST);
    gsBestCover = sBestCover;
    SetRenderFlags(RENDER_FLAG_FULL);
    RenderWorld();
    // RenderCoverDebug();
    InvalidateScreen();
    RefreshScreen();
    /*
iLoop = GetJA2Clock();
do
{

} while( ( GetJA2Clock( ) - iLoop ) < 2000 );
*/
  }
#endif

  // if a better cover location was found
  if (sBestCover != NOWHERE) {
#if defined(_DEBUG) && !defined(PATHAI_VISIBLE_DEBUG)
    gsBestCover = sBestCover;
#endif
    // cover values already take the AP cost of getting there into account in
    // a BIG way, so no need to worry about that here, even small improvements
    // are actually very significant once we get our APs back (if we live!)
    *piPercentBetter =
        CalcPercentBetter(iCurrentCoverValue, iBestCoverValue, iCurrentScale, iBestCoverScale);

    // if best cover value found was at least 5% better than our current cover
    if (*piPercentBetter >= MIN_PERCENT_BETTER) {
#ifdef DEBUGDECISIONS
      DebugAI(String("Found Cover: current %ld, best %ld, %%%%Better %ld\n", iCurrentCoverValue,
                     iBestCoverValue, *piPercentBetter));
#endif

#ifdef BETAVERSION
      SnuggleDebug(pSoldier, "Found Cover");
#endif

      return ((int16_t)sBestCover);  // return the gridno of that cover
    }
  }
  return (NOWHERE);  // return that no suitable cover was found
}

int16_t FindSpotMaxDistFromOpponents(SOLDIERTYPE *pSoldier) {
  int16_t sGridNo;
  int16_t sBestSpot = NOWHERE;
  int32_t iThreatRange, iClosestThreatRange = 1500, iSpotClosestThreatRange;
  int16_t sThreatLoc, sThreatGridNo[MAXMERCS];
  uint32_t uiThreatCnt = 0;
  int32_t iSearchRange;
  int16_t sMaxLeft, sMaxRight, sMaxUp, sMaxDown, sXOffset, sYOffset;
  int8_t *pbPersOL, *pbPublOL, bEscapeDirection, bBestEscapeDirection = -1;
  int16_t sOrigin;
  int32_t iRoamRange;

  bool const fHasGasMask = IsWearingHeadGear(*pSoldier, GASMASK);

  // BUILD A LIST OF THREATENING GRID #s FROM PERSONAL & PUBLIC opplistS

  // look through all opponents for those we know of
  FOR_EACH_MERC(i) {
    SOLDIERTYPE *const pOpponent = *i;

    // if this merc is inactive, at base, on assignment, dead, unconscious
    if (pOpponent->bLife < OKLIFE) continue;  // next merc

    // if this man is neutral / on the same side, he's not an opponent
    if (CONSIDERED_NEUTRAL(pSoldier, pOpponent) || (pSoldier->bSide == pOpponent->bSide)) {
      continue;  // next merc
    }

    pbPersOL = &(pSoldier->bOppList[pOpponent->ubID]);
    pbPublOL = &(gbPublicOpplist[pSoldier->bTeam][pOpponent->ubID]);

    // if this opponent is unknown personally and publicly
    if ((*pbPersOL == NOT_HEARD_OR_SEEN) && (*pbPublOL == NOT_HEARD_OR_SEEN)) {
      continue;  // check next opponent
    }

    // Special stuff for Carmen the bounty hunter
    if (pSoldier->bAttitude == ATTACKSLAYONLY && pOpponent->ubProfile != SLAY) {
      continue;  // next opponent
    }

    // if the opponent is no threat at all for some reason
    if (CalcManThreatValue(pOpponent, pSoldier->sGridNo, FALSE, pSoldier) == -999) {
      continue;  // check next opponent
    }

    // if personal knowledge is more up to date or at least equal
    if ((gubKnowledgeValue[*pbPublOL - OLDEST_HEARD_VALUE][*pbPersOL - OLDEST_HEARD_VALUE] > 0) ||
        (*pbPersOL == *pbPublOL)) {
      // using personal knowledge, obtain opponent's "best guess" gridno
      sThreatLoc = gsLastKnownOppLoc[pSoldier->ubID][pOpponent->ubID];
    } else {
      // using public knowledge, obtain opponent's "best guess" gridno
      sThreatLoc = gsPublicLastKnownOppLoc[pSoldier->bTeam][pOpponent->ubID];
    }

    // calculate how far away this threat is (in adjusted pixels)
    iThreatRange = GetRangeInCellCoordsFromGridNoDiff(pSoldier->sGridNo, sThreatLoc);

    if (iThreatRange < iClosestThreatRange) {
      iClosestThreatRange = iThreatRange;
      // NumMessage("New Closest Threat Range = ",iClosestThreatRange);
    }

    // remember this threat's gridno
    sThreatGridNo[uiThreatCnt] = sThreatLoc;
    uiThreatCnt++;
  }

  // if no known opponents were found to threaten us, can't worry about them
  if (!uiThreatCnt) {
    // NameMessage(pSoldier,"has no known threats - WON'T run away");
    return (sBestSpot);
  }

  // get roaming range here; for civilians, running away is limited by roam
  // range
  if (pSoldier->bTeam == CIV_TEAM) {
    iRoamRange = RoamingRange(pSoldier, &sOrigin);
    if (iRoamRange == 0) {
      return (sBestSpot);
    }
  } else {
    // dummy values
    iRoamRange = 100;
    sOrigin = pSoldier->sGridNo;
  }

  // DETERMINE CO-ORDINATE LIMITS OF SQUARE AREA TO BE CHECKED
  // THIS IS A LOT QUICKER THAN COVER, SO DO A LARGER AREA, NOT AFFECTED BY
  // DIFFICULTY SETTINGS...

  if (pSoldier->bAlertStatus == STATUS_BLACK)  // if already in battle
  {
    iSearchRange = pSoldier->bActionPoints / 2;

    // to speed this up, tell PathAI to cancel any paths beyond our AP reach!
    gubNPCAPBudget = pSoldier->bActionPoints;
  } else {
    // even if not under pressure, limit to 1 turn's travelling distance
    gubNPCAPBudget = std::min((int8_t)(pSoldier->bActionPoints / 2), CalcActionPoints(pSoldier));

    iSearchRange = gubNPCAPBudget / 2;
  }

  if (!gfTurnBasedAI) {
    // search only half as far in realtime
    // but always allow a certain minimum!

    if (iSearchRange > 4) {
      iSearchRange /= 2;
      gubNPCAPBudget /= 2;
    }
  }

  // assume we have to stand up!
  // use the min macro here to make sure we don't wrap the uint8_t to 255...

#if 0 /* doppelt? */
	gubNPCAPBudget = 	gubNPCAPBudget = std::min( gubNPCAPBudget, gubNPCAPBudget - GetAPsToChangeStance( pSoldier, ANIM_STAND ) );
#else
  gubNPCAPBudget = std::min(gubNPCAPBudget,
                            (uint8_t)(gubNPCAPBudget - GetAPsToChangeStance(pSoldier, ANIM_STAND)));
#endif
  // NumMessage("Search Range = ",iSearchRange);
  // NumMessage("gubNPCAPBudget = ",gubNPCAPBudget);

  // determine maximum horizontal limits
  sMaxLeft = std::min(iSearchRange, (pSoldier->sGridNo % MAXCOL));
  // NumMessage("sMaxLeft = ",sMaxLeft);
  sMaxRight = std::min(iSearchRange, MAXCOL - ((pSoldier->sGridNo % MAXCOL) + 1));
  // NumMessage("sMaxRight = ",sMaxRight);

  // determine maximum vertical limits
  sMaxUp = std::min(iSearchRange, (pSoldier->sGridNo / MAXROW));
  // NumMessage("sMaxUp = ",sMaxUp);
  sMaxDown = std::min(iSearchRange, MAXROW - ((pSoldier->sGridNo / MAXROW) + 1));
  // NumMessage("sMaxDown = ",sMaxDown);

  // Call FindBestPath to set flags in all locations that we can
  // walk into within range.  We have to set some things up first...

  // set the distance limit of the square region
  gubNPCDistLimit = (uint8_t)iSearchRange;

  // reset the "reachable" flags in the region we're looking at
  for (sYOffset = -sMaxUp; sYOffset <= sMaxDown; sYOffset++) {
    for (sXOffset = -sMaxLeft; sXOffset <= sMaxRight; sXOffset++) {
      sGridNo = pSoldier->sGridNo + sXOffset + (MAXCOL * sYOffset);
      if (!(sGridNo >= 0 && sGridNo < WORLD_MAX)) {
        continue;
      }

      gpWorldLevelData[sGridNo].uiFlags &= ~(MAPELEMENT_REACHABLE);
    }
  }

  FindBestPath(pSoldier, NOWHERE, pSoldier->bLevel,
               DetermineMovementMode(pSoldier, AI_ACTION_RUN_AWAY), COPYREACHABLE, 0);

  // Turn off the "reachable" flag for his current location
  // so we don't consider it
  gpWorldLevelData[pSoldier->sGridNo].uiFlags &= ~(MAPELEMENT_REACHABLE);

  for (sYOffset = -sMaxUp; sYOffset <= sMaxDown; sYOffset++) {
    for (sXOffset = -sMaxLeft; sXOffset <= sMaxRight; sXOffset++) {
      // calculate the next potential gridno
      sGridNo = pSoldier->sGridNo + sXOffset + (MAXCOL * sYOffset);
      // NumMessage("Testing gridno #",gridno);
      if (!(sGridNo >= 0 && sGridNo < WORLD_MAX)) {
        continue;
      }

      if (!(gpWorldLevelData[sGridNo].uiFlags & MAPELEMENT_REACHABLE)) {
        continue;
      }

      if (sGridNo == pSoldier->sBlackList) {
        continue;
      }

      if (!fHasGasMask) {
        if (InGas(pSoldier, sGridNo)) {
          continue;
        }
      }

      if (pSoldier->bTeam == CIV_TEAM) {
        iRoamRange = RoamingRange(pSoldier, &sOrigin);
        if (PythSpacesAway(sOrigin, sGridNo) > iRoamRange) {
          continue;
        }
      }

      // exclude locations with tear/mustard gas (at this point, smoke is cool!)
      if (InGas(pSoldier, sGridNo)) {
        continue;
      }

      // OK, this place shows potential.  How useful is it as cover?
      // NumMessage("Promising seems gridno #",gridno);

      iSpotClosestThreatRange = 1500;

      if (pSoldier->bTeam == ENEMY_TEAM && GridNoOnEdgeOfMap(sGridNo, &bEscapeDirection)) {
        // We can escape!  This is better than anything else except a closer
        // spot which we can cross over from.

        // Subtract the straight-line distance from our location to this one as
        // an estimate of path cost and for looks...

        // The edge spot closest to us which is on the edge will have the
        // highest value, so it will be picked over locations further away. Only
        // reachable gridnos will be picked so this should hopefully look okay
        iSpotClosestThreatRange -= PythSpacesAway(pSoldier->sGridNo, sGridNo);

      } else {
        bEscapeDirection = -1;
        // for every opponent that threatens, consider this spot's cover vs. him
        for (uint32_t uiLoop = 0; uiLoop < uiThreatCnt; ++uiLoop) {
          // iThreatRange = AdjPixelsAway(CenterX(sGridNo),CenterY(sGridNo),
          // CenterX(sThreatGridNo[iLoop]),CenterY(sThreatGridNo[iLoop]));
          iThreatRange = GetRangeInCellCoordsFromGridNoDiff(sGridNo, sThreatGridNo[uiLoop]);
          if (iThreatRange < iSpotClosestThreatRange) {
            iSpotClosestThreatRange = iThreatRange;
          }
        }
      }

      // if this is better than the best place found so far
      // (i.e. the closest guy would be farther away than previously)
      if (iSpotClosestThreatRange > iClosestThreatRange) {
        // remember it instead
        iClosestThreatRange = iSpotClosestThreatRange;
        // NumMessage("New best range = ",iClosestThreatRange);
        sBestSpot = sGridNo;
        bBestEscapeDirection = bEscapeDirection;
        // NumMessage("New best grid = ",bestSpot);
      }
    }
  }

  gubNPCAPBudget = 0;
  gubNPCDistLimit = 0;

  if (bBestEscapeDirection != -1) {
    // Woohoo!  We can escape!  Fake some stuff with the quote-related actions
    pSoldier->ubQuoteActionID = GetTraversalQuoteActionID(bBestEscapeDirection);
  }

  return (sBestSpot);
}

int16_t FindNearestUngassedLand(SOLDIERTYPE *pSoldier) {
  int16_t sGridNo, sClosestLand = NOWHERE, sPathCost, sShortestPath = 1000;
  int16_t sMaxLeft, sMaxRight, sMaxUp, sMaxDown, sXOffset, sYOffset;
  int32_t iSearchRange;

  // NameMessage(pSoldier,"looking for nearest reachable land");

  // start with a small search area, and expand it if we're unsuccessful
  // this should almost never need to search farther than 5 or 10 squares...
  for (iSearchRange = 5; iSearchRange <= 25; iSearchRange += 5) {
    // NumMessage("Trying iSearchRange = ", iSearchRange);

    // determine maximum horizontal limits
    sMaxLeft = std::min(iSearchRange, (pSoldier->sGridNo % MAXCOL));
    // NumMessage("sMaxLeft = ",sMaxLeft);
    sMaxRight = std::min(iSearchRange, MAXCOL - ((pSoldier->sGridNo % MAXCOL) + 1));
    // NumMessage("sMaxRight = ",sMaxRight);

    // determine maximum vertical limits
    sMaxUp = std::min(iSearchRange, (pSoldier->sGridNo / MAXROW));
    // NumMessage("sMaxUp = ",sMaxUp);
    sMaxDown = std::min(iSearchRange, MAXROW - ((pSoldier->sGridNo / MAXROW) + 1));
    // NumMessage("sMaxDown = ",sMaxDown);

    // Call FindBestPath to set flags in all locations that we can
    // walk into within range.  We have to set some things up first...

    // set the distance limit of the square region
    gubNPCDistLimit = (uint8_t)iSearchRange;

    // reset the "reachable" flags in the region we're looking at
    for (sYOffset = -sMaxUp; sYOffset <= sMaxDown; sYOffset++) {
      for (sXOffset = -sMaxLeft; sXOffset <= sMaxRight; sXOffset++) {
        sGridNo = pSoldier->sGridNo + sXOffset + (MAXCOL * sYOffset);
        if (!(sGridNo >= 0 && sGridNo < WORLD_MAX)) {
          continue;
        }

        gpWorldLevelData[sGridNo].uiFlags &= ~(MAPELEMENT_REACHABLE);
      }
    }

    FindBestPath(pSoldier, NOWHERE, pSoldier->bLevel,
                 DetermineMovementMode(pSoldier, AI_ACTION_LEAVE_WATER_GAS), COPYREACHABLE, 0);

    // Turn off the "reachable" flag for his current location
    // so we don't consider it
    gpWorldLevelData[pSoldier->sGridNo].uiFlags &= ~(MAPELEMENT_REACHABLE);

    // SET UP double-LOOP TO STEP THROUGH POTENTIAL GRID #s
    for (sYOffset = -sMaxUp; sYOffset <= sMaxDown; sYOffset++) {
      for (sXOffset = -sMaxLeft; sXOffset <= sMaxRight; sXOffset++) {
        // calculate the next potential gridno
        sGridNo = pSoldier->sGridNo + sXOffset + (MAXCOL * sYOffset);
        // NumMessage("Testing gridno #",gridno);
        if (!(sGridNo >= 0 && sGridNo < WORLD_MAX)) {
          continue;
        }

        if (!(gpWorldLevelData[sGridNo].uiFlags & MAPELEMENT_REACHABLE)) {
          continue;
        }

        // ignore blacklisted spot
        if (sGridNo == pSoldier->sBlackList) {
          continue;
        }

        // CJC: here, unfortunately, we must calculate a path so we have an AP
        // cost

        // obviously, we're looking for LAND, so water is out!
        sPathCost = LegalNPCDestination(pSoldier, sGridNo, ENSURE_PATH_COST, NOWATER, 0);

        if (!sPathCost) {
          continue;  // skip on to the next potential grid
        }

        // if this path is shorter than the one to the closest land found so far
        if (sPathCost < sShortestPath) {
          // remember it instead
          sShortestPath = sPathCost;
          // NumMessage("New shortest route = ",shortestPath);

          sClosestLand = sGridNo;
          // NumMessage("New closest land at grid = ",closestLand);
        }
      }
    }

    // if we found a piece of land in this search area
    if (sClosestLand != NOWHERE)  // quit now, no need to look any farther
      break;
  }

  // NumMessage("closestLand = ",closestLand);
  return (sClosestLand);
}

int16_t FindNearbyDarkerSpot(SOLDIERTYPE *pSoldier) {
  int16_t sGridNo, sClosestSpot = NOWHERE, sPathCost;
  int32_t iSpotValue, iBestSpotValue = 1000;
  int16_t sMaxLeft, sMaxRight, sMaxUp, sMaxDown, sXOffset, sYOffset;
  int32_t iSearchRange;
  int8_t bLightLevel, bCurrLightLevel, bLightDiff;
  int32_t iRoamRange;
  int16_t sOrigin;

  bCurrLightLevel = LightTrueLevel(pSoldier->sGridNo, pSoldier->bLevel);

  iRoamRange = RoamingRange(pSoldier, &sOrigin);

  // start with a small search area, and expand it if we're unsuccessful
  // this should almost never need to search farther than 5 or 10 squares...
  for (iSearchRange = 5; iSearchRange <= 15; iSearchRange += 5) {
    // determine maximum horizontal limits
    sMaxLeft = std::min(iSearchRange, (pSoldier->sGridNo % MAXCOL));
    // NumMessage("sMaxLeft = ",sMaxLeft);
    sMaxRight = std::min(iSearchRange, MAXCOL - ((pSoldier->sGridNo % MAXCOL) + 1));
    // NumMessage("sMaxRight = ",sMaxRight);

    // determine maximum vertical limits
    sMaxUp = std::min(iSearchRange, (pSoldier->sGridNo / MAXROW));
    // NumMessage("sMaxUp = ",sMaxUp);
    sMaxDown = std::min(iSearchRange, MAXROW - ((pSoldier->sGridNo / MAXROW) + 1));
    // NumMessage("sMaxDown = ",sMaxDown);

    // Call FindBestPath to set flags in all locations that we can
    // walk into within range.  We have to set some things up first...

    // set the distance limit of the square region
    gubNPCDistLimit = (uint8_t)iSearchRange;

    // reset the "reachable" flags in the region we're looking at
    for (sYOffset = -sMaxUp; sYOffset <= sMaxDown; sYOffset++) {
      for (sXOffset = -sMaxLeft; sXOffset <= sMaxRight; sXOffset++) {
        sGridNo = pSoldier->sGridNo + sXOffset + (MAXCOL * sYOffset);
        if (!(sGridNo >= 0 && sGridNo < WORLD_MAX)) {
          continue;
        }

        gpWorldLevelData[sGridNo].uiFlags &= ~(MAPELEMENT_REACHABLE);
      }
    }

    FindBestPath(pSoldier, NOWHERE, pSoldier->bLevel,
                 DetermineMovementMode(pSoldier, AI_ACTION_LEAVE_WATER_GAS), COPYREACHABLE, 0);

    // Turn off the "reachable" flag for his current location
    // so we don't consider it
    gpWorldLevelData[pSoldier->sGridNo].uiFlags &= ~(MAPELEMENT_REACHABLE);

    // SET UP double-LOOP TO STEP THROUGH POTENTIAL GRID #s
    for (sYOffset = -sMaxUp; sYOffset <= sMaxDown; sYOffset++) {
      for (sXOffset = -sMaxLeft; sXOffset <= sMaxRight; sXOffset++) {
        // calculate the next potential gridno
        sGridNo = pSoldier->sGridNo + sXOffset + (MAXCOL * sYOffset);
        // NumMessage("Testing gridno #",gridno);
        if (!(sGridNo >= 0 && sGridNo < WORLD_MAX)) {
          continue;
        }

        if (!(gpWorldLevelData[sGridNo].uiFlags & MAPELEMENT_REACHABLE)) {
          continue;
        }

        // ignore blacklisted spot
        if (sGridNo == pSoldier->sBlackList) {
          continue;
        }

        // require this character to stay within their roam range
        if (PythSpacesAway(sOrigin, sGridNo) > iRoamRange) {
          continue;
        }

        // screen out anything brighter than our current best spot
        bLightLevel = LightTrueLevel(sGridNo, pSoldier->bLevel);

        bLightDiff = gbLightSighting[0][bCurrLightLevel] - gbLightSighting[0][bLightLevel];

        // if the spot is darker than our current location, then bLightDiff > 0
        // plus ignore differences of just 1 light level
        if (bLightDiff <= 1) {
          continue;
        }

        // CJC: here, unfortunately, we must calculate a path so we have an AP
        // cost

        sPathCost = LegalNPCDestination(pSoldier, sGridNo, ENSURE_PATH_COST, NOWATER, 0);

        if (!sPathCost) {
          continue;  // skip on to the next potential grid
        }

        // decrease the "cost" of the spot by the amount of light/darkness
        iSpotValue = sPathCost * 2 - bLightDiff;

        if (iSpotValue < iBestSpotValue) {
          // remember it instead
          iBestSpotValue = iSpotValue;
          // NumMessage("New shortest route = ",shortestPath);

          sClosestSpot = sGridNo;
          // NumMessage("New closest land at grid = ",closestLand);
        }
      }
    }

    // if we found a piece of land in this search area
    if (sClosestSpot != NOWHERE)  // quit now, no need to look any farther
    {
      break;
    }
  }

  return (sClosestSpot);
}

static bool IsGunUsable(OBJECTTYPE const &o, SOLDIERTYPE const &s) {
  Assert(Item[o.usItem].usItemClass & IC_GUN);
  return o.bGunAmmoStatus > 0 && o.ubGunShotsLeft != 0 &&
         (!HasObjectImprint(o) || o.ubImprintID == s.ubID);  // XXX wrong: should compare profile
}

#define MINIMUM_REQUIRED_STATUS 70

int8_t SearchForItems(SOLDIERTYPE &s, ItemSearchReason const reason, uint16_t const usItem) {
  if (s.bActionPoints < AP_PICKUP_ITEM) return AI_ACTION_NONE;

  if (!IS_MERC_BODY_TYPE(&s)) return AI_ACTION_NONE;

  int32_t search_range = gbDiff[DIFF_MAX_COVER_RANGE][SoldierDifficultyLevel(&s)];

  switch (s.bAttitude) {
    case DEFENSIVE:
      search_range -= 1;
      break;
    case BRAVESOLO:
      search_range += 2;
      break;
    case BRAVEAID:
      search_range += 2;
      break;
    case CUNNINGSOLO:
      search_range -= 2;
      break;
    case CUNNINGAID:
      search_range -= 2;
      break;
    case AGGRESSIVE:
      search_range += 1;
      break;
  }

  // Maximum search range is 1 tile / 10 pts of wisdom
  if (search_range > s.bWisdom / 10) {
    search_range = s.bWisdom / 10;
  }

  if (!gfTurnBasedAI) {  // Don't search so far in realtime
    search_range /= 2;
  }

  // Don't search so far for items
  search_range /= 2;

  // determine maximum horizontal limits
  int16_t const max_left = std::min(search_range, s.sGridNo % MAXCOL);
  int16_t const max_right = std::min(search_range, MAXCOL - (s.sGridNo % MAXCOL + 1));

  // determine maximum vertical limits
  int16_t const max_up = std::min(search_range, s.sGridNo / MAXROW);
  int16_t const max_down = std::min(search_range, MAXROW - (s.sGridNo / MAXROW + 1));

  // Call FindBestPath to set flags in all locations that we can
  // walk into within range.  We have to set some things up first...

  // set the distance limit of the square region
  gubNPCDistLimit = search_range;

  // set an AP limit too, to our APs less the cost of picking up an item
  // and less the cost of dropping an item since we might need to do that
  gubNPCAPBudget = s.bActionPoints - AP_PICKUP_ITEM;

  // reset the "reachable" flags in the region we're looking at
  for (int16_t dy = -max_up; dy <= max_down; ++dy) {
    for (int16_t dx = -max_left; dx <= max_right; ++dx) {
      GridNo const grid_no = s.sGridNo + dx + MAXCOL * dy;
      if (grid_no < 0 || WORLD_MAX <= grid_no) continue;
      gpWorldLevelData[grid_no].uiFlags &= ~MAPELEMENT_REACHABLE;
    }
  }

  FindBestPath(&s, NOWHERE, s.bLevel, DetermineMovementMode(&s, AI_ACTION_PICKUP_ITEM),
               COPYREACHABLE, 0);

  GridNo best_spot = NOWHERE;
  int32_t best_value = 0;
  int32_t item_idx = -1;
  int32_t best_item_idx = -1;
  for (int16_t dy = -max_up; dy <= max_down; ++dy) {
    for (int16_t dx = -max_left; dx <= max_right; ++dx) {
      // calculate the next potential gridno
      int16_t const grid_no = s.sGridNo + dx + (MAXCOL * dy);
      if (grid_no < 0 || WORLD_MAX <= grid_no) continue;

      // exclude locations with tear/mustard gas (at this point, smoke is cool!)
      if (InGasOrSmoke(&s, grid_no)) continue;

      if (!(gpWorldLevelData[grid_no].uiFlags & MAPELEMENT_ITEMPOOL_PRESENT)) continue;
      if (!(gpWorldLevelData[grid_no].uiFlags & MAPELEMENT_REACHABLE)) continue;

      // ignore blacklisted spot
      if (grid_no == s.sBlackList) continue;

      int32_t value = 0;
      for (ITEM_POOL const *pItemPool = GetItemPool(grid_no, s.bLevel); pItemPool;
           pItemPool = pItemPool->pNext) {
        OBJECTTYPE const &o = GetWorldItem(pItemPool->iItemIndex).o;
        INVTYPE const &item = Item[o.usItem];
        int32_t temp_value;
        switch (reason) {
          case SEARCH_AMMO:
            // We are looking for ammo to match the gun in usItem
            if (item.usItemClass == IC_GUN &&
                o.bStatus[0] >= MINIMUM_REQUIRED_STATUS) {  // Maybe this gun has ammo (adjust
                                                            // for whether it is better than
                                                            // ours!)
              if (!IsGunUsable(o, s)) continue;
              temp_value =
                  o.ubGunShotsLeft * Weapon[o.usItem].ubDeadliness / Weapon[usItem].ubDeadliness;
            } else {
              if (!ValidAmmoType(usItem, o.usItem)) continue;
              temp_value = TotalPoints(&o);
            }
            break;

          default:
            if (item.usItemClass == IC_ARMOUR && o.bStatus[0] >= MINIMUM_REQUIRED_STATUS) {
              InvSlotPos pos;
              switch (Armour[item.ubClassIndex].ubArmourClass) {
                case ARMOURCLASS_HELMET:
                  pos = HELMETPOS;
                  break;
                case ARMOURCLASS_VEST:
                  pos = VESTPOS;
                  break;
                case ARMOURCLASS_LEGGINGS:
                  pos = LEGPOS;
                  break;
                default:
                  continue;
              }
              OBJECTTYPE const &cur_armour = s.inv[pos];
              if (cur_armour.usItem == NOTHING) {
                temp_value = 200 + EffectiveArmour(&o);
              } else {
                int8_t const new_rating = EffectiveArmour(&o);
                if (EffectiveArmour(&s.inv[HELMETPOS]) <= new_rating)
                  continue;  // XXX makes no sense: always compare with helmet and
                             // only consider when worse
                temp_value = 100 * new_rating / EffectiveArmour(&cur_armour);
              }
            }
            /* FALLTHROUGH */

          case SEARCH_WEAPONS: {
            if (!(item.usItemClass & IC_WEAPON)) continue;
            if (o.bStatus[0] < MINIMUM_REQUIRED_STATUS) continue;
            if (item.usItemClass & IC_GUN && !IsGunUsable(o, s)) continue;
            WEAPONTYPE const &new_wpn = Weapon[o.usItem];
            OBJECTTYPE const &in_hand = s.inv[HANDPOS];
            if (Item[in_hand.usItem].usItemClass & IC_WEAPON) {
              WEAPONTYPE const &cur_wpn = Weapon[in_hand.usItem];
              if (new_wpn.ubDeadliness <= cur_wpn.ubDeadliness) continue;
              temp_value = 100 * new_wpn.ubDeadliness / cur_wpn.ubDeadliness;
            } else {
              temp_value = 200 + new_wpn.ubDeadliness;
            }
            break;
          }
        }

        if (value < temp_value) {
          value = temp_value;
          item_idx = pItemPool->iItemIndex;
        }
      }
      value = 3 * value / (3 + PythSpacesAway(grid_no, s.sGridNo));
      if (best_value < value) {
        best_value = value;
        best_spot = grid_no;
        best_item_idx = item_idx;
      }
    }
  }

  if (best_spot == NOWHERE) return AI_ACTION_NONE;

  OBJECTTYPE const &o = GetWorldItem(best_item_idx).o;
  DebugAI(String("%d decides to pick up %ls", s.ubID, ItemNames[o.usItem]));
  if (Item[o.usItem].usItemClass == IC_GUN && !FindBetterSpotForItem(s, HANDPOS)) {
    if (s.bActionPoints < AP_PICKUP_ITEM + AP_PICKUP_ITEM) {
      return AI_ACTION_NONE;
    }
    if (s.inv[HANDPOS].fFlags & OBJECT_UNDROPPABLE) {  // Destroy this item
      DebugAI(String("%d decides he must drop %ls first so destroys it", s.ubID,
                     ItemNames[s.inv[HANDPOS].usItem]));
      DeleteObj(&s.inv[HANDPOS]);
      DeductPoints(&s, AP_PICKUP_ITEM, 0);
    } else {  // We want to drop this item
      DebugAI(
          String("%d decides he must drop %ls first", s.ubID, ItemNames[s.inv[HANDPOS].usItem]));
      s.bNextAction = AI_ACTION_PICKUP_ITEM;
      s.usNextActionData = best_spot;
      s.iNextActionSpecialData = best_item_idx;
      return AI_ACTION_DROP_ITEM;
    }
  }
  s.uiPendingActionData1 = best_item_idx;
  s.usActionData = best_spot;
  return AI_ACTION_PICKUP_ITEM;
}

int16_t FindClosestDoor(SOLDIERTYPE *pSoldier) {
  int16_t sClosestDoor = NOWHERE;
  int32_t iSearchRange;
  int16_t sMaxLeft, sMaxRight, sMaxUp, sMaxDown, sXOffset, sYOffset;
  int16_t sGridNo;
  int32_t iDist, iClosestDist = 10;

  iSearchRange = 5;

  // determine maximum horizontal limits
  sMaxLeft = std::min(iSearchRange, (pSoldier->sGridNo % MAXCOL));
  // NumMessage("sMaxLeft = ",sMaxLeft);
  sMaxRight = std::min(iSearchRange, MAXCOL - ((pSoldier->sGridNo % MAXCOL) + 1));
  // NumMessage("sMaxRight = ",sMaxRight);

  // determine maximum vertical limits
  sMaxUp = std::min(iSearchRange, (pSoldier->sGridNo / MAXROW));
  // NumMessage("sMaxUp = ",sMaxUp);
  sMaxDown = std::min(iSearchRange, MAXROW - ((pSoldier->sGridNo / MAXROW) + 1));
  // NumMessage("sMaxDown = ",sMaxDown);
  // SET UP double-LOOP TO STEP THROUGH POTENTIAL GRID #s
  for (sYOffset = -sMaxUp; sYOffset <= sMaxDown; sYOffset++) {
    for (sXOffset = -sMaxLeft; sXOffset <= sMaxRight; sXOffset++) {
      // calculate the next potential gridno
      sGridNo = pSoldier->sGridNo + sXOffset + (MAXCOL * sYOffset);
      if (FindStructure(sGridNo, STRUCTURE_ANYDOOR) != NULL) {
        iDist = PythSpacesAway(pSoldier->sGridNo, sGridNo);
        if (iDist < iClosestDist) {
          iClosestDist = iDist;
          sClosestDoor = sGridNo;
        }
      }
    }
  }

  return (sClosestDoor);
}

int16_t FindNearestEdgepointOnSpecifiedEdge(int16_t sGridNo, int8_t bEdgeCode) {
  int32_t iLoop;
  int16_t *psEdgepointArray;
  int32_t iEdgepointArraySize;
  int16_t sClosestSpot = NOWHERE, sClosestDist = 0x7FFF, sTempDist;

  switch (bEdgeCode) {
    case NORTH_EDGEPOINT_SEARCH:
      psEdgepointArray = gps1stNorthEdgepointArray;
      iEdgepointArraySize = gus1stNorthEdgepointArraySize;
      break;
    case EAST_EDGEPOINT_SEARCH:
      psEdgepointArray = gps1stEastEdgepointArray;
      iEdgepointArraySize = gus1stEastEdgepointArraySize;
      break;
    case SOUTH_EDGEPOINT_SEARCH:
      psEdgepointArray = gps1stSouthEdgepointArray;
      iEdgepointArraySize = gus1stSouthEdgepointArraySize;
      break;
    case WEST_EDGEPOINT_SEARCH:
      psEdgepointArray = gps1stWestEdgepointArray;
      iEdgepointArraySize = gus1stWestEdgepointArraySize;
      break;
    default:
      // WTF???
      return (NOWHERE);
  }

  // Do a 2D search to find the closest map edgepoint and
  // try to create a path there

  for (iLoop = 0; iLoop < iEdgepointArraySize; iLoop++) {
    sTempDist = PythSpacesAway(sGridNo, psEdgepointArray[iLoop]);
    if (sTempDist < sClosestDist) {
      sClosestDist = sTempDist;
      sClosestSpot = psEdgepointArray[iLoop];
    }
  }

  return (sClosestSpot);
}

int16_t FindNearestEdgePoint(int16_t sGridNo) {
  int16_t sScreenX, sScreenY, sMaxScreenX, sMaxScreenY;
  int16_t sDist[5], sMinDist;
  int32_t iLoop;
  int8_t bMinIndex;
  int16_t *psEdgepointArray;
  int32_t iEdgepointArraySize;
  int16_t sClosestSpot = NOWHERE, sClosestDist = 0x7FFF, sTempDist;

  GetAbsoluteScreenXYFromMapPos(sGridNo, &sScreenX, &sScreenY);

  sMaxScreenX = gsBRX - gsTLX;
  sMaxScreenY = gsBRY - gsTLY;

  sDist[0] = 0x7FFF;
  sDist[1] = sScreenX;                // west
  sDist[2] = sMaxScreenX - sScreenX;  // east
  sDist[3] = sScreenY;                // north
  sDist[4] = sMaxScreenY - sScreenY;  // south

  sMinDist = sDist[0];
  bMinIndex = 0;
  for (iLoop = 1; iLoop < 5; iLoop++) {
    if (sDist[iLoop] < sMinDist) {
      sMinDist = sDist[iLoop];
      bMinIndex = (int8_t)iLoop;
    }
  }

  switch (bMinIndex) {
    case 1:
      psEdgepointArray = gps1stWestEdgepointArray;
      iEdgepointArraySize = gus1stWestEdgepointArraySize;
      break;
    case 2:
      psEdgepointArray = gps1stEastEdgepointArray;
      iEdgepointArraySize = gus1stEastEdgepointArraySize;
      break;
    case 3:
      psEdgepointArray = gps1stNorthEdgepointArray;
      iEdgepointArraySize = gus1stNorthEdgepointArraySize;
      break;
    case 4:
      psEdgepointArray = gps1stSouthEdgepointArray;
      iEdgepointArraySize = gus1stSouthEdgepointArraySize;
      break;
    default:
      // WTF???
      return (NOWHERE);
  }

  // Do a 2D search to find the closest map edgepoint and
  // try to create a path there

  for (iLoop = 0; iLoop < iEdgepointArraySize; iLoop++) {
    sTempDist = PythSpacesAway(sGridNo, psEdgepointArray[iLoop]);
    if (sTempDist < sClosestDist) {
      sClosestDist = sTempDist;
      sClosestSpot = psEdgepointArray[iLoop];
    }
  }

  return (sClosestSpot);
}

#define EDGE_OF_MAP_SEARCH 5

int16_t FindNearbyPointOnEdgeOfMap(SOLDIERTYPE *pSoldier, int8_t *pbDirection) {
  int32_t iSearchRange;
  int16_t sMaxLeft, sMaxRight, sMaxUp, sMaxDown, sXOffset, sYOffset;

  int16_t sGridNo, sClosestSpot = NOWHERE;
  int8_t bDirection, bClosestDirection;
  int32_t iPathCost, iClosestPathCost = 1000;

  bClosestDirection = -1;

  // Call FindBestPath to set flags in all locations that we can
  // walk into within range.  We have to set some things up first...

  // set the distance limit of the square region
  gubNPCDistLimit = EDGE_OF_MAP_SEARCH;

  iSearchRange = EDGE_OF_MAP_SEARCH;

  // determine maximum horizontal limits
  sMaxLeft = std::min(iSearchRange, (pSoldier->sGridNo % MAXCOL));
  // NumMessage("sMaxLeft = ",sMaxLeft);
  sMaxRight = std::min(iSearchRange, MAXCOL - ((pSoldier->sGridNo % MAXCOL) + 1));
  // NumMessage("sMaxRight = ",sMaxRight);

  // determine maximum vertical limits
  sMaxUp = std::min(iSearchRange, (pSoldier->sGridNo / MAXROW));
  // NumMessage("sMaxUp = ",sMaxUp);
  sMaxDown = std::min(iSearchRange, MAXROW - ((pSoldier->sGridNo / MAXROW) + 1));

  // reset the "reachable" flags in the region we're looking at
  for (sYOffset = -sMaxUp; sYOffset <= sMaxDown; sYOffset++) {
    for (sXOffset = -sMaxLeft; sXOffset <= sMaxRight; sXOffset++) {
      sGridNo = pSoldier->sGridNo + sXOffset + (MAXCOL * sYOffset);
      if (!(sGridNo >= 0 && sGridNo < WORLD_MAX)) {
        continue;
      }

      gpWorldLevelData[sGridNo].uiFlags &= ~(MAPELEMENT_REACHABLE);
    }
  }

  FindBestPath(pSoldier, NOWHERE, pSoldier->bLevel, WALKING, COPYREACHABLE, 0);

  // Turn off the "reachable" flag for his current location
  // so we don't consider it
  gpWorldLevelData[pSoldier->sGridNo].uiFlags &= ~(MAPELEMENT_REACHABLE);

  // SET UP double-LOOP TO STEP THROUGH POTENTIAL GRID #s
  for (sYOffset = -sMaxUp; sYOffset <= sMaxDown; sYOffset++) {
    for (sXOffset = -sMaxLeft; sXOffset <= sMaxRight; sXOffset++) {
      // calculate the next potential gridno
      sGridNo = pSoldier->sGridNo + sXOffset + (MAXCOL * sYOffset);
      if (!(sGridNo >= 0 && sGridNo < WORLD_MAX)) {
        continue;
      }

      if (!(gpWorldLevelData[sGridNo].uiFlags & MAPELEMENT_REACHABLE)) {
        continue;
      }

      if (GridNoOnEdgeOfMap(sGridNo, &bDirection)) {
        iPathCost = PythSpacesAway(pSoldier->sGridNo, sGridNo);

        if (iPathCost < iClosestPathCost) {
          // this place is closer
          sClosestSpot = sGridNo;
          iClosestPathCost = iPathCost;
          bClosestDirection = bDirection;
        }
      }
    }
  }

  *pbDirection = bClosestDirection;
  return (sClosestSpot);
}

int16_t FindClosestBoxingRingSpot(SOLDIERTYPE *pSoldier, BOOLEAN fInRing) {
  int32_t iSearchRange;
  int16_t sMaxLeft, sMaxRight, sMaxUp, sMaxDown, sXOffset, sYOffset;

  int16_t sGridNo, sClosestSpot = NOWHERE;
  int32_t iDistance, iClosestDistance = 9999;

  // set the distance limit of the square region
  iSearchRange = 7;

  // determine maximum horizontal limits
  sMaxLeft = std::min(iSearchRange, (pSoldier->sGridNo % MAXCOL));
  // NumMessage("sMaxLeft = ",sMaxLeft);
  sMaxRight = std::min(iSearchRange, MAXCOL - ((pSoldier->sGridNo % MAXCOL) + 1));
  // NumMessage("sMaxRight = ",sMaxRight);

  if (pSoldier->bTeam == OUR_TEAM && !fInRing) {
    // have player not go to the left of the ring
    sMaxLeft = 0;
  }

  // determine maximum vertical limits
  sMaxUp = std::min(iSearchRange, (pSoldier->sGridNo / MAXROW));
  // NumMessage("sMaxUp = ",sMaxUp);
  sMaxDown = std::min(iSearchRange, MAXROW - ((pSoldier->sGridNo / MAXROW) + 1));

  for (sYOffset = -sMaxUp; sYOffset <= sMaxDown; sYOffset++) {
    for (sXOffset = -sMaxLeft; sXOffset <= sMaxRight; sXOffset++) {
      // calculate the next potential gridno
      sGridNo = pSoldier->sGridNo + sXOffset + (MAXCOL * sYOffset);
      uint8_t const room = GetRoom(sGridNo);
      if (room == NO_ROOM) continue;

      if ((fInRing && room == BOXING_RING) ||
          ((!fInRing && room != BOXING_RING) &&
           LegalNPCDestination(pSoldier, sGridNo, IGNORE_PATH, NOWATER, 0))) {
        iDistance = abs(sXOffset) + abs(sYOffset);
        if (iDistance < iClosestDistance && WhoIsThere2(sGridNo, 0) == NULL) {
          sClosestSpot = sGridNo;
          iClosestDistance = iDistance;
        }
      }
    }
  }

  return (sClosestSpot);
}

int16_t FindNearestOpenableNonDoor(int16_t sStartGridNo) {
  int32_t iSearchRange;
  int16_t sMaxLeft, sMaxRight, sMaxUp, sMaxDown, sXOffset, sYOffset;

  int16_t sGridNo, sClosestSpot = NOWHERE;
  int32_t iDistance, iClosestDistance = 9999;

  // set the distance limit of the square region
  iSearchRange = 7;

  // determine maximum horizontal limits
  sMaxLeft = std::min(iSearchRange, (sStartGridNo % MAXCOL));
  // NumMessage("sMaxLeft = ",sMaxLeft);
  sMaxRight = std::min(iSearchRange, MAXCOL - ((sStartGridNo % MAXCOL) + 1));
  // NumMessage("sMaxRight = ",sMaxRight);

  // determine maximum vertical limits
  sMaxUp = std::min(iSearchRange, (sStartGridNo / MAXROW));
  // NumMessage("sMaxUp = ",sMaxUp);
  sMaxDown = std::min(iSearchRange, MAXROW - ((sStartGridNo / MAXROW) + 1));

  for (sYOffset = -sMaxUp; sYOffset <= sMaxDown; sYOffset++) {
    for (sXOffset = -sMaxLeft; sXOffset <= sMaxRight; sXOffset++) {
      // calculate the next potential gridno
      sGridNo = sStartGridNo + sXOffset + (MAXCOL * sYOffset);
      FOR_EACH_STRUCTURE(pStructure, sGridNo, STRUCTURE_OPENABLE) {
        // skip any doors
        if (pStructure->fFlags & STRUCTURE_ANYDOOR) continue;

        // we have found a valid non-door openable structure
        iDistance = CardinalSpacesAway(sGridNo, sStartGridNo);
        if (iDistance < iClosestDistance) {
          sClosestSpot = sGridNo;
          iClosestDistance = iDistance;
        }
        break;
      }
    }
  }

  return (sClosestSpot);
}
