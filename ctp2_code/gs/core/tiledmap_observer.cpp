// gs/core/tiledmap_observer.cpp
// See tiledmap_observer.h for the rationale. Thin dispatch layer:
// holds the registered Impl pointer and forwards every free function.

#include "ctp/c3.h"
#include "gs/core/tiledmap_observer.h"

namespace tiledmap_observer {

namespace {
    Impl *s_impl = nullptr;
}

void Register(Impl *impl) { s_impl = impl; }
Impl *Get()                { return s_impl; }

#define DISPATCH_VOID(method, ...) \
    do { if (s_impl) s_impl->method(__VA_ARGS__); } while (0)

void RedrawTile(MapPoint const &pos)                  { DISPATCH_VOID(RedrawTile, pos); }
void PostProcessTile(MapPoint &pos, TileInfo *info)   { DISPATCH_VOID(PostProcessTile, pos, info); }
void TileChanged(MapPoint &pos)                       { DISPATCH_VOID(TileChanged, pos); }
void PostProcessMap()                                 { DISPATCH_VOID(PostProcessMap); }

void Refresh()                                        { DISPATCH_VOID(Refresh); }
void InvalidateMap()                                  { DISPATCH_VOID(InvalidateMap); }
void InvalidateMix()                                  { DISPATCH_VOID(InvalidateMix); }

bool TileIsVisible(sint32 mapX, sint32 mapY)          { return s_impl ? s_impl->TileIsVisible(mapX, mapY) : false; }

void CopyVision()                                     { DISPATCH_VOID(CopyVision); }

Vision const *GetLocalVision()                        { return s_impl ? s_impl->GetLocalVision() : nullptr; }

#undef DISPATCH_VOID

} // namespace tiledmap_observer

// --- TiledMap lifecycle factory (default no-op for headless) ---
// The UI build's tiledmap_observer_adapter.cpp provides a STRONG override
// of these symbols and wins the link.  This file's definitions exist so
// the headless build (which does not link the adapter) still has
// symbols to resolve against.  Headless does NOT create a TiledMap.

// Marked __attribute__((weak)) so the UI-side strong definition takes
// precedence when both translation units are linked.
__attribute__((weak))
void tiledmap_factory_recreate(sint32 /*width*/, sint32 /*height*/) {}

__attribute__((weak))
void tiledmap_factory_destroy() {}
