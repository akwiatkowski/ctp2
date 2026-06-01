//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ source
// Description  : Player turn event organisation
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
// - Generate debug version when set.
//
//----------------------------------------------------------------------------
//
// Modifications from the original Activision code:
//
// - PollutionBeginTurn is now triggered from PlayerBeginTurn if executed
//   so that flood events make players invalid after all the player events. (29-Oct-2007 Martin G�hmann)
//
//----------------------------------------------------------------------------

#include "ctp/c3.h"
#include "gs/gameobj/Player.h"
#include "gs/events/GameEventManager.h"
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
#include "gs/gameobj/XY_Coordinates.h"
#include "gs/world/World.h"
#include "gs/slic/SlicEngine.h"
#include "gs/slic/SlicObject.h"
#include "gs/gameobj/UnitData.h"
#include "gs/gameobj/Army.h"
#include "gs/gameobj/EndGame.h"

#include "gs/gameobj/Score.h"
#include "gs/gameobj/pollution.h"                  // g_thePollution

#include "gs/database/profileDB.h"
#include "ctp/civapp.h"
#include "net/general/network.h"
#include "net/general/net_info.h"
#include "gs/gameobj/Strengths.h"
#include "gs/gameobj/MessagePool.h"
#include "net/general/net_strengths.h"

#include "ctp/debugtools/debugmemory.h"

// Clean architecture: game event observer registry
#include "gs/core/game_observer.h"

extern sint32                   g_tileImprovementMode;

void Player::BeginTurn()
{
	if (g_tileImprovementMode)
	{
		g_tileImprovementMode = 0;

	}

	g_gameObservers->NotifyTurnStart(m_owner);
	g_gameObservers->NotifyUpdateControlPanel(m_owner);

	sint32 i;
	for(i = m_messages->Num() - 1; i >= 0; i--)
	{
		if(!messagepool_Get()->IsValid(m_messages->Access(i)))
		{
			m_messages->DelIndex(i);
			continue;
		}
	}

	g_gameObservers->NotifyUpdateMessages(m_owner);

	m_is_turn_over = FALSE;

	m_end_turn_soon = FALSE;

	if(g_network.IsHost())
	{
		g_network.Block(m_owner);
		g_network.Enqueue(new NetInfo(NET_INFO_CODE_SET_ROUND, m_owner, m_current_round));
		g_network.Unblock(m_owner);
	}

	DPRINTF(k_DBG_GAMESTATE, ("\n"));

	if(!g_network.IsActive() || g_network.IsHost() || (m_owner == g_network.GetPlayerIndex()))
	{
		DPRINTF(k_DBG_GAMESTATE, ("Player[%d]::BeginTurn: running\n", m_owner));

		m_civRevoltingCitiesShouldJoin = -1;

		for(sint32 p = 0; p < k_MAX_PLAYERS; p++)
		{
			m_sent_requests_this_turn[p] = 0;
		}

		gevmanager_Get()->AddEvent(GEV_INSERT_Tail,
		                       GEV_WormholeTurn,
		                       GEA_Player, m_owner,
		                       GEA_End);

		gevmanager_Get()->AddEvent(GEV_INSERT_Tail,
		                       GEV_PlayerPatience,
		                       GEA_Player, m_owner,
		                       GEA_End);

		gevmanager_Get()->AddEvent(GEV_INSERT_Tail,
		                       GEV_PeaceMovement,
		                       GEA_Player, m_owner,
		                       GEA_End);

		m_gold->ClearStats();

		gevmanager_Get()->AddEvent(GEV_INSERT_Tail,
		                       GEV_PollutionTurn,
		                       GEA_Player, m_owner,
		                       GEA_End);

		gevmanager_Get()->AddEvent(GEV_INSERT_Tail,
		                       GEV_BeginTurnAllCities,
		                       GEA_Player, m_owner,
		                       GEA_End);

		sint32 n = m_all_cities->Num();
		for(i = 0; i < n; i++)
		{
			gevmanager_Get()->AddEvent(GEV_INSERT_Tail,
			                       GEV_CityTurnPreProduction,
			                       GEA_City, m_all_cities->Access(i),
			                       GEA_End);
		}

		gevmanager_Get()->AddEvent(GEV_INSERT_Tail,
		                       GEV_BeginTurnProduction,
		                       GEA_Player, m_owner,
		                       GEA_End);

		for(i = 0; i < n; i++)
		{
			gevmanager_Get()->AddEvent(GEV_INSERT_Tail,
			                       GEV_CityBeginTurn,
			                       GEA_City, m_all_cities->Access(i),
			                       GEA_End);
		}

		gevmanager_Get()->AddEvent(GEV_INSERT_Tail,
		                       GEV_BeginTurnSupport,
		                       GEA_Player, m_owner,
		                       GEA_End);

		gevmanager_Get()->AddEvent(GEV_INSERT_Tail,
		                       GEV_BeginTurnImprovements,
		                       GEA_Player, m_owner,
		                       GEA_End);

		BeginTurnEnemyUnits();

		gevmanager_Get()->AddEvent(GEV_INSERT_Tail,
		                       GEV_BeginTurnAgreements,
		                       GEA_Player, m_owner,
		                       GEA_End);

		gevmanager_Get()->AddEvent(GEV_INSERT_Tail,
		                       GEV_ResetAllMovement,
		                       GEA_Player, m_owner,
		                       GEA_End);

		gevmanager_Get()->AddEvent(GEV_INSERT_Tail,
		                       GEV_AttemptRevolt,
		                       GEA_Player, m_owner,
		                       GEA_End);

		gevmanager_Get()->AddEvent(GEV_INSERT_Tail,
		                       GEV_BeginTurnEndGame,
		                       GEA_Player, m_owner,
		                       GEA_End);

		BeginTurnUnits();

		gevmanager_Get()->AddEvent(GEV_INSERT_Tail,
		                       GEV_BeginTurnGovernment,
		                       GEA_Player, m_owner,
		                       GEA_End);

		m_strengths->Calculate();
		if(g_network.IsHost())
		{
			g_network.Block(m_owner);
			g_network.QueuePacketToAll(new NetStrengths(m_owner));
			g_network.QueuePacketToAll(new NetScores(m_owner));
			g_network.Unblock(m_owner);
		}
	}
	else
	{
		DPRINTF(k_DBG_GAMESTATE, ("Player[%d]::BeginTurn: not running\n", m_owner));
	}

	if(!g_network.IsClient())
	{
		gevmanager_Get()->AddEvent(GEV_INSERT_Tail,
		                       GEV_FinishBeginTurn,
		                       GEA_Player, m_owner,
		                       GEA_End);
	}

	pollution_Get()->BeginTurn();

	g_player[m_owner]->PreResourceCalculation();
}
