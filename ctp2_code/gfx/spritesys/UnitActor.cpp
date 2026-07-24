//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ source
// Description  : Unit actor
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
// _DEBUG
// - Generates debug information when set.
//
// _DEBUG_MEMORY
// - Generates debug information when set.
//
// _TEST
// ?
//
// _ACTOR_DRAW_OPTIMIZATION
//
//----------------------------------------------------------------------------
//
// Modifications from the original Activision code:
//
// - Fixed number of city styles removed.
// - Prevented crashes due to uninitialised members.
// - Prevented some NULL-dereferencing crashes.
// - Exposed city walls and force field graphics to agecitystyle.txt,
//   by Martin G�hmann.
// - Prevented crashes with invalid (i.e. killed or destroyed) units.
// - PFT 29 mar 05, show # turns until city next grows a pop.
// - Removed refferences to the civilisation database. (Aug 20th 2005 Martin
// G�hmann)
// - Removed unnecessary include files. (Aug 28th 2005 Martin G�hmann)
// - Initialized local variables. (Sep 9th 2005 Martin G�hmann)
// - Removed unused local variables. (Sep 9th 2005 Martin G�hmann)
// - Fixed memory leaks.
// - added Hidden Nationality check for units 2-21-2007
// - Added Civilization flag MAPICONS
// - Added MapIcon database (3-Mar-2007 Martin G�hmann)
// - Implemented but then outcomment DrawCityImps just didn't come out right
//   maybe revisit
// - Made DrawStackingIndicator only the stack sized moved the rest to
// DrawIndicators
// - Made StackingIndicator above the healthbar per Maquiladora's design
// - Move Civ flag underneath the healthbar.
// - Unit stacking indications and special indecations are placed according
//   their size. (9-Feb-2008 Martin G�hmann)
// - Made the elite icon replace the veteran icon, rather than sit below it.
//	 (11-Apr-2009 Maq)
// - Stopped the cargo icon showing for enemy transports if they're only
// carrying
//	 stealth units. (13-Apr-2009 Maq)
// - Changed occurances of UnitRecord::GetMaxHP to
//   UnitData::CalculateTotalHP. (Aug 3rd 2009 Maq)
//
//----------------------------------------------------------------------------

#include "ctp/c3.h"
#include "gfx/spritesys/UnitActor.h"

#include <memory>

#include "ctp/debugtools/debugmemory.h"
#include "gfx/gfx_utils/colorset.h"  // g_colorset
#include "gfx/spritesys/SpriteGroupList.h"
#include "gfx/spritesys/SpriteState.h"
#include "gfx/spritesys/director.h"  // director_Get()
#include "gfx/spritesys/screenmanager.h"
#include "gfx/tilesys/maputils.h"
#include "gfx/tilesys/tiledmap.h"   // tiledmap_Get()
#include "gs/database/profileDB.h"  // profiledb_Get()
#include "gs/fileio/gamefile.h"     // save_file_version_Get()
#include "gs/gameobj/ArmyData.h"
#include "gs/gameobj/Civilisation.h"
#include "gs/gameobj/Player.h"  // player_Get()
#include "gs/utility/safety.h"  // safe_player
#include "gs/gameobj/UnitData.h"
#include "gs/gameobj/UnitPool.h"
#include "gs/gameobj/buildingutil.h"
#include "gs/gameobj/wonderutil.h"
#include "AgeCityStyleRecord.h"
#include "BuildingRecord.h"
#include "CTPRecord.h"
#include "CityStyleRecord.h"
#include "UnitRecord.h"
#include "WonderRecord.h"
#include "gs/world/World.h"      // world_Get()
#include "gs/world/cellunitlist.h"
#include "sound/soundmanager.h"  // soundmgr_Get()
#include "ui/aui_common/aui_bitmapfont.h"
#include "ui/aui_sdl/aui_sdl.h"
#include "ui/aui_ctp2/SelItem.h"  // selitem_Get()
#include "ui/aui_utils/primitives.h"
#include "ui/interface/citywindow.h"  // s_cityWindow

extern SpriteGroupList* g_unitSpriteGroupList;
extern SpriteGroupList* g_citySpriteGroupList;
extern PointerList<Player>* g_deadPlayer;

#define k_SHIELD_ON_TIME 650
#define k_SHIELD_OFF_TIME 150

#ifdef _DEBUG_MEMORY
#define STOMPCHECK() ;
//#define STOMPCHECK() if (m_curAction) {
//Assert(_CrtIsMemoryBlock(m_curAction.get(), sizeof(Action),NULL,NULL,NULL));}
#else
#define STOMPCHECK() ;
#endif

bool g_showHeralds = true;

namespace {
sint32 const CITY_TYPE_LAND = 0;
sint32 const CITY_TYPE_WATER = 1;

bool AddGpuMapIconQuad(TileSet *tileSet, MAPICON icon, sint32 x, sint32 y, Pixel16 color) {
  POINT iconDim = tileSet->GetMapIconDimensions(icon);
  Pixel16 *iconData = tileSet->GetMapIconData(icon);
  SDL_Texture *texture = aui_SDL::EnsureMapIconTexture(iconData, iconDim.x, iconDim.y, color);
  if (!texture)
    return false;
  aui_SDL::AddSpriteQuad({texture, 0, 0, iconDim.x, iconDim.y, x, y, iconDim.x, iconDim.y, false, 255});
  return true;
}

bool AddGpuSolidRect(RECT const &rect, Pixel16 color) {
  if (rect.right <= rect.left || rect.bottom <= rect.top)
    return true;
  SDL_Texture *texture = aui_SDL::EnsureSolidColorTexture(color);
  if (!texture)
    return false;
  aui_SDL::AddSpriteQuad({texture, 0, 0, 1, 1, rect.left, rect.top,
                          rect.right - rect.left, rect.bottom - rect.top, false, 255});
  return true;
}

bool AddGpuSelectionBrackets(sint32 x, sint32 y, double scale, Unit unitID) {
  TileSet* tileSet = tiledmap_Get()->GetTileSet();
  RECT rect;
  SetRect(&rect, 0, 0, 1, 1);
  OffsetRect(&rect,
             x + (sint32)(k_TILE_PIXEL_WIDTH * scale) / 2,
             y + (sint32)(k_TILE_GRID_HEIGHT * scale) / 2);
  InflateRect(&rect, 25, 25);

  POINT iconDim = tileSet->GetMapIconDimensions(MAPICON_BRACKET1);
  rect.right -= (iconDim.x + 1);
  rect.bottom -= (iconDim.y + 1);

  COLOR color = COLOR_YELLOW;
  if (unitID.IsValid()) {
    if (unitID.GetArmy().IsValid() && unitID.GetArmy().CanMove())
      color = COLOR_GREEN;
    else if (unitID.IsCity())
      color = COLOR_RED;
  }
  Pixel16 pixelColor = colorset_Get()->GetColor(color);

  return AddGpuMapIconQuad(tileSet, MAPICON_BRACKET1, rect.left, rect.top, pixelColor)
      && AddGpuMapIconQuad(tileSet, MAPICON_BRACKET2, rect.right, rect.top, pixelColor)
      && AddGpuMapIconQuad(tileSet, MAPICON_BRACKET3, rect.right, rect.bottom, pixelColor)
      && AddGpuMapIconQuad(tileSet, MAPICON_BRACKET4, rect.left, rect.bottom, pixelColor);
}

};  // namespace

UnitActor::UnitActor(SpriteStatePtr ss,
                     Unit id,
                     sint32 unitType,
                     const MapPoint& pos,
                     sint32 owner,
                     BOOL isUnseenCellActor,
                     double visionRange,
                     sint32 citySprite)
    : Actor(ss),
      m_refCount(1),
      m_pos(pos),
      m_unitID(id),
      m_unitDBIndex(unitType),
      m_playerNum(owner),
      m_nextPop(0),
      m_unitSpriteGroup(nullptr),
      m_loadType(LOADTYPE_NONE),
      m_facing(k_DEFAULTSPRITEFACING),
      m_lastMoveFacing(k_DEFAULTSPRITEFACING),
      m_frame(0),
      m_curUnitAction(UNITACTION_NONE),
      m_unitVisibility(0),
      m_unitSaveVisibility(0),
      m_directionalAttack(false),
      m_needsToDie(false),
      m_needsToVictor(false),
      m_killNow(false),
      m_unitVisionRange(visionRange),
      m_newUnitVisionRange(0.0),
      m_bVisSpecial(false),
      m_moveActors(NULL),
      m_numOActors(0),
      m_hidden(false),
      m_hiddenUnderStack(false),
      m_isTransported(false),
      //	sint32
      //m_holdingCurAnimPos[UNITACTION_MAX]; 	sint32
      //m_holdingCurAnimDelayEnd[UNITACTION_MAX]; 	sint32
      //m_holdingCurAnimElapsed[UNITACTION_MAX]; 	sint32
      //m_holdingCurAnimLastFrameTime[UNITACTION_MAX]; 	sint32
      //m_holdingCurAnimSpecialDelayProcess;
      m_size(0),
      m_isUnseenCellActor(isUnseenCellActor),
      //	GROUPTYPE			m_type;
      //	sint32				m_spriteID;
      //	uint32				m_shieldFlashOnTime;
      //	uint32				m_shieldFlashOffTime;
      //	sint32				m_activeListRef;
      //	double				m_healthPercent;
      m_tempStackSize(0)
#ifdef _ACTOR_DRAW_OPTIMIZATION
//	sint32				m_oldFacing;
//	BOOL				m_oldIsFortified;
//	BOOL				m_oldIsFortifying;
//	BOOL				m_oldHasCityWalls;
//	BOOL				m_oldHasForceField;
//	BOOL				m_oldDrawShield;
//	BOOL				m_oldDrawSelectionBrackets;
//	uint16				m_oldFlags;
#endif
{
  sint32 spriteID = CTPRecord::INDEX_INVALID;
  GetIDAndType(owner, ss, id, unitType, pos, &spriteID, &m_type, citySprite);

  m_spriteID = (spriteID < 0) ? 0 : spriteID;
  Assert(m_spriteID >= 0);

  Initialize();
}

void UnitActor::Initialize() {
  RECT tmpRect = {0, 0, 10, 16};

  m_heraldRect = tmpRect;
  m_bVisSpecial = FALSE;
  m_animPos = 0;
  m_needsToDie = FALSE;
  m_needsToVictor = FALSE;
  m_killNow = FALSE;
  m_numOActors = 0;
  m_curUnitAction = UNITACTION_NONE;
  m_transparency = 0;

  for (sint32 i = UNITACTION_MOVE; i < UNITACTION_MAX; i++) {
    m_holdingCurAnimPos[i] = 0;
    m_holdingCurAnimDelayEnd[i] = 0;
    m_holdingCurAnimElapsed[i] = 0;
    m_holdingCurAnimLastFrameTime[i] = 0;
    m_holdingCurAnimSpecialDelayProcess = FALSE;
  }

  if (m_type == GROUPTYPE_UNIT) {
    m_unitSpriteGroup = (UnitSpriteGroup*)g_unitSpriteGroupList->GetSprite(
        (uint32)m_spriteID, m_type, LOADTYPE_BASIC, (GAME_ACTION)0);
  } else {
    m_unitSpriteGroup = (UnitSpriteGroup*)g_citySpriteGroupList->GetSprite(
        (uint32)m_spriteID, m_type, LOADTYPE_BASIC, (GAME_ACTION)0);
  }

  m_loadType = LOADTYPE_BASIC;

  Assert(m_unitSpriteGroup);
  if (m_unitSpriteGroup) {
    m_directionalAttack = m_unitSpriteGroup->HasDirectional();
  } else {
    m_directionalAttack = FALSE;
  }

  m_x = 0;
  m_y = 0;

  m_frame = 0;

  m_curAction.reset();

  m_savedRevealedActors.clear();
  m_revealedActors.clear();
  m_moveActors.clear();
  m_hiddenUnderStack = FALSE;
  m_isTransported = FALSE;

  m_hidden = FALSE;

  // m_isFortified / m_isFortifying / m_hasCityWalls / m_hasForceField
  // removed — all 4 are now read directly from gs/ each frame via
  // m_unitID (see UnitActor::Draw).

  m_shieldFlashOnTime = 0;
  m_shieldFlashOffTime = 0;

  m_activeListRef = 0;

  m_healthPercent = -1.0;

  m_tempStackSize = 0;

  AddIdle();

#ifndef _TEST
  STOMPCHECK();
#endif
}

void UnitActor::AddVision() {
#ifndef _TEST
  STOMPCHECK();
#endif
  if (!m_isUnseenCellActor) {
    if (Player *p = safe_player(m_playerNum))
      p->m_vision->AddVisible(GetPos(), m_unitVisionRange);
  }
}

