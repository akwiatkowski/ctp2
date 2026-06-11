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
#include "gs/gameobj/Player.h"       // player_Get, IsExplored (explore frontier)
#include "gs/world/MapPoint.h"
#include "gs/utility/directions.h"   // WORLD_DIRECTION
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

bool army_AddExplorePath(sint32 owner, Army &army,
                         const MapPoint &src, const MapPoint &target)
{
	// Direct first — works for AI players, whose pathfinding may enter
	// unexplored terrain.
	if (army_AddMovePath(owner, army, src, target))
		return true;

	// Human pathfinding refuses unexplored destinations, and an explore
	// target is unexplored BY DEFINITION. Two fallbacks:
	//
	// 1. Already standing next to the target (ON the frontier): step
	//    straight in with a point MOVE_TO order — adjacent moves need no
	//    pathfinding (and are exactly how a human "walks into the fog").
	if (src.IsNextTo(target)) {
		army.ClearOrders();
		army.AddOrders(UNIT_ORDER_MOVE_TO, target);
		return true;
	}

	// 2. Otherwise walk to the frontier: an explored neighbour of the
	//    target. Vision expands on arrival and the per-turn explore tick
	//    picks the next target (then case 1 applies).
	Player * pl = player_Get(owner);
	if (!pl) return false;

	for (sint32 d = sint32(NORTH); d < sint32(NOWHERE); ++d) {
		MapPoint step;
		if (!target.GetNeighborPosition(WORLD_DIRECTION(d), step)) continue;
		if (!pl->IsExplored(step)) continue;
		if (step == src) continue;  // handled by case 1 next tick
		if (army_AddMovePath(owner, army, src, step))
			return true;
	}
	return false;
}
