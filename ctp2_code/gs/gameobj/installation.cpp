#include "ctp/c3.h"
#include "gs/gameobj/installation.h"
#include "gs/gameobj/installationpool.h"

#include "gs/gameobj/Player.h"
#include "gs/utility/safety.h"          // safe_player
#include "gs/gameobj/XY_Coordinates.h"
#include "gs/world/World.h"

#include "net/general/network.h"
#include "net/general/net_info.h"
#include "gs/core/player_view.h"
#include "gs/core/render_observer.h"
#include "gs/gameobj/terrainutil.h"

void
Installation::KillInstallation()
{
	Installation tmp(*this);
	tmp.RemoveAllReferences();
}

void
Installation::RemoveAllReferences()
{
	MapPoint pos;
	GetPos(pos);

	if(Player *owner = safe_player(GetOwner())) {
		owner->RemoveInstallationReferences(*this);
	}
	world_Get()->RemoveInstallation(*this, pos);
	if(Player *owner = safe_player(GetOwner())) {

		double myVisionRange = terrainutil_GetVisionRange(GetType(), RetPos());
		if(myVisionRange > 0) {
			owner->RemoveUnitVision(pos, myVisionRange);
			if(GetOwner() == player_view::VisiblePlayer()) {
				render_observer::AddCopyVision();
			}
		}
	}

	if(network_Get().IsHost()) {
		network_Get().Enqueue(new NetInfo(NET_INFO_CODE_KILL_INSTALLATION,
									  uint32(*this)));
	}
	installationpool_Get()->Del(*this);
}

const InstallationData *Installation::GetData() const
{
	return installationpool_Get()->GetInstallation(*this);
}

InstallationData *Installation::AccessData()
{
	return installationpool_Get()->AccessInstallation(*this);
}

const TerrainImprovementRecord *Installation::GetDBRec() const
{
	return g_theTerrainImprovementDB->Get(GetData()->GetType());
}

void Installation::UseAirfield(sint32 currentRound)
{
	AccessData()->UseAirfield(currentRound);
}

sint32 Installation::AirfieldLastUsed() const
{
	return GetData()->AirfieldLastUsed();
}

void Installation::ChangeOwner(sint32 toOwner)
{
	AccessData()->ChangeOwner(toOwner);
}
