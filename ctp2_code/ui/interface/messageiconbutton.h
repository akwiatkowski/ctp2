//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ header
// Description  : User interface message icon button
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

#ifndef __MESSAGEICONBUTTON_H__
#define __MESSAGEICONBUTTON_H__

#include "ui/aui_common/aui_button.h"


class MessageIconButton : public aui_Button
{
public:

	MessageIconButton(
		AUI_ERRCODE *retval,
		uint32 id,
		MBCHAR *ldlBlock,
		ControlActionCallback *ActionFunc = nullptr,
		void *cookie = nullptr );

	~MessageIconButton() override;
	AUI_ERRCODE DrawThis(
		aui_Surface *surface = nullptr,
		sint32 x = 0,
		sint32 y = 0 ) override;

	void SetCurrentIconButton( MessageIconButton *button );

protected:
    void	MouseRGrabInside(aui_MouseEvent * data) override;
    void	MouseRDropInside(aui_MouseEvent * data) override;
    void	MouseRDropOutside(aui_MouseEvent * data) override;

private:
	static MessageIconButton	*m_currentButton;

};

#endif
