//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ source
// Description  : Feat tracking
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
// - Memory leak repaired.
// - Propagate feat accomplishments.
// - Initialized local variables. (Sep 9th 2005 Martin GÃ¯Â¿Â½hmann)
// - Added HasFeat to check if a feat has been achieved by E 5-11-2006
//
//----------------------------------------------------------------------------

#include <memory>
#include "ctp/c3.h"
#include "gs/gameobj/FeatTracker.h"
#include "gs/utility/safety.h"
#include "gs/utility/TurnCnt.h"
#include "ctp/ctp2_utils/pointerlist.h"
#include "FeatRecord.h"
#include "gs/gameobj/player.h"
#include "AdvanceRecord.h"
#include "gs/gameobj/WonderTracker.h"
#include "gs/slic/SlicEngine.h"
#include "gs/slic/SlicObject.h"
#include "BuildingRecord.h"
#include "gs/utility/UnitDynArr.h"
#include "gs/gameobj/citydata.h"

#include "gs/gameobj/Events.h"
#include "gs/events/GameEventUser.h"
#include "gs/events/GameEventManager.h"
#include "gs/gameobj/EventTracker.h"
#include "gs/gameobj/Score.h"

#include "net/general/net_info.h"
#include "net/general/network.h"

// g_featTracker is defined in gameinit.cpp (where the lifecycle lives);
// this TU reaches it via feattracker_Get() declared in FeatTracker.h.

//----------------------------------------------------------------------------
//
// Name       : Feat::Feat
//
// Description: Constructor for an accomplished feat
//
// Parameters : type            : feat identifier
//              player          : player that accomplished the feat
//              round           : the round in which the feat was accomplished
//
// Globals    : NewTurnCount    : the current round (when not provided)
//
// Returns    : -
//
// Remark(s)  : The special round value USE_CURRENT_ROUND may be used to
//              indicate that the feat has been accomplished right now.
//
//----------------------------------------------------------------------------
Feat::Feat(sint32 type, sint32 player, sint32 round)
:	m_type		(type),
	m_player	(player)
{
	// USE_CURRENT_ROUND is a sentinel that says "look up the round now."
	// The previous implementation routed through NewTurnCount which was
	// internally null-safe; preserve the same behaviour explicitly so
	// constructing a Feat without a live turn_Get() (e.g. in json_save tests)
	// doesn't crash.
	m_round = (USE_CURRENT_ROUND == round)
	          ? (turn_Get() ? turn_Get()->GetSessionRound() : 0)
	          : round;
}


Feat::~Feat()
= default;


FeatTracker::FeatTracker()
{
	sint32 i;
	for(i = FEAT_EFFECT_NONE + 1; i < FEAT_EFFECT_MAX; i++)
	{
		// m_effectList is std::array<unique_ptr> - default null
	}

	m_achieved.assign(g_theFeatDB->NumRecords(), 0);

	m_buildingFeat.assign(g_theBuildingDB->NumRecords(), 0);

	FindBuildingFeats();
}


FeatTracker::~FeatTracker()
{
	// DeleteAll frees the Feats; the list frees its own nodes.
	m_activeList.DeleteAll();

	// m_effectList is std::array<unique_ptr> - frees itself

	// m_achieved and m_buildingFeat are std::vector<bool>, auto-freed
}


#define CHECK_FEAT_LIST(func, eff) \
if(rec->func()) { \
    if(!m_effectList[eff]) {\
        m_effectList[eff] = std::make_unique<PointerList<Feat>>();\
    }\
    m_effectList[eff]->AddTail(feat);\
}

