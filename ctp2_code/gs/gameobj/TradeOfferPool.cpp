#include "ctp/c3.h"
#include "gs/gameobj/TradeOfferPool.h"
#include "gs/gameobj/player.h"
#include "robot/aibackdoor/dynarr.h"

#include "gs/utility/Globals.h"
#include "gs/fileio/gamefile.h"

TradeOfferPool::TradeOfferPool() : ObjPool(k_BIT_GAME_OBJ_TYPE_TRADE_OFFER)
{
	m_all_offers = new DynamicArray<TradeOffer>;
}

TradeOfferPool::~TradeOfferPool()
{
	
		delete m_all_offers;
}

TradeOffer TradeOfferPool::Create(Unit fromCity,
								  ROUTE_TYPE offerType,
								  sint32 offerResource,
								  ROUTE_TYPE askingType,
								  sint32 askingResource,
								  Unit toCity
								  )
{
	TradeOfferData* newData;
	TradeOffer newOffer(NewKey(k_BIT_GAME_OBJ_TYPE_TRADE_OFFER));

	newData = new TradeOfferData(newOffer, fromCity,
								 offerType, offerResource,
								 askingType, askingResource,
								 toCity);
	Insert(newData);
	player_Get(fromCity.GetOwner())->AddTradeOffer(newOffer);
	m_all_offers->Insert(newOffer);





	return newOffer;
}

void TradeOfferPool::Remove(TradeOffer offer)
{
	m_all_offers->Del(offer);
	Del(offer);
}

sint32 TradeOfferPool::GetNumTradeOffers()
{
	Assert(m_all_offers);
	if(!m_all_offers)
		return 0;

	return m_all_offers->Num();
}

TradeOffer TradeOfferPool::GetTradeOffer(sint32 index)
{
	Assert(index >= 0 && index < m_all_offers->Num());
	if(index < 0 || index >= m_all_offers->Num()) {
		return {};
	}
	Assert(m_all_offers);
	if(!m_all_offers)
		return {};

	return m_all_offers->Access(index);
}

void TradeOfferPool::ReRegisterOffers()
{







}

void TradeOfferPool::RemoveTradeOffersFromCity(Unit &city)
{
	sint32 i;
	for(i = m_all_offers->Num() - 1; i >= 0; i--) {
		if(m_all_offers->Access(i).GetFromCity() == city)
			m_all_offers->Access(i).Kill();
	}
}

void TradeOfferPool::AddFromNetwork(const TradeOffer &offer)
{
	player_Get(offer.GetFromCity().GetOwner())->AddTradeOffer(offer);
	m_all_offers->Insert(offer);
}