void UnitActor::RemoveVision() {
#ifndef _TEST
  STOMPCHECK();
#endif

  if (!m_isUnseenCellActor) {
    if (Player *p = safe_player(m_playerNum))
      p->m_vision->RemoveVisible(GetPos(), m_unitVisionRange);
  }
}

void UnitActor::PositionActor(MapPoint const& pos) {
#ifndef _TEST
  STOMPCHECK();
#endif
  m_pos = pos;

  sint32 pixelX;
  sint32 pixelY;
  maputils_MapXY2PixelXY(pos.x, pos.y, &pixelX, &pixelY);

  SetX(pixelX);
  SetY(pixelY);
  SetPos(pos);
}

void UnitActor::GetIDAndType(sint32 owner,
                             SpriteStatePtr ss,
                             Unit id,
                             sint32 unitType,
                             MapPoint const& pos,
                             sint32* spriteID,
                             GROUPTYPE* groupType,
                             sint32 citySprite) const {
  Player *ownerPlayer = safe_player(owner);
  bool isCity = ownerPlayer &&
                g_theUnitDB->Get(unitType, ownerPlayer->GetGovernmentType())
                    ->GetHasPopAndCanBuild();

  if (isCity) {
    if (citySprite >= 0) {
      *spriteID = citySprite;
    } else if (id.IsValid() && id.CD()) {
      *spriteID = id.CD()->GetDesiredSpriteIndex();
    } else if (ss) {
      *spriteID = ss->GetIndex();
    } else {
      *spriteID = CTPRecord::INDEX_INVALID;
    }

    *groupType = GROUPTYPE_CITY;
  } else {
    *spriteID = ss->GetIndex();
    *groupType = GROUPTYPE_UNIT;
  }
}

UnitActor::~UnitActor() {
  DumpAllActions();

  // Global sprite lists can already be freed by the time UnitActor smart-ptrs
  // unwind during shutdown — guard against the teardown-order race rather
  // than dereferencing a dangling/null pointer. (ASan SEGV under 100-turn
  // autoplay traced to this destructor at shutdown.)
  if (m_type == GROUPTYPE_UNIT) {
    if (g_unitSpriteGroupList) {
      g_unitSpriteGroupList->ReleaseSprite(m_spriteID, m_loadType);
      if (LOADTYPE_BASIC != m_loadType) {
        g_unitSpriteGroupList->ReleaseSprite(m_spriteID, LOADTYPE_BASIC);
      }
    }
  } else {
    if (g_citySpriteGroupList) {
      g_citySpriteGroupList->ReleaseSprite(m_spriteID, m_loadType);
      if (LOADTYPE_BASIC != m_loadType) {
        g_citySpriteGroupList->ReleaseSprite(m_spriteID, LOADTYPE_BASIC);
      }
    }
  }
}

void UnitActor::Hide() {
  m_hidden = TRUE;
}

void UnitActor::Show() {
  m_hidden = FALSE;
}

void UnitActor::ChangeImage(SpriteStatePtr ss, sint32 type, Unit id) {
  if (id.IsValid() && id.IsCity()) {
    id.GetPop(m_size);  // put the city's pop into the actor's m_size

    CityWindow* cityWindow = CityWindow::GetCityWindow();
    CityData* citaData = cityWindow ? cityWindow->GetCityData() : nullptr;
    if (citaData != nullptr && citaData->GetHomeCity() == id) {
      m_nextPop = citaData->TurnsToNextPop();
    } else {
      // PFT, computes TurnsToNextPop and puts
      // it into the actor's m_nextPop
      id.GetTurnsToNextPop(m_nextPop);
    }
  }

  DumpAllActions();

  if (m_type == GROUPTYPE_UNIT) {
    if (g_unitSpriteGroupList->ReleaseSprite(m_spriteID, m_loadType))
      m_unitSpriteGroup = nullptr;
  } else {
    if (g_citySpriteGroupList->ReleaseSprite(m_spriteID, m_loadType))
      m_unitSpriteGroup = nullptr;
  }

  GROUPTYPE groupType;
  sint32 spriteID;
  GetIDAndType(m_playerNum, ss, id, type, GetPos(), &spriteID, &groupType);

  m_type = groupType;

  if (spriteID == -1) {
    spriteID = 1;

    const CityStyleRecord* styleRec = g_theCityStyleDB->Get(0);
    if (styleRec) {
      const AgeCityStyleRecord* ageStyleRec = styleRec->GetAgeStyle(0);
      if (ageStyleRec && ageStyleRec->GetSprites(0)) {
        spriteID = ageStyleRec->GetSprites(0)->GetSprite();
      }
    }
  }

  m_spriteID = spriteID;

  if (groupType == GROUPTYPE_UNIT) {
    m_unitSpriteGroup = (UnitSpriteGroup*)g_unitSpriteGroupList->GetSprite(
        spriteID, groupType, LOADTYPE_BASIC, (GAME_ACTION)0);
  } else {
    m_unitSpriteGroup = (UnitSpriteGroup*)g_citySpriteGroupList->GetSprite(
        spriteID, groupType, LOADTYPE_BASIC, (GAME_ACTION)0);

    m_lastMoveFacing = 0;
    m_facing = 0;
  }

  AddIdle();

  m_loadType = LOADTYPE_BASIC;
}

void UnitActor::ChangeType(SpriteStatePtr ss,
                           sint32 type,
                           Unit id,
                           BOOL updateVision) {
#ifndef _TEST
  STOMPCHECK();
#endif
  if (soundmgr_Get()) {
    soundmgr_Get()->TerminateLoopingSound(SOUNDTYPE_SFX, GetUnitID());
  }

  m_spriteState = ss;
  m_unitID = id;

  ChangeImage(ss, type, id);

  if (updateVision) {
    if (tiledmap_Get()->GetLocalVision() != nullptr &&
        m_playerNum == selitem_Get()->GetVisiblePlayer() &&
        !m_isUnseenCellActor) {
      DPRINTF(
          k_DBG_INFO,
          ("Removing vision for %lx, owner %d, range %lf, center: %d,%d\n",
           (uint32)m_unitID, m_playerNum, m_unitVisionRange, GetPos().x, GetPos().y));
    }
  }

  m_unitVisionRange = m_newUnitVisionRange;

  if (updateVision) {
    if (tiledmap_Get()->GetLocalVision() != nullptr &&
        m_playerNum == selitem_Get()->GetVisiblePlayer() &&
        !m_isUnseenCellActor) {
      DPRINTF(k_DBG_INFO,
              ("Adding vision for %lx, owner %d, range %lf, center: %d,%d\n",
               m_unitID, m_playerNum, m_unitVisionRange, GetPos().x, GetPos().y));
    }
  }

  m_directionalAttack =
      m_unitSpriteGroup && m_unitSpriteGroup->HasDirectional();

  if (id.IsValid()) {
    id.SetSpriteState(ss);
  }

  DumpAllActions();
  AddIdle();
}

void UnitActor::AddIdle(bool NoIdleJustDelay) {
  std::unique_ptr<Anim> anim = CreateAnim(UNITACTION_IDLE);
  m_frame = 0;

  if (anim == nullptr) {
    anim = CreateAnim(UNITACTION_MOVE);
  }

  if (anim && (!m_actionQueue.Empty() || NoIdleJustDelay)) {
    anim->SetNoIdleJustDelay(TRUE);
  }

  ActionPtr idleAction = std::make_shared<Action>(
      UNITACTION_IDLE, ACTIONEND_INTERRUPT, 0, NoIdleJustDelay);

  if (NoIdleJustDelay) {
    idleAction->SetFacing(m_facing);
  }

  idleAction->SetAnim(anim.release());

  AddAction(std::move(idleAction));

  if (soundmgr_Get())
    soundmgr_Get()->TerminateLoopingSound(SOUNDTYPE_SFX, GetUnitID());
}

void UnitActor::ActionQueueUpIdle(bool NoIdleJustDelay) {
  std::unique_ptr<Anim> anim = CreateAnim(UNITACTION_IDLE);

  if (anim == nullptr) {
    anim = CreateAnim(UNITACTION_MOVE);
    Assert(anim != nullptr);
  }

  if (anim && (!m_actionQueue.Empty() || NoIdleJustDelay)) {
    anim->SetNoIdleJustDelay(TRUE);
  }

  ActionPtr tempCurAction = std::make_shared<Action>(
      UNITACTION_IDLE, ACTIONEND_INTERRUPT, 0, NoIdleJustDelay);

  tempCurAction->SetAnim(anim.release());

  m_actionQueue.Push(std::move(tempCurAction));
}

void UnitActor::GetNextAction(bool isVisible) {
  m_curAction.reset();

  if (!m_actionQueue.Empty()) {
    m_curAction = m_actionQueue.Back();
    m_actionQueue.Pop();

    Assert(m_curAction);
    if (!m_curAction) {
      return;
    }

    if (m_curAction->GetActionType() != m_curUnitAction)
      m_frame = 0;

  } else {
    return;
  }

  if (m_curAction->m_actionType == UNITACTION_ATTACK) {
    SetUnitVisibility(m_curAction->GetUnitsVisibility());
  }

  m_curAction->SetSpecialDelayProcess(m_holdingCurAnimSpecialDelayProcess);

  MapPoint curStartMapPoint;
  MapPoint curEndMapPoint;

  m_curAction->GetStartMapPoint(curStartMapPoint);
  m_curAction->GetEndMapPoint(curEndMapPoint);

  auto& moveActors = m_curAction->GetMoveActors();
  long i;
  long j;

  if ((j = i = m_curAction->GetNumOActors()) > 0) {
    i--;
    for (; i >= 0; --i) {
      if (!moveActors[i].expired()) {
        moveActors[i].lock()->SetHiddenUnderStack(TRUE);
      }

      if (!m_isUnseenCellActor &&
          m_playerNum == selitem_Get()->GetVisiblePlayer()) {
      }
    }
    // @TODO: whats is this?
    m_curAction->SetNumOActors(0 - j);
  }

  if (m_playerNum == selitem_Get()->GetVisiblePlayer() &&
      m_curAction->GetActionType() == UNITACTION_MOVE) {
    if (!m_curAction->GetIsSpecialActionType()) {
      if (m_isTransported == k_TRANSPORTADDONLY) {
        m_isTransported = FALSE;
      }
    }

    if (!m_savedRevealedActors.empty()) {
      for (UnitActorWeakPtr tempActor : m_savedRevealedActors) {
        Assert(!tempActor.expired());
        if (!tempActor.expired())
          tempActor.lock()->SetUnitVisibility(m_curAction->GetUnitsVisibility(),
                                              TRUE);
      }

      m_savedRevealedActors.clear();
    }

    auto& revealedActors = m_curAction->GetRevealedActors();
    if (!revealedActors.empty()) {
      for (auto tempActor : revealedActors) {
        Assert(!tempActor.expired());
        if (!tempActor.expired()) {
          tempActor.lock()->SetUnitVisibility(m_curAction->GetUnitsVisibility(),
                                              TRUE);
        }
      }

      SaveRevealedActors(revealedActors);

      m_revealedActors.clear();
      m_curAction->SetRevealedActors(std::vector<UnitActorWeakPtr>());
    }
  }

  m_curUnitAction = (UNITACTION)m_curAction->GetActionType();
}

void UnitActor::Process() {
  if (!m_curAction)
    GetNextAction();

  if (!m_curAction) {
    DumpFullLoad();

    AddIdle(m_facing != 3);
  }

  if (!m_curAction)
    return;

  m_curAction->Process();

  if (m_curAction->Finished()) {
    if (m_curUnitAction == UNITACTION_MOVE) {
      MapPoint pos;

      m_curAction->GetEndMapPoint(pos);
      PositionActor(pos);
    }

    auto& moveActors = m_curAction->GetMoveActors();

    if (!moveActors.empty()) {
      sint32 num = abs(m_curAction->GetNumOActors());
      m_curAction->SetNumOActors(num);

      MapPoint here = GetPos();
      for (std::weak_ptr<UnitActor> actor : moveActors) {
        if (!actor.expired())
          actor.lock()->PositionActor(here);
      }
      m_curAction->SetMoveActors(std::vector<std::weak_ptr<UnitActor> >());
    }

    if (m_curAction->m_actionType != UNITACTION_IDLE &&
        m_curAction->m_actionType != UNITACTION_FACE_OFF)
      director_Get()->ActionFinished(m_curAction->GetSequence());

    if ((m_curAction->m_actionType == UNITACTION_VICTORY && HasDeath()) ||
        m_curAction->m_actionType == UNITACTION_FAKE_DEATH) {
      SetKillNow();
      m_curAction.reset();
    } else {
      GetNextAction();
    }

    director_Get()->HandleNextAction();
  } else {
    if (m_curAction->GetPath() != nullptr) {
      POINT curPt = m_curAction->GetPosition();
      m_x = curPt.x;
      m_y = curPt.y;
    } else {
      sint32 x;
      sint32 y;
      MapPoint const here = GetPos();
      maputils_MapXY2PixelXY(here.x, here.y, &x, &y);
      m_x = x;
      m_y = y;
    }

    if (m_curAction->GetActionType() == UNITACTION_MOVE ||
        m_curAction->GetActionType() == UNITACTION_ATTACK) {
      m_lastMoveFacing = m_curAction->GetFacing();
    }

    if (m_curAction->SpecialDelayProcess() ||
        (m_curUnitAction == UNITACTION_IDLE && m_unitSpriteGroup &&
         m_unitSpriteGroup->GetGroupSprite((GAME_ACTION)m_curUnitAction) ==
             nullptr)) {
      m_facing = m_lastMoveFacing;
    } else {
      m_facing = m_curAction->GetFacing();
    }

    m_frame = m_curAction->GetSpriteFrame();

    m_transparency = m_curAction->GetTransparency();
  }
}

