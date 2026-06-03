#ifdef HAVE_PRAGMA_ONCE
#pragma once
#endif

#ifndef __C3_DROPDOWN_H__
#define __C3_DROPDOWN_H__

#include "ui/aui_common/aui_dropdown.h"
#include "ui/aui_ctp2/patternbase.h"

class c3_DropDown : public aui_DropDown, public PatternBase
{
public:

	c3_DropDown(
		AUI_ERRCODE *retval,
		uint32 id,
		MBCHAR *ldlBlock,
		ControlActionCallback *ActionFunc = nullptr,
		void *cookie = nullptr );
	c3_DropDown(
		AUI_ERRCODE *retval,
		uint32 id,
		sint32 x,
		sint32 y,
		sint32 width,
		sint32 height,
		MBCHAR *pattern,
		sint32 buttonSize = 0,
		sint32 windowSize = 0,
		ControlActionCallback *ActionFunc = nullptr,
		void *cookie = nullptr );
	~c3_DropDown() override = default;

	void Clear();

	AUI_ERRCODE DrawThis(
		aui_Surface *surface = nullptr,
		sint32 x = 0,
		sint32 y = 0 ) override;

protected:
	c3_DropDown() : aui_DropDown() {}
	AUI_ERRCODE InitCommonLdl( MBCHAR *ldlBlock );
	AUI_ERRCODE InitCommon( sint32 buttonSize, sint32 windowSize );
	AUI_ERRCODE CreateComponents( MBCHAR *ldlBlock = nullptr );

protected:
	AUI_ERRCODE	RepositionButton( ) override;
	AUI_ERRCODE	RepositionListBoxWindow( ) override;
};

#endif
