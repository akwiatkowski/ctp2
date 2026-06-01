// test/cpp/test_installationdata.cpp
// Fast unit test for InstallationData airfield round tracking.
// Verifies that UseAirfield(round) now takes its round explicitly
// instead of reading the g_turn global — a small step in the Game
// refactor (Phase A, leaves first).

#include "ctp/c3.h"
#include "doctest.h"
#include "gs/gameobj/ID.h"
#include "gs/gameobj/installationdata.h"

TEST_CASE("InstallationData::UseAirfield records the round it was told") {
    ID id;
    id.m_id = 0;
    InstallationData inst(id);

    inst.UseAirfield(42);
    CHECK(inst.AirfieldLastUsed() == 42);

    inst.UseAirfield(100);
    CHECK(inst.AirfieldLastUsed() == 100);
}

TEST_CASE("InstallationData::UseAirfield does not reach into any global") {
    // Behavioural assertion: a freshly-constructed instance using the
    // lightweight (ID-only) ctor can have UseAirfield called on it
    // without crashing — proving that UseAirfield no longer dereferences
    // g_turn (which would be null in this test binary's context).
    ID id;
    id.m_id = 1;
    InstallationData inst(id);

    inst.UseAirfield(7);
    CHECK(inst.AirfieldLastUsed() == 7);
}
