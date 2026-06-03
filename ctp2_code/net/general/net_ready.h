#ifdef HAVE_PRAGMA_ONCE
#pragma once
#endif
#ifndef _NET_READY_H_
#define _NET_READY_H_

#include "net/general/net_packet.h"

class MilitaryReadiness;

class NetReadiness : public Packetizer {
public:
	NetReadiness(MilitaryReadiness *);
	NetReadiness() = default;

	void Packetize(uint8* buf, uint16& size) override;
	void Unpacketize(uint16 id, uint8* buf, uint16 size) override;

private:
	MilitaryReadiness * m_data;
};

#endif
