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
#include "gs/gameobj/player.h"
#include "gs/events/GameEventManager.h"
#include "gs/gameobj/PlayerEvent.h"
#include "gs/gameobj/Events.h"
#include "gs/gameobj/player.h"
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

	gameobservers_Get()->NotifyTurnStart(m_owner);
	gameobservers_Get()->NotifyUpdateControlPanel(m_owner);

	sint32 i;
	for(i = m_messages->Num() - 1; i >= 0; i--)
	{
		if(!messagepool_Get()->IsValid(m_messages->Access(i)))
		{
			m_messages->DelIndex(i);
			continue;
		}
	}

	gameobservers_Get()->NotifyUpdateMessages(m_owner);

	m_is_turn_over = FALSE;

	m_end_turn_soon = FALSE;

	if(network_Get().IsHost())
	{
		network_Get().Block(m_owner);
		network_Get().Enqueue(new NetInfo(NET_INFO_CODE_SET_ROUND, m_owner, m_current_round));
		network_Get().Unblock(m_owner);
	}

	DPRINTF(k_DBG_GAMESTATE, ("\n"));

	if(!network_Get().IsActive() || network_Get().IsHost() || (m_owner == network_Get().GetPlayerIndex()))
	{
		DPRINTF(k_DBG_GAMESTATE, ("Player[%d]::BeginTurn: running\n", m_owner));

		m_civRevoltingCitiesShouldJoin = -1;

		for(int & p : m_sent_requests_this_turn)
		{
			p = 0;
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
		if(network_Get().IsHost())
		{
			network_Get().Block(m_owner);
			network_Get().QueuePacketToAll(new NetStrengths(m_owner));
			network_Get().QueuePacketToAll(new NetScores(m_owner));
			network_Get().Unblock(m_owner);
		}
	}
	else
	{
		DPRINTF(k_DBG_GAMESTATE, ("Player[%d]::BeginTurn: not running\n", m_owner));
	}

	if(!network_Get().IsClient())
	{
		gevmanager_Get()->AddEvent(GEV_INSERT_Tail,
		                       GEV_FinishBeginTurn,
		                       GEA_Player, m_owner,
		                       GEA_End);
	}

	pollution_Get()->BeginTurn();

	player_Get(m_owner)->PreResourceCalculation();
}
