// test/cpp/test_tiledmap_observer.cpp
// Unit tests for tiledmap_observer dispatch + null-safety.
//
// These tests exercise the observer dispatch logic without needing the full
// game engine (no database init, no SDL, no graphics).  They run in < 1 ms.

#include "ctp/c3.h"
#include "doctest.h"
#include "gs/core/tiledmap_observer.h"
#include "gs/world/MapPoint.h"

// Forward-declare TileInfo; the test only passes nullptr.
class TileInfo;

namespace {

struct RecordingSpy : tiledmap_observer::Impl {
    int redrawTileCalls = 0;
    MapPoint lastRedrawPos;
    void RedrawTile(MapPoint const &pos) override {
        ++redrawTileCalls;
        lastRedrawPos = pos;
    }

    int postProcessTileCalls = 0;
    MapPoint lastPostProcessPos;
    TileInfo *lastPostProcessInfo = nullptr;
    void PostProcessTile(MapPoint &pos, TileInfo *info) override {
        ++postProcessTileCalls;
        lastPostProcessPos = pos;
        lastPostProcessInfo = info;
    }

    int tileChangedCalls = 0;
    MapPoint lastTileChangedPos;
    void TileChanged(MapPoint &pos) override {
        ++tileChangedCalls;
        lastTileChangedPos = pos;
    }

    int postProcessMapCalls = 0;
    void PostProcessMap() override {
        ++postProcessMapCalls;
    }

    int recreateGoodActorsCalls = 0;
    void RecreateGoodActors() override {
        ++recreateGoodActorsCalls;
    }

    int refreshCalls = 0;
    void Refresh() override {
        ++refreshCalls;
    }

    int invalidateMapCalls = 0;
    void InvalidateMap() override {
        ++invalidateMapCalls;
    }

    int invalidateMixCalls = 0;
    void InvalidateMix() override {
        ++invalidateMixCalls;
    }

    int tileIsVisibleCalls = 0;
    sint32 lastTileIsVisibleX = -1;
    sint32 lastTileIsVisibleY = -1;
    bool tileIsVisibleResult = false;
    bool TileIsVisible(sint32 mapX, sint32 mapY) override {
        ++tileIsVisibleCalls;
        lastTileIsVisibleX = mapX;
        lastTileIsVisibleY = mapY;
        return tileIsVisibleResult;
    }

    int copyVisionCalls = 0;
    void CopyVision() override {
        ++copyVisionCalls;
    }

    int getLocalVisionCalls = 0;
    Vision const *getLocalVisionReturn = nullptr;
    Vision const *GetLocalVision() override {
        ++getLocalVisionCalls;
        return getLocalVisionReturn;
    }
};

// RAII helper that restores the previous Impl on destruction.
// Without this, a test that forgets to unregister pollutes other tests.
struct ScopedSpy {
    tiledmap_observer::Impl *prev;
    explicit ScopedSpy(tiledmap_observer::Impl *s) {
        prev = tiledmap_observer::Get();
        tiledmap_observer::Register(s);
    }
    ~ScopedSpy() { tiledmap_observer::Register(prev); }
};

} // anonymous namespace

TEST_CASE("tiledmap_observer::Get returns null when no Impl is registered")
{
    tiledmap_observer::Impl *prev = tiledmap_observer::Get();
    tiledmap_observer::Register(nullptr);
    CHECK(tiledmap_observer::Get() == nullptr);
    tiledmap_observer::Register(prev);  // restore
}

TEST_CASE("tiledmap_observer::Register installs the Impl and Get returns it")
{
    RecordingSpy spy;
    {
        ScopedSpy guard(&spy);
        CHECK(tiledmap_observer::Get() == &spy);
    }
}

TEST_CASE("tiledmap_observer::RedrawTile dispatches to Impl")
{
    RecordingSpy spy;
    ScopedSpy guard(&spy);
    MapPoint pt(3, 7);
    tiledmap_observer::RedrawTile(pt);
    CHECK(spy.redrawTileCalls == 1);
    CHECK(spy.lastRedrawPos.x == 3);
    CHECK(spy.lastRedrawPos.y == 7);
}

