//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ header
// Description  : User interface region (rectangle on the display)
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
//
//----------------------------------------------------------------------------

#ifndef __C3_STATIC_H__
#define __C3_STATIC_H__

#include "ui/aui_common/aui_static.h"
#include "ui/aui_ctp2/patternbase.h"

#define k_C3_STATIC_LDL_BEVELWIDTH			"bevelwidth"
#define k_C3_STATIC_LDL_BEVELTYPE			"beveltype"

#define k_C3_STATIC_ACTION_LMOUSE 1
#define k_C3_STATIC_ACTION_RMOUSE 2

class aui_Surface;

class c3_Static : public aui_Static, public PatternBase
{
public:
	c3_Static(AUI_ERRCODE *retval,
						uint32 id,
						MBCHAR const *ldlBlock );
	c3_Static(AUI_ERRCODE *retval,
						uint32 id,
						sint32 x,
						sint32 y,
						sint32 width,
						sint32 height,
						MBCHAR const *pattern,
						MBCHAR const *text,
						uint32 maxLength,
						uint32 bevelWidth,
						uint32 bevelType);

	AUI_ERRCODE InitCommonLdl( MBCHAR const *ldlBlock );
	AUI_ERRCODE InitCommon(uint32 bevelWidth, uint32 bevelType );

	AUI_ERRCODE DrawThis(aui_Surface *surface,
											sint32 x,
											sint32 y) override;

	uint32 BevelWidth() { return m_bevelWidth; }

	bool IgnoreHighlight() override { return true; }

protected:
	void	MouseLGrabInside(aui_MouseEvent * mouseData) override;
	void	MouseLDropInside(aui_MouseEvent * mouseData) override;
	void	MouseRGrabInside(aui_MouseEvent * mouseData) override;
	void	MouseRDropInside(aui_MouseEvent * mouseData) override;

	uint32 m_bevelWidth;
	uint32 m_bevelType;
};

#endif
