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
#include "gfx/spritesys/Director.h"
#include "gs/gameobj/Army.h"
#include "gs/gameobj/Unit.h"
#include "gs/world/cellunitlist.h"

// Forward declared in battle_view layer; used by combatevent.cpp's
// "close previous view" path.
extern void battleview_ExitButtonActionCallback(aui_Control *control,
                                                uint32 action,
                                                uint32 data,
                                                void *cookie);

namespace {

class BattleObserverAdapter : public battle_observer::Impl
{
public:
    bool StartBattle(const Army &attackers,
                     const CellUnitList &defenders) override
    {
        // Defensive: if a previous battle never got EndBattle'd, drop it
        // before starting a new one so we don't leak.
        if (m_battle)
        {
            delete m_battle;
            m_battle = nullptr;
        }
        m_battle = new Battle();
        m_battle->Initialize(attackers, defenders);

        // Open a placement event — populated by subsequent AddPlacement
        // calls and committed by CommitPlacement.
        m_pendingPlacement = new BattleEvent(BATTLE_EVENT_TYPE_PLACEMENT);

        if (director_Get()) director_Get()->AddBattle(m_battle);
        return true;
    }

    void AddPlacement(const Unit &unit, bool isDefender,
                      sint32 col, sint32 row, bool initial) override
    {
        if (!m_battle || !m_pendingPlacement) return;
        m_battle->PositionUnit(m_pendingPlacement, isDefender ? TRUE : FALSE,
                               unit, col, row, initial);
    }

    void CommitPlacement() override
    {
        if (!m_battle || !m_pendingPlacement) return;
        m_battle->AddEvent(m_pendingPlacement);
        // Open the next placement event so callers can populate again on
        // re-balance.  Battle::AddEvent takes ownership of the prior one.
        m_pendingPlacement = new BattleEvent(BATTLE_EVENT_TYPE_PLACEMENT);
    }

    void AddAttack(const Unit &unit, bool isDefender) override
    {
        if (!m_battle) return;
        BattleEvent *e = new BattleEvent(BATTLE_EVENT_TYPE_ATTACK);
        m_battle->AddUnitAttack(e, isDefender ? TRUE : FALSE, unit);
        m_battle->AddEvent(e);
    }

    void AddDeath(const Unit &unit, bool isDefender) override
    {
        if (!m_battle) return;
        BattleEvent *e = new BattleEvent(BATTLE_EVENT_TYPE_DEATH);
        m_battle->AddUnitDeath(e, isDefender ? TRUE : FALSE, unit);
        m_battle->AddEvent(e);
    }

    void AddExplosion(const Unit &unit, bool isDefender) override
    {
        if (!m_battle) return;
        BattleEvent *e = new BattleEvent(BATTLE_EVENT_TYPE_EXPLODE);
        m_battle->AddUnitExplosion(e, isDefender ? TRUE : FALSE, unit);
        m_battle->AddEvent(e);
    }

    void UpdateBattle() override
    {
        if (BattleViewWindow *bvw = battleviewwindow_Get(); m_battle && bvw)
        {
            bvw->UpdateBattle(m_battle);
        }
    }

    void EndBattle() override
    {
        if (!m_battle) return;
        if (BattleViewWindow *bvw = battleviewwindow_Get()) bvw->EndBattle();
        // Note: any uncommitted m_pendingPlacement is orphaned here.
        // Battle::~Battle() doesn't iterate uncommitted events, so this
        // is a minor leak by design — matches the legacy gs/-side
        // ownership behavior (gs/ also dropped the BattleEvent* on
        // delete m_battle).  Re-evaluate when revisiting Battle ownership.
        delete m_battle;
        m_battle = nullptr;
        m_pendingPlacement = nullptr;
    }

    void CloseBattleView() override
    {
        if (BattleViewWindow *bvw = battleviewwindow_Get();
            bvw && c3ui_Get() && c3ui_Get()->GetWindow(bvw->Id()))
        {
            battleview_ExitButtonActionCallback(NULL,
                                                AUI_BUTTON_ACTION_EXECUTE,
                                                0, NULL);
        }
    }

    Battle *GetCurrentBattle() const { return m_battle; }

private:
    Battle      *m_battle = nullptr;
    BattleEvent *m_pendingPlacement = nullptr;
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
