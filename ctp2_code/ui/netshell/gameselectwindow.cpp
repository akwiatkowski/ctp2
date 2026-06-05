//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ source
// Description  : Game type selection UI
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
// - Default values for new multiplayer game are now taken from profile, like:
//   - World size
//   - World shape
//   - World type wet/dry
//   - World type warm/cold
//   - World type ocean/land
//   - World type island/continent
//   - World type homo/deverse
//   - World type goodcount
// - Default end age is set to last age in database.
//
//----------------------------------------------------------------------------

#include "ctp/c3.h"

#include "ui/aui_common/aui_ldl.h"
#include "ui/aui_common/aui_uniqueid.h"
#include "ui/aui_common/aui_static.h"
#include "ui/aui_common/aui_screen.h"
#include "ui/aui_common/aui_button.h"

#include "ui/aui_ctp2/c3_button.h"
#include "ui/aui_ctp2/c3_static.h"
#include "ui/interface/loadsavewindow.h"


#include "ui/netshell/netshell.h"
#include "ui/netshell/ns_customlistbox.h"
#include "ui/netshell/ns_civlistbox.h"
#include "ui/netshell/passwordscreen.h"




#include "ui/netshell/gameselectwindow.h"
#include "ui/netshell/allinonewindow.h"
#include "ui/netshell/playerselectwindow.h"
#include "ui/netshell/passwordscreen.h"

#include "ui/netshell/netshell_game.h"

#include "ui/interface/scenariowindow.h"

#include "ui/interface/spnewgamewindow.h"
#include "gs/database/profileDB.h"
#include "AgeRecord.h"

static GameSelectWindow * g_gameSelectWindow = nullptr;
static StartSelectingWindow *g_startSelectingWindow = nullptr;

GameSelectWindow * gameselectwindow_Get()
{
    return g_gameSelectWindow;
}

GameSelectWindow::GameSelectWindow(
	AUI_ERRCODE *retval )
	:
	ns_Window(
		retval,
		aui_UniqueId(),
		"gameselectwindow",
		0,
		AUI_WINDOW_TYPE_STANDARD )
{
	Assert( AUI_SUCCESS(*retval) );
	if ( !AUI_SUCCESS(*retval) ) return;

	*retval = InitCommon();
	Assert( AUI_SUCCESS(*retval) );
	if ( !AUI_SUCCESS(*retval) ) return;

	*retval = CreateControls();
	Assert( AUI_SUCCESS(*retval) );
}


AUI_ERRCODE GameSelectWindow::InitCommon( )
{
	g_gameSelectWindow = this;

	m_controls = new aui_Control *[ m_numControls = CONTROL_MAX ];
	Assert( m_controls != nullptr );
	if ( !m_controls ) return AUI_ERRCODE_MEMALLOCFAILED;
	memset( m_controls, 0, m_numControls * sizeof( aui_Control *) );

	return AUI_ERRCODE_OK;
}


