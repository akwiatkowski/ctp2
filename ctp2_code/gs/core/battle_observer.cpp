//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ source
// Description  : Engine-side bridge for the per-combat battle animation queue
//
//----------------------------------------------------------------------------

#include "ctp/c3.h"
#include "gs/core/battle_observer.h"

namespace battle_observer {

namespace {
Impl *s_impl = nullptr;
}

void Register(Impl *impl)
{
    s_impl = impl;
}

Impl *Get()
{
    return s_impl;
}

bool StartBattle(const Army &attackers, const CellUnitList &defenders)
{
    return s_impl ? s_impl->StartBattle(attackers, defenders) : false;
}

void AddPlacement(const Unit &unit, bool isDefender,
                  sint32 col, sint32 row, bool initial)
{
    if (s_impl) s_impl->AddPlacement(unit, isDefender, col, row, initial);
}

void CommitPlacement()
{
    if (s_impl) s_impl->CommitPlacement();
}

void AddAttack(const Unit &unit, bool isDefender)
{
    if (s_impl) s_impl->AddAttack(unit, isDefender);
}

void AddDeath(const Unit &unit, bool isDefender)
{
    if (s_impl) s_impl->AddDeath(unit, isDefender);
}

void AddExplosion(const Unit &unit, bool isDefender)
{
    if (s_impl) s_impl->AddExplosion(unit, isDefender);
}

void UpdateBattle()
{
    if (s_impl) s_impl->UpdateBattle();
}

void EndBattle()
{
    if (s_impl) s_impl->EndBattle();
}

void CloseBattleView()
{
    if (s_impl) s_impl->CloseBattleView();
}

} // namespace battle_observer
