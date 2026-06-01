//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ source
// Description  : Player event handling
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
// _BFR_
// - Force CD checking when set (build final release).
//
//----------------------------------------------------------------------------
//
// Modifications from the original Activision code:
//
// - Moved the autosave file generation to just before the StartMovePhase
//   event, to prevent losing the advance that just was completed.
// - Corrected GrantAdvanceEvent input handling.
// - Corrected memory leaks and invalid arguments for Gaia Controller messages.
// - Corrected recipients for Gaia Controller messages.
// - Propagate PW each turn update
// - Fixed PBEM BeginTurn event execution. (27-Oct-2007 Martin G�hmann)
// - An autosave is now created even if the visible player is a robot. (26-Jab-2008 Martin G�hmann)
// - Separated the Settle event from the Settle in City event. (19-Feb-2008 Martin G�hmann)
// - Added stuff for unit and city gold support. (22-Jul-2009 Maq)
//
//----------------------------------------------------------------------------

#include "ctp/c3.h"
#include "gs/utility/safety.h"
#include "gs/gameobj/PlayerEvent.h"
#include "gs/gameobj/Events.h"
#include "gs/gameobj/Player.h"
#include "gs/events/GameEventUser.h"
#include "gs/gameobj/Wormhole.h"
#include "gs/gameobj/PlayHap.h"
#include "gs/utility/UnitDynArr.h"
#include "gs/gameobj/Gold.h"
#include "gs/gameobj/Readiness.h"
#include "gs/gameobj/UnitPool.h"
#include "gs/world/World.h"
#include "gs/slic/SlicEngine.h"
#include "gs/slic/SlicObject.h"
#include "gs/slic/SlicSegment.h"
#include "gs/gameobj/UnitData.h"
#include "gs/gameobj/Army.h"
#include "gs/gameobj/EndGame.h"
#include "gs/utility/TurnCnt.h"
#include "gs/utility/newturncount.h"
#include "gs/gameobj/Score.h"

#include "gs/database/profileDB.h"
#include "ctp/civapp.h"

#include "net/general/network.h"
#include "net/general/net_info.h"
#include "gs/gameobj/unitutil.h"
#include "gs/gameobj/TerrImprove.h"
#include "gs/gameobj/ArmyData.h"
#include "ai/ctpai.h"
#include "gs/outcom/AICause.h"
#include "gs/gameobj/GSLogs.h"
#include "gs/gameobj/TradeRouteData.h"
#include "ai/diplomacy/Diplomat.h"
#include "net/general/net_action.h"
#include "gs/gameobj/gaiacontroller.h"

// Clean architecture: game event observer registry
#include "gs/core/game_observer.h"
#include "gs/core/player_view.h"

// Propagate PW each turn update
#include "gs/gameobj/MaterialPool.h"

extern CivApp *g_civApp;

extern sint32 g_noai_stop_player;

STDEHANDLER(ContactMadeEvent)
{
	GameEventArgument *firstPlayerArg  = args->GetArg(GEA_Player, 0);
	GameEventArgument *secondPlayerArg = args->GetArg(GEA_Player, 1);

	sint32 p1, p2;

	if(!firstPlayerArg || !(firstPlayerArg->GetPlayer(p1)))
		return GEV_HD_Continue;

	if(!secondPlayerArg || !(secondPlayerArg->GetPlayer(p2)))
		return GEV_HD_Continue;

	safe_player(p1)->ContactMade(p2);
	return GEV_HD_Continue;
}

STDEHANDLER(WormholeEvent)
{
	sint32 player;
	if(!args->GetPlayer(0, player))
		return GEV_HD_Continue;

	if(Wormhole *wh = wormhole_Get())
		wh->BeginTurn(player);
	return GEV_HD_Continue;
}

STDEHANDLER(PatienceEvent)
{
	sint32 player;
	if(!args->GetPlayer(0, player))
		return GEV_HD_Continue;

	return GEV_HD_Continue;
}

