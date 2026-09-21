//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ source
// Description  : The civilization 3 utility dialog box
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
// - Fixed memory leaks.
//
//----------------------------------------------------------------------------

#include "ctp/c3.h"
#include "ui/aui_ctp2/c3_utilitydialogbox.h"

#include <string>
#include <memory>

#include "ui/aui_common/aui.h"
#include "ui/aui_common/aui_ldl.h"
#include "ui/aui_common/aui_listbox.h"
#include "ui/aui_common/aui_static.h"
#include "ui/aui_common/aui_uniqueid.h"
#include "BuildingRecord.h"
#include "ui/aui_ctp2/c3_button.h"
#include "ui/aui_ctp2/c3_listbox.h"
#include "ui/aui_ctp2/c3_popupwindow.h"
#include "ui/aui_ctp2/c3_static.h"
#include "ui/aui_ctp2/c3textfield.h"
#include "ui/aui_ctp2/c3ui.h"
#include "ui/aui_ctp2/c3window.h"
#include "ui/aui_ctp2/c3windows.h"
#include "gs/gameobj/citydata.h"
#include "ui/aui_ctp2/ctp2_button.h"
#include "ui/aui_ctp2/ctp2_listitem.h"
#include "ui/aui_ctp2/ctp2_listbox.h"
#include "gs/utility/Globals.h"                // allocated::...
#include "ui/aui_ctp2/keypress.h"
#include "ui/interface/MainControlPanel.h"
#include "net/general/network.h"
#include "gs/gameobj/player.h"
#include "ui/aui_ctp2/SelItem.h"                // selitem_Get()
#include "TerrainRecord.h"
#include "ui/aui_ctp2/textbutton.h"
#include "ui/aui_ctp2/thermometer.h"
#include "ui/interface/UIUtils.h"
#include "gs/gameobj/Unit.h"
#include "gs/gameobj/UnitData.h"
#include "gs/utility/UnitDynArr.h"
#include "gs/newdb/UnitRec.h"
#include "WonderRecord.h"
#include "gs/world/World.h"

extern sint32   	g_modalWindow;
extern sint32		g_ScreenWidth;
extern sint32		g_ScreenHeight;

std::unique_ptr<c3_ExpelPopup>               g_expelPopup;
std::unique_ptr<c3_UtilityAbortPopup>        g_utilityAbort;
std::unique_ptr<c3_UtilityTextMessagePopup>  g_utilityTextMessage;


void C3UtilityCityListButtonActionCallback( aui_Control *control, uint32 action, uint32 data, void *cookie )
{

	if ( action != (uint32)AUI_BUTTON_ACTION_EXECUTE ) return;

	c3_UtilityCityListPopup *popup = (c3_UtilityCityListPopup *)cookie;
	if (!popup) return;

	if ((ctp2_Button*)control == popup->m_window->Ok())
	{

		SingleListItem *item = (SingleListItem *) popup->m_list->GetSelectedItem();
		if (!item) return;

//      sint32 cityIndex = item->GetValue();





		Assert(FALSE);
		if (popup->m_callback)
		{

			popup->RemoveWindow();
		}
	}
	if ((c3_Button*)control == popup->m_window->Cancel())
	{
		if (popup->m_callback)
			popup->m_callback(Unit(), FALSE);

		popup->RemoveWindow();
	}
}

void C3PiracyButtonCallback( aui_Control *control, uint32 action, uint32 data, void *cookie )
{

	if ( action != (uint32)AUI_BUTTON_ACTION_EXECUTE ) return;

	c3_PiracyPopup *popup = (c3_PiracyPopup *)cookie;
	if (!popup) return;

	if ((c3_Button*)control == popup->m_pirate.get())
	{







		if (popup->m_callback)
		{

			popup->RemoveWindow();
		}
	}
	if ((c3_Button*)control == popup->m_cancel.get())
	{




		popup->RemoveWindow();
	}
}

void C3ExpelButtonCallback( aui_Control *control, uint32 action, uint32 data, void *cookie )
{

	if ( action != (uint32)AUI_BUTTON_ACTION_EXECUTE ) return;

	c3_ExpelPopup *popup = (c3_ExpelPopup *)cookie;
	if (!popup) return;

	if ((c3_Button*)control == popup->m_attack.get())
	{
		if (popup->m_callback)
			popup->m_callback( EXPEL_ACTION_ATTACK );

		popup->RemoveWindow();

	}

	if ((c3_Button*)control == popup->m_expel.get())
	{
		if (popup->m_callback)
			popup->m_callback( EXPEL_ACTION_EXPEL );

		popup->RemoveWindow();
	}

	if ((c3_Button*)control == popup->m_cancel.get())
	{
		if (popup->m_callback)
			popup->m_callback( EXPEL_ACTION_CANCEL );

		popup->RemoveWindow();
	}
}


void C3UtilityTextFieldButtonActionCallback( aui_Control *control, uint32 action, uint32 data, void *cookie )
{

	if ( action != (uint32)AUI_BUTTON_ACTION_EXECUTE &&
		action != (uint32)AUI_TEXTFIELD_ACTION_EXECUTE) return;

	c3_UtilityTextFieldPopup *popup = (c3_UtilityTextFieldPopup *)cookie;
	if (!popup) return;

	if ((ctp2_Button*)control == popup->m_window->Ok() ||
			(((aui_TextField *)control == popup->m_text.get()) &&
				(action == AUI_TEXTFIELD_ACTION_EXECUTE)))
	{

		std::string resultText;
		resultText.resize(256);
		popup->m_text->GetFieldText(&resultText[0], 256);
		resultText.resize(strlen(resultText.c_str()));

		if (resultText.empty())
		{
			if(popup->m_wantEmpties) {
				popup->m_callback(nullptr, TRUE, popup->GetData());
				popup->RemoveWindow();
			}
			return;
		}
		else if (popup->m_callback)
		{
			popup->m_callback(resultText.c_str(), TRUE, popup->GetData());

			popup->RemoveWindow();
		}
	}
	if ((c3_Button*)control == popup->m_window->Cancel())
	{
		if (popup->m_callback)
			popup->m_callback(nullptr, FALSE, popup->GetData());

		popup->RemoveWindow();
	}
}

