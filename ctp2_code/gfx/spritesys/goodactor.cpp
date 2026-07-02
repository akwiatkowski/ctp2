//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ source
// Description  : Good actor
// Id           : $Id$
//
//----------------------------------------------------------------------------
//
// Disclaimer
//
// THIS FILE IS NOT GENERATED OR SUPPORTED BY ACTIVISION.
//
// This material has been developed at apolyton.net by the Apolyton CtP2
// Source Code Project. Contact the authors at ctp2source@apolyton.net.
//
//----------------------------------------------------------------------------
//
// Compiler flags
//
// _ACTOR_DRAW_OPTIMIZATION
//
//----------------------------------------------------------------------------
//
// Modifications from the original Activision code:
//
// - None
//
//----------------------------------------------------------------------------

#include <memory>

#include "ctp/c3.h"
#include "gfx/spritesys/GoodActor.h"

#include "ui/aui_common/aui.h"
#include "gfx/gfx_utils/pixelutils.h"
#include "gfx/tilesys/tileutils.h"
#include "gs/gameobj/Unit.h"

#include "ui/aui_ctp2/SelItem.h"                // selitem_Get()

#include "gfx/spritesys/FacedSprite.h"
#include "gfx/spritesys/GoodSpriteGroup.h"
#include "gfx/spritesys/SpriteState.h"
#include "gfx/spritesys/Actor.h"
#include "gfx/spritesys/SpriteGroupList.h"
#include "gfx/tilesys/tiledmap.h"               // tiledmap_Get()
#include "gfx/spritesys/Anim.h"

#include "gfx/spritesys/ActorPath.h"
#include "gfx/spritesys/Action.h"
#include "gfx/spritesys/director.h"
#include "gfx/tilesys/maputils.h"

extern SpriteGroupList	*   g_goodSpriteGroupList;

namespace {
/// Default transparency value
/// @todo Move to better location
uint16 const        TRANSPARENCY_DEFAULT = 15;
}

GoodActor::GoodActor(sint32 index, const MapPoint &pos) :
    Actor(SpriteStatePtr()),
    m_facing(0),
    m_frame(0),
    m_transparency(TRANSPARENCY_DEFAULT),
    m_index(index),
    m_pos(pos),
    m_goodSpriteGroup(nullptr),
    m_curGoodAction(GOODACTION_IDLE),
    m_loadType(LOADTYPE_BASIC) {
  Assert(g_goodSpriteGroupList);
  m_goodSpriteGroup = (GoodSpriteGroup *)
    g_goodSpriteGroupList->GetSprite(index, GROUPTYPE_GOOD, GetLoadType(), (GAME_ACTION)0);

  AddIdle();
}

GoodActor::GoodActor(GoodActor const & rhs) :
    Actor(rhs),
    m_goodSpriteGroup(nullptr),
    m_loadType(LOADTYPE_BASIC) {
  *this = rhs;
}

GoodActor & GoodActor::operator=(GoodActor const & rhs) {
  if (this != &rhs) {
    Assert(g_goodSpriteGroupList);

    if (m_goodSpriteGroup) {
      g_goodSpriteGroupList->ReleaseSprite(m_index, GetLoadType());
      m_goodSpriteGroup = nullptr;
    }

    Actor::operator=(rhs);
    m_facing = rhs.m_facing;
    m_frame = rhs.m_frame;
    m_transparency = rhs.m_transparency;
    m_index = rhs.m_index;
    m_pos = rhs.m_pos;
    m_curGoodAction = rhs.m_curGoodAction;
    m_curAction = rhs.m_curAction
                      ? std::make_shared<Action>(*rhs.m_curAction)
                      : nullptr;
    m_loadType = rhs.m_loadType;
    m_goodSpriteGroup = (GoodSpriteGroup *)
        g_goodSpriteGroupList->GetSprite(m_index, GROUPTYPE_GOOD, GetLoadType(), (GAME_ACTION)0);
  }

  return *this;
}

GoodActor::~GoodActor() {
  m_curAction.reset();
  m_actionQueue.Clear();

  if (g_goodSpriteGroupList && m_goodSpriteGroup) {
    g_goodSpriteGroupList->ReleaseSprite(m_index, GetLoadType());
  }
}

void GoodActor::FullLoad() { // Doesn't seem to do much
  if (!m_goodSpriteGroup) return;
  if (m_loadType == LOADTYPE_FULL) return;

  SpriteGroup *group = g_goodSpriteGroupList->GetSprite(m_index, GROUPTYPE_GOOD, LOADTYPE_FULL, (GAME_ACTION)0);
  Assert(group == m_goodSpriteGroup);

  m_loadType = LOADTYPE_FULL;
  m_frame = 0;
}

