//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ source
// Description  : turncounter handles the clockwork of turn progression
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
// _DEBUG
//
//----------------------------------------------------------------------------
//
// Modifications from the original Activision code:
//
// - Propagate PW each turn update
// - Altered filename generating for PBEM saves (JJB 2004/12/30)
// - Moved needs refueling check to Unit.cpp to remove code duplication.
//   - April 24th 2005 Martin G�hmann
// - Replaced old difficulty database by new one. (April 29th 2006 Martin G�hmann)
// - Replaced old const database by new one. (5-Aug-2007 Martin G�hmann)
// - Put SendNextPlayerMessage into its own event. (14-Nov-2007 Martin G�hmann)
//
//----------------------------------------------------------------------------

#include "ctp/c3.h"
#include "gs/utility/TurnCnt.h"

#include "robot/pathing/A_Star_Heuristic_Cost.h"
#include "gs/gameobj/AgreementPool.h"          // agreementpool_Get()
#include "gs/outcom/AICause.h"
#include "gs/gameobj/Barbarians.h"
#include "BuildingRecord.h"
#include "gs/gameobj/buildingutil.h"
#include "gs/world/Cell.h"
#include "gs/gameobj/citydata.h"
#include "gs/fileio/CivPaths.h"               // civpaths_Get()
#include "ConstRecord.h"            // g_theConstDB
#include "gs/database/DB.h"
#include "ctp/debugtools/debugmemory.h"
#include "gs/gameobj/Diffcly.h"
#include "DifficultyRecord.h"
#include "gs/gameobj/Diplomacy_Log.h"
#include "gs/gameobj/DiplomaticRequestData.h"
#include "gs/gameobj/DiplomaticRequestPool.h"  // diplomaticrequestpool_Get()
#include "gs/core/render_observer.h"
#include "robot/aibackdoor/dynarr.h"
#include "gs/events/GameEventManager.h"   // gevmanager_Get()
#include "gs/fileio/gamefile.h"
#include "gs/gameobj/GameOver.h"
#include "gs/gameobj/GameSettings.h"
#include "gs/core/audio_types.h"
#include "GovernmentRecord.h"
#include "gs/gameobj/MaterialPool.h"
#include "gs/core/game_observer.h"
#include "gs/core/player_view.h"
#include "net/general/net_action.h"
#include "net/general/net_info.h"
#include "net/general/net_rand.h"
#include "net/general/net_ready.h"
#include "net/general/network.h"
#include "gs/gameobj/Player.h"                 // player_Get, player_arr_Get
#include "gs/gameobj/pollution.h"
#include "gs/database/profileDB.h"              // profiledb_Get()
#include "gs/gameobj/Readiness.h"
#include "gs/gameobj/Score.h"
#include "gs/utility/SimpleDynArr.h"
#include "gs/slic/SlicEngine.h"
#include "gs/slic/SlicObject.h"
#include "gs/slic/SlicSegment.h"
#include "gs/core/audio_observer.h"
#include "gs/core/audio_types.h"           // SOUNDTYPE_SFX, GAMESOUNDS
#include "gs/database/StrDB.h"                  // g_theStringDB
#include "gs/core/tiledmap_observer.h"
#include "gs/gameobj/UnitData.h"
#include "gs/utility/UnitDynArr.h"
#include "UnitRecord.h"
#include "gs/gameobj/XY_Coordinates.h"
#include "gs/world/World.h"                  // world_Get()

extern Diplomacy_Log *  g_theDiplomacyLog;


sint32 g_cantEndTurn = 0;

sint32 TurnCount::sm_the_stop_player = 1;

TurnCount::TurnCount(sint32 numPlayers, sint32 initialYear)
{
	m_sliceList = new SimpleDynamicArray<sint32>;
	Init(numPlayers, initialYear);
}

TurnCount::~TurnCount()
{
	delete m_sliceList;
	m_sliceList = nullptr;
}

void TurnCount::Init(sint32 numPlayers, sint32 initialYear)
{
	m_turn = 0;
	m_round = 0;
	m_simultaneousMode = FALSE;
	m_activePlayers = numPlayers;
	m_year = initialYear;
	m_lastBeginTurn = -1;
	m_isHotSeat = FALSE;
	m_isEmail = FALSE;
	// ChooseHappinessPlayer does m_happinessPlayer++; if the field is
	// uninitialized garbage (MALLOC_PERTURB / fresh allocation), the
	// increment cascades into bad pointer arithmetic in player_Get.
	// Seed to 0 before the search so the increment lands at index 1
	// (the slot the search would settle on anyway when player 0 is the
	// barbarian).  Caught by ASan in HeavyCityDataFixture.
	m_happinessPlayer = 0;
	ChooseHappinessPlayer();
	m_sentGameAlmostOverMessage = FALSE;
	m_sentGameOverMessage = FALSE;
}

