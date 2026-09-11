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
// _ACTOR_DRAW_OPTIMIZATION
//
//----------------------------------------------------------------------------
//
// Modifications from the original Activision code:
//
// - Unit stacking indications and special indecations are placed according
//   their size. (9-Feb-2008 Martin G�hmann)
//
//----------------------------------------------------------------------------

#ifdef HAVE_PRAGMA_ONCE
#pragma once
#endif
#ifndef __UNITACTOR_H__
#define __UNITACTOR_H__

#include <deque>
#include <memory>

// class UnitActor + UnitActorPtr typedef live in gs/core/sprite_state_fwd.h
// so gs/ headers can use the typedef without depending on gfx/.  This
// header re-uses that declaration for ODR consistency.
#include "gs/core/sprite_state_fwd.h"
typedef std::weak_ptr<UnitActor> UnitActorWeakPtr;

#include "gfx/spritesys/Action.h"           // Action, GAME_ACTION
#include "gfx/spritesys/Actor.h"            // Actor
#include "gfx/spritesys/Anim.h"             // Anim
#include "gfx/spritesys/SpriteGroup.h"      // GROUPTYPE, LOADTYPE
#include "gfx/spritesys/UnitSpriteGroup.h"  // UNITACTION
#include "gs/gameobj/Unit.h"                // SPECATTACK, Unit
#include "gs/gameobj/UnitState.h"           // UnitState (renderer's gs/ view)
#include "gs/world/MapPoint.h"              // MapPoint
#include "os/include/ctp2_inttypes.h"       // sintN, uintN

class aui_Surface;
class SpriteState;
// BOOL, POINT, RECT


class UnitActor : public Actor {
 public:
  typedef std::vector<UnitActorWeakPtr> UnitActorVec;

  UnitActor(SpriteStatePtr ss,
            Unit id,
            sint32 type,
            const MapPoint& pos,
            sint32 owner,
            BOOL isUnseenCellActor,
            double visionRange,
            sint32 citySprite);
  ~UnitActor();

  void GetIDAndType(sint32 owner,
                    SpriteStatePtr ss,
                    Unit id,
                    sint32 unitType,
                    MapPoint const& pos,
                    sint32* spriteID,
                    GROUPTYPE* groupType,
                    sint32 citySprite = CTPRecord::INDEX_INVALID) const;

  void AddVision();
  void RemoveVision();
  void PositionActor(MapPoint const& pos);
  void Hide();
  void Show();

  void Initialize();

  void ChangeImage(SpriteStatePtr ss, sint32 type, Unit id);
  void ChangeType(SpriteStatePtr ss, sint32 type, Unit id, BOOL updateVision);

  void SetSize(sint32 size) { m_size = size; }
  sint32 GetSize() const { return m_size; }

  // Only the render-tool command exposes fixed poses; ordinary actors keep animating.
  int SetRenderPose(int action, int frame, int facing, int opacity, bool fogged);
  void Process() override;
  void DumpAllActions();
  void EndTurnProcess();
  ActionPtr WillMorph() const;
  ActionPtr WillDie() const;
  void AddAction(ActionPtr actionObj) override;
  void GetNextAction(bool isVisible = true);
  void AddIdle(bool NoIdleJustDelay = false);
  void ActionQueueUpIdle(bool NoIdleJustDelay = false);

  std::unique_ptr<Anim> CreateAnim(UNITACTION action);
  std::unique_ptr<Anim> MakeFakeDeath();
  std::unique_ptr<Anim> MakeFaceoff();

  bool HasThisAnim(UNITACTION action) const {
    return m_unitSpriteGroup && m_unitSpriteGroup->GetAnim((GAME_ACTION)action);
  }

  void DrawFortified(bool fogged);
  void DrawFortifying(bool fogged);
  void DrawCityWalls(bool fogged);
  void DrawForceField(bool fogged);
  void DrawCityImprovements(bool fogged);  // emod

  bool Draw(bool fogged = FALSE);
  void DrawHerald();
  void DrawSelectionBrackets();
  void DrawHealthBar();
  void DrawStackingIndicator(sint32& x, sint32& y, sint32 stackSize);
  void DrawIndicators(sint32& x, sint32& y, sint32 stackSize);
  void DrawSpecialIndicators(sint32& x, sint32& y, sint32 stackSize);
  void DrawText(sint32 x, sint32 y, MBCHAR* unitText);

  void DrawDirect(aui_Surface* surf, sint32 x, sint32 y, double scale);
  bool AddGpuSpriteQuad(sint32 x, sint32 y, double scale, bool fogged = false);
  char const *GpuSpriteFallbackReason() const { return m_gpuSpriteFallbackReason; }

