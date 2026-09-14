#include "ctp/c3.h"
#include "net/general/net_guid.h"
#include "net/io/net_util.h"
#include "net/general/network.h"
#include "gs/gameobj/player.h"

NetGuid::NetGuid(const GUID *guid)
{
	m_guid = guid;
}

NetGuid::NetGuid()
= default;

void NetGuid::Packetize(uint8 *buf, uint16 &size)
{
	size = 0;
	PUSHID(k_PACKET_GUID_ID);
	memcpy(&buf[size], (uint8*)m_guid, sizeof(*m_guid));
	size += sizeof(*m_guid);
}

void NetGuid::Unpacketize(uint16 id, uint8 *buf, uint16 size)
{
	sint32 pos = 0;
	uint16 packid;
	PULLID(packid);
	Assert(packid == k_PACKET_GUID_ID);

	GUID guid;
	memcpy((uint8*)&guid, &buf[pos], sizeof(GUID));
	pos += sizeof(GUID);
	network_Get().SetGuid(id, &guid);
	Assert(pos == size);
}

void NetSetPlayerGuid::Packetize(uint8 *buf, uint16 &size)
{
	size = 0;
	PUSHID(k_PACKET_SET_PLAYER_GUID_ID);
	PUSHBYTE(m_player);
	memcpy(&buf[size], (uint8*)&player_Get(m_player)->m_networkGuid, sizeof(GUID));
	size += sizeof(GUID);
}

void NetSetPlayerGuid::Unpacketize(uint16 id, uint8 *buf, uint16 size)
{
	sint32 pos = 0;
	uint16 packid;
	PULLID(packid);
	Assert(packid == k_PACKET_SET_PLAYER_GUID_ID);

	PULLBYTE(m_player);

	GUID guid;
	memcpy((uint8*)&guid, &buf[pos], sizeof(GUID));
	pos += sizeof(GUID);
	if(player_Get(m_player)) {
		player_Get(m_player)->m_networkGuid = guid;
	}
	Assert(pos == size);
}
