#ifdef HAVE_PRAGMA_ONCE
#pragma once
#endif
#ifndef __C3DROPDOWN_H__
#define __C3DROPDOWN_H__

#include "ui/aui_common/aui_dropdown.h"
#include "ui/aui_ctp2/patternbase.h"

class aui_Surface;


class C3DropDown : public aui_DropDown, public PatternBase
{
public:

	C3DropDown(
		AUI_ERRCODE *retval,
		uint32 id,
		sint32 x,
		sint32 y,
		sint32 width,
		sint32 height,
		MBCHAR *pattern,
		sint32 buttonSize = k_CONTROL_DEFAULT_SIZE,
		sint32 windowSize = 0,
		ControlActionCallback *ActionFunc = nullptr,
		void *cookie = nullptr );
	C3DropDown(
		AUI_ERRCODE *retval,
		uint32 id,
		MBCHAR *ldlBlock,
		ControlActionCallback *ActionFunc = nullptr,
		void *cookie = nullptr);

	~C3DropDown() override = default;

protected:
	C3DropDown() : aui_DropDown() {}
	AUI_ERRCODE CreateComponents();

public:
	AUI_ERRCODE DrawThis(
		aui_Surface *surface = nullptr,
		sint32 x = 0,
		sint32 y = 0 ) override;

protected:
	AUI_ERRCODE	RepositionButton( ) override;
};

#endif
