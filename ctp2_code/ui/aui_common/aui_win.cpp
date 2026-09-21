#include "ctp/c3.h"
#include "ui/aui_common/aui_ui.h"
#include "ui/aui_common/aui_window.h"
#include "ui/aui_common/aui_surface.h"
#include "ui/aui_common/aui_rectangle.h"

#include "ui/aui_common/aui_win.h"


BOOL aui_Win::m_registered = FALSE;
MBCHAR *aui_Win::m_windowClass = const_cast<MBCHAR *>("aui_Win");
sint32 aui_Win::m_winRefCount = 0;
std::unique_ptr<tech_WLList<aui_Win *>> aui_Win::m_winList;
aui_Win *g_winFocus = nullptr;

aui_Win::aui_Win(
	AUI_ERRCODE *retval,
	uint32 id,
	MBCHAR const *ldlBlock,
	ControlActionCallback *ActionFunc,
	void *cookie )
	:
	aui_ImageBase( ldlBlock ),
	aui_TextBase( ldlBlock, (const MBCHAR *)nullptr ),
	aui_Control( retval, id, ldlBlock, ActionFunc, cookie )
{
	Assert( AUI_SUCCESS(*retval) );
	if ( !AUI_SUCCESS(*retval) ) return;

	*retval = InitCommonLdl( ldlBlock );
	Assert( AUI_SUCCESS(*retval) );
	if ( !AUI_SUCCESS(*retval) ) return;
}


aui_Win::aui_Win(
	AUI_ERRCODE *retval,
	uint32 id,
	sint32 x,
	sint32 y,
	sint32 width,
	sint32 height,
	ControlActionCallback *ActionFunc,
	void *cookie )
	:
	aui_ImageBase( (sint32)0 ),
	aui_TextBase( nullptr ),
	aui_Control( retval, id, x, y, width, height, ActionFunc, cookie )
{
	Assert( AUI_SUCCESS(*retval) );
	if ( !AUI_SUCCESS(*retval) ) return;

	*retval = InitCommon();
	Assert( AUI_SUCCESS(*retval) );
	if ( !AUI_SUCCESS(*retval) ) return;
}


AUI_ERRCODE aui_Win::InitCommonLdl( MBCHAR const *ldlBlock )
{
	return InitCommon();
}


AUI_ERRCODE aui_Win::InitCommon( )
{
	m_hwnd = nullptr;
	m_memdc = nullptr;
	m_hbitmap = nullptr;
	m_hbitmapOld = nullptr;
	memset( &m_offscreen, 0, sizeof( m_offscreen ) );

	if ( !m_registered )
	{
		m_registered = TRUE;
	}


	m_offscreen.x = aui_ui_Get()->Width() + 1;
	m_offscreen.y = 0;

	RECT playground;
	if ( !m_winRefCount++ )
	{
		SetRect( &playground, 0, 0, aui_ui_Get()->Width(), aui_ui_Get()->Height() );

		m_winList = std::make_unique<tech_WLList<aui_Win *>>();
		Assert( m_winList != nullptr );
		if ( !m_winList ) return AUI_ERRCODE_MEMALLOCFAILED;
	}

	m_winList->AddTail( this );

	RECT morePlayground =
	{
		m_offscreen.x,
		m_offscreen.y,
		m_offscreen.x + m_width,
		m_offscreen.y + m_height
	};

	Rectangle_Consolidate( &playground, &playground, &morePlayground );

	return AUI_ERRCODE_OK;
}


aui_Win::~aui_Win()
{




	ListPos position = m_winList->Find( this );
	if ( position )
		m_winList->DeleteAt( position );

	if ( !--m_winRefCount )
	{

		m_winList.reset();
	}
}


aui_Control *aui_Win::SetKeyboardFocus( )
{
	if ( !IsDisabled() )
		g_winFocus = this;

	return aui_Control::SetKeyboardFocus();
}


aui_Win *aui_Win::GetWinFromHWND( HWND hwnd )
{
	aui_Win *win = nullptr;

	ListPos position = m_winList->GetHeadPosition();
	for ( sint32 i = m_winList->L(); i; i-- )
	{
		win = m_winList->GetNext( position );
		if ( win->TheHWND() == hwnd )
			break;
	}

	return win;
}


