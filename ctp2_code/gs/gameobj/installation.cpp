#include "ctp/c3.h"
#include "gs/gameobj/installation.h"
#include "gs/gameobj/installationpool.h"

#include "gs/gameobj/Player.h"
#include "gs/gameobj/XY_Coordinates.h"
#include "gs/world/World.h"

#include "net/general/network.h"
#include "net/general/net_info.h"
#include "gs/core/player_view.h"
#include "gs/core/render_observer.h"
#include "gs/gameobj/terrainutil.h"

extern Player **g_player;
extern World *g_theWorld;

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

	if(GetOwner() >= 0 && g_player[GetOwner()]) {
		g_player[GetOwner()]->RemoveInstallationReferences(*this);
	}
	g_theWorld->RemoveInstallation(*this, pos);
	if(GetOwner() >= 0 && g_player[GetOwner()]) {

		double myVisionRange = terrainutil_GetVisionRange(GetType(), RetPos());
		if(myVisionRange > 0) {
			g_player[GetOwner()]->RemoveUnitVision(pos, myVisionRange);
			if(GetOwner() == player_view::VisiblePlayer()) {
				render_observer::AddCopyVision();
			}
		}
	}

	if(g_network.IsHost()) {
		g_network.Enqueue(new NetInfo(NET_INFO_CODE_KILL_INSTALLATION,
									  uint32(*this)));
	}
	g_theInstallationPool->Del(*this);
}

const InstallationData *Installation::GetData() const
{
	return g_theInstallationPool->GetInstallation(*this);
}

InstallationData *Installation::AccessData()
{
	return g_theInstallationPool->AccessInstallation(*this);
}

const TerrainImprovementRecord *Installation::GetDBRec() const
{
	return g_theTerrainImprovementDB->Get(GetData()->GetType());
}

void Installation::UseAirfield()
{
	AccessData()->UseAirfield();
}

sint32 Installation::AirfieldLastUsed() const
{
	return GetData()->AirfieldLastUsed();
}

void Installation::ChangeOwner(sint32 toOwner)
{
	AccessData()->ChangeOwner(toOwner);
}