void TurnCount::SkipToRound(sint32 round)
{
	m_round = round;
	m_year = diffutil_GetYearFromTurn(gamesettings_Get()->GetDifficulty(), m_round);
}

void TurnCount::InformNetwork()
{
	if(g_network.IsHost()) {
		if(m_lastBeginTurn == player_view::CurPlayer())
			return;

		m_lastBeginTurn = player_view::CurPlayer();

		if(player_Get(player_view::CurPlayer())->IsNetwork())
		{
		g_network.QueuePacket(g_network.IndexToId(player_view::CurPlayer()),
		                                          new NetRand());
		g_network.QueuePacket(g_network.IndexToId(player_view::CurPlayer()),
		                                          new NetInfo(NET_INFO_CODE_GOLD,
		                                          player_view::CurPlayer(),
		                                          player_Get(player_view::CurPlayer())->m_gold->GetLevel()));
		g_network.QueuePacket(g_network.IndexToId(player_view::CurPlayer()),
		                                          new NetReadiness(player_Get(player_view::CurPlayer())->m_readiness));
			// propagate PW each turn update
		g_network.QueuePacket(g_network.IndexToId(player_view::CurPlayer()),
		                                          new NetInfo(NET_INFO_CODE_MATERIALS,
		                                          player_view::CurPlayer(),
		                                          player_Get(player_view::CurPlayer())->m_materialPool->GetMaterials()));
		}
		g_network.BeginTurn(player_view::CurPlayer());
		NetInfo* netInfo = new NetInfo(NET_INFO_CODE_BEGIN_TURN,
		                               player_view::CurPlayer());
		g_network.QueuePacketToAll(netInfo);
		if(player_Get(player_view::CurPlayer())->IsNetwork())
		{
			g_network.SetMyTurn(FALSE);
		}
		else
		{
			g_network.SetMyTurn(TRUE);
		}
	}
}

void TurnCount::InformMessages()
{
	gameobservers_Get()->NotifyBeginTurnMessage(player_view::VisiblePlayer());
}

void TurnCount::SliceInformNetwork()
{
	if(g_network.IsHost()) {
		if(player_Get(player_view::CurPlayer())->IsNetwork())
		{
			g_network.QueuePacket(g_network.IndexToId(player_view::CurPlayer()),
													  new NetRand());
		}
		g_network.QueuePacketToAll(new NetInfo(NET_INFO_CODE_BEGIN_SLICE,
											   player_view::CurPlayer()));
		if(player_Get(player_view::CurPlayer())->IsNetwork())
		{
			g_network.SetMyTurn(FALSE);
		}
		else
		{
			g_network.SetMyTurn(TRUE);
		}
	}
}

void TurnCount::ChooseNextActivePlayer()
{
	if(m_activePlayers <= 0)
		return;

	sint32 count = 0;

		do {
		player_view::NextPlayer();
		render_observer::NextPlayer();
		count++;
	} while(player_Get(player_view::CurPlayer()) == nullptr ||
			(player_Get(player_view::CurPlayer())->IsTurnOver() &&
			player_Get(player_view::CurPlayer())->GetCurRound() == m_round &&
			count <= k_MAX_PLAYERS));

	Assert(count <= k_MAX_PLAYERS);

}

void TurnCount::EndThisTurn()
{
	PLAYER_INDEX curPlayer = player_view::CurPlayer();

#ifdef _DEBUG
	if (g_theDiplomacyLog) g_theDiplomacyLog->EndTurn(GetRound());
#endif

	if(!player_Get(curPlayer)->IsTurnOver()) {
		player_Get(curPlayer)->EndTurn();
		m_activePlayers--;
	}

	if(g_network.IsActive()) {
		g_network.DoResetCityOwnerHack();
	}

	Assert(!g_network.IsClient());
	extern BOOL g_aPlayerIsDead;
	if(!g_network.IsClient() && g_aPlayerIsDead) {
		Player::RemoveDeadPlayers();
	}

	while((m_activePlayers > 0) &&
		  (player_Get(player_view::CurPlayer())->IsTurnOver() &&
		   player_Get(player_view::CurPlayer())->GetCurRound() == m_round)) {

		ChooseNextActivePlayer();

		curPlayer = player_view::CurPlayer();

		if(m_simultaneousMode &&
		   player_Get(curPlayer)->m_end_turn_soon) {
			if(player_Get(curPlayer)->GetCurRound() != m_round) {
				player_Get(curPlayer)->BeginTurn();
			}
			player_Get(curPlayer)->EndTurn();
			m_activePlayers--;
		}
	}
}

