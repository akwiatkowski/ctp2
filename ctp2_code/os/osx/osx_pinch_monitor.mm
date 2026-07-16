//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : Objective-C++ source
// Description  : macOS trackpad pinch input (P11 pinch zoom)
//
//----------------------------------------------------------------------------
//
// See os/osx/osx_pinch_monitor.h for why a native monitor is needed.
//
//----------------------------------------------------------------------------

#include "os/osx/osx_pinch_monitor.h"

#ifdef __APPLE__

#import <AppKit/AppKit.h>

// Written by the monitor handler and read by the game loop — both on the
// main thread (the handler fires inside SDL_PumpEvents' NSApp dispatch).
static float s_magnification = 0.0f;

void osx_InstallPinchMonitor()
{
	static id s_monitor = nil;
	if (s_monitor)
		return;
	s_monitor = [NSEvent
		addLocalMonitorForEventsMatchingMask:NSEventMaskMagnify
		handler:^NSEvent *(NSEvent *event) {
			s_magnification += (float)event.magnification;
			return event;   // pass the event on unmodified
		}];
}

float osx_ConsumePinchMagnification()
{
	float const value = s_magnification;
	s_magnification = 0.0f;
	return value;
}

#endif // __APPLE__