AUI_ERRCODE GameSelectWindow::CreateControls( )
{
	AUI_ERRCODE errcode = AUI_ERRCODE_OK;




	aui_Control *control;









	control = new c3_Static(
		&errcode,
		aui_UniqueId(),
		"gameselectwindow.titlestatictext" );
	Assert( AUI_NEWOK(control,errcode) );
	if ( !AUI_NEWOK(control,errcode) ) return errcode;
	m_controls[ CONTROL_TITLESTATICTEXT ] = control;

	control = new ns_GameSetupListBox(
		&errcode,
		aui_UniqueId(),
		"gameselectwindow.gamenamelistbox" );
	Assert( AUI_NEWOK(control,errcode) );
	if ( !AUI_NEWOK(control,errcode) ) return errcode;
	m_controls[ CONTROL_GAMENAMELISTBOX ] = control;









	control = spNew_ctp2_Button(
		&errcode,
		"gameselectwindow",
		"deletebutton",
		nullptr);
	Assert( AUI_NEWOK(control,errcode) );
	if ( !AUI_NEWOK(control,errcode) ) return errcode;
	m_controls[ CONTROL_DELETEBUTTON ] = control;

	control = new aui_Button(
		&errcode,
		aui_UniqueId(),
		"gameselectwindow.okbutton" );
	Assert( AUI_NEWOK(control,errcode) );
	if ( !AUI_NEWOK(control,errcode) ) return errcode;
	m_controls[ CONTROL_OKBUTTON ] = control;

	control = new aui_Button(
		&errcode,
		aui_UniqueId(),
		"gameselectwindow.cancelbutton" );
	Assert( AUI_NEWOK(control,errcode) );
	if ( !AUI_NEWOK(control,errcode) ) return errcode;
	m_controls[ CONTROL_CANCELBUTTON ] = control;




	aui_Ldl::SetupHeirarchyFromRoot( "gameselectwindow" );




	aui_Action *action;






	action = new DeleteButtonAction;
	Assert( action != nullptr );
	if ( !action ) return AUI_ERRCODE_MEMALLOCFAILED;
	m_controls[ CONTROL_DELETEBUTTON ]->SetAction( action );

	action = new OKButtonAction;
	Assert( action != nullptr );
	if ( !action ) return AUI_ERRCODE_MEMALLOCFAILED;
	m_controls[ CONTROL_OKBUTTON ]->SetAction( action );

	action = new CancelButtonAction;
	Assert( action != nullptr );
	if ( !action ) return AUI_ERRCODE_MEMALLOCFAILED;
	m_controls[ CONTROL_CANCELBUTTON ]->SetAction( action );

	action = new GameListBoxAction;
	Assert( action != nullptr );
	if ( !action ) return AUI_ERRCODE_MEMALLOCFAILED;
	m_controls[ CONTROL_GAMENAMELISTBOX ]->SetAction( action );





	((aui_ListBox *)m_controls[ CONTROL_GAMENAMELISTBOX ])->
		SetForceSelect( TRUE );

	Update();

	return AUI_ERRCODE_OK;
}

GameSelectWindow::~GameSelectWindow()
{
	if (this == g_gameSelectWindow)
	{
		g_gameSelectWindow = nullptr;
	}
}


nf_GameSetup *GameSelectWindow::GetGameSetup(NETFunc::Session *session) {
	ns_GameSetupListBox *l = (ns_GameSetupListBox *)(FindControl( GameSelectWindow ::CONTROL_GAMENAMELISTBOX ));
	ns_GameSetupListBox::iterator i;
	nf_GameSetup *s;

	for(i = l->begin(); i != l->end(); i++) {
		s = (*i);
		if(strcmp(session->GetName(), s->GetName()) == 0)
			return s;
	}

	NETFunc::Game game = NETFunc::Game(session);
	s = new nf_GameSetup(&game);
	l->InsertItem(s);

	return s;
}

void GameSelectWindow::Update()
{
	ns_GameSetupListBox *listbox = (ns_GameSetupListBox *)
		(FindControl( GameSelectWindow::CONTROL_GAMENAMELISTBOX ));
	ns_GameSetupItem *item = (ns_GameSetupItem *)listbox->GetSelectedItem();
	c3_Button *b_ok =
		(c3_Button *)(FindControl( GameSelectWindow::CONTROL_OKBUTTON ));
	c3_Button *b_del =
		(c3_Button *)(FindControl( GameSelectWindow::CONTROL_DELETEBUTTON ));

	if(item) {
		b_ok->Enable(TRUE);
		b_del->Enable(TRUE);
	} else {
		b_ok->Enable(FALSE);
		b_del->Enable(FALSE);
	}
}

AUI_ERRCODE GameSelectWindow::Idle( )
{
	while (NETFunc::Message * m = netfunc_Get()->GetMessage())
    {
		netfunc_Get()->HandleMessage(m);

		if (dp_SESSIONLOST_PACKET_ID == m->GetCode())
		{
			passwordscreen_displayMyWindow(PASSWORDSCREEN_MODE_CONNECTIONLOST);
		}

		delete m;
	}

	return AUI_ERRCODE_OK;
}

