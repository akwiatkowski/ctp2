//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ source
// Description  : Civilisation handling.
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
// - Recycle civilisation indices to prevent a game crash.
// - Corrected civilisation index for MP new player creation.
//
//----------------------------------------------------------------------------

#include "ctp/c3.h"
#include "gs/gameobj/Civilisation.h"

#include "gs/gameobj/Player.h"			    // player_arr_Get()
#include "CivilisationRecord.h"
#include "gs/gameobj/CivilisationPool.h"	// civilisationpool_Get()
#include "net/general/network.h"
#include "gs/core/player_view.h"
#include "gs/outcom/AICause.h"
#include "gs/database/profileDB.h"			// profiledb_Get()
#include "net/general/net_player.h"
#include "AdvanceRecord.h"
#include "gs/world/World.h"			    // world_Get()
#include "net/general/net_vision.h"
#include "ai/ctpai.h"


void Civilisation::KillCivilisation()
{
	Civilisation	tmp(*this);
	tmp.RemoveAllReferences();
}









void Civilisation::RemoveAllReferences()
{
	civilisationpool_Get()->Release(GetCivilisation());
	civilisationpool_Get()->Del(*this);
}









const CivilisationData* Civilisation::GetData() const
{
	return (civilisationpool_Get()->GetData(*this));
}









CivilisationData* Civilisation::AccessData() const
{
	return (civilisationpool_Get()->AccessData(*this));
}

PLAYER_INDEX civilisation_NewCivilisationOrVandals(PLAYER_INDEX old_owner)
{
	PLAYER_INDEX pi = PLAYER_INDEX_INVALID;
	sint32 i;
	sint32 maxPlayers = profiledb_Get()->GetMaxPlayers();
	if(maxPlayers <= 0) {
		maxPlayers = profiledb_Get()->GetNPlayers();
	}

	sint32 count = 0;
	for(i = 0; i < k_MAX_PLAYERS; i++) {
		if(player_Get(i))
			count++;
	}
	if(count < maxPlayers) {
		for(i = 1; i < k_MAX_PLAYERS; i++) {
			if(!player_Get(i)) {
				pi = i;
				break;
			}
		}
		if(pi != PLAYER_INDEX_INVALID) {
			civilisation_CreateNewPlayer(pi, old_owner);
		}
	}

	if (pi == PLAYER_INDEX_INVALID)
	{
		// Maximum number of players reached: add as Barbarians.
		pi = PLAYER_INDEX_VANDALS;
	}

	return pi;
}

//----------------------------------------------------------------------------
//
// Name       : civilisation_CreateNewPlayer
//
// Description: Create a new (AI) player
//
// Parameters : pi          : player index
//              old_owner   : player index of "parent" civilisation
//
// Globals    : player_arr_Get()    : player data
//              g_network   : Multiplayer data
//
// Returns    : -
//
// Remark(s)  : When a parent civilisation is provided, the new civilisation
//              inherits the advances and map of its parent.
//              Assumption  : the new player index is valid and "free".
//
//----------------------------------------------------------------------------
void civilisation_CreateNewPlayer(sint32 pi, sint32 old_owner)
{
	player_arr_Get()[pi] = new Player
	    (PLAYER_INDEX(pi), 0, PLAYER_TYPE_ROBOT, CIV_INDEX_RANDOM, GENDER_RANDOM);

	if (network_Get().IsActive())
	{
		network_Get().AddCivilization
		    (pi, PLAYER_TYPE_ROBOT, player_Get(pi)->GetCivilisation()->GetCivilisation());
	}

	player_view::AddPlayer(pi);

	if (pi != PLAYER_INDEX_VANDALS && 			// Barbarians do not inherit
	    (old_owner >= 0) && player_Get(old_owner)
	   )
	{
	    delete player_Get(pi)->m_advances;
		player_Get(pi)->m_advances = new Advances(*(player_Get(old_owner)->m_advances));
		player_Get(pi)->m_advances->SetOwner(pi);
		player_Get(old_owner)->GiveMap(pi);
	}

	CtpAi::AddPlayer(pi);

	if (network_Get().IsHost())
	{
		network_Get().Block(old_owner);
		network_Get().QueuePacketToAll(new NetPlayer(player_Get(pi)));

		for (uint16 y = 0; y < world_Get()->GetYHeight(); y += k_VISION_STEP)
		{
			network_Get().QueuePacketToAll(new NetVision(pi, y, k_VISION_STEP));
		}

		network_Get().Unblock(old_owner);
	}
}

const CivilisationRecord *Civilisation::GetDBRec() const
{
	return g_theCivilisationDB->Get(GetCivilisation());
}
