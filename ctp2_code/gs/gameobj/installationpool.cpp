#include "ctp/c3.h"
#include "gs/gameobj/installationpool.h"
#include "gs/gameobj/player.h"
#include "gs/gameobj/XY_Coordinates.h"
#include "gs/world/World.h"
#include "gs/utility/Globals.h"

InstallationPool::InstallationPool() : ObjPool(k_BIT_GAME_OBJ_TYPE_INSTALLATION)
{
}

Installation InstallationPool::Create(sint32 owner,
									  MapPoint &pnt,
									  sint32 type)
{
	InstallationData *newData;
	Installation newInstallation(NewKey(k_BIT_GAME_OBJ_TYPE_INSTALLATION));

	newData = new InstallationData(newInstallation, owner, pnt, type);

	Insert(newData);
	player_Get(owner)->AddInstallation(newInstallation);
	world_Get()->InsertInstallation(newInstallation, pnt);
	newData->DoVision();
	return newInstallation;
}

void InstallationPool::RebuildQuadTree()
{
	sint32 i;
	for(i = 0; i < k_OBJ_POOL_TABLE_SIZE; i++) {
		if(m_table[i])
			((InstallationData*)(m_table[i]))->RebuildQuadTree();
	}
}
