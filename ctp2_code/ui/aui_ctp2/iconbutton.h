#ifdef HAVE_PRAGMA_ONCE
#pragma once
#endif
#ifndef __ICONBUTTON_H__
#define __ICONBUTTON_H__

#include "ui/aui_common/aui_button.h"
#include "ui/aui_ctp2/patternbase.h"

class Icon;


class IconButton : public aui_Button, public PatternBase
{
public:

	IconButton(
		AUI_ERRCODE *retval,
		uint32 id,
		sint32 x,
		sint32 y,
		sint32 width,
		sint32 height,
		MBCHAR *pattern,
		MBCHAR *icon,
		uint16 color,
		ControlActionCallback *ActionFunc = nullptr,
		void *cookie = nullptr );

	IconButton(
		AUI_ERRCODE *retval,
		uint32 id,
		MBCHAR *ldlBlock,
		ControlActionCallback *ActionFunc = nullptr,
		void *cookie = nullptr );

	virtual AUI_ERRCODE	InitCommon( MBCHAR *ldlBlock, BOOL isLDL = FALSE);

	~IconButton() override;

	Icon *&TheIcon( ) { return m_icon; }

	AUI_ERRCODE DrawThis(
		aui_Surface *surface = nullptr,
		sint32 x = 0,
		sint32 y = 0 ) override;

protected:
	Icon		*m_icon;
	uint16		m_color;
	MBCHAR		*m_filename;
};

#endif
