//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ source
// Description  : net_chat allow MP players to chat whith each other
// Id           : $Id$
//
//----------------------------------------------------------------------------
//
// Disclaimer
//
// THIS FILE IS NOT GENERATED OR SUPPORTED BY ACTIVISION.
//
// This material has been developed at apolyton.net by the Apolyton CtP2
// Source Code Project. Contact the authors at ctp2source@apolyton.net.
//
//----------------------------------------------------------------------------
//
// Compiler flags
//
// - None
//
//----------------------------------------------------------------------------
//
// Modifications from the original Activision code:
//
// - None
//
//----------------------------------------------------------------------------

#include "ctp/c3.h"
#include "net/general/net_chat.h"
#include "net/io/net_util.h"
#include "net/general/network.h"
#include "gs/gameobj/Player.h"

NetChat::NetChat(uint32 destmask, MBCHAR const * str, size_t len)
	: m_destmask(destmask)
	, m_str(str, len)
	, m_len(static_cast<sint16>(len))
	, m_from(static_cast<uint8>(network_Get().GetPlayerIndex()))
{
}

void
NetChat::Packetize(uint8 *buf, uint16 &size)
{
	buf[0] = k_PACKET_CHAT_ID >> 8;
	buf[1] = k_PACKET_CHAT_ID & 0xff;

	size = 2;
	PUSHLONG(m_destmask);
	PUSHSHORT(m_len);
	PUSHBYTE(m_from);
	memcpy(&buf[size], m_str.data(), m_len * sizeof(MBCHAR));
	size += m_len * sizeof(MBCHAR);
}

void
NetChat::Unpacketize(uint16 id, uint8 *buf, uint16 size)
{
	Assert(MAKE_CIV3_ID(buf[0], buf[1]) == k_PACKET_CHAT_ID);

	sint32 pos = 2;
	PULLLONG(m_destmask);
	PULLSHORT(m_len);
	PULLBYTE(m_from);
	if(network_Get().IsHost()) {
		Assert(m_from == network_Get().IdToIndex(id));
		m_from = (uint8)network_Get().IdToIndex(id);
	}

	m_str.assign(reinterpret_cast<MBCHAR *>(&buf[pos]), m_len);
	pos += m_len * sizeof(MBCHAR);

	if(network_Get().IsHost()) {
		for(sint32 p = 0; p < k_MAX_PLAYERS; p++) {
			if(!player_Get(p)) continue;

			if(m_destmask & (1 << p) && p != network_Get().GetPlayerIndex() &&
			   p != network_Get().IdToIndex(id) &&
			   player_Get(p)->IsNetwork()) {
				network_Get().QueuePacket(network_Get().IndexToId(p), this);
			}
		}
	}
	if(m_destmask & ((uint32)1 << (uint32)network_Get().GetPlayerIndex())) {
		if(m_destmask == ((uint32)1 << (uint32)network_Get().GetPlayerIndex())) {
			network_Get().AddChatText(m_str.c_str(), (sint32)m_len, m_from, TRUE);
		} else {
			network_Get().AddChatText(m_str.c_str(), (sint32)m_len, m_from, FALSE);
		}
	}
}
