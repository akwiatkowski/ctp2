// gfx/tilesys/tiledmap_observer_adapter.h
// UI-build concrete impl of tiledmap_observer::Impl.
// Forwards every call to tiledmap_Get() (null-checked).

#pragma once

#include "gs/core/tiledmap_observer.h"

class TiledMapObserverAdapter : public tiledmap_observer::Impl
{
public:
    void RedrawTile(MapPoint const &pos) override;
    void PostProcessTile(MapPoint &pos, TileInfo *info) override;
    void TileChanged(MapPoint &pos) override;
    void PostProcessMap() override;
    void Refresh() override;
    void InvalidateMap() override;
    void InvalidateMix() override;
    bool TileIsVisible(sint32 mapX, sint32 mapY) override;
    void CopyVision() override;
    Vision const *GetLocalVision() override;
};

// Installs a singleton TiledMapObserverAdapter as the registered
// tiledmap_observer::Impl. Idempotent. Called from TiledMap's
// constructor so that any UI build that creates a TiledMap automatically
// wires up the bridge; headless never reaches this code.
void RegisterTiledMapObserverAdapter();
