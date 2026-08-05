// gfx/tilesys/tiledmap_observer_adapter.cpp
// 1:1 forwarder from tiledmap_observer::Impl to tiledmap_Get().

#include "ctp/c3.h"
#include "gfx/tilesys/tiledmap_observer_adapter.h"
#include "gfx/tilesys/tiledmap.h"     // TiledMap, tiledmap_Get, tiledmap_Set

void TiledMapObserverAdapter::RedrawTile(MapPoint const &pos)
{
    if (tiledmap_Get()) tiledmap_Get()->RedrawTile(&pos);
}

void TiledMapObserverAdapter::PostProcessTile(MapPoint &pos, TileInfo *info)
{
    if (tiledmap_Get()) tiledmap_Get()->PostProcessTile(pos, info);
}

void TiledMapObserverAdapter::TileChanged(MapPoint &pos)
{
    if (tiledmap_Get()) tiledmap_Get()->TileChanged(pos);
}

void TiledMapObserverAdapter::PostProcessMap()
{
    if (tiledmap_Get()) tiledmap_Get()->PostProcessMap();
}

void TiledMapObserverAdapter::RecreateGoodActors()
{
    if (tiledmap_Get()) tiledmap_Get()->RecreateGoodActors();
}

void TiledMapObserverAdapter::Refresh()
{
    if (tiledmap_Get()) tiledmap_Get()->Refresh();
}

void TiledMapObserverAdapter::InvalidateMap()
{
    if (tiledmap_Get()) tiledmap_Get()->InvalidateMap();
}

void TiledMapObserverAdapter::InvalidateMix()
{
    if (tiledmap_Get()) tiledmap_Get()->InvalidateMix();
}

bool TiledMapObserverAdapter::TileIsVisible(sint32 mapX, sint32 mapY)
{
    return tiledmap_Get() ? tiledmap_Get()->TileIsVisible(mapX, mapY) : false;
}

void TiledMapObserverAdapter::CopyVision()
{
    if (tiledmap_Get()) tiledmap_Get()->CopyVision();
}

Vision const *TiledMapObserverAdapter::GetLocalVision()
{
    return tiledmap_Get() ? tiledmap_Get()->GetLocalVision() : nullptr;
}

namespace {
    TiledMapObserverAdapter s_tiledMapObserverAdapter;
}

void RegisterTiledMapObserverAdapter()
{
    tiledmap_observer::Register(&s_tiledMapObserverAdapter);
}

// --- TiledMap lifecycle factory (UI-side strong definitions) ---
// Overrides the weak no-op defaults in gs/core/tiledmap_observer.cpp.
// gameinit.cpp calls these instead of doing the raw new/delete itself,
// so gs/utility/ does not need to know about the TiledMap class.

void tiledmap_factory_recreate(sint32 width, sint32 height)
{
    delete tiledmap_Get();
    MapPoint size(width, height);
    tiledmap_Set(new TiledMap(size));
}

void tiledmap_factory_destroy()
{
    delete tiledmap_Get();
    tiledmap_Set(nullptr);
}