void TurnCount::BeginNewRound()
{
	Assert(m_activePlayers == 0);
	m_activePlayers = 0;
	sint32 i;

	m_round++;

	world_Get()->A_star_heuristic->Update();

#ifdef _DEBUG
    if (g_theDiplomacyLog) {
        g_theDiplomacyLog->BeginRound(GetRound());
    }
#endif // _DEBUG

	Barbarians::BeginYear(m_round);

	ChooseHappinessPlayer();

	m_year += diffutil_GetYearIncrementFromTurn(gamesettings_Get()->GetDifficulty(), m_round);

	RunNewYearMessages() ;
	if(g_network.IsHost()) {
		g_network.QueuePacketToAll(new NetInfo(NET_INFO_CODE_YEAR, m_round, m_year));
	}
	for(i = 0; i < k_MAX_PLAYERS; i++) {
		if(player_Get(i))
			m_activePlayers++;
	}
	player_view::NextRound();
	render_observer::NextPlayer();
	agreementpool_Get()->EndRound();
	pollution_Get()->EndRound();
	slicengine_Get()->RunYearlyTriggers();

	if(m_simultaneousMode && g_network.IsHost()) {
		for(i = 0; i < k_MAX_PLAYERS; i++) {
			if(player_Get(i)) {
				player_view::SetCurrentPlayer(i);
				BeginNewTurn(FALSE);
			}
		}
	}
	player_view::SetCurrentPlayer(0);
	BeginNewTurn(FALSE);





}

void TurnCount::BeginNewTurn(BOOL clientVerification)
{
#ifdef _DEBUG
	sint32 age;
	sint32 player_idx;

	if ((0 == m_round) && (profiledb_Get()->GetCheatAge(age))) {
		switch(age) {
		case 1:
			m_round = 124;
			for (player_idx=0; player_idx<k_MAX_PLAYERS; player_idx++) {
				 if (player_Get(player_idx)) {
					for(sint32 i = 0; i < 20; i++) {
						player_Get(player_idx)->m_advances->GiveAdvance(i, CAUSE_SCI_UNKNOWN);
					}
					player_Get(player_idx)->m_advances->GiveAdvance(30, CAUSE_SCI_UNKNOWN);

				}
			}
			break;
		case 2:
			m_round = 249;
			for (player_idx=0; player_idx<k_MAX_PLAYERS; player_idx++) {
				 if (player_Get(player_idx)) {
					for(sint32 i = 0; i < 40; i++) {
						player_Get(player_idx)->m_advances->GiveAdvance(i, CAUSE_SCI_UNKNOWN);
					}
				}
			}
			break;
		case 3:
			m_round = 374;
			for (player_idx=0; player_idx<k_MAX_PLAYERS; player_idx++) {
				 if (player_Get(player_idx)) {
					for(sint32 i = 0; i < 60; i++) {
						player_Get(player_idx)->m_advances->GiveAdvance(i, CAUSE_SCI_UNKNOWN);
					}
					player_Get(player_idx)->m_advances->GiveAdvance(60, CAUSE_SCI_UNKNOWN);
					player_Get(player_idx)->m_advances->GiveAdvance(64, CAUSE_SCI_UNKNOWN);
				}
			}

			break;
		case 4:
			m_round = 449;
			for (player_idx=0; player_idx<k_MAX_PLAYERS; player_idx++) {
				 if (player_Get(player_idx)) {
					for(sint32 i = 0; i < 80; i++) {
						player_Get(player_idx)->m_advances->GiveAdvance(i, CAUSE_SCI_UNKNOWN);
					}
				}
			}
			break;
		case 5:
			m_round = 524;
			for (player_idx=0; player_idx<k_MAX_PLAYERS; player_idx++) {
				 if (player_Get(player_idx)) {
					for(sint32 i = 0; i < 100; i++) {
						player_Get(player_idx)->m_advances->GiveAdvance(i, CAUSE_SCI_UNKNOWN);
					}
				}
			}
			break;
		}

	}
#endif // _DEBUG

	if(g_network.IsHost()) {





		if(!clientVerification) {
			InformNetwork();
		}
		if(player_Get(player_view::CurPlayer())->IsNetwork())
		{
			if(!clientVerification)
				return;
			DPRINTF(k_DBG_NET, ("Client %d acknowledes begin turn %d\n", player_view::CurPlayer(),
								m_round));
		}
	}

	if(player_Get(player_view::CurPlayer())->GetCurRound() != m_round) {
		gevmanager_Get()->AddEvent(GEV_INSERT_Tail,
							   GEV_BeginTurn,
							   GEA_Player, player_view::CurPlayer(),
							   GEA_Int, m_round,
							   GEA_End);
	} else {
		SliceInformNetwork();
	}




#ifdef _DEBUG

	if (profiledb_Get()->LogPlayerStats()) {
		LogPlayerStats();
	}
#endif
}

