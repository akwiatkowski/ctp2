#ifdef HAVE_PRAGMA_ONCE
#pragma once
#endif
#ifndef __NET_RESEARCH_H__
#define __NET_RESEARCH_H__

#include "net/general/net_packet.h"
class Advances;

class NetResearch : public Packetizer
{
public:
	NetResearch(Advances *adv);
	NetResearch() = default;

	void Packetize(uint8 *buf, uint16 &size) override;
	void Unpacketize(uint16 id, uint8 *buf, uint16 size) override;

private:
	Advances *m_adv;
};

#endif
