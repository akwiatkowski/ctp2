//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ source
// Description  : Activision User Interface movie window
// Id           : $Id$
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
// - None
//
//----------------------------------------------------------------------------
//
// Modifications from the original Activision code:
//
// - Initialized local variables. (Sep 9th 2005 Martin G�hmann)
//
//----------------------------------------------------------------------------

#include "ctp/c3.h"
#include "ui/aui_common/aui_ui.h"
#include "ui/aui_common/aui_surface.h"
#include "ui/aui_common/aui_rectangle.h"
#include "ui/aui_common/aui_mouse.h"

#include "ui/aui_common/aui_movie.h"

#include "ui/aui_sdl/aui_sdlcompat.h"
#include "ui/aui_sdl/aui_sdlmixercompat.h"
#include "sound/soundmanager.h"		// soundmgr_Get()
#include "ui/aui_sdl/aui_sdlsurface.h"

WNDPROC aui_Movie::m_windowProc = nullptr;
aui_Movie *aui_Movie::m_onScreenMovie = nullptr;


aui_Movie::aui_Movie(
	AUI_ERRCODE *retval,
	MBCHAR const * filename )
	:
	aui_Base()
{
	*retval = InitCommon( filename );
	Assert( AUI_SUCCESS(*retval) );
}


AUI_ERRCODE aui_Movie::InitCommon( MBCHAR const * filename )
{
	m_format = nullptr;
	m_surface = nullptr;
	m_isOpen = FALSE;
	m_isPlaying = FALSE;
	m_isFinished = FALSE;
	m_isPaused = FALSE;
	m_flags = 0;
	m_timePerFrame = 0;
	m_lastFrameTime = 0;

	m_windowSurface = nullptr;
	memset( &m_rect, 0, sizeof( m_rect ) );
	memset(&m_windowRect, 0, sizeof(m_windowRect));

	m_curFrame = 0;

	AUI_ERRCODE errcode = SetFilename( filename );
	Assert( AUI_SUCCESS(errcode) );
	if ( !AUI_SUCCESS(errcode) ) return errcode;


	return AUI_ERRCODE_OK;
}


aui_Movie::~aui_Movie()
{
	Unload();

}


AUI_ERRCODE aui_Movie::SetFilename( MBCHAR const *filename )
{

	Unload();

	memset( m_filename, '\0', sizeof( m_filename ) );

	if ( !filename ) return AUI_ERRCODE_INVALIDPARAM;

	strlcpy( m_filename, filename, sizeof( m_filename ) );

	m_format = (aui_MovieFormat *)
		aui_ui_Get()->TheMemMap()->GetFileFormat( m_filename );
	Assert( m_format != nullptr );
	if ( !m_format ) return AUI_ERRCODE_MEMALLOCFAILED;

	return AUI_ERRCODE_OK;
}


AUI_ERRCODE aui_Movie::Load( )
{
	Assert(m_format);
	if ( !m_format ) return AUI_ERRCODE_INVALIDPARAM;

	return m_format->Load( m_filename, this );
}


AUI_ERRCODE aui_Movie::Unload( )
{
	Close();

	aui_ui_Get()->TheMemMap()->ReleaseFileFormat(m_format);
	m_format = nullptr;

	return AUI_ERRCODE_OK;
}


aui_Surface *aui_Movie::SetDestSurface( aui_Surface *surface )
{
	aui_Surface *   prevSurface = m_surface;
    m_surface   = surface;

	if (m_surface)
	{
		RECT surfRect =
		{
			0,
			0,
			m_surface->Width(),
			m_surface->Height()
		};

		Rectangle_Clip( &m_rect, &surfRect );
	}

	return prevSurface;
}


void aui_Movie::SetDestRect( RECT *rect )
{
	if ( rect )
		SetDestRect( rect->left, rect->top, rect->right, rect->bottom );
	else
		SetDestRect( 0, 0, 0, 0 );
}


void aui_Movie::SetDestRect(
	sint32 left,
	sint32 top,
	sint32 right,
	sint32 bottom )
{
	m_rect.left = left;
	m_rect.top = top;
	m_rect.right = right;
	m_rect.bottom = bottom;

	if ( m_surface )
	{
		RECT surfRect =
		{
			0,
			0,
			m_surface->Width(),
			m_surface->Height()
		};

		Rectangle_Clip( &m_rect, &surfRect );
	}
}


void aui_Movie::GetDestRect(
	sint32 *left,
	sint32 *top,
	sint32 *right,
	sint32 *bottom ) const
{
	*left = m_rect.left;
	*top = m_rect.top;
	*right = m_rect.right;
	*bottom = m_rect.bottom;
}