void TurnCount::EndThisTurnBeginNewTurn(BOOL clientRequest)
{
	if (clientRequest)
    {
		if (g_network.CurrentPlayerAckedBeginTurn())
        {
		    EndThisTurnBeginNewTurn(FALSE);
		}
	}
    else
    {
	    EndThisTurn();

	    if (m_activePlayers <= 0)
        {
		    BeginNewRound();
	    }
        else
        {
		    BeginNewTurn(FALSE);
	    }

	    if (!g_network.IsActive() ||
            player_view::CurPlayer() == g_network.GetPlayerIndex()
           )
        {
		    player_view::Refresh();
	    }
    }
}

void TurnCount::EndThisSlice()
{
	sint32 nextPlayer;
	if(!m_simultaneousMode) {
		ChooseNextActivePlayer();
	} else {
		if(m_sliceList->Num() > 0) {
			nextPlayer = m_sliceList->Access(0);
			m_sliceList->DelIndex(0);
			player_view::SetCurrentPlayer(nextPlayer);
			render_observer::NextPlayer();
		} else {
			ChooseNextActivePlayer();
		}
	}
}

BOOL TurnCount::BeginNewSlice()
{
	PLAYER_INDEX curPlayer = player_view::CurPlayer();

	if(player_Get(curPlayer)->GetCurRound() != m_round) {
		BeginNewTurn(FALSE);
	} else if(g_network.IsHost() &&
						  player_view::CurPlayer() == player_view::VisiblePlayer()) {





		audio_observer::AddSound((sint32)SOUNDTYPE_SFX, (uint32)0,
									gamesounds_GetGameSoundID(GAMESOUNDS_NET_YOUR_TURN),
									0,
									0);
	}

	if(player_Get(curPlayer)->m_end_turn_soon) {
		return FALSE;
	}

	if(m_simultaneousMode) {
		if((g_network.IsHost() && !player_Get(curPlayer)->IsNetwork()) ||
		   g_network.GetPlayerIndex() == curPlayer) {
			player_Get(curPlayer)->ProcessUnitOrders(TRUE);
		}
	}

	if(g_network.IsHost()) {
		if(player_Get(player_view::CurPlayer())->IsNetwork())
		{
		g_network.QueuePacket(g_network.IndexToId(player_view::CurPlayer()),
												  new NetRand());
		}
		g_network.QueuePacketToAll(new NetInfo(NET_INFO_CODE_BEGIN_SLICE,
											   player_view::CurPlayer()));
		if(player_Get(player_view::CurPlayer())->IsNetwork())
		{
			g_network.SetMyTurn(FALSE);
		}
		else
		{
			g_network.SetMyTurn(TRUE);
		}
		if(m_sliceList->Num() > 0)
		{
			g_network.QueuePacket(g_network.IndexToId(
				player_view::CurPlayer()),
								  new NetInfo(NET_INFO_CODE_REQUEST_SLICE));
		}

	}


	if(g_network.IsActive()) {
		g_network.UnitsMoved(-g_network.GetUnitMovesUsed());
	}
	return TRUE;
}

void TurnCount::EndThisSliceBeginNewSlice()
{
	if(g_network.IsClient()) {
		g_network.SendAction(new NetAction(NET_ACTION_END_SLICE));
		g_network.SetMyTurn(FALSE);
		return;
	}

	EndThisSlice();
	if(!BeginNewSlice()) {
		EndThisTurnBeginNewTurn();
	}
}