void C3UtilityTextMessageButtonActionCallback( aui_Control *control, uint32 action, uint32 data, void *cookie )
{

	if ( action != (uint32)AUI_BUTTON_ACTION_EXECUTE ) return;

	c3_UtilityTextMessagePopup *popup = (c3_UtilityTextMessagePopup *)cookie;
	if (!popup) return;

	if ((ctp2_Button*)control == popup->m_window->Ok())
	{
		if (popup->m_callback)
			popup->m_callback( TRUE );

		popup->RemoveWindow();

	}

	if ( popup->m_type )
	{
		if ((c3_Button*)control == popup->m_window->Cancel())
		{
			if (popup->m_callback)
				popup->m_callback( FALSE );

			popup->RemoveWindow();
		}
	}
}

void C3AbortButtonActionCallback( aui_Control *control, uint32 action, uint32 data, void *cookie )
{

	if ( action != (uint32)AUI_BUTTON_ACTION_EXECUTE ) return;

	c3_UtilityAbortPopup *popup = (c3_UtilityAbortPopup *)cookie;
	if (!popup) return;

	if ((ctp2_Button*)control == popup->m_abort.get())
	{
		if (popup->m_callback)
			popup->m_callback( FALSE );

		popup->RemoveWindow();

	}
}

void C3UtilityPlayerListButtonActionCallback( aui_Control *control, uint32 action, uint32 data, void *cookie )
{

	if ( action != (uint32)AUI_BUTTON_ACTION_EXECUTE ) return;

	c3_UtilityPlayerListPopup *popup = (c3_UtilityPlayerListPopup *)cookie;
	if (!popup) return;

	if ((ctp2_Button*)control == popup->m_kick.get())
	{

		DoubleListItem *item = (DoubleListItem *) popup->m_list->GetSelectedItem();
		if (!item) return;

		sint32 playerIndex = item->GetValue();

		if (popup->m_callback)
		{
			popup->m_callback( playerIndex, TRUE, PLAYER_ACTION_KICK );
		}
	}
	if ((ctp2_Button*)control == popup->m_open.get())
	{

		DoubleListItem *item = (DoubleListItem *) popup->m_list->GetSelectedItem();
		if (!item) return;

		sint32 playerIndex = item->GetValue();

		if (popup->m_callback)
		{
			popup->m_callback( playerIndex, TRUE, PLAYER_ACTION_OPEN );
		}
	}
	if ((ctp2_Button*)control == popup->m_close.get())
	{

		DoubleListItem *item = (DoubleListItem *) popup->m_list->GetSelectedItem();
		if (!item) return;

		sint32 playerIndex = item->GetValue();

		if (popup->m_callback)
		{
			popup->m_callback( playerIndex, TRUE, PLAYER_ACTION_CLOSE );
		}
	}
	if ((ctp2_Button*)control == popup->m_abort.get())
	{
		if (popup->m_callback)
			popup->m_callback( -1, FALSE, PLAYER_ACTION_MAX );

		popup->RemoveWindow();
	}
}












c3_UtilityCityListPopup::c3_UtilityCityListPopup( c3_UtilityCityListCallback *callback, MBCHAR const *ldlBlock )
:
	m_window        (nullptr),
	m_title_label   (nullptr),
	m_list          (nullptr),
	m_ok            (nullptr),
	m_cancel        (nullptr),
	m_callback      (callback)
{
	MBCHAR		windowBlock[ k_AUI_LDL_MAXBLOCK + 1 ];

	if (ldlBlock) strlcpy(windowBlock, ldlBlock, sizeof(windowBlock));
	else strlcpy(windowBlock, "DefaultUtilityCityListPopup", sizeof(windowBlock));

	{
	    AUI_ERRCODE errcode = AUI_ERRCODE_OK;
		m_window = std::make_unique<c3_PopupWindow>( &errcode, aui_UniqueId(), windowBlock, 16, AUI_WINDOW_TYPE_FLOATING, false);
		Assert( AUI_NEWOK(m_window, errcode) );
		if ( !AUI_NEWOK(m_window, errcode) ) return;

		m_window->Resize(m_window->Width(),m_window->Height());
		m_window->GrabRegion()->Resize(m_window->Width(),m_window->Height());
		m_window->SetStronglyModal(TRUE);
	}

	Initialize(windowBlock);
}

sint32 c3_UtilityCityListPopup::Initialize( MBCHAR const *windowBlock )
{
	AUI_ERRCODE errcode = AUI_ERRCODE_OK;
	MBCHAR		controlBlock[ k_AUI_LDL_MAXBLOCK + 1 ];

	snprintf(controlBlock, sizeof(controlBlock), "%s.%s", windowBlock, "TitleLabel");
	m_window->AddTitle( controlBlock );
	m_window->AddCancel(C3UtilityCityListButtonActionCallback, this);
	m_window->AddOk(C3UtilityCityListButtonActionCallback, this);


















	snprintf(controlBlock, sizeof(controlBlock), "%s.%s", windowBlock, "CityList" );
	m_list = std::make_unique<ctp2_ListBox>(&errcode, aui_UniqueId(), controlBlock, nullptr, nullptr);
	Assert( AUI_NEWOK(m_list, errcode) );
	if ( !AUI_NEWOK(m_list, errcode) ) return -1;

	errcode = aui_Ldl::SetupHeirarchyFromRoot( windowBlock );
	Assert( AUI_SUCCESS(errcode) );

	return 0;
}

c3_UtilityCityListPopup::~c3_UtilityCityListPopup( )
{
    if (m_list)
    {
        m_list->Clear();
    }
    m_list.reset();
    m_title_label.reset();
    m_ok.reset();
    m_cancel.reset();
    m_window.reset();
}

void c3_UtilityCityListPopup::Cleanup()
{
    if (m_list)
    {
        m_list->Clear();
    }
    m_list.reset();
    m_title_label.reset();
    m_ok.reset();
    m_cancel.reset();
    m_window.reset();
	m_callback = nullptr;
}

void c3_UtilityCityListPopup::DisplayWindow( )
{
	UpdateData();

	AUI_ERRCODE auiErr = c3ui_Get()->AddWindow(m_window.get());
	Assert( auiErr == AUI_ERRCODE_OK );
}

void c3_UtilityCityListPopup::RemoveWindow( )
{
	AUI_ERRCODE auiErr = c3ui_Get()->RemoveWindow( m_window->Id() );
	Assert( auiErr == AUI_ERRCODE_OK );
}

