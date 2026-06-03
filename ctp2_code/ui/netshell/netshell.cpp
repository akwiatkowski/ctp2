//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ source
// Description  : Network (multiplayer) user interface
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
// - Prevented memory leaks and debug exit popups.
// - Initialized local variables. (Sep 9th 2005 Martin G�hmann)
//
//----------------------------------------------------------------------------

#include "ctp/c3.h"
#include "ui/netshell/netshell.h"

#include <algorithm>
#include "ui/netshell/allinonewindow.h"
#include "ui/aui_common/aui_button.h"
#include "ui/aui_common/aui_screen.h"
#include "ui/netshell/connectionselectwindow.h"
#include "ui/aui_ctp2/ctp2_button.h"
#include "ui/netshell/gameselectwindow.h"
#include "gs/utility/Globals.h"
#include "ui/netshell/lobbywindow.h"
#include "ui/netshell/lobbychangewindow.h"
#include "ui/netshell/netshell_game.h"
#include "ui/netshell/ns_customlistbox.h"
#include "ui/netshell/ns_tribes.h"
#include "ui/netshell/passwordscreen.h"
#include "ui/netshell/playereditwindow.h"
#include "ui/netshell/playerselectwindow.h"
#include "ui/netshell/serverselectwindow.h"

extern void EnterMainMenu();
extern void LeaveMainMenu();
extern void LaunchGame();
extern MBCHAR g_serverName[ 100 + 1 ];

static NetShell *   g_netshell          = nullptr;

NetShell * netshell_Get() { return g_netshell; }
static NETFunc *      g_netfunc          = nullptr;
static nf_GameSetup   g_gamesetup;

NETFunc *      netfunc_Get()         { return g_netfunc; }
void           netfunc_Set(NETFunc *p)   { g_netfunc = p; }
nf_GameSetup & gamesetup_Get()       { return g_gamesetup; }
static nf_PlayerSetup g_playersetup;

nf_PlayerSetup & playersetup_Get() { return g_playersetup; }
static nf_PlayerSetup g_rplayersetup;

nf_PlayerSetup & rplayersetup_Get() { return g_rplayersetup; }


AUI_ERRCODE NetShell::Enter( uint32 flags )
{
	if ( (flags & k_NS_FLAGS_CREATE) || !g_netshell || !g_netfunc )
	{
		if ( !g_netfunc )
		{
			g_netfunc = new NETFunc();
		}

		if ( !g_netshell )
		{
			g_netshell = new NetShell();
		}
	}

	LeaveMainMenu();

	aui_ui_Get()->SetBackgroundColor( RGB(0,0,0) );

	if (aui_Control * bg = g_netshell->m_bg)
	{
		aui_Image * image    = aui_ui_Get()->LoadImage(bg->GetImage()->GetFilename());
		aui_Image *	oldImage = aui_ui_Get()->SetBackgroundImage
			(image,
			 (aui_ui_Get()->Width() - image->TheSurface()->Width()) / 2,
		     (aui_ui_Get()->Height() - image->TheSurface()->Height()) / 2
			);
		if (oldImage)
		{
			aui_ui_Get()->UnloadImage(oldImage);
		}
	}

	aui_ui_Get()->Invalidate();





	if (flags & k_NS_FLAGS_RETURN)
	{
		g_netfunc->Leave();
		LobbyWindow *w = (LobbyWindow *)(g_netshell->FindWindow( NetShell::WINDOW_LOBBY ));
		g_netshell->GotoScreen( NetShell::SCREEN_LOBBY );
		w->Update();
	}
	else if (flags & k_NS_FLAGS_CREATE3P)
	{
		if(g_netfunc->Connect("freeze.dat") == NETFunc::OK) {

			if(g_netfunc->IsHost()) {
				GameSelectWindow *sw = (GameSelectWindow *)(g_netshell->FindWindow(NetShell::WINDOW_GAMESELECT));
				g_gamesetup = *sw->GetGameSetup(g_netfunc->GetSession());
			}

			AllinoneWindow *w = (AllinoneWindow *)(g_netshell->FindWindow(NetShell::WINDOW_ALLINONE));
			g_netshell->GotoScreen( SCREEN_ALLINONE );
			w->Update();
		} else {
#ifdef WIN32
			PostMessage( aui_ui_Get()->TheHWND(), WM_CLOSE, 0, 0 );
#endif
		}
	}
	else
	{
		g_netshell->GotoScreen( SCREEN_CONNECTIONSELECT );
	}

	return AUI_ERRCODE_OK;
}

