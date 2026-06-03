#ifdef HAVE_PRAGMA_ONCE
#pragma once
#endif
#ifndef _NET_TRADEROUTE_H_
#define _NET_TRADEROUTE_H_

#include "net/general/net_packet.h"

class TradeRouteData;

class NetTradeRoute : public Packetizer {
public:
	NetTradeRoute(TradeRouteData*, bool newRoute);
	NetTradeRoute() = default;

	void Packetize(uint8* buf, uint16& size) override;
	void Unpacketize(uint16 id, uint8* buf, uint16 size) override;

private:
	TradeRouteData* m_routeData;
	bool m_newRoute;
};

#endif
