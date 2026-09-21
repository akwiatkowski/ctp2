//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ source
// Description  : Turn count handler
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
// - Relaxed assert
// - Moved needs refueling check to Unit.cpp to remove code duplication.
//   - April 24th 2005 Martin G�hmann
// - Replaced old difficulty database by new one. (April 29th 2006 Martin G�hmann)
// - Replaced old const database by new one. (5-Aug-2007 Martin G�hmann)
// - Fixed PBEM BeginTurn event execution. (27-Oct-2007 Martin G�hmann)
// - PollutionBeginTurn is now triggered from PlayerBeginTurn if executed
//   so that flood events make players invalid after all the player events. (29-Oct-2007 Martin G�hmann)
//
//----------------------------------------------------------------------------

#include "ctp/c3.h"

#include "gs/utility/newturncount.h"
#include "gs/events/GameEventManager.h"   // gevmanager_Get()

#include "ctp/ctp2_utils/c3errors.h"

#include "gs/core/render_observer.h"

#include "gs/slic/SlicObject.h"
#include "gs/core/game_observer.h"
#include "gs/core/player_view.h"
#include "gs/gameobj/player.h"
#include "gs/world/World.h"
#include "gs/gameobj/Barbarians.h"
#include "robot/pathing/A_Star_Heuristic_Cost.h"
#include "gs/slic/SlicSegment.h"
#include "gs/slic/SlicEngine.h"
#include "gs/database/profileDB.h"
#include "DifficultyRecord.h"
#include "gs/gameobj/Diffcly.h"
#include "gs/core/tiledmap_observer.h"

#include "gs/gameobj/pollution.h"

#include "net/general/network.h"
#include "net/general/net_action.h"
#include "net/general/net_info.h"
#include "net/general/net_rand.h"
#include "net/general/net_ready.h"

#include "ConstRecord.h"
#include "gs/gameobj/Score.h"
#include "gs/gameobj/GameOver.h"
#include "gs/gameobj/GameSettings.h"
#include "gs/gameobj/Unit.h"
#include "gs/gameobj/UnitData.h"
#include "UnitRecord.h"
#include "gs/utility/UnitDynArr.h"
#include "gs/gameobj/Readiness.h"
#include "gs/gameobj/buildingutil.h"

#include "gs/utility/TurnCnt.h"

#include "gs/gameobj/CriticalMessagesPrefs.h"
#include "gs/gameobj/Gold.h"
#include <memory>


extern World                    *g_world;


sint32 NewTurnCount::sm_the_stop_player = 1;

bool NewTurnCount::m_sentGameAlmostOverMessage=false;
bool NewTurnCount::m_sentGameOverMessage=false;

NewTurnCount::NewTurnCount()
{
	m_sentGameAlmostOverMessage=false;
	m_sentGameOverMessage=false;
}

sint32 NewTurnCount::GetStopPlayer()
{
	return sm_the_stop_player;
}

void NewTurnCount::SetStopPlayer(const sint32 &player_index)
{
	sm_the_stop_player = player_index;
}

void NewTurnCount::StartNextPlayer(bool stop)
{
	DPRINTF(1, ("NewTurnCount::StartNextPlayer(%d), curPlayer: %d\n", stop, player_view::CurPlayer()));

	static bool warned=false;
	if (!VerifyEndTurn(warned))
	{
		warned=true;
		return;
	}
	warned=false;

	PLAYER_INDEX current_player = player_view::CurPlayer();

	if(network_Get().IsClient()) {
		network_Get().SendAction(std::make_unique<NetAction>(NET_ACTION_END_TURN).release());
		return;
	}

	player_Get(current_player)->EndTurn();


	extern BOOL g_aPlayerIsDead;
	if(!network_Get().IsClient() && g_aPlayerIsDead) {
		Player::RemoveDeadPlayers();
	}

	NewTurnCount::ChooseNextActivePlayer();
	PLAYER_INDEX next_player = player_view::CurPlayer();
	sint32 next_round = player_Get(next_player)->GetCurRound() + 1;

	if(turn_Get()->IsHotSeat() || turn_Get()->IsEmail())
	{
		if(!player_Get(player_view::CurPlayer())->IsRobot())
		{
			stop = true;
		}

		render_observer::NextPlayer();
		render_observer::AddCopyVision();

		tiledmap_observer::InvalidateMix();
		tiledmap_observer::InvalidateMap();
		tiledmap_observer::Refresh();
		if (gameobservers_Get()) gameobservers_Get()->NotifyRadarMapUpdate(current_player);
		turn_Get()->InformMessages();

		if (gameobservers_Get()) gameobservers_Get()->NotifyHideMainUI();
	}

	if (stop ||
		(network_Get().IsActive() &&
		 (network_Get().IsClient() || !player_Get(next_player)->IsRobot())))
	{
		NewTurnCount::SetStopPlayer(next_player);
		render_observer::NextPlayer();
	}

	if(network_Get().IsHost() && GetStopPlayer() == next_player)
	{
		if(player_Get(next_player)->IsRobot())
		{
			SetStopPlayer(player_view::VisiblePlayer());
		}
	}

	if (gameobservers_Get()) gameobservers_Get()->NotifyUpdatePlayerEndProgress(current_player);

	sint32 oldVis = player_view::VisiblePlayer();
	player_view::SetVisiblePlayer(NewTurnCount::GetStopPlayer());

	if (oldVis != player_view::VisiblePlayer())
	{
		tiledmap_observer::CopyVision();
	}

	if (next_player == 0)
	{
		NewTurnCount::StartNewYear();
	}

	if(network_Get().IsHost())
	{
		network_Get().QueuePacketToAll(std::make_unique<NetInfo>(NET_INFO_CODE_BEGIN_TURN, next_player).release());
	}

	if((turn_Get()->IsHotSeat() || turn_Get()->IsEmail())
	&& !player_Get(player_view::CurPlayer())->IsRobot()
	){
		turn_Get()->SendNextPlayerMessage();

		if(turn_Get()->IsEmail())
		{
			return;
		}
	}

	if(!network_Get().IsHost() || network_Get().IsLocalPlayer(next_player))
	{
		gevmanager_Get()->AddEvent(GEV_INSERT_Tail, GEV_BeginTurn,
		                       GEA_Player,      next_player,
		                       GEA_Int,         next_round,
		                       GEA_End);
	}
	else
	{
		render_observer::NextPlayer();
		pollution_Get()->BeginTurn();
	}
}

