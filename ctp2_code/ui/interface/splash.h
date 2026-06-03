#ifdef HAVE_PRAGMA_ONCE
#pragma once
#endif

#ifndef __SPLASH_H__
#define __SPLASH_H__

class Splash;

#define k_SPLASH_FIRST_X		0
#define k_SPLASH_FIRST_Y		18
#define	k_SPLASH_TEXT_INC		15

#ifdef _DEBUG
// Reset the debug-only "previous splash tick" baseline.  Backing storage
// is `static sint32 g_splash_old` in splash.cpp; only debug-build callers
// (civapp.cpp, civ3_main.cpp) ever poke it.
void splash_MarkOld();
#endif

// SPLASH_STRING / SPLASH_STRING_SIMPLE now live in gs/core/splash_progress.h
// — they call a UI-registered callback instead of touching g_splash and
// c3ui_Get() directly, so non-UI callers (gameinit, ctpai) no longer drag
// splash.h into the simulation core.
#include "gs/core/splash_progress.h"

class Splash
{
public:
	static void Initialize();
	static void Cleanup();

	Splash();

	void AddText(MBCHAR const * text);
	void AddTextNL(MBCHAR const * text);
	void AddHilitedTextNL(MBCHAR const * text);

private:
	sint32		m_textX;
    sint32      m_textY;
};

#endif
