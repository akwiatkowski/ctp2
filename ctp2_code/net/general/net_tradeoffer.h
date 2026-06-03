#ifdef HAVE_PRAGMA_ONCE
#pragma once
#endif
#ifndef _NET_TRADEOFFER_H_
#define _NET_TRADEOFFER_H_

#include "net/general/net_packet.h"

class TradeOfferData;

class NetTradeOffer : public Packetizer {
public:
	NetTradeOffer(TradeOfferData*);
	NetTradeOffer() = default;

	void Packetize(uint8* buf, uint16& size) override;
	void Unpacketize(uint16 id, uint8* buf, uint16 size) override;

private:
	TradeOfferData* m_offerData;
};

#endif
