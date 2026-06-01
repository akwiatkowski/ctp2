#include "ctp/c3.h"
#include "net/general/net_rand.h"
#include "net/io/net_util.h"
#include "gs/utility/RandGen.h"

#define k_RAND_ARRAY_SIZE 56
NetRand::NetRand()
{
}

void NetRand::Packetize(uint8* buf, uint16 &size)
{
	buf[0] = k_PACKET_RAND_ID >> 8;
	buf[1] = k_PACKET_RAND_ID & 0xff;

	size = 2;
	PUSHLONG(rand_ptr()->m_start_seed);
	for(sint32 i = 0; i < k_RAND_ARRAY_SIZE; i++) {
		PUSHLONG(rand_ptr()->m_buffer[i]);
	}
	PUSHLONG(rand_ptr()->m_firstp - rand_ptr()->m_buffer);
	PUSHLONG(rand_ptr()->m_secondp - rand_ptr()->m_buffer);
	PUSHLONG(rand_ptr()->m_endp - rand_ptr()->m_buffer);
	PUSHLONG(rand_ptr()->m_callCount);
}

void NetRand::Unpacketize(uint16 id, uint8 *buf, uint16 size)
{
	sint32 pos;

	Assert(MAKE_CIV3_ID(buf[0], buf[1]) == k_PACKET_RAND_ID);
	sint32 firstpindex, secondpindex, endpindex;

	pos = 2;

	PULLLONG(rand_ptr()->m_start_seed);

	for(sint32 i = 0; i < k_RAND_ARRAY_SIZE; i++) {
		PULLLONG(rand_ptr()->m_buffer[i]);
	}

	PULLLONG(firstpindex);
	PULLLONG(secondpindex);
	PULLLONG(endpindex);

	rand_ptr()->m_firstp = &rand_ptr()->m_buffer[firstpindex];
	rand_ptr()->m_secondp = &rand_ptr()->m_buffer[secondpindex];
	rand_ptr()->m_endp = &rand_ptr()->m_buffer[endpindex];

	PULLLONG(rand_ptr()->m_callCount);
}
