// test/cpp/test_pinch_detector.cpp
//
// Tests for the two-finger pinch detector (P11 pinch zoom, v1). Pure
// geometry — fingers are fed as normalized trackpad coordinates and the
// detector emits whole zoom steps. The critical property is REJECTION:
// a two-finger scroll (both fingers translating, spread ~constant) must
// never zoom, or panning the map would fight the zoom.

#include "doctest.h"
#include "ui/aui_common/pinch_detector.h"

TEST_SUITE_BEGIN("pinch_detector");

TEST_CASE("spreading fingers past the ratio threshold zooms in once")
{
    PinchDetector d;
    CHECK(d.Down(1, 0.40f, 0.50f) == 0);
    CHECK(d.Down(2, 0.60f, 0.50f) == 0);   // base spread 0.20

    // Spread symmetrically to 0.24 (1.2x): below the 1.3x step — no zoom.
    CHECK(d.Motion(1, 0.38f, 0.50f) == 0);
    CHECK(d.Motion(2, 0.62f, 0.50f) == 0);

    // Spread on to 0.28 (1.4x): one zoom-in step.
    int steps = 0;
    steps += d.Motion(1, 0.36f, 0.50f);
    steps += d.Motion(2, 0.64f, 0.50f);
    CHECK(steps == 1);
}

TEST_CASE("pinching fingers together zooms out once")
{
    PinchDetector d;
    d.Down(1, 0.30f, 0.50f);
    d.Down(2, 0.70f, 0.50f);               // base spread 0.40

    // Close to 0.28 (0.70x < 1/1.3): one zoom-out step.
    int steps = 0;
    steps += d.Motion(1, 0.36f, 0.50f);
    steps += d.Motion(2, 0.64f, 0.50f);
    CHECK(steps == -1);
}

TEST_CASE("a long spread rebases and emits multiple steps")
{
    PinchDetector d;
    d.Down(1, 0.45f, 0.50f);
    d.Down(2, 0.55f, 0.50f);               // base spread 0.10

    int total = 0;
    // Spread out to 0.50 over many small motions (5x overall > 1.3^2).
    for (int i = 1; i <= 20; ++i)
    {
        float const half = 0.05f + 0.20f * i / 20.0f;
        total += d.Motion(1, 0.5f - half, 0.50f);
        total += d.Motion(2, 0.5f + half, 0.50f);
    }
    CHECK(total >= 2);                      // stepped more than once
    CHECK(total <= 5);                      // but not on every motion event
}

TEST_CASE("two-finger scroll (translation, constant spread) never zooms")
{
    PinchDetector d;
    d.Down(1, 0.30f, 0.20f);
    d.Down(2, 0.50f, 0.20f);

    int steps = 0;
    // Drag both fingers down the pad with a little jitter in the spread —
    // the shape of a real trackpad scroll.
    for (int i = 1; i <= 30; ++i)
    {
        float const y = 0.20f + 0.02f * i;
        float const jitter = 0.004f * ((i % 3) - 1);
        steps += d.Motion(1, 0.30f - jitter, y);
        steps += d.Motion(2, 0.50f + jitter, y);
    }
    CHECK(steps == 0);
}

TEST_CASE("fingers starting nearly together are ignored")
{
    PinchDetector d;
    d.Down(1, 0.50f, 0.50f);
    d.Down(2, 0.52f, 0.50f);               // base spread 0.02 < minimum

    int steps = 0;
    steps += d.Motion(1, 0.40f, 0.50f);    // huge ratio from a tiny base
    steps += d.Motion(2, 0.62f, 0.50f);
    CHECK(steps == 0);
}

TEST_CASE("lifting a finger ends the gesture; the next pair starts fresh")
{
    PinchDetector d;
    d.Down(1, 0.40f, 0.50f);
    d.Down(2, 0.60f, 0.50f);
    d.Motion(1, 0.38f, 0.50f);              // partial spread, no step yet
    d.Up(1);

    // Fresh gesture: the old partial spread must not carry over.
    CHECK(d.Down(3, 0.40f, 0.50f) == 0);
    CHECK(d.Motion(3, 0.38f, 0.50f) == 0);  // only one finger down — inert
    CHECK(d.Down(2, 0.60f, 0.50f) == 0);
    CHECK(d.Motion(2, 0.62f, 0.50f) == 0);  // 1.09x from the NEW base
}

TEST_CASE("a third finger is ignored")
{
    PinchDetector d;
    d.Down(1, 0.40f, 0.50f);
    d.Down(2, 0.60f, 0.50f);
    CHECK(d.Down(3, 0.10f, 0.10f) == 0);

    int steps = 0;
    steps += d.Motion(3, 0.90f, 0.90f);     // wild third finger: no effect
    CHECK(steps == 0);
    steps += d.Motion(1, 0.36f, 0.50f);
    steps += d.Motion(2, 0.64f, 0.50f);     // tracked pair still works
    CHECK(steps == 1);
}

TEST_SUITE_END();
