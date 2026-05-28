// gfx/spritesys/director_render_observer.h
// Adapter that bridges `render_observer::Impl` (gs/core) to `g_director`
// (gfx/spritesys/director.h).  Lives in gfx/ — it is allowed to depend
// on both layers.

#pragma once

#include "gs/core/render_observer.h"

class DirectorRenderObserver : public render_observer::Impl
{
public:
    // Lifecycle
    void AddShow(Unit hider)                                              override;
    void AddHide(Unit hider)                                              override;
    void AddDeath(Unit dead)                                              override;
    void AddFastKill(Unit dead)                                           override;
    void FastKill(std::shared_ptr<UnitActor> actor)                       override;
    void FastKillEffect(EffectActor *actor)                               override;
    void AddSetOwner(std::shared_ptr<UnitActor> actor, sint32 owner)      override;
    void AddSetVisibility(Unit unit, uint32 visibility)                   override;
    void AddSetVisionRange(std::shared_ptr<UnitActor> actor,
                           double range)                                  override;
    void AddMorphUnit(std::shared_ptr<UnitActor> morphingActor,
                      SpriteStatePtr ss, sint32 type, Unit id)            override;
    void ChangeUnitImage(std::shared_ptr<UnitActor> actor,
                         SpriteStatePtr ss, sint32 type, Unit id)         override;
    void ActiveUnitRemove(std::shared_ptr<UnitActor> unitActor)           override;
    void TradeActorCreate(TradeRoute newRoute)                            override;
    void TradeActorDestroy(TradeRoute routeToDestroy)                     override;

    // Animation
    void AddMove(Unit mover,
                 MapPoint const &oldPos, MapPoint const &newPos,
                 const render_observer::UnitActorVec &revealedActors,
                 const render_observer::UnitActorVec &restOfStack,
                 bool isTransported, sint32 soundID)                      override;
    void AddTeleport(Unit top,
                     MapPoint const &oldPos, MapPoint const &newPos,
                     const render_observer::UnitActorVec &revealedActors,
                     const render_observer::UnitActorVec &moveActors)                      override;
    void AddAttack(Unit attacker, Unit attacked)                          override;
    void AddAttackPos(Unit attacker, MapPoint const &pos)                 override;
    void AddSpecialAttack(Unit attacker, Unit attacked, sint32 attack)    override;
    void AddSpecialEffect(MapPoint &pos,
                          sint32 spriteID, sint32 soundID)                override;
    void AddTerminateFaceoff(Unit &faceoffer)                             override;

    // Camera
    void AddCenterMap(const MapPoint &pos)                                override;
    bool TileWillBeCompletelyVisible(sint32 x, sint32 y)                  override;

    // Turn flow / misc
    void NextPlayer(sint32 forcedUpdate)                                  override;
    void AddCopyVision()                                                  override;
    void AddEndTurn()                                                     override;
    void CatchUp()                                                        override;
    void AddBeginScheduler(sint32 player)                                 override;
    void AddPlaySound(sint32 soundID, MapPoint const &pos)                override;
    void AddPlayWonderMovie(sint32 which)                                 override;
    void IncrementPendingGameActions()                                    override;
    void DecrementPendingGameActions()                                    override;
};

// Convenience: instantiate the singleton adapter and register it.  Called
// once by civapp.cpp during InitializeApp, after g_director is alive.
void RegisterDirectorRenderObserver();
