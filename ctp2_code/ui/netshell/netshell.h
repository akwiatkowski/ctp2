#ifdef HAVE_PRAGMA_ONCE
#pragma once
#endif

#ifndef __NETSHELL_H__
#define __NETSHELL_H__

class NetShell;

#include "ui/aui_common/aui_shell.h"
#include "ui/aui_common/aui_action.h"
#include "ui/aui_common/aui_ui.h"
#include "ui/netshell/netfunc.h"
#include "ui/netshell/ns_gamesetup.h"
#include "ui/netshell/ns_playersetup.h"
#include "ui/netshell/ns_string.h"

class aui_Screen;
class aui_Window;
class NetShell;
class ns_Tribes;

#define k_NS_FLAGS_CREATE			0x00000001
#define k_NS_FLAGS_DESTROYNETSHELL	0x00000002
#define k_NS_FLAGS_DESTROYNETFUNC	0x00000004
#define k_NS_FLAGS_DESTROY			(k_NS_FLAGS_DESTROYNETSHELL|k_NS_FLAGS_DESTROYNETFUNC)
#define k_NS_FLAGS_LAUNCH			0x00000008
#define k_NS_FLAGS_CREATE3P			0x00000010
#define k_NS_FLAGS_RETURN			0x00000020
#define k_NS_FLAGS_MAINMENU			0x00000040

// Lifecycle is internal to netshell.cpp (ctor sets, dtor clears).  All
// external consumers across UI / net / lobby code read it through
// netshell_Get().  No external writer.
NetShell * netshell_Get(void);
// App-singleton accessor pair for g_netfunc.  g_netfunc is file-static
// in netshell.cpp; external consumers go through netfunc_Get / netfunc_Set.
NETFunc * netfunc_Get(void);
void      netfunc_Set(NETFunc *p);

// Reference accessor for g_gamesetup (value-type struct).  The struct is
// file-static in netshell.cpp.
nf_GameSetup & gamesetup_Get(void);
// Local player setup buffer (sibling to g_rplayersetup which is the
// remote one).  Storage is file-scope `static` in netshell.cpp; callers
// get a writable reference via playersetup_Get() and use it for method
// calls / assignment / address-of.
nf_PlayerSetup & playersetup_Get(void);
// Remote player setup buffer.  Definition is file-scope `static` in
// netshell.cpp; callers (allinonewindow, lobbywindow) get a writable
// reference via rplayersetup_Get() and use it for assignment / method
// calls / address-of as before.
nf_PlayerSetup & rplayersetup_Get(void);


#define k_PACKET_DELAY 2000

#define k_NETCHUNKSIZE (dpio_MAXLEN_UNRELIABLE - 8)

enum CustomCode
{

	CUSTOMCODE_REQUESTTRIBE = dppt_MAKE(ns_PACKET_INITIALBYTE, 0),


	CUSTOMCODE_REQUESTDENIED = dppt_MAKE(ns_PACKET_INITIALBYTE, 1),


	CUSTOMCODE_STARTDOWNLOADMAP = dppt_MAKE(ns_PACKET_INITIALBYTE, 2),

	CUSTOMCODE_CONTINUEDOWNLOADMAP = dppt_MAKE(ns_PACKET_INITIALBYTE, 3)
};


class NetShell : public aui_Shell
{
public:
	NetShell();
	virtual ~NetShell();

	enum SCREEN
	{
		SCREEN_FIRST = 0,
		SCREEN_CONNECTIONSELECT = SCREEN_FIRST,
		SCREEN_SERVERSELECT,
		SCREEN_PLAYERSELECT,
		SCREEN_PLAYEREDIT,
		SCREEN_LOBBY,
		SCREEN_LOBBYCHANGE,
		SCREEN_STARTSELECTING,
		SCREEN_GAMESELECT,
		SCREEN_ALLINONE,
		SCREEN_LAST,
		SCREEN_MAX = SCREEN_LAST - SCREEN_FIRST
	};

	enum WINDOW
	{
		WINDOW_FIRST = 0,
		WINDOW_CONNECTIONSELECT = WINDOW_FIRST,
		WINDOW_SERVERSELECT,
		WINDOW_PLAYERSELECT,
		WINDOW_PLAYEREDIT,
		WINDOW_LOBBY,
		WINDOW_LOBBYCHANGE,
		WINDOW_STARTSELECTING,
		WINDOW_GAMESELECT,
		WINDOW_ALLINONE,
		WINDOW_LAST,
		WINDOW_MAX = WINDOW_LAST - WINDOW_FIRST
	};

	virtual aui_Screen *FindScreen( uint32 id );
	aui_Window *FindWindow( uint32 id );

	static AUI_ERRCODE	Enter( uint32 flags );
	static void			Leave( uint32 flags, BOOL safe = FALSE );

	static void SavePlayerSetupList( void );
	static void SaveGameSetupList( void );
	static void SaveAiSetupList( void );

	BOOL &WasMinimizing( void ) { return m_wasMinimizing; }

	AUI_ACTION_BASIC(DestroyAction);

	MBCHAR *GetTrueBmp( void ) { return m_truebmp ? m_truebmp->GetString() : NULL; }

protected:
	static void	DestroyNETFunc( void );

	AUI_ERRCODE	CreateScreens( void );
	void		DestroyScreens( void );
	void MoveButton(aui_Window *window, const MBCHAR *parentBlock, const MBCHAR *regionBlock, BOOL left);

private:
	aui_Screen *m_screens[ SCREEN_MAX ];
	aui_Window *m_windows[ WINDOW_MAX ];

	BOOL m_wasMinimizing;

	ns_String *m_truebmp;

	ns_Tribes *m_tribes;
	ns_Wonders *m_wonders;

	aui_Control *m_bg;
};

#endif