TEST_CASE("tiledmap_observer::PostProcessTile dispatches to Impl")
{
    RecordingSpy spy;
    ScopedSpy guard(&spy);
    MapPoint pt(5, 11);
    tiledmap_observer::PostProcessTile(pt, nullptr);
    CHECK(spy.postProcessTileCalls == 1);
    CHECK(spy.lastPostProcessPos.x == 5);
    CHECK(spy.lastPostProcessPos.y == 11);
    CHECK(spy.lastPostProcessInfo == nullptr);
}

TEST_CASE("tiledmap_observer::TileChanged dispatches to Impl")
{
    RecordingSpy spy;
    ScopedSpy guard(&spy);
    MapPoint pt(13, 2);
    tiledmap_observer::TileChanged(pt);
    CHECK(spy.tileChangedCalls == 1);
    CHECK(spy.lastTileChangedPos.x == 13);
    CHECK(spy.lastTileChangedPos.y == 2);
}

TEST_CASE("tiledmap_observer::PostProcessMap dispatches to Impl")
{
    RecordingSpy spy;
    ScopedSpy guard(&spy);
    tiledmap_observer::PostProcessMap();
    CHECK(spy.postProcessMapCalls == 1);
}

TEST_CASE("tiledmap_observer::RecreateGoodActors dispatches to Impl")
{
    RecordingSpy spy;
    ScopedSpy guard(&spy);
    tiledmap_observer::RecreateGoodActors();
    CHECK(spy.recreateGoodActorsCalls == 1);
    // Distinct from PostProcessMap on purpose: that one regenerates tile
    // numbers and would undo part of what a load just restored.
    CHECK(spy.postProcessMapCalls == 0);
}

TEST_CASE("tiledmap_observer::Refresh dispatches to Impl")
{
    RecordingSpy spy;
    ScopedSpy guard(&spy);
    tiledmap_observer::Refresh();
    CHECK(spy.refreshCalls == 1);
}

TEST_CASE("tiledmap_observer::InvalidateMap dispatches to Impl")
{
    RecordingSpy spy;
    ScopedSpy guard(&spy);
    tiledmap_observer::InvalidateMap();
    CHECK(spy.invalidateMapCalls == 1);
}

TEST_CASE("tiledmap_observer::InvalidateMix dispatches to Impl")
{
    RecordingSpy spy;
    ScopedSpy guard(&spy);
    tiledmap_observer::InvalidateMix();
    CHECK(spy.invalidateMixCalls == 1);
}

TEST_CASE("tiledmap_observer::CopyVision dispatches to Impl")
{
    RecordingSpy spy;
    ScopedSpy guard(&spy);
    tiledmap_observer::CopyVision();
    CHECK(spy.copyVisionCalls == 1);
}

TEST_CASE("tiledmap_observer::TileIsVisible returns false when no Impl registered")
{
    tiledmap_observer::Impl *prev = tiledmap_observer::Get();
    tiledmap_observer::Register(nullptr);
    CHECK(!tiledmap_observer::TileIsVisible(0, 0));
    tiledmap_observer::Register(prev);  // restore
}

TEST_CASE("tiledmap_observer::TileIsVisible forwards to Impl and returns its result")
{
    RecordingSpy spy;
    spy.tileIsVisibleResult = true;
    ScopedSpy guard(&spy);
    CHECK(tiledmap_observer::TileIsVisible(42, 99) == true);
    CHECK(spy.tileIsVisibleCalls == 1);
    CHECK(spy.lastTileIsVisibleX == 42);
    CHECK(spy.lastTileIsVisibleY == 99);
}

TEST_CASE("Null fan-outs are no-ops when no Impl registered")
{
    tiledmap_observer::Impl *prev = tiledmap_observer::Get();
    tiledmap_observer::Register(nullptr);

    MapPoint pt(1, 2);
    tiledmap_observer::RedrawTile(pt);
    tiledmap_observer::PostProcessTile(pt, nullptr);
    tiledmap_observer::TileChanged(pt);
    tiledmap_observer::PostProcessMap();
    tiledmap_observer::RecreateGoodActors();
    tiledmap_observer::Refresh();
    tiledmap_observer::InvalidateMap();
    tiledmap_observer::InvalidateMix();
    tiledmap_observer::CopyVision();

    // If we reached this point without crashing, the test passes.
    CHECK(true);

    tiledmap_observer::Register(prev);  // restore
}