  bool IsAnimating() const;
  // True while a movement/attack action owns pixel positioning (its path
  // interpolates m_x/m_y per frame). Paint-time repositioning must skip
  // these actors or movement freezes mid-glide.
  bool HasActivePath() const;

  // GetPos dispatches: when wired to a gs/UnitState (live unit via
  // UnitData, or fog-of-war snapshot via UnseenCell), reads go through
  // the state (which for live units defers to authoritative UnitData).
  // Fallback m_pos is the legacy path used by actors with no state
  // wired yet — being phased out.
  MapPoint GetPos() const { return m_state ? m_state->GetPos() : m_pos; }
  void SetPos(MapPoint pnt) { m_pos = pnt; }

  // Wired by gs/ (UnitData ctors for live units; UnseenCell ctor for
  // fog-of-war snapshots) right after construction.  Const pointer:
  // the renderer never mutates gs/-side state.
  void SetState(UnitState const * s) { m_state = s; }
  UnitState const * GetState() const { return m_state; }
  void GetPixelPos(sint32& x, sint32& y) const {
    x = m_x;
    y = m_y;
  }

  sint32 GetFacing() const { return m_facing; }

  uint16 GetWidth() const;
  uint16 GetHeight() const;

  uint32 GetUnitID() const { return m_unitID.m_id; }
  sint32 GetUnitDBIndex() const { return m_unitDBIndex; }

  void SetPlayerNum(sint32 playerNum) { m_playerNum = playerNum; }
  sint32 GetPlayerNum() const { return m_playerNum; }

  sint32 GetNextPop() const { return m_nextPop; }

  bool HasDeath() const { return m_unitSpriteGroup->HasDeath(); }
  bool HasDirectional() { return m_unitSpriteGroup->HasDirectional(); }

  void SetUnitVisibility(uint32 val) {
    m_unitSaveVisibility = m_unitVisibility = val;
  }
  void SetUnitVisibility(uint32 val, BOOL bval) {
    m_unitSaveVisibility = m_unitVisibility;
    m_unitVisibility = val;
    m_bVisSpecial = TRUE;
  }

  void SetUnitVisibility() { m_bVisSpecial = FALSE; }
  uint32 GetUnitVisibility() const { return m_unitVisibility; }
  uint32 GetUnitSavedVisibility() const { return m_unitSaveVisibility; }

  BOOL GetVisSpecial() const { return m_bVisSpecial; }
  void SetVisSpecial(BOOL val) { m_bVisSpecial = val; }

  double GetUnitVisionRange() const { return m_unitVisionRange; }
  void SetUnitVisionRange(double range) { m_unitVisionRange = range; }
  void SetNewUnitVisionRange(double range) { m_newUnitVisionRange = range; }

  void SetNeedsToDie(BOOL val) { m_needsToDie = val; }
  BOOL GetNeedsToDie() const { return m_needsToDie; }

  void SetNeedsToVictor(BOOL val) { m_needsToVictor = val; }
  BOOL GetNeedsToVictor() const { return m_needsToVictor; }

  void SetKillNow() { m_killNow = TRUE; }
  BOOL GetKillNow() const { return m_killNow; }

  void SetRevealedActors(const UnitActorVec& revealedActors);
  void SaveRevealedActors(const UnitActorVec& revealedActors);
  const UnitActorVec& GetRevealedActors() const { return m_revealedActors; }

  void SetMoveActors(const UnitActorVec& moveActors);
  const UnitActorVec& GetMoveActors() const { return m_moveActors; }

  BOOL HiddenUnderStack() const { return m_hiddenUnderStack; }
  void SetHiddenUnderStack(BOOL val) { m_hiddenUnderStack = val; }

  void SetIsTransported(sint32 val) { m_isTransported = val; }
  sint32 GetIsTransported() const { return m_isTransported; }

  void GetBoundingRect(RECT* rect) const;

  sint32 GetHoldingCurAnimPos(UNITACTION action) const {
    if (action < 0 || action >= UNITACTION_MAX) return 0;
    return m_holdingCurAnimPos[action];
  }
  sint32 GetHoldingCurAnimDelayEnd(UNITACTION action) const {
    if (action < 0 || action >= UNITACTION_MAX) return 0;
    return m_holdingCurAnimDelayEnd[action];
  }
  sint32 GetHoldingCurAnimElapsed(UNITACTION action) const {
    if (action < 0 || action >= UNITACTION_MAX) return 0;
    return m_holdingCurAnimElapsed[action];
  }
  sint32 GetHoldingCurAnimLastFrameTime(UNITACTION action) const {
    if (action < 0 || action >= UNITACTION_MAX) return 0;
    return m_holdingCurAnimLastFrameTime[action];
  }
  sint32 GetHoldingCurAnimSpecialDelayProcess(UNITACTION action) const {
    (void)action;
    return m_holdingCurAnimSpecialDelayProcess;
  }