sint32 c3_UtilityCityListPopup::UpdateData( )
{
	MBCHAR strbuf[256];
	MBCHAR ldlBlock[ k_AUI_LDL_MAXBLOCK + 1 ];
	sint32 curPlayer = selitem_Get()->GetVisiblePlayer();

	AUI_ERRCODE		retval;

	UnitDynamicArray *cityList = player_Get(curPlayer)->GetAllCitiesList();

	strlcpy(ldlBlock, "SingleListItem", sizeof(ldlBlock));
	m_list->Clear();

	for ( sint32 i = 0 ; i < cityList->Num() ; i++ )
	{
		strlcpy(strbuf, (*cityList)[i].GetData()->GetCityData()->GetName(), sizeof(strbuf));
		m_list->AddItem(std::make_unique<SingleListItem>(&retval, strbuf, i, ldlBlock).release());
	}

	return 0;
}




c3_PiracyPopup::c3_PiracyPopup( c3_PiracyCallback *callback, MBCHAR const *ldlBlock )
:
    m_window            (nullptr),
    m_title_label       (nullptr),
    m_list              (nullptr),
    m_pirate            (nullptr),
    m_cancel            (nullptr),
	m_callback          (callback)
{
	MBCHAR		windowBlock[ k_AUI_LDL_MAXBLOCK + 1 ];

	if (ldlBlock) strlcpy(windowBlock, ldlBlock, sizeof(windowBlock));
	else strlcpy(windowBlock, "DefaultPiracyPopup", sizeof(windowBlock));

	{
	    AUI_ERRCODE errcode = AUI_ERRCODE_OK;
		m_window = std::make_unique<c3_PopupWindow>( &errcode, aui_UniqueId(), windowBlock, 16, AUI_WINDOW_TYPE_FLOATING, false);
		Assert( AUI_NEWOK(m_window, errcode) );
		if ( !AUI_NEWOK(m_window, errcode) ) return;

		m_window->Resize(m_window->Width(),m_window->Height());
		m_window->GrabRegion()->Resize(m_window->Width(),m_window->Height());
		m_window->SetStronglyModal(TRUE);
	}

    Initialize( windowBlock );
}

sint32 c3_PiracyPopup::Initialize( MBCHAR const *windowBlock )
{
	AUI_ERRCODE errcode = AUI_ERRCODE_OK;
	MBCHAR		controlBlock[ k_AUI_LDL_MAXBLOCK + 1 ];

	snprintf(controlBlock, sizeof(controlBlock), "%s.%s", windowBlock, "PirateButton" );
	m_pirate = std::make_unique<c3_Button>(&errcode, aui_UniqueId(), controlBlock, C3PiracyButtonCallback, this);
	Assert( AUI_NEWOK(m_pirate, errcode) );
	if ( !AUI_NEWOK(m_pirate, errcode) ) return -1;

	snprintf(controlBlock, sizeof(controlBlock), "%s.%s", windowBlock, "CancelButton" );
	m_cancel = std::make_unique<c3_Button>(&errcode, aui_UniqueId(), controlBlock, C3PiracyButtonCallback, this);
	Assert( AUI_NEWOK(m_cancel, errcode) );
	if ( !AUI_NEWOK(m_cancel, errcode) ) return -1;

	snprintf(controlBlock, sizeof(controlBlock), "%s.%s", windowBlock, "TitleLabel");
	m_title_label = std::make_unique<c3_Static>( &errcode, aui_UniqueId(), controlBlock);
	Assert( AUI_NEWOK(m_title_label, errcode) );
	if ( !AUI_NEWOK(m_title_label, errcode) ) return -1;

	snprintf(controlBlock, sizeof(controlBlock), "%s.%s", windowBlock, "PlayerList" );
	m_list = std::make_unique<ctp2_ListBox>(&errcode, aui_UniqueId(), controlBlock, nullptr, nullptr);
	Assert( AUI_NEWOK(m_list, errcode) );
	if ( !AUI_NEWOK(m_list, errcode) ) return -1;

	errcode = aui_Ldl::SetupHeirarchyFromRoot( windowBlock );
	Assert( AUI_SUCCESS(errcode) );

	return 0;
}

c3_PiracyPopup::~c3_PiracyPopup( )
{
    if (m_list)
    {
        m_list->Clear();
    }
    m_list.reset();
    m_title_label.reset();
    m_pirate.reset();
    m_cancel.reset();
    m_window.reset();
}

void c3_PiracyPopup::Cleanup( )
{
    if (m_list)
    {
        m_list->Clear();
    }
    m_list.reset();
    m_title_label.reset();
    m_pirate.reset();
    m_cancel.reset();
    m_window.reset();
	m_callback = nullptr;
}

void c3_PiracyPopup::DisplayWindow( )
{
	UpdateData();

	AUI_ERRCODE auiErr = c3ui_Get()->AddWindow(m_window.get());
	Assert( auiErr == AUI_ERRCODE_OK );
}

void c3_PiracyPopup::RemoveWindow( )
{
	AUI_ERRCODE auiErr = c3ui_Get()->RemoveWindow( m_window->Id() );
	Assert( auiErr == AUI_ERRCODE_OK );
}

sint32 c3_PiracyPopup::UpdateData( )
{
	MBCHAR strbuf[256];
	MBCHAR ldlBlock[ k_AUI_LDL_MAXBLOCK + 1 ];
	sint32 curPlayer = selitem_Get()->GetVisiblePlayer();

	AUI_ERRCODE		retval;

	UnitDynamicArray *cityList = player_Get(curPlayer)->GetAllCitiesList();

	strlcpy(ldlBlock, "PiracyListItem", sizeof(ldlBlock));

    m_list->Clear();

	for ( sint32 i = 0 ; i < cityList->Num() ; i++ )
	{
		strlcpy(strbuf, (*cityList)[i].GetData()->GetCityData()->GetName(), sizeof(strbuf));
		m_list->AddItem(std::make_unique<SingleListItem>(&retval, strbuf, i, ldlBlock).release());
	}

	return 0;
}