void FeatTracker::AddFeatToEffectLists(Feat *feat)
{
	const FeatRecord *rec = g_theFeatDB->Get(feat->GetType());

	CHECK_FEAT_LIST(HasEffectBoatMovement,              FEAT_EFFECT_BOAT_MOVEMENT);
	CHECK_FEAT_LIST(HasEffectCityDefenseBonus,          FEAT_EFFECT_CITY_DEFENSE_BONUS);
	CHECK_FEAT_LIST(HasEffectReduceCityWalls,           FEAT_EFFECT_REDUCE_CITY_WALLS);
	CHECK_FEAT_LIST(HasEffectIncreaseCityVision,        FEAT_EFFECT_INCREASE_CITY_VISION);
	CHECK_FEAT_LIST(HasEffectIncreaseProduction,        FEAT_EFFECT_INCREASE_PRODUCTION);
	CHECK_FEAT_LIST(HasEffectIncreaseCommerce,          FEAT_EFFECT_INCREASE_COMMERCE);
	CHECK_FEAT_LIST(HasEffectIncreaseHappiness,         FEAT_EFFECT_INCREASE_HAPPINESS);
	CHECK_FEAT_LIST(HasEffectEliminateDistancePenalty,  FEAT_EFFECT_ELIMINATE_DISTANCE_PENALTY);
	CHECK_FEAT_LIST(HasEffectIncreaseBoatVision,        FEAT_EFFECT_INCREASE_BOAT_VISION);
	CHECK_FEAT_LIST(HasEffectIncreaseScience,           FEAT_EFFECT_INCREASE_SCIENCE);
	CHECK_FEAT_LIST(GetEffectGiveMaps,                  FEAT_EFFECT_GIVE_MAPS);
	CHECK_FEAT_LIST(HasEffectIncreaseHitPoints,         FEAT_EFFECT_INCREASE_HIT_POINTS);
	CHECK_FEAT_LIST(HasEffectScriptedTurn,              FEAT_EFFECT_SCRIPTED_TURN);
	CHECK_FEAT_LIST(HasEffectScriptedCity,              FEAT_EFFECT_SCRIPTED_CITY);
}

#define REMOVE_FROM_FEAT_LIST(func, eff) \
    if(m_effectList[eff] && rec->func()) {\
        PointerList<Feat>::PointerListNode *node = m_effectList[eff]->Find(feat);\
        if(node) {\
            m_effectList[eff]->Remove(node); \
        }\
    }

void FeatTracker::RemoveFeatFromEffectLists(Feat *feat)
{
	const FeatRecord *rec = g_theFeatDB->Get(feat->GetType());

	REMOVE_FROM_FEAT_LIST(HasEffectBoatMovement,                FEAT_EFFECT_BOAT_MOVEMENT);
	REMOVE_FROM_FEAT_LIST(HasEffectCityDefenseBonus,            FEAT_EFFECT_CITY_DEFENSE_BONUS);
	REMOVE_FROM_FEAT_LIST(HasEffectReduceCityWalls,             FEAT_EFFECT_REDUCE_CITY_WALLS);
	REMOVE_FROM_FEAT_LIST(HasEffectIncreaseCityVision,          FEAT_EFFECT_INCREASE_CITY_VISION);
	REMOVE_FROM_FEAT_LIST(HasEffectIncreaseProduction,          FEAT_EFFECT_INCREASE_PRODUCTION);
	REMOVE_FROM_FEAT_LIST(HasEffectIncreaseCommerce,            FEAT_EFFECT_INCREASE_COMMERCE);
	REMOVE_FROM_FEAT_LIST(HasEffectIncreaseHappiness,           FEAT_EFFECT_INCREASE_HAPPINESS);
	REMOVE_FROM_FEAT_LIST(HasEffectEliminateDistancePenalty,    FEAT_EFFECT_ELIMINATE_DISTANCE_PENALTY);
	REMOVE_FROM_FEAT_LIST(HasEffectIncreaseBoatVision,          FEAT_EFFECT_INCREASE_BOAT_VISION);
	REMOVE_FROM_FEAT_LIST(HasEffectIncreaseScience,             FEAT_EFFECT_INCREASE_SCIENCE);
	REMOVE_FROM_FEAT_LIST(GetEffectGiveMaps,                    FEAT_EFFECT_GIVE_MAPS);
	REMOVE_FROM_FEAT_LIST(HasEffectIncreaseHitPoints,           FEAT_EFFECT_INCREASE_HIT_POINTS);
	REMOVE_FROM_FEAT_LIST(HasEffectScriptedTurn,                FEAT_EFFECT_SCRIPTED_TURN);
	REMOVE_FROM_FEAT_LIST(HasEffectScriptedCity,                FEAT_EFFECT_SCRIPTED_CITY);
}

