// gs/core/goodactor_factory.cpp
// Weak no-op defaults for the GoodActor factory.  The UI build's
// gfx/spritesys/goodactor_factory_impl.cpp provides strong overrides
// that do the real new/delete on GoodActor.

#include "ctp/c3.h"
#include "gs/core/goodactor_factory.h"

__attribute__((weak))
GoodActor *goodactor_factory_create(sint32 /*id*/, MapPoint const & /*pos*/)
{
    return nullptr;
}

__attribute__((weak))
void goodactor_factory_destroy(GoodActor * /*actor*/)
{
    // Headless: no GoodActor exists.  No-op.
}