void NewTurnCount::ChooseNextActivePlayer()
{
	do {
		player_view::NextPlayer();
		render_observer::NextPlayer();
	} while( player_Get(player_view::CurPlayer()) == nullptr );
}

void NewTurnCount::StartNewYear()
{

	world_Get()->A_star_heuristic->Update();

	Barbarians::BeginYear(turn_Get()->GetRound());





	slicengine_Get()->RunYearlyTriggers();

	pollution_Get()->EndRound();

	RunNewYearMessages();
}

void NewTurnCount::ClientStartNewYear()
{

	world_Get()->A_star_heuristic->Update();

	RunNewYearMessages();
}

sint32 NewTurnCount::GetCurrentYear(sint32 player)
{
	PLAYER_INDEX current_player = player_view::CurPlayer();
	if(player >= 0 && player < k_MAX_PLAYERS)
		current_player = player;

	Assert(player_arr_Get() != nullptr);
	Assert(player_Get(current_player) != nullptr);
	if(!player_arr_Get() || !player_Get(current_player)) return 0;

	sint32 round = player_Get(current_player)->GetCurRound();

	return diffutil_GetYearFromTurn(gamesettings_Get()->GetDifficulty(), round);
}

sint32 NewTurnCount::GetCurrentRound()
{
	PLAYER_INDEX current_player = player_view::CurPlayer();
	Assert(player_arr_Get() != nullptr);
	if(!player_arr_Get() || !player_Get(current_player)) return 0;

	return player_Get(current_player)->GetCurRound();
}

void NewTurnCount::RunNewYearMessages()
{

	if (GetCurrentYear() >= g_theConstDB->Get(0)->GetEndOfGameYearEarlyWarning())
	{
		if(!m_sentGameAlmostOverMessage)
		{
			m_sentGameAlmostOverMessage = TRUE;

			SendMsgEndOfGameEarlyWarning() ;
		}
	}

	if(GetCurrentYear() >= g_theConstDB->Get(0)->GetEndOfGameYear())
	{
		if(!m_sentGameOverMessage)
		{
			sint32 i;
			sint32 highScore = -1;
			sint32 highPlayer = 1;

			for(i = 1; i < k_MAX_PLAYERS; i++)
			{
				if(player_Get(i))
				{
					if(player_Get(i)->m_score->GetTotalScore() > highScore)
					{
						highScore = player_Get(i)->m_score->GetTotalScore();
						highPlayer = i;
					}
				}
			}
			if(network_Get().IsHost())
			{
				network_Get().Enqueue(std::make_unique<NetInfo>(NET_INFO_CODE_GAME_OVER_OUT_OF_TIME,
				                              highPlayer).release());
			}

			for(i = 0; i < k_MAX_PLAYERS; i++)
			{
				if(player_Get(i))
				{
					if(i == highPlayer)
					{
						player_Get(i)->GameOver(GAME_OVER_WON_OUT_OF_TIME, -1);
					}
					else
					{
						player_Get(i)->GameOver(GAME_OVER_LOST_OUT_OF_TIME, -1);
					}
				}
			}
			m_sentGameOverMessage = true;
		}
		else
		{
			gamesettings_Get()->SetKeepScore(FALSE);
		}
	}
}

void NewTurnCount::SendMsgEndOfGameEarlyWarning()
{
	SendMsgToAllPlayers(const_cast<MBCHAR *>("73EndOfGameTimeIsRunningOut")) ;
	if(network_Get().IsHost())
	{
		network_Get().Enqueue(std::make_unique<NetInfo>(NET_INFO_CODE_TIMES_ALMOST_UP).release());
	}
}