c3_ExpelPopup::c3_ExpelPopup( c3_ExpelCallback *callback, MBCHAR const *ldlBlock )
:
	m_window            (nullptr),
	m_title_label       (nullptr),
	m_attack            (nullptr),
	m_expel             (nullptr),
	m_cancel            (nullptr),
	m_callback          (callback)
{
	MBCHAR		windowBlock[ k_AUI_LDL_MAXBLOCK + 1 ];

    if (ldlBlock) strlcpy(windowBlock, ldlBlock, sizeof(windowBlock));
	else strlcpy(windowBlock, "DefaultExpelPopup", sizeof(windowBlock));

	{
	    AUI_ERRCODE errcode = AUI_ERRCODE_OK;
		m_window = std::make_unique<c3_PopupWindow>( &errcode, aui_UniqueId(), windowBlock, 16, AUI_WINDOW_TYPE_FLOATING, false);
		Assert( AUI_NEWOK(m_window, errcode) );
		if ( !AUI_NEWOK(m_window, errcode) ) return;

		m_window->Resize(m_window->Width(),m_window->Height());
		m_window->GrabRegion()->Resize(m_window->Width(),m_window->Height());
		m_window->SetStronglyModal(TRUE);
	}

	Initialize( windowBlock );
}

sint32 c3_ExpelPopup::Initialize( MBCHAR const *windowBlock )
{
	AUI_ERRCODE errcode = AUI_ERRCODE_OK;
	MBCHAR		controlBlock[ k_AUI_LDL_MAXBLOCK + 1 ];

	snprintf(controlBlock, sizeof(controlBlock), "%s.%s", windowBlock, "AttackButton" );
	m_attack = std::make_unique<c3_Button>(&errcode, aui_UniqueId(), controlBlock, C3ExpelButtonCallback, this);
	Assert( AUI_NEWOK(m_attack, errcode) );
	if ( !AUI_NEWOK(m_attack, errcode) ) return -1;

	snprintf(controlBlock, sizeof(controlBlock), "%s.%s", windowBlock, "ExpelButton" );
	m_expel = std::make_unique<c3_Button>(&errcode, aui_UniqueId(), controlBlock, C3ExpelButtonCallback, this);
	Assert( AUI_NEWOK(m_expel, errcode) );
	if ( !AUI_NEWOK(m_expel, errcode) ) return -1;

	snprintf(controlBlock, sizeof(controlBlock), "%s.%s", windowBlock, "CancelButton" );
	m_cancel = std::make_unique<c3_Button>(&errcode, aui_UniqueId(), controlBlock, C3ExpelButtonCallback, this);
	Assert( AUI_NEWOK(m_cancel, errcode) );
	if ( !AUI_NEWOK(m_cancel, errcode) ) return -1;

	snprintf(controlBlock, sizeof(controlBlock), "%s.%s", windowBlock, "TitleLabel");
	m_title_label = std::make_unique<c3_Static>( &errcode, aui_UniqueId(), controlBlock);
	Assert( AUI_NEWOK(m_title_label, errcode) );
	if ( !AUI_NEWOK(m_title_label, errcode) ) return -1;

	errcode = aui_Ldl::SetupHeirarchyFromRoot( windowBlock );
	Assert( AUI_SUCCESS(errcode) );

	return 0;
}

c3_ExpelPopup::~c3_ExpelPopup( )
{
    m_title_label.reset();
    m_attack.reset();
    m_expel.reset();
    m_cancel.reset();
    m_window.reset();
}

void c3_ExpelPopup::Cleanup()
{
    m_title_label.reset();
    m_attack.reset();
    m_expel.reset();
    m_cancel.reset();
    m_window.reset();
	m_callback = nullptr;
}

void c3_ExpelPopup::DisplayWindow( )
{
	AUI_ERRCODE auiErr = c3ui_Get()->AddWindow(m_window.get());
	Assert( auiErr == AUI_ERRCODE_OK );
}

void c3_ExpelPopup::RemoveWindow( )
{
	AUI_ERRCODE auiErr = c3ui_Get()->RemoveWindow( m_window->Id() );
	Assert( auiErr == AUI_ERRCODE_OK );
}






c3_UtilityTextFieldPopup::c3_UtilityTextFieldPopup
(
    c3_UtilityTextFieldCallback *   callback,
    MBCHAR const *                  titleText,
    MBCHAR const *                  defaultText,
    MBCHAR const *                  messageText,
    MBCHAR const *                        ldlBlock,
    void *                          data,
	bool                            wantEmpties
)
:
	m_window                (nullptr),
	m_title_label           (nullptr),
	m_message_label         (nullptr),
    m_text                  (nullptr),
	m_ok                    (nullptr),
    m_cancel                (nullptr),
    m_callback              (callback),
	m_data                  (data),
	m_wantEmpties           (wantEmpties)
{
	MBCHAR		windowBlock[ k_AUI_LDL_MAXBLOCK + 1 ];

    if (ldlBlock) strlcpy(windowBlock, ldlBlock, sizeof(windowBlock));
	else strlcpy(windowBlock, "DefaultUtilityTextFieldPopup", sizeof(windowBlock));

	{
	    AUI_ERRCODE errcode = AUI_ERRCODE_OK;
		m_window = std::make_unique<c3_PopupWindow>( &errcode, aui_UniqueId(), windowBlock, 16, AUI_WINDOW_TYPE_FLOATING, false);
		Assert( AUI_NEWOK(m_window, errcode) );
		if ( !AUI_NEWOK(m_window, errcode) ) return;

		m_window->Resize(m_window->Width(),m_window->Height());
		m_window->GrabRegion()->Resize(m_window->Width(),m_window->Height());
		m_window->SetStronglyModal(TRUE);
	}

	if (defaultText)
	{
		m_default_text = defaultText;
	}

	if (titleText) {
		m_title_text = titleText;
	}

	if (messageText) {
		m_message_text = messageText;
	}

	Initialize( windowBlock );

	if (m_text)
    {
        m_text->SetKeyboardFocus();
    }
}

sint32 c3_UtilityTextFieldPopup::Initialize( MBCHAR const *windowBlock )
{
	AUI_ERRCODE errcode = AUI_ERRCODE_OK;
	MBCHAR		controlBlock[ k_AUI_LDL_MAXBLOCK + 1 ];

	snprintf(controlBlock, sizeof(controlBlock), "%s.%s", windowBlock, "TitleLabel");
	m_window->AddTitle( controlBlock );
	m_window->AddCancel(C3UtilityTextFieldButtonActionCallback, this);
	m_window->AddOk(C3UtilityTextFieldButtonActionCallback, this);












	snprintf(controlBlock, sizeof(controlBlock), "%s.%s", windowBlock, "TextMessage");
	m_title_label = std::make_unique<c3_Static>( &errcode, aui_UniqueId(), controlBlock);
	Assert( AUI_NEWOK(m_title_label, errcode) );
	if ( !AUI_NEWOK(m_title_label, errcode) ) return -1;
	if (!m_title_text.empty())
		m_title_label->SetText(m_title_text.c_str());

	snprintf(controlBlock, sizeof(controlBlock), "%s.%s", windowBlock, "TextField");
	m_text = std::make_unique<C3TextField>( &errcode, aui_UniqueId(), controlBlock,
		C3UtilityTextFieldButtonActionCallback, this);
	Assert( AUI_NEWOK(m_text, errcode) );
	if ( !AUI_NEWOK(m_text, errcode) ) return -1;

	errcode = aui_Ldl::SetupHeirarchyFromRoot( windowBlock );
	Assert( AUI_SUCCESS(errcode) );

	return 0;
}