ActionPtr UnitActor::WillDie() const {
#ifndef _TEST
  STOMPCHECK();
#endif
  sint32 type;

  if (m_curAction) {
    type = m_curAction->m_actionType;
    if (type == UNITACTION_VICTORY) {
      if (HasDeath())
        return m_curAction;
    } else if (type == UNITACTION_FAKE_DEATH) {
      return m_curAction;
    }
  }

  for (const ActionPtr& action : m_actionQueue.Container()) {
    if (action) {
      type = action->m_actionType;
      if (type == UNITACTION_VICTORY) {
        if (HasDeath())
          return action;
      } else if (type == UNITACTION_FAKE_DEATH) {
        return action;
      }
    }
  }

  return nullptr;
}

ActionPtr UnitActor::WillMorph() const {
#ifndef _TEST
  STOMPCHECK();
#endif

  if (m_curAction && (UNITACTION_MORPH == m_curAction->m_actionType)) {
    return m_curAction;
  }

  for (const ActionPtr& action : m_actionQueue.Container()) {
    if (action && (action->m_actionType == UNITACTION_MORPH)) {
      return action;
    }
  }

  return nullptr;
}

void UnitActor::DumpAllActions() {
#ifndef _TEST
  STOMPCHECK();
#endif
  static MapPoint pos;

  if (m_curAction != nullptr) {
    m_facing = m_curAction->GetFacing();

    if (m_curAction->m_actionType == UNITACTION_MOVE) {
      m_curAction->GetEndMapPoint(pos);
      PositionActor(pos);
    }

    if (m_curAction->m_actionType != UNITACTION_IDLE &&
        m_curAction->m_actionType != UNITACTION_FACE_OFF) {
      director_Get()->ActionFinished(m_curAction->GetSequence());
    }
    m_curAction.reset();
  }

  ActionPtr deadAction;
  while (!m_actionQueue.Empty()) {
    deadAction = m_actionQueue.Back();
    m_actionQueue.Pop();
    if (deadAction != nullptr) {
      m_facing = deadAction->GetFacing();

      if (deadAction->m_actionType == UNITACTION_MOVE) {
        deadAction->GetEndMapPoint(pos);
        PositionActor(pos);
      }

      if (deadAction->m_actionType != UNITACTION_IDLE &&
          deadAction->m_actionType != UNITACTION_FACE_OFF) {
        director_Get()->ActionFinished(deadAction->GetSequence());
      }

      deadAction.reset();
    } else {
      Assert(FALSE);
      return;
    }
  }
}

void UnitActor::EndTurnProcess() {
#ifndef _TEST
  STOMPCHECK();
#endif

  DumpAllActions();

  if (soundmgr_Get())
    soundmgr_Get()->TerminateLoopingSound(SOUNDTYPE_SFX, GetUnitID());
}

void UnitActor::AddAction(ActionPtr actionObj) {
#ifndef _TEST
  STOMPCHECK();
#endif
  Assert(m_unitSpriteGroup != nullptr);
  if (m_unitSpriteGroup == nullptr)
    return;

  Assert(actionObj);

  if (unitpool_Get()) {
    if (unitpool_Get()->IsValid(GetUnitID())) {
      m_playerNum = Unit(GetUnitID()).GetOwner();
    }
  }

  m_actionQueue.Push(actionObj);

  if (m_curAction) {
    if (m_curAction->GetCurrentEndCondition() == ACTIONEND_INTERRUPT) {
      m_curAction->SetFinished(TRUE);
    }
  } else {
    GetNextAction();
  }
}

std::unique_ptr<Anim> UnitActor::CreateAnim(UNITACTION action) {
#ifndef _TEST
  STOMPCHECK();
#endif
  Assert(m_unitSpriteGroup != nullptr);
  if (m_unitSpriteGroup == nullptr)
    return nullptr;

  Anim* origAnim = m_unitSpriteGroup->GetAnim((GAME_ACTION)action);

  if (origAnim == nullptr) {
    if (action != UNITACTION_IDLE) {
      return nullptr;
    } else {
      origAnim = m_unitSpriteGroup->GetAnim((GAME_ACTION)UNITACTION_MOVE);
      ;
      //			Assert(origAnim != NULL);
      if (origAnim == nullptr)
        return nullptr;
      else
        action = UNITACTION_MOVE;
    }
  }

  auto anim = std::make_unique<Anim>(*origAnim);

  if (anim->GetType() == ANIMTYPE_LOOPED) {
    anim->SetDelayEnd(m_holdingCurAnimDelayEnd[action]);
    anim->SetElapsed(m_holdingCurAnimElapsed[action]);
    anim->SetLastFrameTime(director_Get()->GetMasterCurTime() -
                           m_holdingCurAnimElapsed[action]);

    if (m_holdingCurAnimDelayEnd[action] != 0)
      anim->SetWeAreInDelay(TRUE);
  }

  if (action == UNITACTION_IDLE) {
    srand(anim->GetDelay() + director_Get()->GetMasterCurTime());
    anim->AdjustDelay(rand() % 2000);
  }

  return anim;
}

#define k_FAKE_DEATH_FRAMES 15
#define k_FAKE_DEATH_DURATION 1500

std::unique_ptr<Anim> UnitActor::MakeFakeDeath() {
  std::vector<uint16> frames(k_FAKE_DEATH_FRAMES, 0);

  POINT pt = {0, 0};
  std::vector<POINT> moveDeltas(k_FAKE_DEATH_FRAMES, pt);

  std::vector<uint16> transparencies(k_FAKE_DEATH_FRAMES);
  for (int i = 0; i < k_FAKE_DEATH_FRAMES; i++) {
    transparencies[i] = (uint16)(15 - i);
  }

  auto anim = std::make_unique<Anim>();
  anim->SetNumFrames(k_FAKE_DEATH_FRAMES);
  anim->SetFrames(frames.data(), frames.size());
  anim->SetPlaybackTime(k_FAKE_DEATH_DURATION);
  anim->SetDeltas(moveDeltas.data(), moveDeltas.size());
  anim->SetTransparencies(transparencies.data(), transparencies.size());
  anim->SetType(ANIMTYPE_SEQUENTIAL);

  return anim;
}

#define k_FACEOFF_FRAMES 1
#define k_FACEOFF_DURATION 1000

std::unique_ptr<Anim> UnitActor::MakeFaceoff() {
  std::vector<uint16> frames(k_FACEOFF_FRAMES, 0);

  POINT pt = {0, 0};
  std::vector<POINT> moveDeltas(k_FACEOFF_FRAMES, pt);

  std::vector<uint16> transparencies(k_FACEOFF_FRAMES, 15);

  auto anim = std::make_unique<Anim>();
  anim->SetNumFrames(k_FACEOFF_FRAMES);
  anim->SetFrames(frames.data(), frames.size());
  anim->SetPlaybackTime(k_FACEOFF_DURATION);
  anim->SetDeltas(moveDeltas.data(), moveDeltas.size());
  anim->SetTransparencies(transparencies.data(), transparencies.size());
  anim->SetType(ANIMTYPE_LOOPED);

  return anim;
}

void UnitActor::DrawFortified(bool fogged) {
	TileSet* tileSet = tiledmap_Get()->GetTileSet();
	if (!tileSet)
		return;

  sint32 nudgeX =
      (sint32)((k_ACTOR_CENTER_OFFSET_X - 48) * tiledmap_Get()->GetScale());
  sint32 nudgeY =
      (sint32)((k_ACTOR_CENTER_OFFSET_Y - 48) * tiledmap_Get()->GetScale());
  sint32 surfWidth = screenmanager_Get()->GetSurfWidth();
  sint32 surfHeight = screenmanager_Get()->GetSurfHeight();

  if ((m_x + nudgeX) > (surfWidth - tiledmap_Get()->GetZoomTilePixelWidth()))
    return;

  if ((m_y + nudgeY) > (surfHeight - tiledmap_Get()->GetZoomTilePixelHeight()))
    return;

  Pixel16* fortifiedImage = tileSet->GetImprovementData(34);
  if (!fortifiedImage)
	return;

  if (tiledmap_Get()->GetZoomLevel() == k_ZOOM_LARGEST) {
    if (fogged)
      tiledmap_Get()->DrawBlendedOverlayIntoMix(fortifiedImage, m_x + nudgeX,
                                            m_y + nudgeY, k_FOW_COLOR,
                                            k_FOW_BLEND_VALUE);
    else
      tiledmap_Get()->DrawColorizedOverlayIntoMix(fortifiedImage, m_x + nudgeX,
                                              m_y + nudgeY, 0x0000);
  } else {
    if (fogged)
      tiledmap_Get()->DrawBlendedOverlayScaledIntoMix(
          fortifiedImage, m_x + nudgeX, m_y + nudgeY,
          tiledmap_Get()->GetZoomTilePixelWidth(),
          tiledmap_Get()->GetZoomTileGridHeight(), k_FOW_COLOR, k_FOW_BLEND_VALUE);
    else
      tiledmap_Get()->DrawScaledOverlayIntoMix(fortifiedImage, m_x + nudgeX,
                                           m_y + nudgeY,
                                           tiledmap_Get()->GetZoomTilePixelWidth(),
                                           tiledmap_Get()->GetZoomTileGridHeight());
  }
}

void UnitActor::DrawFortifying(bool fogged) {
  aui_BitmapFont* font = tiledmap_Get()->GetFont();
  if (!font)
    return;

  sint32 x = m_x +
             (sint32)(double)(k_ACTOR_CENTER_OFFSET_X * tiledmap_Get()->GetScale());
  sint32 y = m_y +
             (sint32)(double)(k_ACTOR_CENTER_OFFSET_Y * tiledmap_Get()->GetScale());

  MBCHAR* fString = tiledmap_Get()->GetFortifyString();

  sint32 width = font->GetStringWidth(fString);
  sint32 height = font->GetMaxHeight();

  RECT rect = {0, 0, width, height};
  OffsetRect(&rect, x - width / 2, y - height / 2);
  RECT clipRect = rect;

  if (clipRect.left < 0)
    clipRect.left = 0;
  if (clipRect.top < 0)
    clipRect.top = 0;
  if (clipRect.right >= screenmanager_Get()->GetSurfWidth())
    clipRect.right = screenmanager_Get()->GetSurfWidth() - 1;
  if (clipRect.bottom >= screenmanager_Get()->GetSurfHeight())
    clipRect.bottom = screenmanager_Get()->GetSurfHeight() - 1;

  COLORREF colorRef = colorset_Get()->GetColorRef(COLOR_BLACK);

  font->DrawString(screenmanager_Get()->GetSurface(), &rect, &clipRect, fString, 0,
                   colorRef, 0);

  if (fogged) {
    colorRef = colorset_Get()->GetColorRef(COLOR_WHITE);
  } else {
    colorRef = colorset_Get()->GetDarkColorRef(COLOR_WHITE);
  }

  OffsetRect(&rect, -1, -1);
  OffsetRect(&clipRect, -1, -1);

  font->DrawString(screenmanager_Get()->GetSurface(), &rect, &clipRect, fString, 0,
                   colorRef, 0);
}

