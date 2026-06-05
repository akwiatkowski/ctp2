//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ source
// Description  : World events
// Id           : $Id$
//
//----------------------------------------------------------------------------
//
// Disclaimer
//
// THIS FILE IS NOT GENERATED OR SUPPORTED BY ACTIVISION.
//
// This material has been developed at apolyton.net by the Apolyton CtP2
// Source Code Project. Contact the authors at ctp2source@apolyton.net.
//
//----------------------------------------------------------------------------
//
// Compiler flags
//
// - None
//
//----------------------------------------------------------------------------
//
// Modifications from the original Activision code:
//
// - Added GobalWarming and OzoneDepletion events. (29-Oct-2007 Martin G�hmann)
//
//----------------------------------------------------------------------------

#include "ctp/c3.h"
#include "gs/world/World.h"
#include "gs/world/Cell.h"
#include "gs/gameobj/Unit.h"
#include "gs/utility/directions.h"
#include "gs/events/GameEventUser.h"
#include "gs/events/GameEventManager.h"
#include "gs/gameobj/Events.h"
#include "gs/core/tiledmap_observer.h"
#include "net/general/network.h"
#include "gs/slic/SlicObject.h"
#include "gs/slic/SlicEngine.h"
#include "gs/world/cellunitlist.h"
#include "gs/outcom/AICause.h"
#include "gs/world/World.h"             // world_Get()

STDEHANDLER(CutImprovementsEvent)
{
	MapPoint pos;
	if(!args->GetPos(0, pos)) return GEV_HD_Continue;

	MapPoint npos;

	world_Get()->CutImprovements(pos);

	for(WORLD_DIRECTION d = NORTH; d < NOWHERE; d = (WORLD_DIRECTION)((sint32)d + 1))
	{
		if(pos.GetNeighborPosition(d, npos))
		{
			tiledmap_observer::PostProcessTile(npos, world_Get()->GetTileInfo(npos));
			tiledmap_observer::TileChanged(npos);
		}
	}

	tiledmap_observer::RedrawTile(pos);

	if(network_Get().IsHost())
	{
		Cell *cell = world_Get()->GetCell(pos);
		network_Get().Enqueue(cell, pos.x, pos.y);
	}

	static CellUnitList units;
	units.Clear();
	world_Get()->GetArmy(pos, units);

	sint32 i;
	sint32 unitsOwner = 0;
	if(units.Num() > 0) {
		unitsOwner = units.GetOwner();
	}

	for(i = units.Num() -1; i >= 0; i--)
	{
		sint32 num_killed = 0;
		if(!world_Get()->CanEnter(pos, units[i].GetMovementType())
		&& !world_Get()->HasCity(pos)
		){

			gevmanager_Get()->AddEvent(GEV_INSERT_Tail, GEV_KillUnit,
								   GEA_Unit, units[i],
								   GEA_Int, CAUSE_REMOVE_ARMY_ILLEGAL_CELL,
								   GEA_Player, -1,
								   GEA_End);

			num_killed++;
		}
		if (num_killed > 0)
		{
			SlicObject *so = new SlicObject("41IAPillageSuicide");
			so->AddRecipient(unitsOwner);
			slicengine_Get()->Execute(so);
		}
	}

	return GEV_HD_Continue;
}

STDEHANDLER(GlobalWarmingEvent)
{
	sint32 phase;
	if(!args->GetInt(0, phase))
		return GEV_HD_Continue;

	world_Get()->GlobalWarmingEvent(phase);

	return GEV_HD_Continue;
}

STDEHANDLER(OzoneDepletionEvent)
{
	world_Get()->OzoneDepletionEvent();

	return GEV_HD_Continue;
}

void worldevent_Initialize()
{
	gevmanager_Get()->AddCallback(GEV_CutImprovements, GEV_PRI_Primary, &s_CutImprovementsEvent);
	gevmanager_Get()->AddCallback(GEV_GlobalWarming,   GEV_PRI_Primary, &s_GlobalWarmingEvent);
	gevmanager_Get()->AddCallback(GEV_OzoneDepletion,  GEV_PRI_Primary, &s_OzoneDepletionEvent);
}

void worldevent_Cleanup()
{
}
