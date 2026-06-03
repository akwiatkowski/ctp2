// gfx/spritesys/director_render_observer.cpp
// See director_render_observer.h.  Every method forwards 1:1 to director_Get().
// The null-check that previously littered gs/ call sites lives inside
// render_observer's free functions; here we assume director_Get() is alive
// (the adapter is only registered after Director construction).

#include "ctp/c3.h"
#include "gfx/spritesys/director_render_observer.h"
#include "gfx/spritesys/director.h"
#include "gfx/spritesys/UnitActor.h"        // UnitActor::ChangeImage
#include "gs/gameobj/UnitTypes.h"          // SPECATTACK enum cast back
#include "ui/aui_ctp2/ui_unit_actor_registry.h"  // uiunitactorregistry_Get()

void DirectorRenderObserver::AddShow(Unit hider)                                       { director_Get()->AddShow(hider); }
void DirectorRenderObserver::AddHide(Unit hider)                                       { director_Get()->AddHide(hider); }
void DirectorRenderObserver::AddDeath(Unit dead)                                       { director_Get()->AddDeath(dead); }
void DirectorRenderObserver::AddFastKill(Unit dead)                                    { director_Get()->AddFastKill(dead); }
void DirectorRenderObserver::FastKill(Unit unit)
{
    if (auto actor = uiunitactorregistry_Get().Get(unit)) {
        director_Get()->FastKill(actor);
    }
}
void DirectorRenderObserver::FastKillEffect(EffectActor *actor)                        { director_Get()->FastKill(actor); }
// Phase 3 slice 7b/7c: these Impl overrides take Unit identity (not
// UnitActorPtr) and look the actor up in uiunitactorregistry_Get().  When
// the unit is unknown (pre-spawn-event or already destroyed) the call
// is silently skipped — the next render-state refresh on this unit
// will pick up the missed update.
void DirectorRenderObserver::AddSetOwner(Unit unit, sint32 o)
{
    if (auto actor = uiunitactorregistry_Get().Get(unit)) {
        director_Get()->AddSetOwner(actor, o);
    }
}
void DirectorRenderObserver::AddSetVisibility(Unit unit, uint32 v)
{
    if (auto actor = uiunitactorregistry_Get().Get(unit)) {
        director_Get()->AddSetVisibility(actor, v);
    }
}
void DirectorRenderObserver::AddSetVisionRange(Unit unit, double r)
{
    if (auto actor = uiunitactorregistry_Get().Get(unit)) {
        director_Get()->AddSetVisionRange(actor, r);
    }
}
void DirectorRenderObserver::AddMorphUnit(SpriteStatePtr ss, sint32 type, Unit id)
{
    if (auto actor = uiunitactorregistry_Get().Get(id)) {
        director_Get()->AddMorphUnit(actor, ss, type, id);
    }
}
void DirectorRenderObserver::ChangeUnitImage(SpriteStatePtr ss, sint32 type, Unit id)
{
    if (auto actor = uiunitactorregistry_Get().Get(id)) {
        actor->ChangeImage(ss, type, id);
    }
}
void DirectorRenderObserver::ChangeUnitType(SpriteStatePtr ss, sint32 type, Unit id,
                                            bool updateVision)
{
    if (auto actor = uiunitactorregistry_Get().Get(id)) {
        actor->ChangeType(ss, type, id, updateVision ? TRUE : FALSE);
    }
}
void DirectorRenderObserver::HackSetSpriteID(Unit unit, sint32 spriteID)
{
    if (auto actor = uiunitactorregistry_Get().Get(unit)) {
        actor->HackSetSpriteID(spriteID);
    }
}
void DirectorRenderObserver::ActiveUnitRemove(std::shared_ptr<UnitActor> u)            { director_Get()->ActiveUnitRemove(u); }
void DirectorRenderObserver::TradeActorCreate(TradeRoute newRoute)                     { director_Get()->TradeActorCreate(newRoute); }
void DirectorRenderObserver::TradeActorDestroy(TradeRoute r)                           { director_Get()->TradeActorDestroy(r); }

