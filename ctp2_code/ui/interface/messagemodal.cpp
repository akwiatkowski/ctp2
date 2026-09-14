#include "ctp/c3.h"
#include "ui/interface/messagemodal.h"

#include "ui/aui_common/aui.h"
#include "ui/aui_ctp2/c3ui.h"
#include "ui/aui_common/aui_button.h"
#include "ui/aui_common/aui_static.h"
#include "ui/aui_common/aui_ldl.h"
#include "ui/aui_common/aui_uniqueid.h"
#include "ui/aui_common/aui_hypertextbox.h"

#include "ui/aui_ctp2/c3_popupwindow.h"

#include "gs/slic/SlicButton.h"
#include "ui/aui_ctp2/ctp2_button.h"
#include "ui/aui_common/aui_dimension.h"

#include "gs/gameobj/message.h"
#include "gs/gameobj/MessageData.h"
#include "ui/interface/messageactions.h"
#include "ui/interface/messagewindow.h"
#include "ui/interface/messageeyepoint.h"
#include "gs/gameobj/player.h"             // player_Get()
#include "ui/aui_ctp2/SelItem.h"            // selitem_Get()


extern sint32 g_ScreenHeight;

MessageModal		*g_modalMessage = nullptr;

int messagemodal_CreateModalMessage( Message data )
{
	AUI_ERRCODE	errcode = AUI_ERRCODE_OK;
	MBCHAR		windowBlock[ k_AUI_LDL_MAXBLOCK + 1 ];

	if ( g_modalMessage ) return 1;

	uint32 count = data.AccessData()->GetNumButtons();
	Assert( count > 0 );
	if ( count == 0 ) return -1;

	strlcpy( windowBlock, "ModalWindow", sizeof(windowBlock) );

	g_modalMessage = new MessageModal( &errcode, aui_UniqueId(),
										windowBlock, 16, data );
	Assert( AUI_NEWOK( g_modalMessage, errcode ));
	if ( !AUI_NEWOK( g_modalMessage, errcode )) return -1;
	g_modalMessage->SetDraggable( TRUE );

	if(g_ScreenHeight >= 768) {
		g_modalMessage->Move(0, 228);
	}

	c3ui_Get()->AddWindow( g_modalMessage );

	g_modalMessage->AddBordersToUI();

	return 1;
}

void messagemodal_PrepareDestroyWindow()
{
	c3ui_Get()->AddDestructiveAction( new MessageModalDestroyAction());
}

void messagemodal_DestroyModalMessage( )
{
	if ( g_modalMessage )
	{
		if ( c3ui_Get()->GetWindow( g_modalMessage->Id() ))
			c3ui_Get()->RemoveWindow( g_modalMessage->Id() );

		g_modalMessage->RemoveBordersFromUI();

		delete g_modalMessage;
		g_modalMessage = nullptr;


		if(player_arr_Get() && player_Get(selitem_Get()->GetVisiblePlayer())) {
			player_Get(selitem_Get()->GetVisiblePlayer())->NotifyModalMessageDestroyed();
		}
	}
}


MessageModal::MessageModal(
	AUI_ERRCODE *retval,
	uint32 id,
	MBCHAR *ldlBlock,
	sint32 bpp,
	Message data,
	AUI_WINDOW_TYPE type )
	:
	c3_PopupWindow(retval, id, ldlBlock, bpp, type)
{
	*retval = InitCommon( ldlBlock, data );
	Assert( AUI_SUCCESS(*retval) );
	if ( !AUI_SUCCESS(*retval) ) return;
}


AUI_ERRCODE MessageModal::InitCommon( MBCHAR *ldlBlock, Message data )
{
	AUI_ERRCODE errcode = AUI_ERRCODE_OK;

	m_message = data;

	m_leftBar = nullptr;
	m_topBar = nullptr;
	m_rightBar = nullptr;
	m_bottomBar = nullptr;

	SetStronglyModal( TRUE );
	SetDraggable( TRUE );

	errcode = CreateResponses( ldlBlock );
	Assert( errcode == AUI_ERRCODE_OK );
	if ( errcode != AUI_ERRCODE_OK ) return errcode;





	errcode = CreateEyePointBox( ldlBlock );
	Assert( errcode == AUI_ERRCODE_OK );
	if ( errcode != AUI_ERRCODE_OK ) return errcode;


	MakeSureSurfaceIsValid( );

	errcode = CreateStandardTextBox( ldlBlock );
	Assert( errcode == AUI_ERRCODE_OK );
	if ( errcode != AUI_ERRCODE_OK ) return errcode;

	return AUI_ERRCODE_OK;
}













































