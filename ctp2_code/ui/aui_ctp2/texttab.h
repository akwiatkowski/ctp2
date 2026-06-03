#ifdef HAVE_PRAGMA_ONCE
#pragma once
#endif
#ifndef __TEXTTAB_H__
#define __TEXTTAB_H__

#include "ui/aui_common/aui_tab.h"
#include "ui/aui_ctp2/patternbase.h"


class TextTab : public aui_Tab, public PatternBase
{
public:

	TextTab(
		AUI_ERRCODE *retval,
		uint32 id,
		MBCHAR *ldlBlock,
		ControlActionCallback *ActionFunc = nullptr,
		void *cookie = nullptr);
	TextTab(
		AUI_ERRCODE *retval,
		uint32 id,
		sint32 x,
		sint32 y,
		sint32 width,
		sint32 height,
		sint32 paneWidth,
		sint32 paneHeight,
		MBCHAR *pattern = nullptr,
		MBCHAR *text = nullptr,
		ControlActionCallback *ActionFunc = nullptr,
		void *cookie = nullptr,
		BOOL selected = FALSE );
	~TextTab() override = default;

protected:
	TextTab() : aui_Tab() {}

public:
	AUI_ERRCODE DrawThis(
		aui_Surface *surface = nullptr,
		sint32 x = 0,
		sint32 y = 0 ) override;
};

#endif
