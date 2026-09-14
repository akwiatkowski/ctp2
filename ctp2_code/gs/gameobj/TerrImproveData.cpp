//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ source
// Description  : Terrain improvement data
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
// - Production time calculation improved, and safeguarded against negative
//   numbers.
// - Updates the graphics of tile improvements under contruction every
//   turn, so that the process to completeness of a tile improvements is
//   visualized. - Oct. 16th 2004 Martin G�hmann
// - Moved network handling from TerrainImprovementData constructor to prevent
//   reporting the temporary when completing the tile improvement.
// - Restored save game compatibilty. (April 22nd 2006 Martin G�hmann)
//
//----------------------------------------------------------------------------

#include "ctp/c3.h"
#include "gs/gameobj/XY_Coordinates.h"
#include "gs/world/World.h"
#include "gs/world/Cell.h"
#include "gs/gameobj/TerrImproveData.h"
#include "gs/core/tiledmap_observer.h"
#include "TerrainRecord.h"
#include "gs/gameobj/player.h"
#include "gs/gameobj/installation.h"
#include "gs/gameobj/Vision.h"
#include "gs/gameobj/TerrImprove.h"
#include "robot/aibackdoor/dynarr.h"
#include "gs/gameobj/installationtree.h"

#include "net/general/network.h"
#include "gs/gameobj/terrainutil.h"
#include "TerrainImprovementRecord.h"
#include "gs/gameobj/MaterialPool.h"

#include "gs/events/GameEventManager.h"
#include "gs/utility/directions.h"

#include "gs/gameobj/CityInfluenceIterator.h"

TerrainImprovementData::TerrainImprovementData(ID id,
											   sint32 owner,
											   MapPoint pnt,
											   sint32 type,
											   sint32 extraData)
:   GameObj             (id.m_id),
    m_owner             (owner),
    m_type              (type),
    m_point             (pnt),
	m_turnsToComplete   (terrainutil_GetProductionTime(type, pnt, extraData)),
	m_transformType     (extraData),
	m_materialCost      (terrainutil_GetProductionCost(type, pnt, extraData)),
    m_isComplete        (false),
    m_isBuilding        (false)
//	m_materialBonus     (terrainutil_GetBonusProductionExport(type, pnt, extraData)), //EMOD 4-5-2006
{
}

BOOL TerrainImprovementData::Complete()
{
	TerrainImprovement imp(m_id);

	Cell* theCell = world_Get()->GetCell(m_point);

	for(sint32 p = 0; p < k_MAX_PLAYERS; p++) {
		if(!player_Get(p)) continue;
		if(p == m_owner) continue;
		player_Get(p)->m_vision->AddUnseen(m_point);
	}
	DPRINTF(k_DBG_GAMESTATE, ("Completed improvement %d at (%d,%d)\n",
							  m_type, m_point.x, m_point.y));

	const TerrainImprovementRecord *rec = g_theTerrainImprovementDB->Get(m_type);
	if(!rec->GetClassTerraform() && !rec->GetClassOceanform())
	{
		theCell->InsertDBImprovement(m_type);
	}
	else
	{
		sint32 terr;
		if(rec->GetTerraformTerrainIndex(terr)) {
			world_Get()->ClearGoods(m_point.x, m_point.y);
			world_Get()->SmartSetTerrain(m_point, terr, 0);
		}
	}
	theCell->RemoveImprovement(imp);

	if(rec->GetClassTerraform() || rec->GetClassOceanform())
	{
		// Questionable, since we have the exclude classes anyway
		world_Get()->CutImprovements(m_point);
	}

	Assert(player_Get(m_owner));
	if(!player_Get(m_owner))
		return TRUE;

	if (terrainutil_IsInstallation(m_type))
	{
		player_Get(m_owner)->CreateInstallation( m_type, m_point );
	}

	theCell->CalcTerrainMoveCost();

	terrainutil_DoVision(m_point);

	sint32 intRad;
	sint32 sqRad;
	if (rec->GetIntBorderRadius(intRad) && rec->GetSquaredBorderRadius(sqRad))
	{
		GenerateBorders(m_point, m_owner, intRad, sqRad);
	}

	tiledmap_observer::PostProcessTile(m_point, world_Get()->GetTileInfo(m_point));
	tiledmap_observer::TileChanged(m_point);

	MapPoint pos;

	for(WORLD_DIRECTION d = NORTH; d < NOWHERE; d = (WORLD_DIRECTION)((sint32)d + 1))
	{
		if(m_point.GetNeighborPosition(d, pos))
		{
			tiledmap_observer::PostProcessTile(pos, world_Get()->GetTileInfo(pos));
			tiledmap_observer::TileChanged(pos);
			tiledmap_observer::RedrawTile(pos);
		}
	}

	tiledmap_observer::RedrawTile(m_point);
	if(network_Get().IsHost())
	{
		network_Get().Enqueue(theCell, m_point.x, m_point.y);
	}

	world_Get()->GetCell(m_point)->SetColor(1000);

	// Restored the original Kill: the illegal access is prevented in
	// Improvementevent.cpp, and the object has to be killed to enable
	// further tile improvements after terraforming.
	imp.Kill();

	return TRUE;
}

BOOL TerrainImprovementData::AddTurn(sint32 turns)
{
	if(!m_isBuilding)

		return FALSE;

// add turn here can be used to make tile imps grow?

	m_turnsToComplete -= turns;
	network_Get().Block(m_owner);
	ENQUEUE();
	network_Get().Unblock(m_owner);

// Added by Martin G�hmann to update the tileimprovement graphics,
// to indicate increasing completeness.
	if(m_turnsToComplete > 0) {// Is more often true
		tiledmap_observer::RedrawTile(m_point);
	}
	else{

		gevmanager_Get()->AddEvent(GEV_INSERT_Tail,
							   GEV_ImprovementComplete,
							   GEA_Improvement, TerrainImprovement(m_id),
							   GEA_End);

	}
	return FALSE;
}

//----------------------------------------------------------------------------
//
// Name       : TerrainImprovementData::PercentComplete
//
// Description: Get the completion percentage of the improvement.
//
// Parameters : -
//
// Globals    : -
//
// Returns    : sint32	: completion percentage
//
// Remark(s)  : -
//
//----------------------------------------------------------------------------
sint32 TerrainImprovementData::PercentComplete() const
{
	// Function replaced by Martin G�hmann
	// Original function always returns 10 instead of the total production turns.
	sint32 const	totalTurns =
		terrainutil_GetProductionTime(m_type, m_point, m_transformType);

	// Guard against negative numbers and division by 0.
	if ((m_turnsToComplete <= 0) || (totalTurns <= 0))
	{
		return 100;
	}
	else if (m_turnsToComplete >= totalTurns)
	{
		return 0;
	}

	return (100 * (totalTurns - m_turnsToComplete)) / totalTurns;
}

void TerrainImprovementData::StartBuilding()
{
	if(!player_Get(m_owner))
		return;

	if(player_Get(m_owner)->m_materialPool->GetMaterials() < m_materialCost)
		return;

	player_Get(m_owner)->m_materialPool->SubtractMaterials(m_materialCost);
	m_isBuilding = true;
}
