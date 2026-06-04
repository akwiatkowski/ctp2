// gs/gameobj/UnitState.cpp
// See UnitState.h for design notes.  Phase 1 is scaffolding only —
// subsequent phases migrate fields here from UnitActor.

#include "ctp/c3.h"
#include "gs/gameobj/UnitState.h"
// robot/aibackdoor/civarchive.h intentionally NOT included here — the
// Phase 1 Serialize stub is a no-op and only needs CivArchive's
// forward declaration (already in UnitState.h).  Subsequent phases
// that actually serialize will add it.

UnitState::UnitState()
:
    m_unit_id(),
    m_pos()
{
}

UnitState::UnitState(Unit id)
:
    m_unit_id(id),
    m_pos()
{
}

UnitState::UnitState(MapPoint snapshot_pos)
:
    m_unit_id(),
    m_pos(snapshot_pos)
{
}

MapPoint UnitState::GetPos() const
{
    // LIVE: defer to authoritative gs/UnitData position.
    if (m_unit_id.IsValid())
    {
        return m_unit_id.RetPos();
    }
    // SNAPSHOT (fog-of-war, no live unit): use the snapshot.
    return m_pos;
}


