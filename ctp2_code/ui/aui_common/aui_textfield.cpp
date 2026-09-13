#include "ctp/c3.h"
#include "ui/aui_common/aui_textfield.h"

#include "ui/aui_common/aui_ui.h"
#include "ui/aui_common/aui_window.h"
#include "ui/aui_common/aui_surface.h"
#include "ui/aui_common/aui_ldl.h"
#include "ui/aui_common/aui_action.h"

#include "sound/soundmanager.h"
#include "sound/gamesounds.h"
#include "ui/interface/chatbox.h"

#include "ui/ldl/ldl_data.hpp"

#include "ui/aui_sdl/aui_sdlsurface.h"

WNDPROC aui_TextField::m_windowProc = nullptr;
extern aui_Win* g_winFocus;

aui_TextField::aui_TextField(
	AUI_ERRCODE *retval,
	uint32 id,
	MBCHAR *ldlBlock,
	ControlActionCallback *ActionFunc,
	void *cookie )
	:
	aui_ImageBase( ldlBlock ),
	aui_TextBase( ldlBlock, (const MBCHAR *)nullptr ),
	aui_Win( retval, id, ldlBlock, ActionFunc, cookie ),
	m_Font( nullptr ),
	m_holdfont( nullptr )
{
	Assert( AUI_SUCCESS(*retval) );
	if ( !AUI_SUCCESS(*retval) ) return;

	*retval = InitCommonLdl( ldlBlock );
	Assert( AUI_SUCCESS(*retval) );
}


aui_TextField::aui_TextField(
	AUI_ERRCODE *retval,
	uint32 id,
	sint32 x,
	sint32 y,
	sint32 width,
	sint32 height,
	MBCHAR const * text,
	ControlActionCallback *ActionFunc,
	void *cookie )
	:
	aui_ImageBase( (sint32)0 ),
	aui_TextBase( nullptr ),
	aui_Win( retval, id, x, y, width, height, ActionFunc, cookie ),
	m_Font( nullptr ),
	m_holdfont( nullptr )
{
	Assert( AUI_SUCCESS(*retval) );
	if ( !AUI_SUCCESS(*retval) ) return;

	*retval = InitCommon( text, nullptr, 0, FALSE );
	Assert( AUI_SUCCESS(*retval) );
}


AUI_ERRCODE aui_TextField::InitCommonLdl( MBCHAR *ldlBlock )
{
    ldl_datablock * block = aui_Ldl::FindDataBlock(ldlBlock);
	Assert( block != nullptr );
	if ( !block ) return AUI_ERRCODE_LDLFINDDATABLOCKFAILED;

	MBCHAR *text = block->GetString( k_AUI_TEXTFIELD_LDL_TEXT );
	BOOL multiLine = block->GetBool( k_AUI_TEXTFIELD_LDL_MULTILINE );
	BOOL autovscroll =
		block->GetAttributeType( k_AUI_TEXTFIELD_LDL_AUTOVSCROLL ) == ATTRIBUTE_TYPE_BOOL ?
		block->GetBool( k_AUI_TEXTFIELD_LDL_AUTOVSCROLL ) :
		TRUE;
	BOOL autohscroll =
		block->GetAttributeType( k_AUI_TEXTFIELD_LDL_AUTOHSCROLL ) == ATTRIBUTE_TYPE_BOOL ?
		block->GetBool( k_AUI_TEXTFIELD_LDL_AUTOHSCROLL ) :
		TRUE;
	BOOL isfilename = block->GetBool( k_AUI_TEXTFIELD_LDL_ISFILENAME );
	BOOL passwordReady = block->GetBool( k_AUI_TEXTFIELD_LDL_PASSWORD );
	MBCHAR *font = block->GetString( k_AUI_TEXTFIELD_LDL_FONT );
	sint32 fontheight = block->GetInt( k_AUI_TEXTFIELD_LDL_FONT );

	sint32 maxFieldLen =
		block->GetAttributeType( k_AUI_TEXTFIELD_LDL_MAXFIELDLEN ) == ATTRIBUTE_TYPE_INT ?
		block->GetInt( k_AUI_TEXTFIELD_LDL_MAXFIELDLEN ) :
		1024;

	AUI_ERRCODE errcode = InitCommon(
		text, font, fontheight,
		multiLine,
		autovscroll,
		autohscroll,
		isfilename,
		maxFieldLen,
		passwordReady );
	Assert( AUI_SUCCESS(errcode) );
	return errcode;
}