void TurnCount::SetSliceTo(sint32 player)
{
	if(!player_Get(player) ||
	   (player_Get(player)->IsTurnOver() &&
		player_Get(player)->GetCurRound() == m_round)) {
		return;
	}

	player_view::SetCurrentPlayer(player);
	render_observer::NextPlayer();
	BeginNewSlice();
}

void TurnCount::QueueSliceFor(sint32 player)
{
	m_sliceList->Insert(player);
	if(g_network.IsHost()) {
		g_network.QueuePacket(g_network.IndexToId(
			player_view::CurPlayer()),
			new NetInfo(NET_INFO_CODE_REQUEST_SLICE));
	}
}

BOOL TurnCount::SimultaneousMode() const
{
	return m_simultaneousMode;
}

void TurnCount::SetSimultaneousMode(BOOL on)
{
	m_simultaneousMode = on;
}


BOOL TurnCount::VerifyEndTurn(BOOL force)
{
	Player *player = player_Get(player_view::CurPlayer());

	if (!player->IsHuman())
	{
		return(TRUE);
	}

	if (player_view::IsModalMessageActive() && !force)

		return FALSE;

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
				SlicObject *so = new SlicObject("16IAOutOfFuel");
				so->AddRecipient(player->m_owner);
				so->AddCivilisation(player->m_owner);
				slicengine_Get()->Execute(so);
				return(FALSE);
			}
		}
	}

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
			double fudge = (double)(g_theConstDB->Get(0)->GetStarvationWarningFudgeFactor()) / 100.0;
			if ((city->GetProducedFood() < city->GetConsumedFood()) &&
				((fudge * (city->GetStoredCityFood() + city->GetProducedFood())) <
				 city->GetConsumedFood())) {
				SlicObject *so = new SlicObject("23IACityWillStarve") ;
				so->AddRecipient(player->m_owner) ;
				so->AddCity(*unit) ;
				slicengine_Get()->Execute(so) ;
				return(FALSE);
			}
		}
	}

	if (slicengine_Get()->GetSegment("21IACannotAffordMaintenance")->TestLastShown(player->m_owner, 1, turn_Get()->GetRound())) {
		if (player->m_gold->BankruptcyImminent() &&
			(player->CalcTotalBuildingUpkeep() > 0)) {
			SlicObject *so = new SlicObject("21IACannotAffordMaintenance");
			so->AddRecipient(player->m_owner);
			so->AddCivilisation(player->m_owner);
			slicengine_Get()->Execute(so);
			return(FALSE);
		}
	}

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
			SlicObject *so = new SlicObject("22IACannotAffordSupport");
			so->AddRecipient(player->m_owner);
			so->AddCivilisation(player->m_owner);
			slicengine_Get()->Execute(so);
			return(FALSE);
		}
	}

	return(TRUE);
}


void TurnCount::NetworkEndTurn(BOOL force)
{
	Assert(!g_network.SetupMode());
	if(g_network.SetupMode())
		return;

	if (!VerifyEndTurn(force))
		return;

	if(g_network.IsClient()) {
		g_network.SendAction(new NetAction(NET_ACTION_END_TURN));
		g_network.SetMyTurn(FALSE);
		return;
	} else if(g_network.IsHost()) {
		render_observer::AddEndTurn();
		return;
	}

	#if 0 // Unreachable
	{
		if(player_Get(player_view::CurPlayer())->IsNetwork())
		{
			for(sint32 i = 0; i < k_MAX_PLAYERS; i++) {
				if(player_Get(i) && !player_Get(i)->IsNetwork()) {
					if(!player_Get(i)->IsTurnOver() && player_Get(i)->GetCurRound() == m_round) {
						player_Get(i)->EndTurnSoon();
					}
				}
			}
			return;
		}
	}

    EndThisTurnBeginNewTurn(FALSE);

#ifdef _DEBUG
	extern BOOL g_doingFastRounds;

	if (!g_doingFastRounds)
    {
		tiledmap_observer::InvalidateMix();
		tiledmap_observer::InvalidateMap();
		tiledmap_observer::Refresh();
		if (gameobservers_Get()) gameobservers_Get()->NotifyRadarMapUpdate(player_view::VisiblePlayer());
	}
#else
	tiledmap_observer::InvalidateMix();
	tiledmap_observer::InvalidateMap();
	tiledmap_observer::Refresh();
	if (gameobservers_Get()) gameobservers_Get()->NotifyRadarMapUpdate(player_view::VisiblePlayer());
#endif

#endif // Unreachable
}

