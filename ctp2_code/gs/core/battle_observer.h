//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ header
// Description  : Engine-side bridge for the per-combat battle animation queue
//
//----------------------------------------------------------------------------
//
// CTP2Combat used to hold a `Battle *m_battle` member (Battle is defined in
// ui/interface/battle.h).  gs/ created and populated the Battle object
// directly with BattleEvent allocations, then handed it off to the Director
// and g_battleViewWindow.  That direction of include — gs/ knowing about
// gfx/UI types — is the wrong layering.
//
// This namespace mirrors the operations gs/ used to perform on the Battle
// pointer as a set of free function notifications.  The UI build registers
// a BattleObserverAdapter (in ui/interface/battle_observer_adapter.cpp)
// that owns its own `Battle *` and translates the bridge calls into
// new Battle(), Initialize, AddUnitAttack/Death/Explosion, AddEvent,
// director_Get()->AddBattle, g_battleViewWindow->UpdateBattle/EndBattle.
// The headless build leaves the adapter unregistered and every call
// short-circuits.
//
// gs/gameobj/CTP2Combat.h previously exposed `Battle *GetBattle()`,
// `KillBattle()`, `ClearBattle()` — those have been removed.  In their
// place is `bool IsBattleActive()` + `void DeactivateBattle()`, mirroring
// the boolean state the bridge tracks.
//
//----------------------------------------------------------------------------

#pragma once

#include "ctp2_inttypes.h"

class Army;
class CellUnitList;
class Unit;

namespace battle_observer {

class Impl
{
public:
    virtual ~Impl() = default;

    // Create a new battle animation queue and seed it with a placement
    // event.  Implementation calls `new Battle()`, Initialize(attackers,
    // defenders), creates a BattleEvent(PLACEMENT), and registers the
    // battle with the Director via AddBattle.
    //
    // Returns true if a battle was successfully started — gs/ should
    // mirror this into its `m_battleActive` flag so subsequent AddAttack /
    // AddDeath / AddExplosion calls know whether to fire.
    virtual bool StartBattle(const Army &attackers,
                             const CellUnitList &defenders) = 0;

    // Populate the current placement event with one unit's position.
    // Called repeatedly from CombatField::ReportUnits during init and
    // when units re-balance after a round.  isDefender selects which
    // half of the battle field receives the unit.
    virtual void AddPlacement(const Unit &unit, bool isDefender,
                              sint32 col, sint32 row, bool initial) = 0;

    // Commit the accumulated placement event (the queue absorbs it and
    // starts a fresh PLACEMENT for the next round's re-balance).
    virtual void CommitPlacement() = 0;

    // Per-action events.  Each call wraps one BattleEvent of the named
    // type and appends it to the active Battle.  Caller is responsible
    // for gating on `m_battleActive`.
    virtual void AddAttack(const Unit &unit, bool isDefender) = 0;
    virtual void AddDeath(const Unit &unit, bool isDefender) = 0;
    virtual void AddExplosion(const Unit &unit, bool isDefender) = 0;

    // Refresh the BattleViewWindow with the current Battle state
    // (typically called once per visual round, after death/explosion
    // events have settled).
    virtual void UpdateBattle() = 0;

    // Combat finished — end the BattleViewWindow display and delete the
    // adapter's Battle*.  Also fired from CTP2Combat::~CTP2Combat as a
    // safety net if the combat ends without an explicit EndBattle().
    virtual void EndBattle() = 0;

    // Close any open battle view window without ending the underlying
    // battle.  Used by combatevent.cpp::StartCombatEvent to dismiss a
    // leftover view from the previous combat before opening a new one.
    virtual void CloseBattleView() = 0;
};

void Register(Impl *impl);
Impl *Get();   // NULL in headless / pre-registration

// Free-function fan-outs.  When no Impl is registered:
//   StartBattle → false (caller's m_battleActive stays false; events skipped)
//   all void notifications → no-op
bool StartBattle(const Army &attackers, const CellUnitList &defenders);
void AddPlacement(const Unit &unit, bool isDefender,
                  sint32 col, sint32 row, bool initial);
void CommitPlacement();
void AddAttack(const Unit &unit, bool isDefender);
void AddDeath(const Unit &unit, bool isDefender);
void AddExplosion(const Unit &unit, bool isDefender);
void UpdateBattle();
void EndBattle();
void CloseBattleView();

} // namespace battle_observer
