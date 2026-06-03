#ifndef _NET_CIV_H_
#define _NET_CIV_H_

#include "net/general/net_packet.h"
class CivilisationData;

class NetCivilization : public Packetizer
{
public:
	NetCivilization(CivilisationData *data);
	NetCivilization() = default;

	void Packetize(uint8 *buf, uint16 &size) override;
	void Unpacketize(uint16 id, uint8 *buf, uint16 size) override;

private:
	CivilisationData *m_data;
};

class NetSetLeaderName : public Packetizer
{
public:
	NetSetLeaderName(sint32 player) {
		m_player = (uint8)player;
	}
	NetSetLeaderName() = default;

	void Packetize(uint8 *buf, uint16 &size) override;
	void Unpacketize(uint16 id, uint8 *buf, uint16 size) override;
private:
	uint8 m_player;
};

#endif
