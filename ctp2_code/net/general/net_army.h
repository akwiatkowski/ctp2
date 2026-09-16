#ifdef HAVE_PRAGMA_ONCE
#pragma once
#endif
#ifndef __NET_ARMY_H__
#define __NET_ARMY_H__

#include "net/general/net_packet.h"

class ArmyList;
class ArmyData;

typedef sint32 PLAYER_INDEX;
#include "gs/outcom/AICause.h"
#include "gs/world/cellunitlist.h"

class NetNewArmy : public Packetizer
{
public:
	NetNewArmy(PLAYER_INDEX player, const ArmyList &army, sint32 armyIndex,
			   CAUSE_NEW_ARMY cause);
	NetNewArmy() = default;

	void Packetize(uint8 *buf, uint16 &size) override;
	void Unpacketize(uint16 id, uint8 *buf, uint16 size) override;
private:
	// Packet bodies were never implemented (legacy multiplayer is stubbed);
	// the write-only mirror fields of the ctor args are gone.
};

class NetRemoveArmy : public Packetizer
{
public:
	NetRemoveArmy(PLAYER_INDEX player, const ArmyList &army, sint32 armyIndex,
				  CAUSE_REMOVE_ARMY cause);
	NetRemoveArmy() = default;

	void Packetize(uint8 *buf, uint16 &size) override;
	void Unpacketize(uint16 id, uint8 *buf, uint16 size) override;


};

class NetArmy : public Packetizer
{
public:
	NetArmy(ArmyData *data);
	NetArmy() = default;

	void Packetize(uint8 *buf, uint16 &size) override;
	void Unpacketize(uint16 id, uint8 *buf, uint16 size) override;
private:
	ArmyData *m_data;
};

class NetGroupRequest:public Packetizer
{
  public:
	NetGroupRequest(const CellUnitList &units, const Army &army);
	NetGroupRequest() = default;

	void Packetize(uint8 *buf, uint16 &size) override;
	void Unpacketize(uint16 id, uint8 *buf, uint16 size) override;

  private:
	uint32 m_armyId;
	CellUnitList m_units;
};

class NetUngroupRequest:public Packetizer
{
  public:
	NetUngroupRequest(const Army &army, const CellUnitList &units);
	NetUngroupRequest() = default;

	void Packetize(uint8 *buf, uint16 &size) override;
	void Unpacketize(uint16 id, uint8 *buf, uint16 size) override;

  private:
	uint32 m_armyId;
	CellUnitList m_units;
};
#endif
