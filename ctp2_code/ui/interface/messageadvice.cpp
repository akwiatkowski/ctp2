#include "ctp/c3.h"
#include <memory>

#include "ui/aui_common/aui.h"
#include "ui/aui_ctp2/c3ui.h"
#include "ui/aui_common/aui_button.h"
#include "ui/aui_common/aui_static.h"
#include "ui/aui_common/aui_ldl.h"
#include "ui/aui_common/aui_uniqueid.h"

#include "ui/aui_ctp2/c3listbox.h"

#include "ui/interface/messageactions.h"
#include "ui/interface/messagewindow.h"
#include "ui/interface/messageadvice.h"


std::unique_ptr<MessageAdvice>	g_adviceMessageWindow;

int messageadvice_AddText( MBCHAR *text )
{
	AUI_ERRCODE	errcode = AUI_ERRCODE_OK;
	MBCHAR		windowBlock[ k_AUI_LDL_MAXBLOCK + 1 ];

	if ( !g_adviceMessageWindow ) {
		strlcpy( windowBlock, "AdviceWindow", sizeof(windowBlock) );

		g_adviceMessageWindow = std::make_unique<MessageAdvice>( &errcode, aui_UniqueId(),
											windowBlock, 16 );
		Assert( AUI_NEWOK( g_adviceMessageWindow, errcode ));
		if ( !AUI_NEWOK( g_adviceMessageWindow, errcode )) return -1;
		g_adviceMessageWindow->SetDraggable( TRUE );

		c3ui_Get()->AddWindow( g_adviceMessageWindow.get() );

		g_adviceMessageWindow->AddBordersToUI();
	}

	g_adviceMessageWindow->AppendText( text );

	return 1;
}

int messageadvice_DestroyWindow( )
{
	if ( g_adviceMessageWindow )
	{
		if ( c3ui_Get()->GetWindow( g_adviceMessageWindow->Id() ))
			c3ui_Get()->RemoveWindow( g_adviceMessageWindow->Id() );

		g_adviceMessageWindow->RemoveBordersFromUI();

		g_adviceMessageWindow.reset();
	}

	return 1;
}


MessageAdvice::MessageAdvice(
	AUI_ERRCODE *retval,
	uint32 id,
	MBCHAR *ldlBlock,
	sint32 bpp,
	AUI_WINDOW_TYPE type )
	:
	C3Window(retval, id, ldlBlock, bpp, type)
{
	*retval = InitCommon( ldlBlock );
	Assert( AUI_SUCCESS(*retval) );
	if ( !AUI_SUCCESS(*retval) ) return;
}


AUI_ERRCODE MessageAdvice::InitCommon( MBCHAR *ldlBlock )
{
	AUI_ERRCODE errcode = AUI_ERRCODE_OK;

	m_leftBar = nullptr;
	m_topBar = nullptr;
	m_rightBar = nullptr;
	m_bottomBar = nullptr;
	m_listBox = nullptr;

	errcode = CreateWindowEdges( ldlBlock );
	Assert( errcode == AUI_ERRCODE_OK );
	if ( errcode != AUI_ERRCODE_OK ) return errcode;

	errcode = CreateDismissButton( ldlBlock );
	Assert( errcode == AUI_ERRCODE_OK );
	if ( errcode != AUI_ERRCODE_OK ) return errcode;

	MakeSureSurfaceIsValid( );

	errcode = CreateTextBox( ldlBlock );
	Assert( errcode == AUI_ERRCODE_OK );
	if ( errcode != AUI_ERRCODE_OK ) return errcode;

	SetDraggable( TRUE );

	return AUI_ERRCODE_OK;
}

