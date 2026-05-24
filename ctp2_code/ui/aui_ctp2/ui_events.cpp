//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ source
// Description  : Registration of UI-side game event hooks
//
//----------------------------------------------------------------------------

#include "ctp/c3.h"
#include "ui/aui_ctp2/ui_events.h"

#include "ui/aui_ctp2/SelItemEvent.h"
#include "ui/interface/interfaceevent.h"
#include "ui/interface/diplomacywindow.h"
#include "ui/interface/armymanagerwindow.h"
#include "ui/interface/trademanager.h"
#include "ui/interface/unitmanager.h"
#include "ui/interface/dipwizard.h"

void ui_events_Initialize()
{
	selecteditemevent_Initialize();
	interfaceevent_Initialize();

	DiplomacyWindow::InitializeEvents();
	ArmyManagerWindow::InitializeEvents();
	TradeManager::InitializeEvents();
	UnitManager::InitializeEvents();
	DipWizard::InitializeEvents();
}

void ui_events_Cleanup()
{
	selecteditemevent_Cleanup();
	interfaceevent_Cleanup();

	DiplomacyWindow::CleanupEvents();
	ArmyManagerWindow::CleanupEvents();
	TradeManager::CleanupEvents();
	UnitManager::CleanupEvents();
}
