//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ header
// Description  : User interface button
//
//----------------------------------------------------------------------------
//
// Disclaimer
//
// THIS FILE IS NOT GENERATED OR SUPPORTED BY ACTIVISION.
//
// This material has been developed at apolyton.net by the Apolyton CtP2
// Source Code Project. Contact the authors at ctp2source@apolyton.net.
//
//----------------------------------------------------------------------------
//
// Compiler flags
//
//
//----------------------------------------------------------------------------
//
// Modifications from the original Activision code:
//
// - Event handlers declared in a notation that is more standard C++.
// - #pragma once commented out.
//
//----------------------------------------------------------------------------

#ifdef HAVE_PRAGMA_ONCE
#pragma once
#endif

#ifndef CTP2_BUTTON_H__
#define CTP2_BUTTON_H__

#include "ui/aui_common/aui_button.h"

class aui_ImageList;

class ctp2_Button : public aui_Button
{
public:

	ctp2_Button(AUI_ERRCODE *retval,
				uint32 id,
				MBCHAR const *ldlBlock,
				ControlActionCallback *ActionFunc = nullptr,
				void *cookie = nullptr) :
		aui_ImageBase(ldlBlock),
		aui_TextBase(ldlBlock, (MBCHAR const *) nullptr),
		aui_Button(retval, id, ldlBlock, ActionFunc, cookie)
	{ *retval = InitCommonLdl(ldlBlock, nullptr, 0, 0, 0, 0); }

	ctp2_Button(AUI_ERRCODE *retval,
				uint32 id,
				MBCHAR const *ldlBlock,
				MBCHAR const *ldlTemplate,
				sint32 x,
				sint32 y,
				sint32 width,
				sint32 height,
				ControlActionCallback *ActionFunc = nullptr,
				void *cookie = nullptr) :
		aui_ImageBase(ldlBlock),
		aui_TextBase(ldlBlock, (MBCHAR const *) nullptr),
		aui_Button(retval, id, ldlBlock, ActionFunc, cookie)
	{ *retval = InitCommonLdl(ldlBlock, ldlTemplate,
		x, y, width, height); }

	~ctp2_Button() override;

	AUI_ERRCODE InitCommonLdl(MBCHAR const *ldlBlock, MBCHAR const *ldlTemplate,
		sint32 x, sint32 y, sint32 width, sint32 height);


	AUI_ERRCODE DrawThis(aui_Surface *surface = nullptr,
								 sint32 x = 0, sint32 y = 0) override;

	void	MouseLGrabInside(aui_MouseEvent * mouseData) override;
	void	MouseLDropInside(aui_MouseEvent * mouseData) override;
	void	MouseLDragOver(aui_MouseEvent * mouseData) override;
	void	MouseLDragAway(aui_MouseEvent * mouseData) override;

	static const sint32 k_CTP2_BUTTON_LAYER_FLAG_UP;
	static const sint32 k_CTP2_BUTTON_LAYER_FLAG_DOWN;
	static const sint32 k_CTP2_BUTTON_LAYER_FLAG_UP_ON;

	bool IgnoreHighlight() override { return m_ignoreHighlight; }

	bool CanAttract() override { return true; }
	void SetAttract(bool on, uint32 ticksSinceStart) override ;
	bool GetAttracting() override { return m_attract; }

	void SetToggleState(bool state);
	bool GetToggleState() { return m_toggleState; }

private:

	void ResetCurrentRenderFlags() override;

	void RenderDown(bool status = true);

	bool m_ignoreHighlight;
	bool m_attract;
	uint32 m_attractTicks;
	bool m_isDropdownButton;

	bool m_isToggle;
	bool m_toggleState;
};

#endif
