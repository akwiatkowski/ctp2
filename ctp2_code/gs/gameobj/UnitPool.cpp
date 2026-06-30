//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ source
// Description  : The unit pool
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
//----------------------------------------------------------------------------
//
// Modifications from the original Activision code:
//
// - Made government modified work here
//
//----------------------------------------------------------------------------

#include "ctp/c3.h"
#include "gs/gameobj/UnitPool.h"

#include "ctp/ctp2_utils/civlog.h"
#include "gs/core/game_observer.h"

namespace {
auto unitpool_log = civlog::Get("unitpool");
}  // namespace

#include "gs/utility/Globals.h"
#include "gs/utility/gstypes.h"
#include "gs/newdb/UnitRec.h"
#include "gs/gameobj/UnitData.h"
#include "gs/database/StrDB.h"





#include "ctp/civ3_main.h"

UnitPool::UnitPool () : ObjPool (k_BIT_GAME_OBJ_TYPE_UNIT)
{
}

Unit UnitPool::Create (
    const sint32 t,
    const PLAYER_INDEX owner,
    const MapPoint &pos,
    const Unit hc,
    std::shared_ptr<UnitActor> actor)
{
	UnitData *ptr = nullptr;
	Unit id(NewKey(k_BIT_GAME_OBJ_TYPE_UNIT));

	Assert(owner < PLAYER_INDEX_INVALID);

	sint32 trans_t = 0;
	g_theUnitDB->Get(t, player_Get(owner)->GetGovernmentType())->GetTransType(trans_t);
	ptr = new UnitData(t, trans_t, id, owner, pos, hc, actor);

	Assert(ptr);

	Insert(ptr);
	// Phase 3 slice 7a: fire spawn event AFTER pool insertion so
	// observers' `Unit::GetActor()` / pool lookups succeed.
	if (gameobservers_Get()) gameobservers_Get()->NotifyUnitSpawned(id, ptr->GetState());
	return id;
}


Unit UnitPool::Create (
    const sint32 t,
    const PLAYER_INDEX owner,
    const MapPoint &actor_pos)
{
	UnitData *ptr = nullptr;
	Unit id(NewKey(k_BIT_GAME_OBJ_TYPE_UNIT));

	Assert(owner < PLAYER_INDEX_INVALID);

	sint32 trans_t = 0;
	g_theUnitDB->Get(t, player_Get(owner)->GetGovernmentType())->GetTransType(trans_t);
	ptr = new UnitData(t, trans_t, id, owner, actor_pos);

	Assert(ptr);

	Insert(ptr);
	// Phase 3 slice 7a: fire spawn event AFTER pool insertion (see above).
	if (gameobservers_Get()) gameobservers_Get()->NotifyUnitSpawned(id, ptr->GetState());
	return id;
}

void UnitPool::RebuildQuadTree()
{
	sint32 i;
	for(i = 0; i < k_OBJ_POOL_TABLE_SIZE; i++)
	{
		if(m_table[i])
		{
			((UnitData *)(m_table[i]))->RebuildQuadTree();
		}
	}
}

void UnitPool::RecreateActors()
{
	sint32 count = 0, city_count = 0;
	for (sint32 i = 0; i < k_OBJ_POOL_TABLE_SIZE; ++i)
	{
		if (m_table[i])
		{
			((UnitData *)(m_table[i]))->RecreateGfxState();
			count++;
			if (((UnitData *)(m_table[i]))->GetCityData())
				city_count++;
		}
	}
	unitpool_log->debug("RecreateActors: {} units, {} cities", count, city_count);
}

uint32 UnitPool_UnitPool_GetVersion()
{
	return (k_UNITPOOL_VERSION_MAJOR<<16 | k_UNITPOOL_VERSION_MINOR);
}

const UnitRecord *UnitPool::GetDBRec(const Unit id) const
{
	Player *    player  = player_Get(id.GetOwner());

	if(player)
	{
		return g_theUnitDB->Get(id.GetType(), player->GetGovernmentType());
	}
	else
	{
		return g_theUnitDB->Get(id.GetType());
	}
}
