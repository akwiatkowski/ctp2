#ifndef __GAMESELECTWINDOW_H__
#define __GAMESELECTWINDOW_H__

#include "ui/netshell/ns_window.h"
#include "ui/aui_common/aui_action.h"
#include "ui/netshell/ns_customlistbox.h"

class GameSelectWindow;
// g_gameSelectWindow demoted to file-scope `static` in gameselectwindow.cpp.
// Use gameselectwindow_Get() instead.  Returns NULL when the window
// has not been opened (or has already been torn down).
GameSelectWindow * gameselectwindow_Get();

class GameSelectWindow : public ns_Window
{
public:

	GameSelectWindow( AUI_ERRCODE *retval );
	~GameSelectWindow() override;

protected:
	GameSelectWindow() : ns_Window() {}
	AUI_ERRCODE	InitCommon( ) override;
	AUI_ERRCODE CreateControls( );

public:
	void	Update( );
	AUI_ERRCODE Idle( ) override;
	AUI_ERRCODE SetParent( aui_Region *region ) override;
	nf_GameSetup *GetGameSetup(NETFunc::Session *session);

	void PasswordScreenDone( MBCHAR *password );


	enum CONTROL
	{
		CONTROL_FIRST = 0,

		CONTROL_TITLESTATICTEXT = CONTROL_FIRST,

		CONTROL_GAMENAMELISTBOX,
		CONTROL_DELETEBUTTON,
		CONTROL_OKBUTTON,
		CONTROL_CANCELBUTTON,
		CONTROL_LAST,
		CONTROL_MAX = CONTROL_LAST - CONTROL_FIRST
	};

protected:
    AUI_ACTION_BASIC(DeleteButtonAction);
	AUI_ACTION_BASIC(OKButtonAction);
	AUI_ACTION_BASIC(CancelButtonAction);
	AUI_ACTION_BASIC(GameListBoxAction);
};

class StartSelectingWindow;

class StartSelectingWindow : public ns_Window
{
public:

	StartSelectingWindow( AUI_ERRCODE *retval );
	~StartSelectingWindow() override;

protected:
	StartSelectingWindow() : ns_Window() {}
	AUI_ERRCODE	InitCommon( ) override;
	AUI_ERRCODE CreateControls( );

public:
	AUI_ERRCODE Idle( ) override;
	AUI_ERRCODE SetParent( aui_Region *region ) override;


	enum CONTROL
	{
		CONTROL_FIRST = 0,

		CONTROL_TITLESTATICTEXT = CONTROL_FIRST,
		CONTROL_NEWBUTTON,
		CONTROL_GAMESETUPBUTTON,
		CONTROL_SAVEDBUTTON,
		CONTROL_SCENARIOBUTTON,
		CONTROL_DELETEBUTTON,
		CONTROL_CANCELBUTTON,
		CONTROL_LAST,
		CONTROL_MAX = CONTROL_LAST - CONTROL_FIRST
	};

protected:
    AUI_ACTION_BASIC(NewButtonAction);
	AUI_ACTION_BASIC(GameSetupButtonAction);
    AUI_ACTION_BASIC(SavedButtonAction);
    AUI_ACTION_BASIC(ScenarioButtonAction);
	AUI_ACTION_BASIC(CancelButtonAction);
};

void StartSelectingLoadSaveCallback(
	aui_Control *control,
	uint32 action,
	uint32 data,
	void* cookie );

void StartSelectingScenarioCallback(
	aui_Control *control,
	uint32 action,
	uint32 data,
	void* cookie );

#endif
