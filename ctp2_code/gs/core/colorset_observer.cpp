// gs/core/colorset_observer.cpp
// See colorset_observer.h for the rationale. Thin dispatch layer.

#include "ctp/c3.h"
#include "gs/core/colorset_observer.h"

namespace colorset_observer {

namespace {
    Impl *s_impl = nullptr;
}

void Register(Impl *impl) { s_impl = impl; }
Impl *Get()                { return s_impl; }

uint16 GetColor(COLOR color)
{
    return s_impl ? s_impl->GetColor(color) : 0;
}

uint16 GetPlayerColor(sint32 playerNum)
{
    return s_impl ? s_impl->GetPlayerColor(playerNum) : 0;
}

} // namespace colorset_observer