STDEHANDLER(PeaceMovementEvent)
{
	sint32 player;
	if(!args->GetPlayer(0, player))
		return GEV_HD_Continue;

	Player *p = safe_player(player);

	safe_player(player)->m_global_happiness->CalcPeaceMovement(g_player[player],
	                                                        *safe_player(player)->m_all_armies,
	                                                        *safe_player(player)->m_all_cities);


	if(p->m_assasinationTimer > 0) {
		p->m_assasinationTimer--;
		if(p->m_assasinationTimer <= 0)
			p->m_assasinationModifier = 0;
	}

	return GEV_HD_Continue;
}

STDEHANDLER(PollutionTurnEvent)
{
	sint32 player;
	if(!args->GetPlayer(0, player))
		return GEV_HD_Continue;

	safe_player(player)->BeginTurnPollution();
	return GEV_HD_Continue;
}

STDEHANDLER(BeginTurnAllCitiesEvent)
{
	sint32 player;

	if(!args->GetPlayer(0, player))
		return GEV_HD_Continue;

	Player *p = safe_player(player);

	p->m_pop_science = 0;

	p->m_gold->SetSavings();

	p->m_readiness->BeginTurn(p->m_government_type);

	if(p->m_capitol && unitpool_Get()->IsValid(p->m_capitol->m_id)) {
		MapPoint pos;
		p->m_capitol->GetPos(pos);
		g_theWorld->FindCityDistances(p->m_owner, pos);
	}

	safe_player(player)->m_virtualGoldSpent = 0;

	if(safe_player(player)->GetGaiaController()->CanStartCountdown()) {
		SlicSegment *       seg  = g_slicEngine->GetSegment("GCReadyToActivateUs");
		if (seg && !seg->TestLastShown(player, 10000, g_turn->GetSessionRound()))
		{
			SlicObject *    so   = new SlicObject("GCReadyToActivateUs");
			so->AddPlayer(player);
			so->AddRecipient(player);
			g_slicEngine->Execute(so);

			so = new SlicObject("GCReadyToActivateThem");
			so->AddPlayer(player);
			so->AddAllRecipientsBut(player);
			g_slicEngine->Execute(so);
		}
	}

	return GEV_HD_Continue;
}

STDEHANDLER(BeginTurnProductionEvent)
{
	sint32 player;
	if(!args->GetPlayer(0, player))
		return GEV_HD_Continue;

	safe_player(player)->BeginTurnProduction();
	return GEV_HD_Continue;
}

STDEHANDLER(BeginTurnSupportEvent)
{
	sint32 player;
	if(!args->GetPlayer(0, player))
		return GEV_HD_Continue;

	Player *p = safe_player(player);

	p->BeginTurnWonders();
	p->BeginTurnCommodityMarket();
	p->BeginTurnUnitSupportGold();
	p->BeginTurnScience();

	return GEV_HD_Continue;
}

STDEHANDLER(BeginTurnImprovementsEvent)
{
	sint32 player;
	if(!args->GetPlayer(0, player))
		return GEV_HD_Continue;

	safe_player(player)->BeginTurnImprovements();
	return GEV_HD_Continue;
}

STDEHANDLER(BeginTurnAgreementsEvent)
{
	sint32 player;
	if(!args->GetPlayer(0, player))
		return GEV_HD_Continue;

	safe_player(player)->BeginTurnAgreements();
	return GEV_HD_Continue;
}

STDEHANDLER(ResetAllMovementEvent)
{
	sint32 player;
	if(!args->GetPlayer(0, player))
		return GEV_HD_Continue;

	safe_player(player)->ResetAllMovement();

	safe_player(player)->m_oversea_lost_unit_count = 0;
	safe_player(player)->m_home_lost_unit_count = 0;

	return GEV_HD_Continue;
}

STDEHANDLER(AttemptRevoltEvent)
{
	sint32 player;
	if(!args->GetPlayer(0, player))
		return GEV_HD_Continue;

	safe_player(player)->AttemptRevolt();
	return GEV_HD_Continue;
}

STDEHANDLER(BeginTurnEndGameEvent)
{
	return GEV_HD_Continue;
}

