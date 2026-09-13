//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ header
// Description  : User interface text field
//
//----------------------------------------------------------------------------
//
// Disclaimer
//
// THIS FILE IS NOT GENERATED OR SUPPORTED BY ACTIVISION.
//
// This material has been developed at apolyton.net by the Apolyton CtP2
// Source Code Project. Contact the authors at ctp2source@apolyton.net.
//
//----------------------------------------------------------------------------
//
// Compiler flags
//
// _MSC_VER
// - Use Microsoft C++ extensions when set.
//
//----------------------------------------------------------------------------
//
// Modifications from the original Activision code:
//
// - Event handlers declared in a notation that is more standard C++.
//
//----------------------------------------------------------------------------

#ifndef __AUI_TEXTFIELD_H__
#define __AUI_TEXTFIELD_H__

#include <string>
#include "ui/aui_common/aui_bitmapfont.h"
#include "ui/aui_common/aui_win.h"


enum AUI_TEXTFIELD_ACTION
{
	AUI_TEXTFIELD_ACTION_FIRST = 0,
	AUI_TEXTFIELD_ACTION_NULL = 0,
	AUI_TEXTFIELD_ACTION_EXECUTE,
	AUI_TEXTFIELD_ACTION_DISMISS,
	AUI_TEXTFIELD_ACTION_LAST
};


#define k_AUI_TEXTFIELD_LDL_TEXT		"fieldtext"
#define k_AUI_TEXTFIELD_LDL_MULTILINE	"multiline"
#define k_AUI_TEXTFIELD_LDL_AUTOVSCROLL	"autovscroll"
#define k_AUI_TEXTFIELD_LDL_AUTOHSCROLL	"autohscroll"
#define k_AUI_TEXTFIELD_LDL_ISFILENAME	"isfilename"
#define k_AUI_TEXTFIELD_LDL_PASSWORD	"password"
#define k_AUI_TEXTFIELD_LDL_FONT		"fontname"
#define k_AUI_TEXTFIELD_LDL_FONTHEIGHT	"fontheight"
#define k_AUI_TEXTFIELD_LDL_MAXFIELDLEN	"maxfieldlen"


class aui_TextField : public aui_Win
{
public:

	aui_TextField(
		AUI_ERRCODE *retval,
		uint32 id,
		MBCHAR *ldlBlock,
		ControlActionCallback *ActionFunc = nullptr,
		void *cookie = nullptr );
	aui_TextField(
		AUI_ERRCODE *retval,
		uint32 id,
		sint32 x,
		sint32 y,
		sint32 width,
		sint32 height,
		const MBCHAR *text = nullptr,
		ControlActionCallback *ActionFunc = nullptr,
		void *cookie = nullptr );
	~aui_TextField() override;

protected:
	aui_TextField() : aui_Win() {}
	AUI_ERRCODE InitCommonLdl( MBCHAR *ldlBlock );
	AUI_ERRCODE InitCommon(
		const MBCHAR *text,
		const MBCHAR *font,
		sint32 fontheight,
		BOOL multiLine,
		BOOL autovscroll = TRUE,
		BOOL autohscroll = TRUE,
		BOOL isfilename = FALSE,
		sint32 maxFieldLen = 1024,
		BOOL passwordReady = FALSE);

public:
	sint32	GetFieldText( MBCHAR *text, sint32 maxCount );
	BOOL	SetFieldText( const MBCHAR *text );


	BOOL	IsFileName( ) const { return m_isFileName; }
	BOOL	SetIsFileName( BOOL isFileName );

	sint32	GetMaxFieldLen( ) const { return m_maxFieldLen; }
	sint32	SetMaxFieldLen( sint32 maxFieldLen );

	aui_Control	*SetKeyboardFocus( ) override;
	AUI_ERRCODE	ReleaseKeyboardFocus( ) override;

	AUI_ERRCODE	DrawThis(
		aui_Surface *surface,
		sint32 x,
		sint32 y ) override;

	static WNDPROC	m_windowProc;
	void HitEnter();
	static BOOL IsFileName( HWND hwnd );
	static sint32 GetMaxFieldLen( HWND hwnd );


	void SetSelection(sint32 start, sint32 end);
	void GetSelection(sint32 *start, sint32 *end);
	void SelectAll();

protected:
	BOOL	m_blink;
	BOOL	m_blinkThisFrame;

	BOOL	m_multiLine;
	BOOL	m_passwordReady;

	BOOL	m_isFileName;

	sint32	m_maxFieldLen;
	std::string m_Text;
	sint32  m_selStart;
	sint32  m_selEnd;

public:
	sint32	m_textHeight;
	MBCHAR	m_desiredFont[256];
	aui_BitmapFont *m_Font;
	aui_BitmapFont *m_holdfont;

	void	MouseLGrabOutside(aui_MouseEvent * mouseData) override;
	void	PostChildrenCallback(aui_MouseEvent * mouseData) override;

	void	KeyboardCallback(aui_KeyboardEvent * keyboardData) override;
};


void TextFieldWindowProc(SDL_Event &event);

#endif