void TurnCount::RunNewYearMessages()
{

	if (GetYear() >= g_theConstDB->Get(0)->GetEndOfGameYearEarlyWarning()) {
		if(!m_sentGameAlmostOverMessage) {
			m_sentGameAlmostOverMessage = TRUE;

			SendMsgEndOfGameEarlyWarning() ;
		}
	}

	if(GetYear() >= g_theConstDB->Get(0)->GetEndOfGameYear()) {
		if(!m_sentGameOverMessage) {
			sint32 i;
			sint32 highScore = -1;
			sint32 highPlayer = 1;

			for(i = 1; i < k_MAX_PLAYERS; i++) {
				if(player_Get(i)) {
					if(player_Get(i)->m_score->GetTotalScore() > highScore) {
						highScore = player_Get(i)->m_score->GetTotalScore();
						highPlayer = i;
					}
				}
			}
			if(g_network.IsHost()) {
				g_network.Enqueue(new NetInfo(NET_INFO_CODE_GAME_OVER_OUT_OF_TIME,
				                              highPlayer));
			}

			for(i = 0; i < k_MAX_PLAYERS; i++) {
				if(player_Get(i)) {
					if(i == highPlayer) {
						player_Get(i)->GameOver(GAME_OVER_WON_OUT_OF_TIME, -1);
					} else {
						player_Get(i)->GameOver(GAME_OVER_LOST_OUT_OF_TIME, -1);
					}
				}
			}
		} else {
			gamesettings_Get()->SetKeepScore(FALSE);
		}
	}
}

void TurnCount::SendMsgEndOfGameEarlyWarning()
{
	SendMsgToAllPlayers("73EndOfGameTimeIsRunningOut") ;
	if(g_network.IsHost()) {
		g_network.Enqueue(new NetInfo(NET_INFO_CODE_TIMES_ALMOST_UP));
	}
}

void TurnCount::SendMsgToAllPlayers(MBCHAR *s)
	{
	sint32	i ;

	SlicObject *so = new SlicObject(s) ;

	for(i=0; i<k_MAX_PLAYERS; i++)
		{
		if ((player_Get(i)) && (!player_Get(i)->IsDead()))
			so->AddRecipient(i) ;

		}

	slicengine_Get()->Execute(so) ;
	}

void TurnCount::CountActivePlayers()
{
	sint32 i;
	m_activePlayers = 0;
	for(i = 0; i < k_MAX_PLAYERS; i++) {
		if(player_Get(i) &&
		   ((!player_Get(i)->IsTurnOver()) ||
			player_Get(i)->GetCurRound() != m_round)) {
			m_activePlayers++;
		}
	}
}

void TurnCount::PlayerDead(PLAYER_INDEX player)
{
	CountActivePlayers();
}

void TurnCount::RegisterNewPlayer(PLAYER_INDEX player)
{
	CountActivePlayers();
}

void TurnCount::SetHotSeat(BOOL on)
{
	m_isHotSeat = on;
}

void TurnCount::SetEmail(BOOL on)
{
	m_isEmail = on;
}

BOOL TurnCount::IsHotSeat()
{
	return m_isHotSeat;
}

BOOL TurnCount::IsEmail()
{
	return m_isEmail;
}

sint32 g_noai_stop_player = 1;

