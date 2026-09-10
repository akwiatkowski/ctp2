#include "ctp/c3.h"

#include "ui/aui_common/aui.h"
#include "ui/aui_ctp2/c3ui.h"
#include "ui/aui_common/aui_control.h"
#include "ui/aui_common/aui_imagebase.h"
#include "ui/aui_common/aui_textbase.h"
#include "ui/aui_common/aui_button.h"
#include "ui/aui_common/aui_ldl.h"
#include "ui/aui_common/aui_uniqueid.h"
#include "ui/aui_common/aui_bitmapfont.h"

#include "ui/aui_ctp2/c3_dropdown.h"
#include "ui/aui_ctp2/c3_listbox.h"
#include "ui/aui_ctp2/c3_static.h"

#include "gs/gameobj/message.h"
#include "gs/gameobj/MessageData.h"
#include "ui/interface/messageactions.h"
#include "ui/interface/messagewindow.h"
#include "ui/interface/messagemodal.h"
#include "ui/interface/messageeyepoint.h"

extern uint8		g_messageEyeDropWidth;
extern uint8		g_messageEyeGreatPadding;


MessageEyePointListItem::MessageEyePointListItem
(
    AUI_ERRCODE *   retval,
    MBCHAR const *  name,
    sint32          index,
    MBCHAR const *  ldlBlock
)
:
	aui_ImageBase(ldlBlock),
	aui_TextBase(ldlBlock, (MBCHAR const *) nullptr),
	c3_ListItem( retval, ldlBlock)
{
	Assert( AUI_SUCCESS(*retval) );
	if ( !AUI_SUCCESS(*retval) ) return;

	*retval = InitCommonLdl(name, index, ldlBlock);
	Assert( AUI_SUCCESS(*retval) );
}

AUI_ERRCODE MessageEyePointListItem::InitCommonLdl(MBCHAR const *name, sint32 index, MBCHAR const *ldlBlock)
{
	MBCHAR			block[ k_AUI_LDL_MAXBLOCK + 1 ];
	AUI_ERRCODE		retval;

	strlcpy(m_name, name, sizeof(m_name));
	m_index = index;

	c3_Static		*subItem;

	snprintf(block, sizeof(block), "%s.%s", ldlBlock, "name");
	subItem = new c3_Static(&retval, aui_UniqueId(), block);
	subItem->TextFlags() = k_AUI_BITMAPFONT_DRAWFLAG_JUSTCENTER;
	AddChild(subItem);

	Update();

	return AUI_ERRCODE_OK;
}

void MessageEyePointListItem::Update()
{
	c3_Static *subItem;

	subItem = (c3_Static *)GetChildByIndex(0);
	subItem->SetText(m_name);
}

sint32 MessageEyePointListItem::Compare(c3_ListItem *item2, uint32 column)
{
	return 0;
}



















MessageEyePointStandard::MessageEyePointStandard(
	AUI_ERRCODE *retval,
	MBCHAR const *ldlBlock,
	MessageWindow *window )
{
	*retval = InitCommon( ldlBlock, window );
	Assert( AUI_SUCCESS(*retval) );
}


MessageEyePointStandard::MessageEyePointStandard(
	AUI_ERRCODE *retval,
	MBCHAR const *ldlBlock,
	MessageModal *window )
{
	*retval = InitCommon( ldlBlock, window );
	Assert( AUI_SUCCESS(*retval) );
}


AUI_ERRCODE MessageEyePointStandard::InitCommon( MBCHAR const *ldlBlock, MessageWindow *window )
{
	AUI_ERRCODE errcode = AUI_ERRCODE_OK;
	MBCHAR		buttonBlock[ k_AUI_LDL_MAXBLOCK + 1 ];

	snprintf(buttonBlock, sizeof(buttonBlock), "%s.%s", ldlBlock, "StandardEyeButton" );
	m_button.reset(new aui_Button( &errcode, aui_UniqueId(), buttonBlock ));
	Assert( AUI_NEWOK( m_button, errcode ));
	if ( !AUI_NEWOK( m_button, errcode )) return AUI_ERRCODE_MEMALLOCFAILED;

	m_action.reset(new MessageStandardEyePointAction( window ));

	m_button->SetAction( m_action.get() );

	if ( window->GetGreatLibraryButton() ) {
		m_button->Offset( window->GetGreatLibraryButton()->Width() + 3, 0 );
	}

	errcode = window->AddControl( m_button.get() );
	Assert( errcode == AUI_ERRCODE_OK );
	if ( errcode != AUI_ERRCODE_OK ) return errcode;

	return AUI_ERRCODE_OK;
}


