//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ header
// Description  : Game-state half of the split of UnitActor
//
//----------------------------------------------------------------------------
//
// UnitActor (gfx/spritesys/) historically held a mix of game-state data
// (position, ownership, fortified flags, vision, etc.) and rendering
// machinery (sprite, animation, Director queue).  This conflation is
// the root cause of the remaining gs/→gfx/ coupling: UnitData owns a
// shared_ptr<UnitActor> and gs/ code mutates the actor's state.
//
// `UnitState` is the first half of the split.  It will own the
// game-state fields.  The second half (`UnitRenderer`, the renamed
// UnitActor with only sprite/animation state) will live in gfx/.
//
// This file is intentionally empty in its current form — Phase 1 is
// just scaffolding.  Subsequent migration phases (write-through, then
// drop) move fields one batch at a time from UnitActor into here.
//
// Lifetime model:
//   * UnitState is a value-type member of UnitData.  Created/destroyed
//     with the UnitData it belongs to.
//   * UnitRenderer (UI-build only) holds a `UnitState const *` and
//     polls each frame.  The renderer is destroyed before its UnitData
//     via game_observer::OnUnitDestroyed (added in Phase 2).
//
//----------------------------------------------------------------------------

#pragma once

#include "ctp2_inttypes.h"
#include "gs/gameobj/Unit.h"   // Unit (the gs/-side handle, value type)

class CivArchive;

class UnitState
{
public:
    UnitState();
    explicit UnitState(Unit id);

    // Identity — links back to the gs/-side Unit handle so renderers
    // can find their state.
    Unit GetUnitID() const { return m_unit_id; }
    void SetUnitID(Unit id) { m_unit_id = id; }

    // Phase 1 placeholder.  Real serialization populates as fields are
    // migrated in Phase 3.  Renderer state is NOT serialized (transient).
    void Serialize(CivArchive &archive);

private:
    Unit m_unit_id;
};
