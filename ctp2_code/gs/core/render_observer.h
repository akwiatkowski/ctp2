//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ header
// Description  : Engine-side notifications to the rendering layer
//
//----------------------------------------------------------------------------
//
// Game-state code (`gs/`) and AI code (`ai/`) historically called
// `g_director->X()` directly to schedule animations, camera moves, sprite
// lifecycle events, and similar rendering work.  The Director lives in
// `gfx/spritesys/` — that is the wrong direction in the layered
// architecture, since gs/ should not depend on gfx/.
//
// This header exposes the same surface as a set of free functions that
// fan out to a registered `Impl` callback.  The UI build registers a
// `DirectorRenderObserver` (in `gfx/spritesys/director_render_observer.cpp`)
// that forwards to `g_director`; the headless build leaves the observer
// unregistered and every call becomes a no-op.
//
// Migration pattern at the call site:
//   before:  if (g_director) g_director->AddMove(...);
//   after:   render_observer::AddMove(...);   // internal null-check
//
// The free functions in this namespace MUST stay in sync with `Impl`.
// When adding a Director method, declare it both as a virtual on Impl and
// as a free function; the DirectorRenderObserver adapter and any test
// stubs then need to grow to cover it.
//
//----------------------------------------------------------------------------

#pragma once

#include "ctp2_inttypes.h"
#include <memory>                    // std::shared_ptr, std::weak_ptr
#include <vector>

// Forward declarations only.  Value-type parameters are fine with forward
// decls in function DECLARATIONS — the .cpp pulls in the complete types it
// needs to forward calls to the registered Impl.
class Unit;                          // gs/gameobj/Unit.h
class MapPoint;                      // gs/world/MapPoint.h
class TradeRoute;                    // gs/gameobj/TradeRoute.h
class UnitActor;                     // gfx/spritesys/UnitActor.h
class EffectActor;                   // gfx/spritesys/EffectActor.h
class SpriteState;                   // gfx/spritesys/SpriteState.h
typedef std::shared_ptr<SpriteState> SpriteStatePtr;  // must match gfx/spritesys/SpriteState.h:38

namespace render_observer {

// `UnitActorVec` mirrors the typedef inside `Director` so call sites do
// not need to include director.h.  Must stay byte-compatible with
// `Director::UnitActorVec`.
using UnitActorVec = std::vector<std::weak_ptr<UnitActor> >;

// --- The Impl interface ---
// 30 virtuals, one per call site found in the scout (gs/ + ai/).  All are
// pure — concrete implementations live in:
//   - gfx/spritesys/director_render_observer.cpp (UI build, forwards to g_director)
//   - test fixtures (record-and-replay spies, no-op stubs)
// Headless does not register an Impl; the free functions below short-circuit.
class Impl
{
public:
    virtual ~Impl() = default;

    // Lifecycle
    virtual void AddShow(Unit hider)                                            = 0;
    virtual void AddHide(Unit hider)                                            = 0;
    virtual void AddDeath(Unit dead)                                            = 0;
    virtual void AddFastKill(Unit dead)                                         = 0;
    virtual void FastKill(Unit unit)                                            = 0;
    virtual void FastKillEffect(EffectActor *actor)                             = 0;
    // Phase 3 slice 7b/7c: takes Unit identity (not UnitActorPtr) so gs/
    // does not need to hold the actor.  The UI Impl looks up the actor
    // via g_uiUnitActorRegistry; headless / no-Impl makes this a no-op.
    virtual void AddSetOwner(Unit unit, sint32 owner)                           = 0;
    virtual void AddSetVisibility(Unit unit, uint32 visibility)                 = 0;
    virtual void AddSetVisionRange(Unit unit, double range)                     = 0;
    virtual void AddMorphUnit(SpriteStatePtr ss, sint32 type, Unit id)          = 0;

    // Synchronous re-skin — called when a unit/city advances in age.
    // Not interchangeable with AddMorphUnit (which is async via the
    // Director queue).  Implementation calls UnitActor::ChangeImage()
    // immediately and dumps pending actions.
    virtual void ChangeUnitImage(SpriteStatePtr ss, sint32 type, Unit id)       = 0;
    // Phase 3 slice 7f: ChangeImage's bigger sibling.  In addition to the
    // sprite swap, ChangeType updates vision range (when updateVision is
    // true).  Forwards to UnitActor::ChangeType.
    virtual void ChangeUnitType(SpriteStatePtr ss, sint32 type, Unit id,
                                bool updateVision)                              = 0;
    // Phase 3 slice 7f: forces the actor's sprite-id field to a specific
    // value, bypassing the usual lookup.  Used at game-init time when
    // restoring a saved sprite choice (e.g. capital indicator on a city).
    // The "Hack" name comes from UnitActor::HackSetSpriteID itself.
    virtual void HackSetSpriteID(Unit unit, sint32 spriteID)                    = 0;
    virtual void ActiveUnitRemove(std::shared_ptr<UnitActor> unitActor)         = 0;
    virtual void TradeActorCreate(TradeRoute newRoute)                          = 0;
    virtual void TradeActorDestroy(TradeRoute routeToDestroy)                   = 0;

