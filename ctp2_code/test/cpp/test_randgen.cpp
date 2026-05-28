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

TEST_CASE("RandomGenerator with seed 0 is deterministic")
{
    RandomGenerator r1(static_cast<sint32>(0));
    RandomGenerator r2(static_cast<sint32>(0));

    for (int i = 0; i < 100; ++i)
    {
        CHECK(r1.Next() == r2.Next());
    }
}

TEST_CASE("RandomGenerator with a very large seed is deterministic")
{
    RandomGenerator r1(static_cast<sint32>(0xDEADBEEF));
    RandomGenerator r2(static_cast<sint32>(0xDEADBEEF));

    for (int i = 0; i < 100; ++i)
    {
        CHECK(r1.Next() == r2.Next());
    }
}

// NOTE: Next(0) triggers Assert(0 < r) inside RandomGenerator::Next(sint32).
// In debug builds this fires an assertion dialog; in release builds it returns 0.
// We deliberately do NOT test this edge here to avoid crashing the test runner.
// The contract is: caller must pass a positive bound.

TEST_CASE("RandomGenerator::Next(1) always returns 0")
{
    RandomGenerator r(static_cast<sint32>(77777));

    for (int i = 0; i < 100; ++i)
    {
        CHECK(r.Next(1) == 0);
    }
}

TEST_CASE("RandomGenerator::NextF() across 10000 samples stays in [0, 1)")
{
    RandomGenerator r(static_cast<sint32>(55555));

    for (int i = 0; i < 10000; ++i)
    {
        double val = r.NextF();
        CHECK(val >= 0.0);
        CHECK(val < 1.0);
    }
}

TEST_CASE("RandomGenerator::NextF() distribution sanity check")
{
    RandomGenerator r(static_cast<sint32>(44444));

    int above_half = 0;
    int below_half = 0;

    for (int i = 0; i < 10000; ++i)
    {
        double val = r.NextF();
        if (val > 0.5)
            ++above_half;
        if (val < 0.5)
            ++below_half;
    }

    CHECK(above_half >= 100);
    CHECK(below_half >= 100);
}

TEST_CASE("Two RandomGenerators with same seed produce identical Next(int) sequences")
{
    RandomGenerator r1(static_cast<sint32>(123456));
    RandomGenerator r2(static_cast<sint32>(123456));
    RandomGenerator bound_gen(static_cast<sint32>(999999));

    for (int i = 0; i < 100; ++i)
    {
        sint32 bound = 1 + (bound_gen.Next() % 1000); // bound in [1,1000]
        CHECK(r1.Next(bound) == r2.Next(bound));
    }
}

TEST_CASE("Calling GetSeed() does not perturb the sequence")
{
    RandomGenerator r1(static_cast<sint32>(987654));
    RandomGenerator r2(static_cast<sint32>(987654));

    for (int i = 0; i < 100; ++i)
    {
        (void)r1.GetSeed(); // read seed mid-stream
        CHECK(r1.Next() == r2.Next());
    }
}

TEST_CASE("RandomGenerator::Initialize() resets the sequence mid-stream")
{
    RandomGenerator r1(static_cast<sint32>(111222));
    RandomGenerator r2(static_cast<sint32>(111222));

    // Advance both 50 steps
    for (int i = 0; i < 50; ++i)
    {
        r1.Next();
        r2.Next();
    }

    // Re-seed r1 back to the original seed
    r1.Initialize(static_cast<sint32>(111222));

    // r1 (reset) should now match a fresh generator with the same seed
    RandomGenerator r3(static_cast<sint32>(111222));
    for (int i = 0; i < 100; ++i)
    {
        CHECK(r1.Next() == r3.Next());
    }
}
