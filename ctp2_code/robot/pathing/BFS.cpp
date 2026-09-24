#include "ctp/c3.h"

#include "robot/pathing/BFS.h"

#include "robot/pathing/astarpnt.h"
#include "robot/pathing/Astar.h"

#include "gs/world/MapPoint.h"

#include "gs/gameobj/player.h"

#include "gs/world/Cell.h"
#include "gs/gameobj/XY_Coordinates.h"
#include "gs/world/World.h"

#include "gs/gameobj/Unit.h"
#include "gs/gameobj/UnitData.h"
#include "gs/gameobj/citydata.h"
#include "gs/gameobj/Happy.h"
#include "gs/gameobj/ArmyData.h"
#include "GovernmentRecord.h"

 // Search epoch + node pool now come from AstarPathing() (PathingContext).

BestFirstSearch::BestFirstSearch()
{
}

BestFirstSearch::~BestFirstSearch()
{
}

sint32  BestFirstSearch::FindNumCitiesAtHeight(const sint32 player_idx, const sint32 z_height)
{
    sint32 count=0;
    MapPoint city_pos;
    MapPoint capitol_pos;
    player_Get(player_idx)->GetCapitolPos(capitol_pos);

    sint32 city_idx;
    sint32 city_num=player_Get(player_idx)->m_all_cities->Num();
    for (city_idx=0; city_idx<city_num; city_idx++) {
        player_Get(player_idx)->m_all_cities->Access(city_idx).GetPos(city_pos);
        if (z_height != city_pos.z) continue;

        if ((capitol_pos.x == city_pos.x) &&
            (capitol_pos.y == city_pos.y) &&
            (capitol_pos.z == city_pos.z)) continue;

            count++;

    }

    return count;
}

BOOL BestFirstSearch::InitPoint(const sint32 player_idx, AstarPoint *parent, AstarPoint *&node,
    MapPoint &pos, const double past_cost, const double max_cost)
{

    Cell *the_cell = world_Get()->GetCell(pos);
    double entry_cost = the_cell->GetMoveCost();






























    node = AstarPathing().GetHeap().GetNew();

    node->m_is_zoc = FALSE;
    node->m_parent = parent;
    node->m_past_cost = past_cost;
    node->m_entry_cost = entry_cost;
    node->m_future_cost =0.0;

    node->m_total_cost = node->m_past_cost + node->m_entry_cost + node->m_future_cost;

    node->m_pos = pos;
    node->m_is_expanded = FALSE;
    node->m_queue_idx = -1;

    return TRUE;
}

void BestFirstSearch::FindMoveCostToCitiesZ(const sint32 player_idx, const sint32 z_height,
    const double max_cost, const double min_cost)
{
    sint32 const searchEpoch = AstarPathing().NextSearchEpoch();

    BOOL searching = TRUE;
    sint32 num_cities_found = 0;
    sint32 num_cities_out_there = FindNumCitiesAtHeight(player_idx, z_height);
    if (num_cities_out_there < 1) return;

    m_priority_queue.Clear();

    MapPoint start_pos;
    MapPoint neighbor_pos;
    double start_cost;
	player_Get(player_idx)->GetCapitolPos(start_pos);
    start_cost = -world_Get()->GetCell(start_pos)->GetMoveCost() + ((start_pos.z != z_height) ? 1000.0 : 0) ;
    start_pos.z = z_height;
    Cell *neighbor_cell = world_Get()->GetCell(start_pos);
    neighbor_cell->m_search_count = searchEpoch;
    InitPoint(player_idx, nullptr, neighbor_cell->m_point, start_pos, start_cost, max_cost);
    sint32 nodes_opened = 1;
    AstarPoint* best = neighbor_cell->m_point;
    double past_cost;

    sint32 i;
    sint32 bfs_loop_count =0;
    Unit a_city;
    double dist_cost;
    do {
        Assert(bfs_loop_count++ < 140000);

        best->m_is_expanded = TRUE;

        past_cost = best->m_total_cost;

        for (i=0; i <= 7; i++) {

            if (!best->m_pos.GetNeighborPosition(WORLD_DIRECTION(i), neighbor_pos)) continue;
            neighbor_cell = world_Get()->GetCell(neighbor_pos);

            if (neighbor_cell->m_search_count == searchEpoch)
                continue;

            if (InitPoint(player_idx, best, neighbor_cell->m_point, neighbor_pos, past_cost, max_cost)) {

world_Get()->SetColor(neighbor_pos, int(past_cost));
                nodes_opened++;
                neighbor_cell->m_search_count = searchEpoch;
                a_city = neighbor_cell->GetCity();
                if (a_city.m_id != 0) {
                    if (a_city.GetOwner() == player_idx) {

                        dist_cost = max(0.0, neighbor_cell->m_point->m_total_cost - min_cost);
                        a_city.AccessData()->GetCityData()->GetHappy()->SetDistToCapitol(dist_cost);
                        num_cities_found++;
                        if (num_cities_found == num_cities_out_there) {
                            searching = FALSE;
                            break;
                        }
                    }
                }
                m_priority_queue.Insert(neighbor_cell->m_point);
            }
        }

        if (!searching || (m_priority_queue.Len() < 1)) {
            break;
        } else {
            best = m_priority_queue.Remove(1);
        }

    } while (searching);


    AstarPathing().GetHeap().MassDelete(FALSE);
}

void BestFirstSearch::CalcDistToCapitol(const sint32 player_idx)
{

MapPoint pos;
MapPoint *size;

pos.z=0;
size = world_Get()->GetSize();
for (pos.x=0; pos.x<size->x; pos.x++) {
    for (pos.y=0; pos.y<size->y; pos.y++) {
        world_Get()->SetColor(pos, 0);
    }
}

    Assert(0 <= player_idx);
    Assert(player_idx < k_MAX_PLAYERS);
    Assert(player_Get(player_idx));
    if (!player_Get(player_idx)) return;


    double raw_max_cost = player_Get(player_idx)->GetMaxEmpireDistance();
    double raw_min_cost = g_theGovernmentDB->Get(player_Get(player_idx)->GetGovernmentType())->GetMinEmpireDistance();
    double dist_cost = max(0.0, raw_max_cost - raw_min_cost);

    sint32 city_idx;
    sint32 city_num;
    city_num = player_Get(player_idx)->m_all_cities->Num();

    BOOL searching;
    MapPoint start;
    if (player_Get(player_idx)->GetCapitolPos(start)) {
        searching = TRUE;
    } else {
        searching = FALSE;
        start.x=-1;
        start.y=-1;
        start.z=-1;
    }

    MapPoint city_pos;
    for (city_idx=0; city_idx<city_num; city_idx++) {

        player_Get(player_idx)->m_all_cities->Access(city_idx).GetPos(city_pos);
        if ((city_pos.x == start.x) && (city_pos.y == start.y) && (city_pos.z == start.z)) {
            player_Get(player_idx)->m_all_cities->Access(city_idx).AccessData()->GetCityData()->GetHappy()->SetDistToCapitol(0);
        } else {
            player_Get(player_idx)->m_all_cities->Access(city_idx).AccessData()->GetCityData()->GetHappy()->SetDistToCapitol(dist_cost);
        }
    }

    if (searching) {
        FindMoveCostToCitiesZ(player_idx, GROUND_Z, raw_max_cost, raw_min_cost);
        FindMoveCostToCitiesZ(player_idx, SPACE_Z, raw_max_cost, raw_min_cost);
    }
}