AUI_ERRCODE MessageEyePointStandard::InitCommon( MBCHAR const *ldlBlock, MessageModal *window )
{
	AUI_ERRCODE errcode = AUI_ERRCODE_OK;
	MBCHAR		buttonBlock[ k_AUI_LDL_MAXBLOCK + 1 ];

	snprintf(buttonBlock, sizeof(buttonBlock), "%s.%s", ldlBlock, "StandardEyeButton" );
	m_button.reset(new aui_Button( &errcode, aui_UniqueId(), buttonBlock ));
	Assert( AUI_NEWOK( m_button, errcode ));
	if ( !AUI_NEWOK( m_button, errcode )) return AUI_ERRCODE_MEMALLOCFAILED;

	m_action.reset(new MessageStandardEyePointAction( nullptr, window ));

	m_button->SetAction( m_action.get() );

	errcode = window->AddControl( m_button.get() );
	Assert( errcode == AUI_ERRCODE_OK );
	if ( errcode != AUI_ERRCODE_OK ) return errcode;

	return AUI_ERRCODE_OK;
}


MessageEyePointStandard::~MessageEyePointStandard ()
{
	m_button.reset();
	m_action.reset();
}





















MessageEyePointDropdown::MessageEyePointDropdown(
	AUI_ERRCODE *retval,
	MBCHAR const *ldlBlock,
	MessageWindow *window )
{
	*retval = InitCommon( ldlBlock, window );
	Assert( AUI_SUCCESS(*retval) );
}


MessageEyePointDropdown::MessageEyePointDropdown(
	AUI_ERRCODE *retval,
	MBCHAR const *ldlBlock,
	MessageModal *window )
{
	*retval = InitCommon( ldlBlock, window );
	Assert( AUI_SUCCESS(*retval) );
}

AUI_ERRCODE MessageEyePointDropdown::InitCommonCommon
(
    MBCHAR const *      ldlBlock,
    MessageData *       a_Message
)
{
	AUI_ERRCODE errcode = AUI_ERRCODE_OK;
	MBCHAR		buttonBlock[k_AUI_LDL_MAXBLOCK + 1];

	snprintf(buttonBlock, sizeof(buttonBlock), "%s.%s", ldlBlock, "StandardEyeButton");
	m_button.reset(new aui_Button(&errcode, aui_UniqueId(), buttonBlock));
	Assert(AUI_NEWOK(m_button, errcode));
	if (!AUI_NEWOK(m_button, errcode)) return AUI_ERRCODE_MEMALLOCFAILED;
    m_button->TextReloadFont();

	snprintf(buttonBlock, sizeof(buttonBlock), "%s.%s", ldlBlock, "StandardEyePointDropdown");
	m_dropdown.reset(new c3_DropDown(&errcode, aui_UniqueId(), buttonBlock));
	Assert(AUI_NEWOK(m_dropdown, errcode));
	if (!AUI_NEWOK(m_dropdown, errcode)) return AUI_ERRCODE_MEMALLOCFAILED;

	if (m_dropdown->GetListBox())
    {
		m_dropdown->GetListBox()->SetForceSelect(false);
    }

	snprintf(buttonBlock, sizeof(buttonBlock), "%s.%s", ldlBlock, "StandardEyePointDropdownItem" );
	sint32 i = 0;
	while (MBCHAR const * text = a_Message->GetEyePointName(i++))
    {
		MessageEyePointListItem	* item =
            new MessageEyePointListItem(&errcode, text, i, buttonBlock);

		if (item)
			m_dropdown->AddItem((aui_Item *) item);
    }

	return errcode;
}

