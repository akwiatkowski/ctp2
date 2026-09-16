//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ header
// Description  : User interface slider
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

#ifdef HAVE_PRAGMA_ONCE
#pragma once
#endif

#ifndef __C3SLIDER_H__
#define __C3SLIDER_H__

#include "ui/aui_common/aui_ranger.h"
#include "ui/aui_ctp2/patternbase.h"

class aui_Surface;

#define k_C3SLIDER_LDL_TICKS	"ticks"


class C3Slider : public aui_Ranger, public PatternBase
{
public:

	C3Slider(
		AUI_ERRCODE *retval,
		uint32 id,
		MBCHAR const *ldlBlock,
		ControlActionCallback *ActionFunc = nullptr,
		void *cookie = nullptr );
	C3Slider(
		AUI_ERRCODE *retval,
		uint32 id,
		sint32 x,
		sint32 y,
		sint32 width,
		sint32 height,
		BOOL isVertical,
		MBCHAR const *pattern,
		ControlActionCallback *ActionFunc = nullptr,
		void *cookie = nullptr );
	~C3Slider() override = default;

	AUI_ERRCODE DrawThis(
		aui_Surface *surface = nullptr,
		sint32 x = 0,
		sint32 y = 0 ) override;

	BOOL	IsVertical( ) const
	{ return m_orientation == AUI_RANGER_ORIENTATION_VERTICAL; }

	void	MouseRGrabInside(aui_MouseEvent * mouseData) override;

protected:
	C3Slider() : aui_Ranger() {}
	AUI_ERRCODE InitCommon( MBCHAR const *ldlBlock );
	AUI_ERRCODE InitCommon( );
	AUI_ERRCODE CreateThumb( MBCHAR const *ldlBlock );

	sint32 m_ticks;
};

aui_Control::ControlActionCallback C3SliderThumbActionCallback;
aui_Control::ControlActionCallback C3SliderButtonActionCallback;

#endif