//----------------------------------------------------------------------------
//
// Name       : UnitActor::DrawCityWalls
//
// Description: Draw city walls
//
// Parameters : fogged	: city is under fog of war
//
// Globals    : tiledmap_Get()
//				g_theCityStyleDB
//				player_Get()
//				g_theTerrainDB
//				world_Get()
//
// Returns    : -
//
// Remark(s)  : Assumption: unit is a valid city with walls.
//              Does not draw force fields: use DrawForceField for that.
//
//----------------------------------------------------------------------------
void UnitActor::DrawCityWalls(
    bool fogged)  // TODO make a draw wonders and draw buildings method
{
  TileSet const* tileSet = tiledmap_Get()->GetTileSet();
  if (!tileSet)
	return;

  Pixel16* cityImage = tileSet->GetImprovementData(38);  // default
  if (!cityImage)
	return;

  Unit unit(GetUnitID());

  // Test city style overrides.
  CityStyleRecord const* styleRec =
      g_theCityStyleDB->Get(unit.CD()->GetCityStyle());

  if (styleRec) {
    PLAYER_INDEX ownerIndex = unit->GetOwner();
    if (ownerIndex < 0 || ownerIndex >= k_MAX_PLAYERS)
      return;

    Player *owner = player_Get(ownerIndex);
    if (!owner)
      return;

    AgeCityStyleRecord const* ageStyleRec =
        styleRec->GetAgeStyle(owner->m_age);

    if (ageStyleRec) {
      bool const isWater = world_Get()->IsWater(GetPos());
      sint32 const spriteCount = ageStyleRec->GetNumSprites();
      AgeCityStyleRecord::SizeSprite const* matchingSprite = nullptr;

      for (sint32 i = 0; i < spriteCount; ++i) {
        AgeCityStyleRecord::SizeSprite const* spr = ageStyleRec->GetSprites(i);

        if (spr && (isWater == (CITY_TYPE_WATER == spr->GetType()))) {
          matchingSprite = spr;

          // Check city size
          sint32 p;
          unit.CD()->GetPop(p);
          if ((spr->GetMinSize() <= p) && (spr->GetMaxSize() >= p)) {
            break;  // exact match found
          }

          // When no exact match has been found, the last (largest?)
          // of the correct type will be used.
        }
      }

      if (matchingSprite) {
        cityImage = tileSet->GetImprovementData(
            static_cast<uint16>(matchingSprite->GetWalls()));
        if (!cityImage)
          return;
      }
      // else: keep default
    }
  }
  // else: keep default

  sint32 nudgeX = (sint32)((double)((k_ACTOR_CENTER_OFFSET_X)-48) *
                           tiledmap_Get()->GetScale());
  sint32 nudgeY = (sint32)((double)((k_ACTOR_CENTER_OFFSET_Y)-48) *
                           tiledmap_Get()->GetScale());

  if (tiledmap_Get()->GetZoomLevel() == k_ZOOM_LARGEST) {
    if (fogged)
      tiledmap_Get()->DrawBlendedOverlayIntoMix(cityImage, m_x + nudgeX,
                                            m_y + nudgeY, k_FOW_COLOR,
                                            k_FOW_BLEND_VALUE);
    else
      tiledmap_Get()->DrawColorizedOverlayIntoMix(cityImage, m_x + nudgeX,
                                              m_y + nudgeY, 0x0000);
  } else {
    if (fogged)
      tiledmap_Get()->DrawBlendedOverlayScaledIntoMix(
          cityImage, m_x + nudgeX, m_y + nudgeY,
          tiledmap_Get()->GetZoomTilePixelWidth(),
          tiledmap_Get()->GetZoomTileGridHeight(), k_FOW_COLOR, k_FOW_BLEND_VALUE);
    else
      tiledmap_Get()->DrawScaledOverlayIntoMix(cityImage, m_x + nudgeX,
                                           m_y + nudgeY,
                                           tiledmap_Get()->GetZoomTilePixelWidth(),
                                           tiledmap_Get()->GetZoomTileGridHeight());
  }
}

//----------------------------------------------------------------------------
//
// Name       : UnitActor::DrawForceField
//
// Description: Draw city force field
//
// Parameters : fogged	: city is under fog of war
//
// Globals    : tiledmap_Get()
//				g_theCityStyleDB
//				player_Get()
//				g_theTerrainDB
//				world_Get()
//
// Returns    : -
//
// Remark(s)  : Assumption: unit is a valid city with a force field.
//              Does not draw walls: use DrawCityWalls for that.
//
//----------------------------------------------------------------------------
void UnitActor::DrawForceField(bool fogged) {
  sint32 const nudgeX =
      (sint32)((double)((k_ACTOR_CENTER_OFFSET_X)-48) * tiledmap_Get()->GetScale());
  sint32 const nudgeY =
      (sint32)((double)((k_ACTOR_CENTER_OFFSET_Y)-48) * tiledmap_Get()->GetScale());

  // Default sprite index (fixed number from original code)
  sint32 which;
  MapPoint const here = GetPos();
  if (world_Get()->IsLand(here)) {
    which = 154;
  } else if (world_Get()->IsWater(here)) {
    which = 155;
  } else {
    which = 156;  // space?
  }

  Unit unit(GetUnitID());

  // Test city style overrides.
  CityStyleRecord const* styleRec =
      g_theCityStyleDB->Get(unit.CD()->GetCityStyle());

  if (styleRec) {
    PLAYER_INDEX ownerIndex = unit->GetOwner();
    if (ownerIndex < 0 || ownerIndex >= k_MAX_PLAYERS)
      return;

    Player *owner = player_Get(ownerIndex);
    if (!owner)
      return;

    AgeCityStyleRecord const* ageStyleRec =
        styleRec->GetAgeStyle(owner->m_age);

    if (ageStyleRec) {
      bool const isWater = world_Get()->IsWater(GetPos());
      sint32 const spriteCount = ageStyleRec->GetNumSprites();
      AgeCityStyleRecord::SizeSprite const* matchingSprite = nullptr;

      for (sint32 i = 0; i < spriteCount; ++i) {
        AgeCityStyleRecord::SizeSprite const* spr = ageStyleRec->GetSprites(i);

        if (spr && (isWater == (CITY_TYPE_WATER == spr->GetType()))) {
          matchingSprite = spr;

          // Check city size
          sint32 p;
          unit.CD()->GetPop(p);
          if ((spr->GetMinSize() <= p) && (spr->GetMaxSize() >= p)) {
            break;  // exact match found
          }

          // When no exact match has been found, the last (largest?)
          // of the correct type will be used.
        }
      }

      if (matchingSprite) {
        which = matchingSprite->GetForceField();
      }
      // else: keep default
    }
  }
  // else: keep default

  TileSet* tileSet = tiledmap_Get()->GetTileSet();
  if (!tileSet)
	return;

  Pixel16* cityImage = tileSet->GetImprovementData((uint16)which);
  if (!cityImage)
	return;

  if (tiledmap_Get()->GetZoomLevel() == k_ZOOM_LARGEST) {
    tiledmap_Get()->DrawDitheredOverlayIntoMix(cityImage, m_x + nudgeX,
                                           m_y + nudgeY, fogged);

  } else {
    tiledmap_Get()->DrawDitheredOverlayScaledIntoMix(
        cityImage, m_x + nudgeX, m_y + nudgeY,
        tiledmap_Get()->GetZoomTilePixelWidth(),
        tiledmap_Get()->GetZoomTileGridHeight(), fogged);
  }
}

bool UnitActor::Draw(bool fogged) {
  if (m_hidden)
    return false;

  if (m_hiddenUnderStack)
    return false;

  sint32 xoffset = (sint32)(k_ACTOR_CENTER_OFFSET_X * tiledmap_Get()->GetScale());
  sint32 yoffset = (sint32)(k_ACTOR_CENTER_OFFSET_Y * tiledmap_Get()->GetScale());

  uint16 flags = k_DRAWFLAGS_NORMAL;
  if (m_transparency < 15) {
    flags |= k_BIT_DRAWFLAGS_TRANSPARENCY;
  }
  if (fogged) {
    flags |= k_BIT_DRAWFLAGS_FOGGED;
  }

  bool isCloaked = false;

  if (m_unitID.IsValid()) {
    if (m_unitID.IsAsleep()) {
      flags |= k_BIT_DRAWFLAGS_DESATURATED;
      // m_isFortified / m_isFortifying removed — fortified state in
      // gs/.  Renderer can't reset gs/ from here.  The visual effect
      // of "asleep doesn't show fortified" is handled at the draw
      // sites below by gating on !m_unitID.IsAsleep().
    }

    isCloaked = m_unitID.IsCloaked();
  }

  bool directionAttack =
      m_directionalAttack && (m_curUnitAction == UNITACTION_ATTACK);

  SELECT_TYPE selectType;
  ID selectedID;
  PLAYER_INDEX selectedPlayer;
  selitem_Get()->GetTopCurItem(selectedPlayer, selectedID, selectType);

  Unit selectedUnit;
  if (selectType == SELECT_TYPE_LOCAL_CITY) {
    selectedUnit = selectedID;
  } else if (selectType == SELECT_TYPE_LOCAL_ARMY) {
    selectedUnit = Army(selectedID).GetTopVisibleUnit(selectedPlayer);
  }

  bool drawShield = true;
  bool drawSelectionBrackets = false;

  if (selectedUnit.IsValid() && selectedUnit.GetActor().get() == this) {
    drawSelectionBrackets = true;
    if (GetTickCount() > m_shieldFlashOffTime) {
      drawShield = GetTickCount() > m_shieldFlashOnTime;
      if (drawShield) {
        m_shieldFlashOffTime = GetTickCount() + k_SHIELD_ON_TIME;
        m_shieldFlashOnTime = m_shieldFlashOffTime + k_SHIELD_OFF_TIME;
      }
    }
  }

#ifdef _ACTOR_DRAW_OPTIMIZATION

  if ((m_frame == m_oldFrame) && (m_facing == m_oldFacing) &&
      (m_x + xoffset == m_oldOffsetX) && (m_y + yoffset == m_oldOffsetY) &&
      (flags == m_oldFlags) &&
      ((m_unitID.IsValid() && m_unitID.IsEntrenched()) == m_oldIsFortified) &&
      ((m_unitID.IsValid() && m_unitID.IsEntrenching()) == m_oldIsFortifying) &&
      ((m_unitID.IsValid() && m_unitID.HasForceField()) == m_oldHasForceField) &&
      ((m_unitID.IsValid() && m_unitID.HasCityWalls()) == m_oldHasCityWalls) &&
      (drawShield == m_oldDrawShield)) {
    if (m_paintTwice > 1) {
      return (FALSE);
    }
    m_paintTwice++;
    return FALSE;
  }

  m_paintTwice = 0;

  m_oldFrame = m_frame;
  m_oldFacing = m_facing;
  m_oldOffsetX = m_x + xoffset;
  m_oldOffsetY = m_y + yoffset;

  m_oldFlags = flags;
  m_oldIsFortified   = m_unitID.IsValid() && m_unitID.IsEntrenched();
  m_oldIsFortifying  = m_unitID.IsValid() && m_unitID.IsEntrenching();
  m_oldHasForceField = m_unitID.IsValid() && m_unitID.HasForceField();
  m_oldHasCityWalls  = m_unitID.IsValid() && m_unitID.HasCityWalls();
  m_oldDrawShield = drawShield;
  m_oldDrawSelectionBrackets = drawSelectionBrackets;

#endif

  if (m_unitSpriteGroup && m_unitID.IsValid()) {
    if (m_unitID.IsValid() && m_unitID.IsCity())  // emod - 3-10-2007
      DrawCityImprovements(fogged);

    // Sleeping units don't show the fortified indicator (old code reset
    // a cached m_isFortified to FALSE in the IsAsleep branch above).
    if (m_unitID.IsValid() && m_unitID.IsEntrenched() && !m_unitID.IsAsleep())
      DrawFortified(fogged);

    if (m_unitID.IsValid() && m_unitID.IsEntrenching() && !m_unitID.IsAsleep()) {
      DrawFortifying(fogged);
    }

    if (m_unitID.IsValid() && m_unitID.HasCityWalls()) {
      DrawCityWalls(fogged);
    }

    uint16 oldTransparency = 0;
    if (isCloaked) {
      oldTransparency = m_transparency;
      m_transparency = static_cast<uint16>(8 + (rand() % 5));
      flags |= k_BIT_DRAWFLAGS_TRANSPARENCY;
    }

    Pixel16 color = 0x0000;
    if (m_curAction == nullptr) {
      m_unitSpriteGroup->Draw(m_curUnitAction, m_frame, m_x + xoffset,
                              m_y + yoffset, m_facing, tiledmap_Get()->GetScale(),
                              m_transparency, color, flags, FALSE,
                              directionAttack);
    } else {
      m_unitSpriteGroup->Draw(
          m_curUnitAction, m_frame, m_x + xoffset, m_y + yoffset, m_facing,
          tiledmap_Get()->GetScale(), m_transparency, color, flags,
          m_curAction->SpecialDelayProcess(), directionAttack);
    }

    if (isCloaked)
      m_transparency = oldTransparency;

    bool forcefieldsEverywhere = false;

    if (player_Get(m_playerNum)) {
      if (wonderutil_GetForcefieldEverywhere(
              player_Get(m_playerNum)->m_builtWonders)) {
        forcefieldsEverywhere = m_unitID.IsValid() && m_unitID.IsCity();
      }
    }

    if ((m_unitID.IsValid() && m_unitID.HasForceField()) || forcefieldsEverywhere) {
      DrawForceField(fogged);
    }

    // adding drawwonders and buildings using mapicons
    // mapicons will save tile file space and make it so modders don't have to
    // add a new tilefile all the time
    // also to cut on visible wonders slic and my visible wonder code doesn't
    // work to well bool check is in the method
    // emod
  }

  if (drawSelectionBrackets)
    DrawSelectionBrackets();

  if (drawShield)
    DrawHealthBar();

  return true;
}