//----------------------------------------------------------------------------
//
// Name       : FeatTracker::AddFeat
//
// Description: Add an accomplished feat to the records
//
// Parameters : type            : feat identifier
//              player          : player that accomplished the feat
//              round           : the round in which the feat was accomplished
//
// Globals    : g_theFeatDB     : database of feat descriptions
//              TODO: add more globals
//
// Returns    : -
//
// Remark(s)  : The special round value USE_CURRENT_ROUND may be used to
//              indicate that the feat has been accomplished right now.
//
//----------------------------------------------------------------------------
void FeatTracker::AddFeat(sint32 type, sint32 player, sint32 round)
{
	const FeatRecord *rec = g_theFeatDB->Get(type);
	Assert(rec);
	if(!rec) return;
	if(type < 0 || type >= g_theFeatDB->NumRecords()) return;

	if(m_achieved[type])
	{
		return;
	}

	if(rec->GetNumExcludeAdvance() > 0)
	{
		for(sint32 a = 0; a < rec->GetNumExcludeAdvance(); a++)
		{
			for(sint32 p = 0; p < k_MAX_PLAYERS; p++)
			{
				if(player_Get(p) && safe_player(p)->HasAdvance(rec->GetExcludeAdvanceIndex(a)))
				{
					return;
				}
			}
		}
	}

	if(rec->GetNumExcludeWonder() > 0)
	{
		for(sint32 w = 0; w < rec->GetNumExcludeWonder(); w++)
		{
			if(wonder_tracker_Get()->HasWonderBeenBuilt(rec->GetExcludeWonderIndex(w)))
				return;
		}
	}

	if(rec->GetNumExcludeFeat() > 0)
	{
		for(sint32 f = 0; f < rec->GetNumExcludeFeat(); f++)
		{
			if(m_achieved[rec->GetExcludeFeatIndex(f)])
				return;
		}
	}

	const MBCHAR *slicFunc;
	if(rec->GetExcludeFunction(slicFunc))
	{
		if(slicengine_Get()->CallExcludeFunc(slicFunc, type, player))
			return;
	}

	m_achieved[type] = true;

	auto theFeat = std::make_unique<Feat>(type, player, round);
	m_activeList.AddTail(theFeat.get());
	theFeat.release();

	AddFeatToEffectLists(theFeat.get());

	const MBCHAR *slicMessage;
	std::unique_ptr<SlicObject> so;
	if(rec->GetSlicMessage(slicMessage))
	{
		so = std::make_unique<SlicObject>((char *)slicMessage);
	}
	else
	{
		so = std::make_unique<SlicObject>("MGenericFeatAccomplished");
	}

	so->AddPlayer(player);
	so->AddRecipient(player);
	so->AddInt(type);
	slicengine_Get()->Execute(std::move(so));

	if (Player* p = safe_player(player)) {
		p->m_score->AddFeat();
	}

	eventtracker_Get()->AddEvent(EVENT_TYPE_FEAT, player, theFeat->GetRound(), type);

	sint32 hpBonus;
	if(rec->GetEffectIncreaseHitPoints(hpBonus))
	{
		if(hpBonus > 0)
		{
			if (Player* p = safe_player(player)) {
				p->AddFeatHPBonus(hpBonus);
			}
		}
	}
}

void FeatTracker::AddFeat(const MBCHAR *name, sint32 player)
{
	sint32 featIndex;
	if(!g_theFeatDB->GetNamedItem(name, featIndex))
	{
		bool unknown_feat = false;
		Assert(unknown_feat);
		return;
	}

	gevmanager_Get()->AddEvent(GEV_INSERT_Tail, GEV_AccomplishFeat,
						   GEA_Int, featIndex,
						   GEA_Player, player,
						   GEA_End);
}

