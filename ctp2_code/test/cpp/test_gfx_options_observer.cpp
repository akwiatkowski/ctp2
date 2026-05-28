// test/cpp/test_gfx_options_observer.cpp
// Spy contract tests for gfx_options_observer fan-out interface.
//
// Verifies register/unregister, dispatch of every fan-out method onto a
// recording Impl, and safe-default returns when no Impl is registered.
// Runs without engine initialisation (< 1 ms).

#include "ctp/c3.h"
#include "doctest.h"
#include "gs/core/gfx_options_observer.h"
#include "gs/world/MapPoint.h"
#include "gs/gameobj/Army.h"

namespace {

struct RecordingSpy : gfx_options_observer::Impl {
    // AddTextToCell
    int addTextToCellCalls = 0;
    MapPoint lastCellPos;
    const char *lastCellText = nullptr;
    uint8 lastCellColor = 0;
    bool addTextToCellReturn = true;
    bool AddTextToCell(MapPoint const &pos, const char *text, uint8 colorMagnitude) override {
        ++addTextToCellCalls;
        lastCellPos = pos;
        lastCellText = text;
        lastCellColor = colorMagnitude;
        return addTextToCellReturn;
    }

    // AddTextToArmy
    int addTextToArmyCalls = 0;
    Army lastArmy;
    const char *lastArmyText = nullptr;
    uint8 lastArmyColor = 0;
    sint32 lastArmyGoalType = -999;
    bool addTextToArmyReturn = true;
    bool AddTextToArmy(Army army, const char *text, uint8 colorMagnitude, sint32 goalType) override {
        ++addTextToArmyCalls;
        lastArmy = army;
        lastArmyText = text;
        lastArmyColor = colorMagnitude;
        lastArmyGoalType = goalType;
        return addTextToArmyReturn;
    }

    // IsCellTextOn
    int isCellTextOnCalls = 0;
    bool isCellTextOnReturn = false;
    bool IsCellTextOn() override {
        ++isCellTextOnCalls;
        return isCellTextOnReturn;
    }
};

struct ScopedSpy {
    gfx_options_observer::Impl *prev;
    explicit ScopedSpy(gfx_options_observer::Impl *s) {
        prev = gfx_options_observer::Get();
        gfx_options_observer::Register(s);
    }
    ~ScopedSpy() { gfx_options_observer::Register(prev); }
};

} // anonymous namespace

TEST_CASE("gfx_options_observer::Get returns null when no Impl is registered") {
    gfx_options_observer::Impl *prev = gfx_options_observer::Get();
    gfx_options_observer::Register(nullptr);
    CHECK(gfx_options_observer::Get() == nullptr);
    gfx_options_observer::Register(prev);
}

TEST_CASE("gfx_options_observer::Register installs the Impl; Get returns it") {
    RecordingSpy spy;
    gfx_options_observer::Impl *prev = gfx_options_observer::Get();
    gfx_options_observer::Register(&spy);
    CHECK(gfx_options_observer::Get() == &spy);
    gfx_options_observer::Register(prev);
}

TEST_CASE("gfx_options_observer::AddTextToCell dispatches all three args to Impl") {
    RecordingSpy spy;
    ScopedSpy guard(&spy);
    MapPoint pt(3, 7);
    CHECK(gfx_options_observer::AddTextToCell(pt, "hello", 100) == true);
    CHECK(spy.addTextToCellCalls == 1);
    CHECK(spy.lastCellPos.x == 3);
    CHECK(spy.lastCellPos.y == 7);
    CHECK(std::strcmp(spy.lastCellText, "hello") == 0);
    CHECK(spy.lastCellColor == 100);
}

TEST_CASE("gfx_options_observer::AddTextToCell returns false when no Impl registered") {
    gfx_options_observer::Impl *prev = gfx_options_observer::Get();
    gfx_options_observer::Register(nullptr);
    MapPoint pt(1, 2);
    CHECK(gfx_options_observer::AddTextToCell(pt, "nope", 50) == false);
    gfx_options_observer::Register(prev);
}

TEST_CASE("gfx_options_observer::AddTextToArmy dispatches all four args to Impl") {
    RecordingSpy spy;
    ScopedSpy guard(&spy);
    Army army(42);
    CHECK(gfx_options_observer::AddTextToArmy(army, "army_text", 200, 7) == true);
    CHECK(spy.addTextToArmyCalls == 1);
    CHECK(spy.lastArmy == army);
    CHECK(std::strcmp(spy.lastArmyText, "army_text") == 0);
    CHECK(spy.lastArmyColor == 200);
    CHECK(spy.lastArmyGoalType == 7);
}

TEST_CASE("gfx_options_observer::AddTextToArmy with default goalType forwards -1") {
    RecordingSpy spy;
    ScopedSpy guard(&spy);
    Army army(99);
    CHECK(gfx_options_observer::AddTextToArmy(army, "default_goal", 255) == true);
    CHECK(spy.addTextToArmyCalls == 1);
    CHECK(spy.lastArmyGoalType == -1);
}

TEST_CASE("gfx_options_observer::AddTextToArmy returns false when no Impl registered") {
    gfx_options_observer::Impl *prev = gfx_options_observer::Get();
    gfx_options_observer::Register(nullptr);
    Army army(1);
    CHECK(gfx_options_observer::AddTextToArmy(army, "nope", 50) == false);
    gfx_options_observer::Register(prev);
}

TEST_CASE("gfx_options_observer::IsCellTextOn returns false when no Impl registered") {
    gfx_options_observer::Impl *prev = gfx_options_observer::Get();
    gfx_options_observer::Register(nullptr);
    CHECK(gfx_options_observer::IsCellTextOn() == false);
    gfx_options_observer::Register(prev);
}

TEST_CASE("gfx_options_observer::IsCellTextOn forwards to Impl and returns its value") {
    RecordingSpy spy;
    spy.isCellTextOnReturn = true;
    ScopedSpy guard(&spy);
    CHECK(gfx_options_observer::IsCellTextOn() == true);
    CHECK(spy.isCellTextOnCalls == 1);
}

TEST_CASE("gfx_options_observer::ScopedSpy restores previous Impl on scope exit") {
    RecordingSpy spy1;
    RecordingSpy spy2;
    gfx_options_observer::Register(&spy1);
    CHECK(gfx_options_observer::Get() == &spy1);
    {
        ScopedSpy guard(&spy2);
        CHECK(gfx_options_observer::Get() == &spy2);
    }
    CHECK(gfx_options_observer::Get() == &spy1);
    gfx_options_observer::Register(nullptr);
}
