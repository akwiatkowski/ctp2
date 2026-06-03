#ifdef HAVE_PRAGMA_ONCE
#pragma once
#endif
#ifndef __NS_CIVLISTBOX__
#define __NS_CIVLISTBOX__

#include "ui/aui_ctp2/patternbase.h"
#include "ui/aui_common/aui_listbox.h"

#define k_NS_CIVLISTBOX_LDL_BEVELWIDTH		"bevelwidth"
#define k_NS_CIVLISTBOX_LDL_BEVELTYPE		"beveltype"

class aui_Surface;

class ns_CivListBox : public aui_ListBox, public PatternBase
{
public:
	ns_CivListBox(
		AUI_ERRCODE *retval,
		uint32 id,
		MBCHAR *ldlBlock,
		ControlActionCallback *ActionFunc = nullptr,
		void *cookie = nullptr );
	ns_CivListBox(
		AUI_ERRCODE *retval,
		uint32 id,
		sint32 x,
		sint32 y,
		sint32 width,
		sint32 height,
		MBCHAR *pattern,
		sint32 bevelwidth = 0,
		sint32 beveltype = 0,
		ControlActionCallback *ActionFunc = nullptr,
		void *cookie = nullptr );
	~ns_CivListBox() override = default;

	AUI_ERRCODE Draw(
		aui_Surface *surface = nullptr,
		sint32 x = 0,
		sint32 y = 0) override;
	AUI_ERRCODE DrawThis(
		aui_Surface *surface = nullptr,
		sint32 x = 0,
		sint32 y = 0 ) override;

	AUI_ERRCODE	RemoveItem( uint32 itemId ) override;

	sint32 GetBevelWidth() { return m_bevelWidth; }
	sint32 GetBevelType() { return m_bevelType; }

	BOOL m_selectableList;

protected:
	ns_CivListBox() : aui_ListBox() {}

	AUI_ERRCODE InitCommonLdl( MBCHAR *ldlBlock );
	AUI_ERRCODE InitCommon( sint32 bevelWidth, sint32 bevelType );
	AUI_ERRCODE CreateRangersAndHeader( MBCHAR *ldlBlock = nullptr );

private:
	sint32 m_bevelWidth;
	sint32 m_bevelType;
};

class ns_HPlayerListBox : public ns_CivListBox
{
public:
	ns_HPlayerListBox(
		AUI_ERRCODE *retval,
		uint32 id,
		MBCHAR *ldlBlock,
		ControlActionCallback *ActionFunc = nullptr,
		void *cookie = nullptr );
	ns_HPlayerListBox(
		AUI_ERRCODE *retval,
		uint32 id,
		sint32 x,
		sint32 y,
		sint32 width,
		sint32 height,
		MBCHAR *pattern,
		sint32 bevelwidth = 0,
		sint32 beveltype = 0,
		ControlActionCallback *ActionFunc = nullptr,
		void *cookie = nullptr );
	~ns_HPlayerListBox() override;

protected:
	ns_HPlayerListBox() : ns_CivListBox() {}

	AUI_ERRCODE InitCommonLdl( MBCHAR *ldlBlock );
	AUI_ERRCODE InitCommon( );
};

#endif
