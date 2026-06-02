//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ header
// Description  : Engine-side notifications to the debug-overlay layer
//
//----------------------------------------------------------------------------
//
// `ai/` and `gs/` code historically called `graphicsoptions_Get()->X()` to
// drop debug overlay text on the map (army labels, cell annotations).
// GraphicsOptions lives in `gfx/gfx_utils/` — gs/ and ai/ should not
// depend on gfx/.
//
// This header exposes the relevant surface as free functions that fan
// out to a registered `Impl`.  The UI build registers a
// `GraphicsOptionsObserverAdapter` (in
// `gfx/gfx_utils/gfx_options_observer_adapter.cpp`) that forwards to
// `graphicsoptions_Get()`; the headless build leaves the observer
// unregistered and every call becomes a no-op (with safe defaults for
// non-void returns).
//
// Migration pattern at the call site:
//   before:  if (graphicsoptions_Get() && graphicsoptions_Get()->IsCellTextOn())
//                graphicsoptions_Get()->AddTextToCell(pos, "hi", 255);
//   after:   if (gfx_options_observer::IsCellTextOn())
//                gfx_options_observer::AddTextToCell(pos, "hi", 255);
//
// Mirrors `gs/core/audio_observer.h`, `gs/core/render_observer.h`,
// `gs/core/tiledmap_observer.h`.
//
//----------------------------------------------------------------------------

#pragma once

#include "ctp2_inttypes.h"
#include "gs/gameobj/Army.h"   // Army (value type, passed by value)

class MapPoint;

namespace gfx_options_observer {

// --- The Impl interface ---
// Concrete implementations live in:
//   - gfx/gfx_utils/gfx_options_observer_adapter.cpp (UI build, forwards
//     to graphicsoptions_Get())
//   - test fixtures (record-and-replay spies, no-op stubs)
// Headless does not register an Impl; the free functions below short-circuit.
class Impl
{
public:
    virtual ~Impl() = default;

    // Returns true if the call did anything (e.g., text was added).
    virtual bool AddTextToCell(MapPoint const &pos, const char *text,
                               uint8 colorMagnitude) = 0;

    // goalType -1 means "no goal context".  Returns true if added.
    virtual bool AddTextToArmy(Army army, const char *text,
                               uint8 colorMagnitude, sint32 goalType) = 0;

    // Feature query — gs/ai code uses this to skip building the text
    // string when the overlay is disabled.  Returns false in headless.
    virtual bool IsCellTextOn() = 0;
};

// --- Registration ---
void Register(Impl *impl);   // pass nullptr to unregister
Impl *Get();

// --- Free-function fan-outs ---
// Each null-checks Get() and forwards.  Non-void functions return a
// safe default when no Impl is registered.

bool AddTextToCell(MapPoint const &pos, const char *text, uint8 colorMagnitude);
bool AddTextToArmy(Army army, const char *text, uint8 colorMagnitude,
                   sint32 goalType = -1);
bool IsCellTextOn();   // returns false in headless

} // namespace gfx_options_observer