STDEHANDLER(BeginTurnGovernmentEvent)
{
	sint32 player;
	if(!args->GetPlayer(0, player))
		return GEV_HD_Continue;

	Player *p = safe_player(player);
	Assert(p != NULL);
	if((p != NULL) && p->m_change_government_turn == p->GetCurRound()) {
		p->ActuallySetGovernment(p->m_set_government_type);
		p->m_changed_government_this_turn = TRUE;
	} else {
		p->m_changed_government_this_turn = FALSE;
	}

	g_slicEngine->RunPlayerTriggers(player);

	return GEV_HD_Continue;
}

STDEHANDLER(FinishBeginTurnEvent)
{
	sint32 player;
	if(!args->GetPlayer(0, player))
		return GEV_HD_Continue;

	Player *p = safe_player(player);

	bool atPeace = true;

	for(sint32 i = 1; i < k_MAX_PLAYERS; i++)
	{
		if(p->m_contactedPlayers & (1 << i) && g_player[i])
		{
			if(p->m_diplomatic_state[i] == DIPLOMATIC_STATE_WAR)
			{
				atPeace = false;
				break;
			}
		}
	}

	if(atPeace)
	{
		p->m_score->AddYearAtPeace();
	}

	if (!p->m_isDead)
	{
		// Something is missing here
	}

	DPRINTF(k_DBG_GAMESTATE, ("It's player %d's turn - year %d.\n", p->m_owner, p->GetCurRound()));
	DPRINTF(k_DBG_GAMESTATE, ("Gold: %d\n", p->m_gold->GetLevel()));
	DPRINTF(k_DBG_GAMESTATE, ("Public Works: %d\n", p->m_materialPool->GetMaterials()));

	g_gameObservers->NotifyUpdateScienceWindow(p->m_owner);

	if(g_network.IsHost())
	{
		g_network.Block(p->m_owner);
		g_network.QueuePacketToAll(new NetInfo(NET_INFO_CODE_GOLD,
		                                       p->m_owner, p->m_gold->GetLevel()));
		// propagate PW each turn update
		g_network.QueuePacketToAll(new NetInfo(NET_INFO_CODE_MATERIALS,
		                                       p->m_owner, p->m_materialPool->GetMaterials()));
		g_network.Unblock(p->m_owner);
	}

	// Auto-select first unit — only meaningful when UI is present.
	if ((p->IsHuman() ||
	     (p->IsNetwork() && g_network.IsLocalPlayer(p->m_owner))) &&
	    g_gameObservers)
	{
		g_gameObservers->NotifyAutoSelectFirstUnit(p->m_owner);
	}

	if(g_network.IsHost())
	{
		g_network.SyncRand();
		g_network.Enqueue(new NetInfo(NET_INFO_CODE_TURN_SYNC));
	}

	if(!g_network.IsClient())
	{
		gevmanager_Get()->AddEvent(GEV_INSERT_Tail, GEV_FinishBuildPhase,
		                       GEA_Player, player,
		                       GEA_End);
	}

	return GEV_HD_Continue;
}

STDEHANDLER(CreateUnitEvent)
{
	MapPoint    pos;
	sint32      utype;
	sint32      cause;
	Unit        homeCity;
	sint32      pl;

	if(!args->GetPos(0, pos)) return GEV_HD_Continue;
	args->GetCity(0, homeCity);
	if(!args->GetInt(0, utype)) return GEV_HD_Continue;
	if(!args->GetInt(1, cause)) return GEV_HD_Continue;
	if(!args->GetPlayer(0, pl)) return GEV_HD_Continue;

	Assert(g_player[pl]);
	if(!g_player[pl])
		return GEV_HD_Continue;

	Unit u = g_player[pl]->CreateUnit(utype, pos, homeCity, false, (CAUSE_NEW_ARMY)cause);
	if(u.m_id == 0) {
		return GEV_HD_Stop;
	}

	args->Add(new GameEventArgument(GEA_Unit, u));
	return GEV_HD_Continue;
}