void TurnCount::NextRound(BOOL fromDirector, BOOL force)
{
	extern sint32 g_isCheatModeOn;
	if(g_isCheatModeOn)
		return;

	if(g_cantEndTurn)
		return;

	if (player_view::CurPlayer() != player_view::VisiblePlayer())
		return;

	if(!fromDirector) {
		player_view::RegisterManualEndTurn();
	}

	if (profiledb_Get()->IsAIOn()) {

		TurnCount::SetStopPlayer(player_view::CurPlayer());
	} else {
		g_noai_stop_player = player_view::CurPlayer();
	}

#ifdef _DEBUG
sint32 finite_count=0;
#endif

	do
	{
		Assert(finite_count++ < 100);

		sint32 curPlayer = player_view::CurPlayer();
		if((player_Get(curPlayer)->IsHuman() ||
			(player_Get(curPlayer)->IsNetwork() &&
			 g_network.IsLocalPlayer(curPlayer))) &&
		   player_view::CurPlayer() == player_view::VisiblePlayer()) {
			player_Get(player_view::CurPlayer())->ProcessUnitOrders();
		}

		if(g_network.IsActive())
		{
			NetworkEndTurn();
			return;
		}

		if (!VerifyEndTurn(force))
			return;

		player_view::SetVisiblePlayer(player_view::VisiblePlayer());
		if (profiledb_Get()->IsAIOn())
		{
			TurnCount::SetStopPlayer(player_view::CurPlayer());
		}

		EndThisTurnBeginNewTurn();

		if(m_isHotSeat || m_isEmail)
		{
			if(!player_Get(player_view::CurPlayer())->IsRobot())
			{
				player_view::SetVisiblePlayer(player_view::CurPlayer());

				TurnCount::SetStopPlayer(player_view::CurPlayer());
			}

			if(!player_Get(player_view::CurPlayer())->IsRobot())
			{
				SendNextPlayerMessage();
			}

			render_observer::NextPlayer();
			render_observer::AddCopyVision();

			tiledmap_observer::InvalidateMix();
			tiledmap_observer::InvalidateMap();
			tiledmap_observer::Refresh();
			if (gameobservers_Get()) gameobservers_Get()->NotifyRadarMapUpdate(player_view::VisiblePlayer());
			InformMessages();
		}
	} while (false);
}

void TurnCount::ChooseHappinessPlayer()
{
	if(!player_arr_Get()) {
		m_happinessPlayer = 0;
		return;
	}

	sint32 i;
	for(i = 0; i < k_MAX_PLAYERS + 1; i++) {
		m_happinessPlayer++;
		if(m_happinessPlayer >= k_MAX_PLAYERS)
			m_happinessPlayer = 0;

		if(!player_Get(m_happinessPlayer))
			continue;
		if(!player_Get(m_happinessPlayer)->IsRobot())
			continue;
		break;
	}

}

#ifdef _DEBUG
void TurnCount::LogPlayerStats()
{
	PLAYER_INDEX    playerNum   = player_view::CurPlayer();
	MBCHAR          filename[80];
	snprintf(filename, sizeof(filename), "Playerlog%#.2d.txt", playerNum);
	FILE *  logfile = fopen(filename, "rt");

	if (logfile)
    {
        // Created before
		fclose(logfile);
		logfile = fopen(filename, "at");
       	if (!logfile) return;
    }
    else
    {
        // First creation
		logfile = fopen(filename, "wt");
		if (!logfile) return;

		fprintf(logfile, "%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t\n",
		        "Turn #",
		        "# Cities",
		        "Total Prod/Turn",
		        "Total Food/Turn",
		        "Total Gold/Turn",
		        "Total Sci/Turn",
		        "# Rioting",
		        "# Revolting",
		        "Gov Type",
		        "Workday",
		        "Wages",
		        "Rations",
		        "Science Setting",
		        "PW Setting",
		        "# of units",
		        "Unit Support",
		        "Total Pop",
		        "Largest City",
		        "Pollution",
		        "Income Percent",
		        "# advances known",
		        "AIP File");
	}

	UnitDynamicArray *  cityList    = player_Get(playerNum)->GetAllCitiesList();
	sint32              citySize;
	sint32              maxCitySize = -1;
	sint32              numCitiesRioting = 0;
	sint32              totalPop = 0;
	sint32              totalFood = 0;
	sint32              totalProduction = 0;
	sint32              totalGold = 0;

	CityData            *cityData;
	Unit                city;

	for (int i = 0; i < cityList->Num(); i++) {
		city = cityList->Access(i);
		cityData = city.AccessData()->GetCityData();

		cityData->GetPop(citySize);

		totalPop += citySize;

		if (citySize > maxCitySize)
			maxCitySize = citySize;

		if (cityData->GetIsRioting())
			numCitiesRioting++;

		totalFood       += cityData->GetNetCityFood();
		totalProduction += cityData->GetNetCityProduction();
		totalGold       += cityData->GetNetCityGold();

	}

	fprintf(logfile, "%d\t", m_round);
	fprintf(logfile, "%d\t", player_Get(playerNum)->GetNumCities());
	fprintf(logfile, "%d\t", totalProduction);
	fprintf(logfile, "%d\t", totalFood);
	fprintf(logfile, "%d\t", totalGold);
	fprintf(logfile, "%d\t", player_Get(playerNum)->m_gold->GetScience());
	fprintf(logfile, "%d\t", numCitiesRioting);
	fprintf(logfile, "%d\t", player_Get(playerNum)->GetNumRevolted());
	fprintf(logfile, "%d\t", player_Get(playerNum)->GetGovernmentType());
	fprintf(logfile, "%#.3f\t", player_Get(playerNum)->GetWorkdayPerPerson());
	fprintf(logfile, "%#.3f\t", player_Get(playerNum)->GetWagesPerPerson());
	fprintf(logfile, "%#.3f\t", player_Get(playerNum)->GetRationsPerPerson());

	double taxRate;
	player_Get(playerNum)->GetScienceTaxRate(taxRate);
	fprintf(logfile, "%#.2f\t", taxRate);

	fprintf(logfile, "%#.2f\t", player_Get(playerNum)->m_materialsTax);
	fprintf(logfile, "%d\t", player_Get(playerNum)->GetAllUnitList()->Num());
	fprintf(logfile, "%d\t", player_Get(playerNum)->GetReadinessCost());
	fprintf(logfile, "%d\t", totalPop);
	fprintf(logfile, "%d\t", maxCitySize);
	fprintf(logfile, "%d\t", player_Get(playerNum)->GetCurrentPollution());
	fprintf(logfile, "%#.3f\t", player_Get(playerNum)->GetIncomePercent());

	sint32 numAdvances = 0;
	for (sint32 adv=0; adv<player_Get(playerNum)->NumAdvances(); adv++)
		if (player_Get(playerNum)->HasAdvance(adv)) {
			numAdvances++;
		}
	fprintf(logfile, "%d\t", numAdvances);

    fprintf(logfile, "\n");
	fclose(logfile);
}

