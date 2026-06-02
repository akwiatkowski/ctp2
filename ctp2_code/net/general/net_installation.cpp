#include "ctp/c3.h"
#include "net/general/network.h"
#include "net/general/net_installation.h"
#include "net/io/net_util.h"
#include "gs/gameobj/installationpool.h"
#include "gs/gameobj/Player.h"
#include "gs/gameobj/XY_Coordinates.h"
#include "gs/world/World.h"

NetInstallation::NetInstallation(InstallationData *data)
{
	m_data = data;
}

void NetInstallation::Packetize(uint8 *buf, uint16 &size)
{
	buf[0] = k_PACKET_INSTALLATION_ID >> 8;
	buf[1] = k_PACKET_INSTALLATION_ID & 0xff;

	size = 2;
	PUSHLONG(m_data->m_id);
	PUSHBYTE((uint8)m_data->m_owner);
	PUSHBYTE((uint8)m_data->m_type);
	PUSHSHORT((uint16)m_data->m_point.x);
	PUSHSHORT((uint16)m_data->m_point.y);
	PUSHLONG(m_data->m_visibility);
}

void NetInstallation::Unpacketize(uint16 id, uint8 *buf, uint16 size)
{
	Assert(MAKE_CIV3_ID(buf[0], buf[1]) == k_PACKET_INSTALLATION_ID);
	sint32 pos = 2;

	Installation inst;
	PULLLONGTYPE(inst, Installation);

	g_network.CheckReceivedObject((uint32)inst);

	if(!installationpool_Get()->IsValid(inst)) {
		m_data = new InstallationData(inst);
	} else {
		m_data = installationpool_Get()->AccessInstallation(inst);
	}

	pos = 6;
	PULLBYTE(m_data->m_owner);
	PULLBYTE(m_data->m_type);
	PULLSHORT(m_data->m_point.x);
	PULLSHORT(m_data->m_point.y);
	PULLLONG(m_data->m_visibility);

	if(!installationpool_Get()->IsValid(inst)) {
		installationpool_Get()->HackSetKey(((uint32)inst & k_ID_KEY_MASK) + 1);
		installationpool_Get()->Insert(m_data);
		player_Get(m_data->m_owner)->AddInstallation(inst);
		world_Get()->InsertInstallation(inst, m_data->m_point);
		m_data->DoVision();
	}
}