void GameSelectWindow::GameListBoxAction::Execute(
	aui_Control *control,
	uint32 action,
	uint32 data )
{
	switch ( action )
	{
	case AUI_LISTBOX_ACTION_SELECT:
		((GameSelectWindow *)control->GetParent())->Update();
		break;

	case AUI_LISTBOX_ACTION_DOUBLECLICKSELECT:
	{
		aui_ListBox *listbox = (aui_ListBox *)control;
		sint32 index = listbox->ExtractDoubleClickedItem( data );
		aui_Item *item = listbox->GetItemByIndex( index );
		listbox->SelectItem( item );

		c3_Button *button = (c3_Button *)((GameSelectWindow *)control->
			GetParentWindow())->FindControl( CONTROL_OKBUTTON );
		button->GetAction()->Execute( button, AUI_BUTTON_ACTION_EXECUTE, 0 );
		break;
	}

	default:
		break;
	}
}

AUI_ERRCODE GameSelectWindow::SetParent( aui_Region *region )
{


















	return ns_Window::SetParent( region );
}


void GameSelectWindow::DeleteButtonAction::Execute(
	aui_Control *control,
	uint32 action,
	uint32 data )
{
	if ( action != (uint32)AUI_BUTTON_ACTION_EXECUTE ) return;
	GameSelectWindow *w = (GameSelectWindow *)control->GetParentWindow();
	ns_GameSetupListBox *listbox = (ns_GameSetupListBox *)w->
		FindControl( GameSelectWindow ::CONTROL_GAMENAMELISTBOX );
	ns_GameSetupItem *item = (ns_GameSetupItem *)listbox->GetSelectedItem();

	if(item) {
		nf_GameSetup *game = item->GetNetShellObject()->GetNETFuncObject();
		listbox->DeleteItem(game);
		w->Update();
	}
}


void GameSelectWindow::OKButtonAction::Execute(
	aui_Control *control,
	uint32 action,
	uint32 data )
{
	if ( action != (uint32)AUI_BUTTON_ACTION_EXECUTE ) return;

	passwordscreen_displayMyWindow( PASSWORDSCREEN_MODE_ASK );
}


uint32 TellEricAboutThisBug( aui_Window *w )
{
	uint32 id = 0;
	if ( w ) id = w->Id();
	return id;
}