#endif

void TurnCount::NotifyBecameHost()
{
	CountActivePlayers();
}

void TurnCount::SendNextPlayerMessage()
{
	gevmanager_Get()->AddEvent(GEV_INSERT_Tail, GEV_SendEmailAndHotSeatMessage,
	                       GEA_End);

}

// Only called from the event:
void TurnCount::SendNextPlayerMessageEvent()
{
	if(!m_isHotSeat && !m_isEmail)
		return;

	sint32 player = player_view::CurPlayer();

	if(!player_Get(player))
		return;

	if(m_isEmail) {

		is_scenario_Set(FALSE);

		MBCHAR fullPath[_MAX_PATH];
		MBCHAR *c;
		MBCHAR *startc;
		MBCHAR *fc;
		strncpy(fullPath, civpaths_Get()->GetDesktopPath(), sizeof(fullPath));
		fullPath[sizeof(fullPath) - 1] = '\0';
		// JJB changed this from CTP to CTP2 to avoid confusion between the two games
		strncat(fullPath, "\\CTP2 Email To ", sizeof(fullPath) - strlen(fullPath) - 1);

		startc = player_Get(player)->m_email;
		c = startc;
		fc = &fullPath[strlen(fullPath)];
		while(*c && (fc - fullPath) < (sint32)(sizeof(fullPath) - 50)) {
			if((*c >= 'a' && *c <= 'z') ||
			   (*c >= 'A' && *c <= 'Z') ||
			   (*c >= '0' && *c <= '9') ||
			   *c == '-' || *c == '_' || *c == '@' ||
			   *c == '.') {
				*fc = *c;
				fc++;
				*fc = 0;
			}
			c++;
		}
		MBCHAR turnString[_MAX_PATH];
		// JJB changed this from CTP to CTP2 to avoid confusion between the two games
		// since m_round seems to always be zero
		snprintf(turnString, sizeof(turnString), " (Turn %d)", GetRound()); // New turn is changed with the BeginTurn event, which still has to be executed.
		strncat(fullPath, turnString, sizeof(fullPath) - strlen(fullPath) - 1);
		strncat(fullPath, ".c2g", sizeof(fullPath) - strlen(fullPath) - 1);
		GameFile::SaveGame(fullPath, nullptr);
	}

	SlicObject * so =
	    new SlicObject(m_isHotSeat ? "104NextHotSeatPlayer" : "105NextEmailPlayer");
	so->AddRecipient(player);
	so->AddCivilisation(player);
	if(m_isEmail)
		so->AddAction(player_Get(player)->m_email);

	slicengine_Get()->Execute(so);

}