sint32 FeatTracker::GetEffect(FEAT_EFFECT effect, sint32 player, bool getTotal)
{
	if(!m_effectList[effect])
		return 0;

	sint32 result = 0;
	sint32 sub = 0;
	PointerList<Feat>::Walker walk(m_effectList[effect].get());
	while(walk.IsValid())
	{
		if(walk.GetObj()->GetPlayer() == player)
		{
			const FeatRecord *rec = g_theFeatDB->Get(walk.GetObj()->GetType());
			switch(effect)
			{
				case FEAT_EFFECT_BOAT_MOVEMENT:               rec->GetEffectBoatMovement(sub);break;
				case FEAT_EFFECT_CITY_DEFENSE_BONUS:          rec->GetEffectCityDefenseBonus(sub);break;
				case FEAT_EFFECT_REDUCE_CITY_WALLS:           rec->GetEffectReduceCityWalls(sub);break;
				case FEAT_EFFECT_INCREASE_CITY_VISION:        rec->GetEffectIncreaseCityVision(sub);break;
				case FEAT_EFFECT_INCREASE_PRODUCTION:         rec->GetEffectIncreaseProduction(sub);break;
				case FEAT_EFFECT_INCREASE_COMMERCE:           rec->GetEffectIncreaseCommerce(sub);break;
				case FEAT_EFFECT_INCREASE_HAPPINESS:          rec->GetEffectIncreaseHappiness(sub);break;
				case FEAT_EFFECT_ELIMINATE_DISTANCE_PENALTY:  rec->GetEffectEliminateDistancePenalty(sub);break;
				case FEAT_EFFECT_INCREASE_BOAT_VISION:        rec->GetEffectIncreaseBoatVision(sub);break;
				case FEAT_EFFECT_INCREASE_SCIENCE:            rec->GetEffectIncreaseScience(sub);break;

				case FEAT_EFFECT_INCREASE_HIT_POINTS:         rec->GetEffectIncreaseHitPoints(sub);break;
				//increase food
				//increase citylimit
				//increasepopulation
				//increase armysize
				//increase land move
				//increase land vision - C4ISR
				//increase airmove
				//increase attack for unit?
				//increase defense for unit?
				//increase move for unit?
				//increase vision for unit?
				//add other civ4 like promotions - but unit wide

				default:
					Assert(FALSE);
					break;
			}

			if(getTotal)
				result += sub;
			else
				result = std::max(sub, result);
		}

		walk.Next();
	}

	return result;
}

sint32 FeatTracker::GetAdditiveEffect(FEAT_EFFECT effect, sint32 player)
{
	return GetEffect(effect, player, true);
}

sint32 FeatTracker::GetMaxEffect(FEAT_EFFECT effect, sint32 player)
{
	return GetEffect(effect, player, false);
}

void FeatTracker::BeginTurn(sint32 player)
{
	PointerList<Feat>::Walker walk(const_cast<PointerList<Feat> *>(&m_activeList));
	while(walk.IsValid())
	{
		Feat *feat = walk.GetObj();
		if(feat->GetPlayer() == player)
		{
			const FeatRecord *rec = g_theFeatDB->Get(feat->GetType());
			if(rec->GetDuration() + feat->GetRound() <= turn_Get()->GetSessionRound())
			{
				walk.Remove();
				RemoveFeatFromEffectLists(feat);
				std::unique_ptr<Feat>{feat};
				continue;
			}
		}

		walk.Next();
	}
}

void FeatTracker::FindBuildingFeats()
{
	for(sint32 f = 0; f < g_theFeatDB->NumRecords(); f++)
	{
		const FeatRecord *rec = g_theFeatDB->Get(f);
		const FeatRecord::BuildingFeat *bf;
		if(rec->GetBuilding(bf))
		{
			m_buildingFeat[bf->GetBuildingIndex()] = true;
		}
	}
}

void FeatTracker::CheckBuildingFeat(Unit &city, sint32 building)
{
	if(!m_buildingFeat[building]) return;

	for(sint32 f = 0; f < g_theFeatDB->NumRecords(); f++)
	{
		const FeatRecord *rec = g_theFeatDB->Get(f);
		const FeatRecord::BuildingFeat *bf;

		if(rec->GetBuilding(bf))
		{
			if(bf->GetBuildingIndex() == building)
			{
				sint32 numCities = 0;
				if (Player* owner = safe_player(city.GetOwner())) {
					for(sint32 c = 0; c < owner->m_all_cities->Num(); c++)
					{
						Unit aCity = owner->m_all_cities->Access(c);
						if(aCity.CD()->HasBuilding(building))
							numCities++;
					}
				}

				sint32 num;
				sint32 percent;

				if(bf->GetNum(num))
				{
					if(numCities >= num)
					{
						gevmanager_Get()->AddEvent(GEV_INSERT_Tail, GEV_AccomplishFeat,
											   GEA_Int, f,
											   GEA_Player, city.GetOwner(),
											   GEA_End);
					}
				}
				else if(bf->GetPercentCities(percent))
				{
					sint32 totalCities = 0;
					if (Player* owner = safe_player(city.GetOwner()))
						totalCities = owner->m_all_cities->Num();
					sint32 havePercent = safe_divide((numCities * 100), totalCities);
					if(havePercent >= percent)
					{
						gevmanager_Get()->AddEvent(GEV_INSERT_Tail, GEV_AccomplishFeat,
											   GEA_Int, f,
											   GEA_Player, city.GetOwner(),
											   GEA_End);
					}
				}
			}
		}
	}
}