void UnitActor::DrawDirect(aui_Surface* surf,
                           sint32 x,
                           sint32 y,
                           double scale) {
  if (m_unitSpriteGroup) {
    uint16 flags = k_DRAWFLAGS_NORMAL;
    Pixel16 color = 0;
    BOOL directionAttack = FALSE;
    sint32 xoffset = (sint32)(k_ACTOR_CENTER_OFFSET_X * scale);
    sint32 yoffset = (sint32)(k_ACTOR_CENTER_OFFSET_Y * scale);
    m_unitSpriteGroup->DrawDirect(surf, m_curUnitAction, m_frame, x + xoffset,
                                  y + yoffset, m_facing, scale, m_transparency,
                                  color, flags, FALSE, directionAttack);
  }
}

bool UnitActor::AddGpuSpriteQuad(sint32 x, sint32 y, double scale, bool fogged) {
  m_gpuSpriteFallbackReason = nullptr;
  auto fail = [this](char const *reason) {
    m_gpuSpriteFallbackReason = reason;
    return false;
  };

  if (m_hidden || m_hiddenUnderStack)
    return true;
  if (!m_unitSpriteGroup)
    return fail("unit-no-sprite-group");
  if (!m_curAction)
    GetNextAction();
  if (!m_curAction)
    return fail("unit-no-action");

  uint16 flags = k_DRAWFLAGS_NORMAL;
  if (m_transparency < 15)
    flags |= k_BIT_DRAWFLAGS_TRANSPARENCY;
  if (fogged)
    flags |= k_BIT_DRAWFLAGS_FOGGED;
  if (flags & ~(k_DRAWFLAGS_NORMAL | k_BIT_DRAWFLAGS_TRANSPARENCY | k_BIT_DRAWFLAGS_FOGGED))
    return fail("unit-draw-flags");
  if (m_unitID.IsValid()) {
    if (m_unitID.IsAsleep() && !(flags & (k_BIT_DRAWFLAGS_TRANSPARENCY | k_BIT_DRAWFLAGS_FOGGED)))
      return fail("unit-asleep");
    if (m_unitID.IsCloaked())
      return fail("unit-cloaked");
    if (m_unitID.IsEntrenched())
      return fail("unit-entrenched");
    if (m_unitID.IsEntrenching())
      return fail("unit-entrenching");
    if (m_unitID.HasCityWalls())
      return fail("unit-city-walls");
    if (m_unitID.HasForceField())
      return fail("unit-forcefield");
  }
  SELECT_TYPE selectType;
  ID selectedID;
  PLAYER_INDEX selectedPlayer;
  selitem_Get()->GetTopCurItem(selectedPlayer, selectedID, selectType);
  Unit selectedUnit;
  if (selectType == SELECT_TYPE_LOCAL_CITY)
    selectedUnit = selectedID;
  else if (selectType == SELECT_TYPE_LOCAL_ARMY)
    selectedUnit = Army(selectedID).GetTopVisibleUnit(selectedPlayer);
  bool const selected = selectedUnit.IsValid() && selectedUnit.GetActor().get() == this;

  Pixel16 color = 0;
  BOOL directionAttack = FALSE;
  sint32 xoffset = (sint32)(k_ACTOR_CENTER_OFFSET_X * scale);
  sint32 yoffset = (sint32)(k_ACTOR_CENTER_OFFSET_Y * scale);
  if (!m_unitSpriteGroup->AddGpuSpriteQuad(
      m_curUnitAction, m_frame, x + xoffset, y + yoffset, m_facing, scale,
      m_transparency, color, flags, FALSE, directionAttack))
    return fail("unit-atlas");
  if (selected && !AddGpuSelectionBrackets(x, y, scale, m_unitID))
    return fail("unit-selection-brackets");

  if (g_showHeralds && m_size <= 0 && (!m_unitID.IsValid() || !m_unitID.IsCity())) {
    TileSet* tileSet = tiledmap_Get()->GetTileSet();
    if (!tileSet)
      return fail("unit-no-tileset");

    sint32 stackSize = 1;
    Cell* myCell = world_Get()->GetCell(GetPos());
    if (m_tempStackSize != 0) {
      stackSize = m_tempStackSize;
    } else if (IsActive()) {
      if (m_unitID.IsValid()) {
        Army army = m_unitID.GetArmy();
        if (army.IsValid())
          stackSize = army.Num();
      }
    } else if (myCell && myCell->UnitArmy()) {
      stackSize = myCell->UnitArmy()->Num();
    }

    double ratio = 1.0;
    if (m_unitID.IsValid()) {
      if (myCell && stackSize > 1 && myCell->GetNumUnits() && myCell->UnitArmy()) {
        ratio = std::max(0.0, myCell->UnitArmy()->GetAverageHealthPercentage());
      } else if (m_healthPercent < 0) {
        sint32 totalHP = m_unitID->CalculateTotalHP();
        ratio = (totalHP > 0) ? std::max(0.0, static_cast<double>(m_unitID.GetHP()) / static_cast<double>(totalHP)) : 0.0;
      } else {
        ratio = 0.0;
      }
    } else {
      ratio = std::max(0.0, m_healthPercent);
    }

    POINT iconDim = tileSet->GetMapIconDimensions(MAPICON_HERALD);
    RECT iconRect = {0, 0, iconDim.x, iconDim.y};
    UNITACTION unitAction = m_curUnitAction;
    if (m_unitSpriteGroup->GetGroupSprite((GAME_ACTION)unitAction) == nullptr)
      unitAction = UNITACTION_IDLE;
    POINT* shieldPoint = nullptr;
    if (unitAction == UNITACTION_IDLE &&
        m_unitSpriteGroup->GetGroupSprite((GAME_ACTION)UNITACTION_IDLE) == nullptr) {
      shieldPoint = m_unitSpriteGroup->GetShieldPoints(UNITACTION_MOVE);
    } else if (m_unitSpriteGroup->GetGroupSprite((GAME_ACTION)unitAction) != nullptr) {
      shieldPoint = m_unitSpriteGroup->GetShieldPoints(unitAction);
    }
    if (shieldPoint) {
      OffsetRect(&iconRect, x + (sint32)((double)shieldPoint->x * scale),
                 y + (sint32)((double)shieldPoint->y * scale));
    } else {
      sint32 top = y;
      sint32 middle = x + (sint32)((k_TILE_PIXEL_WIDTH) * scale) / 2;
      OffsetRect(&iconRect, middle - iconDim.x / 2, top - iconDim.y);
    }

    sint32 displayedOwner;
    if (m_unitID.IsValid() && m_unitID.IsHiddenNationality() &&
        m_playerNum != selitem_Get()->GetVisiblePlayer()) {
      displayedOwner = PLAYER_INDEX_VANDALS;
    } else {
      displayedOwner = m_playerNum;
    }
    Pixel16 playerColor = colorset_Get()->GetPlayerColor(displayedOwner);

    sint32 specialIcon = 0;
    if (m_unitID.IsValid() && m_unitID.GetDBRec()->GetHasReligionIconIndex(specialIcon)) {
      if (!AddGpuMapIconQuad(tileSet, (MAPICON)specialIcon, iconRect.left, iconRect.top, playerColor))
        return fail("unit-religion-icon");
    } else if (profiledb_Get()->IsCivFlags()) {
      sint32 civ = -1;
      if (player_Get(displayedOwner) != nullptr) {
        civ = player_Get(displayedOwner)->GetCivilisation()->GetCivilisation();
      } else {
        for (PointerList<Player>::Walker walk(g_deadPlayer); walk.IsValid(); walk.Next()) {
          Player* p = walk.GetObj();
          if (p) {
            Civilisation* civP = p->GetCivilisation();
            if (civP != nullptr && civilisationpool_Get()->IsValid(*civP) && civP->GetOwner() == displayedOwner)
              civ = civP->GetCivilisation();
          }
        }
      }

      sint32 civIcon = 0;
      auto const *civRec = civ > -1 ? g_theCivilisationDB->Get(civ) : nullptr;
      if (civRec && civRec->GetNationUnitFlagIndex(civIcon)) {
        if (!AddGpuMapIconQuad(tileSet, (MAPICON)civIcon, iconRect.left, iconRect.top, playerColor))
          return fail("unit-civ-flag");
      }
    }
    iconRect.top += iconDim.y;
    iconRect.bottom += iconDim.y;

    Pixel16 black = colorset_Get()->GetColor(COLOR_BLACK);
    if (black == 0x0000)
      black = 0x0001;
    if (profiledb_Get()->GetShowEnemyHealth() || m_playerNum == selitem_Get()->GetVisiblePlayer()) {
      iconRect.bottom += 4;
      RECT healthBar = iconRect;
      if (!AddGpuSolidRect(healthBar, black))
        return fail("unit-health-bar");
      InflateRect(&healthBar, -1, -1);
      RECT leftRect = healthBar;
      RECT rightRect = healthBar;
      Pixel16 healthColor = colorset_Get()->GetColor(COLOR_GREEN);
      if (ratio < 1.0) {
        leftRect.right = leftRect.left + (sint32)(ratio * (double)(healthBar.right - iconRect.left));
        rightRect.left = leftRect.right;
        if (ratio < 0.25)
          healthColor = colorset_Get()->GetColor(COLOR_RED);
        else if (ratio < 0.50)
          healthColor = colorset_Get()->GetColor(COLOR_ORANGE);
        else if (ratio < 0.75)
          healthColor = colorset_Get()->GetColor(COLOR_YELLOW);
        if (!AddGpuSolidRect(rightRect, black))
          return fail("unit-health-bar");
      }
      if (!AddGpuSolidRect(leftRect, healthColor))
        return fail("unit-health-bar");
      iconRect.top = iconRect.bottom;
    }

    MAPICON stackIcon = MAPICON_HERALD;
    if (stackSize > 1 && stackSize <= 9)
      stackIcon = (MAPICON)((sint32)MAPICON_HERALD2 + stackSize - 2);
    else if (stackSize >= 10 && stackSize <= 12)
      stackIcon = (MAPICON)((sint32)MAPICON_HERALD10 + stackSize - 10);
    else if (stackSize > 12)
      return fail("unit-stack-size");
    if (!AddGpuMapIconQuad(tileSet, stackIcon, iconRect.left, iconRect.top, playerColor))
      return fail("unit-stack-icon");
    iconRect.top += iconDim.y;

    if (m_unitID.IsValid() && m_unitID->GetArmy().IsValid() && m_unitID->GetArmy()->Num() > 1) {
      if (!AddGpuMapIconQuad(tileSet, MAPICON_ARMY, iconRect.left, iconRect.top, playerColor))
        return fail("unit-army-icon");
      iconRect.top += tileSet->GetMapIconDimensions(MAPICON_ARMY).y;
    }

    if (m_unitID.IsValid() && m_unitID->GetArmy().IsValid()) {
      Army army = m_unitID->GetArmy();
      if (army->HasVeterans() && !army->HasElite()) {
        if (!AddGpuMapIconQuad(tileSet, MAPICON_VETERAN, iconRect.left, iconRect.top, playerColor))
          return fail("unit-veteran-icon");
        iconRect.top += tileSet->GetMapIconDimensions(MAPICON_VETERAN).y;
      } else if (army->HasElite()) {
        if (!AddGpuMapIconQuad(tileSet, MAPICON_ELITE, iconRect.left, iconRect.top, playerColor))
          return fail("unit-elite-icon");
        iconRect.top += tileSet->GetMapIconDimensions(MAPICON_ELITE).y;
      }

      if (army->HasCargo() && !(army->HasCargoOnlyStealth() && m_playerNum != selitem_Get()->GetVisiblePlayer())) {
        if (!AddGpuMapIconQuad(tileSet, MAPICON_CARGO, iconRect.left, iconRect.top, playerColor))
          return fail("unit-cargo-icon");
      }
    }

  }
  return true;
}

void UnitActor::DrawText(sint32 x, sint32 y, MBCHAR* unitText) {
#ifndef _TEST
  STOMPCHECK();
#endif
  if (m_unitSpriteGroup) {
    m_unitSpriteGroup->DrawText(x, y, unitText);
  }
}