void NewTurnCount::SendMsgToAllPlayers(MBCHAR *s)
{
	sint32	i ;

	auto so = std::make_unique<SlicObject>(s) ;

	for(i=0; i<k_MAX_PLAYERS; i++)
	{
		if ((player_Get(i)) && (!player_Get(i)->IsDead()))
			so->AddRecipient(i) ;

	}

	slicengine_Get()->Execute(so.release()) ;
}

BOOL NewTurnCount::VerifyEndTurn(BOOL force)
{
	Player *player = player_Get(player_view::CurPlayer());

	if (!player->IsHuman())
	{
		return(TRUE);
	}

	if(network_Get().IsActive() && (network_Get().IsSpeedStyle() || network_Get().IsTimedStyle())) {
		return TRUE;
	}

	if (player_view::IsModalMessageActive() && !force)
		return FALSE;

	if(critical_messages_prefs_Get()->IsEnabled("16IAOutOfFuel")) {
		if (slicengine_Get()->GetSegment("16IAOutOfFuel")->TestLastShown(player->m_owner, 1, turn_Get()->GetRound())) {
			int i;
			int n = player->GetAllUnitList()->Num();
			for (i=0; i<n; i++) {
				Unit *unit = &(player->GetAllUnitList()->Access(i));
				if (!(unit->GetMovementTypeAir()) && !(unit->GetMovementTypeSpace()))
					continue;
				if (!(unit->GetDBRec()->GetNoFuelThenCrash()))
					continue;
				if(unit->AccessData()->CheckForRefuel())
					continue;
				if (unit->NeedsRefueling()) {
					auto so = std::make_unique<SlicObject>("16IAOutOfFuel") ;
					so->AddRecipient(player->m_owner) ;
					so->AddCivilisation(player->m_owner) ;
					slicengine_Get()->Execute(so.release()) ;
					return(FALSE);
				}
			}
		}
	}

	if(critical_messages_prefs_Get()->IsEnabled("23IACityWillStarve")) {
		if (slicengine_Get()->GetSegment("23IACityWillStarve")->TestLastShown(player->m_owner, 1, turn_Get()->GetRound())) {
			int i;
			int n = player->GetAllCitiesList()->Num();
			for (i=0; i<n; i++) {
				double tmp;
				Unit *unit = &(player->GetAllCitiesList()->Access(i));
				if (!(unit->IsCity()))
					continue;
				if (buildingutil_HaveFoodVat(unit->GetImprovements(), tmp, unit->GetOwner()))
					continue;
				CityData *city = unit->GetData()->GetCityData();
//				double fudge = (double)(g_theConstDB->StarvationWarningFudgeFactor()) / 100.0;
				if ((city->GetProducedFood() < city->GetConsumedFood()) &&
					(city->GetStarvationTurns() == 0)) {
					auto so = std::make_unique<SlicObject>("23IACityWillStarve") ;
					so->AddRecipient(player->m_owner) ;
					so->AddCity(*unit) ;
					slicengine_Get()->Execute(so.release()) ;
					return(FALSE);
				}
			}
		}
	}

	if(critical_messages_prefs_Get()->IsEnabled("21IACannotAffordMaintenance")) {
		if (slicengine_Get()->GetSegment("21IACannotAffordMaintenance")->TestLastShown(player->m_owner, 1, turn_Get()->GetRound())) {
			if (player->m_gold->BankruptcyImminent() &&
				(player->CalcTotalBuildingUpkeep() > 0)) {
				auto so = std::make_unique<SlicObject>("21IACannotAffordMaintenance") ;
				so->AddRecipient(player->m_owner) ;
				so->AddCivilisation(player->m_owner) ;
				slicengine_Get()->Execute(so.release()) ;
				return(FALSE);
			}
		}
	}

	if(critical_messages_prefs_Get()->IsEnabled("22IACannotAffordSupport")) {
		if (slicengine_Get()->GetSegment("22IACannotAffordSupport")->TestLastShown(player->m_owner, 1, turn_Get()->GetRound())) {
			int i;
			int n = player->GetAllCitiesList()->Num();
			double prod_total = 0.0;
			double fudge = (double)(g_theConstDB->Get(0)->GetSupportWarningFudgeFactor()) / 100.0;
			for (i=0; i<n; i++) {
				Unit *unit = &(player->GetAllCitiesList()->Access(i));
				if (!(unit->IsCity()))
					continue;
				CityData *city = unit->GetData()->GetCityData();
				prod_total += city->ProjectMilitaryContribution();
			}
			prod_total *= fudge;
			if (!(player->m_first_city) &&
				(prod_total < player->m_readiness->GetCost())) {
				auto so = std::make_unique<SlicObject>("22IACannotAffordSupport") ;
				so->AddRecipient(player->m_owner) ;
				so->AddCivilisation(player->m_owner) ;
				slicengine_Get()->Execute(so.release()) ;
				return(FALSE);
			}
		}
	}

	return(TRUE);
}