AUI_ERRCODE aui_TextField::InitCommon(
	const MBCHAR *text,
	const MBCHAR *font,
	sint32 fontheight,
	BOOL multiLine,
	BOOL autovscroll,
	BOOL autohscroll,
	BOOL isfilename,
	sint32 maxFieldLen,
	BOOL passwordReady )
{
	m_blink = FALSE;
	m_blinkThisFrame = FALSE;
	m_multiLine = multiLine;
	m_isFileName = isfilename;
	m_maxFieldLen = maxFieldLen;
	m_passwordReady = passwordReady;
	m_textHeight = 12;
	m_Font = nullptr;
	m_holdfont = nullptr;

	if (font) strlcpy(m_desiredFont, font, sizeof(m_desiredFont));
	else strlcpy(m_desiredFont, "times.ttf", sizeof(m_desiredFont));

	if ( !m_registered ) return AUI_ERRCODE_INVALIDPARAM;

	m_Text.clear();
	if (text != nullptr)
		// TODO(phase-2): strncpy → strlcpy — dst is `char *`, capacity unknown at call site
		m_Text.assign( text, strnlen( text, m_maxFieldLen ) );
        //printf("%s L%d: aui_textfield text assigned: %s!\n", __FILE__, __LINE__, m_Text.c_str());

	// select nothing, move insertion point to end
	m_selStart = m_selEnd = (sint32) m_Text.size();

	m_Font = aui_ui_Get()->LoadBitmapFont(m_desiredFont);
	Assert(m_Font);
	// FIXME: HACK: I'm setting the font size here because it doesn't seem to be
	// being set anywhere else, which was causing textboxes to display no text.
	// With this fix they do display text, but it's usually of the wrong size.
	// More needs to be done on this problem

        //m_Font->SetMaxHeight(m_textHeight); //adjusting font to boxhight does not work
	m_Font->SetPointSize(k_AUI_TEXTBASE_DEFAULT_FONTSIZE);
	if (fontheight)
            m_textHeight = fontheight;
	else
            m_textHeight = m_Font->GetMaxHeight(); //well, let's set at least the box height to something
        //printf("%s L%d: aui_textfield text height: %d!\n", __FILE__, __LINE__, m_textHeight);


	sint32 newHeight = m_height - Mod(m_height,m_textHeight);
	if ( newHeight > 0 )
		Resize( m_width, m_height - Mod(m_height,m_textHeight) );
	else
		Resize( m_width, m_textHeight );

	return AUI_ERRCODE_OK;
}


aui_TextField::~aui_TextField()
{
	if (m_Font )
	{
		aui_ui_Get()->UnloadBitmapFont(m_Font);;
		m_Font = nullptr;
	}

}


sint32 aui_TextField::GetFieldText( MBCHAR *text, sint32 maxCount )
{
	sint32 n = std::min(m_maxFieldLen,maxCount);
	if (n <= 0)
		return 0;
	// TODO(phase-2): strncpy → strlcpy — dst is `char *`, capacity unknown at call site
	strncpy(text, m_Text.c_str(), n-1);
	text[n] = '\0';
	return strlen(text);

}


BOOL aui_TextField::SetFieldText( const MBCHAR *text )
{
	m_draw |= m_drawMask & k_AUI_REGION_DRAWFLAG_UPDATE;

	if (!text) return FALSE;
	// TODO(phase-2): strncpy → strlcpy — dst is `char *`, capacity unknown at call site
	m_Text.assign( text, strnlen( text, m_maxFieldLen ) );

	// select nothing, move insertion point to end
	m_selStart = m_selEnd = (sint32) m_Text.size();

	if ( GetKeyboardFocus() == this ) g_winFocus = this;

	return TRUE;
}


BOOL aui_TextField::SetIsFileName( BOOL isFileName )
{
	BOOL wasFileName = m_isFileName;

	if ( (m_isFileName = isFileName) != wasFileName )
	{

	}

	return wasFileName;
}


sint32 aui_TextField::SetMaxFieldLen( sint32 maxFieldLen )
{
	sint32 prevMaxFieldLen = m_maxFieldLen;

	if (maxFieldLen <= 0 || maxFieldLen == prevMaxFieldLen)
		return prevMaxFieldLen;

	m_maxFieldLen = maxFieldLen;

	// Reallocate the SDL-managed buffer so a later SetFieldText cannot
	// overflow the heap. Preserve as much of the existing content as fits,
	// then clamp the selection to the new length.
	if ( m_Text.size() > (size_t) m_maxFieldLen )
		m_Text.resize( m_maxFieldLen );
	{
		sint32 newLen = (sint32) m_Text.size();
		if (m_selStart < 0)      m_selStart = 0;
		if (m_selStart > newLen) m_selStart = newLen;
		if (m_selEnd   < 0)      m_selEnd   = 0;
		if (m_selEnd   > newLen) m_selEnd   = newLen;
	}

	return prevMaxFieldLen;
}