STDEHANDLER(SettleEvent)
{
	Army a;
	if(!args->GetArmy(0, a)) return GEV_HD_Continue;

	if(g_player[a.GetOwner()])
	{
		if(g_player[a.GetOwner()]->Settle(a))
		{
			args->Add(new GameEventArgument(GEA_Int, 1));
		}
	}

	return GEV_HD_Continue;
}

STDEHANDLER(SettleInCityEvent)
{
	Army a;
	if(!args->GetArmy(0, a)) return GEV_HD_Continue;

	if(g_player[a.GetOwner()])
	{
		if(g_player[a.GetOwner()]->SettleInCity(a))
		{
			args->Add(new GameEventArgument(GEA_Int, 1));
		}
	}

	return GEV_HD_Continue;
}

STDEHANDLER(CreateCityEvent)
{
	MapPoint pos;
	sint32 player;
	sint32 cause;
	sint32 unitType;

	if(!args->GetPlayer(0, player)) return GEV_HD_Continue;
	if(!args->GetPos(0, pos)) return GEV_HD_Continue;
	if(!args->GetInt(0, cause)) return GEV_HD_Continue;
	if(!args->GetInt(1, unitType)) return GEV_HD_Continue;

	if(g_player[player])
	{
		sint32 cityType = unitutil_GetCityTypeFor(pos);
		Unit city = safe_player(player)->CreateCity(cityType, pos, (CAUSE_NEW_CITY)cause, NULL, unitType);
		if(city.IsValid())
		{
			args->Add(new GameEventArgument(GEA_City, city));

			if(cause == CAUSE_NEW_CITY_GOODY_HUT)
			{
				SlicObject *so = new SlicObject("80RuinBecomesCity");
				so->AddRecipient(player);
				so->AddCity(city);
				g_slicEngine->Execute(so);
				DPRINTF(k_DBG_GAMESTATE, ("You get a city!\n"));

				g_gameObservers->NotifyCityFounded(player, city, pos, cause);
			}
		}
		else if(cause == CAUSE_NEW_CITY_GOODY_HUT)
		{
			SlicObject *so = new SlicObject("93BesetByNothing");
			so->AddRecipient(player);
			g_slicEngine->Execute(so);
		}

	}
	return GEV_HD_Continue;
}

STDEHANDLER(CreateImprovementEvent)
{
	MapPoint pos;
	sint32 player;
	sint32 imptype;

	if(!args->GetPlayer(0, player)) return GEV_HD_Continue;
	if(!args->GetPos(0, pos)) return GEV_HD_Continue;
	if(!args->GetInt(0, imptype)) return GEV_HD_Continue;

	safe_player(player)->CreateImprovement(imptype, pos, 0);

	if(g_player[player] && safe_player(player)->GetGaiaController()->HasMinTowersBuilt()) {
		SlicSegment *	seg = g_slicEngine->GetSegment("GCMinObelisksReachedUs");
		if (seg && !seg->TestLastShown(player, 10000, g_turn->GetSessionRound()))
		{
			SlicObject *	so = new SlicObject("GCMinObelisksReachedUs");
			so->AddRecipient(player);
			g_slicEngine->Execute(so);

			so = new SlicObject("GCMinObelisksReachedThem");
			so->AddPlayer(player);
			so->AddAllRecipientsBut(player);
			g_slicEngine->Execute(so);
		}
	}

	return GEV_HD_Continue;
}

STDEHANDLER(GrantAdvanceEvent)
{
	sint32 advance;
	sint32 player;
	sint32 cause;

	if(!args->GetPlayer(0, player))  return GEV_HD_Continue;
	if(!args->GetInt   (0, advance)) return GEV_HD_Continue;
	if(!args->GetInt   (1, cause))   return GEV_HD_Continue;

	safe_player(player)->m_advances->GiveAdvance(advance, (CAUSE_SCI)cause, false);
	return GEV_HD_Continue;
}

