#include "ctp/c3.h"
#include "gs/outcom/RobotAstar.h"
#include "gs/gameobj/Player.h"
#include "civarchive.h"

RobotAstar::RobotAstar(Player *p)
{
	m_refCount = 0;
	m_player = p;
}

RobotAstar::RobotAstar(Player *p, CivArchive &archive)
{
	m_refCount = 0;
	m_player = p;
	Serialize(archive);
}

RobotAstar::~RobotAstar()
{
}

void RobotAstar::Serialize(CivArchive &archive)
{
	if (archive.IsStoring()) {
		archive << m_refCount;
	} else {
		archive >> m_refCount;
	}
}

BOOL RobotAstar::FindPath(
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
	const BOOL check_units_in_cell)
{
	// Stub: pathfinding not yet implemented
	return FALSE;
}
