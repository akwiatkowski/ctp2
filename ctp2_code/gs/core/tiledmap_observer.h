//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ header
// Description  : Engine-side notifications to the tile-map / map-render layer
//
//----------------------------------------------------------------------------
//
// Game-state code (`gs/`) and AI code (`ai/`) historically called
// `g_tiledMap->X()` directly to invalidate dirty rects, request tile
// redraws, post-process tiles after terrain changes, and similar
// map-render work.  TiledMap lives in `gfx/tilesys/` — that is the wrong
// direction in the layered architecture, since gs/ should not depend on
// gfx/.
//
// This header exposes a subset of TiledMap's surface as a set of free
// functions that fan out to a registered `Impl` callback.  The UI build
// registers a `TiledMapObserverAdapter` (in
// `gfx/tilesys/tiledmap_observer_adapter.cpp`) that forwards to
// `g_tiledMap`; the headless build leaves the observer unregistered and
// every call becomes a no-op (with safe defaults for non-void returns).
//
// Migration pattern at the call site:
//   before:  if (g_tiledMap) g_tiledMap->RedrawTile(&pos);
//   after:   tiledmap_observer::RedrawTile(pos);
//
// before (block):
//   if (g_tiledMap) {
//       g_tiledMap->InvalidateMix();
//       g_tiledMap->InvalidateMap();
//       g_tiledMap->Refresh();
//   }
// after:
//   tiledmap_observer::InvalidateMix();
//   tiledmap_observer::InvalidateMap();
//   tiledmap_observer::Refresh();
//
// Mirrors `gs/core/audio_observer.h` and `gs/core/render_observer.h`.
//
// Out of scope for this observer:
//   - Lifecycle of g_tiledMap itself (new/delete) — owned by the UI build's
//     initialization code; gameinit.cpp still references TiledMap directly.
//   - GetLocalVision() — returns Vision*; treated as a query, exposed as a
//     pointer-returning fan-out below.
//
//----------------------------------------------------------------------------

#pragma once

#include "ctp2_inttypes.h"

class MapPoint;
class TileInfo;
class Vision;

namespace tiledmap_observer {

// --- The Impl interface ---
// Concrete implementations live in:
//   - gfx/tilesys/tiledmap_observer_adapter.cpp (UI build, forwards to g_tiledMap)
//   - test fixtures (record-and-replay spies, no-op stubs)
// Headless does not register an Impl; the free functions below short-circuit.
class Impl
{
public:
    virtual ~Impl() = default;

    // Per-tile redraw requests.  Position passed by const ref; the UI
    // adapter takes &pos when forwarding to TiledMap::RedrawTile(MapPoint*).
    virtual void RedrawTile(MapPoint const &pos) = 0;

    // Post-process a tile after a terrain / improvement / city change.
    // TileInfo* is g_theWorld->GetTileInfo(pos) at the call site; the
    // observer just forwards it.
    virtual void PostProcessTile(MapPoint const &pos, TileInfo *info) = 0;

    // Mark a tile as having changed (used to refresh sprite layers, etc.).
    virtual void TileChanged(MapPoint const &pos) = 0;

    // Whole-map post-process (currently only WrldPoll calls this).
    virtual void PostProcessMap() = 0;

    // Composited refresh + invalidations.
    virtual void Refresh() = 0;
    virtual void InvalidateMap() = 0;
    virtual void InvalidateMix() = 0;

    // Camera / visibility query — returns false in headless.
    virtual bool TileIsVisible(sint32 mapX, sint32 mapY) = 0;

    // Vision sync — used between turns.
    virtual void CopyVision() = 0;
};

// --- Registration ---
void Register(Impl *impl);
Impl *Get();

// --- Free-function fan-outs ---
// Each function null-checks Get() and forwards.  Non-void functions return
// a safe default when no Impl is registered.

void RedrawTile(MapPoint const &pos);
void PostProcessTile(MapPoint const &pos, TileInfo *info);
void TileChanged(MapPoint const &pos);
void PostProcessMap();

void Refresh();
void InvalidateMap();
void InvalidateMix();

bool TileIsVisible(sint32 mapX, sint32 mapY);   // returns false in headless

void CopyVision();

} // namespace tiledmap_observer
