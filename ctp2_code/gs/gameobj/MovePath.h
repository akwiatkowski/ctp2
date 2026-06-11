//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ header
// Description  : Queue a Move-Order event along a computed path
//
//----------------------------------------------------------------------------
//
// Body extracted from SelectedItem::EnterMovePath, which despite living in
// ui/aui_ctp2/SelItem.cpp is pure engine work: pathfind from src to dest,
// then enqueue a GEV_MoveOrder event.  Lifting it to gs/gameobj/ lets game
// logic (Player::GiveArmyCommand, armyevent, slicfunc) call it without
// depending on the UI subsystem.
//
//----------------------------------------------------------------------------

#pragma once

class Army;
class MapPoint;

// Pathfinds from src to dest for army owned by owner.  If a full path is
// found, queues a GEV_MoveOrder event and returns true.  If pathfinding
// fails or the path is broken, returns false.
bool army_QueueMovePath(sint32 owner, Army &army,
                        const MapPoint &src, const MapPoint &dest);

// Pathfinds from src to dest for army owned by owner.  If a full path is
// found, clears existing orders and directly adds a UNIT_ORDER_MOVE with the
// computed path (synchronous, unlike army_QueueMovePath).  Returns true on
// success, false if no path exists or pathfinding is unavailable.  Used by
// auto-explore, which must update the army order queue immediately so that
// subsequent BeginTurnUnitEvent checks see NumOrders() > 0.
bool army_AddMovePath(sint32 owner, Army &army,
                      const MapPoint &src, const MapPoint &dest);

// Like army_AddMovePath, but for explore targets, which are UNEXPLORED by
// definition: human-player pathfinding refuses unexplored destinations, so
// when the direct path fails this falls back to the nearest EXPLORED
// neighbour of the target (the frontier) — arriving there expands vision
// and the per-turn explore tick re-targets. Returns false only when neither
// the target nor any explored neighbour is reachable.
bool army_AddExplorePath(sint32 owner, Army &army,
                         const MapPoint &src, const MapPoint &target);