  LOADTYPE GetLoadType() const;

  void FullLoad(UNITACTION action);
  void DumpFullLoad();

  // m_isFortified / m_isFortifying / m_hasCityWalls / m_hasForceField
  // removed (UnitActor split Phase 3 slices 1-2).  Renderer reads
  // m_unitID.IsEntrenched() / IsEntrenching() / HasCityWalls() /
  // HasForceField() directly from gs/ each frame.

  void SetHealthPercent(double p) { m_healthPercent = p; }
  double GetHealthPercent() const { return m_healthPercent; }

  void SetTempStackSize(sint32 i) { m_tempStackSize = i; }
  sint32 GetTempStackSize() const { return m_tempStackSize; }

  BOOL HitTest(POINT mousePt);

  void AddActiveListRef() { m_activeListRef++; }
  sint32 ReleaseActiveListRef() { return --m_activeListRef; }
  sint32 GetActiveListRef() const { return m_activeListRef; }

#ifdef _DEBUG
  void DumpActor();
#endif
  sint32 m_refCount;

  bool ActionMove(ActionPtr actionObj) override;
  bool ActionAttack(ActionPtr actionObj, sint32 facing);
  bool ActionSpecialAttack(ActionPtr actionObj, sint32 facing);
  bool TryAnimation(ActionPtr actionObj, UNITACTION action);

  void TerminateLoopingSound(uint32 sound_type);
  void AddSound(uint32 sound_type, sint32 sound_id);
  void AddLoopingSound(uint32 sound_type, sint32 sound_id);

  void HackSetSpriteID(sint32 spriteID) { m_spriteID = spriteID; }

 protected:
  // Non-owning pointer into a gs/ UnitState.  Wired by UnitData (LIVE)
  // or UnseenCell (SNAPSHOT) right after this actor's construction.
  // Null means "no state wired yet — fall back to m_pos cache."
  UnitState const * m_state = nullptr;

  MapPoint m_pos;
  Unit m_unitID;
  sint32 m_unitDBIndex;
  sint32 m_playerNum;

  sint32 m_nextPop;  // PFT 29 mar 05, show # turns until city next grows a pop

  UnitSpriteGroup* m_unitSpriteGroup;
  LOADTYPE m_loadType;

  sint32 m_facing;
  sint32 m_lastMoveFacing;
  sint32 m_frame;
  bool m_renderPose = false;
  bool m_renderFogged = false;
  uint16 m_transparency;
  char const *m_gpuSpriteFallbackReason = nullptr;

  UNITACTION m_curUnitAction;

  RECT m_heraldRect;

  uint32 m_unitVisibility;
  uint32 m_unitSaveVisibility;

  BOOL m_directionalAttack;
  BOOL m_needsToDie;
  BOOL m_needsToVictor;
  BOOL m_killNow;
  double m_unitVisionRange;
  double m_newUnitVisionRange;

  UnitActorVec m_revealedActors;
  UnitActorVec m_savedRevealedActors;

  BOOL m_bVisSpecial;

  UnitActorVec m_moveActors;
  sint32 m_numOActors;
  BOOL m_hidden;
  BOOL m_hiddenUnderStack;
  sint32 m_isTransported;

  sint32 m_holdingCurAnimPos[UNITACTION_MAX];
  sint32 m_holdingCurAnimDelayEnd[UNITACTION_MAX];
  sint32 m_holdingCurAnimElapsed[UNITACTION_MAX];
  sint32 m_holdingCurAnimLastFrameTime[UNITACTION_MAX];
  sint32 m_holdingCurAnimSpecialDelayProcess;

  sint32 m_size;
  BOOL m_isUnseenCellActor;

  GROUPTYPE m_type;
  sint32 m_spriteID;

  // m_isFortified / m_isFortifying / m_hasCityWalls / m_hasForceField
  // removed (UnitActor split Phase 3 slices 1-2).

  uint32 m_shieldFlashOnTime;
  uint32 m_shieldFlashOffTime;

  sint32 m_activeListRef;
  double m_healthPercent;
  sint32 m_tempStackSize;

#ifdef _ACTOR_DRAW_OPTIMIZATION

  sint32 m_oldFacing;
  BOOL m_oldIsFortified;
  BOOL m_oldIsFortifying;
  BOOL m_oldHasCityWalls;
  BOOL m_oldHasForceField;
  BOOL m_oldDrawShield;
  BOOL m_oldDrawSelectionBrackets;
  uint16 m_oldFlags;

#endif
};

#endif
