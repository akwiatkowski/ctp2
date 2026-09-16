#ifdef HAVE_PRAGMA_ONCE
#pragma once
#endif
#ifndef _BMH_END_GAME_WINDOW_H_
#define _BMH_END_GAME_WINDOW_H_

#include <memory>
#include <vector>

#include "ui/aui_ctp2/c3window.h"
#include "ui/aui_ctp2/keyboardhandler.h"

class aui_Control;
class aui_Static;
class c3_Static;
class c3_ColoredStatic;
class c3_Button;
class c3_Blend;
class c3_Animation;
class c3_DarkenArea;
class c3_YetAnotherProgressBar;
class EndGame;

extern sint32 endgamewindow_Initialize();

extern sint32 endgamewindow_Cleanup();

class EndGameWindow : public C3Window, public KeyboardHandler {
public:

	EndGameWindow(
		AUI_ERRCODE *retval,
		sint32 id,
		MBCHAR *ldlBlock,
		sint32 bpp,
		AUI_WINDOW_TYPE type = AUI_WINDOW_TYPE_STANDARD, bool bevel = true );

	EndGameWindow(
		AUI_ERRCODE *retval,
		uint32 id,
		sint32 x,
		sint32 y,
		sint32 width,
		sint32 height,
		sint32 bpp,
		MBCHAR *pattern,
		AUI_WINDOW_TYPE type = AUI_WINDOW_TYPE_STANDARD, bool bevel = true );

	~EndGameWindow() override;

	void SetStage(sint32 stage, sint32 lastStage);

	void SetEmbryoTank(bool exists, bool existed, bool isBroken, sint32 soundID);

	void SetContainmentFields(sint32 number, sint32 alreadyDisplayed, sint32 soundID);
	void SetECDs(sint32 number, sint32 alreadyDisplayed, sint32 soundID);
	void SetSplicers(sint32 number, sint32 alreadyDisplayed, sint32 soundID);

	void Update(EndGame *endGame);

	void UpdateTurn(EndGame *endGame);


	AUI_ERRCODE Idle() override;

	void kh_Close() override;

protected:

	void InitCommonLdl(MBCHAR *ldlBlock);

	void UpdateBlend(sint32 deltaTime, c3_Blend *blendControl);

private:

	MBCHAR const *m_embryoTankName = nullptr;
	MBCHAR const *m_containmentFieldName = nullptr;
	MBCHAR const *m_ECDName = nullptr;
	MBCHAR const *m_splicerName = nullptr;

	// LDL-driven sizes; the vectors below are resized to exactly these.
	sint32 m_numberOfStages = 0;
	sint32 m_numberOfContainmentFields = 0;
	sint32 m_numberOfECDs = 0;
	sint32 m_numberOfSplicers = 0;
	sint32 m_numberOfBackgroundAnims = 0;

	std::unique_ptr<aui_Static> m_background;
	std::unique_ptr<aui_Static> m_border;
	std::unique_ptr<aui_Static> m_brokenTank;
	std::unique_ptr<c3_Blend> m_embryoTank;
	std::vector<std::unique_ptr<c3_Blend>> m_embryoStage;
	std::vector<std::unique_ptr<c3_Blend>> m_containmentField;
	std::vector<std::unique_ptr<c3_Blend>> m_ECD;
	std::vector<std::unique_ptr<c3_Blend>> m_splicer;

	std::vector<std::unique_ptr<c3_Animation>> m_backgroundAnim;
	std::unique_ptr<c3_Animation> m_embryoGlow;


	std::unique_ptr<c3_DarkenArea> m_darkenArea;
	sint32 m_numberOfLabels = 0;
	std::vector<std::unique_ptr<aui_Static>> m_labels;
	std::unique_ptr<c3_ColoredStatic> m_progressBackground;
	std::unique_ptr<c3_YetAnotherProgressBar> m_turnProgress;
	std::unique_ptr<aui_Static> m_turnsRemaining;
	std::unique_ptr<aui_Static> m_ecdRatio;
	std::unique_ptr<aui_Static> m_containmentFieldRatio;
	std::unique_ptr<aui_Static> m_splicerRatio;
	std::unique_ptr<aui_Static> m_chanceOfFailure;
	sint32 m_numberOfStageLights = 0;
	std::vector<std::unique_ptr<c3_ColoredStatic>> m_stageLights;

	std::unique_ptr<c3_Button> m_exitButton;

	sint32 m_blendSpeed;

	uint32 lastIdle;
};

// g_endgameWindow demoted to file-scope `static` in EndgameWindow.cpp.
// External callers go through endgamewindow_Get() (returns NULL when the
// end-game dialog has not been opened).
EndGameWindow * endgamewindow_Get();

#endif
