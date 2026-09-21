//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ source
// Description  : The civilization 3 ranger
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
// - Initialized local variables. (Sep 9th 2005 Martin Gühmann)
//
//----------------------------------------------------------------------------

#include "ctp/c3.h"
#include "ui/aui_ctp2/c3_ranger.h"

#include "ui/aui_common/aui.h"
#include "ui/aui_common/aui_ldl.h"
#include "ui/aui_common/aui_ranger.h"
#include "ui/aui_common/aui_static.h"
#include "ui/aui_common/aui_ui.h"
#include "ui/aui_common/aui_uniqueid.h"
#include "ui/aui_ctp2/c3_button.h"
#include "ui/aui_common/aui_window.h"
#include "ui/aui_common/aui_dimension.h"

#include "ui/aui_ctp2/c3_thumb.h"

#include "ui/aui_ctp2/pattern.h"
#include "ui/aui_ctp2/patternbase.h"
#include "ui/aui_utils/primitives.h"

#include "ui/ldl/ldl_file.hpp"



c3_Ranger::c3_Ranger()
:
	aui_Ranger()
{
}


c3_Ranger::c3_Ranger(
	AUI_ERRCODE *retval,
	uint32 id,
	MBCHAR const *ldlBlock,
	ControlActionCallback *ActionFunc,
	void *cookie )
	:
	aui_ImageBase( ldlBlock ),
	aui_TextBase( ldlBlock, (MBCHAR *)nullptr ),
	aui_Ranger()
{
	*retval = aui_Region::InitCommonLdl( id, ldlBlock );
	Assert( AUI_SUCCESS(*retval) );
	if ( !AUI_SUCCESS(*retval) ) return;

	*retval = aui_Control::InitCommonLdl( ldlBlock, ActionFunc, cookie );
	Assert( AUI_SUCCESS(*retval) );
	if ( !AUI_SUCCESS(*retval) ) return;

	*retval = aui_Ranger::InitCommonLdl(ldlBlock);
	Assert( AUI_SUCCESS(*retval) );
	if ( !AUI_SUCCESS(*retval) ) return;

	*retval = aui_SoundBase::InitCommonLdl( ldlBlock);
	Assert( AUI_SUCCESS(*retval) );
	if ( !AUI_SUCCESS(*retval) ) return;

	*retval = PatternBase::InitCommonLdl( ldlBlock, (MBCHAR *)nullptr );
	Assert( AUI_SUCCESS(*retval) );
	if ( !AUI_SUCCESS(*retval) ) return;

	*retval = InitCommonLdl( ldlBlock );
	Assert( AUI_SUCCESS(*retval) );
	if ( !AUI_SUCCESS(*retval) ) return;

	*retval = CreateButtonsAndThumb(ldlBlock);
	Assert( AUI_SUCCESS(*retval) );
	if ( !AUI_SUCCESS(*retval) ) return;
}


c3_Ranger::c3_Ranger(
	AUI_ERRCODE *retval,
	uint32 id,
	sint32 x,
	sint32 y,
	sint32 width,
	sint32 height,
	AUI_RANGER_TYPE type,
	AUI_RANGER_ORIENTATION orientation,
	MBCHAR const *pattern,
	ControlActionCallback *ActionFunc,
	void *cookie )
	:
	aui_ImageBase( (sint32)0 ),
	aui_TextBase( nullptr ),
	aui_Ranger()
{
	*retval = aui_Region::InitCommon( id, x, y, width, height );
	Assert( AUI_SUCCESS(*retval) );
	if ( !AUI_SUCCESS(*retval) ) return;

	*retval = aui_Control::InitCommon( ActionFunc, cookie );
	Assert( AUI_SUCCESS(*retval) );
	if ( !AUI_SUCCESS(*retval) ) return;

	*retval = aui_Ranger::InitCommon( type, orientation );
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

	*retval = CreateButtonsAndThumb(nullptr);
	Assert( AUI_SUCCESS(*retval) );
	if ( !AUI_SUCCESS(*retval) ) return;
}


AUI_ERRCODE c3_Ranger::InitCommonLdl( MBCHAR const *ldlBlock )
{
	AUI_ERRCODE errcode = InitCommon();
	Assert( AUI_SUCCESS(errcode) );
	if ( !AUI_SUCCESS(errcode) ) return errcode;

	return AUI_ERRCODE_OK;
}


AUI_ERRCODE c3_Ranger::InitCommon( )
{
	for (auto &arrow : m_arrows) arrow.reset();

	return AUI_ERRCODE_OK;
}


