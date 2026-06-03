//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ header
// Description  : User interface thumb?
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

#ifndef __AUI_THUMB_H__
#define __AUI_THUMB_H__

#include "ui/aui_common/aui_control.h"

class aui_Surface;


enum AUI_THUMB_ACTION
{
	AUI_THUMB_ACTION_FIRST = 0,
	AUI_THUMB_ACTION_NULL = 0,
	AUI_THUMB_ACTION_GRAB,
	AUI_THUMB_ACTION_DRAG,
	AUI_THUMB_ACTION_DROP,
	AUI_THUMB_ACTION_LAST
};


class aui_Thumb : public aui_Control
{
public:

	aui_Thumb(
		AUI_ERRCODE *retval,
		uint32 id,
		MBCHAR *ldlBlock,
		ControlActionCallback *ActionFunc = nullptr,
		void *cookie = nullptr );
	aui_Thumb(
		AUI_ERRCODE *retval,
		uint32 id,
		sint32 x,
		sint32 y,
		sint32 width,
		sint32 height,
		ControlActionCallback *ActionFunc = nullptr,
		void *cookie = nullptr );
	~aui_Thumb() override = default;

public:
	AUI_ERRCODE	Reposition( sint32 x, sint32 y );

protected:
	aui_Thumb() : aui_Control() {}
	AUI_ERRCODE	InitCommon( );

	POINT	m_grabPoint;

	void	MouseLDragOver(aui_MouseEvent * mouseData) override;
	void	MouseLDragAway(aui_MouseEvent * mouseData) override;
	void	MouseLDragInside(aui_MouseEvent * mouseData) override;
	void	MouseLDragOutside(aui_MouseEvent * mouseData) override;

	void	MouseLGrabInside(aui_MouseEvent * mouseData) override;
	void	MouseRGrabInside(aui_MouseEvent * mouseData) override {};
	void	MouseLDropInside(aui_MouseEvent * mouseData) override;
	void	MouseLDropOutside(aui_MouseEvent * mouseData) override;

	void	MouseRDropInside(aui_MouseEvent * mouseData) override;
};

#endif
