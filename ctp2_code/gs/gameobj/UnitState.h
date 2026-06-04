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
#include "gs/gameobj/Unit.h"     // Unit (the gs/-side handle, value type)
#include "gs/world/MapPoint.h"   // MapPoint (position value type)

#include <nlohmann/json.hpp>


// Two construction modes determine how reads dispatch:
//   * LIVE mode    — m_unit_id is valid.  Position reads delegate to the
//                    authoritative UnitData via m_unit_id.RetPos().
//                    m_pos is unused.  Used for live units (UnitData ctors).
//   * SNAPSHOT mode — m_unit_id is default (invalid).  Position reads
//                    return m_pos.  Used for fog-of-war snapshots that
//                    must remember a unit's last-known location after
//                    its gs/ object is gone or has moved.
class UnitState
{
public:
    UnitState();
    explicit UnitState(Unit id);
    explicit UnitState(MapPoint snapshot_pos);  // SNAPSHOT mode

    Unit GetUnitID() const { return m_unit_id; }
    void SetUnitID(Unit id) { m_unit_id = id; }

    // Dispatches: LIVE → m_unit_id.RetPos(); SNAPSHOT → m_pos.
    MapPoint GetPos() const;

    // Writes m_pos unconditionally.  For LIVE units this is a no-op
    // from the reader's perspective (GetPos doesn't read m_pos), but
    // the field is kept in sync so a UnitState can be converted from
    // LIVE to SNAPSHOT later by simply clearing m_unit_id.
    void SetPos(MapPoint pnt) { m_pos = pnt; }

    // JSON savegame bridge.  Currently serialises just unit_id + pos
    // — matches the binary path's Phase 1 placeholder (nothing
    // serialised yet).  Subsequent migration phases that move fields
    // from UnitActor will extend both Serialize and this bridge.
    friend void to_json(nlohmann::json &j, UnitState const &s);
    friend void from_json(nlohmann::json const &j, UnitState &s);

private:
    Unit m_unit_id;
    MapPoint m_pos;
};
