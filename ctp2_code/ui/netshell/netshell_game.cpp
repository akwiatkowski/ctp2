#include "ctp/c3.h"
#include <memory>

#include "ui/aui_common/aui_ui.h"

#include "ui/netshell/netshell.h"

#include "ui/netshell/allinonewindow.h"
#include "ui/interface/loadsavewindow.h"
#include "ui/interface/scenariowindow.h"

#include "ui/interface/initialplaywindow.h"

#include "net/general/network.h"
#include "ctp/civapp.h"

#include "ui/netshell/netshell_game.h"
#include "gs/fileio/civscenarios.h"



void EnterMainMenu( )
{
	initialplayscreen_Initialize();
	initialplayscreen_displayMyWindow();
}


void LeaveMainMenu( )
{

	aui_ui_Get()->AddAction( std::make_unique<DestroyInitialPlayScreenAction>().release() );
}


void LaunchGame( )
{
	AllinoneWindow *w = allinonewindow_Get();


	switch ( w->GetMode() )
	{
	case AllinoneWindow::CONTINUE_CREATE:
	case AllinoneWindow::CONTINUE_JOIN:

		{
			network_Get().SetLaunchFromNetFunc(TRUE);
			loadsavescreen_LoadMPGame();
		}






		break;

	case AllinoneWindow::CREATE:

		network_Get().SetLaunchFromNetFunc(FALSE);

		if(w->IsScenarioGame()) {
			if(w->GetScenarioInfo()->m_haveSavedGame) {
				ScenarioPack *pack;
				Scenario *scen;
				if(civscenarios_Get()->FindScenario(scenario_name_buf(),
												&pack, &scen)) {
					MBCHAR path[_MAX_PATH];
					snprintf(path, sizeof(path), "%s\\%s",
							scen->m_path,
							k_SCENARIO_DEFAULT_SAVED_GAME_NAME);
					civapp_Get()->PostLoadSaveGameAction(path);
					break;
				}
			}
		}
		civapp_Get()->PostStartGameAction();
		break;

	case AllinoneWindow::JOIN:
		network_Get().SetLaunchFromNetFunc(FALSE);
		civapp_Get()->PostStartGameAction();
		break;

	default:

		Assert( FALSE );
		break;
	}
}




void DestroyInitialPlayScreenAction::Execute(
	aui_Control *control,
	uint32 action,
	uint32 data )
{
	initialplayscreen_Cleanup();
}
