#ifdef HAVE_PRAGMA_ONCE
#pragma once
#endif
#ifndef _NET_PLAYER_H_
#define _NET_PLAYER_H_

class NetPlayer;
class NetAddPlayer;

#include "net/general/net_packet.h"     // Packetizer
class Player;

class NetPlayer : public Packetizer
{
public:
	NetPlayer(Player* player);
	NetPlayer() = default;

	void Packetize(uint8* buf, uint16& size) override;
	void Unpacketize(uint16 id, uint8* buf, uint16 size) override;
private:
	Player* m_player;
};

class NetAddPlayer : public Packetizer
{
public:
	NetAddPlayer(uint16 id, char *name) {
		m_id = id;
		m_name = name;
	}

	NetAddPlayer() = default;
	void Packetize(uint8 *buf, uint16 &size) override;
	void Unpacketize(uint16 id, uint8 *buf, uint16 size) override;
private:
	uint16 m_id;
	char *m_name;
};

#endif
