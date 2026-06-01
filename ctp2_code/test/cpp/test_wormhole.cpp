// test/cpp/test_wormhole.cpp
// Fast unit test for the Wormhole helper types that carry an
// explicit round through the API (post-g_turn migration).
//
// The Wormhole ctor itself is too entangled with g_theWorld,
// g_theConstDB and the goodactor factory to exercise here; we
// validate the round-passing data type (EntryRecord) plus the
// public read-back accessors that don't reach into globals.

#include "ctp/c3.h"
#include "doctest.h"
#include "gs/gameobj/Wormhole.h"

TEST_CASE("EntryRecord stores the round it was constructed with") {
    Unit u;
    EntryRecord rec(u, 42);
    CHECK(rec.m_round == 42);

    EntryRecord rec2(u, 0);
    CHECK(rec2.m_round == 0);

    EntryRecord rec3(u, 9999);
    CHECK(rec3.m_round == 9999);
}

TEST_CASE("EntryRecord default ctor leaves round in a known-not-checked state") {
    // Verifying the default ctor exists and is callable — the round
    // is not initialised, so we don't assert its value.  This keeps
    // EntryRecord usable in containers that need a default-constructible
    // element type (e.g. PointerList sentinels).
    EntryRecord rec;
    (void)rec;
}