// it doesn't look like this is used. I did create a drawstackingindicator that
// is used - E
void UnitActor::DrawHerald() {
#ifndef _TEST
  STOMPCHECK();
#endif
  if (m_hiddenUnderStack)
    return;

  if (!g_showHeralds)
    return;

  if (!m_unitSpriteGroup)
    return;

  if (!m_unitID.IsValid())
    return;

  CellUnitList army;
  world_Get()->GetArmy(m_unitID.RetPos(), army);

  MAPICON icon = MAPICON_HERALD;
  if (army.Num() > 1 && army.Num() < 10) {
    icon = (MAPICON)((sint32)MAPICON_HERALD2 + army.Num() - 2);
  } else if (army.Num() >= 10) {
    icon = (MAPICON)((sint32)MAPICON_HERALD10 + (army.Num() - 10));
  }

  Pixel16 color = colorset_Get()->GetPlayerColor(m_playerNum);
  TileSet* tileSet = tiledmap_Get()->GetTileSet();
  POINT iconDim = tileSet->GetMapIconDimensions(icon);
  RECT rect = {0, 0, iconDim.x + 1, iconDim.y + 1};

  if (m_x < 0 || m_x > screenmanager_Get()->GetSurfWidth() - rect.right)
    return;
  if (m_y < 0 || m_y > screenmanager_Get()->GetSurfHeight() - rect.bottom)
    return;

#if 0  // Unused
	POINT		*pt;
	if (m_curUnitAction == UNITACTION_IDLE && m_unitSpriteGroup->GetGroupSprite((GAME_ACTION)UNITACTION_IDLE) == NULL) {
		pt = m_unitSpriteGroup->GetShieldPoints(UNITACTION_MOVE);
	} else {
		pt = m_unitSpriteGroup->GetShieldPoints(m_curUnitAction);
	}
#endif

  OffsetRect(&rect, m_x + 0 - iconDim.x / 2, m_y + 0 - iconDim.y / 2);

  tiledmap_Get()->DrawColorizedOverlayIntoMix(tileSet->GetMapIconData(icon),
                                          rect.left, rect.top, color);
  tiledmap_Get()->AddDirtyRectToMix(rect);
}

void UnitActor::DrawHealthBar() {
#ifndef _TEST
  STOMPCHECK();
#endif

  if (m_size > 0)
    return;
  if (!g_showHeralds)
    return;

  if (m_unitID.IsValid() && m_unitID.IsCity()) {
    return;
  }

  TileSet* tileSet = tiledmap_Get()->GetTileSet();
  if (!tileSet || !m_unitSpriteGroup)
	return;

  Cell* myCell = world_Get()->GetCell(GetPos());

  sint32 stackSize = 1;
  if (m_tempStackSize != 0) {
    stackSize = m_tempStackSize;
  } else if (IsActive()) {
    if (m_unitID.IsValid()) {
      Army army = m_unitID.GetArmy();

      if (army.IsValid()) {
        stackSize = army.Num();
      }
    }
  } else {
    CellUnitList* unitList = myCell->UnitArmy();
    if (unitList) {
      stackSize = unitList->Num();
      for (sint32 i = 0; i < unitList->Num(); i++) {
        Unit top;

        Army a = Army(unitList->Access(i).GetArmy().m_id);
        if (a.IsValid()) {
          top = a->GetTopVisibleUnit(selitem_Get()->GetVisiblePlayer());
        }

        if (!top.IsValid()) {
          top.m_id = unitList->Access(i).m_id;
        }

        if (top.GetActor() && top.GetActor()->IsActive()) {
          stackSize--;
        }
      }
    }
  }

  double ratio;
  if (m_unitID.IsValid()) {
    CellUnitList* cellArmy = myCell->UnitArmy();
    if (stackSize > 1 && myCell->GetNumUnits() && cellArmy) {
      ratio = std::max(0.0, cellArmy->GetAverageHealthPercentage());
    } else {
		if (m_healthPercent < 0) {
			sint32 totalHP = m_unitID->CalculateTotalHP();
			ratio = (totalHP > 0) ? std::max(0.0, static_cast<double>(m_unitID.GetHP()) / static_cast<double>(totalHP)) : 0.0;
		} else {
        ratio = 0.0;  // m_healthPercent;
      }
    }
  } else {
    ratio = std::max(0.0, m_healthPercent);
  }

  POINT iconDim = tileSet->GetMapIconDimensions(MAPICON_HERALD);

  RECT iconRect = {0, 0, iconDim.x, iconDim.y};  //	RECT	iconRect = {0,
                                                 //0, iconDim.x, iconDim.y};
                                                 //original
  // RECT	flagRect = {0, 0, iconDim.x, iconDim.y};  //added this to be the
  // flag rect and set it at 0,0

  UNITACTION unitAction = m_curUnitAction;
  if (m_unitSpriteGroup->GetGroupSprite((GAME_ACTION)unitAction) == nullptr)
    unitAction = UNITACTION_IDLE;

  POINT* shieldPoint;
  if (unitAction == UNITACTION_IDLE &&
      m_unitSpriteGroup->GetGroupSprite((GAME_ACTION)UNITACTION_IDLE) == nullptr) {
    shieldPoint = m_unitSpriteGroup->GetShieldPoints(UNITACTION_MOVE);
    if (!shieldPoint)
      return;
    OffsetRect(
        &iconRect,
        m_x + (sint32)((double)(shieldPoint->x) * tiledmap_Get()->GetScale()),
        m_y + (sint32)((double)(shieldPoint->y) * tiledmap_Get()->GetScale()));
  } else {
    if (m_unitSpriteGroup &&
        m_unitSpriteGroup->GetGroupSprite((GAME_ACTION)unitAction) != nullptr) {
      shieldPoint = m_unitSpriteGroup->GetShieldPoints(unitAction);
      if (!shieldPoint)
        return;
      OffsetRect(
          &iconRect,
          m_x + (sint32)((double)(shieldPoint->x) * tiledmap_Get()->GetScale()),
          m_y + (sint32)((double)(shieldPoint->y) * tiledmap_Get()->GetScale()));
    } else {
      sint32 top = m_y;
      sint32 middle =
          m_x + (sint32)((k_TILE_PIXEL_WIDTH)*tiledmap_Get()->GetScale()) / 2;
      OffsetRect(&iconRect, middle - iconDim.x / 2, top - iconDim.y);
    }
  }

  if (iconRect.left < 0)
    return;
  if (iconRect.right >= screenmanager_Get()->GetSurfWidth())
    return;
  if (iconRect.top < 0)
    return;
  if (iconRect.bottom >= screenmanager_Get()->GetSurfHeight())
    return;

  // @ToDo: Cleanup this type mess
  sint32 x = iconRect.left;
  sint32 y = iconRect.top;
  DrawSpecialIndicators(x, y, stackSize);
  iconRect.left = x;
  iconRect.top = y;
  RECT tempRect = iconRect;
  InflateRect(&tempRect, 2, 2);
  tempRect.top -= 8;

  tiledmap_Get()->AddDirtyRectToMix(tempRect);

  Pixel16 black = colorset_Get()->GetColor(COLOR_BLACK);
  if (black == 0x0000)
    black = 0x0001;

  if (profiledb_Get()->GetShowEnemyHealth() ||
      m_playerNum == selitem_Get()->GetVisiblePlayer()) {
    iconRect.bottom += 4;

    tagRECT healthBar = iconRect;

    if (healthBar.left < 0)
      return;
    if (healthBar.right >= screenmanager_Get()->GetSurfWidth())
      return;
    if (healthBar.top < 0)
      return;
    if (healthBar.bottom >= screenmanager_Get()->GetSurfHeight())
      return;

    primitives_FrameRect16(screenmanager_Get()->GetSurface(), &healthBar, black);

    InflateRect(&healthBar, -1, -1);

    RECT leftRect = healthBar;
    RECT rightRect = healthBar;

    Pixel16 color = colorset_Get()->GetColor(COLOR_GREEN);

    if (ratio < 1.0) {
      leftRect.right =
          leftRect.left +
          (sint32)(ratio * (double)(healthBar.right - iconRect.left));
      rightRect.left = leftRect.right;

      if (ratio < 0.25) {
        color = colorset_Get()->GetColor(COLOR_RED);
      } else if (ratio < 0.50) {
        color = colorset_Get()->GetColor(COLOR_ORANGE);
      } else if (ratio < 0.75) {
        color = colorset_Get()->GetColor(COLOR_YELLOW);
      }

      primitives_PaintRect16(screenmanager_Get()->GetSurface(), &rightRect, black);
    }

    if (leftRect.left > leftRect.right) {
      RECT temprect = leftRect;
      leftRect.left = temprect.right;
      leftRect.right = temprect.left;
    }

    primitives_PaintRect16(screenmanager_Get()->GetSurface(), &leftRect, color);

    iconRect.top = iconRect.bottom;
  }

  // @ToDo: Cleanup this type mess
  x = iconRect.left;
  y = iconRect.top;
  DrawStackingIndicator(x, y, stackSize);
  DrawIndicators(x, y, stackSize);
  iconRect.left = x;
  iconRect.top = y;
}

// moved stacking indcators here because its actually called by the healthbar
// above.

void UnitActor::DrawStackingIndicator(sint32& x, sint32& y, sint32 stack) {
#ifndef _TEST
  STOMPCHECK();
#endif

  if (!g_showHeralds)
    return;
  if (x < 0)
    return;
  if (y < 0)
    return;

  TileSet* tileSet = tiledmap_Get()->GetTileSet();
  if (!tileSet)
	return;

  POINT iconDim = tileSet->GetMapIconDimensions(MAPICON_HERALD);
  if (x >= screenmanager_Get()->GetSurfWidth() - iconDim.x)
    return;
  if (y >= screenmanager_Get()->GetSurfHeight() - iconDim.y)
    return;

  sint32 displayedOwner;
  if (m_unitID.IsValid() && m_unitID.IsHiddenNationality() &&
      (m_playerNum !=
       selitem_Get()->GetVisiblePlayer())  // You want to spot your own units
  ) {
    // Display unit as barbarians
    displayedOwner = PLAYER_INDEX_VANDALS;
  } else {
    displayedOwner = m_playerNum;
  }
  sint32 x2 = x;
  sint32 y2 = y + iconDim.y;
  sint32 w = iconDim.x;
  sint32 h = iconDim.y;
  Pixel16 displayedColor = colorset_Get()->GetPlayerColor(displayedOwner);

// Remove the next line when the scaling and centering of the text has been
// implemented properly - or you want to test its operation. Currently, the
// generated text looks too ugly to include it in a release.
#define USE_PREDEFINED_ICONS

#if defined(USE_PREDEFINED_ICONS)
  MAPICON icon = MAPICON_HERALD;  // default: plain icon

  if (stack > 1 && stack <= 9) {
    // single digit predefined icons
    icon = (MAPICON)((sint32)MAPICON_HERALD2 + stack - 2);
  } else if (stack >= 10 && stack <= 12) {
    // double digits predefined icons
    icon = (MAPICON)((sint32)MAPICON_HERALD10 + stack - 10);
  }

  tiledmap_Get()->DrawColorizedOverlayIntoMix(tileSet->GetMapIconData(icon), x, y,
                                          displayedColor);
#else
  tiledmap_Get()->DrawColorizedOverlayIntoMix(
      tileSet->GetMapIconData(MAPICON_HERALD), x, y, displayedColor);

  // Generate text
  MBCHAR strn[80];
  snprintf(strn, sizeof(strn), "%i", stack);

  /// @todo Scale and center text
  if (stack > 1 && stack <= 9) {
    // single digit
    DrawText(x + 5, y, strn);
  } else if (stack >= 10 && stack <= 12) {
    // double digits
    DrawText(x, y, strn);
  }
#endif

  tiledmap_Get()->AddDirtyToMix(x, y, w, h);

  // @ToDo clean the code so that the following is superflous
  x = x2;
  y = y2;
}

// emod - moved these from stacking indicators to separate indicators

