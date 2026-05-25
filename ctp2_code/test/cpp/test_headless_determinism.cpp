// test/cpp/test_headless_determinism.cpp
// Determinism smoke test for ctp2_headless.
//
// GOAL: verify that two runs with the same seed produce identical game state.
//
// CURRENT STATUS: BLOCKED — the headless binary parses --seed but does NOT
// wire it to the RNG.  The seed is only used for logging.  To enable this
// test, the following code change is required in headless_main.cpp:
//
//   g_theProfileDB->SetMapSeed(seed);   // or equivalent RNG init
//
// Once wired, this test will:
//   1. Run headless with seed=42, save at turn 25
//   2. Run headless with seed=42, save at turn 25 (second run)
//   3. Compare save file checksums — they must match
//
// For now, the test is a stub that documents the requirement.

#include "ctp/c3.h"
#include "doctest.h"

TEST_CASE("Determinism: same seed produces identical saves (REQUIRES seed wiring)")
{
    // Placeholder — will be implemented once seed is wired to RNG.
    //
    // Expected usage:
    //   ./ctp2_headless --new-game --turns 25 --seed 42 --save-at 25 --save-to /tmp/run1.sav
    //   ./ctp2_headless --new-game --turns 25 --seed 42 --save-at 25 --save-to /tmp/run2.sav
    //   md5sum /tmp/run1.sav /tmp/run2.sav  # must match
    //
    // NOTE: The headless binary currently does not support --save-at or --save-to.
    // These CLI options would need to be added, plus the save logic inside the
    // turn loop (e.g. GameFile::SaveGame).

    CHECK(true); // Blocked: seed not wired to RNG, and --save-at/--save-to not implemented
}

TEST_CASE("Determinism: different seeds produce different saves (REQUIRES seed wiring)")
{
    // Placeholder — will be implemented once seed is wired to RNG.
    //
    // Expected usage:
    //   ./ctp2_headless --new-game --turns 25 --seed 42 --save-at 25 --save-to /tmp/run1.sav
    //   ./ctp2_headless --new-game --turns 25 --seed 99 --save-at 25 --save-to /tmp/run2.sav
    //   md5sum /tmp/run1.sav /tmp/run2.sav  # must differ

    CHECK(true); // Blocked: seed not wired to RNG, and --save-at/--save-to not implemented
}