AUI_ERRCODE MessageModal::CreateStandardTextBox( MBCHAR *ldlBlock )
{
	MBCHAR			textBlock[ k_AUI_LDL_MAXBLOCK + 1 ];
	AUI_ERRCODE		errcode = AUI_ERRCODE_OK;

	snprintf(textBlock, sizeof(textBlock), "%s.%s", ldlBlock, "MessageTextBox" );
	m_messageText.reset(new aui_HyperTextBox( &errcode, aui_UniqueId(), textBlock ));
	Assert( AUI_NEWOK( m_messageText, errcode ));
	if ( !AUI_NEWOK( m_messageText, errcode )) return AUI_ERRCODE_MEMALLOCFAILED;

	m_messageText->SetDrawMask( k_AUI_REGION_DRAWFLAG_UPDATE );

	AddControl( m_messageText.get() );

	errcode = m_messageText->SetHyperText( m_message.GetText( ));
	Assert( errcode == AUI_ERRCODE_OK );
	if ( errcode != AUI_ERRCODE_OK ) return errcode;

	return AUI_ERRCODE_OK;
}

AUI_ERRCODE MessageModal::CreateEyePointBox( MBCHAR *ldlBlock )
{
	switch( m_message.AccessData()->GetEyePointStyle())
	{
	case MESSAGE_EYEPOINT_STYLE_NONE:
		return AUI_ERRCODE_OK;
	case MESSAGE_EYEPOINT_STYLE_STANDARD:
		return CreateStandardEyePointBox( ldlBlock );
	case MESSAGE_EYEPOINT_STYLE_DROPDOWN:
		return CreateDropdownEyePointBox( ldlBlock );
	case MESSAGE_EYEPOINT_STYLE_LIST:
		return CreateListboxEyePointBox( ldlBlock );
	default:
		return AUI_ERRCODE_OK;
	}
}


AUI_ERRCODE MessageModal::CreateStandardEyePointBox( MBCHAR *ldlBlock )
{
	AUI_ERRCODE		errcode = AUI_ERRCODE_OK;

	m_eyePointStandard.reset(new MessageEyePointStandard( &errcode, ldlBlock, this ));
	Assert( AUI_NEWOK( m_eyePointStandard, errcode ));
	if ( !AUI_NEWOK( m_eyePointStandard, errcode ))
	{
		m_eyePointStandard.reset();
		return AUI_ERRCODE_MEMALLOCFAILED;
	}

	return AUI_ERRCODE_OK;
}


AUI_ERRCODE MessageModal::CreateDropdownEyePointBox( MBCHAR *ldlBlock )
{
	AUI_ERRCODE		errcode = AUI_ERRCODE_OK;

	m_eyePointDropdown.reset(new MessageEyePointDropdown( &errcode, ldlBlock, this ));
	Assert( AUI_NEWOK( m_eyePointDropdown, errcode ));
	if ( !AUI_NEWOK( m_eyePointDropdown, errcode ))
	{
		m_eyePointDropdown.reset();
		return AUI_ERRCODE_MEMALLOCFAILED;
	}

	return AUI_ERRCODE_OK;
}


AUI_ERRCODE MessageModal::CreateListboxEyePointBox( MBCHAR *ldlBlock )
{
	AUI_ERRCODE		errcode = AUI_ERRCODE_OK;

	m_eyePointListbox.reset(new MessageEyePointListbox( &errcode, ldlBlock, this ));
	Assert( AUI_NEWOK( m_eyePointListbox, errcode ));
	if ( !AUI_NEWOK( m_eyePointListbox, errcode ))
	{
		m_eyePointListbox.reset();
		return AUI_ERRCODE_MEMALLOCFAILED;
	}

	return AUI_ERRCODE_OK;
}

