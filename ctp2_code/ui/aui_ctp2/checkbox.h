#ifdef HAVE_PRAGMA_ONCE
#pragma once
#endif
#ifndef __CHECKBOX_H__
#define __CHECKBOX_H__

#include "ui/aui_common/aui_switch.h"
#include "ui/aui_ctp2/patternbase.h"


class Checkbox : public aui_Switch,  public PatternBase
{
public:

	Checkbox(
		AUI_ERRCODE *retval,
		uint32 id,
		MBCHAR *ldlBlock,
		ControlActionCallback *ActionFunc = nullptr,
		void *cookie = nullptr);
	Checkbox(
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
	~Checkbox() override = default;

protected:
	Checkbox() : aui_Switch() {}

public:
	AUI_ERRCODE DrawThis(
		aui_Surface *surface = nullptr,
		sint32 x = 0,
		sint32 y = 0 ) override;
};

#endif
