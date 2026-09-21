//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ source
// Description  : Player strength history
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
// - Crash prevented.
// - Duplicate code removed.
//
//----------------------------------------------------------------------------

#include "ctp/c3.h"
#include "gs/utility/safety.h"
#include "gs/gameobj/Strengths.h"
#include "gs/gameobj/player.h"
#include "gs/utility/UnitDynArr.h"
#include "BuildingRecord.h"
#include "WonderRecord.h"
#include "gs/gameobj/Unit.h"
#include "gs/gameobj/UnitData.h"
#include "gs/gameobj/citydata.h"
#include "gs/gameobj/Gold.h"
#include "UnitRecord.h"
#include "gs/gameobj/wonderutil.h"
#include "gs/utility/TurnCnt.h"
#include "gs/gameobj/buildingutil.h"

Strengths::Strengths(sint32 owner)
{
	m_owner = owner;
 
	sint32 const curRound = turn_Get() ? turn_Get()->GetSessionRound() : 0;
 
	if (curRound <= 0)
		return;
 
	sint32 c;
	sint32 y;
	for(y = 1; y < curRound; y++) {
		for(c = 0; c < sint32(STRENGTH_CAT_MAX); c++) {
			m_strengthRecords[c].Insert(0);
		}
	}
}

Strengths::~Strengths()
= default;

void Strengths::Calculate()
{
	for(sint32 i = STRENGTH_CAT_NONE + 1; i < STRENGTH_CAT_MAX; i++)
	{
		switch(i)
		{
			case STRENGTH_CAT_KNOWLEDGE:
				m_strengthRecords[i].Insert(player_Get(m_owner)->GetKnowledgeStrength());
				break;
			case STRENGTH_CAT_MILITARY:
				m_strengthRecords[i].Insert(player_Get(m_owner)->GetMilitaryStrength());
				break;
			case STRENGTH_CAT_POLLUTION:
				m_strengthRecords[i].Insert(player_Get(m_owner)->GetCurrentPollution()) ;
				break;
			case STRENGTH_CAT_TRADE:
				m_strengthRecords[i].Insert(player_Get(m_owner)->GetTradeStrength());
				break;
			case STRENGTH_CAT_GOLD:
				m_strengthRecords[i].Insert(player_Get(m_owner)->m_gold->GetIncome());
				break;
			case STRENGTH_CAT_POPULATION:
				m_strengthRecords[i].Insert(player_Get(m_owner)->GetTotalPopulation());
				break;
			case STRENGTH_CAT_CITIES:
				m_strengthRecords[i].Insert(player_Get(m_owner)->m_all_cities->Num());
				break;
			case STRENGTH_CAT_GEOGRAPHICAL:
				m_strengthRecords[i].Insert(player_Get(m_owner)->GetLandArea());
				break;
			case STRENGTH_CAT_SPACE:
				m_strengthRecords[i].Insert(player_Get(m_owner)->GetSpaceStrength());
				break;
			case STRENGTH_CAT_UNDERSEA:
				m_strengthRecords[i].Insert(player_Get(m_owner)->GetUnderseaStrength());
				break;

			case STRENGTH_CAT_UNITS:
				m_strengthRecords[i].Insert(GetTotalUnitCost());
				break;
			case STRENGTH_CAT_BUILDINGS:
				m_strengthRecords[i].Insert(GetTotalBuildingCost());
				break;
			case STRENGTH_CAT_WONDERS:
				m_strengthRecords[i].Insert(GetTotalWonderCost());
				break;
			case STRENGTH_CAT_PRODUCTION:
				m_strengthRecords[i].Insert(GetTotalProduction());
				break;
			default:
				Assert(FALSE);
				break;
		}
	}
}

sint32 Strengths::GetStrength(STRENGTH_CAT category) const
{
	return m_strengthRecords[category].Num()
		   ? m_strengthRecords[category].GetLast()
		   : 0;
}

sint32 Strengths::GetTurnStrength(STRENGTH_CAT category, sint32 turn) const
{
	if (!m_strengthRecords[category].Num()) return 0;


	if (turn < 0) turn = m_strengthRecords[category].Num() -1;

	if (turn >= m_strengthRecords[category].Num()) return 0;

	return m_strengthRecords[category][turn];
}

sint32 Strengths::GetTotalUnitCost() const
{
	sint32 i;
	sint32 c = 0;
	UnitDynamicArray *units = player_Get(m_owner)->m_all_units.get();
	for(i = units->Num() - 1; i >= 0; i--) {
		c += units->Access(i).GetDBRec()->GetShieldCost();
	}
	return c;
}

sint32 Strengths::GetTotalBuildingCost() const
{
	sint32 i;
	sint32 j;
	sint32 c = 0;
	UnitDynamicArray *cities = player_Get(m_owner)->m_all_cities.get();
	for(i = cities->Num() - 1; i >= 0; i--) {
		uint64 builtImprovements = cities->Access(i).GetImprovements();
		for(j = g_theBuildingDB->NumRecords() - 1; j >= 0; j--) {
			if(builtImprovements & (safe_shift_left_u64(j))) {
				c += buildingutil_Get(j, m_owner)->GetProductionCost();
			}
		}
	}
	return c;
}

sint32 Strengths::GetTotalWonderCost() const
{
	sint32 i;
	sint32 c = 0;
	for(i = g_theWonderDB->NumRecords() - 1; i >= 0; i--) {
		if(player_Get(m_owner)->m_builtWonders & (uint64(1) << uint64(i))) {
			c += wonderutil_Get(i, m_owner)->GetProductionCost();
		}
	}
	return c;
}

sint32 Strengths::GetTotalProduction() const
{
	sint32 i;
	sint32 c = 0;
	UnitDynamicArray *cities = player_Get(m_owner)->m_all_cities.get();
	for(i = cities->Num() - 1; i >= 0; i--) {
		c += cities->Access(i).GetData()->GetCityData()->GetNetCityProduction();
	}
	return c;
}
