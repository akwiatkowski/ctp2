#ifdef HAVE_PRAGMA_ONCE
#pragma once
#endif
#ifndef _TRADEOFFER_H_
#define _TRADEOFFER_H_

#include "gs/gameobj/ID.h"
#include "gs/gameobj/Unit.h"
#include "gs/gameobj/TradeOfferData.h"
#include <nlohmann/json.hpp>

class TradeOffer : public ID {
public:
	TradeOffer () : ID() { } ;
	TradeOffer (sint32 val) : ID (val) { };
	TradeOffer (uint32 val) : ID (val) { };

	void KillOffer();
	void Kill() { KillOffer(); }
	void RemoveAllReferences();

	const TradeOfferData* GetData() const;
	TradeOfferData* AccessData();

	PLAYER_INDEX GetOwner() const { return GetData()->GetOwner(); }
	Unit GetFromCity() const { return GetData()->GetFromCity(); }
	Unit GetToCity() const { return GetData()->GetToCity(); }
	ROUTE_TYPE GetOfferType() const { return GetData()->GetOfferType(); }
	sint32 GetOfferResource() const { return GetData()->GetOfferResource(); }
	ROUTE_TYPE GetAskingType() const { return GetData()->GetAskingType(); }
	sint32 GetAskingResource() const { return GetData()->GetAskingResource(); }

	BOOL Accept(PLAYER_INDEX player, const Unit &sourceCity, Unit const & destCity);

	void Castrate() {};
};

// JSON bridge — TradeOffer is a pure ID-derived handle, serialise as uint32.
inline void to_json(nlohmann::json &j, TradeOffer const &t)
{
    j = t.m_id;
}

inline void from_json(nlohmann::json const &j, TradeOffer &t)
{
    t.m_id = j.get<uint32>();
}

#endif
