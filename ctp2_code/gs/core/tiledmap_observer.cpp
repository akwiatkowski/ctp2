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

#undef DISPATCH_VOID

} // namespace tiledmap_observer
