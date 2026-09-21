#include "ctp/c3.h"
#include "ui/netshell/ns_headerswitch.h"

#include "ui/aui_common/aui_ui.h"
#include "ui/aui_common/aui_blitter.h"
#include "ui/aui_common/aui_surface.h"
#include "ui/aui_common/aui_window.h"
#include "ui/aui_common/aui_action.h"
#include "ui/aui_common/aui_ldl.h"

#include "ui/aui_ctp2/pattern.h"
#include "ui/aui_utils/primitives.h"

#include "ui/ldl/ldl_data.hpp"

ns_HeaderSwitch::ns_HeaderSwitch(
	AUI_ERRCODE *retval,
	uint32 id,
	MBCHAR *ldlBlock,
	ControlActionCallback *ActionFunc,
	void *cookie )
	:
	aui_ImageBase( ldlBlock ),
	aui_TextBase( ldlBlock, (MBCHAR *)nullptr ),
	aui_Radio(
		retval,
		id,
		ldlBlock,
		ActionFunc,
		cookie ),
	PatternBase( ldlBlock, (MBCHAR *)nullptr )
{
	Assert( AUI_SUCCESS(*retval) );
	if ( !AUI_SUCCESS(*retval) ) return;

	*retval = InitCommonLdl( ldlBlock );
	Assert( AUI_SUCCESS(*retval) );
	if ( !AUI_SUCCESS(*retval) ) return;
}


ns_HeaderSwitch::ns_HeaderSwitch(
	AUI_ERRCODE *retval,
	uint32 id,
	sint32 x,
	sint32 y,
	sint32 width,
	sint32 height,
	MBCHAR *text,
	MBCHAR *icon,
	MBCHAR *pattern,
	ControlActionCallback *ActionFunc,
	void *cookie,
	sint32 state,
	sint32 numStates )
	:
	aui_ImageBase( numStates ),
	aui_TextBase( text ),
	aui_Radio(
		retval,
		id,
		x,
		y,
		width,
		height,
		ActionFunc,
		cookie,
		state,
		numStates ),
	PatternBase( pattern )
{
	Assert( AUI_SUCCESS(*retval) );
	if ( !AUI_SUCCESS(*retval) ) return;

	*retval = InitCommon(icon);
}


AUI_ERRCODE ns_HeaderSwitch::InitCommonLdl( MBCHAR *ldlBlock )
{
	ldl_datablock * block = aui_Ldl::FindDataBlock(ldlBlock);
	Assert( block != nullptr );
	if ( !block ) return AUI_ERRCODE_LDLFINDDATABLOCKFAILED;

	return InitCommon(block->GetString(k_NS_HEADERSWITCH_LDL_ICON));
}


AUI_ERRCODE ns_HeaderSwitch::InitCommon( MBCHAR const *icon )
{
	m_icon = nullptr;

	if ( icon ) SetIcon( icon );

	SetNumStates( k_NS_HEADERSWITCH_DEFAULTNUMSTATES );

	return AUI_ERRCODE_OK;
}


ns_HeaderSwitch::~ns_HeaderSwitch()
{
	if ( m_icon )
	{
		aui_ui_Get()->UnloadImage( m_icon );
		m_icon = nullptr;
	}
}


AUI_ERRCODE ns_HeaderSwitch::SetIcon( MBCHAR const *icon )
{
	Assert( icon != nullptr );
	if ( !icon ) return AUI_ERRCODE_INVALIDPARAM;

	aui_Image *prevImage = m_icon;

	m_icon = aui_ui_Get()->LoadImage( icon );
	Assert( m_icon != nullptr );
	if ( !m_icon ) return AUI_ERRCODE_LOADFAILED;

	if ( prevImage ) aui_ui_Get()->UnloadImage( prevImage );

	return AUI_ERRCODE_OK;
}


AUI_ERRCODE ns_HeaderSwitch::DrawThis(
	aui_Surface *surface,
	sint32 x,
	sint32 y )
{

	if ( IsHidden() ) return AUI_ERRCODE_OK;

	if ( !surface ) surface = m_window->TheSurface();

	RECT rect = { 0, 0, m_width, m_height };
	OffsetRect( &rect, m_x + x, m_y + y );
	ToWindow( &rect );

	if ( m_pattern )
		m_pattern->Draw( surface, &rect );


	aui_Radio::DrawThis( surface, x, y );

	if ( IsOn() )
		primitives_BevelRect16( surface, &rect, 2, 1, 16, 16 );
	else
		primitives_BevelRect16( surface, &rect, 2, 0, 16, 16 );

	if ( m_icon )
	{
		RECT srcRect =
		{
			0,
			0,
			m_icon->TheSurface()->Width(),
			m_icon->TheSurface()->Height()
		};

		aui_ui_Get()->TheBlitter()->Blt(
			surface,
			( rect.right + rect.left - srcRect.right ) / 2,
			( rect.bottom + rect.top - srcRect.bottom ) / 2,
			m_icon->TheSurface(),
			&srcRect,
			k_AUI_BLITTER_FLAG_CHROMAKEY );
	}

	if ( surface == m_window->TheSurface() )
		m_window->AddDirtyRect( &rect );

	return AUI_ERRCODE_OK;
}