class EnterMainMenuAction : public aui_Action
{
public:
	virtual void	Execute
	(
		aui_Control	*	control,
		uint32			action,
		uint32			data
	)
    {
        EnterMainMenu();
    };
};

void NetShell::Leave( uint32 flags, BOOL safe )
{
	if ( g_netshell )
	{

		aui_ui_Get()->Draw();
		aui_ui_Get()->SetBackgroundColor( k_AUI_UI_NOCOLOR );
		aui_Image *prev = aui_ui_Get()->SetBackgroundImage( nullptr );
		aui_ui_Get()->UnloadImage(prev);


		g_netshell->LeaveCurrentScreen();
	}

	if ( flags & k_NS_FLAGS_MAINMENU ) {
		if(safe) {
			aui_ui_Get()->AddAction(new EnterMainMenuAction);
		} else {
			EnterMainMenu();
		}
	}

	if ( flags & k_NS_FLAGS_DESTROYNETFUNC )
		DestroyNETFunc();

	if ( flags & ( k_NS_FLAGS_DESTROYNETSHELL | k_NS_FLAGS_LAUNCH ) )
	{
		if ( safe )
		{
			aui_ui_Get()->AddAction(new DestroyAction);
		}
		else
		{
			DestroyAction().Execute( nullptr, 0, 0 );
		}
	}

	if ( flags & k_NS_FLAGS_LAUNCH )
	{
		LaunchGame();
	}
}


NetShell::NetShell()
:
    aui_Shell            (),
    m_wasMinimizing      (false),
    m_truebmp            (nullptr),
    m_tribes             (nullptr),
    m_wonders            (nullptr),
    m_bg                 (nullptr)
{
	std::fill(m_screens, m_screens + SCREEN_MAX, (aui_Screen *) nullptr);
	std::fill(m_windows, m_windows + WINDOW_MAX, (aui_Window *) nullptr);

	m_truebmp = new ns_String( "strings.truebmp" );
	m_tribes  = new ns_Tribes;

	AUI_ERRCODE	errcode = AUI_ERRCODE_OK;
	m_bg      = new aui_Control(&errcode, aui_UniqueId(), "nsbackground");
	Assert( AUI_NEWOK(m_bg, errcode) );

	errcode = CreateScreens();
	Assert( AUI_SUCCESS(errcode) );

	if (!g_netshell)
	{
		g_netshell = this;
		/// @todo Check next 4 lines
		nsunits_Set(new ns_Units);
		nsimprovements_Set(new ns_Improvements);
		nswonders_Set(new ns_Wonders);
		strncpy( g_serverName, "", 100 );
	}

}


