// gs/core/gfx_options_observer.cpp
// See gfx_options_observer.h for the rationale. Thin dispatch layer.

#include "ctp/c3.h"
#include "gs/core/gfx_options_observer.h"

namespace gfx_options_observer {

namespace {
    Impl *s_impl = nullptr;
}

void Register(Impl *impl) { s_impl = impl; }
Impl *Get()                { return s_impl; }

bool AddTextToCell(MapPoint const &pos, const char *text, uint8 colorMagnitude)
{
    return s_impl ? s_impl->AddTextToCell(pos, text, colorMagnitude) : false;
}

bool AddTextToArmy(Army army, const char *text, uint8 colorMagnitude,
                   sint32 goalType)
{
    return s_impl ? s_impl->AddTextToArmy(army, text, colorMagnitude, goalType)
                  : false;
}

bool IsCellTextOn()
{
    return s_impl ? s_impl->IsCellTextOn() : false;
}

} // namespace gfx_options_observer
