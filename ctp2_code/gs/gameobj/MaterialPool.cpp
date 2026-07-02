//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ source
// Description  : The material pool
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
// - Replaced old const database by new one. (5-Aug-2007 Martin G�hmann)
//
//----------------------------------------------------------------------------

#include "ctp/c3.h"
#include "gs/gameobj/MaterialPool.h"
#include "net/general/network.h"
#include "net/general/net_action.h"
#include "net/general/net_info.h"
#include "gs/gameobj/Player.h"
#include "gs/utility/safety.h"          // safe_player
#include "ConstRecord.h"

extern BOOL g_powerPointsMode;

void MaterialPool::AddMaterials(sint32 amt)
{
	if(m_level > 0 && amt > 0 && (m_level + amt) < 0) {
		m_level = 0x7fffffff;
	} else {
		m_level += amt;
	}
}

void MaterialPool::CheatAddMaterials(sint32 amt)
{
	if((network_Get().IsActive() && network_Get().SetupMode()) || g_powerPointsMode) {
		Player * owner = safe_player(m_owner);
		if(!owner)
			return;
		sint32 pointCost = sint32(double(amt) * g_theConstDB->Get(0)->GetPowerPointsToMaterials());
		if(owner->GetPoints() < pointCost)
			return;
		owner->DeductPoints(pointCost);

		if(network_Get().IsHost()) {
			network_Get().Enqueue(new NetInfo(NET_INFO_CODE_POWER_POINTS,
										  m_owner, owner->GetPoints()));
		}
	}
	if(network_Get().IsClient()) {
		network_Get().SendAction(new NetAction(NET_ACTION_CHEAT_ADD_MATERIALS,
										   amt));
	}
	AddMaterials(amt);
}

sint32 MaterialPool::CheatSubtractMaterials(sint32 amt)
{
	if((network_Get().IsActive() && network_Get().SetupMode()) | g_powerPointsMode) {
		if(Player * owner = safe_player(m_owner)) {
			sint32 pointCost = sint32(double(amt) * g_theConstDB->Get(0)->GetPowerPointsToMaterials());
			owner->AddPoints(pointCost);

			if(network_Get().IsHost()) {
				network_Get().Enqueue(new NetInfo(NET_INFO_CODE_POWER_POINTS,
											  m_owner, owner->GetPoints()));
			}
		}
	}
	if(network_Get().IsClient()) {
		network_Get().SendAction(new NetAction(NET_ACTION_CHEAT_SUB_MATERIALS,
										   amt));
	}
	SubtractMaterials(amt);
	return amt;
}
