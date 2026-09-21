//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ header
// Description  : User interface radio button
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
// _MSC_VER
// - Use Microsoft C++ extensions when set.
//
//----------------------------------------------------------------------------
//
// Modifications from the original Activision code:
//
// - Event handlers declared in a notation that is more standard C++.
// - #pragma once commented out.
//
//----------------------------------------------------------------------------

#ifndef __AUI_TAB_H__
#define __AUI_TAB_H__

#include <memory>

#include "ui/aui_common/aui_radio.h"


class aui_Tab : public aui_Radio
{
public:

	aui_Tab(
		AUI_ERRCODE *retval,
		uint32 id,
		MBCHAR const *ldlBlock,
		ControlActionCallback *ActionFunc = nullptr,
		void *cookie = nullptr );
	aui_Tab(
		AUI_ERRCODE *retval,
		uint32 id,
		sint32 x,
		sint32 y,
		sint32 width,
		sint32 height,
		sint32 paneWidth,
		sint32 paneHeight,
		ControlActionCallback *ActionFunc = nullptr,
		void *cookie = nullptr,
		sint32 state = 0,
		sint32 numStates = k_AUI_SWITCH_DEFAULTNUMSTATES );
	~aui_Tab() override;

protected:
	aui_Tab() : aui_Radio() {}
	AUI_ERRCODE InitCommon( MBCHAR const *ldlBlock );
	AUI_ERRCODE InitCommon( sint32 paneWidth, sint32 paneHeight );

public:
	sint32 SetState( sint32 state ) override;

	aui_Control	*GetPane( ) const { return m_pane.get(); }

	AUI_ERRCODE	AddPaneControl( aui_Control *control );
	AUI_ERRCODE RemovePaneControl( uint32 controlId );
	aui_Control	*GetPaneControl( uint32 controlId ) const
		{ return (aui_Control *)m_pane->GetChild( controlId ); }

	AUI_ERRCODE	Show( ) override;

protected:
	std::unique_ptr<aui_Control>	m_pane;

	void	MouseLDragOver(aui_MouseEvent * mouseData) override;
	void	MouseLDragAway(aui_MouseEvent * mouseData) override;

	void	MouseLGrabInside(aui_MouseEvent * mouseData) override;
	void	MouseLDropInside(aui_MouseEvent * mouseData) override;
	void	MouseLDropOutside(aui_MouseEvent * mouseData) override {};

	void	MouseRGrabInside(aui_MouseEvent * mouseData) override;
};

#endif