void UnitActor::DrawIndicators(sint32& x, sint32& y, sint32 stack) {
#ifndef _TEST
  STOMPCHECK();
#endif

  if (!g_showHeralds)
    return;
  if (x < 0)
    return;
  if (y < 0)
    return;

  TileSet* tileSet = tiledmap_Get()->GetTileSet();
  POINT iconDim = tileSet->GetMapIconDimensions(MAPICON_HERALD);
  if (x >= screenmanager_Get()->GetSurfWidth() - iconDim.x)
    return;
  if (y >= screenmanager_Get()->GetSurfHeight() - iconDim.y)
    return;

  sint32 displayedOwner;
  if (m_unitID.IsValid() && m_unitID.IsHiddenNationality() &&
      (m_playerNum !=
       selitem_Get()->GetVisiblePlayer())  // You want to spot your own units
  ) {
    // Display unit as barbarians
    displayedOwner = PLAYER_INDEX_VANDALS;
  } else {
    displayedOwner = m_playerNum;
  }

  Pixel16 displayedColor = colorset_Get()->GetPlayerColor(displayedOwner);

  sint32 x2 = x;
  sint32 y2 = y;
  sint32 w = 0;
  sint32 h = 0;

  if (m_unitID.IsValid() && m_unitID->GetArmy().IsValid()) {
    if (m_unitID->GetArmy()->Num() > 1) {
      if (y2 < screenmanager_Get()->GetSurfHeight() - iconDim.y) {
        tiledmap_Get()->DrawColorizedOverlayIntoMix(
            tileSet->GetMapIconData(MAPICON_ARMY), x2, y2, displayedColor);
        iconDim = tileSet->GetMapIconDimensions(MAPICON_ARMY);
        y2 += iconDim.y;
        h += iconDim.y;
        w = std::max<sint32>(w, iconDim.x);
      }
    }
    // Replace veteran icon with elite icon if an elite unit exists in army.
    if (m_unitID->GetArmy()->HasVeterans() &&
        !m_unitID->GetArmy()->HasElite()) {
      if (y2 < screenmanager_Get()->GetSurfHeight() - iconDim.y) {
        tiledmap_Get()->DrawColorizedOverlayIntoMix(
            tileSet->GetMapIconData(MAPICON_VETERAN), x2, y2, displayedColor);
        iconDim = tileSet->GetMapIconDimensions(MAPICON_VETERAN);
        y2 += iconDim.y;
        h += iconDim.y;
        w = std::max<sint32>(w, iconDim.x);
      }
    } else if (m_unitID->GetArmy()->HasElite()) {
      if (y2 < screenmanager_Get()->GetSurfHeight() - iconDim.y) {
        tiledmap_Get()->DrawColorizedOverlayIntoMix(
            tileSet->GetMapIconData(MAPICON_ELITE), x2, y2, displayedColor);
        iconDim = tileSet->GetMapIconDimensions(MAPICON_ELITE);
        y2 += iconDim.y;
        h += iconDim.y;
        w = std::max<sint32>(w, iconDim.x);
      }
    }

    if (m_unitID->GetArmy()->HasCargo()) {
      // Do not draw the cargo icon if enemy army is carrying only stealth.
      if (m_unitID->GetArmy()->HasCargoOnlyStealth() &&
          m_playerNum != selitem_Get()->GetVisiblePlayer()) {
      }
      // Draw it in all other cases.
      else {
        if (y2 < screenmanager_Get()->GetSurfHeight() - iconDim.y) {
          tiledmap_Get()->DrawColorizedOverlayIntoMix(
              tileSet->GetMapIconData(MAPICON_CARGO), x2, y2, displayedColor);
          iconDim = tileSet->GetMapIconDimensions(MAPICON_CARGO);
          y2 += iconDim.y;
          h += iconDim.y;
          w = std::max<sint32>(w, iconDim.x);
        }
      }
    }
  }

  tiledmap_Get()->AddDirtyToMix(x, y, w, h);

  // @ToDo clean the code so that the following is superflous
  x = x2;
  y = y2;
}

void UnitActor::DrawSpecialIndicators(
    sint32& x,
    sint32& y,
    sint32 stack)  // identifier for religious unit or national flag
{
#ifndef _TEST
  STOMPCHECK();
#endif

  if (!g_showHeralds)
    return;
  if (x < 0)
    return;
  if (y < 0)
    return;

  TileSet* tileSet = tiledmap_Get()->GetTileSet();
  POINT iconDim = tileSet->GetMapIconDimensions(MAPICON_HERALD);
  if (x >= screenmanager_Get()->GetSurfWidth() - iconDim.x)
    return;
  if (y >= screenmanager_Get()->GetSurfHeight() - iconDim.y)
    return;

  sint32 displayedOwner;
  if (m_unitID.IsValid() && m_unitID.IsHiddenNationality() &&
      (m_playerNum !=
       selitem_Get()->GetVisiblePlayer())  // You want to spot your own units
  ) {
    // Display unit as barbarians
    displayedOwner = PLAYER_INDEX_VANDALS;
  } else {
    displayedOwner = m_playerNum;
  }

  Pixel16 displayedColor = colorset_Get()->GetPlayerColor(displayedOwner);
  sint32 x2 = x;
  sint32 y2 = y + iconDim.y;
  sint32 w = iconDim.x;
  sint32 h = iconDim.y;

  // If religious unit it shows the religion icon else it shows the national
  // flag - E Aug 27 2007

  sint32 religionicon = 0;
  if (m_unitID.IsValid() &&
      m_unitID.GetDBRec()->GetHasReligionIconIndex(religionicon)) {
    sint32 xf = x;  // + iconDim.x;
    tiledmap_Get()->DrawColorizedOverlayIntoMix(
        tileSet->GetMapIconData(religionicon), xf, y, displayedColor);
  } else if (profiledb_Get()->IsCivFlags()) {
    sint32 civ = -1;
    // Add civilization flags here - moved flags here and edited the
    // heralds to put numbers on national flags emod 2-21-2007
    if (player_Get(displayedOwner) != nullptr) {
      civ = player_Get(displayedOwner)->GetCivilisation()->GetCivilisation();
    } else {
      for (PointerList<Player>::Walker walk(g_deadPlayer); walk.IsValid();
           walk.Next()) {
        Player* p = walk.GetObj();

        if (p) {
          Civilisation* civP = p->GetCivilisation();
          if (civP != nullptr && civilisationpool_Get()->IsValid(*civP)) {
            if (civP->GetOwner() == displayedOwner) {
              civ = civP->GetCivilisation();
            }
          }
        }
      }
    }

    sint32 civicon = 0;

    auto const *civRec = civ > -1 ? g_theCivilisationDB->Get(civ) : nullptr;
    if (civRec && civRec->GetNationUnitFlagIndex(civicon)) {
      sint32 xf = x;  // + iconDim.x;
      tiledmap_Get()->DrawColorizedOverlayIntoMix(tileSet->GetMapIconData(civicon),
                                              xf, y, displayedColor);
    }
  }

  tiledmap_Get()->AddDirtyToMix(x, y, w, h);

  // @ToDo clean the code so that the following is superflous
  x = x2;
  y = y2;
}

// end emod

void UnitActor::DrawSelectionBrackets() {
#ifndef _TEST
  STOMPCHECK();
#endif
  if (!m_unitSpriteGroup)
    return;

  RECT rect;
  SetRect(&rect, 0, 0, 1, 1);

  OffsetRect(&rect,
             m_x + (sint32)(k_TILE_PIXEL_WIDTH * tiledmap_Get()->GetScale()) / 2,
             m_y + (sint32)(k_TILE_GRID_HEIGHT * tiledmap_Get()->GetScale()) / 2);

  InflateRect(&rect, 25, 25);

  tiledmap_Get()->AddDirtyRectToMix(rect);

  TileSet* tileSet = tiledmap_Get()->GetTileSet();
  POINT iconDim = tileSet->GetMapIconDimensions(MAPICON_BRACKET1);

  rect.right -= (iconDim.x + 1);
  rect.bottom -= (iconDim.y + 1);

  Pixel16* topLeft = tileSet->GetMapIconData(MAPICON_BRACKET1);
  Assert(topLeft);
  if (!topLeft)
    return;
  Pixel16* topRight = tileSet->GetMapIconData(MAPICON_BRACKET2);
  Assert(topRight);
  if (!topRight)
    return;
  Pixel16* botRight = tileSet->GetMapIconData(MAPICON_BRACKET3);
  Assert(botRight);
  if (!botRight)
    return;
  Pixel16* botLeft = tileSet->GetMapIconData(MAPICON_BRACKET4);
  Assert(botLeft);
  if (!botLeft)
    return;

  COLOR color = COLOR_YELLOW;
  if (m_unitID.IsValid()) {
    if (m_unitID.GetArmy().IsValid() && m_unitID.GetArmy().CanMove()) {
      color = COLOR_GREEN;
    } else if (m_unitID.IsCity()) {
      color = COLOR_RED;
    }
  }
  Pixel16 pixelColor = colorset_Get()->GetColor(color);

  tiledmap_Get()->DrawColorizedOverlayIntoMix(topLeft, rect.left, rect.top,
                                          pixelColor);
  tiledmap_Get()->DrawColorizedOverlayIntoMix(topRight, rect.right, rect.top,
                                          pixelColor);
  tiledmap_Get()->DrawColorizedOverlayIntoMix(botRight, rect.right, rect.bottom,
                                          pixelColor);
  tiledmap_Get()->DrawColorizedOverlayIntoMix(botLeft, rect.left, rect.bottom,
                                          pixelColor);
}

bool UnitActor::IsAnimating() const {
#ifndef _TEST
  STOMPCHECK();
#endif
  return m_curAction && (m_curAction->GetActionType() != UNITACTION_IDLE);
}

uint16 UnitActor::GetWidth() const {
#ifndef _TEST
  STOMPCHECK();
#endif
  Assert(m_unitSpriteGroup != nullptr);
  if (m_unitSpriteGroup == nullptr)
    return 0;

  Sprite* theSprite =
      m_unitSpriteGroup->GetGroupSprite((GAME_ACTION)m_curUnitAction);

  if (!theSprite && (m_curUnitAction == UNITACTION_IDLE)) {
    theSprite = m_unitSpriteGroup->GetGroupSprite((GAME_ACTION)UNITACTION_MOVE);
  }

  return theSprite ? theSprite->GetWidth() : 0;
}

uint16 UnitActor::GetHeight() const {
#ifndef _TEST
  STOMPCHECK();
#endif
  Assert(m_unitSpriteGroup != nullptr);
  if (m_unitSpriteGroup == nullptr)
    return 0;

  Sprite* theSprite =
      m_unitSpriteGroup->GetGroupSprite((GAME_ACTION)m_curUnitAction);

  if (!theSprite && (m_curUnitAction == UNITACTION_IDLE)) {
    theSprite = m_unitSpriteGroup->GetGroupSprite((GAME_ACTION)UNITACTION_MOVE);
  }

  return theSprite ? theSprite->GetHeight() : 0;
}

void UnitActor::GetBoundingRect(RECT* rect) const {
#ifndef _TEST
  STOMPCHECK();
#endif
  Assert(rect != nullptr);
  if (rect == nullptr)
    return;

  if (!m_unitSpriteGroup)
    return;

  POINT hotPoint = m_unitSpriteGroup->GetHotPoint(m_curUnitAction, m_facing);
  double scale = tiledmap_Get()->GetScale();

  sint32 x = m_x;
  if (m_facing >= 5) {
    x += (sint32)(
        (double)(k_ACTOR_CENTER_OFFSET_X - (GetWidth() - hotPoint.x)) * scale);
  } else {
    x += (sint32)((double)(k_ACTOR_CENTER_OFFSET_X - hotPoint.x) * scale);
  }
  sint32 y =
      m_y + (sint32)((double)(k_ACTOR_CENTER_OFFSET_Y - hotPoint.y) * scale);

  rect->left = x;
  rect->top = y;
  rect->right = x + (sint32)((double)GetWidth() * scale);
  rect->bottom = y + (sint32)((double)GetHeight() * scale);
}

LOADTYPE UnitActor::GetLoadType() const {
  return (m_unitSpriteGroup) ? m_loadType : LOADTYPE_NONE;
}

void UnitActor::FullLoad(UNITACTION action) {
  if (!profiledb_Get()->IsUnitAnim())
    return;

  if (!m_unitSpriteGroup)
    return;
  if (m_loadType == LOADTYPE_FULL)
    return;

  SpriteGroup* group =
      g_unitSpriteGroupList->GetSprite(m_spriteID, m_unitSpriteGroup->GetType(),
                                       LOADTYPE_FULL, (GAME_ACTION)action);

  m_loadType = LOADTYPE_FULL;

  Assert(group == m_unitSpriteGroup);
}

bool UnitActor::ActionMove(ActionPtr actionObj) {
  Assert(actionObj != nullptr);

  if (actionObj == nullptr)
    return false;

  if (GetNeedsToDie())
    return false;

  // m_isFortified / m_isFortifying removed — fortified state lives in
  // gs/ (UnitData::m_flags k_UDF_IS_ENTRENCHED / k_UDF_IS_ENTRENCHING).
  // Resetting that bit from the renderer would be wrong (renderer
  // mutating game state).  If gameplay needs the unit un-entrenched
  // on move-start, the gs/ side already handles it.

  actionObj->SetActionType(UNITACTION_MOVE);
  actionObj->SetAnimPos(GetHoldingCurAnimPos(UNITACTION_MOVE));
  actionObj->SetSpecialDelayProcess(
      GetHoldingCurAnimSpecialDelayProcess(UNITACTION_MOVE));
  actionObj->SetCurrentEndCondition(ACTIONEND_PATHEND);

  if (GetLoadType() != LOADTYPE_FULL)
    FullLoad(UNITACTION_MOVE);

  std::unique_ptr<Anim> anim = CreateAnim(UNITACTION_MOVE);
  Assert(anim != nullptr);
  if (anim == nullptr)
    return false;

  actionObj->SetAnim(anim.release());
  actionObj->SetUnitsVisibility(GetUnitVisibility());
  actionObj->SetUnitVisionRange(GetUnitVisionRange());
  actionObj->SetMaxActionCounter(k_MAX_UNIT_MOVEMENT_ITERATIONS -
                                 profiledb_Get()->GetUnitSpeed());
  actionObj->SetCurActionCounter(0);

  AddAction(actionObj);

  if (GetIsTransported() == k_TRANSPORTREMOVEONLY) {
    TerminateLoopingSound(SOUNDTYPE_SFX);
  } else {
    sint32 const visiblePlayer = selitem_Get()->GetVisiblePlayer();

    if ((visiblePlayer == GetPlayerNum()) ||
        (GetUnitVisibility() & (1u << visiblePlayer))) {
      AddLoopingSound(SOUNDTYPE_SFX, actionObj->GetSoundEffect());
    }
  }

  return true;
}

