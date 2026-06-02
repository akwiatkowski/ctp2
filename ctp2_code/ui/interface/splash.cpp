#include "ctp/c3.h"
#include "ui/interface/splash.h"
#include "gs/core/splash_progress.h"

#include "ui/aui_common/aui.h"
#include "ui/aui_common/aui_surface.h"
#include "ui/aui_ctp2/c3ui.h"
#include "gfx/gfx_utils/colorset.h"               // g_colorSet
#include "gs/utility/Globals.h"
#include "gfx/gfx_utils/pixelutils.h"
#include "ui/aui_utils/primitives.h"
#include "gs/database/profileDB.h"


static Splash *        g_splash    = NULL;

#ifdef _DEBUG
// SPLASH_STRING macro state — moved from gameinit.cpp so the splash globals
// live next to the Splash class that owns them.
static sint32          g_splash_cur = 0;
static sint32   g_splash_old = 0;

#ifdef _DEBUG
void splash_MarkOld(void) { g_splash_old = GetTickCount(); }
#endif
static MBCHAR          g_splash_buf[100] = {0};

namespace {
void UISplashShow(const char *msg)
{
	if (!g_splash) return;
	g_splash_cur = GetTickCount();
	snprintf(g_splash_buf, sizeof(g_splash_buf), " %4.2f secs  ",
	         double(g_splash_cur - g_splash_old) * 0.001);
	g_splash_old = g_splash_cur;
	g_splash->AddTextNL(g_splash_buf);
	g_splash->AddText(msg);
}
} // anonymous namespace
#endif

void Splash::Initialize(void)
{
    allocated::reassign(g_splash, new Splash());
#ifdef _DEBUG
    // Register the SPLASH_STRING callback once the Splash object exists.
    splash_progress::Register(&UISplashShow);
#endif
}

void Splash::Cleanup(void)
{
    allocated::clear(g_splash);
}

Splash::Splash()
:
    m_textX     (k_SPLASH_FIRST_X),
    m_textY     (k_SPLASH_FIRST_Y)
{
}

void Splash::AddText(MBCHAR const * text)
{
	if (c3ui_Get())
	{
		primitives_DrawText(c3ui_Get()->Secondary(),
		                    m_textX, m_textY,
		                    text,
		                    g_colorSet->GetColorRef(COLOR_WHITE),
		                    true
		                   );

		if (profiledb_Get() && profiledb_Get()->IsUseDirectXBlitter())
		{
			c3ui_Get()->BltSecondaryToPrimary(k_AUI_BLITTER_FLAG_COPY);
		}
		else
		{
			c3ui_Get()->BltSecondaryToPrimary(k_AUI_BLITTER_FLAG_COPY | k_AUI_BLITTER_FLAG_FAST);
		}
	}
}

void Splash::AddTextNL(MBCHAR const * text)
{
	if (c3ui_Get())
	{
		aui_Surface * surface = c3ui_Get()->Secondary();
		if (!surface) return;

		primitives_DrawText(surface,
		                    m_textX + 325, m_textY,
		                    text,
		                    g_colorSet->GetColorRef(COLOR_WHITE),
		                    true
		                   );

		m_textY += k_SPLASH_TEXT_INC;
		if (m_textY >= surface->Height()) m_textY = k_SPLASH_FIRST_Y;

		if (profiledb_Get() && profiledb_Get()->IsUseDirectXBlitter())
		{
			c3ui_Get()->BltSecondaryToPrimary(k_AUI_BLITTER_FLAG_COPY);
		}
		else
		{
			c3ui_Get()->BltSecondaryToPrimary(k_AUI_BLITTER_FLAG_COPY | k_AUI_BLITTER_FLAG_FAST);
		}
	}
}

void Splash::AddHilitedTextNL(MBCHAR const *text)
{
	if (c3ui_Get())
	{
		aui_Surface * surface = c3ui_Get()->Secondary();

		if (!surface) return;

		primitives_DrawText(surface,
		                    m_textX, m_textY,
		                    text,
		                    g_colorSet->GetColorRef(COLOR_YELLOW),
		                    true
		                   );

		m_textY += k_SPLASH_TEXT_INC;
		if (m_textY >= surface->Height()) m_textY = k_SPLASH_FIRST_Y;

		if (profiledb_Get() && profiledb_Get()->IsUseDirectXBlitter())
		{
			c3ui_Get()->BltSecondaryToPrimary(k_AUI_BLITTER_FLAG_COPY);
	}
		else
		{
			c3ui_Get()->BltSecondaryToPrimary(k_AUI_BLITTER_FLAG_COPY | k_AUI_BLITTER_FLAG_FAST);
		}
	}
}
