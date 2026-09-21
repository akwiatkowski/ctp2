#include "ctp/c3.h"
#include "ui/aui_ctp2/c3scroller.h"

#include "ui/aui_common/aui.h"
#include "ui/aui_common/aui_uniqueid.h"
#include "ui/aui_common/aui_ldl.h"
#include "ui/aui_common/aui_surface.h"
#include "ui/aui_common/aui_window.h"
#include "ui/aui_ctp2/c3thumb.h"
#include "ui/aui_ctp2/c3ui.h"
#include "ui/aui_ctp2/textbutton.h"
#include "ui/aui_ctp2/c3_button.h"
#include "ui/aui_common/aui_ranger.h"
#include "ui/aui_ctp2/pattern.h"
#include "ui/aui_utils/primitives.h"

#include "ui/ldl/ldl_data.hpp"



C3Scroller::C3Scroller
(
	AUI_ERRCODE *retval,
	uint32 id,
	MBCHAR const *ldlBlock,
	ControlActionCallback *ActionFunc,
	void *cookie
)
:
	aui_ImageBase( ldlBlock ),
	aui_TextBase( ldlBlock, (MBCHAR *)nullptr ),
    aui_Ranger      (),
    PatternBase     (),
    m_isVertical    (false)
{
	*retval = aui_Region::InitCommonLdl( id, ldlBlock );
	Assert( AUI_SUCCESS(*retval) );
	if ( !AUI_SUCCESS(*retval) ) return;

	*retval = aui_Control::InitCommonLdl( ldlBlock, ActionFunc, cookie );
	Assert( AUI_SUCCESS(*retval) );
	if ( !AUI_SUCCESS(*retval) ) return;

	*retval = aui_Ranger::InitCommon( AUI_RANGER_TYPE_SCROLLER, AUI_RANGER_ORIENTATION_VERTICAL);
	Assert( AUI_SUCCESS(*retval) );
	if ( !AUI_SUCCESS(*retval) ) return;

	*retval = aui_SoundBase::InitCommonLdl( ldlBlock);
	Assert( AUI_SUCCESS(*retval) );
	if ( !AUI_SUCCESS(*retval) ) return;

	*retval = PatternBase::InitCommonLdl( ldlBlock, (MBCHAR *)nullptr );
	Assert( AUI_SUCCESS(*retval) );
	if ( !AUI_SUCCESS(*retval) ) return;

    ldl_datablock * block = aui_Ldl::FindDataBlock(ldlBlock);
	Assert( block != nullptr );
	if (!block)
    {
        *retval = AUI_ERRCODE_LDLFINDDATABLOCKFAILED;
        return;
    }

	m_isVertical = block->GetBool("vertical");

	*retval = InitCommon();
	Assert( AUI_SUCCESS(*retval) );
	if ( !AUI_SUCCESS(*retval) ) return;

	*retval = CreateButtonsAndThumb();
	Assert( AUI_SUCCESS(*retval) );
}


C3Scroller::C3Scroller
(
	AUI_ERRCODE *retval,
	uint32 id,
	sint32 x,
	sint32 y,
	sint32 width,
	sint32 height,
	bool isVertical,
	MBCHAR const *pattern,
	ControlActionCallback *ActionFunc,
	void *cookie
)
:
	aui_ImageBase( (sint32)0 ),
	aui_TextBase( nullptr ),
    aui_Ranger      (),
    PatternBase     (),
    m_isVertical    (isVertical)
{
	*retval = aui_Region::InitCommon( id, x, y, width, height );
	Assert( AUI_SUCCESS(*retval) );
	if ( !AUI_SUCCESS(*retval) ) return;

	*retval = aui_Control::InitCommon( ActionFunc, cookie );
	Assert( AUI_SUCCESS(*retval) );
	if ( !AUI_SUCCESS(*retval) ) return;

	*retval = aui_Ranger::InitCommon( AUI_RANGER_TYPE_SCROLLER, AUI_RANGER_ORIENTATION_VERTICAL );
	Assert( AUI_SUCCESS(*retval) );
	if ( !AUI_SUCCESS(*retval) ) return;

	*retval = aui_SoundBase::InitCommon((MBCHAR const **) nullptr);
	Assert( AUI_SUCCESS(*retval) );
	if ( !AUI_SUCCESS(*retval) ) return;

	*retval = PatternBase::InitCommon( pattern );
	Assert( AUI_SUCCESS(*retval) );
	if ( !AUI_SUCCESS(*retval) ) return;

	*retval = InitCommon();
	Assert( AUI_SUCCESS(*retval) );
	if ( !AUI_SUCCESS(*retval) ) return;

	*retval = CreateButtonsAndThumb();
	Assert( AUI_SUCCESS(*retval) );
}






AUI_ERRCODE C3Scroller::InitCommon()
{
	if (m_isVertical)
    {
		m_valX = m_minX = m_maxX = m_incX = m_pageX = 0;
    }
	else
    {
		m_valY = m_minY = m_maxY = m_incY = m_pageY = 0;
    }

	return AUI_ERRCODE_OK;
}


AUI_ERRCODE C3Scroller::CreateButtonsAndThumb( )
{
	aui_Button *button1;
	aui_Button *button2;

	AUI_ERRCODE errcode;

	m_thumb = std::make_unique<C3Thumb>( &errcode, aui_UniqueId(), 0, 0, 0, 0, m_pattern->GetFilename(), RangerThumbActionCallback, this );
	if ( m_isVertical )
	{
		m_incYButton = std::make_unique<c3_Button>( &errcode,
			aui_UniqueId(), 0, 0, 0, 0, m_pattern->GetFilename(), RangerButtonActionCallback, this );
		button1 = m_incYButton.get();
		m_decYButton = std::make_unique<c3_Button>( &errcode,
			aui_UniqueId(), 0, 0, 0, 0, m_pattern->GetFilename(), RangerButtonActionCallback, this );
		button2 = m_decYButton.get();
	}
	else
	{
		m_incXButton = std::make_unique<TextButton>( &errcode,
			aui_UniqueId(), 0, 0, 0, 0, m_pattern->GetFilename(), ">", RangerButtonActionCallback, this );
		button1 = m_incXButton.get();
		m_decXButton = std::make_unique<TextButton>( &errcode,
			aui_UniqueId(), 0, 0, 0, 0, m_pattern->GetFilename(), "<", RangerButtonActionCallback, this );
		button2 = m_decXButton.get();
	}

	AddChild( m_thumb.get() );
	AddChild( button1 );
	AddChild( button2 );

	RepositionButtons();

	RepositionThumb( FALSE );

	return AUI_ERRCODE_OK;
}


AUI_ERRCODE C3Scroller::DrawThis( aui_Surface *surface, sint32 x, sint32 y )
{




	if ( IsHidden() ) return AUI_ERRCODE_OK;

	if ( !surface ) surface = m_window->TheSurface();

	RECT rect = { 0, 0, m_width, m_height };
	OffsetRect( &rect, m_x + x, m_y + y );
	ToWindow( &rect );

	if ( m_pattern ) m_pattern->Draw( surface, &rect );












	primitives_BevelRect16( surface, &rect, 1, 1, 16, 16 );
	if ( IsActive() )
	{

		primitives_BevelRect16( surface, &rect, 1, 1, 16, 16 );
	}

	if ( surface == m_window->TheSurface() )
		m_window->AddDirtyRect( &rect );

	return AUI_ERRCODE_OK;
}
