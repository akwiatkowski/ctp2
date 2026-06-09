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

static bool army_ComputeMovePath(sint32 owner, Army &army,
                                 const MapPoint &src, const MapPoint &dest,
                                 Path *good_path)
{
	if (!g_theUnitAstar)
		return false;

	Path bad_path;
	bool is_broken = false;
	float cost = 0.0f;

	sint32 r = g_theUnitAstar->FindPath(army, src,
	                                    owner, dest,
	                                    *good_path, is_broken,
	                                    bad_path,
	                                    cost);
	return r && !is_broken;
}

bool army_QueueMovePath(sint32 owner, Army &army,
                        const MapPoint &src, const MapPoint &dest)
{
	Path *good_path = new Path;
	if (!army_ComputeMovePath(owner, army, src, dest, good_path)) {
		delete good_path;
		return false;
	}

	army.ClearOrders();
	good_path->JustSetStart(army->RetPos());

	gevmanager_Get()->AddEvent(GEV_INSERT_Tail, GEV_MoveOrder,
	                       GEA_Army, army,
	                       GEA_Path, good_path,
	                       GEA_MapPoint, src,
	                       GEA_Int, 0,
	                       GEA_End);
	return true;
}

bool army_AddMovePath(sint32 owner, Army &army,
                      const MapPoint &src, const MapPoint &dest)
{
	Path *good_path = new Path;
	if (!army_ComputeMovePath(owner, army, src, dest, good_path)) {
		delete good_path;
		return false;
	}

	army.ClearOrders();
	good_path->JustSetStart(army->RetPos());

	army.AddOrders(UNIT_ORDER_MOVE, good_path, src, 0);
	return true;
}