TEST_CASE("tiledmap_observer::RedrawTile accumulates across 10 dispatches")
{
    RecordingSpy spy;
    ScopedSpy guard(&spy);
    MapPoint pt(1, 2);
    for (int i = 0; i < 10; ++i) {
        tiledmap_observer::RedrawTile(pt);
    }
    CHECK(spy.redrawTileCalls == 10);
}

TEST_CASE("tiledmap_observer::RedrawTile forwards negative MapPoint coords")
{
    RecordingSpy spy;
    ScopedSpy guard(&spy);
    MapPoint pt(-1, -1);
    tiledmap_observer::RedrawTile(pt);
    CHECK(spy.redrawTileCalls == 1);
    CHECK(spy.lastRedrawPos.x == -1);
    CHECK(spy.lastRedrawPos.y == -1);
}

TEST_CASE("tiledmap_observer::Refresh, InvalidateMap, PostProcessMap in sequence")
{
    RecordingSpy spy;
    ScopedSpy guard(&spy);
    tiledmap_observer::Refresh();
    tiledmap_observer::InvalidateMap();
    tiledmap_observer::PostProcessMap();
    CHECK(spy.refreshCalls == 1);
    CHECK(spy.invalidateMapCalls == 1);
    CHECK(spy.postProcessMapCalls == 1);
}

TEST_CASE("tiledmap_observer::GetLocalVision returns nullptr when no Impl is registered")
{
    tiledmap_observer::Impl *prev = tiledmap_observer::Get();
    tiledmap_observer::Register(nullptr);
    CHECK(tiledmap_observer::GetLocalVision() == nullptr);
    tiledmap_observer::Register(prev);  // restore
}

TEST_CASE("tiledmap_observer::GetLocalVision forwards the Impl's pointer when registered")
{
    RecordingSpy spy;
    // Vision is an incomplete type in this TU; use a buffer for pointer
    // round-trip testing (never dereferenced).
    static int dummy_storage;
    Vision const *dummy = reinterpret_cast<Vision const *>(&dummy_storage);
    spy.getLocalVisionReturn = dummy;
    ScopedSpy guard(&spy);
    CHECK(tiledmap_observer::GetLocalVision() == dummy);
    CHECK(spy.getLocalVisionCalls == 1);
}

TEST_CASE("tiledmap_observer::TileIsVisible with extreme args doesn't crash")
{
    RecordingSpy spy;
    ScopedSpy guard(&spy);
    // INT32_MIN and INT32_MAX should be forwarded without crashing.
    CHECK(!tiledmap_observer::TileIsVisible(INT32_MIN, INT32_MAX));
    CHECK(spy.tileIsVisibleCalls == 1);
    CHECK(spy.lastTileIsVisibleX == INT32_MIN);
    CHECK(spy.lastTileIsVisibleY == INT32_MAX);
}

TEST_CASE("tiledmap_observer::PostProcessTile, TileChanged, RedrawTile in migration sequence")
{
    RecordingSpy spy;
    ScopedSpy guard(&spy);
    MapPoint pt(7, 9);
    tiledmap_observer::PostProcessTile(pt, nullptr);
    tiledmap_observer::TileChanged(pt);
    tiledmap_observer::RedrawTile(pt);
    CHECK(spy.postProcessTileCalls == 1);
    CHECK(spy.tileChangedCalls == 1);
    CHECK(spy.redrawTileCalls == 1);
    CHECK(spy.lastPostProcessPos.x == 7);
    CHECK(spy.lastPostProcessPos.y == 9);
    CHECK(spy.lastTileChangedPos.x == 7);
    CHECK(spy.lastTileChangedPos.y == 9);
    CHECK(spy.lastRedrawPos.x == 7);
    CHECK(spy.lastRedrawPos.y == 9);
}
