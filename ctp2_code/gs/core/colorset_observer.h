//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ header
// Description  : Engine-side query interface for color lookups
//
//----------------------------------------------------------------------------
//
// Game-state code (`gs/`) and AI code (`ai/`) historically called
// `g_colorSet->GetColor(COLOR_X)` and `g_colorSet->GetPlayerColor(p)`
// directly to look up Pixel16 color values stored in member variables.
// ColorSet lives in `gfx/gfx_utils/` — gs/ and ai/ should not depend
// on gfx/.
//
// This header exposes the query surface as free functions that fan
// out to a registered `Impl` callback.  The UI build registers a
// `ColorSetObserverAdapter` (in
// `gfx/gfx_utils/colorset_observer_adapter.cpp`) that forwards to
// `g_colorSet`; the headless build leaves the observer unregistered
// and every call returns 0 (a safe default for Pixel16/uint16).
//
// Migration pattern at the call site:
//   before:  m_color = g_colorSet->GetColor(COLOR_YELLOW);
//   after:   m_color = colorset_observer::GetColor(COLOR_YELLOW);
//
// Mirrors `gs/core/audio_observer.h`, `gs/core/render_observer.h`,
// `gs/core/tiledmap_observer.h`, `gs/core/gfx_options_observer.h`.
//
//----------------------------------------------------------------------------

#pragma once

#include "ctp2_inttypes.h"
#include "gs/core/color_types.h"   // COLOR enum

namespace colorset_observer {

// --- The Impl interface ---
// Concrete implementations live in:
//   - gfx/gfx_utils/colorset_observer_adapter.cpp (UI build, forwards to g_colorSet)
//   - test fixtures (record-and-replay spies, no-op stubs)
// Headless does not register an Impl; the free functions below return 0.
class Impl
{
public:
    virtual ~Impl() = default;

    // Returns Pixel16 (uint16) for the given COLOR enum value.
    virtual uint16 GetColor(COLOR color) = 0;

    // Returns Pixel16 (uint16) for the given player index.
    virtual uint16 GetPlayerColor(sint32 playerNum) = 0;
};

// --- Registration ---
void Register(Impl *impl);   // pass nullptr to unregister
Impl *Get();

// --- Free-function fan-outs ---
// Each null-checks Get() and forwards.  Return 0 in headless (safe
// default — black/transparent in 565 RGB).

uint16 GetColor(COLOR color);
uint16 GetPlayerColor(sint32 playerNum);

} // namespace colorset_observer
