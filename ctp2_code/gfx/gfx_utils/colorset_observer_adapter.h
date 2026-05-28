// gfx/gfx_utils/colorset_observer_adapter.h
// UI-build concrete impl of colorset_observer::Impl.
// Forwards every call to g_colorSet (null-checked).

#pragma once

#include "gs/core/colorset_observer.h"

class ColorSetObserverAdapter : public colorset_observer::Impl
{
public:
    uint16 GetColor(COLOR color) override;
    uint16 GetPlayerColor(sint32 playerNum) override;
};

// Installs a singleton ColorSetObserverAdapter as the registered
// colorset_observer::Impl. Idempotent. Called from ColorSet::Initialize()
// so that any UI build that boots the colour set automatically wires
// the bridge; headless never reaches this code.
void RegisterColorSetObserverAdapter();
