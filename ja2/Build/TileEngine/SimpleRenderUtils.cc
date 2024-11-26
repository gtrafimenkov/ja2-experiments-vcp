#include "TileEngine/SimpleRenderUtils.h"

#include "TileEngine/IsometricUtils.h"
#include "TileEngine/RenderWorld.h"

void MarkMapIndexDirty(int32_t iMapIndex) {
  gpWorldLevelData[iMapIndex].uiFlags |= MAPELEMENT_REDRAW;
  SetRenderFlags(RENDER_FLAG_MARKED);
}

void CenterScreenAtMapIndex(const int32_t iMapIndex) {
  // Set the render values, so that the screen will render here next frame.
  ConvertGridNoToCellXY(iMapIndex, &gsRenderCenterX, &gsRenderCenterY);
  SetRenderFlags(RENDER_FLAG_FULL);
}

void MarkWorldDirty() { SetRenderFlags(RENDER_FLAG_FULL); }
