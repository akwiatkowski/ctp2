#include "ctp/c3.h"
#include "net/general/network.h"
#include "net/general/net_tradeoffer.h"
#include "net/io/net_util.h"

#include "gs/gameobj/TradeOfferPool.h"
#include "gs/gameobj/TradeOfferData.h"
#include "gs/gameobj/TradeOffer.h"
#include "gs/gameobj/UnitPool.h"

#include "gs/gameobj/Player.h"

NetTradeOffer::NetTradeOffer(TradeOfferData* data) : m_offerData(data)
{
}

void NetTradeOffer::Packetize(uint8* buf, uint16 &size)
{
	buf[0] = k_PACKET_TRADE_OFFER_ID >> 8;
	buf[1] = k_PACKET_TRADE_OFFER_ID & 0xff;

	size = 2;
	PUSHLONG(m_offerData->m_id);
	PUSHLONG(m_offerData->m_owner);
	PUSHLONG(m_offerData->m_fromCity.m_id);
	PUSHBYTE(m_offerData->m_offerType);
	PUSHLONG(m_offerData->m_offerResource);
	PUSHBYTE(m_offerData->m_askingType);
	PUSHLONG(m_offerData->m_askingResource);
	PUSHLONG(m_offerData->m_toCity.m_id);
}

void NetTradeOffer::Unpacketize(uint16 id, uint8* buf, uint16 size)
{
	sint32 pos;
	sint32 fromCity;
	sint32 toCity;

	Assert(MAKE_CIV3_ID(buf[0], buf[1]) == k_PACKET_TRADE_OFFER_ID);
	TradeOffer offer(getlong(&buf[2]));

	network_Get().CheckReceivedObject((uint32)offer);

	if(!tradeofferpool_Get()->IsValid(offer)) {
		m_offerData = new TradeOfferData(offer);
	} else {
		m_offerData = tradeofferpool_Get()->AccessTradeOffer(offer);
	}

	pos = 6;
	PULLLONG(m_offerData->m_owner);
	PULLLONG(fromCity);
	m_offerData->m_fromCity = Unit(fromCity);
	PULLBYTETYPE(m_offerData->m_offerType, ROUTE_TYPE);
	PULLLONG(m_offerData->m_offerResource);
	PULLBYTETYPE(m_offerData->m_askingType, ROUTE_TYPE);
	PULLLONG(m_offerData->m_askingResource);
	PULLLONG(toCity);
	m_offerData->m_toCity = Unit(toCity);

	if(!tradeofferpool_Get()->IsValid(offer)) {
		tradeofferpool_Get()->HackSetKey(((uint32)offer & k_ID_KEY_MASK) + 1);
		tradeofferpool_Get()->Insert(m_offerData);
		tradeofferpool_Get()->AddFromNetwork(offer);
	}
}
