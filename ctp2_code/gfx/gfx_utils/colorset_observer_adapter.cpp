// gfx/gfx_utils/colorset_observer_adapter.cpp
// 1:1 forwarder from colorset_observer::Impl to colorset_Get().

#include "ctp/c3.h"
#include "gfx/gfx_utils/colorset_observer_adapter.h"
#include "gfx/gfx_utils/colorset.h"   // ColorSet, colorset_Get()

uint16 ColorSetObserverAdapter::GetColor(COLOR color)
{
    return colorset_Get() ? colorset_Get()->GetColor(color) : 0;
}

uint16 ColorSetObserverAdapter::GetPlayerColor(sint32 playerNum)
{
    return colorset_Get() ? colorset_Get()->GetPlayerColor(playerNum) : 0;
}

namespace {
    ColorSetObserverAdapter s_colorSetObserverAdapter;
}

void RegisterColorSetObserverAdapter()
{
    colorset_observer::Register(&s_colorSetObserverAdapter);
}