aui_Control *aui_TextField::SetKeyboardFocus( )
{
	if ( !IsDisabled() )
	{
		m_blink = TRUE;
		m_blinkThisFrame = FALSE;
		m_startWaitTime = 0;
	}

	return aui_Win::SetKeyboardFocus();
}


AUI_ERRCODE aui_TextField::ReleaseKeyboardFocus( )
{
	m_blink = FALSE;
	m_blinkThisFrame = FALSE;
	m_draw |= m_drawMask & k_AUI_REGION_DRAWFLAG_UPDATE;

	return aui_Win::ReleaseKeyboardFocus();
}

void aui_TextField::HitEnter()
{
	aui_TextField *textfield = this;

	if ( textfield->GetActionFunc() )
		textfield->GetActionFunc()(
			textfield,
			AUI_TEXTFIELD_ACTION_EXECUTE,
			0,
			textfield->GetCookie() );
	else if ( textfield->GetAction() )
		textfield->GetAction()->Execute(
			textfield,
			AUI_TEXTFIELD_ACTION_EXECUTE,
			0 );
}


BOOL aui_TextField::IsFileName( HWND hwnd )
{
	aui_TextField *textfield = (aui_TextField *)GetWinFromHWND( hwnd );
	Assert( textfield != nullptr );
	if ( !textfield ) return FALSE;

	return textfield->IsFileName();
}


sint32 aui_TextField::GetMaxFieldLen( HWND hwnd )
{
	aui_TextField *textfield = (aui_TextField *)GetWinFromHWND( hwnd );
	Assert( textfield != nullptr );
	if ( !textfield ) return FALSE;

	return textfield->GetMaxFieldLen();
}


AUI_ERRCODE aui_TextField::DrawThis( aui_Surface *surface, sint32 x, sint32 y )
{

	if ( IsHidden() ) return AUI_ERRCODE_OK;

	if ( !surface ) surface = m_window->TheSurface();

	RECT rect = { 0, 0, m_width, m_height };
	RECT srcRect = rect;
	OffsetRect( &rect, m_x + x, m_y + y );
	ToWindow( &rect );

	SDL_Surface* SDLsurf = static_cast<aui_SDLSurface*>(surface)->DDS();
	// fill background
	SDL_Rect r1 = { rect.left, rect.top, rect.right-rect.left, rect.bottom-rect.top };
	SDL_FillRect(SDLsurf, &r1, CTP2_SDL_MapRGB(SDLsurf, 0xff, 0xff, 0xff));

	m_Font->DrawString(surface, &rect, &rect, m_Text.c_str(),
	                   k_AUI_BITMAPFONT_DRAWFLAG_JUSTLEFT,
	                   RGB(20,20,20), 0);
	// Clamp the cursor index against the actual text length before indexing.
	// m_selStart can be left out-of-range by SetSelection callers or by
	// stale state across resize/text changes; reading past the buffer here
	// caused a heap-buffer-overflow in aui_textfield.cpp:533.
	sint32 textLen = (sint32) m_Text.size();
	sint32 selPos  = m_selStart < 0      ? 0
	               : m_selStart > textLen ? textLen
	               : m_selStart;
	char save = m_Text[selPos];
	m_Text[selPos] = '\0';
	int offset = m_Font->GetStringWidth(m_Text.c_str());
	m_Text[selPos] = save;
	SDL_Rect r2 = { rect.left+offset-1, rect.top+2, 2, rect.bottom-rect.top-4 };
	SDL_FillRect(SDLsurf, &r2, 0);

	if ( surface == m_window->TheSurface() )
		m_window->AddDirtyRect( &rect );

	return AUI_ERRCODE_OK;
}








void aui_TextField::PostChildrenCallback( aui_MouseEvent *mouseData )
{

	if ( !mouseData->framecount
	&&   GetKeyboardFocus() == this )
	{

		g_winFocus = this;

		m_blinkThisFrame = TRUE;
		m_draw |= m_drawMask & k_AUI_REGION_DRAWFLAG_UPDATE;
	}

	if ( m_blinkThisFrame && mouseData->time - m_startWaitTime > m_timeOut )
	{
		m_blinkThisFrame = FALSE;
		m_startWaitTime = mouseData->time;
		m_blink = !m_blink;
	}
}


