#pragma once
#ifndef __ROBOT_ASTAR_H__
#define __ROBOT_ASTAR_H__ 1

#include <objbase.h>
#include "gs/outcom/IC3RobotAstar.h"
#include "gs/world/MapPoint.h"

class Player;
class CivArchive;

class RobotAstar : public IC3RobotAstar
{
	ULONG m_refCount;
	Player *m_player;

public:
	RobotAstar(Player *p);
	RobotAstar(Player *p, CivArchive &archive);
	~RobotAstar();

	STDMETHODIMP QueryInterface(REFIID riid, void **obj) {
		*obj = nullptr;
		return E_NOINTERFACE;
	}
	STDMETHODIMP_(ULONG) AddRef() { return ++m_refCount; }
	STDMETHODIMP_(ULONG) Release() {
		if (--m_refCount) return m_refCount;
		delete this;
		return 0;
	}

	void Serialize(CivArchive &archive);

	BOOL FindPath(
		RobotPathEval *cb,
		uint32 army_id,
		PATH_ARMY_TYPE pat,
		uint32 army_type,
		MapPointData *start,
		MapPointData *dest,
		sint32 *bufSize,
		MapPointData **buffer,
		sint32 *nPoints,
		float *total_cost,
		BOOL made_up_can_space_launch,
		BOOL made_up_can_space_land,
		BOOL check_rail_launch,
		BOOL pretty_path,
		sint32 cutoff,
		sint32 &nodes_opened,
		BOOL check_dest,
		BOOL no_straigth_line,
		const BOOL check_units_in_cell
	);
};

#endif