void DirectorRenderObserver::AddMove(Unit mover,
                                     MapPoint const &oldPos, MapPoint const &newPos,
                                     const render_observer::UnitActorVec &revealedActors,
                                     const render_observer::UnitActorVec &restOfStack,
                                     bool isTransported, sint32 soundID)
{
    director_Get()->AddMove(mover, oldPos, newPos, revealedActors, restOfStack, isTransported, soundID);
}
void DirectorRenderObserver::AddTeleport(Unit top,
                                         MapPoint const &oldPos, MapPoint const &newPos,
                                         const render_observer::UnitActorVec &revealedActors,
                                         const render_observer::UnitActorVec &moveActors)
{
    director_Get()->AddTeleport(top, oldPos, newPos, revealedActors, moveActors);
}
void DirectorRenderObserver::AddAttack(Unit attacker, Unit attacked)                   { director_Get()->AddAttack(attacker, attacked); }
void DirectorRenderObserver::AddAttackPos(Unit attacker, MapPoint const &pos)          { director_Get()->AddAttackPos(attacker, pos); }
void DirectorRenderObserver::AddSpecialAttack(Unit attacker, Unit attacked, sint32 a)  { director_Get()->AddSpecialAttack(attacker, attacked, (SPECATTACK)a); }
void DirectorRenderObserver::PositionActor(Unit unit, MapPoint const &pos)
{
    if (auto actor = uiunitactorregistry_Get().Get(unit)) {
        actor->PositionActor(pos);
    }
}
void DirectorRenderObserver::AddSpecialEffect(MapPoint &pos, sint32 spriteID, sint32 soundID){ director_Get()->AddSpecialEffect(pos, spriteID, soundID); }
void DirectorRenderObserver::AddTerminateFaceoff(Unit &faceoffer)                      { director_Get()->AddTerminateFaceoff(faceoffer); }

void DirectorRenderObserver::AddCenterMap(const MapPoint &pos)                         { director_Get()->AddCenterMap(pos); }
bool DirectorRenderObserver::TileWillBeCompletelyVisible(sint32 x, sint32 y)           { return director_Get()->TileWillBeCompletelyVisible(x, y); }

void DirectorRenderObserver::NextPlayer(sint32 forcedUpdate)                           { director_Get()->NextPlayer(forcedUpdate ? TRUE : FALSE); }
void DirectorRenderObserver::AddCopyVision()                                           { director_Get()->AddCopyVision(); }
void DirectorRenderObserver::AddEndTurn()                                              { director_Get()->AddEndTurn(); }
void DirectorRenderObserver::CatchUp()                                                 { director_Get()->CatchUp(); }
void DirectorRenderObserver::AddBeginScheduler(sint32 player)                          { director_Get()->AddBeginScheduler(player); }
void DirectorRenderObserver::AddPlaySound(sint32 soundID, MapPoint const &pos)         { director_Get()->AddPlaySound(soundID, pos); }
void DirectorRenderObserver::AddPlayWonderMovie(sint32 which)                          { director_Get()->AddPlayWonderMovie(which); }
void DirectorRenderObserver::IncrementPendingGameActions()                             { director_Get()->IncrementPendingGameActions(); }
void DirectorRenderObserver::DecrementPendingGameActions()                             { director_Get()->DecrementPendingGameActions(); }

#include "gfx/spritesys/directorevent.h"
void DirectorRenderObserver::OnEventsInitialize()                                      { directorevent_Initialize(); }
void DirectorRenderObserver::OnEventsCleanup()                                         { directorevent_Cleanup(); }

namespace {
    DirectorRenderObserver s_directorRenderObserver;
}

void RegisterDirectorRenderObserver()
{
    render_observer::Register(&s_directorRenderObserver);
}
