//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ header
// Description  : Camera window geometry — how far the GPU camera may pan at a
//                given zoom before it samples outside rendered content
//                (P13 step 0)
//
//----------------------------------------------------------------------------
//
// Pure arithmetic, no SDL types, so it can be unit-tested directly.
//
// The present (aui_SDLSurface::Flip) shows the world by windowing a source
// rect out of the oversized world texture:
//
//     srcW = W / z
//     srcX = baseX + (W - srcW) / 2 - offX
//
// At zoom z the source rect is W/z wide but is still centred on the screen
// region, so it reaches
//
//     overscan = W * (1 - z) / (2z)
//
// pixels beyond EACH side of that region before any panning happens. Only what
// is left of the rendered margin after paying that cost is available to pan
// into.
//
// This matters because it was previously assumed the whole margin was
// available regardless of zoom. It is not, and the failure is not graceful:
// SDL clips an out-of-bounds srcrect to the texture and rescales the
// destination proportionally, so the texture-to-screen mapping changes
// discontinuously and the map visibly teleports.
//
// Sign conventions: z > 1 (zoomed in) shrinks the source rect, so it never
// reaches past the screen edge and overscan is zero — the full margin remains.
// z < 1 (zoomed out) is the direction that costs margin. Note that the engine's
// ZoomIn() sets a camera z of oldScale/newScale, which is BELOW 1 — so zooming
// the map in is what shrinks this budget, not zooming out.
//
//----------------------------------------------------------------------------

#if defined(HAVE_PRAGMA_ONCE)
#pragma once
#endif

#ifndef CAMERA_WINDOW_H_
#define CAMERA_WINDOW_H_

namespace camera_window
{

// How far past each side of the screen region the source rect reaches at this
// zoom, in texture pixels. Zero when zoomed in (z >= 1) or when z is
// degenerate — a non-positive zoom has no meaningful window, and returning 0
// keeps callers on the conservative "assume no overscan" path rather than
// dividing by zero.
inline float OverscanPx(float screenPx, float zoom)
{
	if (zoom <= 0.0f)
		return 0.0f;
	float const overscan = screenPx * (1.0f - zoom) / (2.0f * zoom);
	return (overscan > 0.0f) ? overscan : 0.0f;
}

// The pan budget on one axis: how far the camera offset may travel from centre
// before the source rect leaves rendered content. Never negative — once the
// overscan alone exceeds the margin, no offset is safe and the caller should
// treat the budget as exhausted rather than as a negative allowance.
inline float PanBudgetPx(float screenPx, float marginPx, float zoom)
{
	float const budget = marginPx - OverscanPx(screenPx, zoom);
	return (budget > 0.0f) ? budget : 0.0f;
}

// The zoom below which even a perfectly centred window (offset 0) overruns the
// margin. Solving overscan(z) == margin for z:
//
//     W(1 - z) / 2z = m   ->   W - Wz = 2mz   ->   z = W / (W + 2m)
//
// Below this the rendered margin simply cannot supply the pixels the present
// asks for, at any pan offset. Callers clamp the camera zoom to it.
inline float MinSafeZoom(float screenPx, float marginPx)
{
	float const denom = screenPx + 2.0f * marginPx;
	if (denom <= 0.0f)
		return 1.0f;
	return screenPx / denom;
}

// P13 step 2.6 — the 100% detent. Continuous zoom makes it hard to land exactly
// on 1.0 by hand, and 1.0 is the one zoom worth landing on: it is where the
// texture maps 1:1 to the screen, so terrain is pixel-exact rather than
// resampled. Inside a narrow band the camera is pulled to exactly 1.0.
//
// Applied only as the camera settles, not while the gesture is live -- a detent
// that fights an active pinch feels like the zoom is sticking.
inline float ZoomDetent(float zoom, float band = 0.04f)
{
	float const d = (zoom > 1.0f) ? (zoom - 1.0f) : (1.0f - zoom);
	return (d <= band) ? 1.0f : zoom;
}

}  // namespace camera_window

#endif // CAMERA_WINDOW_H_
