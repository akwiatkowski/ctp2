// gs/core/render_observer.cpp
// See render_observer.h for the rationale.  This file is a thin dispatch
// layer: it holds the registered Impl pointer and forwards every free
// function to it (or no-ops if unregistered).

#include "ctp/c3.h"
#include "gs/core/render_observer.h"
#include "gs/gameobj/TradeRoute.h"   // complete type needed for value-parameter forwarding

namespace render_observer {

namespace {
    Impl *s_impl = nullptr;
}

void Register(Impl *impl) { s_impl = impl; }
Impl *Get()                { return s_impl; }

// Macro to keep the dispatch body trivial.  Each fan-out is just
// "if registered, forward; else (void)".
#define DISPATCH_VOID(method, ...) \
    do { if (s_impl) s_impl->method(__VA_ARGS__); } while (0)

// Lifecycle
void AddShow(Unit hider)                                           { DISPATCH_VOID(AddShow, hider); }
void AddHide(Unit hider)                                           { DISPATCH_VOID(AddHide, hider); }
void AddDeath(Unit dead)                                           { DISPATCH_VOID(AddDeath, dead); }
void AddFastKill(Unit dead)                                        { DISPATCH_VOID(AddFastKill, dead); }
void FastKill(std::shared_ptr<UnitActor> actor)                    { DISPATCH_VOID(FastKill, actor); }
void FastKillEffect(EffectActor *actor)                            { DISPATCH_VOID(FastKillEffect, actor); }
void AddSetOwner(std::shared_ptr<UnitActor> actor, sint32 owner)   { DISPATCH_VOID(AddSetOwner, actor, owner); }
void AddSetVisibility(Unit unit, uint32 v)                         { DISPATCH_VOID(AddSetVisibility, unit, v); }
void AddSetVisionRange(std::shared_ptr<UnitActor> actor, double r) { DISPATCH_VOID(AddSetVisionRange, actor, r); }
void AddMorphUnit(std::shared_ptr<UnitActor> morphingActor,
                  SpriteStatePtr ss, sint32 type, Unit id)         { DISPATCH_VOID(AddMorphUnit, morphingActor, ss, type, id); }
void ChangeUnitImage(std::shared_ptr<UnitActor> actor,
                     SpriteStatePtr ss, sint32 type, Unit id)      { DISPATCH_VOID(ChangeUnitImage, actor, ss, type, id); }
void ActiveUnitRemove(std::shared_ptr<UnitActor> unitActor)        { DISPATCH_VOID(ActiveUnitRemove, unitActor); }
void TradeActorCreate(TradeRoute newRoute)                         { DISPATCH_VOID(TradeActorCreate, newRoute); }
void TradeActorDestroy(TradeRoute routeToDestroy)                  { DISPATCH_VOID(TradeActorDestroy, routeToDestroy); }

// Animation
void AddMove(Unit mover,
             MapPoint const &oldPos, MapPoint const &newPos,
             const UnitActorVec &revealedActors,
             const UnitActorVec &restOfStack,
             bool isTransported, sint32 soundID)
{
    DISPATCH_VOID(AddMove, mover, oldPos, newPos, revealedActors, restOfStack, isTransported, soundID);
}
void AddTeleport(Unit top,
                 MapPoint const &oldPos, MapPoint const &newPos,
                 const UnitActorVec &revealedActors,
                 const UnitActorVec &moveActors)
{
    DISPATCH_VOID(AddTeleport, top, oldPos, newPos, revealedActors, moveActors);
}
void AddAttack(Unit attacker, Unit attacked)                                  { DISPATCH_VOID(AddAttack, attacker, attacked); }
void AddAttackPos(Unit attacker, MapPoint const &pos)                         { DISPATCH_VOID(AddAttackPos, attacker, pos); }
void AddSpecialAttack(Unit attacker, Unit attacked, sint32 attack)            { DISPATCH_VOID(AddSpecialAttack, attacker, attacked, attack); }
void AddSpecialEffect(MapPoint &pos, sint32 spriteID, sint32 soundID)         { DISPATCH_VOID(AddSpecialEffect, pos, spriteID, soundID); }
void AddTerminateFaceoff(Unit &faceoffer)                                     { DISPATCH_VOID(AddTerminateFaceoff, faceoffer); }

// Camera
void AddCenterMap(const MapPoint &pos)                                        { DISPATCH_VOID(AddCenterMap, pos); }
bool TileWillBeCompletelyVisible(sint32 x, sint32 y)
{
    return s_impl ? s_impl->TileWillBeCompletelyVisible(x, y) : false;
}

// Turn flow
void NextPlayer(sint32 forcedUpdate)                                          { DISPATCH_VOID(NextPlayer, forcedUpdate); }
void AddCopyVision()                                                          { DISPATCH_VOID(AddCopyVision); }
void AddEndTurn()                                                             { DISPATCH_VOID(AddEndTurn); }
void CatchUp()                                                                { DISPATCH_VOID(CatchUp); }
void AddBeginScheduler(sint32 player)                                         { DISPATCH_VOID(AddBeginScheduler, player); }
void AddPlaySound(sint32 soundID, MapPoint const &pos)                        { DISPATCH_VOID(AddPlaySound, soundID, pos); }
void AddPlayWonderMovie(sint32 which)                                         { DISPATCH_VOID(AddPlayWonderMovie, which); }
void IncrementPendingGameActions()                                            { DISPATCH_VOID(IncrementPendingGameActions); }
void DecrementPendingGameActions()                                            { DISPATCH_VOID(DecrementPendingGameActions); }

#undef DISPATCH_VOID

} // namespace render_observer
