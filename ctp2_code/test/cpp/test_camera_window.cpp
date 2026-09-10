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

TEST_CASE("screen<->texture round-trips exactly at every zoom and offset")
{
    // This IS the picking oracle. The forward direction is already known good
    // (terrain renders in the right place, and the whole-map origin was
    // measured to agree with the legacy path), so an exact inverse means
    // picking is correct -- no screenshot comparison needed.
    float const origins[] = { 0.0f, 94.0f, 2256.0f };
    float const zooms[]   = { 0.21f, 0.5f, 1.0f, 1.5f, 2.0f };
    float const offs[]    = { -120.0f, 0.0f, 37.5f };
    float const points[]  = { 0.0f, 1.0f, 640.0f, 959.5f, 1919.0f };
    for (float o : origins)
        for (float z : zooms)
            for (float c : offs)
                for (float p : points)
                {
                    CAPTURE(o); CAPTURE(z); CAPTURE(c); CAPTURE(p);
                    float const tex = camera_window::ScreenToTexture(p, k_W, o, c, z);
                    float const back = camera_window::TextureToScreen(tex, k_W, o, c, z);
                    CHECK(back == doctest::Approx(p).epsilon(1e-4));
                }
}

TEST_CASE("at identity the screen maps onto the texture at the origin")
{
    // zoom 1, no pan: screen x lands at origin + x, which is what the
    // window-mirror path did with origin = WorldContentOffX.
    CHECK(camera_window::ScreenToTexture(0.0f, k_W, 94.0f, 0.0f, 1.0f) == doctest::Approx(94.0f));
    CHECK(camera_window::ScreenToTexture(100.0f, k_W, 94.0f, 0.0f, 1.0f) == doctest::Approx(194.0f));
    // Panning right moves the sampled texture window left by the same pixels.
    CHECK(camera_window::ScreenToTexture(0.0f, k_W, 94.0f, 30.0f, 1.0f) == doctest::Approx(64.0f));
}

TEST_CASE("zooming in samples a smaller texture span across the same screen")
{
    // At 2x the screen covers half as much texture, centred: the span is W/2.
    float const left  = camera_window::ScreenToTexture(0.0f,  k_W, 0.0f, 0.0f, 2.0f);
    float const right = camera_window::ScreenToTexture(k_W,   k_W, 0.0f, 0.0f, 2.0f);
    CHECK((right - left) == doctest::Approx(k_W / 2.0f));
    // ...and at 0.5x it covers twice as much.
    float const l2 = camera_window::ScreenToTexture(0.0f, k_W, 0.0f, 0.0f, 0.5f);
    float const r2 = camera_window::ScreenToTexture(k_W,  k_W, 0.0f, 0.0f, 0.5f);
    CHECK((r2 - l2) == doctest::Approx(k_W * 2.0f));
}

TEST_CASE("whole-map pan is limited by the texture edge, not a margin")
{
    // 1920-wide viewport over a 4606-wide map texture, screen origin at 0.
    float const texW = 4606.0f;
    // At zoom 1 the window is 1920 wide, so the offset may run from
    // 1920-4606 = -2686 (right edge) up to 0 (left edge).
    CHECK(camera_window::PanLimitHigh(k_W, 0.0f, 1.0f) == doctest::Approx(0.0f));
    CHECK(camera_window::PanLimitLow(k_W, texW, 0.0f, 1.0f) == doctest::Approx(k_W - texW));

    // Clamping keeps an in-range offset untouched and pulls outliers to the edge.
    CHECK(camera_window::ClampPan(-1000.0f, k_W, texW, 0.0f, 1.0f) == doctest::Approx(-1000.0f));
    CHECK(camera_window::ClampPan(500.0f,   k_W, texW, 0.0f, 1.0f) == doctest::Approx(0.0f));
    CHECK(camera_window::ClampPan(-9999.0f, k_W, texW, 0.0f, 1.0f) == doctest::Approx(k_W - texW));

    // The pannable span is texture minus visible span, and it SHRINKS as you
    // zoom out (more map visible = less left to pan to).
    float const span1 = camera_window::PanLimitHigh(k_W, 0.0f, 1.0f)
                      - camera_window::PanLimitLow(k_W, texW, 0.0f, 1.0f);
    float const span05 = camera_window::PanLimitHigh(k_W, 0.0f, 0.5f)
                       - camera_window::PanLimitLow(k_W, texW, 0.0f, 0.5f);
    CHECK(span1 == doctest::Approx(texW - k_W));
    CHECK(span05 == doctest::Approx(texW - k_W * 2.0f));
    CHECK(span05 < span1);

    // Zoomed out past fit there is nothing to pan: both limits collapse and any
    // offset resolves to the same centred value.
    float const a = camera_window::ClampPan(-5000.0f, k_W, texW, 0.0f, 0.2f);
    float const b = camera_window::ClampPan( 5000.0f, k_W, texW, 0.0f, 0.2f);
    CHECK(a == doctest::Approx(b));
}

TEST_SUITE_END();