AUI_ERRCODE MessageEyePointDropdown::InitCommon(MBCHAR const * ldlBlock, MessageWindow *window )
{
	AUI_ERRCODE errcode = InitCommonCommon(ldlBlock, window->GetMessage()->AccessData());
    if (AUI_ERRCODE_OK != errcode) return errcode;

	m_action.reset(new MessageDropdownEyePointAction(window));

	m_action->SetDropdown(m_dropdown.get());
	m_button->SetAction(m_action.get());

	if (window->GetGreatLibraryButton())
    {
		m_button->Offset(window->GetGreatLibraryButton()->Width() + 3, 0);
	}

	errcode = window->AddControl( m_button.get() );
	Assert( errcode == AUI_ERRCODE_OK );
	if ( errcode != AUI_ERRCODE_OK ) return errcode;


	m_dropaction.reset(new MessageDropdownAction( window, m_dropdown.get() ));
	Assert( m_dropaction != nullptr );

	m_dropdown->SetAction( m_dropaction.get() );

	if ( window->GetGreatLibraryButton() ) {
		m_dropdown->Offset( window->GetGreatLibraryButton()->Width() +
							g_messageEyeGreatPadding, 0 );
	}

	window->AddControl( m_dropdown.get() );

	return AUI_ERRCODE_OK;
}


AUI_ERRCODE MessageEyePointDropdown::InitCommon(MBCHAR const *ldlBlock, MessageModal * window)
{
	AUI_ERRCODE     errcode = InitCommonCommon(ldlBlock, window->GetMessage()->AccessData());
    if (AUI_ERRCODE_OK != errcode) return errcode;

	m_action.reset(new MessageDropdownEyePointAction( nullptr, window ));

	m_action->SetDropdown(m_dropdown.get());
	m_button->SetAction(m_action.get());

	errcode = window->AddControl( m_button.get() );
	Assert( errcode == AUI_ERRCODE_OK );
	if ( errcode != AUI_ERRCODE_OK ) return errcode;


	m_dropaction.reset(new MessageDropdownAction( nullptr, m_dropdown.get(), window ));

	m_dropdown->SetAction( m_dropaction.get() );

	window->AddControl( m_dropdown.get() );

	return AUI_ERRCODE_OK;
}


MessageEyePointDropdown::~MessageEyePointDropdown()
{
	m_button.reset();
	m_action.reset();
	m_dropaction.reset();
	m_dropdown.reset();
}





















MessageEyePointListbox::MessageEyePointListbox(
	AUI_ERRCODE *retval,
	MBCHAR const *ldlBlock,
	MessageWindow *window )
{
	*retval = InitCommon( ldlBlock, window );
	Assert( AUI_SUCCESS(*retval) );
	if ( !AUI_SUCCESS(*retval) ) return;
}


MessageEyePointListbox::MessageEyePointListbox(
	AUI_ERRCODE *retval,
	MBCHAR const *ldlBlock,
	MessageModal *window )
{
	*retval = InitCommon( ldlBlock, window );
	Assert( AUI_SUCCESS(*retval) );
	if ( !AUI_SUCCESS(*retval) ) return;
}