AUI_ERRCODE c3_Ranger::CreateButtonsAndThumb( MBCHAR const *ldlBlock )
{
	AUI_ERRCODE errcode = AUI_ERRCODE_OK;
    MBCHAR *    patternFilename = (m_pattern) ? m_pattern->GetFilename() : nullptr;

	static MBCHAR block[ k_AUI_LDL_MAXBLOCK + 1 ];

	sint32 maxButtonSize = 0;

	if ( m_type == AUI_RANGER_TYPE_SLIDER
	||   m_type == AUI_RANGER_TYPE_SCROLLER )
	{
		if ( ldlBlock )
		{
			snprintf(block, sizeof(block), "%s.%s", ldlBlock, k_AUI_RANGER_LDL_THUMB );

			if (aui_Ldl::GetLdl()->FindDataBlock( block ) )
				m_thumb = std::make_unique<c3_Thumb>(
					&errcode,
					aui_UniqueId(),
					block,
					RangerThumbActionCallback,
					this );
		}

		if ( !m_thumb )
			m_thumb = std::make_unique<c3_Thumb>(
				&errcode,
				aui_UniqueId(),
				0, 0, 0, 0,
				patternFilename,
				RangerThumbActionCallback,
				this );

		Assert( AUI_NEWOK(m_thumb,errcode) );
		if ( !AUI_NEWOK(m_thumb,errcode) )
			return AUI_ERRCODE_MEMALLOCFAILED;

		AddChild( m_thumb.get() );

		RepositionThumb( FALSE );
	}

	if ( m_type == AUI_RANGER_TYPE_SCROLLER
	||   m_type == AUI_RANGER_TYPE_SPINNER )
	{
		if ( m_orientation == AUI_RANGER_ORIENTATION_HORIZONTAL
		||   m_orientation == AUI_RANGER_ORIENTATION_BIDIRECTIONAL )
		{
			if ( ldlBlock )
			{
				snprintf(block, sizeof(block), "%s.%s", ldlBlock, k_AUI_RANGER_LDL_INCX );

                if (aui_Ldl::GetLdl()->FindDataBlock( block ) )
					m_incXButton = std::make_unique<c3_Button>(
						&errcode,
						aui_UniqueId(),
						block,
						RangerButtonActionCallback,
						this );
			}

			if ( !m_incXButton ) {
				m_incXButton = std::make_unique<c3_Button>(
					&errcode,
					aui_UniqueId(),
					0, 0, 0, 0,
					patternFilename,
					RangerButtonActionCallback,
					this );
			}

			Assert( AUI_NEWOK(m_incXButton,errcode) );
			if ( !AUI_NEWOK(m_incXButton,errcode) )
				return AUI_ERRCODE_MEMALLOCFAILED;

			sint32 i = 2;
			snprintf(block, sizeof(block), "RangerRight");
            if (aui_Ldl::GetLdl()->FindDataBlock(block))
			{
				m_arrows[ i ] = std::make_unique<aui_Static>(
					&errcode,
					aui_UniqueId(),
					block );
				Assert( AUI_NEWOK(m_arrows[ i ],errcode) );
				if ( !AUI_NEWOK(m_arrows[ i ],errcode) )
					return AUI_ERRCODE_MEMALLOCFAILED;

				m_arrows[ i ]->SetBlindness( TRUE );
				m_incXButton->AddChild( m_arrows[ i ].get() );

				m_arrows[ i ]->GetDim()->SetParent( m_incXButton.get() );
			}

			AddChild( m_incXButton.get() );

			if ( m_incXButton->Width() > maxButtonSize )
				maxButtonSize = m_incXButton->Width();

			if ( ldlBlock )
			{
				snprintf(block, sizeof(block), "%s.%s", ldlBlock, k_AUI_RANGER_LDL_DECX );

                if (aui_Ldl::GetLdl()->FindDataBlock( block ) )
					m_decXButton = std::make_unique<c3_Button>(
						&errcode,
						aui_UniqueId(),
						block,
						RangerButtonActionCallback,
						this );
			}

			if ( !m_decXButton ) {
				m_decXButton = std::make_unique<c3_Button>(
					&errcode,
					aui_UniqueId(),
					0, 0, 0, 0,
					patternFilename,
					RangerButtonActionCallback,
					this );
			}

			Assert( AUI_NEWOK(m_decXButton,errcode) );
			if ( !AUI_NEWOK(m_decXButton,errcode) )
				return AUI_ERRCODE_MEMALLOCFAILED;

			i = 3;
			snprintf(block, sizeof(block), "RangerLeft" );
            if (aui_Ldl::GetLdl()->FindDataBlock(block))
			{
				m_arrows[ i ] = std::make_unique<aui_Static>(
					&errcode,
					aui_UniqueId(),
					block );
				Assert( AUI_NEWOK(m_arrows[ i ],errcode) );
				if ( !AUI_NEWOK(m_arrows[ i ],errcode) )
					return AUI_ERRCODE_MEMALLOCFAILED;

				m_arrows[ i ]->SetBlindness( TRUE );
				m_decXButton->AddChild( m_arrows[ i ].get() );

				m_arrows[ i ]->GetDim()->SetParent( m_decXButton.get() );
			}

			AddChild( m_decXButton.get() );

			if ( m_decXButton->Width() > maxButtonSize )
				maxButtonSize = m_decXButton->Width();
		}

		if ( m_orientation == AUI_RANGER_ORIENTATION_VERTICAL
		||   m_orientation == AUI_RANGER_ORIENTATION_BIDIRECTIONAL )
		{
			if ( ldlBlock )
			{
				snprintf(block, sizeof(block), "%s.%s", ldlBlock, k_AUI_RANGER_LDL_INCY );

                if (aui_Ldl::GetLdl()->FindDataBlock( block ) )
					m_incYButton = std::make_unique<c3_Button>(
						&errcode,
						aui_UniqueId(),
						block,
						RangerButtonActionCallback,
						this );
			}

			if ( !m_incYButton )
				m_incYButton = std::make_unique<c3_Button>(
					&errcode,
					aui_UniqueId(),
					0, 0, 0, 0,
					patternFilename,
					RangerButtonActionCallback,
					this );

			Assert( AUI_NEWOK(m_incYButton,errcode) );
			if ( !AUI_NEWOK(m_incYButton,errcode) )
				return AUI_ERRCODE_MEMALLOCFAILED;

			sint32 i = 0;
			snprintf(block, sizeof(block), "RangerDown");
            if (aui_Ldl::GetLdl()->FindDataBlock(block))
			{
				m_arrows[ i ] = std::make_unique<aui_Static>(
					&errcode,
					aui_UniqueId(),
					block );
				Assert( AUI_NEWOK(m_arrows[ i ],errcode) );
				if ( !AUI_NEWOK(m_arrows[ i ],errcode) )
					return AUI_ERRCODE_MEMALLOCFAILED;

				m_arrows[ i ]->SetBlindness( TRUE );
				m_incYButton->AddChild( m_arrows[ i ].get() );

				m_arrows[ i ]->GetDim()->SetParent( m_incYButton.get() );
			}

			AddChild( m_incYButton.get() );

			if ( m_incYButton->Height() > maxButtonSize )
				maxButtonSize = m_incYButton->Height();

			if ( ldlBlock )
			{
				snprintf(block, sizeof(block), "%s.%s", ldlBlock, k_AUI_RANGER_LDL_DECY );

                if (aui_Ldl::GetLdl()->FindDataBlock( block ) )
					m_decYButton = std::make_unique<c3_Button>(
						&errcode,
						aui_UniqueId(),
						block,
						RangerButtonActionCallback,
						this );
			}

			if ( !m_decYButton )
				m_decYButton = std::make_unique<c3_Button>(
					&errcode,
					aui_UniqueId(),
					0, 0, 0, 0,
					patternFilename,
					RangerButtonActionCallback,
					this );

			Assert( AUI_NEWOK(m_decYButton,errcode) );
			if ( !AUI_NEWOK(m_decYButton,errcode) )
				return AUI_ERRCODE_MEMALLOCFAILED;

			i = 1;
			snprintf(block, sizeof(block), "RangerUp");
            if (aui_Ldl::GetLdl()->FindDataBlock(block))
			{
				m_arrows[ i ] = std::make_unique<aui_Static>(
					&errcode,
					aui_UniqueId(),
					block );
				Assert( AUI_NEWOK(m_arrows[ i ],errcode) );
				if ( !AUI_NEWOK(m_arrows[ i ],errcode) )
					return AUI_ERRCODE_MEMALLOCFAILED;

				m_arrows[ i ]->SetBlindness( TRUE );
				m_decYButton->AddChild( m_arrows[ i ].get() );

				m_arrows[ i ]->GetDim()->SetParent( m_decYButton.get() );
			}

			AddChild( m_decYButton.get() );

			if ( m_decYButton->Height() > maxButtonSize )
				maxButtonSize = m_decYButton->Height();
		}

		if ( maxButtonSize )
			SetButtonSize( maxButtonSize );
		else
			RepositionButtons();
	}

	return AUI_ERRCODE_OK;
}


c3_Ranger::~c3_Ranger()
{
	for (auto & m_arrow : m_arrows)
    {
	    if ( m_arrow )
	    {
		    aui_Ldl::Remove(m_arrow.get());

		    m_arrow.reset();
	    }
    }
}


AUI_ERRCODE c3_Ranger::RepositionButtons( )
{
	AUI_ERRCODE errcode = aui_Ranger::RepositionButtons();


	for (auto & m_arrow : m_arrows)
	if ( m_arrow )
		m_arrow->Adjust();

	return errcode;
}


AUI_ERRCODE c3_Ranger::DrawThis( aui_Surface *surface, sint32 x, sint32 y )
{

	if ( IsHidden() ) return AUI_ERRCODE_OK;

	if ( !surface ) surface = m_window->TheSurface();

	RECT rect = { 0, 0, m_width, m_height };
	OffsetRect( &rect, m_x + x, m_y + y );
	ToWindow( &rect );

	if ( m_pattern ) m_pattern->Draw( surface, &rect );

	primitives_BevelRect16( surface, &rect, 1, 1, 16, 16 );

	if ( surface == m_window->TheSurface() )
		m_window->AddDirtyRect( &rect );

	return AUI_ERRCODE_OK;
}