void GameSelectWindow::PasswordScreenDone( MBCHAR *password )
{

	AllinoneWindow *w = allinonewindow_Get();
	AllinoneWindow::Mode mode = AllinoneWindow::JOIN;

	TellEricAboutThisBug( loadsavewindow_Get() );

	if ( loadsavewindow_Get() && aui_ui_Get()->GetChild( loadsavewindow_Get()->Id() ) )
	{

		SaveInfo *saveInfo = loadsavewindow_Get()->GetSaveInfo();
		Assert( saveInfo != nullptr );
		if ( saveInfo )
		{
			loadsavescreen_removeMyWindow( AUI_BUTTON_ACTION_EXECUTE );

			gamesetup_Get() = saveInfo->gameSetup;

			NETFunc::Session *s = (NETFunc::Session *)&gamesetup_Get();

			dp_session_t *sess =
				(dp_session_t *)((uint8*)s + sizeof(NETFunc::Key));

			if(sess->sessionType == 0) {
				sess->sessionType = GAMEID;
				char name[ dp_SNAMELEN + 1 ];
				ns_String format( "strings.newgame" );

				char truncname[ dp_PNAMELEN + 1 ];
				strcpy( truncname, playersetup_Get().GetName() );

				sint32 trunclen = dp_SNAMELEN -
					( strlen( truncname ) + ( strlen( format.GetString() ) - 2  ) );

			if ( trunclen < 0 )
			{
				memset( truncname, 0, sizeof( truncname ) );
				// TODO(phase-2): strncpy → strlcpy — non-standard length argument, requires manual review
				strncpy(
					truncname,
					playersetup_Get().GetName(),
					strlen( truncname ) + trunclen );
			}

				snprintf(name, sizeof(name), format.GetString(), truncname );

				strcpy( sess->sessionName, name );
			}

			mode = AllinoneWindow::CONTINUE_CREATE;

			switch ( loadsavewindow_Get()->GetType() )
			{
			case LSS_LOAD_GAME:
			case LSS_LOAD_MP:
				w->SetScenarioGame( FALSE );

				gamesetup_Get().SetSavedId( 1 );
				break;








			default:

				Assert( FALSE );
				break;
			}
		}
	}

	else
	{
		ns_GameSetupListBox *listbox = (ns_GameSetupListBox *)
			FindControl( GameSelectWindow::CONTROL_GAMENAMELISTBOX );
		ns_GameSetupItem *item = (ns_GameSetupItem *)listbox->GetSelectedItem();

		if(item) {

			((aui_ListBox *)listbox)->RemoveItem( item->Id() );
			((aui_ListBox *)listbox)->InsertItem( item, 0 );
			listbox->SelectItem( (sint32)0 );

			gamesetup_Get() = *item->GetNetShellObject()->GetNETFuncObject();

			mode = AllinoneWindow::CREATE;
			gamesetup_Get().SetSavedId( 0 );
		}
	}

	if ( mode != AllinoneWindow::JOIN )
	{
		MBCHAR temp[ dp_PASSWORDLEN + 1 ] = "";
		if ( password )
		{
			strlcpy( temp, password, sizeof( temp ) );
			for ( size_t i = 0; i < strlen( temp ); i++ )
			{
				temp[i] = static_cast<MBCHAR>(tolower(temp[i]));
			}
		}

		gamesetup_Get().SetPassword( temp );
		gamesetup_Get().SetSize( k_NS_MAX_HUMANS );
		gamesetup_Get().SetClosed( false );
		gamesetup_Get().SetSyncLaunch( true );

		PlayerSelectWindow *psw = (PlayerSelectWindow *)netshell_Get()->
			FindWindow(NetShell::WINDOW_PLAYERSELECT);
		psw->GetPlayerSetup(netfunc_Get()->GetPlayer())->Reset();

		playersetup_Get().SetReadyToLaunch(false);
		if(netfunc_Get()->Create(&gamesetup_Get()) == NETFunc::OK) {
			w->SetMode(mode);
			netshell_Get()->GotoScreen( NetShell::SCREEN_ALLINONE );
			w->Update();
		}
	}
}


void GameSelectWindow::CancelButtonAction::Execute(
	aui_Control *control,
	uint32 action,
	uint32 data )
{
	if ( action != (uint32)AUI_BUTTON_ACTION_EXECUTE ) return;

	netshell_Get()->GotoScreen( NetShell::SCREEN_STARTSELECTING );
}




StartSelectingWindow::StartSelectingWindow(
	AUI_ERRCODE *retval )
	:
	ns_Window(
		retval,
		aui_UniqueId(),
		"startselectingwindow",
		0,
		AUI_WINDOW_TYPE_STANDARD )
{
	if ( !AUI_SUCCESS(*retval) ) return;

	*retval = InitCommon();
	if ( !AUI_SUCCESS(*retval) ) return;

	*retval = CreateControls();
}


AUI_ERRCODE StartSelectingWindow::InitCommon( )
{
	g_startSelectingWindow = this;

	m_controls = new aui_Control *[ m_numControls = CONTROL_MAX ];
	Assert( m_controls != nullptr );
	if ( !m_controls ) return AUI_ERRCODE_MEMALLOCFAILED;
	memset( m_controls, 0, m_numControls * sizeof( aui_Control *) );

	loadsavescreen_Initialize( StartSelectingLoadSaveCallback );

	return AUI_ERRCODE_OK;
}