STDEHANDLER(SendGoodEvent)
{
	sint32 resIndex;
	Unit sourceCity, destCity;

	if(!args->GetInt(0, resIndex)) return GEV_HD_Continue;
	if(!args->GetCity(0, sourceCity)) return GEV_HD_Continue;
	if(!args->GetCity(1, destCity)) return GEV_HD_Continue;

	if(g_network.IsClient()) {
		g_network.SendAction(new NetAction(NET_ACTION_REQUEST_TRADE_ROUTE,
		                                   resIndex, sourceCity.m_id, destCity.m_id));
	} else {
		g_player[sourceCity.GetOwner()]->CreateTradeRoute(sourceCity, ROUTE_TYPE_RESOURCE,
		                                                  resIndex, destCity,
		                                                  sourceCity.GetOwner(), 0);
	}
	return GEV_HD_Continue;
}

STDEHANDLER(TradeBidEvent)
{
	sint32 player;
	sint32 resIndex;
	Unit sourceCity, destCity;

	if(!args->GetPlayer(0, player)) return GEV_HD_Continue;
	if(!args->GetInt(0, resIndex)) return GEV_HD_Continue;
	if(!args->GetCity(0, sourceCity)) return GEV_HD_Continue;
	if(!args->GetCity(1, destCity)) return GEV_HD_Continue;

	safe_player(player)->CreateTradeBid(sourceCity, resIndex, destCity);
	return GEV_HD_Continue;
}

STDEHANDLER(CreatedArmyEvent)
{
	// Post army creation event doesn't do anything if you call it from slic.

	return GEV_HD_Continue;
}

STDEHANDLER(SubGoldEvent)
{
	sint32 player, amt;
	if(!args->GetPlayer(0, player)) return GEV_HD_Continue;
	if(!args->GetInt(0, amt)) return GEV_HD_Continue;

	safe_player(player)->SubGold(amt);
	return GEV_HD_Continue;
}

STDEHANDLER(AddGoldEvent)
{
	sint32 player;
	sint32 amt;
	if(!args->GetPlayer(0, player)) return GEV_HD_Continue;
	if(!args->GetInt(0, amt)) return GEV_HD_Continue;

	safe_player(player)->AddGold(amt);
	return GEV_HD_Continue;
}

STDEHANDLER(EstablishEmbassyEvent)
{
	sint32 owner, otherguy;
	if(!args->GetPlayer(0, owner)) return GEV_HD_Continue;
	if(!args->GetPlayer(1, otherguy)) return GEV_HD_Continue;

	g_player[owner]->EstablishEmbassy(otherguy);
	return GEV_HD_Continue;
}

STDEHANDLER(ThrowPartyEvent)
{
	sint32 owner, otherguy;
	if(!args->GetPlayer(0, owner)) return GEV_HD_Continue;
	if(!args->GetPlayer(1, otherguy)) return GEV_HD_Continue;

	Diplomat::GetDiplomat(otherguy).ThrowParty(owner);
	return GEV_HD_Continue;
}

STDEHANDLER(FinishBuildPhaseEvent)
{
	sint32 player;
	if(!args->GetPlayer(0, player)) return GEV_HD_Continue;

	if((g_player[player] && !Player::IsThisPlayerARobot(player))
	||  player_view::VisiblePlayer() == player
	){
		if (g_theProfileDB->IsAutoSave() &&
			(!g_network.IsActive() || g_network.IsHost())
		   )
		{
			g_civApp->AutoSave(player);
		}

		// Not sure whether this is needed, but it seems logical to update the
		// control panel data after the build phase.
		g_gameObservers->NotifyBuildPhaseComplete(player);
	}

	gevmanager_Get()->AddEvent(GEV_INSERT_Tail, GEV_StartMovePhase,
	                       GEA_Player, player,
	                       GEA_End);

	if(g_network.IsActive()) {
		gevmanager_Get()->AddEvent(GEV_INSERT_Tail, GEV_NetworkTurnSync,
		                       GEA_Player, player,
		                       GEA_End);
	}

	return GEV_HD_Continue;
}