AUI_ERRCODE MessageEyePointListbox::InitCommon(MBCHAR const * ldlBlock, MessageWindow *window )
{
	AUI_ERRCODE		errcode = AUI_ERRCODE_OK;
	MBCHAR			buttonBlock[ k_AUI_LDL_MAXBLOCK + 1 ];

	sint32 count = window->GetMessage()->AccessData()->GetNumEyePoints( );

	snprintf(buttonBlock, sizeof(buttonBlock), "%s.%s", ldlBlock, "EyeLeftButton" );
	m_buttonLeft.reset(new aui_Button( &errcode, aui_UniqueId(), buttonBlock ));
	Assert( AUI_NEWOK( m_buttonLeft, errcode ));
	if ( !AUI_NEWOK( m_buttonLeft, errcode )) return AUI_ERRCODE_MEMALLOCFAILED;

	m_actionLeft.reset(new MessageListboxEyePointAction( window, 0, -1, 0, count ));

	m_buttonLeft->SetAction( m_actionLeft.get() );

	if ( window->GetGreatLibraryButton() ) {
		m_buttonLeft->Offset( window->GetGreatLibraryButton()->Width() + 3, 0 );
	}

	errcode = window->AddControl( m_buttonLeft.get() );
	Assert( errcode == AUI_ERRCODE_OK );
	if ( errcode != AUI_ERRCODE_OK ) return errcode;

	snprintf(buttonBlock, sizeof(buttonBlock), "%s.%s", ldlBlock, "EyeRightButton" );
	m_buttonRight.reset(new aui_Button( &errcode, aui_UniqueId(), buttonBlock ));
	Assert( AUI_NEWOK( m_buttonRight, errcode ));
	if ( !AUI_NEWOK( m_buttonRight, errcode )) return AUI_ERRCODE_MEMALLOCFAILED;

	m_actionRight.reset(new MessageListboxEyePointAction( window, 0, 1, 0, count ));

	m_buttonRight->SetAction( m_actionRight.get() );

	if ( window->GetGreatLibraryButton() ) {
		m_buttonRight->Offset( window->GetGreatLibraryButton()->Width() + 3, 0 );
	}

	errcode = window->AddControl( m_buttonRight.get() );
	Assert( errcode == AUI_ERRCODE_OK );
	if ( errcode != AUI_ERRCODE_OK ) return errcode;


	// The two paging actions step the same eye-point list from either end,
	// so each needs to reach the other when it fires.
	m_actionLeft->SetAction( m_actionRight.get() );
	m_actionRight->SetAction( m_actionLeft.get() );

	return AUI_ERRCODE_OK;
}


AUI_ERRCODE MessageEyePointListbox::InitCommon(MBCHAR const * ldlBlock, MessageModal *window )
{
	AUI_ERRCODE		errcode = AUI_ERRCODE_OK;
	MBCHAR			buttonBlock[ k_AUI_LDL_MAXBLOCK + 1 ];

	sint32 count = window->GetMessage()->AccessData()->GetNumEyePoints( );

	snprintf(buttonBlock, sizeof(buttonBlock), "%s.%s", ldlBlock, "EyeLeftButton" );
	m_buttonLeft.reset(new aui_Button( &errcode, aui_UniqueId(), buttonBlock ));
	Assert( AUI_NEWOK( m_buttonLeft, errcode ));
	if ( !AUI_NEWOK( m_buttonLeft, errcode )) return AUI_ERRCODE_MEMALLOCFAILED;

	m_actionLeft.reset(new MessageListboxEyePointAction( nullptr, 0, -1, 0, count, window ));

	m_buttonLeft->SetAction( m_actionLeft.get() );

	errcode = window->AddControl( m_buttonLeft.get() );
	Assert( errcode == AUI_ERRCODE_OK );
	if ( errcode != AUI_ERRCODE_OK ) return errcode;

	snprintf(buttonBlock, sizeof(buttonBlock), "%s.%s", ldlBlock, "EyeRightButton" );
	m_buttonRight.reset(new aui_Button( &errcode, aui_UniqueId(), buttonBlock ));
	Assert( AUI_NEWOK( m_buttonRight, errcode ));
	if ( !AUI_NEWOK( m_buttonRight, errcode )) return AUI_ERRCODE_MEMALLOCFAILED;

	m_actionRight.reset(new MessageListboxEyePointAction( nullptr, 0, 1, 0, count, window ));

	m_buttonRight->SetAction( m_actionRight.get() );

	errcode = window->AddControl( m_buttonRight.get() );
	Assert( errcode == AUI_ERRCODE_OK );
	if ( errcode != AUI_ERRCODE_OK ) return errcode;


	m_actionLeft->SetAction( m_actionRight.get() );
	m_actionRight->SetAction( m_actionLeft.get() );

	return AUI_ERRCODE_OK;
}


MessageEyePointListbox::~MessageEyePointListbox()
{
	m_buttonLeft.reset();
	m_buttonRight.reset();
	m_actionLeft.reset();
	m_actionRight.reset();
}
