// gfx/gfx_utils/gfx_options_observer_adapter.cpp
// 1:1 forwarder from gfx_options_observer::Impl to graphicsoptions_Get().

#include "ctp/c3.h"
#include "gfx/gfx_utils/gfx_options_observer_adapter.h"
#include "gfx/gfx_utils/gfx_options.h"   // GraphicsOptions, graphicsoptions_Get()

bool GraphicsOptionsObserverAdapter::AddTextToCell(MapPoint const &pos,
                                                   const char *text,
                                                   uint8 colorMagnitude)
{
    return graphicsoptions_Get()
         ? graphicsoptions_Get()->AddTextToCell(pos, text, colorMagnitude)
         : false;
}

bool GraphicsOptionsObserverAdapter::AddTextToArmy(Army army, const char *text,
                                                   uint8 colorMagnitude,
                                                   sint32 goalType)
{
    return graphicsoptions_Get()
         ? graphicsoptions_Get()->AddTextToArmy(army, text, colorMagnitude, goalType)
         : false;
}

bool GraphicsOptionsObserverAdapter::IsCellTextOn()
{
    return graphicsoptions_Get() ? graphicsoptions_Get()->IsCellTextOn() : false;
}

namespace {
    GraphicsOptionsObserverAdapter s_graphicsOptionsObserverAdapter;
}

void RegisterGraphicsOptionsObserverAdapter()
{
    gfx_options_observer::Register(&s_graphicsOptionsObserverAdapter);
}
