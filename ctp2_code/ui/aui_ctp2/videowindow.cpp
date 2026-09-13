#include "ctp/c3.h"
#include "ui/aui_common/aui.h"

#include "ctp/ctp2_utils/c3errors.h"
#include "ui/aui_common/aui_control.h"
#include "ui/aui_ctp2/pattern.h"

#include "ui/aui_utils/primitives.h"

#include "ui/aui_common/aui_Factory.h"
#include "gs/fileio/CivPaths.h"
#include "ui/aui_ctp2/c3ui.h"

#include "ui/aui_ctp2/videowindow.h"




VideoWindow::VideoWindow(
	AUI_ERRCODE *retval,
	uint32 id,
	sint32 x,
	sint32 y,
	sint32 width,
	sint32 height,
	sint32 bpp,
	MBCHAR *pattern,
	MBCHAR *name,
	BOOL modal,
	AUI_WINDOW_TYPE type )
	:
	C3Window()
{

	*retval = aui_Region::InitCommon( id, x, y, width, height );
	Assert( AUI_SUCCESS(*retval) );
	if ( !AUI_SUCCESS(*retval) ) return;

	*retval = aui_Window::InitCommon( bpp, type );
	Assert( AUI_SUCCESS(*retval) );
	if ( !AUI_SUCCESS(*retval) ) return;

	*retval = C3Window::InitCommon();
	Assert( AUI_SUCCESS(*retval) );
	if ( !AUI_SUCCESS(*retval) ) return;

	*retval = PatternBase::InitCommon( pattern );
	Assert( AUI_SUCCESS(*retval) );
	if ( !AUI_SUCCESS(*retval) ) return;

	*retval = InitCommon();
	Assert( AUI_SUCCESS(*retval) );
	if ( !AUI_SUCCESS(*retval) ) return;

	*retval = CreateVideoSurface( name, modal );
	Assert( AUI_SUCCESS(*retval) );
	if ( !AUI_SUCCESS(*retval) ) return;
}


AUI_ERRCODE VideoWindow::InitCommon( )
{
	m_modal = FALSE;

	return AUI_ERRCODE_OK;
}


AUI_ERRCODE VideoWindow::CreateVideoSurface(MBCHAR *name, BOOL modal)
{
	HRESULT			hr;
	AUI_ERRCODE		errcode;


	return AUI_ERRCODE_OK;
}


VideoWindow::~VideoWindow()
{
}


AUI_ERRCODE VideoWindow::Idle()
{
	return AUI_ERRCODE_OK;
}

AUI_ERRCODE VideoWindow::DrawThis(aui_Surface *surface, sint32 x, sint32 y)
{

	if ( IsHidden() ) return AUI_ERRCODE_OK;

	RECT rect = { 0, 0, m_width, m_height };

	m_pattern->Draw( m_surface, &rect );

	primitives_BevelRect16( m_surface, &rect, 1, 0, 16, 16 );

	primitives_DropText(m_surface, 5, 3, m_filename, 0xFFFF, TRUE);

	rect.top += 20;

	InflateRect(&rect, -5, -5);
	primitives_BevelRect16( m_surface, &rect, 5, 1, 16, 16 );

	m_dirtyList->AddRect( &rect );

	return AUI_ERRCODE_OK;
}
