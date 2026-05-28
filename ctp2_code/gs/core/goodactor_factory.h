//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ header
// Description  : Factory free functions for GoodActor lifecycle
//
//----------------------------------------------------------------------------
//
// Wormhole.cpp and other gs/ code historically called
// `m_actor = new GoodActor(id, pos);` directly to spawn a GoodActor.
// GoodActor lives in `gfx/spritesys/` — gs/ should not include it.
//
// These free functions wrap the new/delete behind an opaque-pointer
// interface so gs/ code can construct + destroy GoodActor without
// the full include.  Weak no-op defaults live in
// `gs/core/goodactor_factory.cpp`; the UI build's
// `gfx/spritesys/goodactor_factory_impl.cpp` provides strong
// overrides that do the real work.
//
// Note: TileInfo.cpp ALSO constructs GoodActor (via copy + load
// archive ctors and calls FullLoad/Serialize methods).  That is a
// documented exception — TileInfo legitimately owns a GoodActor with
// many operations.  Migrating it would need 6+ helper functions for
// modest benefit.
//
//----------------------------------------------------------------------------

#pragma once

#include "ctp2_inttypes.h"

class GoodActor;
class MapPoint;

// Returns a heap-allocated GoodActor, or nullptr in headless / when no
// implementation is registered.  Caller owns the returned pointer.
GoodActor *goodactor_factory_create(sint32 id, MapPoint const &pos);

// Deletes a GoodActor obtained from the factory.  No-op on nullptr.
void goodactor_factory_destroy(GoodActor *actor);
