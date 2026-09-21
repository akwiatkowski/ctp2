//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ source
// Description  : The tutorial window
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

#include <memory>

#include "ui/aui_common/aui.h"
#include "ui/aui_common/aui_ldl.h"
#include "ui/aui_common/aui_uniqueid.h"
#include "ui/aui_common/aui_stringtable.h"

#include "ui/aui_ctp2/c3ui.h"
#include "ui/aui_ctp2/c3_popupwindow.h"
#include "ui/aui_ctp2/c3_button.h"
#include "ui/aui_ctp2/c3_switch.h"
#include "ui/aui_ctp2/c3_listbox.h"

#include "gs/slic/SlicEngine.h"
#include "gs/gameobj/player.h"
#include "ui/aui_ctp2/SelItem.h"

#include "ctp/ctp2_utils/pointerlist.h"
#include "gs/slic/SlicRecord.h"

#include "gs/database/profileDB.h"

#include "ui/aui_ctp2/c3_utilitydialogbox.h"

#include "ui/interface/UIUtils.h"
#include "ui/interface/screenutils.h"

#include "ui/interface/tutorialwin.h"





static std::unique_ptr<TutorialWin>	g_tutorialWin;

TutorialWin * tutorialwin_Get()           { return g_tutorialWin.get(); }
void          tutorialwin_Set(TutorialWin *p) { g_tutorialWin.reset(p); }



void tutorialwin_DialogCallback( sint32 val )
{
	if ( val ) {
		profiledb_Get()->SetTutorialAdvice( FALSE );

		close_TutorialWin();

	}
}

void tutorialwin_ButtonCallback( aui_Control *control, uint32 action, uint32 data, void *cookie )
{
	if ( action != (uint32)AUI_BUTTON_ACTION_EXECUTE ) return;

	g_tutorialWin->HandleButton( (c3_Button *)control );
}

void tutorialwin_SwitchCallback( aui_Control *control, uint32 action, uint32 data, void *cookie )
{
	switch ( action ) {
	case AUI_SWITCH_ACTION_ON:
	case AUI_SWITCH_ACTION_OFF:
		if ( g_tutorialWin ) {
			g_tutorialWin->HandleSwitch( (c3_Switch *)control );
		}
		break;
	}

}

void tutorialwin_ListCallback( aui_Control *control, uint32 action, uint32 data, void *cookie )
{

	if ( action != (uint32)AUI_LISTBOX_ACTION_SELECT ) return;

	SingleListItem *item;

	item = (SingleListItem *)(((aui_ListBox *)control)->GetSelectedItem());
	if ( !item ) return;

	sint32 i = 0;

	sint32 player = selitem_Get()->GetVisiblePlayer();
	PointerList<SlicRecord> *recordList = slicengine_Get()->GetRecords(player);
	if ( !recordList ) return;

	PointerList<SlicRecord>::Walker walk(recordList);

	while(walk.IsValid()) {
		if ( i++ == item->GetValue() ) {
			walk.GetObj()->Reconstitute();
			return;
		}
		walk.Next();
	}
}

sint32 tutorialwin_Initialize( )
{
	if ( g_tutorialWin ) {
		g_tutorialWin->UpdateData();
		return 0;
	}

	g_tutorialWin = std::make_unique<TutorialWin>();

	return 0;
}

sint32 tutorialwin_Cleanup( )
{
	g_tutorialWin.reset();

	return 0;
}

TutorialWin::TutorialWin( )
{
	AUI_ERRCODE errcode = AUI_ERRCODE_OK;
	MBCHAR		windowBlock[ k_AUI_LDL_MAXBLOCK + 1 ];

	strlcpy(windowBlock,"TutorialWin", sizeof(windowBlock));

	m_window = std::make_unique<c3_PopupWindow>( &errcode, aui_UniqueId(), windowBlock, 16, AUI_WINDOW_TYPE_FLOATING );
	Assert( AUI_NEWOK(m_window, errcode) );
	if ( !AUI_NEWOK(m_window, errcode) ) return;

	m_window->Resize( m_window->Width(), m_window->Height() );
	m_window->GrabRegion()->Resize( m_window->Width(), m_window->Height() );
	m_window->SetDraggable( TRUE );

	m_minimized = TRUE;

	Initialize( windowBlock );

	UpdateData();
}