STDEHANDLER(StartMovePhaseEvent)
{
	sint32 player;
	if(!args->GetPlayer(0, player)) return GEV_HD_Continue;

	if(safe_player(player)->IsRobot()
	&& (!g_network.IsActive() || g_network.IsHost())
	){
	}

	if(!g_network.IsClient()) {


		gevmanager_Get()->AddEvent(GEV_INSERT_Tail, GEV_AIFinishBeginTurn,
		                       GEA_Player, player,
		                       GEA_End);
	}


#ifndef _BFR_
	gslog_LogPlayerStats(player_view::CurPlayer());

	Diplomat::GetDiplomat(player).LogDebugStatus(1);
#endif

	return GEV_HD_Continue;
}

STDEHANDLER(ProcessUnitOrdersEvent)
{
	sint32 player;
	if(!args->GetPlayer(0, player)) return GEV_HD_Continue;

	safe_player(player)->ProcessUnitOrders();
	return GEV_HD_Continue;
}

STDEHANDLER(AIFinishBeginTurnEvent)
{
	sint32 player;
	if(!args->GetPlayer(0, player)) return GEV_HD_Continue;

	CtpAi::FinishBeginTurn(player);

	return GEV_HD_Continue;
}

STDEHANDLER(GiveMapEvent)
{
	sint32 from_player;
	sint32 to_player;
	if(!args->GetPlayer(0, from_player))
		return GEV_HD_Continue;
	if(!args->GetPlayer(1, to_player))
		return GEV_HD_Continue;

	Assert(g_player[from_player] != NULL);
	g_player[from_player]->GiveMap(to_player);

	return GEV_HD_Continue;
}


STDEHANDLER(GiveCityEvent)
{
	Unit giftCity;
	sint32 player;

	if(!args->GetPlayer(0, player)) return GEV_HD_Continue;
	if(!args->GetCity(0, giftCity)) return GEV_HD_Continue;

	Assert(g_player[player] != NULL);
	g_player[giftCity->GetOwner()]->GiveCity(player, giftCity);

	return GEV_HD_Continue;
}

STDEHANDLER(EnterAgeEvent)
{
	sint32 player;
	sint32 age;

	if(!args->GetPlayer(0, player)) return GEV_HD_Continue;
	if(!args->GetInt(0, age)) return GEV_HD_Continue;

	Assert(g_player[player] != NULL);
	safe_player(player)->EnterNewAge(age);
	return GEV_HD_Continue;
}

STDEHANDLER(EndTurnEvent)
{
	sint32 player;

	if(!args->GetPlayer(0, player)) return GEV_HD_Continue;

	Assert(player == player_view::CurPlayer());

	if(player == player_view::CurPlayer()) {
		NewTurnCount::StartNextPlayer(false);
	}

	return GEV_HD_Continue;
}

