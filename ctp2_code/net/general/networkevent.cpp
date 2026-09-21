//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ source
// Description  : Network events
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
// - None
//
//----------------------------------------------------------------------------

#include "ctp/c3.h"
#include <memory>


#include "net/general/network.h"
#include "net/general/net_diplomacy.h"
#include "net/general/net_action.h"

#include "gs/gameobj/Events.h"
#include "gs/events/GameEventUser.h"
#include "net/general/net_info.h"
#include "gs/gameobj/player.h"
#include "gs/gameobj/citydata.h"
#include "gs/gameobj/UnitData.h"

#include "gs/diplomacy/diplomacy_types.h"
#include "ai/diplomacy/Diplomat.h"

#include "ai/ctpai.h"

#include "gs/utility/newturncount.h"
#include "gfx/spritesys/director.h"

#include "net/general/net_city.h"
#include "net/general/net_happy.h"
#include "gs/events/GameEventManager.h"

STDEHANDLER(NetBeginTurnEvent)
{
	sint32 pl;
	if(!args->GetPlayer(0, pl)) return GEV_HD_Continue;





	if(network_Get().IsActive()) {
		if(pl == network_Get().GetPlayerIndex())
			network_Get().SetMyTurn(TRUE);
		else
			network_Get().SetMyTurn(FALSE);

		Diplomat::GetDiplomat(network_Get().GetPlayerIndex()).ClearInitiatives();
	}

	if(network_Get().IsClient() && network_Get().IsLocalPlayer(pl)) {
		network_Get().SendAction(std::make_unique<NetAction>(NET_ACTION_ACK_BEGIN_TURN).release());
	}

	return GEV_HD_Continue;
}

STDEHANDLER(NetTurnSyncEvent)
{
	sint32 pl;
	if(!args->GetPlayer(0, pl)) return GEV_HD_Continue;














	return GEV_HD_Continue;
}

STDEHANDLER(NetStartMovePhaseEvent)
{
	sint32 pl;
	if(!args->GetPlayer(0, pl)) return GEV_HD_Continue;

	if(network_Get().IsHost()) {

		PointerList<Packetizer> cityPackets;
		PointerList<Packetizer> buildQueuePackets;
		sint32 p;
		for(p = 0; p < k_MAX_PLAYERS; p++) {

			if(!player_Get(p))
				continue;

			if(p == network_Get().GetPlayerIndex())
				continue;

			if(!player_Get(p)->IsNetwork())
				continue;

			uint16 id = network_Get().IndexToId(p);
			if(id != 0xffff) {

				sint32 i;
				for(i = 0; i < player_Get(pl)->m_all_cities->Num(); i++) {
					UnitData *ud = player_Get(pl)->m_all_cities->Access(i).AccessData();
					CityData *cd = ud->GetCityData();

					cityPackets.AddTail(std::make_unique<NetCity>(ud, FALSE).release());
					cityPackets.AddTail(std::make_unique<NetCity2>(cd, FALSE).release());
					cityPackets.AddTail(std::make_unique<NetHappy>(player_Get(pl)->m_all_cities->Access(i), cd->GetHappy(), FALSE).release());

					buildQueuePackets.AddTail(std::make_unique<NetCityBuildQueue>(cd).release());


				}

				network_Get().ChunkList(id, &cityPackets);
				if(p != pl) {

					network_Get().ChunkList(id, &buildQueuePackets);
				}
			}
		}

		network_Get().Enqueue(std::make_unique<NetInfo>(NET_INFO_CODE_CITIES_DONE, pl).release());
	}

	return GEV_HD_Continue;
}

STDEHANDLER(NetAIFinishBeginTurnEvent)
{
	sint32 pl;
	if(!args->GetPlayer(0, pl)) return GEV_HD_Continue;




	if(network_Get().IsHost() && !network_Get().IsLocalPlayer(pl)) {
		network_Get().Enqueue(std::make_unique<NetInfo>(NET_INFO_CODE_FINISH_AI_TURN, pl).release());
	} else if(network_Get().IsClient() && network_Get().IsLocalPlayer(pl) && player_Get(pl)->IsRobot()) {

	}





	return GEV_HD_Continue;
}

STDEHANDLER(NetNewProposalEvent)
{
	if(!network_Get().IsActive()) return GEV_HD_Continue;

	sint32 sender;
	sint32 receiver;
	if(!args->GetPlayer(0, sender)) return GEV_HD_Continue;
	if(!args->GetPlayer(1, receiver)) return GEV_HD_Continue;

	NewProposal prop = Diplomat::GetDiplomat(sender).GetMyLastNewProposal(receiver);
	if(prop == Diplomat::s_badNewProposal) return GEV_HD_Continue;

	if(network_Get().IsHost()) {
		network_Get().Block(prop.senderId);
		network_Get().QueuePacketToAll(std::make_unique<NetDipProposal>(prop).release());
		network_Get().Unblock(prop.senderId);
	} else if(network_Get().IsLocalPlayer(sender)) {
		network_Get().SendToServer(std::make_unique<NetDipProposal>(prop).release());
	}

	return GEV_HD_Continue;
}

STDEHANDLER(NetResponseEvent)
{
	return GEV_HD_Continue;
}

STDEHANDLER(NetEndAIClientTurnEvent)
{
	if(!network_Get().IsClient()) return GEV_HD_Stop;

	sint32 p;
	if(!args->GetPlayer(0, p)) return GEV_HD_Continue;

	Assert(network_Get().IsLocalPlayer(p));
	if(!network_Get().IsLocalPlayer(p))
		return GEV_HD_Continue;

	if(player_Get(p)->IsRobot()) {
		DPRINTF(k_DBG_NET, ("NetEndAIClientTurnEvent, %d\n", p));
		director_Get()->AddEndTurn();

	}
	return GEV_HD_Continue;
}

void networkevent_Initialize()
{
	gevmanager_Get()->AddCallback(GEV_BeginTurn, GEV_PRI_Pre, &s_NetBeginTurnEvent);
	gevmanager_Get()->AddCallback(GEV_NetworkTurnSync, GEV_PRI_Primary, &s_NetTurnSyncEvent);

	gevmanager_Get()->AddCallback(GEV_NewProposal, GEV_PRI_Primary, &s_NetNewProposalEvent);
	gevmanager_Get()->AddCallback(GEV_ResponseReady, GEV_PRI_Primary, &s_NetResponseEvent);

	gevmanager_Get()->AddCallback(GEV_StartMovePhase, GEV_PRI_Pre, &s_NetStartMovePhaseEvent);

	gevmanager_Get()->AddCallback(GEV_AIFinishBeginTurn, GEV_PRI_Post, &s_NetAIFinishBeginTurnEvent);

	gevmanager_Get()->AddCallback(GEV_EndAIClientTurn, GEV_PRI_Primary, &s_NetEndAIClientTurnEvent);

}

void networkevent_Cleanup()
{
}
