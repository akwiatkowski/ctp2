#ifdef HAVE_PRAGMA_ONCE
#pragma once
#endif
#ifndef __ICONTAB_H__
#define __ICONTAB_H__

#include "ui/aui_common/aui_tab.h"
#include "ui/aui_ctp2/patternbase.h"

class Pattern;
class Icon;

class IconTab : public aui_Tab, public PatternBase
{
public:

	IconTab(
		AUI_ERRCODE *retval,
		uint32 id,
		sint32 x,
		sint32 y,
		sint32 width,
		sint32 height,
		sint32 paneWidth,
		sint32 paneHeight,
		MBCHAR const *pattern = nullptr,
		Icon *icon = nullptr,
		uint16 color = 0,
		ControlActionCallback *ActionFunc = nullptr,
		void *cookie = nullptr,
		BOOL selected = FALSE );
	~IconTab() override = default;

	Pattern *&ThePattern( ) { return m_pattern; }
	Icon *&TheIcon( ) { return m_icon; }
	uint16 TheColor( ) { return m_color; }

	AUI_ERRCODE DrawThis(
		aui_Surface *surface = nullptr,
		sint32 x = 0,
		sint32 y = 0 ) override;

protected:
	Icon *m_icon;
	uint16 m_color;
};

#endif
