#ifdef HAVE_PRAGMA_ONCE
#pragma once
#endif
#ifndef __NET_PLAYERDATA_H__
#define __NET_PLAYERDATA_H__

#include "ctp/ctp2_utils/pointerlist.h"
#include "net/general/net_hash.h"
#include "robot/aibackdoor/dynarr.h"

#include <string>
#include <memory>


class UnitDynamicArray;




class PlayerData
{
public:
	PlayerData(char* name, uint16 id);

	~PlayerData();

	std::string m_name;
	uint16 m_id;
	sint32 m_index;
	sint32 m_group;
	std::unique_ptr<PointerList<Packetizer>> m_packetList;
	DynamicArray<Unit> m_createdUnits;
	DynamicArray<Army> m_createdArmies;
	GUID m_guid;

	NetHash m_unitHash;
	BOOL m_frozen;




	std::unique_ptr<PointerList<PointerList<Packetizer>::PointerListNode>> m_bookmarks;
	BOOL m_ready;
	sint32 m_blocked;

	sint32 m_totalTimeUsed;

	BOOL m_ackBeginTurn;
	BOOL m_sentResync;
	std::unique_ptr<UnitDynamicArray> m_createdCities;
};

#endif