AUI_ERRCODE NetShell::CreateScreens( )
{
	AUI_ERRCODE errcode = AUI_ERRCODE_OK;

	aui_Screen *screen;

	screen = new aui_Screen( &errcode, SCREEN_PLAYERSELECT );
	Assert( AUI_NEWOK(screen,errcode) );
	if ( !AUI_NEWOK(screen,errcode) ) return errcode;
	m_screens[ SCREEN_PLAYERSELECT ] = screen;

	{
		m_windows[ WINDOW_PLAYERSELECT ] = new PlayerSelectWindow( &errcode );
		m_screens[ SCREEN_PLAYERSELECT ]->
			AddWindow( m_windows[ WINDOW_PLAYERSELECT ] );
	}

	screen = new aui_Screen( &errcode, SCREEN_CONNECTIONSELECT );
	Assert( AUI_NEWOK(screen,errcode) );
	if ( !AUI_NEWOK(screen,errcode) ) return errcode;
	m_screens[ SCREEN_CONNECTIONSELECT ] = screen;

	{
		ConnectionSelectWindow * window = new ConnectionSelectWindow( &errcode );
		Assert( AUI_NEWOK(window,errcode) );
		if ( !AUI_NEWOK(window,errcode) ) return errcode;
		m_windows[ WINDOW_CONNECTIONSELECT ] = window;

		MoveButton(window, "connectionselectwindow", "cancelbutton", true);
		MoveButton(window, "connectionselectwindow", "okbutton", false);

		m_screens[ SCREEN_CONNECTIONSELECT ]->
			AddWindow( m_windows[ WINDOW_CONNECTIONSELECT ] );
	}

	screen = new aui_Screen( &errcode, SCREEN_SERVERSELECT );
	Assert( AUI_NEWOK(screen,errcode) );
	if ( !AUI_NEWOK(screen,errcode) ) return errcode;
	m_screens[ SCREEN_SERVERSELECT ] = screen;
	{
		m_windows[ WINDOW_SERVERSELECT ] = new ServerSelectWindow( &errcode );
		m_screens[ SCREEN_SERVERSELECT ]->
			AddWindow( m_windows[ WINDOW_SERVERSELECT ] );
	}

	screen = new aui_Screen( &errcode, SCREEN_PLAYEREDIT );
	Assert( AUI_NEWOK(screen,errcode) );
	if ( !AUI_NEWOK(screen,errcode) ) return errcode;
	m_screens[ SCREEN_PLAYEREDIT ] = screen;
	{
		m_windows[ WINDOW_PLAYEREDIT ] = new PlayerEditWindow( &errcode );
		m_screens[ SCREEN_PLAYEREDIT ]->
			AddWindow( m_windows[ WINDOW_PLAYEREDIT ] );
	}

	screen = new aui_Screen( &errcode, SCREEN_LOBBY );
	Assert( AUI_NEWOK(screen,errcode) );
	if ( !AUI_NEWOK(screen,errcode) ) return errcode;
	m_screens[ SCREEN_LOBBY ] = screen;
	{
		LobbyWindow * window = new LobbyWindow( &errcode );
		Assert( AUI_NEWOK(window,errcode) );
		if ( !AUI_NEWOK(window,errcode) ) return errcode;
		m_windows[ WINDOW_LOBBY ] = window;

		MoveButton(window, "lobbywindow", "backbutton", true);
		MoveButton(window, "lobbywindow", "closebutton", false);

		m_screens[ SCREEN_LOBBY ]->
			AddWindow( m_windows[ WINDOW_LOBBY ] );
	}

	screen = new aui_Screen( &errcode, SCREEN_LOBBYCHANGE );
	Assert( AUI_NEWOK(screen,errcode) );
	if ( !AUI_NEWOK(screen,errcode) ) return errcode;
	m_screens[ SCREEN_LOBBYCHANGE ] = screen;
	{
		m_windows[ WINDOW_LOBBYCHANGE ] = new LobbyChangeWindow( &errcode) ;
		m_screens[ SCREEN_LOBBYCHANGE ]->
			AddWindow( m_windows[ WINDOW_LOBBYCHANGE ] );
	}




	screen = new aui_Screen( &errcode, SCREEN_GAMESELECT );
	Assert( AUI_NEWOK(screen,errcode) );
	if ( !AUI_NEWOK(screen,errcode) ) return errcode;
	m_screens[ SCREEN_GAMESELECT ] = screen;
	{
		m_windows[ WINDOW_GAMESELECT ] = new GameSelectWindow( &errcode );
		m_screens[ SCREEN_GAMESELECT ]->
			AddWindow( m_windows[ WINDOW_GAMESELECT ] );
	}




	screen = new aui_Screen( &errcode, SCREEN_STARTSELECTING );
	Assert( AUI_NEWOK(screen,errcode) );
	if ( !AUI_NEWOK(screen,errcode) ) return errcode;
	m_screens[ SCREEN_STARTSELECTING ] = screen;
	{
		StartSelectingWindow * window = new StartSelectingWindow( &errcode );
		Assert( AUI_NEWOK(window,errcode) );
		if ( !AUI_NEWOK(window,errcode) ) return errcode;
		m_windows[ WINDOW_STARTSELECTING ] = window;

		MoveButton(window, "startselectingwindow", "cancelbutton", true);

		m_screens[ SCREEN_STARTSELECTING ]->
			AddWindow( m_windows[ WINDOW_STARTSELECTING ] );
	}

	screen = new aui_Screen( &errcode, SCREEN_ALLINONE );
	Assert( AUI_NEWOK(screen,errcode) );
	if ( !AUI_NEWOK(screen,errcode) ) return errcode;
	m_screens[ SCREEN_ALLINONE ] = screen;
	{
		m_windows[ WINDOW_ALLINONE ] = new AllinoneWindow( &errcode );
		m_screens[ SCREEN_ALLINONE ]->
			AddWindow( m_windows[ WINDOW_ALLINONE ] );
	}

	passwordscreen_Initialize();

	return AUI_ERRCODE_OK;
}


