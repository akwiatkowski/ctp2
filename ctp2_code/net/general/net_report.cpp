#include "ctp/c3.h"
#include "net/io/net_util.h"
#include "net/general/net_report.h"
#include "net/general/network.h"
#include "net/general/net_info.h"

NetReport::NetReport(NET_REPORT type)
{
	m_type = type;
}

void NetReport::Packetize(uint8 *buf, uint16 &size)
{
	buf[0] = k_PACKET_REPORT_ID >> 8;
	buf[1] = k_PACKET_REPORT_ID & 0xff;

	size = 2;
	PUSHBYTE(m_type);
}

void NetReport::Unpacketize(uint16 id, uint8 *buf, uint16 size)
{
	Assert(MAKE_CIV3_ID(buf[0], buf[1]) == k_PACKET_REPORT_ID);

	sint32 pos = 2;

	PULLBYTETYPE(m_type,NET_REPORT);
	switch(m_type) {
		case NET_REPORT_READY_FOR_DATA:
		{
			BOOL wasReady = network_Get().ReadyToStart();

			network_Get().ProcessNewPlayer(id);




			if(!wasReady && network_Get().ReadyToStart()) {
				network_Get().Enqueue(new NetInfo(NET_INFO_CODE_ALL_PLAYERS_READY));
			}

			break;
		}
		case NET_REPORT_ACK_RESYNC:
		{
			DPRINTF(k_DBG_NET, ("Client %d acks resync\n", network_Get().IdToIndex(id)));
			network_Get().AckResync(network_Get().IdToIndex(id));
			break;
		}
		default:
			Assert(FALSE);
			break;
	}
}
