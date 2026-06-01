//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ source
// Description  : Wonder Tracker
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
#include "gs/gameobj/WonderTracker.h"
#include "gs/utility/safety.h"

#include "robot/aibackdoor/civarchive.h"
#include "gs/utility/Globals.h"        // k_GAME_OBJ_TYPE_WONDER
#include "gs/gameobj/Player.h"
#include "net/general/network.h"
#include "net/general/net_info.h"
#include "gs/gameobj/Unit.h"
#include "gs/utility/UnitDynArr.h"
#include "gs/gameobj/UnitData.h"
#include "gs/gameobj/citydata.h"
#include "gs/fileio/gamefile.h"        // save_file_version_Get
#include "WonderRecord.h"
#include "gs/gameobj/wonderutil.h"
#include "gs/gameobj/UnitPool.h"


// Storage + accessors moved to gs/utility/gameinit.cpp where the rest
// of the gs/ singletons live.

WonderTracker::WonderTracker()
:   m_builtWonders      (0),
//  m_buildingWonders
    m_globeSatFlags     (0)
{
	sint32 i;
	for(i=0; i<k_MAX_PLAYERS; i++)
		m_buildingWonders[i] = (uint64) 0x0;
}

void WonderTracker::Serialize(CivArchive &archive)
{
	if(archive.IsStoring()) {
		archive << m_builtWonders;
		archive.Store((uint8*)m_buildingWonders, k_MAX_PLAYERS * sizeof(uint64));
		archive << m_globeSatFlags;
	} else {
		archive >> m_builtWonders;

		if (save_file_version_Get() >= 58) {
			archive.Load((uint8*)m_buildingWonders, k_MAX_PLAYERS * sizeof(uint64));
			if(save_file_version_Get() >= 63) {
				archive >> m_globeSatFlags;
			} else {
				m_globeSatFlags = 0;
			}
		}
		else {

			sint32 tmp[k_MAX_PLAYERS];
			archive.Load((uint8*)tmp, k_MAX_PLAYERS * sizeof(sint32));
			for(int i=0; i<k_MAX_PLAYERS; i++)
				m_buildingWonders[i] = (uint64) 0x0;
		}
	}
}

bool WonderTracker::HasWonderBeenBuilt(sint32 which)
{
	return (m_builtWonders & safe_shift_left_u64(which)) != 0;
}

sint32 WonderTracker::WhoOwnsWonder(sint32 which)
{
	if(!HasWonderBeenBuilt(which))
		return -1;

	sint32 i;
	for(i = 0; i < k_MAX_PLAYERS; i++) {
		if(g_player[i]) {
			if(g_player[i]->m_builtWonders & safe_shift_left_u64(which)) {
				return i;
			}
		}
	}
	return -1;
}

void WonderTracker::AddBuilt(sint32 which)
{
	m_builtWonders |= safe_shift_left_u64(which);
	if(g_network.IsHost()) {
		g_network.Enqueue(new NetInfo(NET_INFO_CODE_BUILT_WONDERS,
									  (uint32)(m_builtWonders & 0xffffffff),
									  (uint32)(m_builtWonders >> (uint64)32)));
	}
}

void WonderTracker::SetBuiltWonders(uint64 built)
{
	m_builtWonders = built;
}

bool WonderTracker::GetCityWithWonder(sint32 which, Unit &city)
{
	sint32 p;
	for(p = 0; p < k_MAX_PLAYERS; p++) {
		if(!g_player[p])
			continue;
		sint32 c;
		for(c = g_player[p]->m_all_cities->Num() - 1; c >= 0; c--) {
			if(g_player[p]->m_all_cities->Access(c).GetData()->GetCityData()->GetBuiltWonders() & safe_shift_left_u64(which)) {

				city = g_player[p]->m_all_cities->Access(c);
				return true;
			}
		}
	}
	return false;
}
void WonderTracker::SetBuildingWonder(sint32 which, PLAYER_INDEX who)
{

	m_buildingWonders[who] |= safe_shift_left_u64(which);
}

void WonderTracker::ClearBuildingWonder(sint32 which, PLAYER_INDEX who)
{
	m_buildingWonders[who] &= ~safe_shift_left_u64(which);
}

bool WonderTracker::IsBuildingWonder(sint32 which, PLAYER_INDEX who)
{
	return (m_buildingWonders[who] & safe_shift_left_u64(which)) != 0;
}

void WonderTracker::RecomputeIsBuilding(const PLAYER_INDEX who)
{


	m_buildingWonders[who] = (uint64) 0x0;

	Unit city;
	sint32 category;
	sint32 num_cities = g_player[who]->m_all_cities->Num();
	for (sint32 i = 0; i < num_cities; i++)
	{
		city = g_player[who]->m_all_cities->Access(i);
		Assert( unitpool_Get()->IsValid(city) );
		Assert( city->GetCityData() != NULL );

		if(city.CD() && city.CD()->GetBuildQueue()->GetHead())
		{
			category = city.CD()->GetBuildQueue()->GetHead()->m_category;
			if (category == k_GAME_OBJ_TYPE_WONDER)
			{
				SetBuildingWonder(city.CD()->GetBuildQueue()->GetHead()->m_type, who);
			}
		}
	}
}
