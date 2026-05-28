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
    m_unit_id()
{
}

UnitState::UnitState(Unit id)
:
    m_unit_id(id)
{
}

void UnitState::Serialize(CivArchive &archive)
{
    // Phase 1: nothing to serialize yet — m_unit_id is recoverable from
    // the owning UnitData on load and doesn't need to be persisted
    // separately.  Subsequent phases add fields here as they are
    // migrated from UnitActor.
    (void)archive;
}