AUI_ERRCODE StartSelectingWindow::CreateControls( )
{
	AUI_ERRCODE errcode = AUI_ERRCODE_OK;




	aui_Control *control;









	control = new c3_Static(
		&errcode,
		aui_UniqueId(),
		"startselectingwindow.titlestatictext" );
	Assert( AUI_NEWOK(control,errcode) );
	if ( !AUI_NEWOK(control,errcode) ) return errcode;
	m_controls[ CONTROL_TITLESTATICTEXT ] = control;

	control = spNew_ctp2_Button(
		&errcode,
		"startselectingwindow",
		"newbutton",
		nullptr);
	Assert( AUI_NEWOK(control,errcode) );
	if ( !AUI_NEWOK(control,errcode) ) return errcode;
	m_controls[ CONTROL_NEWBUTTON ] = control;

	control = spNew_ctp2_Button(
		&errcode,
		"startselectingwindow",
		"gamesetupbutton",
		nullptr);
	Assert( AUI_NEWOK(control,errcode) );
	if ( !AUI_NEWOK(control,errcode) ) return errcode;
	m_controls[ CONTROL_GAMESETUPBUTTON ] = control;

	control = spNew_ctp2_Button(
		&errcode,
		"startselectingwindow",
		"savedbutton",
		nullptr);
	Assert( AUI_NEWOK(control,errcode) );
	if ( !AUI_NEWOK(control,errcode) ) return errcode;
	m_controls[ CONTROL_SAVEDBUTTON ] = control;

	control = spNew_ctp2_Button(
		&errcode,
		"startselectingwindow",
		"scenariobutton",
		nullptr);
	Assert( AUI_NEWOK(control,errcode) );
	if ( !AUI_NEWOK(control,errcode) ) return errcode;
	m_controls[ CONTROL_SCENARIOBUTTON ] = control;

	control = new aui_Button(
		&errcode,
		aui_UniqueId(),
		"startselectingwindow.cancelbutton" );
	Assert( AUI_NEWOK(control,errcode) );
	if ( !AUI_NEWOK(control,errcode) ) return errcode;
	m_controls[ CONTROL_CANCELBUTTON ] = control;




	aui_Ldl::SetupHeirarchyFromRoot( "startselectingwindow" );




	aui_Action *action;






	action = new NewButtonAction;
	Assert( action != nullptr );
	if ( !action ) return AUI_ERRCODE_MEMALLOCFAILED;
	m_controls[ CONTROL_NEWBUTTON ]->SetAction( action );

	action = new GameSetupButtonAction;
	Assert( action != nullptr );
	if ( !action ) return AUI_ERRCODE_MEMALLOCFAILED;
	m_controls[ CONTROL_GAMESETUPBUTTON ]->SetAction( action );

	action = new SavedButtonAction;
	Assert( action != nullptr );
	if ( !action ) return AUI_ERRCODE_MEMALLOCFAILED;
	m_controls[ CONTROL_SAVEDBUTTON ]->SetAction( action );

	action = new ScenarioButtonAction;
	Assert( action != nullptr );
	if ( !action ) return AUI_ERRCODE_MEMALLOCFAILED;
	m_controls[ CONTROL_SCENARIOBUTTON ]->SetAction( action );

	action = new CancelButtonAction;
	Assert( action != nullptr );
	if ( !action ) return AUI_ERRCODE_MEMALLOCFAILED;
	m_controls[ CONTROL_CANCELBUTTON ]->SetAction( action );





	return AUI_ERRCODE_OK;
}


StartSelectingWindow::~StartSelectingWindow()
{
	loadsavescreen_Cleanup();

	if (this == g_startSelectingWindow)
	{
		g_startSelectingWindow = nullptr;
	}
}


AUI_ERRCODE StartSelectingWindow::Idle( )
{
	while (NETFunc::Message * m = netfunc_Get()->GetMessage())
    {
		netfunc_Get()->HandleMessage(m);

		if (dp_SESSIONLOST_PACKET_ID == m->GetCode())
		{
			passwordscreen_displayMyWindow(PASSWORDSCREEN_MODE_CONNECTIONLOST);
		}

		delete m;
	}

	return AUI_ERRCODE_OK;
}


AUI_ERRCODE StartSelectingWindow::SetParent( aui_Region *region )
{
	return ns_Window::SetParent( region );
}


