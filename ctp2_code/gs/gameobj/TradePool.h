#ifdef HAVE_PRAGMA_ONCE
#pragma once
#endif
#ifndef _TRADEPOOL_H_
#define _TRADEPOOL_H_

#include "gs/gameobj/ObjPool.h"

#include "gs/gameobj/TradeRoute.h"

#include <nlohmann/json.hpp>
#include <memory>

class TradeRouteData;
enum ROUTE_TYPE;
class aui_Surface;
class TradeDynamicArray;

class TradePool : public ObjPool
{









	std::unique_ptr<TradeDynamicArray> m_all_routes;


	friend class NetTradeRoute;
	friend class Network;

public:
	TradeRouteData* AccessTradeRoute(const TradeRoute id)
	{
		return (TradeRouteData*) Access(id);
	}

	TradeRouteData* GetTradeRoute(const TradeRoute id) const
	{
		return (TradeRouteData*) Get(id);
	}

	TradePool();
	~TradePool() override;

	TradeRoute Create(Unit sourceCity, Unit destCity, PLAYER_INDEX owner,
					  ROUTE_TYPE sType, sint32 sResource,
					  PLAYER_INDEX paying_for,
					  sint32 gold_in_return);
	void Remove(TradeRoute route);
	TradeRoute GetRouteIndex(sint32 index);

	void Draw(aui_Surface* surface);
	void RecreateActors();

	// JSON bridge — mirrors TradePool::Serialize.  Persists ObjPool key
	// counter + every live TradeRouteData entry.  m_all_routes (a flat
	// view of the table) is rebuilt during from_json.
	friend void to_json(nlohmann::json &j, TradePool const &p);
	friend void from_json(nlohmann::json const &j, TradePool &p);

	sint32 GetSingleGoodValue(sint32 resource, sint32 nth_good);

	const TradeDynamicArray &GetAllRoutes();
	TradeDynamicArray *AccessAllRoutes() { return m_all_routes.get(); }

	sint32 GetGoldValue(sint32 resource, sint32 num);
	void BreakOffTrade(PLAYER_INDEX pl1, PLAYER_INDEX pl2);
};

// g_theTradePool's lifecycle (new / archive-load / Cleanup) lives in
// gs/utility/gameinit.cpp; the variable is now file-scope `static`
// there.  External readers go through tradepool_Get().
TradePool * tradepool_Get();
void        tradepool_Set(TradePool *p);
#else

class TradePool;
TradePool * tradepool_Get();
void        tradepool_Set(TradePool *p);

#endif