void aui_TextField::MouseLGrabOutside( aui_MouseEvent *mouseData )
{
	if ( IsDisabled() ) return;
	aui_Win::MouseLGrabOutside( mouseData );

	if ( GetActionFunc() )
		GetActionFunc()(
			this,
			AUI_TEXTFIELD_ACTION_DISMISS,
			0,
			GetCookie() );
	else if ( GetAction() )
		GetAction()->Execute(
			this,
			AUI_TEXTFIELD_ACTION_DISMISS,
			0 );
}

void aui_TextField::SetSelection(sint32 start, sint32 end)
{
	// Clamp the selection range to [0, strlen(m_Text)] so DrawThis cannot
	// index past the buffer.
	sint32 textLen = (sint32) m_Text.size();
	if (start < 0)       start = 0;
	if (end   < 0)       end   = 0;
	if (start > textLen) start = textLen;
	if (end   > textLen) end   = textLen;
	m_selStart = start;
	m_selEnd   = end;
}

void aui_TextField::GetSelection(sint32 *start, sint32 *end)
{
	*start = m_selStart;
	*end = m_selEnd;
}

void aui_TextField::SelectAll()
{
	SetSelection(9999, 9999);
}







#include "ui/aui_sdl/aui_sdlcompat.h"

void aui_TextField::KeyboardCallback(aui_KeyboardEvent *keyboardData)
{
	if (!keyboardData->down) {
		// Play typing sound on key release, matching Windows WM_KEYUP behavior
		soundmgr_Get()->AddGameSound(GAMESOUNDS_EDIT_TEXT);
		return;
	}


	uint32 key = keyboardData->key;

	// Handle special keys
	switch (key) {
		case AUI_KEYBOARD_KEY_RETURN:
			HitEnter();
			return;
		case AUI_KEYBOARD_KEY_TAB:
			// Ignore tab - UI framework handles focus switching
			return;
		case AUI_KEYBOARD_KEY_LEFTARROW:
			if (m_selStart > 0) {
				m_selStart--;
				m_selEnd = m_selStart;
				m_draw |= m_drawMask & k_AUI_REGION_DRAWFLAG_UPDATE;
			}
			return;
		case AUI_KEYBOARD_KEY_RIGHTARROW:
		{
			sint32 len = (sint32) m_Text.size();
			if (m_selStart < len) {
				m_selStart++;
				m_selEnd = m_selStart;
				m_draw |= m_drawMask & k_AUI_REGION_DRAWFLAG_UPDATE;
			}
			return;
		}
		case AUI_KEYBOARD_KEY_SPACE:
			key = ' ';
			break;
	}

	// Handle backspace (ASCII 8)
	if (key == 8) {
		if (m_selStart > 0) {
			m_Text.erase( m_selStart - 1, 1 );
			m_selStart--;
			m_selEnd = m_selStart;
			m_draw |= m_drawMask & k_AUI_REGION_DRAWFLAG_UPDATE;
		}
		return;
	}

	// Handle delete (ASCII 127)
	if (key == 127) {
		sint32 len = (sint32) m_Text.size();
		if (m_selStart < len) {
			m_Text.erase( m_selStart, 1 );
			m_draw |= m_drawMask & k_AUI_REGION_DRAWFLAG_UPDATE;
		}
		return;
	}

	// Handle printable ASCII characters (32-126)
	if (key >= 32 && key < 127) {
		char ch = (char)key;

		// Apply shift to letters
		if (ch >= 'a' && ch <= 'z') {
			SDL_Keymod mod = SDL_GetModState();
			if (mod & KMOD_SHIFT) {
				ch = ch - 'a' + 'A';
			}
		}

		// Filename character filtering
		if (m_isFileName) {
			switch (ch) {
				case '\\': case '*': case '"': case '/':
				case ':': case '|': case '?': case '<':
				case '>':
					return;
			}
		}

		sint32 len = (sint32) m_Text.size();
		if (len >= m_maxFieldLen) {
			return;
		}

		// Clamp cursor position to valid range
		if (m_selStart < 0) m_selStart = 0;
		if (m_selStart > len) m_selStart = len;

		// Insert character at cursor position
		m_Text.insert( m_selStart, 1, ch );
		m_selStart++;
		m_selEnd = m_selStart;
		m_draw |= m_drawMask & k_AUI_REGION_DRAWFLAG_UPDATE;
	}
}
