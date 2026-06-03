#ifdef HAVE_PRAGMA_ONCE
#pragma once
#endif
#ifndef __ICONSWITCH_H__
#define __ICONSWITCH_H__

#include "ui/aui_common/aui_switch.h"
#include "ui/aui_ctp2/patternbase.h"

class Pattern;
class Icon;

class IconSwitch : public aui_Switch, public PatternBase
{
public:

	IconSwitch(
		AUI_ERRCODE *retval,
		uint32 id,
		sint32 x,
		sint32 y,
		MBCHAR *pattern = nullptr,
		Icon *icon = nullptr,
		uint16 color = NULL,
		ControlActionCallback *ActionFunc = nullptr,
		void *cookie = nullptr,
		BOOL selected = FALSE );
	virtual ~IconSwitch() {}

	Pattern *&ThePattern( ) { return m_pattern; }
	Icon *&TheIcon( ) { return m_icon; }
	uint16 TheColor( ) { return m_color; }

	virtual AUI_ERRCODE DrawThis(
		aui_Surface *surface = nullptr,
		sint32 x = 0,
		sint32 y = 0 );

protected:
	Icon *m_icon;
	uint16 m_color;
};

#endif
