//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ header
// Description  : SPLASH_STRING progress macro without the UI dependency
//
//----------------------------------------------------------------------------
//
// The original SPLASH_STRING macro (ui/interface/splash.h) wrote progress
// text directly to g_splash and used c3ui_Get() — pulling the splash window
// into every translation unit that wanted to log "Loading Sprite DB..."
// during startup.  Game-state init files (gameinit.cpp, ctpai.cpp) have no
// business knowing about the splash window.
//
// This header exposes SPLASH_STRING as a thin macro that calls
// splash_progress::Show(), a free function backed by a UI-registered
// callback.  The UI build registers a callback that drives the actual
// Splash widget; the headless build leaves it unregistered, and the macro
// becomes a no-op via the null-callback check.  In release builds the
// macro is a true compile-time no-op (matches the legacy splash.h behavior).
//
//----------------------------------------------------------------------------

#pragma once

#include "ctp2_inttypes.h"

namespace splash_progress {

using ShowFn = void (*)(const char *msg);

void Register(ShowFn fn);
void Show(const char *msg);

} // namespace splash_progress

#ifdef _DEBUG
#define SPLASH_STRING(x)        splash_progress::Show(x)
#define SPLASH_STRING_SIMPLE(x) splash_progress::Show(x)
#else
#define SPLASH_STRING(x)
#define SPLASH_STRING_SIMPLE(x)
#endif
