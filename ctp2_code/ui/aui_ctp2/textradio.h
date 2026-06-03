#ifdef HAVE_PRAGMA_ONCE
#pragma once
#endif
#ifndef __TEXTRADIO_H__
#define __TEXTRADIO_H__

#include "ui/aui_common/aui_radio.h"
#include "ui/aui_ctp2/patternbase.h"


class TextRadio : public aui_Radio, public PatternBase
{
public:

	TextRadio(
		AUI_ERRCODE *retval,
		uint32 id,
		MBCHAR *ldlBlock,
		ControlActionCallback *ActionFunc = nullptr,
		void *cookie = nullptr);
	TextRadio(
		AUI_ERRCODE *retval,
		uint32 id,
		sint32 x,
		sint32 y,
		sint32 width,
		sint32 height,
		MBCHAR *pattern = nullptr,
		MBCHAR *text = nullptr,
		ControlActionCallback *ActionFunc = nullptr,
		void *cookie = nullptr,
		BOOL selected = FALSE );
	~TextRadio() override = default;

protected:
	TextRadio() : aui_Radio() {}

public:
	AUI_ERRCODE DrawThis(
		aui_Surface *surface = nullptr,
		sint32 x = 0,
		sint32 y = 0 ) override;
};

#endif