void playerevent_Initialize()
{
	gevmanager_Get()->AddCallback(GEV_ContactMade,           GEV_PRI_Primary, &s_ContactMadeEvent);

	gevmanager_Get()->AddCallback(GEV_WormholeTurn,          GEV_PRI_Primary, &s_WormholeEvent);
	gevmanager_Get()->AddCallback(GEV_PlayerPatience,        GEV_PRI_Primary, &s_PatienceEvent);
	gevmanager_Get()->AddCallback(GEV_PeaceMovement,         GEV_PRI_Primary, &s_PeaceMovementEvent);
	gevmanager_Get()->AddCallback(GEV_PollutionTurn,         GEV_PRI_Primary, &s_PollutionTurnEvent);
	gevmanager_Get()->AddCallback(GEV_BeginTurnAllCities,    GEV_PRI_Primary, &s_BeginTurnAllCitiesEvent);
	gevmanager_Get()->AddCallback(GEV_BeginTurnProduction,   GEV_PRI_Primary, &s_BeginTurnProductionEvent);
	gevmanager_Get()->AddCallback(GEV_BeginTurnSupport,      GEV_PRI_Primary, &s_BeginTurnSupportEvent);
	gevmanager_Get()->AddCallback(GEV_BeginTurnImprovements, GEV_PRI_Primary, &s_BeginTurnImprovementsEvent);
	gevmanager_Get()->AddCallback(GEV_BeginTurnAgreements,   GEV_PRI_Primary, &s_BeginTurnAgreementsEvent);
	gevmanager_Get()->AddCallback(GEV_ResetAllMovement,      GEV_PRI_Primary, &s_ResetAllMovementEvent);
	gevmanager_Get()->AddCallback(GEV_AttemptRevolt,         GEV_PRI_Primary, &s_AttemptRevoltEvent);
	gevmanager_Get()->AddCallback(GEV_BeginTurnEndGame,      GEV_PRI_Primary, &s_BeginTurnEndGameEvent);
	gevmanager_Get()->AddCallback(GEV_BeginTurnGovernment,   GEV_PRI_Primary, &s_BeginTurnGovernmentEvent);
	gevmanager_Get()->AddCallback(GEV_FinishBeginTurn,       GEV_PRI_Primary, &s_FinishBeginTurnEvent);

	gevmanager_Get()->AddCallback(GEV_CreateUnit,            GEV_PRI_Primary, &s_CreateUnitEvent);

	gevmanager_Get()->AddCallback(GEV_Settle,                GEV_PRI_Primary, &s_SettleEvent);
	gevmanager_Get()->AddCallback(GEV_SettleInCity,          GEV_PRI_Primary, &s_SettleInCityEvent);
	gevmanager_Get()->AddCallback(GEV_CreateCity,            GEV_PRI_Primary, &s_CreateCityEvent);

	gevmanager_Get()->AddCallback(GEV_CreateImprovement,     GEV_PRI_Primary, &s_CreateImprovementEvent);

	gevmanager_Get()->AddCallback(GEV_GrantAdvance,          GEV_PRI_Primary, &s_GrantAdvanceEvent);

	gevmanager_Get()->AddCallback(GEV_SendGood,              GEV_PRI_Primary, &s_SendGoodEvent);
	gevmanager_Get()->AddCallback(GEV_TradeBid,              GEV_PRI_Primary, &s_TradeBidEvent);

	gevmanager_Get()->AddCallback(GEV_CreatedArmy,           GEV_PRI_Primary, &s_CreatedArmyEvent);
	gevmanager_Get()->AddCallback(GEV_SubGold,               GEV_PRI_Primary, &s_SubGoldEvent);
	gevmanager_Get()->AddCallback(GEV_AddGold,               GEV_PRI_Primary, &s_AddGoldEvent);

	gevmanager_Get()->AddCallback(GEV_EstablishEmbassy,      GEV_PRI_Primary, &s_EstablishEmbassyEvent);
	gevmanager_Get()->AddCallback(GEV_ThrowParty,            GEV_PRI_Primary, &s_ThrowPartyEvent);

	gevmanager_Get()->AddCallback(GEV_FinishBuildPhase,      GEV_PRI_Primary, &s_FinishBuildPhaseEvent);
	gevmanager_Get()->AddCallback(GEV_StartMovePhase,        GEV_PRI_Primary, &s_StartMovePhaseEvent);

	gevmanager_Get()->AddCallback(GEV_ProcessUnitOrders,     GEV_PRI_Primary, &s_ProcessUnitOrdersEvent);
	gevmanager_Get()->AddCallback(GEV_AIFinishBeginTurn,     GEV_PRI_Primary, &s_AIFinishBeginTurnEvent);
	gevmanager_Get()->AddCallback(GEV_GiveMap,               GEV_PRI_Primary, &s_GiveMapEvent);
	gevmanager_Get()->AddCallback(GEV_GiveCity,              GEV_PRI_Primary, &s_GiveCityEvent);

	gevmanager_Get()->AddCallback(GEV_EnterAge,              GEV_PRI_Primary, &s_EnterAgeEvent);
	gevmanager_Get()->AddCallback(GEV_EndTurn,               GEV_PRI_Primary, &s_EndTurnEvent);
}

void playerevent_Cleanup()
{
}
