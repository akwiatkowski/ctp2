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

extern Director *g_director;

void DirectorRenderObserver::AddShow(Unit hider)                                       { g_director->AddShow(hider); }
void DirectorRenderObserver::AddHide(Unit hider)                                       { g_director->AddHide(hider); }
void DirectorRenderObserver::AddDeath(Unit dead)                                       { g_director->AddDeath(dead); }
void DirectorRenderObserver::AddFastKill(Unit dead)                                    { g_director->AddFastKill(dead); }
void DirectorRenderObserver::FastKill(std::shared_ptr<UnitActor> actor)                { g_director->FastKill(actor); }
void DirectorRenderObserver::FastKillEffect(EffectActor *actor)                        { g_director->FastKill(actor); }
void DirectorRenderObserver::AddSetOwner(std::shared_ptr<UnitActor> actor, sint32 o)   { g_director->AddSetOwner(actor, o); }
void DirectorRenderObserver::AddSetVisibility(std::shared_ptr<UnitActor> actor, uint32 v){ g_director->AddSetVisibility(actor, v); }
void DirectorRenderObserver::AddSetVisionRange(std::shared_ptr<UnitActor> actor, double r){ g_director->AddSetVisionRange(actor, r); }
void DirectorRenderObserver::AddMorphUnit(std::shared_ptr<UnitActor> m,
                                          SpriteStatePtr ss, sint32 type, Unit id)     { g_director->AddMorphUnit(m, ss, type, id); }
void DirectorRenderObserver::ChangeUnitImage(std::shared_ptr<UnitActor> actor,
                                             SpriteStatePtr ss, sint32 type, Unit id)
{
    if (actor) actor->ChangeImage(ss, type, id);
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
