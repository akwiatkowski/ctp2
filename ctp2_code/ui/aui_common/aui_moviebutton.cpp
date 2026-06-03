#include "ctp/c3.h"
#include "ui/aui_common/aui_moviebutton.h"

#include "ui/aui_common/aui_ui.h"
#include "ui/aui_common/aui_window.h"
#include "ui/aui_common/aui_ldl.h"
#include "ui/aui_common/aui_movie.h"

#include "ui/aui_ctp2/c3ui.h"

#include "gs/database/profileDB.h"

#include "ui/ldl/ldl_data.hpp"

aui_MovieButton::aui_MovieButton(
	AUI_ERRCODE *retval,
	uint32 id,
	MBCHAR *ldlBlock,
	ControlActionCallback *ActionFunc,
	void *cookie )
	:
	aui_ImageBase( ldlBlock ),
	aui_TextBase( ldlBlock, (const MBCHAR *)nullptr ),
	aui_Button( retval, id, ldlBlock, ActionFunc, cookie )
{
	m_flags = 0;
	m_fullScreen = false;

	Assert( AUI_SUCCESS(*retval) );
	if ( !AUI_SUCCESS(*retval) ) return;

	*retval = InitCommonLdl( ldlBlock );
	Assert( AUI_SUCCESS(*retval) );
	if ( !AUI_SUCCESS(*retval) ) return;
}


aui_MovieButton::aui_MovieButton(
	AUI_ERRCODE *retval,
	uint32 id,
	sint32 x,
	sint32 y,
	sint32 width,
	sint32 height,
	MBCHAR *movie,
	ControlActionCallback *ActionFunc,
	void *cookie )
	:
	aui_ImageBase( (sint32)0 ),
	aui_TextBase( nullptr ),
	aui_Button( retval, id, x, y, width, height, ActionFunc, cookie )
{
	m_flags = 0;
	m_fullScreen = false;

	Assert( AUI_SUCCESS(*retval) );
	if ( !AUI_SUCCESS(*retval) ) return;

	*retval = InitCommon( movie );
	Assert( AUI_SUCCESS(*retval) );
	if ( !AUI_SUCCESS(*retval) ) return;
}


AUI_ERRCODE aui_MovieButton::InitCommonLdl( MBCHAR *ldlBlock )
{
    ldl_datablock * block = aui_Ldl::FindDataBlock(ldlBlock);
	Assert( block != nullptr );
	if ( !block ) return AUI_ERRCODE_LDLFINDDATABLOCKFAILED;

	AUI_ERRCODE errcode = InitCommon(
		block->GetString( k_AUI_MOVIEBUTTON_LDL_MOVIE ) );
	Assert( AUI_SUCCESS(errcode) );
	return errcode;
}


AUI_ERRCODE aui_MovieButton::InitCommon( MBCHAR *movie )
{
	m_movie = nullptr;

	SetMovie( movie );


	m_drawMask = 0;

	return AUI_ERRCODE_OK;
}


aui_MovieButton::~aui_MovieButton()
{
	if ( m_movie )
	{
		aui_ui_Get()->UnloadMovie( m_movie );
		m_movie = nullptr;
	}
}


aui_Movie *aui_MovieButton::SetMovie( const MBCHAR *movie )
{
	aui_Movie *prevMovie = m_movie;

	if ( movie )
	{
		m_movie = aui_ui_Get()->LoadMovie(movie);
		Assert( m_movie != nullptr );
		if ( !m_movie )
		{
			m_movie = prevMovie;
			return nullptr;
		}


		m_movie->SetDestSurface( m_window ? m_window->TheSurface() : nullptr );
		m_movie->SetDestRect( m_x, m_y, m_x + m_width, m_y + m_height );














		if (m_window) {
			m_window->SetDynamic(FALSE);
			m_movie->SetWindowSurface(m_window->TheSurface());

			RECT windowRect = {m_x, m_y, m_x + m_width, m_y + m_height};
			m_movie->SetWindowRect(&windowRect);
		}
	}
	else
		m_movie = nullptr;

	if ( prevMovie ) aui_ui_Get()->UnloadMovie( prevMovie );

	return prevMovie;
}


AUI_ERRCODE aui_MovieButton::Idle( )
{
	if ( m_movie )
	{

		if ( !m_movie->GetDestSurface() ) {
			m_movie->SetDestSurface( m_window->TheSurface() );
		}

		if ( !m_movie->IsOpen() ) {
			uint32 flags = m_flags;

			if (m_fullScreen) {
				flags |= k_AUI_MOVIE_PLAYFLAG_ONSCREEN;
			}

			RECT adjustedRect = {m_x, m_y, m_x+m_width, m_y+m_height};

			ToScreen(&adjustedRect);

			if (m_movie->Open(flags, c3ui_Get()->Secondary(), &adjustedRect) != AUI_ERRCODE_OK) {

				SetMovie(nullptr);
				if (GetActionFunc())
					GetActionFunc()(this, AUI_BUTTON_ACTION_EXECUTE, 0, nullptr);
			}
		}

		if (m_movie)
		{
			if ( !m_movie->IsPlaying() && !m_movie->IsFinished())
			{
				m_movie->Play();
			}

			(void) m_movie->Process();

			if (m_movie->IsFinished() && !(m_flags & k_AUI_MOVIE_PLAYFLAG_PLAYANDHOLD))
			{
				if (m_ActionFunc)
					m_ActionFunc((aui_Control *)this, AUI_BUTTON_ACTION_EXECUTE, 0, nullptr);
			}
		}
	}

	return AUI_ERRCODE_OK;
}
