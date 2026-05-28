//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ header
// Description  : UI-side registry mapping live Units → their UnitActor.
//
//----------------------------------------------------------------------------
//
// Phase 3 slice 7a of the UnitActor split — first step of the
// observer-event pivot.
//
// The registry is a UI-build concept: it tracks which UnitActor
// belongs to which Unit, so future event handlers (OnUnitMoved,
// OnUnitVisibilityChanged, etc.) can look up a render target without
// going through gs/-side `Unit::GetActor()`.
//
// In this slice the registry is a *passive shadow*: it simply records
// the same UnitActorPtr that `UnitData::m_actor` already owns.  No
// gs/ code changes; existing `unit.GetActor()` calls keep working.
// Once subsequent slices move every gs/ → UnitActor mutation onto an
// observer event, gs/ can drop its own m_actor and the registry
// becomes the canonical owner.
//
// Lifecycle: the registry is populated by `UIGameObserver::OnUnitSpawned`
// and drained by `OnUnitDestroyed`.  In the headless build no observer
// is registered, so the registry never sees any inserts — it stays
// empty and weighs nothing.
//
//----------------------------------------------------------------------------

#pragma once

#include <unordered_map>

#include "ctp2_inttypes.h"
#include "gs/core/sprite_state_fwd.h"  // UnitActorPtr
#include "gs/gameobj/Unit.h"           // Unit (gs/ handle, value type)

class UIUnitActorRegistry
{
public:
    // Inserts (or overwrites) the actor for `unit`.  Null `actor` is
    // treated as no-op: a registry entry pointing at nullptr would
    // surprise lookups, and it has the same observable behaviour as
    // simply not having an entry.
    void Insert(Unit unit, UnitActorPtr actor);

    // Removes any entry for `unit`.  Safe to call for units that were
    // never inserted (e.g. in tests).
    void Remove(Unit unit);

    // Returns the registered actor, or nullptr if `unit` is unknown.
    UnitActorPtr Get(Unit unit) const;

    size_t Size() const { return m_actors.size(); }
    void Clear() { m_actors.clear(); }

private:
    std::unordered_map<uint32, UnitActorPtr> m_actors;  // keyed by Unit::m_id
};

// Single global instance.  Declared here, defined in the .cpp.  In the
// headless build the instance still exists (so unit tests that link
// the UI sources can poke at it) but no observer feeds it.
extern UIUnitActorRegistry g_uiUnitActorRegistry;
