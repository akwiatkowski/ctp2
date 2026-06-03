//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ source
// Description  : Event initialization and cleanup
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
// - Stripped UI window event registrations (DiplomacyWindow,
//   ArmyManagerWindow, TradeManager, UnitManager, DipWizard,
//   selecteditemevent, interfaceevent).  Those live in
//   ui/aui_ctp2/ui_events.cpp now; the UI-enabled build calls
//   ui_events_Initialize() / ui_events_Cleanup() from civapp.
//
//----------------------------------------------------------------------------

#include "ctp/c3.h"
#include "gs/gameobj/Events.h"
#include "gs/events/GameEventUser.h"
#include "gs/events/GameEventManager.h"

#include "gs/gameobj/Player.h"
#include "gs/gameobj/Score.h"

#include "gs/slic/SlicObject.h"
#include "gs/slic/SlicEngine.h"

#include "gs/gameobj/ArmyEvent.h"
#include "gs/gameobj/CityEvent.h"
#include "gs/gameobj/PlayerEvent.h"
#include "gs/gameobj/UnitEvent.h"
#include "gs/utility/TurnCntEvent.h"
#include "sound/soundevent.h"
#include "gs/gameobj/improvementevent.h"
#include "gs/world/worldevent.h"
#include "gs/gameobj/combatevent.h"
#include "net/general/networkevent.h"
#include "gs/gameobj/tradeevent.h"

#include "gs/core/render_observer.h"   // render_observer::OnEventsInitialize/Cleanup

#include "gs/gameobj/Order.h"

#include "ai/ctpai.h"

#include "gs/gameobj/EventTracker.h"
#include "gs/gameobj/FeatTracker.h"

#include "gs/gameobj/gaiacontroller.h"

STDEHANDLER(ScoreEventTest)
{
	Assert(gameEventType == GEV_CalcScores);
	GameEventArgument *arg = args->GetArg(GEA_Player, 0);
	sint32 player;
	if(arg->GetPlayer(player)) {
		player_Get(player)->m_score->AddYearAtPeace();
	}
	return GEV_HD_Continue;
}

void events_Initialize()
{
	armyevent_Initialize();
	cityevent_Initialize();
	playerevent_Initialize();
	unitevent_Initialize();
	turncountevent_Initialize();
	soundevent_Initialize();
	improvementevent_Initialize();
	render_observer::OnEventsInitialize();
	worldevent_Initialize();
	combatevent_Initialize();
	tradeevent_Initialize();

	trackerevent_Initialize();

	gevmanager_Get()->AddCallback(GEV_CalcScores, GEV_PRI_Primary, &s_ScoreEventTest);

	Order::AssociateEventsWithOrders();

	networkevent_Initialize();

	FeatTracker::InitializeEvents();

	CtpAi::InitializeEvents();

	GaiaController::InitializeEvents();
}

void events_Cleanup()
{
	armyevent_Cleanup();
	cityevent_Cleanup();
	playerevent_Cleanup();
	unitevent_Cleanup();
	turncountevent_Cleanup();
	soundevent_Cleanup();
	improvementevent_Cleanup();
	render_observer::OnEventsCleanup();
	worldevent_Cleanup();
	combatevent_Cleanup();
	tradeevent_Cleanup();

	networkevent_Cleanup();

	FeatTracker::CleanupEvents();

	CtpAi::CleanupEvents();
}