    // Animation
    virtual void AddMove(Unit mover,
                         MapPoint const &oldPos, MapPoint const &newPos,
                         const UnitActorVec &revealedActors,
                         const UnitActorVec &restOfStack,
                         bool isTransported, sint32 soundID)                    = 0;
    virtual void AddTeleport(Unit top,
                             MapPoint const &oldPos, MapPoint const &newPos,
                             const UnitActorVec &revealedActors,
                             const UnitActorVec &moveActors)                    = 0;
    virtual void AddAttack(Unit attacker, Unit attacked)                        = 0;
    virtual void AddAttackPos(Unit attacker, MapPoint const &pos)               = 0;
    // Phase 3 slice 7e: synchronous snap of the rendered actor to a map
    // tile.  Bypasses the Director animation queue — that's the existing
    // semantic of UnitActor::PositionActor and we preserve it.  The UI
    // Impl looks up the actor via g_uiUnitActorRegistry; if absent the
    // call is a no-op.  Used for fog-of-war reveals, debark snaps, etc.
    virtual void PositionActor(Unit unit, MapPoint const &pos)                  = 0;
    virtual void AddSpecialAttack(Unit attacker, Unit attacked,
                                  sint32 attack)                                = 0;  // SPECATTACK
    virtual void AddSpecialEffect(MapPoint &pos,
                                  sint32 spriteID, sint32 soundID)              = 0;
    virtual void AddTerminateFaceoff(Unit &faceoffer)                           = 0;

    // Camera / visibility query
    virtual void AddCenterMap(const MapPoint &pos)                              = 0;
    virtual bool TileWillBeCompletelyVisible(sint32 x, sint32 y)                = 0;

    // Turn flow and misc
    virtual void NextPlayer(sint32 forcedUpdate /* BOOL */)                     = 0;
    virtual void AddCopyVision()                                                = 0;
    virtual void AddEndTurn()                                                   = 0;
    virtual void CatchUp()                                                      = 0;
    virtual void AddBeginScheduler(sint32 player)                               = 0;
    virtual void AddPlaySound(sint32 soundID, MapPoint const &pos)              = 0;
    virtual void AddPlayWonderMovie(sint32 which)                               = 0;
    virtual void IncrementPendingGameActions()                                  = 0;
    virtual void DecrementPendingGameActions()                                  = 0;
};

// --- Registration ---
// UI build calls Register() once after constructing its DirectorRenderObserver;
// the engine takes a non-owning pointer.  Caller keeps ownership.  Passing
// NULL unregisters (used in shutdown / test teardown).
void Register(Impl *impl);
Impl *Get();   // returns NULL in headless or pre-registration

// --- Free-function fan-outs ---
// Each function below null-checks Get() and forwards.  Non-void functions
// return a safe default when no Impl is registered (documented inline).

// Lifecycle
void AddShow(Unit hider);
void AddHide(Unit hider);
void AddDeath(Unit dead);
void AddFastKill(Unit dead);
void FastKill(Unit unit);
void FastKillEffect(EffectActor *actor);
void AddSetOwner(Unit unit, sint32 owner);
void AddSetVisibility(Unit unit, uint32 visibility);
void AddSetVisionRange(Unit unit, double range);
void AddMorphUnit(SpriteStatePtr ss, sint32 type, Unit id);
void ChangeUnitImage(SpriteStatePtr ss, sint32 type, Unit id);
void ChangeUnitType(SpriteStatePtr ss, sint32 type, Unit id, bool updateVision);
void HackSetSpriteID(Unit unit, sint32 spriteID);
void ActiveUnitRemove(std::shared_ptr<UnitActor> unitActor);
void TradeActorCreate(TradeRoute newRoute);
void TradeActorDestroy(TradeRoute routeToDestroy);

// Animation
void AddMove(Unit mover,
             MapPoint const &oldPos, MapPoint const &newPos,
             const UnitActorVec &revealedActors,
             const UnitActorVec &restOfStack,
             bool isTransported, sint32 soundID);
void AddTeleport(Unit top,
                 MapPoint const &oldPos, MapPoint const &newPos,
                 const UnitActorVec &revealedActors,
                 const UnitActorVec &moveActors);
void AddAttack(Unit attacker, Unit attacked);
void AddAttackPos(Unit attacker, MapPoint const &pos);
void PositionActor(Unit unit, MapPoint const &pos);
void AddSpecialAttack(Unit attacker, Unit attacked, sint32 attack);
void AddSpecialEffect(MapPoint &pos, sint32 spriteID, sint32 soundID);
void AddTerminateFaceoff(Unit &faceoffer);

// Camera
void AddCenterMap(const MapPoint &pos);
// Returns false in headless / when no Impl is registered.
bool TileWillBeCompletelyVisible(sint32 x, sint32 y);

// Turn flow
void NextPlayer(sint32 forcedUpdate = 0 /* BOOL FALSE */);
void AddCopyVision();
void AddEndTurn();
void CatchUp();
void AddBeginScheduler(sint32 player);
void AddPlaySound(sint32 soundID, MapPoint const &pos);
void AddPlayWonderMovie(sint32 which);
void IncrementPendingGameActions();
void DecrementPendingGameActions();

} // namespace render_observer
