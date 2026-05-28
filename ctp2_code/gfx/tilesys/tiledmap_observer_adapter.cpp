// gfx/tilesys/tiledmap_observer_adapter.cpp
// 1:1 forwarder from tiledmap_observer::Impl to g_tiledMap.

#include "ctp/c3.h"
#include "gfx/tilesys/tiledmap_observer_adapter.h"
#include "gfx/tilesys/tiledmap.h"     // TiledMap, g_tiledMap

void TiledMapObserverAdapter::RedrawTile(MapPoint const &pos)
{
    if (g_tiledMap) g_tiledMap->RedrawTile(&pos);
}

void TiledMapObserverAdapter::PostProcessTile(MapPoint &pos, TileInfo *info)
{
    if (g_tiledMap) g_tiledMap->PostProcessTile(pos, info);
}

void TiledMapObserverAdapter::TileChanged(MapPoint &pos)
{
    if (g_tiledMap) g_tiledMap->TileChanged(pos);
}

void TiledMapObserverAdapter::PostProcessMap()
{
    if (g_tiledMap) g_tiledMap->PostProcessMap();
}

void TiledMapObserverAdapter::Refresh()
{
    if (g_tiledMap) g_tiledMap->Refresh();
}

void TiledMapObserverAdapter::InvalidateMap()
{
    if (g_tiledMap) g_tiledMap->InvalidateMap();
}

void TiledMapObserverAdapter::InvalidateMix()
{
    if (g_tiledMap) g_tiledMap->InvalidateMix();
}

bool TiledMapObserverAdapter::TileIsVisible(sint32 mapX, sint32 mapY)
{
    return g_tiledMap ? g_tiledMap->TileIsVisible(mapX, mapY) : false;
}

void TiledMapObserverAdapter::CopyVision()
{
    if (g_tiledMap) g_tiledMap->CopyVision();
}

Vision const *TiledMapObserverAdapter::GetLocalVision()
{
    return g_tiledMap ? g_tiledMap->GetLocalVision() : nullptr;
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
    delete g_tiledMap;
    MapPoint size(width, height);
    g_tiledMap = new TiledMap(size);
}

void tiledmap_factory_destroy()
{
    delete g_tiledMap;
    g_tiledMap = nullptr;
}