c3_UtilityTextFieldPopup::~c3_UtilityTextFieldPopup( )
{
    m_ok.reset();
    m_cancel.reset();
    m_text.reset();
    m_title_label.reset();
    m_window.reset();
}

void c3_UtilityTextFieldPopup::Cleanup()
{
    m_ok.reset();
    m_cancel.reset();
    m_text.reset();
    m_title_label.reset();
    m_window.reset();
}

void c3_UtilityTextFieldPopup::DisplayWindow( )
{
	UpdateData();

	AUI_ERRCODE auiErr = c3ui_Get()->AddWindow(m_window.get());
	Assert( auiErr == AUI_ERRCODE_OK );
	keypress_RegisterHandler(m_window.get());
}

void c3_UtilityTextFieldPopup::RemoveWindow( )
{
	AUI_ERRCODE auiErr = c3ui_Get()->RemoveWindow( m_window->Id() );
	Assert( auiErr == AUI_ERRCODE_OK );
	keypress_RemoveHandler(m_window.get());
}

sint32 c3_UtilityTextFieldPopup::UpdateData( )
{

	if (!m_default_text.empty())
		m_text->SetFieldText(m_default_text.c_str());

	return 0;
}






c3_UtilityTextMessagePopup::c3_UtilityTextMessagePopup
(
    MBCHAR const *                  text,
    sint32                          type,
    c3_UtilityTextMessageCallback * callback,
    MBCHAR const *                  ldlBlock
)
:
    m_window        (nullptr),
    m_callback      (callback),
    m_type          (type),
    m_title_label   (nullptr),
    m_text          (nullptr),
	m_ok            (nullptr),
	m_cancel        (nullptr)
{
	MBCHAR		windowBlock[ k_AUI_LDL_MAXBLOCK + 1 ];

	if (ldlBlock)
    {
        strlcpy(windowBlock, ldlBlock, sizeof(windowBlock));
    }
	else if (type)
    {
        strlcpy(windowBlock, "DefaultUtilityTextMessagePopup", sizeof(windowBlock));
    }
    else
    {
        strlcpy(windowBlock, "DefaultUtilityTextMessageOkPopup", sizeof(windowBlock));
	}

	{
	    AUI_ERRCODE errcode = AUI_ERRCODE_OK;
		m_window = std::make_unique<c3_PopupWindow>( &errcode, aui_UniqueId(), windowBlock, 16, AUI_WINDOW_TYPE_FLOATING, false);
		Assert( AUI_NEWOK(m_window, errcode) );
		if ( !AUI_NEWOK(m_window, errcode) ) return;

		m_window->Resize(m_window->Width(),m_window->Height());
		m_window->GrabRegion()->Resize(m_window->Width(),m_window->Height());
		m_window->SetStronglyModal(TRUE);
	}

    Initialize( windowBlock );
}

sint32 c3_UtilityTextMessagePopup::Initialize( MBCHAR const *windowBlock )
{
	AUI_ERRCODE errcode = AUI_ERRCODE_OK;
	MBCHAR		controlBlock[ k_AUI_LDL_MAXBLOCK + 1 ];

	snprintf(controlBlock, sizeof(controlBlock), "%s.%s", windowBlock, "TitleLabel");
	m_window->AddTitle( controlBlock );

	switch ( m_type ) {
	case k_UTILITY_TEXTMESSAGE_OK:
		m_window->AddClose(C3UtilityTextMessageButtonActionCallback, this);
		break;

	case k_UTILITY_TEXTMESSAGE_OKCANCEL:
		m_window->AddCancel(C3UtilityTextMessageButtonActionCallback, this);
		m_window->AddOk(C3UtilityTextMessageButtonActionCallback, this);
		break;

	case k_UTILITY_TEXTMESSAGE_YESNO:
		m_window->AddYes(C3UtilityTextMessageButtonActionCallback, this);
		m_window->AddNo(C3UtilityTextMessageButtonActionCallback, this);
		break;
	}





















	snprintf(controlBlock, sizeof(controlBlock), "%s.%s", windowBlock, "TextMessage");
	m_text = std::make_unique<c3_Static>( &errcode, aui_UniqueId(), controlBlock);
	Assert( AUI_NEWOK(m_text, errcode) );
	if ( !AUI_NEWOK(m_text, errcode) ) return -1;

	errcode = aui_Ldl::SetupHeirarchyFromRoot( windowBlock );
	Assert( AUI_SUCCESS(errcode) );

	return 0;
}

c3_UtilityTextMessagePopup::~c3_UtilityTextMessagePopup( )
{
    m_ok.reset();
    m_cancel.reset();
    m_text.reset();
    m_title_label.reset();
    m_window.reset();
}

void c3_UtilityTextMessagePopup::Cleanup( )
{
    m_ok.reset();
    m_cancel.reset();
    m_text.reset();
    m_title_label.reset();
    m_window.reset();
	m_type = 0;
}

void c3_UtilityTextMessagePopup::DisplayWindow( MBCHAR const *text )
{
	UpdateData(text);

	AUI_ERRCODE auiErr = c3ui_Get()->AddWindow(m_window.get());
	Assert( auiErr == AUI_ERRCODE_OK );

	keypress_RegisterHandler(m_window.get());
}

void c3_UtilityTextMessagePopup::RemoveWindow( )
{
	AUI_ERRCODE auiErr = c3ui_Get()->RemoveWindow( m_window->Id() );
	Assert( auiErr == AUI_ERRCODE_OK );

	keypress_RemoveHandler(m_window.get());

    g_utilityTextMessage.reset();
}

sint32 c3_UtilityTextMessagePopup::UpdateData( MBCHAR const *text )
{

	if (text)
		m_text->SetText(text);

	return 0;
}

