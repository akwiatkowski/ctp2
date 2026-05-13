// test/cpp/test_randgen.cpp
// Unit tests for RandomGenerator — the simplest game class with zero global
// dependencies. Serves as a smoke test for the harness itself.

#include "doctest.h"
#include "ctp/c3.h"                     // Assert, sint32, and other base types
#include "gs/utility/RandGen.h"

TEST_CASE("RandomGenerator produces identical sequence with same seed")
{
    RandomGenerator r1(static_cast<sint32>(42));
    RandomGenerator r2(static_cast<sint32>(42));

    for (int i = 0; i < 100; ++i)
    {
        CHECK(r1.Next() == r2.Next());
    }
}

TEST_CASE("RandomGenerator produces different sequence with different seeds")
{
    RandomGenerator r1(static_cast<sint32>(42));
    RandomGenerator r2(static_cast<sint32>(43));

    // It's vanishingly unlikely that two different seeds produce the same
    // first value. We check the first 10 values to be safe.
    bool any_different = false;
    for (int i = 0; i < 10; ++i)
    {
        if (r1.Next() != r2.Next())
        {
            any_different = true;
            break;
        }
    }
    CHECK(any_different);
}

TEST_CASE("RandomGenerator::Next(int) stays within bounds")
{
    RandomGenerator r(static_cast<sint32>(12345));

    for (sint32 bound : {1, 2, 5, 10, 100, 1000, 1000000})
    {
        for (int i = 0; i < 50; ++i)
        {
            sint32 val = r.Next(bound);
            CHECK(val >= 0);
            CHECK(val < bound);
        }
    }
}

TEST_CASE("RandomGenerator::NextF() returns values in [0, 1)")
{
    RandomGenerator r(static_cast<sint32>(99999));

    for (int i = 0; i < 1000; ++i)
    {
        double val = r.NextF();
        CHECK(val >= 0.0);
        CHECK(val < 1.0);
    }
}

TEST_CASE("RandomGenerator::GetSeed() returns the original seed")
{
    RandomGenerator r(static_cast<sint32>(314159));
    CHECK(r.GetSeed() == 314159);
}
