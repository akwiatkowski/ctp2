//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ source
// Description  : Queue a Move-Order event along a computed path
//
//----------------------------------------------------------------------------

#include "ctp/c3.h"
#include "gs/gameobj/MovePath.h"

#include "gs/gameobj/Army.h"
#include "gs/gameobj/ArmyData.h"
#include "gs/world/MapPoint.h"
#include "gs/events/GameEventManager.h"
#include "robot/pathing/Path.h"
#include "robot/pathing/UnitAstar.h"

extern UnitAstar *g_theUnitAstar;

bool army_QueueMovePath(sint32 owner, Army &army,
                        const MapPoint &src, const MapPoint &dest)
{
	Path *good_path = new Path;
	Path bad_path;
	bool is_broken = false;
	float cost = 0.0f;

	sint32 r = g_theUnitAstar->FindPath(army, src,
	                                    owner, dest,
	                                    *good_path, is_broken,
	                                    bad_path,
	                                    cost);
	if (!r || is_broken) {
		delete good_path;
		return false;
	}

	army.ClearOrders();
	good_path->JustSetStart(army->RetPos());

	g_gevManager->AddEvent(GEV_INSERT_Tail, GEV_MoveOrder,
	                       GEA_Army, army,
	                       GEA_Path, good_path,
	                       GEA_MapPoint, src,
	                       GEA_Int, 0,
	                       GEA_End);
	return true;
}
