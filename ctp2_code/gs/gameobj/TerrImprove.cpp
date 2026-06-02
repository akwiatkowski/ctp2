#include "ctp/c3.h"
#include "gs/gameobj/TerrImprove.h"

#include "net/general/net_info.h"             // NetInfo
#include "net/general/network.h"              // g_network
#include "gs/gameobj/Player.h"               // player_Get
#include "gs/core/player_view.h"
#include "gs/gameobj/TerrImprovePool.h"      // terrimprovepool_Get()
#include "gs/world/World.h"                // world_Get()

void TerrainImprovement::KillImprovement()
{
	TerrainImprovement tmp(*this);
	tmp.RemoveAllReferences();
}

void TerrainImprovement::RemoveAllReferences()
{
	player_Get(GetOwner())->RemoveImprovementReferences(*this);
	world_Get()->RemoveImprovement(*this, GetLocation());
	if(g_network.IsHost()) {
		if(player_view::CurPlayer() == GetOwner())
			g_network.Block(GetOwner());
		g_network.Enqueue(new NetInfo(NET_INFO_CODE_KILL_IMPROVEMENT,
									  uint32(*this)));
		if(player_view::CurPlayer() == GetOwner())
			g_network.Unblock(GetOwner());
	}

	terrimprovepool_Get()->Del(*this);
}

const TerrainImprovementData *TerrainImprovement::GetData() const
{
	return terrimprovepool_Get()->GetTerrainImprovement(*this);
}

TerrainImprovementData *TerrainImprovement::AccessData() const
{
	return terrimprovepool_Get()->AccessTerrainImprovement(*this);
}

void TerrainImprovement::AddTurn()
{
	AccessData()->AddTurn(1);
}

bool TerrainImprovement::IsValid()
{
	return terrimprovepool_Get()->IsValid(*this);
}