void GoodActor::DumpFullLoad() {
  if (!m_goodSpriteGroup) return;
  if (m_loadType != LOADTYPE_FULL) return;

  bool purged = g_goodSpriteGroupList->ReleaseSprite(m_index, LOADTYPE_FULL);

  if (purged) {
    m_goodSpriteGroup = nullptr;
  } else {
    m_loadType = LOADTYPE_BASIC;
  }

  m_frame = 0;
}

void GoodActor::PositionActor(MapPoint &pos) {
  sint32 pixelX;
  sint32 pixelY;
  maputils_MapXY2PixelXY(pos.x, pos.y, &pixelX, &pixelY);
  Actor::SetPos(pixelX, pixelY);
  SetPos(pos);
}

void GoodActor::AddIdle() {
  if (m_curAction) return;

  m_curAction = std::make_shared<Action>(GOODACTION_IDLE, ACTIONEND_ANIMEND);

  std::unique_ptr<Anim> anim = CreateAnim(GOODACTION_IDLE);
  if (anim) {
    m_curAction->SetAnim(anim.release());
    m_curAction->SetDelay(0);
    m_curGoodAction = GOODACTION_IDLE;
  }
}

void GoodActor::Process() {
  sint32		tickCount = GetTickCount();

  if (m_curAction) {
    m_curAction->Process();

    if (m_curAction->Finished()) {
      if (m_curAction->GetDelay() > 0 &&
        tickCount > m_curAction->GetDelay()
        ) {
        MapPoint  end;
        m_curAction->GetEndMapPoint(end);
        if (end.x != 0 || end.y != 0) {
          m_pos = end;
        }

        GetNextAction();
      } else {
        if (m_curAction->GetDelay() == 0) {
          m_curAction->SetDelay(tickCount + 8000 + rand() % 5000);
        } else {
          return;
        }
      }
    }
  }

  if (m_curAction) {
    sint32 x;
    sint32 y;
    maputils_MapXY2PixelXY(m_pos.x, m_pos.y, &x, &y);
    Actor::SetPos(x, y);

    m_frame = m_curAction->GetSpriteFrame();

    m_transparency = m_curAction->GetTransparency();

    if (m_curAction->GetPath()) {
      Actor::SetPos(m_curAction->GetPosition());
    }

    m_facing = m_curAction->GetFacing();
  }
}

void GoodActor::GetNextAction() {
  m_curAction.reset();

  if (!m_actionQueue.Empty()) {
    m_curAction = m_actionQueue.Back();
    m_actionQueue.Pop();
    if (m_curAction) {
      m_curGoodAction = (GOODACTION)m_curAction->GetActionType();
    } else {
      Assert(FALSE);
    }
  } else {

    AddIdle();
  }
}

void GoodActor::AddAction(ActionPtr actionObj) {
  Assert(m_goodSpriteGroup && actionObj);
  if (!m_goodSpriteGroup || !actionObj) return;

  m_actionQueue.Push(actionObj);

  if (m_curAction) {
    if (m_curAction->GetAnim()->GetType() == ANIMTYPE_LOOPED) {
      m_curAction->SetFinished(TRUE);
    }
  }
}

std::unique_ptr<Anim> GoodActor::CreateAnim(GOODACTION action) {
  Assert(m_goodSpriteGroup);
  if (!m_goodSpriteGroup) return nullptr;

  Anim	*origAnim = m_goodSpriteGroup->GetAnim((GAME_ACTION)action);
  if (origAnim == nullptr) {
    origAnim = m_goodSpriteGroup->GetAnim((GAME_ACTION)GOODACTION_IDLE);
  }

  return origAnim ? std::make_unique<Anim>(*origAnim) : nullptr;
}

void GoodActor::DrawSelectionBrackets() {

  TileSet		*tileSet = tiledmap_Get()->GetTileSet();

  RECT		rect;
  SetRect(&rect, 0, 0, 1, 1);


  OffsetRect(&rect, m_x + (sint32)(k_TILE_PIXEL_WIDTH * tiledmap_Get()->GetScale()) / 2,
    m_y + (sint32)(k_TILE_GRID_HEIGHT * tiledmap_Get()->GetScale()) / 2);

  InflateRect(&rect, 25, 25);

  tiledmap_Get()->AddDirtyRectToMix(rect);

  POINT iconDim = tileSet->GetMapIconDimensions(MAPICON_BRACKET1);

  rect.right -= (iconDim.x + 1);
  rect.bottom -= (iconDim.y + 1);

  Pixel16 * topLeft = tileSet->GetMapIconData(MAPICON_BRACKET1);
  Assert(topLeft); if (!topLeft) return;
  Pixel16 * topRight = tileSet->GetMapIconData(MAPICON_BRACKET2);
  Assert(topRight); if (!topRight) return;
  Pixel16 * botRight = tileSet->GetMapIconData(MAPICON_BRACKET3);
  Assert(botRight); if (!botRight) return;
  Pixel16 * botLeft = tileSet->GetMapIconData(MAPICON_BRACKET4);
  Assert(botLeft); if (!botLeft) return;

  tiledmap_Get()->DrawColorizedOverlayIntoMix(topLeft, rect.left, rect.top);
  tiledmap_Get()->DrawColorizedOverlayIntoMix(topRight, rect.right, rect.top);
  tiledmap_Get()->DrawColorizedOverlayIntoMix(botRight, rect.right, rect.bottom);
  tiledmap_Get()->DrawColorizedOverlayIntoMix(botLeft, rect.left, rect.bottom);
}