void c3_UtilityTextMessageCleanupAction::Execute(aui_Control *control,
									uint32 action,
									uint32 data )
{
	if (g_utilityTextMessage)
    {
		g_utilityTextMessage->Cleanup();
        g_utilityTextMessage.reset();
    }
}

c3_UtilityTextMessageCreateAction::c3_UtilityTextMessageCreateAction
(
    MBCHAR const *                  text,
    sint32                          type,
    c3_UtilityTextMessageCallback * callback,
    MBCHAR const *                  ldlBlock
)
:
    m_text      (text),
    m_type      (type),
    m_callback  (callback),
    m_ldlBlock  (ldlBlock)
{ ; }

void c3_UtilityTextMessageCreateAction::Execute( aui_Control *control, uint32 action, uint32 data )
{

	c3_TextMessage( m_text, m_type, m_callback, m_ldlBlock );
}

void c3_UtilityAbortCleanupAction::Execute(aui_Control *control,
									uint32 action,
									uint32 data )
{
	// That's a design: Deleting from a member function a global object from the same type.
	if (g_utilityAbort)
    {
		g_utilityAbort->Cleanup();
        g_utilityAbort.reset();
    }
}





void c3_TextMessage(MBCHAR const *text, sint32 type, c3_UtilityTextMessageCallback *callback, MBCHAR const *ldlBlock )
{

	if (g_utilityTextMessage) return;

	g_utilityTextMessage = std::make_unique<c3_UtilityTextMessagePopup>( text, type, callback, ldlBlock );
	g_utilityTextMessage->DisplayWindow(text);
}

void c3_KillTextMessage( )
{
	if (g_utilityTextMessage)
    {
		g_utilityTextMessage->Cleanup();
        g_utilityTextMessage.reset();
    }
}





void c3_AbortMessage(MBCHAR const *text, sint32 type, c3_AbortMessageCallback *callback, MBCHAR const *ldlBlock )
{

	if (g_utilityAbort) return;

	g_utilityAbort = std::make_unique<c3_UtilityAbortPopup>( text, type, callback, ldlBlock );
	g_utilityAbort->DisplayWindow(text);
}

void c3_AbortUpdateData( MBCHAR const *text, sint32 percentFilled )
{
	if ( !g_utilityAbort ) return;

	if(text) {
		g_utilityAbort->UpdateData( text );
	}

	if ( g_utilityAbort->m_type == k_UTILITY_PROGRESS_ABORT ) {
		g_utilityAbort->UpdateMeter( percentFilled );
	}
}

void c3_RemoveAbortMessage( )
{
	if ( g_utilityAbort ) {
		g_utilityAbort->RemoveWindow();
	}
}





c3_UtilityAbortPopup::c3_UtilityAbortPopup( MBCHAR const *text, sint32 type, c3_UtilityTextMessageCallback* callback,  MBCHAR const *ldlBlock )
:
    m_window        (nullptr),
    m_text          (nullptr),
    m_meter         (nullptr),
    m_abort         (nullptr),
    m_type          (type),
    m_callback      (callback)
{
	AUI_ERRCODE errcode = AUI_ERRCODE_OK;
	MBCHAR		windowBlock[ k_AUI_LDL_MAXBLOCK + 1 ];

	if (ldlBlock) strlcpy(windowBlock, ldlBlock, sizeof(windowBlock));
	else
	{
		if (type == k_UTILITY_ABORT) strlcpy(windowBlock, "DefaultUtilityAbortPopup", sizeof(windowBlock));
		else strlcpy(windowBlock, "DefaultUtilityAbortProgressPopup", sizeof(windowBlock));
	}

	{
		m_window = std::make_unique<c3_PopupWindow>( &errcode, aui_UniqueId(), windowBlock, 16, AUI_WINDOW_TYPE_FLOATING, false);
		Assert( AUI_NEWOK(m_window, errcode) );
		if ( !AUI_NEWOK(m_window, errcode) ) return;

		m_window->Resize(m_window->Width(),m_window->Height());
		m_window->GrabRegion()->Resize(m_window->Width(),m_window->Height());
		m_window->SetStronglyModal(TRUE);
	}

	Initialize( windowBlock );
}

sint32 c3_UtilityAbortPopup::Initialize( MBCHAR const *windowBlock )
{
	AUI_ERRCODE errcode = AUI_ERRCODE_OK;
	MBCHAR		controlBlock[ k_AUI_LDL_MAXBLOCK + 1 ];

	snprintf(controlBlock, sizeof(controlBlock), "%s.%s", windowBlock, "AbortButton" );
	m_abort = std::make_unique<ctp2_Button>(&errcode, aui_UniqueId(), controlBlock, C3AbortButtonActionCallback, this);
	TestControl( m_abort );

	if (m_type == k_UTILITY_PROGRESS_ABORT)
	{
		snprintf(controlBlock, sizeof(controlBlock), "%s.%s", windowBlock, "Meter" );
		m_meter = std::make_unique<Thermometer>(&errcode, aui_UniqueId(), controlBlock );
		TestControl( m_meter );
	}

	snprintf(controlBlock, sizeof(controlBlock), "%s.%s", windowBlock, "TextMessage");
	m_text = std::make_unique<c3_Static>( &errcode, aui_UniqueId(), controlBlock);
	Assert( AUI_NEWOK(m_text, errcode) );
	if ( !AUI_NEWOK(m_text, errcode) ) return -1;

	errcode = aui_Ldl::SetupHeirarchyFromRoot( windowBlock );
	Assert( AUI_SUCCESS(errcode) );

	return 0;
}

c3_UtilityAbortPopup::~c3_UtilityAbortPopup( )
{
    m_abort.reset();
    m_meter.reset();
    m_text.reset();
    m_window.reset();
}

void c3_UtilityAbortPopup::Cleanup()
{
    m_abort.reset();
    m_meter.reset();
    m_text.reset();
    m_window.reset();
	m_type = 0;
}

void c3_UtilityAbortPopup::DisplayWindow( MBCHAR const *text, sint32 percentFilled )
{
	if ( text ) {
		UpdateData(text);
	}

	if ( m_type == k_UTILITY_PROGRESS_ABORT ) {
		UpdateMeter( percentFilled );
	}
	keypress_RegisterHandler(this);

	AUI_ERRCODE auiErr = c3ui_Get()->AddWindow(m_window.get());
	Assert( auiErr == AUI_ERRCODE_OK );
}

