// test/cpp/test_colorset_observer.cpp
// Spy contract tests for colorset_observer fan-out interface.
//
// Verifies register/unregister, dispatch of GetColor + GetPlayerColor onto a
// recording Impl, and safe-default (0) returns when no Impl is registered.
// Runs without engine initialisation (< 1 ms).

#include "ctp/c3.h"
#include "doctest.h"
#include "gs/core/colorset_observer.h"

namespace {

struct RecordingSpy : colorset_observer::Impl {
    // GetColor
    int getColorCalls = 0;
    COLOR lastColor = COLOR_BLACK;
    uint16 getColorReturn = 0x1234;
    uint16 GetColor(COLOR color) override {
        ++getColorCalls;
        lastColor = color;
        return getColorReturn;
    }

    // GetPlayerColor
    int getPlayerColorCalls = 0;
    sint32 lastPlayerNum = -1;
    uint16 getPlayerColorReturn = 0x5678;
    uint16 GetPlayerColor(sint32 playerNum) override {
        ++getPlayerColorCalls;
        lastPlayerNum = playerNum;
        return getPlayerColorReturn;
    }
};

struct ScopedSpy {
    colorset_observer::Impl *prev;
    explicit ScopedSpy(colorset_observer::Impl *s) {
        prev = colorset_observer::Get();
        colorset_observer::Register(s);
    }
    ~ScopedSpy() { colorset_observer::Register(prev); }
};

} // anonymous namespace

TEST_CASE("colorset_observer::Get returns null when no Impl is registered") {
    colorset_observer::Impl *prev = colorset_observer::Get();
    colorset_observer::Register(nullptr);
    CHECK(colorset_observer::Get() == nullptr);
    colorset_observer::Register(prev);
}

TEST_CASE("colorset_observer::Register installs the Impl; Get returns it") {
    RecordingSpy spy;
    colorset_observer::Impl *prev = colorset_observer::Get();
    colorset_observer::Register(&spy);
    CHECK(colorset_observer::Get() == &spy);
    colorset_observer::Register(prev);
}

TEST_CASE("colorset_observer::GetColor dispatches the COLOR arg to Impl and returns Impl's value") {
    RecordingSpy spy;
    ScopedSpy guard(&spy);
    CHECK(colorset_observer::GetColor(COLOR_RED) == 0x1234);
    CHECK(spy.getColorCalls == 1);
    CHECK(spy.lastColor == COLOR_RED);
}

TEST_CASE("colorset_observer::GetColor returns 0 when no Impl registered") {
    colorset_observer::Impl *prev = colorset_observer::Get();
    colorset_observer::Register(nullptr);
    CHECK(colorset_observer::GetColor(COLOR_BLUE) == 0);
    colorset_observer::Register(prev);
}

TEST_CASE("colorset_observer::GetPlayerColor dispatches the sint32 arg and returns Impl's value") {
    RecordingSpy spy;
    ScopedSpy guard(&spy);
    CHECK(colorset_observer::GetPlayerColor(7) == 0x5678);
    CHECK(spy.getPlayerColorCalls == 1);
    CHECK(spy.lastPlayerNum == 7);
}

TEST_CASE("colorset_observer::GetPlayerColor returns 0 when no Impl registered") {
    colorset_observer::Impl *prev = colorset_observer::Get();
    colorset_observer::Register(nullptr);
    CHECK(colorset_observer::GetPlayerColor(3) == 0);
    colorset_observer::Register(prev);
}

TEST_CASE("colorset_observer::different COLOR enum values produce different Impl calls") {
    RecordingSpy spy;
    ScopedSpy guard(&spy);
    CHECK(colorset_observer::GetColor(COLOR_RED) == 0x1234);
    CHECK(spy.lastColor == COLOR_RED);
    CHECK(colorset_observer::GetColor(COLOR_GREEN) == 0x1234);
    CHECK(spy.lastColor == COLOR_GREEN);
    CHECK(spy.getColorCalls == 2);
}

TEST_CASE("colorset_observer::ScopedSpy restores previous Impl on scope exit") {
    RecordingSpy spy1;
    RecordingSpy spy2;
    colorset_observer::Register(&spy1);
    CHECK(colorset_observer::Get() == &spy1);
    {
        ScopedSpy guard(&spy2);
        CHECK(colorset_observer::Get() == &spy2);
    }
    CHECK(colorset_observer::Get() == &spy1);
    colorset_observer::Register(nullptr);
}

TEST_CASE("colorset_observer::GetColor called for every COLOR_BLACK..COLOR_GRAY counts all distinct calls") {
    RecordingSpy spy;
    ScopedSpy guard(&spy);
    int expected = COLOR_GRAY - COLOR_BLACK + 1;
    for (int c = COLOR_BLACK; c <= COLOR_GRAY; ++c) {
        colorset_observer::GetColor(static_cast<COLOR>(c));
    }
    CHECK(spy.getColorCalls == expected);
    CHECK(expected >= 12);
}

TEST_CASE("colorset_observer::GetPlayerColor forwards player 0 to Impl") {
    RecordingSpy spy;
    ScopedSpy guard(&spy);
    colorset_observer::GetPlayerColor(0);
    CHECK(spy.getPlayerColorCalls == 1);
    CHECK(spy.lastPlayerNum == 0);
}

TEST_CASE("colorset_observer::GetPlayerColor forwards player 31 to Impl") {
    RecordingSpy spy;
    ScopedSpy guard(&spy);
    colorset_observer::GetPlayerColor(31);
    CHECK(spy.getPlayerColorCalls == 1);
    CHECK(spy.lastPlayerNum == 31);
}

TEST_CASE("colorset_observer::GetPlayerColor with player -1 does not crash and forwards as-is") {
    RecordingSpy spy;
    ScopedSpy guard(&spy);
    colorset_observer::GetPlayerColor(-1);
    CHECK(spy.getPlayerColorCalls == 1);
    CHECK(spy.lastPlayerNum == -1);
}

TEST_CASE("colorset_observer::nested ScopedSpy restores outer Impl when inner is dropped") {
    RecordingSpy spyA;
    RecordingSpy spyB;
    colorset_observer::Impl *original = colorset_observer::Get();
    {
        ScopedSpy outer(&spyA);
        CHECK(colorset_observer::Get() == &spyA);
        {
            ScopedSpy inner(&spyB);
            CHECK(colorset_observer::Get() == &spyB);
        }
        CHECK(colorset_observer::Get() == &spyA);
    }
    CHECK(colorset_observer::Get() == original);
}

TEST_CASE("colorset_observer::Spy return value 0xABCD round-trips through fan-out") {
    RecordingSpy spy;
    spy.getColorReturn = 0xABCD;
    spy.getPlayerColorReturn = 0xABCD;
    ScopedSpy guard(&spy);
    CHECK(colorset_observer::GetColor(COLOR_RED) == 0xABCD);
    CHECK(colorset_observer::GetPlayerColor(5) == 0xABCD);
}
