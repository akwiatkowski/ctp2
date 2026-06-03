#ifndef _NET_POLLUTION_H_
#define _NET_POLLUTION_H_

#include "net/general/net_packet.h"

class NetPollution : public Packetizer
{
public:
	NetPollution() = default;

	void Packetize(uint8* buf, uint16 &size) override;
	void Unpacketize(uint16 id, uint8 *buf, uint16 size) override;
};

#endif