void c3_UtilityAbortPopup::RemoveWindow( )
{
	AUI_ERRCODE auiErr = c3ui_Get()->RemoveWindow( m_window->Id() );
	Assert( auiErr == AUI_ERRCODE_OK );

	keypress_RemoveHandler(this);

    g_utilityAbort.reset();
}

sint32 c3_UtilityAbortPopup::UpdateData( MBCHAR const *text )
{

	if (text)
		m_text->SetText(text);

	m_window->ShouldDraw();

	return 0;
}

sint32 c3_UtilityAbortPopup::UpdateMeter( sint32 percentFilled )
{
	m_meter->SetPercentFilled( percentFilled );

	m_window->ShouldDraw();

	return 0;
}

void c3_UtilityAbortPopup::kh_Close()
{
	if(m_callback)
		m_callback(FALSE);

	RemoveWindow();
}




c3_UtilityPlayerListPopup::c3_UtilityPlayerListPopup( c3_UtilityPlayerListCallback *callback, MBCHAR const *ldlBlock )
:
    m_window        (nullptr),
	m_list          (nullptr),
    m_abort         (nullptr),
    m_kick          (nullptr),
    m_open          (nullptr),
    m_close         (nullptr),
    m_callback      (callback)
{
	MBCHAR		windowBlock[ k_AUI_LDL_MAXBLOCK + 1 ];

	if (ldlBlock) strlcpy(windowBlock, ldlBlock, sizeof(windowBlock));
	else strlcpy(windowBlock, "DefaultUtilityPlayerListPopup", sizeof(windowBlock));

	{
	    AUI_ERRCODE errcode = AUI_ERRCODE_OK;
		m_window = std::make_unique<c3_PopupWindow>( &errcode, aui_UniqueId(), windowBlock, 16, AUI_WINDOW_TYPE_FLOATING, false);
		Assert( AUI_NEWOK(m_window, errcode) );
		if ( !AUI_NEWOK(m_window, errcode) ) return;

		m_window->Resize(m_window->Width(),m_window->Height());
		m_window->GrabRegion()->Resize(m_window->Width(),m_window->Height());
		m_window->SetStronglyModal(TRUE);
	}

    Initialize( windowBlock );
}

sint32 c3_UtilityPlayerListPopup::Initialize( MBCHAR const *windowBlock )
{
	AUI_ERRCODE errcode = AUI_ERRCODE_OK;
	MBCHAR		controlBlock[ k_AUI_LDL_MAXBLOCK + 1 ];

	snprintf(controlBlock, sizeof(controlBlock), "%s.%s", windowBlock, "AbortButton" );
	m_abort = std::make_unique<ctp2_Button>(&errcode, aui_UniqueId(), controlBlock, C3UtilityPlayerListButtonActionCallback, this);
	TestControl( m_abort );

	snprintf(controlBlock, sizeof(controlBlock), "%s.%s", windowBlock, "KickButton" );
	m_kick = std::make_unique<ctp2_Button>(&errcode, aui_UniqueId(), controlBlock, C3UtilityPlayerListButtonActionCallback, this);
	TestControl( m_kick );

	snprintf(controlBlock, sizeof(controlBlock), "%s.%s", windowBlock, "OpenButton" );
	m_open = std::make_unique<ctp2_Button>(&errcode, aui_UniqueId(), controlBlock, C3UtilityPlayerListButtonActionCallback, this);
	TestControl( m_open );

	snprintf(controlBlock, sizeof(controlBlock), "%s.%s", windowBlock, "CloseButton" );
	m_close = std::make_unique<ctp2_Button>(&errcode, aui_UniqueId(), controlBlock, C3UtilityPlayerListButtonActionCallback, this);
	TestControl( m_close );

	snprintf(controlBlock, sizeof(controlBlock), "%s.%s", windowBlock, "PlayerList" );
	m_list = std::make_unique<ctp2_ListBox>(&errcode, aui_UniqueId(), controlBlock, nullptr, nullptr);
	Assert( AUI_NEWOK(m_list, errcode) );
	if ( !AUI_NEWOK(m_list, errcode) ) return -1;

	errcode = aui_Ldl::SetupHeirarchyFromRoot( windowBlock );
	Assert( AUI_SUCCESS(errcode) );

	return 0;
}

c3_UtilityPlayerListPopup::~c3_UtilityPlayerListPopup( )
{
    m_abort.reset();
    m_kick.reset();
    m_open.reset();
    m_close.reset();
    if (m_list)
    {
        m_list->Clear();
    }
    m_list.reset();
    m_window.reset();
}

void c3_UtilityPlayerListPopup::Cleanup( )
{
    m_abort.reset();
    m_kick.reset();
    m_open.reset();
    m_close.reset();
    if (m_list)
    {
        m_list->Clear();
    }
    m_list.reset();
    m_window.reset();
	m_callback = nullptr;
}

void c3_UtilityPlayerListPopup::DisplayWindow( )
{
	UpdateData();

	AUI_ERRCODE auiErr = c3ui_Get()->AddWindow(m_window.get());
	Assert( auiErr == AUI_ERRCODE_OK );

	keypress_RegisterHandler(this);
}

void c3_UtilityPlayerListPopup::RemoveWindow( )
{
	AUI_ERRCODE auiErr = c3ui_Get()->RemoveWindow(m_window->Id());
	Assert(auiErr == AUI_ERRCODE_OK);

	keypress_RemoveHandler(this);
}

void c3_UtilityPlayerListPopup::kh_Close()
{
	RemoveWindow();
}

sint32 c3_UtilityPlayerListPopup::UpdateData( )
{
	MBCHAR ldlBlock[k_AUI_LDL_MAXBLOCK + 1];
	strlcpy(ldlBlock, "DoubleListItem", sizeof(ldlBlock));

	MBCHAR strbuf[256];
	AUI_ERRCODE		retval;

	m_list->Clear();

	for (sint32 i = 1 ; i < k_MAX_PLAYERS ; ++i)
	{
		if (player_arr_Get()[i])
        {
			strlcpy(strbuf, player_Get(i)->GetLeaderName(), sizeof(strbuf));
            m_list->AddItem(std::make_unique<DoubleListItem>
                                (&retval,
                                 strbuf,
                                 i,
                                 network_Get().GetStatusString(i),
                                 ldlBlock
                                ).release()
                           );
		}
	}

	return 0;
}