void FeatTracker::CheckConquerFeat(sint32 defeated, sint32 defeatedByWhom)
{
	sint32 featIndex;
	if(g_theFeatDB->GetNamedItem("FEAT_CONQUERED_BY_FORCE", featIndex))
	{
		sint32 minCityCount = 0;
		if(g_theFeatDB->Get(featIndex)->GetMinimumSizeOfCiv(minCityCount))
		{
			if (Player* dp = safe_player(defeated)) {
				if (dp->GetMaxCityCount() >= minCityCount)
				{
					gevmanager_Get()->AddEvent(GEV_INSERT_Tail, GEV_AccomplishFeat,
										   GEA_Int, featIndex,
										   GEA_Player, defeatedByWhom,
										   GEA_End);
				}
			}
		}
	}
	else
	{
		bool unknown_feat = false;
		Assert(unknown_feat);
		return;
	}
}

//EMOD added to check if a player achieved a feat 5-11-2006
bool FeatTracker::HasFeat(sint32 type) const
{
//
//	const FeatRecord *rec = g_theFeatDB->Get(type);
//	Assert(rec);
//	if(!rec) return;

	return m_achieved[type];
}

//EMOD added to check if a player achieved a feat 5-11-2006
bool FeatTracker::PlayerHasFeat(sint32 type, sint32 player) const
{
	PointerList<Feat>::Walker walk(const_cast<PointerList<Feat> *>(&m_activeList));
	while(walk.IsValid())
	{
		Feat *feat = walk.GetObj();
		if(feat->GetPlayer() != player)
		{
			return false;
		}

		if(feat->GetType() != type )
		{
			return false;
		}
	}

	return true;
}

//----------------------------------------------------------------------------
//
// Name       : AccomplishFeat::GEVHookCallback
//
// Description: Handler for GEV_AccomplishFeat events.
//
// Parameters : gameEventType   : should be GEV_AccomplishFeat
//              args            : list of arguments
//
// Globals    : -
//
// Returns    : GEV_HD_Continue always
//
// Remark(s)  : The arguments must contain the identifier of the accomplished
//              feat and the player that accomplished it.
//
//----------------------------------------------------------------------------
STDEHANDLER(AccomplishFeat)
{

	sint32 player;
	sint32 featIndex;
	if(!args->GetInt(0, featIndex)) return GEV_HD_Continue;
	if(!args->GetPlayer(0, player)) return GEV_HD_Continue;

	feattracker_Get()->AddFeat(featIndex, player);

	if (network_Get().IsHost())
	{
		// Propagate the information to the clients.
		// Remark: player_Get(player) has been verified in GetPlayer.
		network_Get().Block(player);
		network_Get().Enqueue(std::make_unique<NetInfo>(NET_INFO_CODE_ACCOMPLISHED_FEAT,
									  featIndex,
									  player,
									  safe_player(player)->GetCurRound()
									 ).release()
						 );
		network_Get().Unblock(player);
	}

	return GEV_HD_Continue;
}

STDEHANDLER(FeatBeginTurn)
{
	sint32 player;
	if(!args->GetPlayer(0, player)) return GEV_HD_Continue;

	feattracker_Get()->BeginTurn(player);
	return GEV_HD_Continue;
}

STDEHANDLER(FeatBuildingBuilt)
{
	Unit city;
	sint32 building;

	if(!args->GetInt(0, building)) return GEV_HD_Continue;
	if(!args->GetCity(0, city)) return GEV_HD_Continue;

	feattracker_Get()->CheckBuildingFeat(city, building);
	return GEV_HD_Continue;
}

void FeatTracker::InitializeEvents()
{
	gevmanager_Get()->AddCallback(GEV_AccomplishFeat, GEV_PRI_Primary, &s_AccomplishFeat);

	gevmanager_Get()->AddCallback(GEV_BeginTurn, GEV_PRI_Post, &s_FeatBeginTurn);
	gevmanager_Get()->AddCallback(GEV_CreateBuilding, GEV_PRI_Post, &s_FeatBuildingBuilt);
}

void FeatTracker::CleanupEvents()
{
}
