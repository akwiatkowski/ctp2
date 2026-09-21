//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ source
// Description  : UI-side adapter for battle_observer — owns the active
//                Battle* and forwards engine notifications into it.
//
//----------------------------------------------------------------------------
//
// gs/ no longer holds a Battle pointer; the engine just fires events to
// battle_observer.  This adapter, registered once at UI startup,
// translates those events into new Battle()/BattleEvent allocations and
// the AddBattle / UpdateBattle / EndBattle calls the director and
// battleViewWindow expect.
//
// battleviewwindow.cpp asks `BattleObserverAdapter::GetCurrentBattle()`
// when it needs to compare the live Battle against its rendered view —
// previously this was `g_theCurrentBattle->GetBattle()` reaching into
// gs/.
//
//----------------------------------------------------------------------------

#include "ctp/c3.h"
#include "gs/core/battle_observer.h"
#include "ui/interface/battle.h"
#include "ui/interface/battleevent.h"
#include "ui/interface/battleviewwindow.h"
#include "ui/interface/battleview.h"
#include "ui/aui_common/aui_button.h"            // AUI_BUTTON_ACTION_EXECUTE
#include "ui/aui_ctp2/c3ui.h"
#include "gfx/spritesys/director.h"
#include "gs/gameobj/Army.h"
#include "gs/gameobj/Unit.h"
#include "gs/world/cellunitlist.h"
#include <memory>


// Forward declared in battle_view layer; used by combatevent.cpp's
// "close previous view" path.


namespace {

class BattleObserverAdapter : public battle_observer::Impl
{
public:
    bool StartBattle(const Army &attackers,
                     const CellUnitList &defenders) override
    {
        // Defensive: if a previous battle never got EndBattle'd, drop it
        // before starting a new one so we don't leak.
        m_battle = std::make_unique<Battle>();
        m_battle->Initialize(attackers, defenders);

        // Open a placement event — populated by subsequent AddPlacement
        // calls and committed by CommitPlacement.
        m_pendingPlacement = std::make_unique<BattleEvent>(BATTLE_EVENT_TYPE_PLACEMENT);

        if (director_Get()) director_Get()->AddBattle(m_battle.get());
        return true;
    }

    void AddPlacement(const Unit &unit, bool isDefender,
                      sint32 col, sint32 row, bool initial) override
    {
        if (!m_battle || !m_pendingPlacement) return;
        m_battle->PositionUnit(m_pendingPlacement.get(), isDefender ? TRUE : FALSE,
                               unit, col, row, initial);
    }

    void CommitPlacement() override
    {
        if (!m_battle || !m_pendingPlacement) return;
        m_battle->AddEvent(m_pendingPlacement.release());
        // Open the next placement event so callers can populate again on
        // re-balance.  Battle::AddEvent takes ownership of the prior one.
        m_pendingPlacement = std::make_unique<BattleEvent>(BATTLE_EVENT_TYPE_PLACEMENT);
    }

    void AddAttack(const Unit &unit, bool isDefender) override
    {
        if (!m_battle) return;
        auto e = std::make_unique<BattleEvent>(BATTLE_EVENT_TYPE_ATTACK);
        m_battle->AddUnitAttack(e.get(), isDefender ? TRUE : FALSE, unit);
        m_battle->AddEvent(e.release());
    }

    void AddDeath(const Unit &unit, bool isDefender) override
    {
        if (!m_battle) return;
        auto e = std::make_unique<BattleEvent>(BATTLE_EVENT_TYPE_DEATH);
        m_battle->AddUnitDeath(e.get(), isDefender ? TRUE : FALSE, unit);
        m_battle->AddEvent(e.release());
    }

    void AddExplosion(const Unit &unit, bool isDefender) override
    {
        if (!m_battle) return;
        auto e = std::make_unique<BattleEvent>(BATTLE_EVENT_TYPE_EXPLODE);
        m_battle->AddUnitExplosion(e.get(), isDefender ? TRUE : FALSE, unit);
        m_battle->AddEvent(e.release());
    }

    void UpdateBattle() override
    {
        if (BattleViewWindow *bvw = battleviewwindow_Get(); m_battle && bvw)
        {
            bvw->UpdateBattle(m_battle.get());
        }
    }

    void EndBattle() override
    {
        if (!m_battle) return;
        if (BattleViewWindow *bvw = battleviewwindow_Get()) bvw->EndBattle();
        // unique_ptr members; an uncommitted m_pendingPlacement is now
        // destroyed by reset() rather than orphaned — the legacy gs/-side
        // path leaked it on delete m_battle.
        m_battle.reset();
        m_pendingPlacement.reset();
    }

    void CloseBattleView() override
    {
        if (BattleViewWindow *bvw = battleviewwindow_Get();
            bvw && c3ui_Get() && c3ui_Get()->GetWindow(bvw->Id()))
        {
            battleview_ExitButtonActionCallback(nullptr,
                                                AUI_BUTTON_ACTION_EXECUTE,
                                                0, nullptr);
        }
    }

    Battle *GetCurrentBattle() const { return m_battle.get(); }

private:
    std::unique_ptr<Battle>      m_battle;
    std::unique_ptr<BattleEvent> m_pendingPlacement;
};

BattleObserverAdapter g_battleObserverAdapter;

} // namespace

void RegisterBattleObserverAdapter()
{
    battle_observer::Register(&g_battleObserverAdapter);
}

void UnregisterBattleObserverAdapter()
{
    battle_observer::Register(nullptr);
}

// Accessor for battleviewwindow.cpp's IsCurrentBattle comparison.  Kept
// non-namespace so the include footprint there is minimal — just declare
// it `extern Battle *BattleObserverAdapter_GetCurrentBattle()` in the
// caller.
Battle *BattleObserverAdapter_GetCurrentBattle()
{
    return g_battleObserverAdapter.GetCurrentBattle();
}
