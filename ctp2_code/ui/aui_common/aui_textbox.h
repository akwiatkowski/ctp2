#ifndef __AUI_TEXTBOX_H__
#define __AUI_TEXTBOX_H__

#include <array>
#include <memory>

#include "ui/aui_common/aui_listbox.h"

class aui_Static;

#define k_AUI_TEXTBOX_MAXTEXT		256
#define k_AUI_TEXTBOX_MAXITEMS		100


class aui_TextBox : public aui_ListBox
{
public:

	aui_TextBox(
		AUI_ERRCODE *retval,
		uint32 id,
		MBCHAR const *ldlBlock,
		ControlActionCallback *ActionFunc = nullptr,
		void *cookie = nullptr );
	aui_TextBox(
		AUI_ERRCODE *retval,
		uint32 id,
		sint32 x,
		sint32 y,
		sint32 width,
		sint32 height,
		MBCHAR const *text = nullptr,
		ControlActionCallback *ActionFunc = nullptr,
		void *cookie = nullptr );
	~aui_TextBox() override;

protected:
	aui_TextBox() : aui_ListBox() {}
	AUI_ERRCODE InitCommonLdl( MBCHAR const *ldlBlock );
	AUI_ERRCODE InitCommon( );

public:
	AUI_ERRCODE	SetText(
		const MBCHAR *text,
		uint32 maxlen = 0xffffffff ) override;
	AUI_ERRCODE	AppendText(MBCHAR const *text) override;

	virtual AUI_ERRCODE AppendText(
		MBCHAR const *  text,
		COLORREF color,
		sint32 bold = 0,
		sint32 italic = 0 );

	AUI_ERRCODE DrawThis(
		aui_Surface *surface = nullptr,
		sint32 x = 0,
		sint32 y = 0 ) override;

	void SetTextFont( MBCHAR const * ttffile ) override;
	void SetTextFontSize( sint32 pointSize ) override;

protected:

	AUI_ERRCODE	CalculateItems(MBCHAR const * text = nullptr);
	AUI_ERRCODE	CalculateAppendedItems(MBCHAR const * text = nullptr);

	sint32		m_numItems;
	sint32		m_curItem;
	std::array<std::unique_ptr<aui_Static>, k_AUI_TEXTBOX_MAXITEMS> m_items;

	COLORREF	m_curColor;
	sint32		m_curBold;
	sint32		m_curItalic;
};

#endif
