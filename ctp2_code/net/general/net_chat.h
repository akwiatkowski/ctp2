#ifdef HAVE_PRAGMA_ONCE
#pragma once
#endif
#ifndef _NET_CHAT_H_
#define _NET_CHAT_H_

#include <string>

#include "net/general/net_packet.h"
#include "net/general/net_const.h"

class NetChat : public Packetizer
{
public:
	NetChat(uint32 dest, MBCHAR const * str, size_t len);
	NetChat() : m_destmask(0), m_len(0) {}
	~NetChat() override = default;

	void Packetize(uint8 *buf, uint16 &size) override;
	void Unpacketize(uint16 id, uint8 *buf, uint16 size) override;

private:
	uint32 m_destmask;
	std::string m_str;
	sint16 m_len;
	uint8 m_from;
};

#endif