uint32 aui_Movie::SetTimePerFrame( uint32 timePerFrame )
{
	uint32 prevTimePerFrame = m_timePerFrame;
	m_timePerFrame = timePerFrame;
	return prevTimePerFrame;
}


AUI_ERRCODE aui_Movie::Open(
	uint32 flags,
	aui_Surface *surface,
	RECT *rect )
{
	if ( !m_isOpen )
	{
		m_flags = flags;

		if ( surface )
			SetDestSurface( surface );
		if ( rect )
			SetDestRect( rect );

		m_isOpen = TRUE;
		m_isPlaying = FALSE;
		m_isPaused = FALSE;
	}

	return AUI_ERRCODE_OK;
}


AUI_ERRCODE aui_Movie::Close( )
{
	if ( m_isOpen )
	{

		Stop();


		m_isOpen = FALSE;
	}

	return AUI_ERRCODE_OK;
}


AUI_ERRCODE aui_Movie::Play( )
{
	if ( !m_isPlaying )
	{

		Open();

		m_isPlaying = TRUE;
		m_isPaused = FALSE;


		if ( m_flags & k_AUI_MOVIE_PLAYFLAG_ONSCREEN )
			PlayOnScreenMovie();
	}

	return AUI_ERRCODE_OK;
}





AUI_ERRCODE aui_Movie::PlayOnScreenMovie( )
{
	aui_Mouse *mouse = aui_ui_Get()->TheMouse();
	sint32 numEvents;
	static aui_MouseEvent mouseEvents[ k_MOUSE_MAXINPUT ];
	aui_MouseEvent *mouseState = nullptr;

	if (mouse) {

		numEvents = mouse->ManipulateInputs( mouseEvents, FALSE );

		mouseState = numEvents ?
			mouseEvents + numEvents - 1 :
			mouse->GetLatestMouseEvent();

		mouse->Hide();
	}


	m_onScreenMovie = this;

	while ( !m_isFinished && m_isPlaying )
	{

		Process();


		if (mouse) {
			numEvents = mouse->ManipulateInputs( mouseEvents, FALSE );
			aui_MouseEvent *curEvent = mouseEvents;
			for ( sint32 k = numEvents; k; k--, curEvent++ )
			{
				if ( curEvent->lbutton != mouseState->lbutton
				||   curEvent->rbutton != mouseState->rbutton )
				{
					Stop();
					break;
				}
			}
		}
	}

	m_onScreenMovie = nullptr;


	if (mouse)
		mouse->Show();

	aui_ui_Get()->AddDirtyRect( &m_rect );

	return Stop();
}


AUI_ERRCODE aui_Movie::Stop( )
{
	if ( m_isPlaying )
	{

		m_isPlaying = FALSE;
		m_isPaused = FALSE;
	}

	return AUI_ERRCODE_OK;
}


AUI_ERRCODE aui_Movie::Pause()
{
	if (m_isPlaying)
	{
		m_isPaused = TRUE;
	}

	return AUI_ERRCODE_OK;
}


AUI_ERRCODE aui_Movie::Resume()
{
	if (m_isPlaying)
	{
		m_isPaused = FALSE;
	}

	return AUI_ERRCODE_OK;
}


AUI_ERRCODE aui_Movie::Process( )
{
	AUI_ERRCODE retval = AUI_ERRCODE_UNHANDLED;

	if ( m_isPlaying && !m_isPaused )
	{
		uint32 time = GetTickCount();
		if ( time - m_lastFrameTime > m_timePerFrame )
		{


			m_lastFrameTime = time;

			retval = AUI_ERRCODE_HANDLED;
		}
	}

	return retval;
}


LRESULT CALLBACK OnScreenMovieWindowProc(
	HWND hwnd,
	UINT message,
	WPARAM wParam,
	LPARAM lParam )
{
	if ( aui_Movie::m_onScreenMovie )
	{
		aui_ui_Get()->HandleWindowsMessage( hwnd, message, wParam, lParam );

		switch ( message )
		{

		case WM_LBUTTONUP:
		case WM_MBUTTONUP:
		case WM_RBUTTONUP:
		case WM_KEYUP:
		case WM_KEYDOWN:
		case WM_SYSKEYUP:
		case WM_SYSKEYDOWN:
			aui_Movie::m_onScreenMovie->Stop();
			break;

		case WM_CLOSE:
			if ( hwnd != aui_ui_Get()->TheHWND() ) break;

			aui_Movie::m_onScreenMovie->Close();


			return 0;
		}
	}
	LRESULT lr = 0;

	return lr;
}