NetShell::~NetShell()
{
	DestroyScreens();

	delete m_truebmp;
	delete m_tribes;

	if ( m_bg )
	{
		aui_Image *	mpBackgroundImage = aui_ui_Get()->SetBackgroundImage(nullptr);
		aui_ui_Get()->UnloadImage(mpBackgroundImage);
		delete m_bg;
	}

    if (g_netshell == this)
    {
        delete nsunits_Get();        nsunits_Set(nullptr);
        delete nsimprovements_Get(); nsimprovements_Set(nullptr);
        delete nswonders_Get();      nswonders_Set(nullptr);

        g_netshell = nullptr;
    }
}

void NetShell::DestroyScreens( )
{
    SavePlayerSetupList();
    SaveGameSetupList();

    if (GetCurrentScreen())
    {
        GetCurrentScreen()->Hide();
    }

	int i;
	for ( i = 0; i < SCREEN_MAX; i++ )
	{
		delete m_screens[ i ];
		m_screens[i] = nullptr;
	}

	for ( i = 0; i < WINDOW_MAX; i++ )
	{
		delete m_windows[ i ];
		m_windows[i] = nullptr;
	}

	passwordscreen_Cleanup();
}

void NetShell::SavePlayerSetupList( )
{
	PlayerSelectWindow *pw = (PlayerSelectWindow *)g_netshell->FindWindow( NetShell::WINDOW_PLAYERSELECT );
	ns_PlayerSetupListBox *pl = (ns_PlayerSetupListBox *)(pw->FindControl( PlayerSelectWindow::CONTROL_PLAYERNAMELISTBOX ));
	pl->Save();
}

void NetShell::SaveGameSetupList( )
{
	GameSelectWindow *gw = (GameSelectWindow *)g_netshell->FindWindow( NetShell::WINDOW_GAMESELECT );
	ns_GameSetupListBox *gl = (ns_GameSetupListBox *)(gw->FindControl( GameSelectWindow::CONTROL_GAMENAMELISTBOX ));
	gl->Save();
}

void NetShell::SaveAiSetupList( )
{
}

void NetShell::DestroyNETFunc( )
{
	allocated::clear(g_netfunc);
}

aui_Screen *NetShell::FindScreen( uint32 id )
{
	Assert(id < (uint32)SCREEN_MAX);
	return (id < (uint32)SCREEN_MAX) ? m_screens[id] : nullptr;
}

aui_Window *NetShell::FindWindow( uint32 id )
{
	Assert(id < (uint32)WINDOW_MAX);
	return (id < (uint32)WINDOW_MAX) ? m_windows[id] : nullptr;
}

void NetShell::DestroyAction::Execute(
	aui_Control *control,
	uint32 action,
	uint32 data )
{
	allocated::clear(g_netshell);
}

void NetShell::MoveButton(aui_Window *window, const MBCHAR *parentBlock, const MBCHAR *regionBlock, BOOL left)
{
	ctp2_Button *button = (ctp2_Button *)aui_Ldl::GetObject(parentBlock, regionBlock);
	if (button)
	{
		sint32 xPos = (left) ? 17 : window->Width() - button->Width() - 14;
		button->Move(xPos, window->Height() - button->Height() - 17);
	}
}
