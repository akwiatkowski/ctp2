// test/cpp/test_camera_window.cpp
//
// Tests for the camera window geometry (P13 step 0). Pure arithmetic: given a
// screen size, the rendered content margin around it, and a camera zoom, how
// far may the camera pan before the present samples outside valid content?
//
// This exists because the answer was previously assumed to be "the margin",
// independent of zoom. It is not. The present builds a source rect of W/z
// pixels centred on the screen region, so at z < 1 it reaches
// W*(1-z)/(2z) px beyond each side BEFORE any panning. SDL clips an
// out-of-bounds srcrect and rescales the destination proportionally, so the
// texture-to-screen mapping changes discontinuously and the map visibly
// teleports.
//
// The expected values below were derived from the geometry by hand, not read
// off the implementation.

#include "doctest.h"
#include "ui/aui_common/camera_window.h"

TEST_SUITE_BEGIN("camera_window");

// The shipping layout: 1920x1080 screen, world content = the background window
// mirror, so the screen's top-left sits 94/72 px inside it (k_TILE_GRID_*).
static constexpr float k_W = 1920.0f;
static constexpr float k_H = 1080.0f;
static constexpr float k_MarginX = 94.0f;
static constexpr float k_MarginY = 72.0f;

TEST_CASE("at identity zoom the whole margin is available to pan")
{
    CHECK(camera_window::OverscanPx(k_W, 1.0f) == doctest::Approx(0.0f));
    CHECK(camera_window::PanBudgetPx(k_W, k_MarginX, 1.0f) == doctest::Approx(94.0f));
    CHECK(camera_window::PanBudgetPx(k_H, k_MarginY, 1.0f) == doctest::Approx(72.0f));
}

TEST_CASE("zooming in past 1.0 costs no margin (the window shrinks)")
{
    // z > 1 makes the source rect SMALLER than the screen region, so it never
    // reaches past the screen edge; the full margin stays available.
    CHECK(camera_window::OverscanPx(k_W, 1.08f) == doctest::Approx(0.0f));
    CHECK(camera_window::PanBudgetPx(k_W, k_MarginX, 1.08f) == doctest::Approx(94.0f));
}

TEST_CASE("the last engine zoom-in step leaves far less budget than the margin")
{
    // ZoomIn from scale 0.92631 to 1.0 sets camera z = oldScale/newScale.
    // Overscan = 1920 * (1 - 0.92631) / (2 * 0.92631) = 76.4px, so only
    // 94 - 76.4 = 17.6px of the 94px margin survives. The old code assumed 88.
    float const z = 0.92631f / 1.0f;
    CHECK(camera_window::OverscanPx(k_W, z) == doctest::Approx(76.39f).epsilon(0.01));
    CHECK(camera_window::PanBudgetPx(k_W, k_MarginX, z) == doctest::Approx(17.61f).epsilon(0.01));
}

TEST_CASE("the other four engine zoom-in steps overrun the margin entirely")
{
    // Every remaining ZoomIn step needs more overscan than the 94px margin can
    // supply, so NO pan offset is safe — and even offset 0 samples outside
    // valid content. Budget must clamp to zero rather than go negative.
    struct Step { float from, to, overscanX; };
    Step const steps[] = {
        { 0.50526f, 0.58947f, 160.0f },
        { 0.58947f, 0.71578f, 205.7f },
        { 0.71578f, 0.80000f, 113.0f },
        { 0.80000f, 0.92631f, 151.6f },
    };
    for (Step const &s : steps)
    {
        float const z = s.from / s.to;
        CAPTURE(z);
        CHECK(camera_window::OverscanPx(k_W, z) == doctest::Approx(s.overscanX).epsilon(0.01));
        CHECK(camera_window::OverscanPx(k_W, z) > k_MarginX);
        CHECK(camera_window::PanBudgetPx(k_W, k_MarginX, z) == doctest::Approx(0.0f));
    }
}

TEST_CASE("MinSafeZoom is the zoom below which even a centred window overruns")
{
    // Solve overscan(z) == margin: W(1-z)/(2z) = m  ->  z = W / (W + 2m).
    CHECK(camera_window::MinSafeZoom(k_W, k_MarginX) == doctest::Approx(1920.0f / 2108.0f));
    CHECK(camera_window::MinSafeZoom(k_H, k_MarginY) == doctest::Approx(1080.0f / 1224.0f));

    // At exactly MinSafeZoom the budget is zero, and just above it is positive.
    // Compared with an absolute bound: at the boundary the budget is the
    // difference of two ~94px quantities, so a relative epsilon around 0 would
    // be tighter than the float arithmetic can honour.
    float const zx = camera_window::MinSafeZoom(k_W, k_MarginX);
    CHECK(camera_window::PanBudgetPx(k_W, k_MarginX, zx) < 0.01f);
    CHECK(camera_window::PanBudgetPx(k_W, k_MarginX, zx + 0.01f) > 0.0f);

    // The binding constraint across both axes is the larger of the two.
    CHECK(camera_window::MinSafeZoom(k_W, k_MarginX)
          > camera_window::MinSafeZoom(k_H, k_MarginY));
}

TEST_CASE("degenerate zooms do not produce garbage budgets")
{
    // A zero or negative zoom would divide by zero in the overscan formula.
    CHECK(camera_window::OverscanPx(k_W, 0.0f) == doctest::Approx(0.0f));
    CHECK(camera_window::OverscanPx(k_W, -1.0f) == doctest::Approx(0.0f));
    // A zero-width screen has no overscan and no budget beyond its margin.
    CHECK(camera_window::PanBudgetPx(0.0f, k_MarginX, 0.5f) == doctest::Approx(94.0f));
    // Budget never goes negative, whatever the inputs.
    CHECK(camera_window::PanBudgetPx(k_W, 0.0f, 0.5f) == doctest::Approx(0.0f));
}

TEST_CASE("the 100% detent snaps only inside its band")
{
    // Inside the band, from either side, land exactly on 1.0.
    CHECK(camera_window::ZoomDetent(1.00f) == doctest::Approx(1.0f));
    CHECK(camera_window::ZoomDetent(1.03f) == doctest::Approx(1.0f));
    CHECK(camera_window::ZoomDetent(0.97f) == doctest::Approx(1.0f));
    CHECK(camera_window::ZoomDetent(1.04f) == doctest::Approx(1.0f));   // boundary is inclusive

    // Outside it, the zoom is left alone -- the detent must not drag the whole
    // range toward 1.0 or intermediate zooms become unreachable.
    CHECK(camera_window::ZoomDetent(1.05f) == doctest::Approx(1.05f));
    CHECK(camera_window::ZoomDetent(0.90f) == doctest::Approx(0.90f));
    CHECK(camera_window::ZoomDetent(2.00f) == doctest::Approx(2.0f));
    CHECK(camera_window::ZoomDetent(0.21f) == doctest::Approx(0.21f));

    // A wider band widens the catch symmetrically.
    CHECK(camera_window::ZoomDetent(1.10f, 0.20f) == doctest::Approx(1.0f));
    CHECK(camera_window::ZoomDetent(0.90f, 0.20f) == doctest::Approx(1.0f));
}

TEST_SUITE_END();
