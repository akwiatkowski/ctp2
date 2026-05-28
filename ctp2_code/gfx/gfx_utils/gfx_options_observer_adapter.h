// gfx/gfx_utils/gfx_options_observer_adapter.h
// UI-build concrete impl of gfx_options_observer::Impl.
// Forwards every call to g_graphicsOptions (null-checked).

#pragma once

#include "gs/core/gfx_options_observer.h"

class GraphicsOptionsObserverAdapter : public gfx_options_observer::Impl
{
public:
    bool AddTextToCell(MapPoint const &pos, const char *text,
                       uint8 colorMagnitude) override;
    bool AddTextToArmy(Army army, const char *text, uint8 colorMagnitude,
                       sint32 goalType) override;
    bool IsCellTextOn() override;
};

// Installs a singleton GraphicsOptionsObserverAdapter as the registered
// gfx_options_observer::Impl. Idempotent. Called from
// GraphicsOptions::Initialize() so any UI build that initialises the
// options singleton automatically wires the bridge; headless never
// reaches this code.
void RegisterGraphicsOptionsObserverAdapter();