void StartSelectingWindow::NewButtonAction::Execute(
	aui_Control *control,
	uint32 action,
	uint32 data )
{
	if ( action != (uint32)AUI_BUTTON_ACTION_EXECUTE ) return;

	allinonewindow_Get()->SetScenarioGame(FALSE);

	GameSelectWindow *w = g_gameSelectWindow;
	ns_GameSetupListBox *listbox = (ns_GameSetupListBox *)w->
		FindControl( GameSelectWindow::CONTROL_GAMENAMELISTBOX );

	nf_GameSetup *s = new nf_GameSetup;
	if ( s )
	{
		char name[ dp_SNAMELEN + 1 ];
		ns_String format( "strings.newgame" );

		char truncname[ dp_PNAMELEN + 1 ];
		strcpy( truncname, playersetup_Get().GetName() );

		sint32 trunclen = dp_SNAMELEN -
			( strlen( truncname ) + ( strlen( format.GetString() ) - 2  ) );

		if ( trunclen < 0 )
		{
			memset( truncname, 0, sizeof( truncname ) );
			// TODO(phase-2): strncpy → strlcpy — non-standard length argument, requires manual review
			strncpy(
				truncname,
				playersetup_Get().GetName(),
				strlen( truncname ) + trunclen );
		}

		snprintf(name, sizeof(name), format.GetString(), truncname );

		char test[ dp_SNAMELEN + 1 ];
		strlcpy( test, name, sizeof( test ) );
		sint32 num = 2;
		while ( true )
		{
			sint32 i;
			for ( i = 0; i < listbox->NumItems(); i++ )
			{
				NETFunc::GameSetup *game = (NETFunc::GameSetup *)
					((ns_GameSetupItem *)listbox->GetItemByIndex( i ))->
					GetNetShellObject()->GetNETFuncObject();

				char *existing = game->GetName();
				if ( strnicmp( test, existing, dp_SNAMELEN ) == 0 )
					break;
			}

			if ( i == listbox->NumItems() )
			{
				strlcpy( name, test, sizeof( name ) );
				break;
			}

			snprintf(test, sizeof(test), "%s %d", name, num++ );
		}

		s->SetName( name );
		//Special rules
		s->SetBloodlust(!profiledb_Get()->IsAlienEndGameOn());
		s->SetPollution(static_cast<char>(profiledb_Get()->IsPollutionRule()));

		//Ages
		s->SetStartAge(0);
		s->SetEndAge(static_cast<char>(g_theAgeDB->NumRecords() - 1));
		//World size and shape
		s->SetMapSize(static_cast<char>(profiledb_Get()->GetMapSize()));
		s->SetWorldShape(static_cast<char>(profiledb_Get()->GetWorldShape()));
		//World types
		s->SetWorldType1(static_cast<char>(profiledb_Get()->GetWetDry()));
		s->SetWorldType2(static_cast<char>(profiledb_Get()->GetWarmCold()));
		s->SetWorldType3(static_cast<char>(profiledb_Get()->GetOceanLand()));
		s->SetWorldType4(static_cast<char>(profiledb_Get()->GetIslandContinent()));
		s->SetWorldType5(static_cast<char>(profiledb_Get()->GetHomoDiverse()));
		s->SetWorldType6(static_cast<char>(profiledb_Get()->GetGoodCount()));

		//Level of difficuilties
		s->SetDifficulty1(static_cast<char>(profiledb_Get()->GetDifficulty()));
		s->SetDifficulty2(static_cast<char>(profiledb_Get()->GetRiskLevel()));
		listbox->InsertItem( s );
		listbox->SelectItem(listbox->FindItem(s));

		c3_Button *button = (c3_Button *)w->FindControl( GameSelectWindow::CONTROL_OKBUTTON );
		button->GetAction()->Execute( button, AUI_BUTTON_ACTION_EXECUTE, 0 );
	}
}


void StartSelectingWindow::GameSetupButtonAction::Execute(
	aui_Control *control,
	uint32 action,
	uint32 data )
{
	if ( action != (uint32)AUI_BUTTON_ACTION_EXECUTE ) return;

	netshell_Get()->GotoScreen( NetShell::SCREEN_GAMESELECT );
}


