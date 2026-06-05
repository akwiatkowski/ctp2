//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ source
// Description  : Network message
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
#include "net/general/network.h"
#include "net/general/net_message.h"
#include "net/general/net_packet.h"
#include "net/io/net_util.h"
#include "gs/gameobj/MessageData.h"
#include "gs/gameobj/MessagePool.h"
#include "gs/utility/UnitDynArr.h"
#include "net/general/net_info.h"
#include "gs/gameobj/Player.h"

void NetMessage::Packetize(uint8 *buf, uint16 &size)
{
	Assert(FALSE);

	size = 0;
	PUSHID(k_PACKET_MESSAGE_ID);
	PUSHLONG(m_data->m_id);
	PUSHBYTE((uint8)m_data->m_owner);
	PUSHBYTE((uint8)m_data->m_sender);
	PUSHBYTE((uint8)m_data->m_msgType);
	PUSHLONG(m_data->m_timestamp);
	// m_text is std::string; inline the PUSHSTRING expansion against
	// .data() / .size() because the macro takes a writable char* lvalue.
	// Net code is disabled pending protocol rewrite; this preserves wire
	// shape so the file compiles.
	{
		uint16 l = static_cast<uint16>(m_data->m_text.size());
		PUSHSHORT(l);
		if (l > 0) { memcpy(&buf[size], m_data->m_text.data(), l); size += l; }
	}
	if(!m_data->m_cityList) {
		PUSHSHORT(-1);
	} else {
		PUSHSHORT((uint16)m_data->m_cityList->Num());
		for(sint32 i = 0 ; i < m_data->m_cityList->Num(); i++) {
			PUSHLONG(uint32(m_data->m_cityList->Access(i)));
		}
	}
}

void NetMessage::Unpacketize(uint16 id, uint8 *buf, uint16 size)
{
	Assert(FALSE);

	uint16 packid;
	sint32 pos = 0;
	PULLID(packid);
	Assert(packid == k_PACKET_MESSAGE_ID);

	Message msg;
	Message realmsg;
	PULLLONGTYPE(msg, Message);

	if(network_Get().IsHost()) {
		realmsg = messagepool_Get()->ServerCreate();
		m_data = realmsg.AccessData();
	} else {
		network_Get().CheckReceivedObject((uint32)msg);
		if(messagepool_Get()->IsValid(msg)) {
			m_data = messagepool_Get()->AccessMessage(msg);
		} else {
			m_data = new MessageData(msg, 0);
		}
	}

	PULLBYTETYPE(m_data->m_owner, PLAYER_INDEX);
	PULLBYTETYPE(m_data->m_sender, PLAYER_INDEX);
	PULLBYTETYPE(m_data->m_msgType, MESSAGE_TYPE);
	PULLLONG(m_data->m_timestamp);
	// m_text is std::string; inline the PULLSTRING expansion — see
	// matching Packetize() comment. Resize first, then memcpy into
	// .data() (writable in C++17+).
	{
		uint16 l; PULLSHORT(l);
		m_data->m_text.resize(l);
		if (l > 0) { memcpy(&m_data->m_text[0], &buf[pos], l); pos += l; }
	}

	sint16 numCities;
	PULLSHORT(numCities);

	m_data->m_cityList->Clear();

	if(numCities > 0) {
		for(sint32 i = 0 ; i < numCities; i++) {
			PULLLONGTYPE(m_data->m_cityList->Access(i), Unit);
		}
	}
	if(network_Get().IsHost()) {
		if(realmsg == msg) {
			network_Get().QueuePacket(id, new NetInfo(NET_INFO_CODE_ACK_OBJECT,
												  (uint32)msg));
		} else {
			network_Get().QueuePacket(id, new NetInfo(NET_INFO_CODE_NAK_OBJECT,
												  (uint32)msg, (uint32)realmsg));
		}
		player_Get(m_data->m_owner)->AddMessage(realmsg);
		network_Get().Enqueue(m_data);
	} else if(!messagepool_Get()->IsValid(msg)) {
		messagepool_Get()->HackSetKey(((uint32)msg & k_ID_KEY_MASK)+1);
		messagepool_Get()->Insert(m_data);
		player_Get(m_data->m_owner)->AddMessage(msg);
	}
}

NetInfoMessage::NetInfoMessage(NET_MSG_TYPE msg,
							   const MBCHAR *playerName,
							   sint32 index)
{
	m_msg = msg;
	m_name = playerName;
	m_player = index;
}

void NetInfoMessage::Packetize(uint8 *buf, uint16 &size)
{
	size = 0;
	PUSHID(k_PACKET_NET_INFO_MESSAGE_ID);
	PUSHLONG(m_msg);
	PUSHSTRING(m_name);
	PUSHLONG(m_player);
}

void NetInfoMessage::Unpacketize(uint16 id, uint8 *buf, uint16 size)
{
	uint16 packid;
	sint32 pos = 0;
	PULLID(packid);
	Assert(packid == k_PACKET_NET_INFO_MESSAGE_ID);

	PULLLONGTYPE(m_msg, NET_MSG_TYPE);
	MBCHAR name[1024];
	PULLSTRING(name);
	PULLLONG(m_player);

	switch(m_msg) {
		case NET_MSG_PLAYER_JOINED:
			network_Get().SendJoinedMessage(name, m_player);
			break;
		case NET_MSG_PLAYER_LEFT:
			network_Get().SendLeftMessage(name, m_player);
			break;
		case NET_MSG_NEW_HOST:
			network_Get().SendNewHostMessage(name, m_player);
			break;
		default:
			Assert(FALSE);
			break;
	}

	Assert(pos == size);
}
