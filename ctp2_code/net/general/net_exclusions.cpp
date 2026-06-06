#include "ctp/c3.h"
#include "net/general/net_exclusions.h"
#include "gs/gameobj/Exclusions.h"
#include "net/io/net_util.h"

#include <memory>

void NetExclusions::Packetize(uint8 *buf, uint16 &size)
{
	PUSHID(k_PACKET_EXCLUSIONS_ID);
	PUSHLONG(exclusions_Get()->m_numUnits);
	PUSHLONG(exclusions_Get()->m_numBuildings);
	PUSHLONG(exclusions_Get()->m_numWonders);

	sint32 i;
	sint32 bitPos;
	uint8 next;
	for(i = 0; i < exclusions_Get()->m_numUnits; i += 8) {
		next = 0;
		for(bitPos = 0; (bitPos < 8) && ((i + bitPos) < exclusions_Get()->m_numUnits); bitPos++) {
			if(exclusions_Get()->IsUnitExcluded(i + bitPos))
				next |= 1 << bitPos;
		}
		PUSHBYTE(next);
	}

	for(i = 0; i < exclusions_Get()->m_numBuildings; i += 8) {
		next = 0;
		for(bitPos = 0; (bitPos < 8) && ((i + bitPos) < exclusions_Get()->m_numBuildings); bitPos++) {
			if(exclusions_Get()->IsBuildingExcluded(i + bitPos))
				next |= 1 << bitPos;
		}
		PUSHBYTE(next);
	}

	for(i = 0; i < exclusions_Get()->m_numWonders; i += 8) {
		next = 0;
		for(bitPos = 0; (bitPos < 8) && ((i + bitPos) < exclusions_Get()->m_numWonders); bitPos++) {
			if(exclusions_Get()->IsWonderExcluded(i + bitPos))
				next |= 1 << bitPos;
		}
		PUSHBYTE(next);
	}
}

void NetExclusions::Unpacketize(uint16 id, uint8 *buf, uint16 size)
{
	uint16 packid;
	uint16 pos = 0;

	PULLID(packid);
	Assert(packid == k_PACKET_EXCLUSIONS_ID);

	if(!exclusions_Get()) {
		exclusions_Set(new Exclusions());
	}

	PULLLONG(exclusions_Get()->m_numUnits);
	PULLLONG(exclusions_Get()->m_numBuildings);
	PULLLONG(exclusions_Get()->m_numWonders);

	exclusions_Get()->m_units = std::make_unique<sint32[]>(exclusions_Get()->m_numUnits);
	exclusions_Get()->m_buildings = std::make_unique<sint32[]>(exclusions_Get()->m_numBuildings);
	exclusions_Get()->m_wonders = std::make_unique<sint32[]>(exclusions_Get()->m_numWonders);
	memset(exclusions_Get()->m_units.get(), 0, sizeof(sint32) * exclusions_Get()->m_numUnits);
	memset(exclusions_Get()->m_buildings.get(), 0, sizeof(sint32) * exclusions_Get()->m_numBuildings);
	memset(exclusions_Get()->m_wonders.get(), 0, sizeof(sint32) * exclusions_Get()->m_numWonders);

	sint32 i;
	sint32 bitPos;
	uint8 next;

	for(i = 0; i < exclusions_Get()->m_numUnits; i += 8) {
		PULLBYTE(next);
		for(bitPos = 0; (bitPos < 8) && ((i + bitPos) < exclusions_Get()->m_numUnits); bitPos++) {
			if(next & (1 << bitPos)) {
				exclusions_Get()->ExcludeUnit(i + bitPos, TRUE);
			}
		}
	}

	for(i = 0; i < exclusions_Get()->m_numBuildings; i += 8) {
		PULLBYTE(next);
		for(bitPos = 0; (bitPos < 8) && ((i + bitPos) < exclusions_Get()->m_numBuildings); bitPos++) {
			if(next & (1 << bitPos)) {
				exclusions_Get()->ExcludeBuilding(i + bitPos, TRUE);
			}
		}
	}

	for(i = 0; i < exclusions_Get()->m_numWonders; i += 8) {
		PULLBYTE(next);
		for(bitPos = 0; (bitPos < 8) && ((i + bitPos) < exclusions_Get()->m_numWonders); bitPos++) {
			if(next & (1 << bitPos)) {
				exclusions_Get()->ExcludeWonder(i + bitPos, TRUE);
			}
		}
	}
}
