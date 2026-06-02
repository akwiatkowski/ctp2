#include "ctp/c3.h"
#include "gs/gameobj/TradeRoute.h"
#include "gs/gameobj/XY_Coordinates.h"
#include "gs/world/World.h"
#include "gs/gameobj/TradePool.h"
#include "gs/gameobj/Player.h"
#include "gs/gameobj/TradeRouteData.h"
#include "gs/core/render_observer.h"
#include "gs/world/MapPoint.h"
#include "robot/aibackdoor/dynarr.h"
#include "gs/world/Cell.h"
#include "gs/gameobj/UnitPool.h"
#include "gs/slic/SlicObject.h"
#include "gs/slic/SlicEngine.h"

#include "net/general/network.h"
#include "net/general/net_info.h"
#include "gs/core/game_observer.h"

bool TradeRoute::IsValid() const
{
	return tradepool_Get()->IsValid(m_id);
}

void TradeRoute::KillRoute(CAUSE_KILL_TRADE_ROUTE cause)
{
	TradeRoute tmp(*this);
	tmp.RemoveAllReferences(cause);
}

void TradeRoute::RemoveAllReferences(CAUSE_KILL_TRADE_ROUTE cause)
{
	render_observer::TradeActorDestroy(*this);
	TradeRouteData* data = AccessData();






	Unit source(data->GetSource()), dest(data->GetDestination());


	if(unitpool_Get()->IsValid(source))
		source.DelTradeRoute(*this);
	if(unitpool_Get()->IsValid(dest))
		dest.DelTradeRoute(*this);

	if(unitpool_Get()->IsValid(source) && unitpool_Get()->IsValid(dest)) {
		if(source.GetOwner() != dest.GetOwner()) {
			if(cause == CAUSE_KILL_TRADE_ROUTE_SENDER_KILLED) {
				SlicObject *so = new SlicObject("360SenderKilledTradeRoute");
				ROUTE_TYPE type;
				sint32 good;
				GetSourceResource(type, good);
				so->AddGood(good);
				so->AddCity(source);
				so->AddCity(dest);
				so->AddCivilisation(source.GetOwner());
				so->AddRecipient(dest.GetOwner());
				slicengine_Get()->Execute(so);
			}
		}
	}

    if ((NULL != player_arr_Get())  &&
        (NULL != player_Get(GetPayingFor()))) {
    	player_Get(GetPayingFor())->RemoveTradeRoute(*this, cause);
    }

	data->RemoveFromCells();

#ifdef RECIPROCAL_ROUTES

	if (GetRecip().IsValid())
	{
		AccessRecip().SetRecip(TradeRoute());
		AccessRecip().KillRoute();
	}
#endif




	if(g_network.IsHost()) {
		g_network.Enqueue(new NetInfo(NET_INFO_CODE_KILL_TRADE_ROUTE, (uint32)*this, (uint32)cause));
	} else if(g_network.IsClient()) {
		g_network.AddDeadUnit(m_id);
	}

	tradepool_Get()->Remove(*this);

	if (g_gameObservers) g_gameObservers->NotifyTradeChanged();
}

Unit TradeRoute::GetSource() const
{
	return GetData()->GetSource();
}

PLAYER_INDEX TradeRoute::GetOwner() const
{
	return GetData()->GetOwner();
}

PLAYER_INDEX TradeRoute::GetPayingFor() const
{
	return GetData()->GetPayingFor();
}

const TradeRouteData* TradeRoute::GetData() const
{
    Assert(tradepool_Get());
	return tradepool_Get()->GetTradeRoute(*this);
}

TradeRouteData* TradeRoute::AccessData() const
{
    Assert(tradepool_Get());
	return tradepool_Get()->AccessTradeRoute(*this);
}

Unit TradeRoute::GetDestination() const
{
	return GetData()->GetDestination();
}

double TradeRoute::GetCost() const
{
	return GetData()->GetCost();
}

