// test/cpp/test_scoped_rand.cpp
//
// Tests for the ScopedRand fixture itself — proves the swap+restore
// contract and that civrand() resolves to the fixture's generator inside
// the scope. These tests are the foundation for migrating gs/ call sites
// off the raw g_rand pointer onto civrand(), one file at a time, with
// real fixturable tests at each step.

#include "doctest.h"
#include "ctp/c3.h"
#include "gs/utility/RandGen.h"
#include "scoped_rand.h"

// All tests in this file pivot on g_rand. Run them together so a stray
// pointer state from one test never leaks into another via doctest's
// scheduler.
TEST_SUITE_BEGIN("scoped_rand");

TEST_CASE("ScopedRand swaps g_rand for the duration of the scope")
{
    RandomGenerator * const before = g_rand;
    {
        ScopedRand fixture(12345);
        CHECK(g_rand == &fixture.get());
        CHECK(g_rand != before);
    }
    CHECK(g_rand == before);
}

TEST_CASE("ScopedRand restores g_rand even when prior pointer was null")
{
    RandomGenerator * const before = g_rand;
    g_rand = nullptr;
    {
        ScopedRand fixture(7);
        CHECK(g_rand != nullptr);
    }
    CHECK(g_rand == nullptr);
    g_rand = before;
}

TEST_CASE("civrand() resolves to the fixture's generator inside scope")
{
    ScopedRand fixture(99);
    // The seed flows through: fixture.get() and civrand() must agree on the
    // next value because they reference the same underlying RNG.
    sint32 const direct = fixture.get().Next();
    // Re-seed a fresh comparison generator to predict civrand()'s next
    // output: after one Next() call above, civrand() is offset by one.
    RandomGenerator predictor(99);
    (void)predictor.Next();
    CHECK(civrand().Next() == predictor.Next());
}

TEST_CASE("Same seed in ScopedRand produces identical sequences across runs")
{
    sint32 first[16];
    sint32 second[16];

    {
        ScopedRand fixture(424242);
        for (int i = 0; i < 16; ++i) first[i] = civrand().Next(1000);
    }
    {
        ScopedRand fixture(424242);
        for (int i = 0; i < 16; ++i) second[i] = civrand().Next(1000);
    }
    for (int i = 0; i < 16; ++i) CHECK(first[i] == second[i]);
}

TEST_CASE("Nested ScopedRand restores the outer generator on exit")
{
    ScopedRand outer(1);
    RandomGenerator * const outer_ptr = g_rand;

    {
        ScopedRand inner(2);
        CHECK(g_rand != outer_ptr);
    }

    CHECK(g_rand == outer_ptr);
}

TEST_SUITE_END();