AUI_ERRCODE aui_Win::DrawThis( aui_Surface *surface, sint32 x, sint32 y )
{

	if ( IsHidden() ) return AUI_ERRCODE_OK;

	if ( !surface ) surface = m_window->TheSurface();

	RECT rect = { 0, 0, m_width, m_height };
	OffsetRect( &rect, m_x + x, m_y + y );
	ToWindow( &rect );

	if ( m_hwnd && m_memdc )
	{
	}

	if ( surface == m_window->TheSurface() )
		m_window->AddDirtyRect( &rect );

	return AUI_ERRCODE_OK;
}








void aui_Win::MouseMoveInside( aui_MouseEvent *mouseData )
{
	if ( IsDisabled() ) return;
	if ( GetWhichSeesMouse() && GetWhichSeesMouse() != this )
		MouseMoveAway( mouseData );
	else if ( !IsActive() )
		MouseMoveOver( mouseData );
	else
		WinMouseMove( mouseData );
}


void aui_Win::MouseMoveOver( aui_MouseEvent *mouseData )
{
	if ( IsDisabled() ) return;

	aui_Control::MouseMoveOver( mouseData );
	if ( GetWhichSeesMouse() == this ) WinMouseMove( mouseData );
}


void aui_Win::MouseMoveAway( aui_MouseEvent *mouseData )
{
	if ( IsDisabled() ) return;
	aui_Control::MouseMoveAway( mouseData );
	WinMouseMove( mouseData );
}


void aui_Win::MouseMoveOutside( aui_MouseEvent *mouseData )
{
	if ( IsDisabled() ) return;

}


void aui_Win::WinMouseMove( aui_MouseEvent *mouseData )
{


	if ( m_mouseCode == AUI_ERRCODE_UNHANDLED )
		m_mouseCode = AUI_ERRCODE_HANDLED;
}


void aui_Win::MouseLDragOver( aui_MouseEvent *mouseData )
{
	if ( IsDisabled() ) return;
	WinMouseLDrag( mouseData );
}


void aui_Win::MouseLDragAway( aui_MouseEvent *mouseData )
{
	if ( IsDisabled() ) return;
	WinMouseLDrag( mouseData );
}


void aui_Win::MouseLDragInside( aui_MouseEvent *mouseData )
{
	if ( IsDisabled() ) return;
	WinMouseLDrag( mouseData );
}


void aui_Win::MouseLDragOutside( aui_MouseEvent *mouseData )
{
	if ( IsDisabled() ) return;
	WinMouseLDrag( mouseData );
}


void aui_Win::WinMouseLDrag( aui_MouseEvent *mouseData )
{
	if ( GetMouseOwnership() == this )
	{

		m_draw |= m_drawMask & k_AUI_REGION_DRAWFLAG_MOUSELDRAGOVER;
		if ( m_mouseCode == AUI_ERRCODE_UNHANDLED )
			m_mouseCode = AUI_ERRCODE_HANDLED;
	}
}


void aui_Win::MouseRDragOver( aui_MouseEvent *mouseData )
{
	if ( IsDisabled() ) return;
	WinMouseRDrag( mouseData );
}


void aui_Win::MouseRDragAway( aui_MouseEvent *mouseData )
{
	if ( IsDisabled() ) return;
	WinMouseRDrag( mouseData );
}


void aui_Win::MouseRDragInside( aui_MouseEvent *mouseData )
{
	if ( IsDisabled() ) return;
	WinMouseRDrag( mouseData );
}


void aui_Win::MouseRDragOutside( aui_MouseEvent *mouseData )
{
	if ( IsDisabled() ) return;
	WinMouseRDrag( mouseData );
}


void aui_Win::WinMouseRDrag( aui_MouseEvent *mouseData )
{
	if ( GetMouseOwnership() == this )
	{

		m_draw |= m_drawMask & k_AUI_REGION_DRAWFLAG_MOUSERDRAGOVER;
		if ( m_mouseCode == AUI_ERRCODE_UNHANDLED )
			m_mouseCode = AUI_ERRCODE_HANDLED;
	}
}


