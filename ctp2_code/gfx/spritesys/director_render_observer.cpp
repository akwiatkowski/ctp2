// gfx/spritesys/director_render_observer.cpp
// See director_render_observer.h.  Every method forwards 1:1 to g_director.
// The null-check that previously littered gs/ call sites lives inside
// render_observer's free functions; here we assume g_director is alive
// (the adapter is only registered after Director construction).

#include "ctp/c3.h"
#include "gfx/spritesys/director_render_observer.h"
#include "gfx/spritesys/director.h"
#include "gfx/spritesys/UnitActor.h"        // UnitActor::ChangeImage
#include "gs/gameobj/UnitTypes.h"          // SPECATTACK enum cast back
#include "ui/aui_ctp2/ui_unit_actor_registry.h"  // uiunitactorregistry_Get()

extern Director *g_director;

void DirectorRenderObserver::AddShow(Unit hider)                                       { g_director->AddShow(hider); }
void DirectorRenderObserver::AddHide(Unit hider)                                       { g_director->AddHide(hider); }
void DirectorRenderObserver::AddDeath(Unit dead)                                       { g_director->AddDeath(dead); }
void DirectorRenderObserver::AddFastKill(Unit dead)                                    { g_director->AddFastKill(dead); }
void DirectorRenderObserver::FastKill(Unit unit)
{
    if (auto actor = uiunitactorregistry_Get().Get(unit)) {
        g_director->FastKill(actor);
    }
}
void DirectorRenderObserver::FastKillEffect(EffectActor *actor)                        { g_director->FastKill(actor); }
// Phase 3 slice 7b/7c: these Impl overrides take Unit identity (not
// UnitActorPtr) and look the actor up in uiunitactorregistry_Get().  When
// the unit is unknown (pre-spawn-event or already destroyed) the call
// is silently skipped — the next render-state refresh on this unit
// will pick up the missed update.
void DirectorRenderObserver::AddSetOwner(Unit unit, sint32 o)
{
    if (auto actor = uiunitactorregistry_Get().Get(unit)) {
        g_director->AddSetOwner(actor, o);
    }
}
void DirectorRenderObserver::AddSetVisibility(Unit unit, uint32 v)
{
    if (auto actor = uiunitactorregistry_Get().Get(unit)) {
        g_director->AddSetVisibility(actor, v);
    }
}
void DirectorRenderObserver::AddSetVisionRange(Unit unit, double r)
{
    if (auto actor = uiunitactorregistry_Get().Get(unit)) {
        g_director->AddSetVisionRange(actor, r);
    }
}
void DirectorRenderObserver::AddMorphUnit(SpriteStatePtr ss, sint32 type, Unit id)
{
    if (auto actor = uiunitactorregistry_Get().Get(id)) {
        g_director->AddMorphUnit(actor, ss, type, id);
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
void DirectorRenderObserver::ActiveUnitRemove(std::shared_ptr<UnitActor> u)            { g_director->ActiveUnitRemove(u); }
void DirectorRenderObserver::TradeActorCreate(TradeRoute newRoute)                     { g_director->TradeActorCreate(newRoute); }
void DirectorRenderObserver::TradeActorDestroy(TradeRoute r)                           { g_director->TradeActorDestroy(r); }

void DirectorRenderObserver::AddMove(Unit mover,
                                     MapPoint const &oldPos, MapPoint const &newPos,
                                     const render_observer::UnitActorVec &revealedActors,
                                     const render_observer::UnitActorVec &restOfStack,
                                     bool isTransported, sint32 soundID)
{
    g_director->AddMove(mover, oldPos, newPos, revealedActors, restOfStack, isTransported, soundID);
}
void DirectorRenderObserver::AddTeleport(Unit top,
                                         MapPoint const &oldPos, MapPoint const &newPos,
                                         const render_observer::UnitActorVec &revealedActors,
                                         const render_observer::UnitActorVec &moveActors)
{
    g_director->AddTeleport(top, oldPos, newPos, revealedActors, moveActors);
}
void DirectorRenderObserver::AddAttack(Unit attacker, Unit attacked)                   { g_director->AddAttack(attacker, attacked); }
void DirectorRenderObserver::AddAttackPos(Unit attacker, MapPoint const &pos)          { g_director->AddAttackPos(attacker, pos); }
void DirectorRenderObserver::AddSpecialAttack(Unit attacker, Unit attacked, sint32 a)  { g_director->AddSpecialAttack(attacker, attacked, (SPECATTACK)a); }
void DirectorRenderObserver::PositionActor(Unit unit, MapPoint const &pos)
{
    if (auto actor = uiunitactorregistry_Get().Get(unit)) {
        actor->PositionActor(pos);
    }
}
void DirectorRenderObserver::AddSpecialEffect(MapPoint &pos, sint32 spriteID, sint32 soundID){ g_director->AddSpecialEffect(pos, spriteID, soundID); }
void DirectorRenderObserver::AddTerminateFaceoff(Unit &faceoffer)                      { g_director->AddTerminateFaceoff(faceoffer); }

void DirectorRenderObserver::AddCenterMap(const MapPoint &pos)                         { g_director->AddCenterMap(pos); }
bool DirectorRenderObserver::TileWillBeCompletelyVisible(sint32 x, sint32 y)           { return g_director->TileWillBeCompletelyVisible(x, y); }

void DirectorRenderObserver::NextPlayer(sint32 forcedUpdate)                           { g_director->NextPlayer(forcedUpdate ? TRUE : FALSE); }
void DirectorRenderObserver::AddCopyVision()                                           { g_director->AddCopyVision(); }
void DirectorRenderObserver::AddEndTurn()                                              { g_director->AddEndTurn(); }
void DirectorRenderObserver::CatchUp()                                                 { g_director->CatchUp(); }
void DirectorRenderObserver::AddBeginScheduler(sint32 player)                          { g_director->AddBeginScheduler(player); }
void DirectorRenderObserver::AddPlaySound(sint32 soundID, MapPoint const &pos)         { g_director->AddPlaySound(soundID, pos); }
void DirectorRenderObserver::AddPlayWonderMovie(sint32 which)                          { g_director->AddPlayWonderMovie(which); }
void DirectorRenderObserver::IncrementPendingGameActions()                             { g_director->IncrementPendingGameActions(); }
void DirectorRenderObserver::DecrementPendingGameActions()                             { g_director->DecrementPendingGameActions(); }

namespace {
    DirectorRenderObserver s_directorRenderObserver;
}

void RegisterDirectorRenderObserver()
{
    render_observer::Register(&s_directorRenderObserver);
}