AUI_ERRCODE MessageAdvice::CreateWindowEdges( MBCHAR *ldlBlock )
{
	MBCHAR		imageBlock[ k_AUI_LDL_MAXBLOCK + 1 ];
	AUI_ERRCODE		errcode = AUI_ERRCODE_OK;

	snprintf(imageBlock, sizeof(imageBlock), "%s.%s", ldlBlock, "MessageLeftBar" );
	m_leftBar = std::make_unique<aui_Static>( &errcode, aui_UniqueId(), imageBlock );
	Assert( AUI_NEWOK( m_leftBar, errcode ));
	if ( !AUI_NEWOK( m_leftBar, errcode )) return AUI_ERRCODE_MEMALLOCFAILED;
	m_leftBar->SetImageBltType( AUI_IMAGEBASE_BLTTYPE_TILE );
	AddControl(m_leftBar.get());

	snprintf(imageBlock, sizeof(imageBlock), "%s.%s", ldlBlock, "MessageRightBar" );
	m_rightBar = std::make_unique<aui_Static>( &errcode, aui_UniqueId(), imageBlock );
	Assert( AUI_NEWOK( m_rightBar, errcode ));
	if ( !AUI_NEWOK( m_rightBar, errcode )) return AUI_ERRCODE_MEMALLOCFAILED;
	m_rightBar->SetImageBltType( AUI_IMAGEBASE_BLTTYPE_TILE );
	AddControl(m_rightBar.get());

	strlcpy( imageBlock, "FancyAdviceTopBar", sizeof(imageBlock) );
	m_topBar = std::make_unique<C3Window>( &errcode, aui_UniqueId(), imageBlock, 16, AUI_WINDOW_TYPE_FLOATING, false );
	Assert( AUI_NEWOK( m_topBar, errcode ));
	if ( !AUI_NEWOK( m_topBar, errcode )) return AUI_ERRCODE_MEMALLOCFAILED;
	m_topBar->SetTransparent( TRUE );
	m_topBar->SetBlindness( TRUE );

	m_offsetTop.x = m_topBar->X();
	m_offsetTop.y = m_topBar->Y();
	m_topBar->Offset( m_x, m_y );

	snprintf(imageBlock, sizeof(imageBlock), "FancyAdviceBottomBar" );
	m_bottomBar = std::make_unique<C3Window>( &errcode, aui_UniqueId(), imageBlock, 16, AUI_WINDOW_TYPE_FLOATING, false );
	Assert( AUI_NEWOK( m_bottomBar, errcode ));
	if ( !AUI_NEWOK( m_bottomBar, errcode )) return AUI_ERRCODE_MEMALLOCFAILED;
	m_bottomBar->SetTransparent( TRUE );

	m_offsetBottom.x = m_bottomBar->X();
	m_offsetBottom.y = m_bottomBar->Y();
	m_bottomBar->Offset( m_x, m_y );

	return AUI_ERRCODE_OK;
}

AUI_ERRCODE MessageAdvice::AppendText( MBCHAR *text )
{
	AUI_ERRCODE errcode = AUI_ERRCODE_OK;
	aui_Static *item;

	item = std::make_unique<aui_Static>( &errcode, aui_UniqueId(), "AdviceWindowItem" ).release();
	Assert( AUI_NEWOK( item, errcode ));
	if ( !AUI_NEWOK( item, errcode )) return AUI_ERRCODE_MEMALLOCFAILED;

	item->SetText( text );

	m_listBox->AddItem((aui_Item *)item );

	return AUI_ERRCODE_OK;

}


AUI_ERRCODE MessageAdvice::CreateDismissButton( MBCHAR *ldlBlock )
{
	AUI_ERRCODE		errcode = AUI_ERRCODE_OK;
	MBCHAR			buttonBlock[ k_AUI_LDL_MAXBLOCK + 1 ];

	snprintf(buttonBlock, sizeof(buttonBlock), "%s.%s", ldlBlock, "DismissButton" );
	m_dismissButton = std::make_unique<aui_Button>( &errcode, aui_UniqueId(), buttonBlock );
	Assert( AUI_NEWOK( m_dismissButton, errcode ));
	if ( !AUI_NEWOK( m_dismissButton, errcode )) return AUI_ERRCODE_MEMALLOCFAILED;

	errcode = AddControl( m_dismissButton.get() );
	Assert( errcode == AUI_ERRCODE_OK );
	if ( errcode != AUI_ERRCODE_OK ) return errcode;

	m_dismissAction = std::make_unique<MessageAdviceDismissAction>( );
	Assert( m_dismissAction != nullptr );
	if ( m_dismissAction == nullptr ) return AUI_ERRCODE_MEMALLOCFAILED;

	m_dismissButton->SetAction( m_dismissAction.get() );

	return AUI_ERRCODE_OK;
}


