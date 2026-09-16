#ifdef HAVE_PRAGMA_ONCE
#pragma once
#endif
#ifndef __C3_LISTBOX__
#define __C3_LISTBOX__

#include "ui/aui_ctp2/patternbase.h"
#include "ui/aui_common/aui_listbox.h"

#define k_C3_LISTBOX_LDL_BEVELWIDTH		"bevelwidth"
#define k_C3_LISTBOX_LDL_BEVELTYPE		"beveltype"

class aui_Surface;
class c3_ListItem;

class c3_ListBox : public aui_ListBox, public PatternBase
{
public:
	c3_ListBox() : aui_ListBox() {}
	c3_ListBox(AUI_ERRCODE *retval,	uint32 id, MBCHAR const *ldlBlock,
							ControlActionCallback *ActionFunc=nullptr, void *cookie=nullptr );
	c3_ListBox(AUI_ERRCODE *retval, uint32 id, sint32 x, sint32 y, sint32 width, sint32 height,
							MBCHAR const *pattern, sint32 bevelwidth = 0, sint32 beveltype = 0,
							ControlActionCallback *ActionFunc = nullptr, void *cookie = nullptr);

	~c3_ListBox() override;

	AUI_ERRCODE InitCommonLdl( MBCHAR const *ldlBlock );
	AUI_ERRCODE InitCommon(sint32 bevelWidth, sint32 bevelType );
	AUI_ERRCODE CreateRangersAndHeader( MBCHAR const *ldlBlock = nullptr );

	void Clear();

	AUI_ERRCODE SortByColumn( sint32 column, BOOL ascending ) override;
	AUI_ERRCODE Draw(aui_Surface *surface = nullptr, sint32 x = 0, sint32 y = 0) override;
	AUI_ERRCODE DrawThis(aui_Surface *surface = nullptr, sint32 x = 0, sint32 y = 0 ) override;

	sint32 GetBevelWidth() { return m_bevelWidth; }
	sint32 GetBevelType() { return m_bevelType; }

protected:
	AUI_ERRCODE ReformatItemFromHeader(aui_Item *item);

private:
	sint32		m_bevelWidth;
	sint32		m_bevelType;
};

#endif