void StartSelectingWindow::SavedButtonAction::Execute(
	aui_Control *control,
	uint32 action,
	uint32 data )
{
	if ( action != (uint32)AUI_BUTTON_ACTION_EXECUTE ) return;

	uint32 type = LSS_LOAD_MP;

	NETFunc::TransportSetup *t = netfunc_Get()->GetTransport();
	if ( t->GetType() == NETFunc::Transport::UNKNOWN )
	{
		if ( ((FakeTransport *)t)->GetSubType() == FakeTransport::EMAIL )
		{


		}
	}

	loadsavescreen_Initialize( StartSelectingLoadSaveCallback );
	loadsavescreen_displayMyWindow( type );
}

void gameselectwindow_scenarioExitCallback(aui_Control *control,
										   uint32 action,
										   uint32 data,
										   void *cookie )
{

	GameSelectWindow *w = g_gameSelectWindow;
	ns_GameSetupListBox *listbox = (ns_GameSetupListBox *)w->
		FindControl( GameSelectWindow::CONTROL_GAMENAMELISTBOX );

	nf_GameSetup *s = new nf_GameSetup;
	if ( s )
	{
		char name[ dp_SNAMELEN + 1 ];
		ns_String format( "strings.newgame" );

		char truncname[ dp_PNAMELEN + 1 ];
		strcpy( truncname, playersetup_Get().GetName() );

		sint32 trunclen = dp_SNAMELEN -
			( strlen( truncname ) + ( strlen( format.GetString() ) - 2  ) );

		if ( trunclen < 0 )
		{
			memset( truncname, 0, sizeof( truncname ) );
			// TODO(phase-2): strncpy → strlcpy — non-standard length argument, requires manual review
			strncpy(
				truncname,
				playersetup_Get().GetName(),
				strlen( truncname ) + trunclen );
		}

		snprintf(name, sizeof(name), format.GetString(), truncname );

		char test[ dp_SNAMELEN + 1 ];
		strlcpy( test, name, sizeof( test ) );
		sint32 num = 2;
		while ( true )
		{
			sint32 i;
			for ( i = 0; i < listbox->NumItems(); i++ )
			{
				NETFunc::GameSetup *game = (NETFunc::GameSetup *)
					((ns_GameSetupItem *)listbox->GetItemByIndex( i ))->
					GetNetShellObject()->GetNETFuncObject();

				char *existing = game->GetName();
				if ( strnicmp( test, existing, dp_SNAMELEN ) == 0 )
					break;
			}

			if ( i == listbox->NumItems() )
			{
				strlcpy( name, test, sizeof( name ) );
				break;
			}

			snprintf(test, sizeof(test), "%s %d", name, num++ );
		}

		s->SetName( name );
		listbox->InsertItem( s );
		listbox->SelectItem(listbox->FindItem(s));

		c3_Button *button = (c3_Button *)w->FindControl( GameSelectWindow::CONTROL_OKBUTTON );
		button->GetAction()->Execute( button, AUI_BUTTON_ACTION_EXECUTE, 0 );
	}
	allinonewindow_Get()->SetScenarioGame(TRUE);
	passwordscreen_displayMyWindow(PASSWORDSCREEN_MODE_ASK);
}

void StartSelectingWindow::ScenarioButtonAction::Execute(
	aui_Control *control,
	uint32 action,
	uint32 data )
{
	if ( action != (uint32)AUI_BUTTON_ACTION_EXECUTE ) return;








	scenarioscreen_displayMyWindow();
	scenarioscreen_SetExitCallback(gameselectwindow_scenarioExitCallback);
}


void StartSelectingWindow::CancelButtonAction::Execute(
	aui_Control *control,
	uint32 action,
	uint32 data )
{
	if ( action != (uint32)AUI_BUTTON_ACTION_EXECUTE ) return;

	netshell_Get()->GotoScreen( NetShell::SCREEN_LOBBY );
}




void StartSelectingLoadSaveCallback(
	aui_Control *control,
	uint32 action,
	uint32 data,
	void* cookie )
{
	if ( action != (uint32)AUI_BUTTON_ACTION_EXECUTE ) return;

	if ( loadsavewindow_Get()->GetSaveInfo() )
		passwordscreen_displayMyWindow( PASSWORDSCREEN_MODE_ASK );
}