AUI_ERRCODE MessageAdvice::CreateTextBox( MBCHAR *ldlBlock )
{
	MBCHAR			textBlock[ k_AUI_LDL_MAXBLOCK + 1 ];
	AUI_ERRCODE		errcode = AUI_ERRCODE_OK;

	snprintf(textBlock, sizeof(textBlock), "%s.%s", ldlBlock, "MessageTextBox" );
	m_listBox = std::make_unique<C3ListBox>( &errcode, aui_UniqueId(), textBlock );
	Assert( AUI_NEWOK( m_listBox, errcode ));
	if ( !AUI_NEWOK( m_listBox, errcode )) return AUI_ERRCODE_MEMALLOCFAILED;

	m_listBox->SetForceSelect( FALSE );
	m_listBox->SetMultiSelect( FALSE );

	AddControl(m_listBox.get());

	return AUI_ERRCODE_OK;
}


MessageAdvice::~MessageAdvice()
{
	// The bars and the list box free themselves. The list ITEMS do not: the
	// box holds them as plain pointers, so they are still deleted by hand.
	if ( m_listBox ) {
		aui_Static *item = nullptr;
		sint32 count = m_listBox->NumItems();

		for ( sint32 i = 0; i < count; i++ ) {
			item = (aui_Static *)m_listBox->GetItemByIndex( i );

			if ( item ) {
				std::unique_ptr<aui_Static>{item};
				item = nullptr;
			}
		}

	}

}


void MessageAdvice::MouseLGrabInside (aui_MouseEvent *mouseData)
{
	if ( IsDisabled() ) return;
	C3Window::MouseLGrabInside(mouseData);

	BringBorderToTop();

}


void MessageAdvice::MouseLDragAway (aui_MouseEvent *mouseData)
{
	if ( IsDisabled() ) return;
	C3Window::MouseLDragAway(mouseData);

	if ( m_topBar )
		m_topBar->Move( m_offsetTop.x + m_x, m_offsetTop.y + m_y );

	if ( m_bottomBar )
		m_bottomBar->Move( m_offsetBottom.x + m_x, m_offsetBottom.y + m_y );

}


void MessageAdvice::BringBorderToTop()
{
	if ( m_topBar )
		c3ui_Get()->BringWindowToTop( m_topBar.get() );

	if ( m_bottomBar )
		c3ui_Get()->BringWindowToTop( m_bottomBar.get() );
}


AUI_ERRCODE MessageAdvice::AddBordersToUI()
{
	AUI_ERRCODE errcode = AUI_ERRCODE_OK;

	if ( m_topBar )
	{
		errcode = c3ui_Get()->AddWindow(m_topBar.get());
		Assert( errcode == AUI_ERRCODE_OK );
		if(	errcode != AUI_ERRCODE_OK ) return errcode;
	}

	if ( m_bottomBar )
	{
		errcode = c3ui_Get()->AddWindow(m_bottomBar.get());
		Assert( errcode == AUI_ERRCODE_OK );
		if(	errcode != AUI_ERRCODE_OK ) return errcode;
	}

	return AUI_ERRCODE_OK;
}


AUI_ERRCODE MessageAdvice::RemoveBordersFromUI()
{
	AUI_ERRCODE errcode = AUI_ERRCODE_OK;

	if ( m_topBar )
	{
		errcode = c3ui_Get()->RemoveWindow( m_topBar->Id( ));
		Assert( errcode == AUI_ERRCODE_OK );
		if ( errcode != AUI_ERRCODE_OK ) return errcode;
	}

	if ( m_bottomBar )
	{
		errcode = c3ui_Get()->RemoveWindow( m_bottomBar->Id( ));
		Assert( errcode == AUI_ERRCODE_OK );
		if ( errcode != AUI_ERRCODE_OK ) return errcode;
	}

	return AUI_ERRCODE_OK;
}