void aui_Win::MouseLGrabInside( aui_MouseEvent *mouseData )
{
	if ( IsDisabled() ) return;
	if ( !GetWhichSeesMouse() || GetWhichSeesMouse() == this )
	{
		SetWhichSeesMouse( this );

		PlaySound( AUI_SOUNDBASE_SOUND_ENGAGE );

		SetMouseOwnership();

		SetKeyboardFocus();



		m_draw |= m_drawMask & k_AUI_REGION_DRAWFLAG_MOUSELGRABINSIDE;
		m_mouseCode = AUI_ERRCODE_HANDLEDEXCLUSIVE;
	}
	else
		MouseLGrabOutside( mouseData );
}


void aui_Win::MouseLGrabOutside( aui_MouseEvent *mouseData )
{
	if ( IsDisabled() ) return;

	if ( GetKeyboardFocus() == this ) ReleaseKeyboardFocus();
}


void aui_Win::MouseLDropInside( aui_MouseEvent *mouseData )
{
	if ( IsDisabled() ) return;
	if ( GetMouseOwnership() == this )
	{
		ReleaseMouseOwnership();

		PlaySound( AUI_SOUNDBASE_SOUND_EXECUTE );



		WinMouseMove( mouseData );

		m_draw |= m_drawMask & k_AUI_REGION_DRAWFLAG_MOUSELDROPINSIDE;
		if ( m_mouseCode == AUI_ERRCODE_UNHANDLED )
			m_mouseCode = AUI_ERRCODE_HANDLED;








		HandleGameSpecificLeftClick( this );
	}
}

void aui_Win::MouseLDropOutside( aui_MouseEvent *mouseData )
{
	if ( IsDisabled() ) return;
	MouseLDropInside( mouseData );
}


void aui_Win::MouseRGrabInside( aui_MouseEvent *mouseData )
{
	if ( IsDisabled() ) return;
}


void aui_Win::MouseRGrabOutside( aui_MouseEvent *mouseData )
{
	if ( IsDisabled() ) return;
}


void aui_Win::MouseRDropInside( aui_MouseEvent *mouseData )
{
	if ( IsDisabled() ) return;
	if ( !GetWhichSeesMouse() || GetWhichSeesMouse() == this ) {
		SetWhichSeesMouse( this );

		HandleGameSpecificRightClick((void *)this);
		m_mouseCode = AUI_ERRCODE_HANDLED;
	}
	else {
		MouseRDropOutside( mouseData );
	}
}


void aui_Win::MouseRDropOutside( aui_MouseEvent *mouseData )
{
	if ( IsDisabled() ) return;
}


void aui_Win::MouseLDoubleClickInside( aui_MouseEvent *mouseData )
{
	if ( IsDisabled() ) return;
	if ( !GetWhichSeesMouse() || GetWhichSeesMouse() == this )
	{
		SetWhichSeesMouse( this );

		PlaySound( AUI_SOUNDBASE_SOUND_ENGAGE );

		SetMouseOwnership();



		WinMouseMove( mouseData );

		m_draw |= m_drawMask & k_AUI_REGION_DRAWFLAG_MOUSELDOUBLECLICKINSIDE;
		m_mouseCode = AUI_ERRCODE_HANDLEDEXCLUSIVE;
	}
}


void aui_Win::MouseRDoubleClickInside( aui_MouseEvent *mouseData )
{
	if ( IsDisabled() ) return;
	if ( !GetWhichSeesMouse() || GetWhichSeesMouse() == this )
	{
		SetWhichSeesMouse( this );

		PlaySound( AUI_SOUNDBASE_SOUND_ENGAGE );

		SetMouseOwnership();



		WinMouseMove( mouseData );

		m_draw |= m_drawMask & k_AUI_REGION_DRAWFLAG_MOUSERDOUBLECLICKINSIDE;
		m_mouseCode = AUI_ERRCODE_HANDLEDEXCLUSIVE;
	}
}
