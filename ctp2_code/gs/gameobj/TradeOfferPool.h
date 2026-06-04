#ifdef HAVE_PRAGMA_ONCE
#pragma once
#endif
#ifndef _TRADEOFFERPOOL_H_
#define _TRADEOFFERPOOL_H_

#include "gs/gameobj/ObjPool.h"

#include "gs/gameobj/TradeOffer.h"

#include <nlohmann/json.hpp>

class CivArchive;
template <class T> class DynamicArray;

class TradeOfferPool : public ObjPool
{
private:

	DynamicArray<TradeOffer> *m_all_offers;


public:
	TradeOfferPool();
	~TradeOfferPool() override;

	TradeOfferData* AccessTradeOffer(const TradeOffer id)
	{
		return (TradeOfferData*)Access(id);
	}

	TradeOfferData* GetTradeOffer(const TradeOffer id)
	{
		return (TradeOfferData*)Get(id);
	}

	TradeOffer Create(Unit fromCity,
					  ROUTE_TYPE offerType,  sint32 offerResource,
					  ROUTE_TYPE askingType, sint32 askingResource,
					  Unit toCity);
	void Remove(TradeOffer offer);

	sint32 GetNumTradeOffers();
	TradeOffer GetTradeOffer(sint32 index);
	void ReRegisterOffers();

	void RemoveTradeOffersFromCity(Unit &city);
	void AddFromNetwork(const TradeOffer &offer);

	friend void to_json(nlohmann::json &j, TradeOfferPool const &p);
	friend void from_json(nlohmann::json const &j, TradeOfferPool &p);
};

// g_theTradeOfferPool's lifecycle (new / archive-load / Cleanup) lives
// in gs/utility/gameinit.cpp; the variable is now file-scope `static`
// there.  External readers go through tradeofferpool_Get().
TradeOfferPool * tradeofferpool_Get();
void             tradeofferpool_Set(TradeOfferPool *p);
#endif