sint32 TutorialWin::Initialize( MBCHAR *windowBlock )
{
	AUI_ERRCODE errcode = AUI_ERRCODE_OK;
	MBCHAR		controlBlock[ k_AUI_LDL_MAXBLOCK + 1 ];

	snprintf(controlBlock, sizeof(controlBlock), "%s.%s", windowBlock, "List" );
	m_list = std::make_unique<c3_ListBox>( &errcode, aui_UniqueId(), controlBlock, tutorialwin_ListCallback, nullptr );
	TestControl( m_list );
	m_list->Hide();

	snprintf(controlBlock, sizeof(controlBlock), "%s.%s", windowBlock, "TitleButton" );
	m_titleButton = std::make_unique<c3_Switch>( &errcode, aui_UniqueId(), controlBlock, tutorialwin_SwitchCallback );
	TestControl( m_titleButton );

	snprintf(controlBlock, sizeof(controlBlock), "%s.%s", windowBlock, "EndButton" );
	m_endButton = std::make_unique<c3_Button>( &errcode, aui_UniqueId(), controlBlock, tutorialwin_ButtonCallback );
	TestControl( m_endButton );
	m_endButton->Hide();

	snprintf(controlBlock, sizeof(controlBlock), "%s.%s", windowBlock, "ExitButton" );
	m_exitButton = std::make_unique<c3_Button>( &errcode, aui_UniqueId(), controlBlock, tutorialwin_ButtonCallback );
	TestControl( m_exitButton );

	snprintf(controlBlock, sizeof(controlBlock), "TutorialWinStrings" );
	m_string = std::make_unique<aui_StringTable>( &errcode, controlBlock );
	TestControl( m_string );

	errcode = aui_Ldl::SetupHeirarchyFromRoot( windowBlock );
	Assert( AUI_SUCCESS(errcode) );

	return 0;
}

TutorialWin::~TutorialWin( )
{
	Remove();

	// unique_ptr members; reset in the original explicit order.
	m_list.reset();
	m_titleButton.reset();
	m_endButton.reset();
	m_exitButton.reset();
	m_string.reset();

	m_window.reset();
}

void TutorialWin::Display( )
{
	AUI_ERRCODE errcode = c3ui_Get()->AddWindow( m_window.get() );
	Assert( errcode == AUI_ERRCODE_OK );


}

void TutorialWin::Remove( )
{
	AUI_ERRCODE errcode = c3ui_Get()->RemoveWindow( m_window->Id() );
	Assert( errcode == AUI_ERRCODE_OK );


}

sint32 TutorialWin::UpdateData( )
{
	AUI_ERRCODE errcode = AUI_ERRCODE_OK;
	MBCHAR ldlBlock[ k_AUI_LDL_MAXBLOCK + 1 ];
	MBCHAR title[_MAX_PATH];
	sint32 i = 0;

	sint32 player = selitem_Get()->GetVisiblePlayer();
	PointerList<SlicRecord> *recordList = slicengine_Get()->GetRecords(player);
	if ( !recordList ) return -1;

	m_list->Clear();

	snprintf(ldlBlock, sizeof(ldlBlock), "TutorialListItem" );
	std::unique_ptr<SingleListItem> item;

	PointerList<SlicRecord>::Walker walk(recordList);

	while(walk.IsValid()) {
		strlcpy( title, walk.GetObj()->GetTitle(), sizeof(title) );
		item = std::make_unique<SingleListItem>( &errcode, title, i++, ldlBlock );
		TestControl( item );

		m_list->AddItem( (c3_ListItem *)item.release() );

		walk.Next();
	}

	return 0;
}

sint32 TutorialWin::AddToList( MBCHAR *text, sint32 index )
{
	AUI_ERRCODE errcode = AUI_ERRCODE_OK;
	MBCHAR ldlBlock[ k_AUI_LDL_MAXBLOCK + 1 ];

	sint32 player = selitem_Get()->GetVisiblePlayer();
	PointerList<SlicRecord> *recordList = slicengine_Get()->GetRecords(player);
	if ( !recordList ) return -1;

	snprintf(ldlBlock, sizeof(ldlBlock), "TutorialListItem" );
	auto item = std::make_unique<SingleListItem>( &errcode, text, index, ldlBlock );
	TestControl( item );

	m_list->AddItem( (c3_ListItem *)item.release() );

	return 0;
}

sint32 TutorialWin::HandleSwitch( c3_Switch *button )
{
	if ( button == m_titleButton.get() ) {
		m_minimized = !m_minimized;
		if ( !button->IsOn() ) {
			m_window->Resize( m_window->Width(), k_MINIMIZED_HEIGHT );
			m_list->Hide();
			m_endButton->Hide();
		}
		else {
			m_window->Resize( m_window->Width(), k_MAXIMIZED_HEIGHT );
			m_list->Show();
			m_endButton->Show();
		}
	}

	return 0;
}

sint32 TutorialWin::HandleButton( c3_Button *button )
{
	if ( button == m_endButton.get() ) {
		c3_TextMessage( m_string->GetString(0), k_UTILITY_TEXTMESSAGE_YESNO, tutorialwin_DialogCallback );
	}
	else if ( button == m_exitButton.get() ) {
		if ( m_titleButton->IsOn() ) {
			m_titleButton->SetState( 0 );
		}
	}

	return 0;
}
