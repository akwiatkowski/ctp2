#include "ctp/c3.h"
#include "net/general/net_pollution.h"
#include "net/general/network.h"
#include "net/io/net_util.h"

#include "gs/gameobj/pollution.h"
#include "gs/gameobj/PollutionConst.h"

void NetPollution::Packetize(uint8 *buf, uint16 &size)
{
	size = 0;

	PUSHID(k_PACKET_POLLUTION_ID);

	PUSHLONG(pollution_Get()->m_trend);
	for(int i : pollution_Get()->m_history) {
		PUSHLONG(i);
	}
	PUSHLONG(pollution_Get()->m_phase);

}

void NetPollution::Unpacketize(uint16 id, uint8 *buf, uint16 size)
{
	sint32 pos = 0;
	uint16 packid;
	PULLID(packid);
	Assert(packid == k_PACKET_POLLUTION_ID);
	if(packid != k_PACKET_POLLUTION_ID)
		return;

	PULLLONG(pollution_Get()->m_trend);
	for(int & i : pollution_Get()->m_history) {
		PULLLONG(i);
	}
	PULLLONG(pollution_Get()->m_phase);

	Assert(pos == size);
}
