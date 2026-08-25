#ifdef HAVE_PRAGMA_ONCE
#pragma once
#endif
#ifndef _BMH_CREDITS_SCREEN_H_
#define _BMH_CREDITS_SCREEN_H_

class CreditsWindow;

#include <memory>
#include <vector>

#include "ui/aui_ctp2/c3window.h"

class aui_Control;
class aui_Static;
class aui_Button;
class ctp2_Button;
class c3_SimpleAnimation;
class c3_TriggeredAnimation;

class aui_BitmapFont;
class c3_CreditsText;


sint32  creditsscreen_Initialize();
void    creditsscreen_Cleanup();

// Accessor for the credits window pointer.  Used by screenutils.cpp to
// register the window with c3ui_Get() after Initialize.  Returns NULL until
// Initialize has run.  Replaces the previous `extern CreditsWindow*
// g_creditsWindow` — encapsulation step toward future synchronisation.
class CreditsWindow;
CreditsWindow * creditsscreen_GetWindow();

class CreditsWindow : public C3Window {
public:

	CreditsWindow(
		AUI_ERRCODE *retval,
		sint32 id,
		MBCHAR *ldlBlock,
		sint32 bpp,
		AUI_WINDOW_TYPE type = AUI_WINDOW_TYPE_STANDARD, bool bevel = true );

	CreditsWindow(
		AUI_ERRCODE *retval,
		uint32 id,
		sint32 x,
		sint32 y,
		sint32 width,
		sint32 height,
		sint32 bpp,
		MBCHAR *pattern,
		AUI_WINDOW_TYPE type = AUI_WINDOW_TYPE_STANDARD, bool bevel = true );

	~CreditsWindow() override;


	AUI_ERRCODE Idle() override;

	void ToggleAnimation();

	void ShowSecretImage();

	void InitCommonLdl(MBCHAR *ldlBlock);

private:

	std::vector<std::unique_ptr<c3_SimpleAnimation>> m_backgroundAnims;
	std::vector<std::unique_ptr<c3_TriggeredAnimation>> m_triggeredAnims;

	std::unique_ptr<aui_Static> m_background;
	std::unique_ptr<aui_Static> m_border;

	std::unique_ptr<ctp2_Button> m_exitButton;

	std::unique_ptr<aui_Button> m_pauseButton;

	std::unique_ptr<aui_Static> m_secretImage;
	std::unique_ptr<aui_Button> m_secretButton;

	std::unique_ptr<c3_CreditsText> m_creditsText;

	bool m_animating;

	sint32 m_animationSpeed;

	uint32 m_lastIdleTicks;
};






// g_creditsWindow demoted to file-scope `static` in creditsscreen.cpp.
// Use creditsscreen_GetWindow() instead.

#endif