bool UnitActor::ActionAttack(ActionPtr actionObj, sint32 facing) {
  Assert(actionObj != nullptr);

  if (actionObj == nullptr)
    return false;

  if (GetNeedsToDie())
    return false;

  actionObj->SetCurrentEndCondition(ACTIONEND_ANIMEND);

  if (!TryAnimation(actionObj, UNITACTION_ATTACK))
    if (!TryAnimation(actionObj, UNITACTION_IDLE))
      return false;

  actionObj->SetActionType(UNITACTION_ATTACK);
  actionObj->SetFacing(facing);
  actionObj->SetUnitsVisibility(GetUnitVisibility());
  actionObj->SetUnitVisionRange(GetUnitVisionRange());

  AddAction(actionObj);

  TerminateLoopingSound(SOUNDTYPE_SFX);

  sint32 const visiblePlayer = selitem_Get()->GetVisiblePlayer();

  if ((visiblePlayer == GetPlayerNum()) ||
      (GetUnitVisibility() & (1u << visiblePlayer)))
    AddSound(SOUNDTYPE_SFX, actionObj->GetSoundEffect());

  return true;
}

bool UnitActor::ActionSpecialAttack(ActionPtr actionObj, sint32 facing) {
  Assert(actionObj != nullptr);

  if (actionObj == nullptr)
    return false;

  if (GetNeedsToDie())
    return false;

  actionObj->SetCurrentEndCondition(ACTIONEND_ANIMEND);

  if (!TryAnimation(actionObj, UNITACTION_WORK))
    if (!TryAnimation(actionObj, UNITACTION_ATTACK))
      if (!TryAnimation(actionObj, UNITACTION_IDLE))
        return false;

  actionObj->SetActionType(UNITACTION_ATTACK);
  actionObj->SetFacing(facing);
  actionObj->SetUnitsVisibility(GetUnitVisibility());
  actionObj->SetUnitVisionRange(GetUnitVisionRange());

  AddAction(actionObj);

  TerminateLoopingSound(SOUNDTYPE_SFX);

  sint32 const visiblePlayer = selitem_Get()->GetVisiblePlayer();

  if ((visiblePlayer == GetPlayerNum()) ||
      (GetUnitVisibility() & (1u << visiblePlayer)))
    AddSound(SOUNDTYPE_SFX, actionObj->GetSoundEffect());

  return true;
}

bool UnitActor::TryAnimation(ActionPtr actionObj, UNITACTION action) {
  FullLoad(action);

  std::unique_ptr<Anim> theAnim = CreateAnim(action);
  if (theAnim) {
    actionObj->SetAnimPos(GetHoldingCurAnimPos(action));
    actionObj->SetSpecialDelayProcess(
        GetHoldingCurAnimSpecialDelayProcess(action));
    actionObj->SetAnim(theAnim.release());
    return true;
  }

  return false;
}

void UnitActor::DumpFullLoad() {
  if (!m_unitSpriteGroup)
    return;
  if (m_loadType != LOADTYPE_FULL)
    return;

  bool purged = g_unitSpriteGroupList->ReleaseSprite(m_spriteID, LOADTYPE_FULL);

  if (purged) {
    m_unitSpriteGroup = nullptr;
  } else {
    m_loadType = LOADTYPE_BASIC;
  }
}

BOOL UnitActor::HitTest(POINT mousePt) {
  bool isDirectionAttack =
      m_directionalAttack && (m_curUnitAction == UNITACTION_ATTACK);
  sint32 xoffset = (sint32)(k_ACTOR_CENTER_OFFSET_X * tiledmap_Get()->GetScale());
  sint32 yoffset = (sint32)(k_ACTOR_CENTER_OFFSET_Y * tiledmap_Get()->GetScale());
  Pixel16 color = COLOR_WHITE;
  uint16 flags = 0;
  bool isSpecialDelay = m_curAction && m_curAction->SpecialDelayProcess();

  return m_unitSpriteGroup->HitTest(
      mousePt, m_curUnitAction, m_frame, m_x + xoffset, m_y + yoffset, m_facing,
      tiledmap_Get()->GetScale(), m_transparency, color, flags, isSpecialDelay,
      isDirectionAttack);
}

void UnitActor::AddLoopingSound(uint32 sound_type, sint32 sound_id) {
  if ((soundmgr_Get()) && (sound_id >= 0))
    soundmgr_Get()->AddLoopingSound((SOUNDTYPE)sound_type, (uint32)GetUnitID(),
                                    sound_id, GetPos().x, GetPos().y);
}

void UnitActor::AddSound(uint32 sound_type, sint32 sound_id) {
  if ((soundmgr_Get()) && (sound_id >= 0))
    soundmgr_Get()->AddSound((SOUNDTYPE)sound_type, (uint32)GetUnitID(),
                             sound_id, GetPos().x, GetPos().y);
}

void UnitActor::TerminateLoopingSound(uint32 sound_type) {
  if (soundmgr_Get())
    soundmgr_Get()->TerminateLoopingSound(SOUNDTYPE_SFX, GetUnitID());
}

void UnitActor::SetRevealedActors(const UnitActorVec& revealedActors) {
  m_revealedActors = revealedActors;
}

void UnitActor::SaveRevealedActors(const UnitActorVec& revealedActors) {
  m_savedRevealedActors = revealedActors;
}

void UnitActor::SetMoveActors(const UnitActorVec& moveActors) {
  m_moveActors = moveActors;
}

//----------------------------------------------------------------------------
//
// Name       : UnitActor::DrawCityImprovements
//
// Description: Draw wonders and buildings on city graphics
//
// Parameters : fogged  : city is under fog of war
//
// Globals    : tiledmap_Get()
//              g_theCityStyleDB
//              player_Get()
//              g_theTerrainDB
//              world_Get()
//
// Returns    : -
//
// Remark(s)  : Assumption: checks for icons
//              Replaced dithered with overlay to make it work better
//              Places wonder "behind" the city and city name
//              Looks a lot nicer
//----------------------------------------------------------------------------
void UnitActor::DrawCityImprovements(bool fogged) {
  TileSet* tileSet = tiledmap_Get()->GetTileSet();

  sint32 nudgeX =
      (sint32)((double)((k_ACTOR_CENTER_OFFSET_X)-48) * tiledmap_Get()->GetScale());
  sint32 nudgeY =
      (sint32)((double)((k_ACTOR_CENTER_OFFSET_Y)-48) * tiledmap_Get()->GetScale());

  POINT iconDim = tileSet->GetMapIconDimensions(MAPICON_HERALD);
  if ((m_x + nudgeX) >= screenmanager_Get()->GetSurfWidth() - iconDim.x)
    return;
  if ((m_y + nudgeY) >= screenmanager_Get()->GetSurfHeight() - iconDim.y)
    return;

  Unit unit(m_unitID);
  sint32 cityIcon = 0;
  if (unit.IsValid() && unit.IsCity()) {
    for (sint32 b = 0; b < g_theBuildingDB->NumRecords() && b < 64; b++) {
      if (buildingutil_Get(b, m_playerNum)
              ->GetShowCityIconBottomIndex(cityIcon)) {
        if (unit.CD()->GetImprovements() & ((uint64)1 << b)) {
          if (tiledmap_Get()->GetZoomLevel() == k_ZOOM_LARGEST) {
            if (fogged)
              tiledmap_Get()->DrawBlendedOverlayIntoMix(
                  tileSet->GetMapIconData(cityIcon), m_x + nudgeX, m_y + nudgeY,
                  k_FOW_COLOR, k_FOW_BLEND_VALUE);
            else
              tiledmap_Get()->DrawColorizedOverlayIntoMix(
                  tileSet->GetMapIconData(cityIcon), m_x + nudgeX, m_y + nudgeY,
                  0x0000);
          } else {
            if (fogged)
              tiledmap_Get()->DrawBlendedOverlayScaledIntoMix(
                  tileSet->GetMapIconData(cityIcon), m_x + nudgeX, m_y + nudgeY,
                  tiledmap_Get()->GetZoomTilePixelWidth(),
                  tiledmap_Get()->GetZoomTileGridHeight(), k_FOW_COLOR,
                  k_FOW_BLEND_VALUE);
            else
              tiledmap_Get()->DrawScaledOverlayIntoMix(
                  tileSet->GetMapIconData(cityIcon), m_x + nudgeX, m_y + nudgeY,
                  tiledmap_Get()->GetZoomTilePixelWidth(),
                  tiledmap_Get()->GetZoomTileGridHeight());
          }
          nudgeX += 5;
        }
      }
    }

    for (sint32 i = 0; i < g_theWonderDB->NumRecords() && i < 64; i++) {
      if (wonderutil_Get(i, m_playerNum)
              ->GetShowCityIconBottomIndex(cityIcon)) {
        if (unit.CD()->GetBuiltWonders() & (uint64)1 << (uint64)i) {
          if (tiledmap_Get()->GetZoomLevel() == k_ZOOM_LARGEST) {
            if (fogged)
              tiledmap_Get()->DrawBlendedOverlayIntoMix(
                  tileSet->GetMapIconData(cityIcon), m_x + nudgeX, m_y + nudgeY,
                  k_FOW_COLOR, k_FOW_BLEND_VALUE);
            else
              tiledmap_Get()->DrawColorizedOverlayIntoMix(
                  tileSet->GetMapIconData(cityIcon), m_x + nudgeX, m_y + nudgeY,
                  0x0000);
          } else {
            if (fogged)
              tiledmap_Get()->DrawBlendedOverlayScaledIntoMix(
                  tileSet->GetMapIconData(cityIcon), m_x + nudgeX, m_y + nudgeY,
                  tiledmap_Get()->GetZoomTilePixelWidth(),
                  tiledmap_Get()->GetZoomTileGridHeight(), k_FOW_COLOR,
                  k_FOW_BLEND_VALUE);
            else
              tiledmap_Get()->DrawScaledOverlayIntoMix(
                  tileSet->GetMapIconData(cityIcon), m_x + nudgeX, m_y + nudgeY,
                  tiledmap_Get()->GetZoomTilePixelWidth(),
                  tiledmap_Get()->GetZoomTileGridHeight());
          }
          nudgeX += 5;
        }
      }
    }
  }
}

#ifdef _DEBUG
void UnitActor::DumpActor() {
  DPRINTF(k_DBG_UI, ("Actor %#.8lx\n", this));
  DPRINTF(k_DBG_UI, ("  m_unitID           :%#.8lx\n", m_unitID));
  DPRINTF(k_DBG_UI, ("  m_unitDBIndex      :%d\n", m_unitDBIndex));
  DPRINTF(k_DBG_UI, ("  m_curAction        :%#.8lx\n", m_curAction));

  if (m_curAction) {
    DPRINTF(k_DBG_UI, ("  m_curAction.m_actionType     :%ld\n",
                       m_curAction->m_actionType));
    DPRINTF(k_DBG_UI,
            ("  m_curAction.m_finished       :%ld\n", m_curAction->Finished()));

    if (!m_curAction->GetSequence().expired()) {
      DPRINTF(k_DBG_UI, ("Actor %#.8lx m_curAction:\n", this));
      DPRINTF(k_DBG_UI, ("  m_curAction.m_sequence->m_sequenceID     :%ld\n",
                         m_curAction->GetSequence().lock()->GetSequenceID()));
      DQItemPtr item = m_curAction->GetSequence().lock()->GetItem();
      director_Get()->DumpItem(item.get());
    }
  }
  DPRINTF(k_DBG_UI, (" ------------------\n"));

  DPRINTF(k_DBG_UI, ("  m_actionQueue         :%d\n", m_actionQueue.Size()));
  if (!m_actionQueue.Empty()) {
    unsigned i = 0;
    for (const ActionPtr& action : m_actionQueue.Container()) {
      DPRINTF(k_DBG_UI, ("  m_actionQueue Item      :%u\n", i));

      if (action) {
        DPRINTF(k_DBG_UI,
                ("  action.m_actionType     :%ld\n", action->m_actionType));
        DPRINTF(k_DBG_UI,
                ("  action.m_finished       :%ld\n", action->Finished()));
        if (!action->GetSequence().expired()) {
          DPRINTF(k_DBG_UI, ("  action.m_sequence->m_sequenceID:%ld\n",
                             action->GetSequence().lock()->GetSequenceID()));
          director_Get()->DumpItem(action->GetSequence().lock()->GetItem().get());
        }
      }
      ++i;
    }
  }
  DPRINTF(k_DBG_UI, (" ------------------\n"));
}
#endif