sint32 c3_UtilityPlayerListPopup::EnableButtons( )
{
	m_kick->Enable( TRUE );
	m_open->Enable( TRUE );
	m_close->Enable( TRUE );

	return 1;
}

sint32 c3_UtilityPlayerListPopup::DisableButtons( )
{
	m_kick->Enable( FALSE );
	m_open->Enable( FALSE );
	m_close->Enable( FALSE );

	return 1;
}

void c3_UtilityPlayerListPopup::SetText(MBCHAR * s, sint32 index)
{
	DoubleListItem * item = (DoubleListItem *) m_list->GetItemByIndex(index);

	if (item)
    {
		item->SetSecondColumn(s);
	}
}













DoubleListItem::DoubleListItem(AUI_ERRCODE *retval, MBCHAR const *name, sint32 value, MBCHAR const *text, MBCHAR const *ldlBlock)
:
	aui_ImageBase   (ldlBlock),
	aui_TextBase    (ldlBlock, (MBCHAR const *) nullptr),
	c3_ListItem     (retval, ldlBlock)
{
	Assert( AUI_SUCCESS(*retval) );
	if ( !AUI_SUCCESS(*retval) ) return;

	*retval = InitCommonLdl(name, value, text, ldlBlock);
	Assert( AUI_SUCCESS(*retval) );
}

AUI_ERRCODE DoubleListItem::InitCommonLdl(MBCHAR const *name, sint32 value, MBCHAR const *text, MBCHAR const *ldlBlock)
{
	MBCHAR			block[ k_AUI_LDL_MAXBLOCK + 1 ];
	AUI_ERRCODE		retval;

	strlcpy(m_name, name, sizeof(m_name));
	m_value = value;

	if ( text ) {
		strlcpy( m_text, text, sizeof(m_text) );
	}
	else {
		strlcpy( m_text, "", sizeof(m_text) );
	}

	snprintf(block, sizeof(block), "%s.%s", ldlBlock, "Name");
	AddChild(std::make_unique<c3_Static>(&retval, aui_UniqueId(), block).release());

	snprintf(block, sizeof(block), "%s.%s", ldlBlock, "Text");
	AddChild(std::make_unique<c3_Static>(&retval, aui_UniqueId(), block).release());

	Update();

	return AUI_ERRCODE_OK;
}

void DoubleListItem::Update()
{

	c3_Static *subItem;

	subItem = (c3_Static *)GetChildByIndex(0);
	subItem->SetText(m_name);

	subItem = (c3_Static *)GetChildByIndex(1);
	subItem->SetText(m_text);

}

sint32 DoubleListItem::SetSecondColumn( MBCHAR *s)
{
	c3_Static * subItem = (c3_Static *) GetChildByIndex(1);
	subItem->SetText(s);

	return 0;
}

sint32 DoubleListItem::Compare(c3_ListItem *item2, uint32 column)
{
	switch (column)
    {
    default:
        return 0;

	case 0:
	case 1:
        {
		    c3_Static * i1 = (c3_Static *)this->GetChildByIndex(column);
		    c3_Static * i2 = (c3_Static *)item2->GetChildByIndex(column);

	    MBCHAR			strbuf1[256];
	    strlcpy(strbuf1, i1->GetText(), sizeof(strbuf1));
        MBCHAR			strbuf2[256];
	    strlcpy(strbuf2, i2->GetText(), sizeof(strbuf2));

		    return (strbuf1[0] - strbuf2[0]);
        }
	}
}

void c3Expel_Initialize( c3_ExpelCallback callback )
{
	if ( g_expelPopup ) return;

	g_expelPopup = std::make_unique<c3_ExpelPopup>( callback );
}

void c3Expel_Cleanup( )
{
    g_expelPopup.reset();
}


static std::unique_ptr<c3_UtilityTextFieldPopup>	s_nameTheCityPopup;
static Unit						s_unit;

void NameTheCityDialogBoxCallback(MBCHAR const * text, sint32 val2, void *data)
{
	if (!val2) return;

	if (s_unit.IsValid())
    {
		if (s_unit.GetOwner() == selitem_Get()->GetVisiblePlayer())
        {
			s_unit.GetData()->GetCityData()->SetName(text);
			MainControlPanel::UpdateCityList();
		}
	}
}

void c3_utilitydialogbox_NameCity(Unit city)
{
	MBCHAR		nameText[k_MAX_NAME_LEN];

	if (!city.IsValid()) return;

	s_unit = city;

	strlcpy(nameText, city.GetData()->GetCityData()->GetName(), sizeof(nameText));

	if ( !s_nameTheCityPopup ) {

		s_nameTheCityPopup = std::make_unique<c3_UtilityTextFieldPopup>(NameTheCityDialogBoxCallback,
									nullptr,
									nameText,
									nullptr,
									"NewNameTheCityPopup"
									);
	} else {

		if (!s_nameTheCityPopup->m_default_text.empty()) {
			s_nameTheCityPopup->m_default_text = nameText;

			s_nameTheCityPopup->UpdateData();

			s_nameTheCityPopup->m_text->SetKeyboardFocus();
			s_nameTheCityPopup->m_text->SelectAll();
		}
	}

	s_nameTheCityPopup->DisplayWindow();
}

void c3_utilitydialogbox_NameCityCleanup()
{
    s_nameTheCityPopup.reset();
}




std::unique_ptr<c3_UtilityTextFieldPopup>		s_genericTextEntryPopup;

void c3_utilitydialogbox_TextFieldDialog(MBCHAR *titleText,
								   MBCHAR *defaultText,
								   MBCHAR *messageText,
								   c3_UtilityTextFieldCallback *callback,
								   MBCHAR const *ldlBlock)
{

	if ( !s_genericTextEntryPopup ) {

		s_genericTextEntryPopup = std::make_unique<c3_UtilityTextFieldPopup>(callback,
																titleText,
																defaultText,
																messageText,
																ldlBlock);
		Assert(s_genericTextEntryPopup);
	}

	if (!s_genericTextEntryPopup->m_default_text.empty()) {
		s_genericTextEntryPopup->m_default_text = defaultText;
	}


	s_genericTextEntryPopup->UpdateData();

	s_genericTextEntryPopup->m_text->SetKeyboardFocus();
	s_genericTextEntryPopup->m_text->SelectAll();

	s_genericTextEntryPopup->DisplayWindow();
}

void c3_utilitydialogbox_CleanupTextFieldDialog()
{
    s_genericTextEntryPopup.reset();
}
