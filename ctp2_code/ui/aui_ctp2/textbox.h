#ifdef HAVE_PRAGMA_ONCE
#pragma once
#endif
#ifndef __TEXTBOX_H__
#define __TEXTBOX_H__

#include "ui/aui_common/aui_textbox.h"
#include "ui/aui_ctp2/patternbase.h"

class aui_Surface;

#define k_TEXTBOX_MAXTEXT		256
#define k_TEXTBOX_MAXITEMS		200
#define k_TEXTBOX_LDL_TEXTSIZE	"textsize"

class TextBox : public aui_TextBox, public PatternBase
{
public:

	TextBox(
		AUI_ERRCODE *retval,
		uint32 id,
		MBCHAR const *ldlBlock,
		ControlActionCallback *ActionFunc = nullptr,
		void *cookie = nullptr);
	TextBox(
		AUI_ERRCODE *retval,
		uint32 id,
		sint32 x,
		sint32 y,
		sint32 width,
		sint32 height,
		MBCHAR const *pattern,
		MBCHAR const *text = nullptr,
		ControlActionCallback *ActionFunc = nullptr,
		void *cookie = nullptr);
	~TextBox() override = default;

	AUI_ERRCODE InitCommonLdl( MBCHAR const *ldlBlock );
	AUI_ERRCODE InitCommon( BOOL fromLDL );

	// SetTextFont/SetTextFontSize are inherited from aui_TextBox; the old
	// forwarding declarations used mismatched signatures and hid the base
	// overloads instead of overriding them.
	AUI_ERRCODE	RepositionItems( ) override;

protected:
	AUI_ERRCODE	CreateRangers( MBCHAR const *ldlBlock );

public:

	AUI_ERRCODE DrawThis(
		aui_Surface *surface = nullptr,
		sint32 x = 0,
		sint32 y = 0 ) override;
};

#endif
