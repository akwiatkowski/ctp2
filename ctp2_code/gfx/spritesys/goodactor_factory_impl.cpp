// gfx/spritesys/goodactor_factory_impl.cpp
// Strong overrides of the weak goodactor_factory free functions.
// Linked into the UI build only; headless gets the weak no-ops from
// gs/core/goodactor_factory.cpp.

#include "ctp/c3.h"
#include <memory>
#include "gs/core/goodactor_factory.h"
#include "gfx/spritesys/GoodActor.h"   // GoodActor, full type

GoodActor *goodactor_factory_create(sint32 id, MapPoint const &pos)
{
    return std::make_unique<GoodActor>(id, pos).release();
}

void goodactor_factory_destroy(GoodActor *actor)
{
    std::unique_ptr<GoodActor>(actor).reset();
}