const DynamicArray<MapPoint>* TradeRoute::GetPath() const
{
	return GetData()->GetPath();
}

const DynamicArray<MapPoint>* TradeRoute::GetSelectedPath() const
{
	return GetData()->GetSelectedPath();
}

void TradeRoute::GetSourceResource(ROUTE_TYPE &type, sint32 &resource) const
{
	GetData()->GetSourceResource(type, resource);
}

TradeRoute TradeRoute::GetRecip() const
{
	return GetData()->GetRecip();
}

TradeRoute TradeRoute::AccessRecip()
{
	return AccessData()->AccessRecip();
}

void TradeRoute::SetRecip(TradeRoute route)
{
	AccessData()->SetRecip(route);
}

BOOL TradeRoute::PassesThrough(sint32 player) const
{
	return GetData()->PassesThrough(player);
}

BOOL TradeRoute::CrossesWater() const
{
	return GetData()->CrossesWater();
}

BOOL TradeRoute::IsActive() const
{
	return GetData()->IsActive();
}

void TradeRoute::Activate()
{
	AccessData()->Activate();
}

void TradeRoute::Deactivate()
{
	AccessData()->Deactivate();
}

StringId TradeRoute::GetResourceName() const
{
	return GetData()->GetResourceName();
}

uint32 TradeRoute::GetColor() const
{
	return GetData()->GetColor();
}

uint32 TradeRoute::GetOutlineColor() const
{
	return GetData()->GetOutlineColor();
}

void TradeRoute::SetColor( uint32 color )
{
	 AccessData()->SetColor(color);
}

void TradeRoute::SetOutlineColor( uint32 color )
{
	 AccessData()->SetOutlineColor(color);
}

void TradeRoute::ReturnPath(const PLAYER_INDEX owner, DynamicArray<MapPoint> &waypoints,
							DynamicArray<MapPoint> &fullpath,
							double &cost)
{
	AccessData()->ReturnPath(owner, waypoints, fullpath, cost);
}

void TradeRoute::SetPath(DynamicArray<MapPoint> &fullpath,
						 DynamicArray<MapPoint> &waypoints)
{
	AccessData()->SetPath(fullpath, waypoints);
}

void TradeRoute::GenerateSelectedPath(const MapPoint &pos)
{
	AccessData()->GenerateSelectedPath(pos);
}

sint32 TradeRoute::AddSelectedWayPoint(const MapPoint &pos)
{
	return AccessData()->AddSelectedWayPoint(pos);
}

BOOL TradeRoute::IsSelectedPathSame()
{
	return AccessData()->IsSelectedPathSame();
}

BOOL TradeRoute::IsPosInSelectedPath(const MapPoint &pos)
{
	return AccessData()->IsPosInSelectedPath(pos);
}

BOOL TradeRoute::IsPosInPath(const MapPoint &pos)
{
	return AccessData()->IsPosInPath(pos);
}

void TradeRoute::UpdateSelectedCellData(TradeRoute route)
{
	AccessData()->UpdateSelectedCellData(route);
}

void TradeRoute::ClearSelectedCellData(TradeRoute route)
{
	AccessData()->ClearSelectedCellData(route);
}

BOOL TradeRoute::InitSelectedData()
{
	return AccessData()->InitSelectedData();
}

void TradeRoute::ClearSelectedPath()
{
	AccessData()->ClearSelectedPath();
}

sint32 TradeRoute::GetPathSelectionState() const
{
	return GetData()->GetPathSelectionState();
}

void TradeRoute::SetPathSelectionState( sint32 state )
{
	AccessData()->SetPathSelectionState(state);
}

void TradeRoute::BeginTurn()
{
	AccessData()->BeginTurn();
}

sint32 TradeRoute::GetGoldInReturn() const
{
	return GetData()->GetGoldInReturn();
}

void TradeRoute::DontAdjustPointsWhenKilled()
{
	AccessData()->DontAdjustPointsWhenKilled();
}
