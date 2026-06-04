#include "ctp/c3.h"
#include "gs/gameobj/TradePool.h"
#include "gs/gameobj/TradeRouteData.h"

#include "gs/core/render_observer.h"
#include "TerrainRecord.h"
#include "gs/utility/TradeDynArr.h"
#include "ResourceRecord.h"
#include "gs/utility/Globals.h"
#include "gs/gameobj/Events.h"
#include "gs/events/GameEventUser.h"
#include "gs/events/GameEventManager.h"

// TradePool::Draw lives in ui/aui_ctp2/trade_pool_draw.cpp (UI-side
// rendering — keeps gs/gameobj/ free of aui_Surface dependencies).

TradePool::TradePool() : ObjPool(k_BIT_GAME_OBJ_TYPE_TRADE_ROUTE)
{
	m_all_routes = new TradeDynamicArray;
}

TradePool::~TradePool()
{
	sint32 i;
	for(i = m_all_routes->Num() - 1; i >= 0; i--) {
		m_all_routes->Access(i).Kill(CAUSE_KILL_TRADE_ROUTE_UNKNOWN);
	}
	delete m_all_routes;
}

TradeRoute TradePool::Create(Unit sourceCity,
							 Unit destCity,
							 PLAYER_INDEX owner,
							 ROUTE_TYPE sType,
							 sint32 sResource,
							 PLAYER_INDEX paying_for,
							 sint32 gold_in_return)
{
	TradeRouteData* newData;
	TradeRoute newRoute(NewKey(k_BIT_GAME_OBJ_TYPE_TRADE_ROUTE));

	newData = new TradeRouteData(newRoute, sourceCity, destCity, owner,
								 sType, sResource, paying_for,
								 gold_in_return);
	if(!newData->IsValid()) {

		delete newData;
		return {};
	}

	Insert(newData);

	sourceCity.AddTradeRoute(newRoute);
	destCity.AddTradeRoute(newRoute);
	m_all_routes->Insert(newRoute);
	render_observer::TradeActorCreate(newRoute);
	sourceCity.RecalculateResources();

	return newRoute;
}

void TradePool::Remove(TradeRoute route)
{
	m_all_routes->Del(route);
	Del(route);
}


TradeRoute TradePool::GetRouteIndex(sint32 index)
{
	Assert(index >= 0 && index < m_all_routes->Num());

	return m_all_routes->Access(index);
}

// TradePool::Draw() body moved to ui/aui_ctp2/trade_pool_draw.cpp.

void TradePool::RecreateActors()
{
	sint32 i;
	for(i = 0; i < m_all_routes->Num(); i++) {
		render_observer::TradeActorCreate(m_all_routes->Access(i));
	}
}

sint32 TradePool::GetGoldValue(sint32 resource, sint32 n)
{
	return ((n * (n+1)) / 2) * g_theResourceDB->Get(resource)->GetGold();

#if 0
	sint32 terrain = resource % k_BASE_TERRAIN_TYPES;
 	sint32 good = resource / k_BASE_TERRAIN_TYPES;
 	sint32 gold = (n+1) *
		g_theTerrainDB->Get(terrain)->GetGood(good)->GetGoodGoldValue();
	return gold;
#endif
}

sint32 TradePool::GetSingleGoodValue(sint32 resource, sint32 nth_good)
{
	return (((nth_good * (nth_good + 1) / 2)
			- ((nth_good-1) * (nth_good) / 2 ))
			* g_theResourceDB->Get(resource)->GetGold() );
}

const TradeDynamicArray &TradePool::GetAllRoutes()
{
	return *m_all_routes;
}

void TradePool::BreakOffTrade(PLAYER_INDEX attack_owner,
							  PLAYER_INDEX defense_owner)
{
	sint32 i;

	for(i = 0; i < m_all_routes->Num(); i++) {
		TradeRoute route = m_all_routes->Access(i);
		if((route.GetSource().GetOwner() == attack_owner &&
			route.GetDestination().GetOwner() == defense_owner) ||
		   (route.GetDestination().GetOwner() == attack_owner &&
			route.GetSource().GetOwner() == defense_owner)) {

			gevmanager_Get()->Pause();
			gevmanager_Get()->AddEvent(GEV_INSERT_AfterCurrent, GEV_KillTradeRoute,
				GEA_TradeRoute, route.m_id,
				GEA_Int, CAUSE_KILL_TRADE_ROUTE_SENDER_KILLED,
				GEA_End);
			gevmanager_Get()->Resume();
		}
	}
}
