#ifdef HAVE_PRAGMA_ONCE
#pragma once
#endif
#ifndef __VIDEOWINDOW_H__
#define __VIDEOWINDOW_H__

#include "ui/aui_ctp2/c3window.h"


class VideoWindow : public C3Window
{
public:
	VideoWindow(
		AUI_ERRCODE *retval,
		uint32 id,
		sint32 x,
		sint32 y,
		sint32 width,
		sint32 height,
		sint32 bpp,
		MBCHAR *pattern,
		MBCHAR *name,
		BOOL modal,
		AUI_WINDOW_TYPE type = AUI_WINDOW_TYPE_FLOATING );
	~VideoWindow() override;

protected:
	VideoWindow() : C3Window() {};
	AUI_ERRCODE InitCommon( );
	AUI_ERRCODE CreateVideoSurface( MBCHAR *name, BOOL modal );

public:
	AUI_ERRCODE Idle( ) override;
	AUI_ERRCODE DrawThis(aui_Surface *surface = nullptr, sint32 x = 0, sint32 y = 0 ) override;

private:
#ifdef __AUI_USE_DIRECTX__
	DirectVideo		*m_video;
#endif
	MBCHAR			m_filename[_MAX_PATH];
	BOOL			m_modal;
};

#endif
