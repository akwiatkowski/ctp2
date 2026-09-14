#include "ctp/c3.h"
#include "gs/utility/Globals.h"
#include "net/general/net_army.h"
#include "net/general/network.h"
#include "net/io/net_util.h"
#include "gs/gameobj/player.h"
#include "gs/gameobj/UnitPool.h"
#include "gs/gameobj/ArmyPool.h"
#include "gs/gameobj/Army.h"
#include "gs/gameobj/ArmyData.h"
#include "ai/ctpai.h"
#include "net/general/net_info.h"

NetNewArmy::NetNewArmy(PLAYER_INDEX player, const ArmyList &army,
					   sint32 armyIndex, CAUSE_NEW_ARMY cause)
{
}

void NetNewArmy::Packetize(uint8 *buf, uint16 &size)
{
}

void NetNewArmy::Unpacketize(uint16 id, uint8 *buf, uint16 size)
{
}

NetRemoveArmy::NetRemoveArmy(PLAYER_INDEX player, const ArmyList &army,
							 sint32 armyIndex, CAUSE_REMOVE_ARMY cause)
{
}

void NetRemoveArmy::Packetize(uint8 *buf, uint16 &size)
{
}

void NetRemoveArmy::Unpacketize(uint16 id, uint8 *buf, uint16 size)
{
}

NetArmy::NetArmy(ArmyData *data)
{
	m_data = data;
}

void NetArmy::Packetize(uint8 *buf, uint16 &size)
{
	size = 0;
	PUSHID(k_PACKET_ARMY_ID);

	PUSHLONG(m_data->m_id);

	uint8 c = (uint8)m_data->m_nElements;
	PUSHBYTE(c);
	sint32 i;
	for(i = 0; i < c; i++) {
		PUSHLONG((uint32)m_data->Access(i));
	}
	PUSHBYTE((uint8)m_data->m_owner);
	PUSHSHORT((uint16)m_data->m_pos.x);
	PUSHSHORT((uint16)m_data->m_pos.y);
	PUSHLONG(m_data->m_removeCause);
}

void NetArmy::Unpacketize(uint16 id, uint8 *buf, uint16 size)
{
	sint32 pos = 0;
	uint16 packid;
	sint32 i;
	PULLID(packid);
	Assert(packid == k_PACKET_ARMY_ID);

	Army army;
	PULLLONGTYPE(army, Army);

	network_Get().CheckReceivedObject((uint32)army);

	if(armypool_Get()->IsValid(army)) {
		m_data = armypool_Get()->AccessArmy(army);
	} else {
		m_data = new ArmyData(army);
	}

	uint8 c;
	PULLBYTE(c);
	m_data->Clear();
	for(i = 0; i < c; i++) {
		Unit u;
		PULLLONGTYPE(u, Unit);
		if(unitpool_Get()->IsValid(u))
			m_data->Insert(u);
	}

	PULLBYTE(m_data->m_owner);
	PULLSHORT(m_data->m_pos.x);
	PULLSHORT(m_data->m_pos.y);
	PULLLONGTYPE(m_data->m_removeCause, CAUSE_REMOVE_ARMY);

	if(!armypool_Get()->IsValid(army)) {
		armypool_Get()->HackSetKey(((uint32)m_data->m_id & k_ID_KEY_MASK) + 1);
		armypool_Get()->Insert(m_data);

		CtpAi::AddGoalsForArmy(army);
	}
}

NetGroupRequest::NetGroupRequest(const CellUnitList &units, const Army &army)
{
	m_armyId = army.m_id;
	m_units = units;
}

void NetGroupRequest::Packetize(uint8 *buf, uint16 &size)
{
	size  = 0;
	PUSHID(k_PACKET_GROUP_REQUEST_ID);

	PUSHLONG(m_armyId);
	uint8 n;
	n = (uint8)m_units.Num();
	PUSHBYTE(n);
	uint8 i;
	for(i = 0; i < n; i++) {
		PUSHLONG(m_units[i].m_id);
	}
}

void NetGroupRequest::Unpacketize(uint16 id, uint8 *buf, uint16 size)
{
	sint32 pos = 0;
	uint16 packid;
	sint32 pl = network_Get().IdToIndex(id);

	PULLID(packid);
	Assert(packid == k_PACKET_GROUP_REQUEST_ID);

	PULLLONG(m_armyId);

	Army theArmy;

	if(m_armyId != 0) {
		Assert(armypool_Get()->IsValid(m_armyId));
		if(!armypool_Get()->IsValid(m_armyId)) {
			network_Get().Resync(network_Get().IdToIndex(id));
			return;
		}
		theArmy.m_id = m_armyId;
	} else {
		theArmy = player_Get(pl)->GetNewArmy(CAUSE_NEW_ARMY_REMOTE_GROUPING);
		network_Get().Enqueue(new NetInfo(NET_INFO_CODE_ADD_ARMY, pl, CAUSE_NEW_ARMY_REMOTE_GROUPING, theArmy.m_id));
	}

	uint8 n;
	uint8 i;
	PULLBYTE(n);
	m_units.Clear();
	for(i = 0; i < n; i++) {
		uint32 uid;
		PULLLONG(uid);
		m_units.Insert(uid);
		theArmy->GroupUnit(m_units[i]);
		network_Get().QueuePacket(id, new NetInfo(NET_INFO_CODE_REMOTE_GROUP, theArmy, m_units[i].m_id));
	}
}

NetUngroupRequest::NetUngroupRequest(const Army &army, const CellUnitList &units)
{
	m_armyId = army.m_id;
	m_units = units;
}

void NetUngroupRequest::Packetize(uint8 *buf, uint16 &size)
{
	size = 0;
	PUSHID(k_PACKET_UNGROUP_REQUEST_ID);

	PUSHLONG(m_armyId);
	uint8 n = (uint8)m_units.Num();
	PUSHBYTE(n);
	uint8 i;
	for(i = 0; i < n; i++) {
		PUSHLONG(m_units[i].m_id);
	}
}

void NetUngroupRequest::Unpacketize(uint16 id, uint8 *buf, uint16 size)
{
	sint32 pos = 0;
	uint16 packid;
	sint32 pl = network_Get().IdToIndex(id);

	PULLID(packid);
	Assert(packid == k_PACKET_UNGROUP_REQUEST_ID);

	PULLLONG(m_armyId);

	Army theArmy(m_armyId);
	Assert(theArmy.IsValid());
	if(!theArmy.IsValid()) {
		network_Get().Resync(pl);
		return;
	}

	uint8 n;
	uint8 i;
	PULLBYTE(n);
	for(i = 0; i < n; i++) {
		Army newArmy = player_Get(pl)->GetNewArmy(CAUSE_NEW_ARMY_REMOTE_UNGROUPING);
		network_Get().Enqueue(new NetInfo(NET_INFO_CODE_ADD_ARMY, pl, CAUSE_NEW_ARMY_REMOTE_UNGROUPING, newArmy.m_id));

		uint32 unitId;
		PULLLONG(unitId);
		m_units.Insert(Unit(unitId));
		m_units[i].ChangeArmy(newArmy, CAUSE_NEW_ARMY_REMOTE_UNGROUPING);
	}
	network_Get().Enqueue(new NetInfo(NET_INFO_CODE_REMOTE_UNGROUP, theArmy, pl));
}
