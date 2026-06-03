#ifdef HAVE_PRAGMA_ONCE
#pragma once
#endif
#ifndef __C3LISTBOX_H__
#define __C3LISTBOX_H__

#include "ui/aui_common/aui_listbox.h"
#include "ui/aui_ctp2/patternbase.h"

class aui_Surface;


class C3ListBox : public aui_ListBox, public PatternBase
{
public:

	C3ListBox(
		AUI_ERRCODE *retval,
		uint32 id,
		MBCHAR *ldlBlock,
		ControlActionCallback *ActionFunc = nullptr,
		void *cookie = nullptr );
	C3ListBox(
		AUI_ERRCODE *retval,
		uint32 id,
		sint32 x,
		sint32 y,
		sint32 width,
		sint32 height,
		MBCHAR *pattern,
		ControlActionCallback *ActionFunc = nullptr,
		void *cookie = nullptr );
	~C3ListBox() override = default;

protected:
	C3ListBox() : aui_ListBox() {}
	AUI_ERRCODE CreateRangers( );

public:
	AUI_ERRCODE DrawThis(
		aui_Surface *surface = nullptr,
		sint32 x = 0,
		sint32 y = 0 ) override;
};

#endif