bool GoodActor::Draw(bool fogged) {
  uint16			flags = k_DRAWFLAGS_NORMAL;
  Pixel16			color = 0x0000;
  sint32			xoffset = (sint32)((double)k_ACTOR_CENTER_OFFSET_X * tiledmap_Get()->GetScale());
  sint32			yoffset = (sint32)((double)k_ACTOR_CENTER_OFFSET_Y * tiledmap_Get()->GetScale());










#ifdef _ACTOR_DRAW_OPTIMIZATION

  if ((m_frame == m_oldFrame) &&
    (m_x + xoffset == m_oldOffsetX) && (m_y + yoffset == m_oldOffsetY)) {
    if (m_paintTwice < 2) {
      m_paintTwice++;
    }
    return false;
  }

  m_paintTwice = 0;

  m_oldFrame = m_frame;
  m_oldOffsetX = m_x + xoffset;
  m_oldOffsetY = m_y + yoffset;
#endif

  if (fogged)
    flags |= k_BIT_DRAWFLAGS_FOGGED;

  m_goodSpriteGroup->Draw(m_curGoodAction, m_frame, m_x + xoffset, m_y + yoffset, m_facing,
    tiledmap_Get()->GetScale(), m_transparency, color, flags);

  if (selitem_Get()->GetState() == SELECT_TYPE_GOOD) {
    if (m_pos == selitem_Get()->GetCurSelectPos()) {
      DrawSelectionBrackets();
    }
  }

  return true;
}

void GoodActor::DrawDirect(aui_Surface *surf, sint32 x, sint32 y, double scale) {
  uint16			flags = k_DRAWFLAGS_NORMAL;
  Pixel16			color = 0x0000;

  sint32			xoffset = (sint32)((double)k_ACTOR_CENTER_OFFSET_X * scale);
  sint32			yoffset = (sint32)((double)k_ACTOR_CENTER_OFFSET_Y * scale);

  m_goodSpriteGroup->DrawDirect(surf, GOODACTION_IDLE, m_frame, x + xoffset, y + yoffset, m_facing, scale,
    m_transparency, color, flags);
}

void GoodActor::DrawText(sint32 x, sint32 y, MBCHAR const * goodText) {
  m_goodSpriteGroup->DrawText(x, y, goodText);
}

POINT GoodActor::GetHotpoint() const {
  POINT pt = { 0,0 };

  if (!m_goodSpriteGroup) return pt;
  if (!m_goodSpriteGroup->GetGroupSprite((GAME_ACTION)GOODACTION_IDLE)) return pt;

  return m_goodSpriteGroup->GetGroupSprite((GAME_ACTION)GOODACTION_IDLE)->GetHotPoint();
}

bool GoodActor::IsAnimating() const {
  return false;
}

uint16 GoodActor::GetWidth() const {
  Assert(m_goodSpriteGroup);
  if (!m_goodSpriteGroup) return 0;

  Sprite	*theSprite = m_goodSpriteGroup->GetGroupSprite((GAME_ACTION)m_curGoodAction);
  return (theSprite) ? theSprite->GetWidth() : 0;
}

uint16 GoodActor::GetHeight() const {
  Assert(m_goodSpriteGroup);
  if (!m_goodSpriteGroup) return 0;

  Sprite *theSprite = m_goodSpriteGroup->GetGroupSprite((GAME_ACTION)m_curGoodAction);
  return (theSprite) ? theSprite->GetHeight() : 0;
}

void GoodActor::GetBoundingRect(RECT *rect) const {
  Assert(m_goodSpriteGroup && rect);
  if (!m_goodSpriteGroup || !rect) return;

  POINT	hotPoint = m_goodSpriteGroup->GetHotPoint(m_curGoodAction);
  double	scale = tiledmap_Get()->GetScale();
  sint32	 xoff = (sint32)((double)(k_ACTOR_CENTER_OFFSET_X - hotPoint.x) * scale);
  sint32	 yoff = (sint32)((double)(k_ACTOR_CENTER_OFFSET_Y - hotPoint.y) * scale);

  rect->left = 0;
  rect->top = 0;
  rect->right = (sint32)((double)GetWidth() * scale);
  rect->bottom = (sint32)((double)GetHeight() * scale);

  OffsetRect(rect, m_x + xoff, m_y + yoff);
}