AUI_ERRCODE MessageModal::CreateResponses( MBCHAR *ldlBlock )
{
	AUI_ERRCODE		errcode = AUI_ERRCODE_OK;
	ctp2_Button		*lastbutton = nullptr;
	MBCHAR			buttonBlock[ k_AUI_LDL_MAXBLOCK + 1 ];
	snprintf(buttonBlock, sizeof(buttonBlock), "%s.%s", ldlBlock, "ModalResponseButton");
	sint32			responseCount = 0;

	while (SlicButton * sButton = m_message.AccessData()->GetButton(responseCount))
    {
		std::unique_ptr<ctp2_Button> button(new ctp2_Button(&errcode, aui_UniqueId(), buttonBlock));
		Assert( AUI_NEWOK( button, errcode ));
		if ( !AUI_NEWOK( button, errcode )) return AUI_ERRCODE_MEMALLOCFAILED;

		button->TextReloadFont();
		button->Enable(TRUE);

		aui_BitmapFont *    font        = button->GetTextFont();
		Assert(font);

		MBCHAR const *      text        = sButton->GetName();
		sint32              textlength  = font->GetStringWidth(text);

		button->Resize((textlength + k_MODAL_BUTTON_TEXT_PADDING), button->Height());
		button->SetText(text);

		if ( lastbutton ) {
			button->Move( lastbutton->X() -
						  button->Width() -
						  k_MODAL_BUTTON_SPACING, button->Y() );
		} else {
			button->Move(Width() - button->Width() - k_MODAL_BUTTON_SPACING - button->GetDim()->HorizontalPositionData(), button->Y());
		}

		std::unique_ptr<MessageModalResponseAction> action(
			new MessageModalResponseAction( &m_message, responseCount ));

		button->SetAction( action.get() );

		AddControl( button.get() );

		button->ResetThis();

		lastbutton = button.get();

		m_responseButtons.push_back(std::move(button));
		m_responseActions.push_back(std::move(action));

		responseCount++;
    }

	return AUI_ERRCODE_OK;
}


MessageModal::~MessageModal ()
{
	// Buttons and actions must go before the base window tears down, in the
	// order the hand-written teardown used; the eye-point helper (which owns
	// its own button/dropdown/action set) is released with the members and
	// fixes the old leak where it was never freed at all.
	m_responseButtons.clear();
	m_responseActions.clear();
	m_messageText.reset();
	m_eyePointStandard.reset();
	m_eyePointDropdown.reset();
	m_eyePointListbox.reset();
}


void MessageModal::MouseLGrabInside (aui_MouseEvent *mouseData)
{
	if ( IsDisabled() ) return;
	C3Window::MouseLGrabInside(mouseData);

	BringBorderToTop();

}


void MessageModal::MouseLDragAway (aui_MouseEvent *mouseData)
{
	if ( IsDisabled() ) return;
	C3Window::MouseLDragAway(mouseData);

	if ( m_topBar )
		m_topBar->Move( m_offsetTop.x + m_x, m_offsetTop.y + m_y );

	if ( m_bottomBar )
		m_bottomBar->Move( m_offsetBottom.x + m_x, m_offsetBottom.y + m_y );

}


void MessageModal::BringBorderToTop()
{
	if ( m_topBar )
		c3ui_Get()->BringWindowToTop( m_topBar );

	if ( m_bottomBar )
		c3ui_Get()->BringWindowToTop( m_bottomBar );
}


AUI_ERRCODE MessageModal::AddBordersToUI()
{
	AUI_ERRCODE errcode = AUI_ERRCODE_OK;

	if ( m_topBar )
	{
		errcode = c3ui_Get()->AddWindow( m_topBar );
		Assert( errcode == AUI_ERRCODE_OK );
		if(	errcode != AUI_ERRCODE_OK ) return errcode;
	}

	if ( m_bottomBar )
	{
		errcode = c3ui_Get()->AddWindow( m_bottomBar );
		Assert( errcode == AUI_ERRCODE_OK );
		if(	errcode != AUI_ERRCODE_OK ) return errcode;
	}

	return AUI_ERRCODE_OK;
}


AUI_ERRCODE MessageModal::RemoveBordersFromUI()
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
