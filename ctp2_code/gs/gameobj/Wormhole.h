#pragma once
#ifndef __WORMHOLE_H__
#define __WORMHOLE_H__

#include "gs/world/MapPoint.h"
#include "gs/gameobj/Unit.h"

class CivArchive;
class GoodActor;
template <class T> class PointerList;

class EntryRecord {
public:
	EntryRecord(const Unit &unit, sint32 round)
	{
		m_unit = unit;
		m_round = round;
	}
	EntryRecord() = default;

	Unit m_unit;
	sint32 m_round;
};

class Wormhole
{
private:
	MapPoint m_pos;
	WORLD_DIRECTION m_curDir;
	sint32 m_horizontalMoves;
	sint32 m_topY, m_bottomY;
	sint32 m_discoverer;
	GoodActor	*m_actor;

	sint32 m_discoveredAt;




	PointerList<EntryRecord> *m_entries;
	void Move();

	friend class NetWormhole;

public:
	Wormhole(sint32 discoverer, sint32 currentRound);
	Wormhole(sint32 discoverer, MapPoint &startPos, sint32 currentRound);

	~Wormhole();

	BOOL IsVisible(sint32 player, sint32 currentRound) const;

	BOOL CheckEnter(const Unit &unit, sint32 currentRound);
	void BeginTurn(sint32 player);

	sint32 GetDiscoveredAt() const { return m_discoveredAt; }

	GoodActor *GetActor() { return m_actor;}
	MapPoint	GetPos() { return m_pos; }
};

// Lifecycle (new from archive / NULL / cleanup) lives in
// gs/utility/gameinit.cpp; the variable is file-scope `static` there.
// External readers go through wormhole_Get(); the cleanup path uses
// wormhole_Set(NULL) after destroying the instance.
Wormhole * wormhole_Get();
void       wormhole_Set(Wormhole *w);

#endif
