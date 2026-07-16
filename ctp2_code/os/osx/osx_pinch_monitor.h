//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ header
// Description  : macOS trackpad pinch input (P11 pinch zoom)
//
//----------------------------------------------------------------------------
//
// SDL3 on macOS reports the trackpad as a mouse and forwards NO touch
// events by default; its SDL_TRACKPAD_IS_TOUCH_ONLY hint would deliver
// them only by turning the trackpad into a touch-only device (breaking
// normal cursor control). Pinches, however, arrive as native Cocoa
// magnify gestures — a local NSEvent monitor taps them without touching
// SDL's view or event queue.
//
// The monitor's handler runs on the main thread inside SDL_PumpEvents;
// it only accumulates the magnification. The game loop consumes the
// accumulated value once per frame, OUTSIDE the pump, so zoom re-renders
// never re-enter the event dispatch.
//
//----------------------------------------------------------------------------

#if defined(HAVE_PRAGMA_ONCE)
#pragma once
#endif

#ifndef OSX_PINCH_MONITOR_H_
#define OSX_PINCH_MONITOR_H_

#ifdef __APPLE__

// Install the magnify-gesture monitor (idempotent; main thread).
void osx_InstallPinchMonitor();

// Return the magnification accumulated since the last call and reset it.
// Positive = fingers spreading (zoom in); a full relaxed pinch sums ~±1.
float osx_ConsumePinchMagnification();

#endif // __APPLE__

#endif // OSX_PINCH_MONITOR_H_
