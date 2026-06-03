//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ source
// Description  :
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
//
//----------------------------------------------------------------------------
//
// Modifications from the original Activision code:
//
// - Repaired memory leak.
//
//----------------------------------------------------------------------------
//
/// \file   TileInfo.cpp
/// \brief  Tile information (definitions)

#include "ctp/c3.h"
#include "gs/world/TileInfo.h"

#include <algorithm>        // std::fill
#include "robot/aibackdoor/civarchive.h"
#include "gfx/spritesys/GoodActor.h"
#include "gs/database/profileDB.h"      // profiledb_Get()

TileInfo::TileInfo()
:
    m_riverPiece    (-1),
    m_megaInfo      (0),
    m_terrainType   (0),
    m_transform     (static_cast<uint8>(rand() % 256)),
    m_tileNum       (0),
    m_goodActor     (NULL)
{
    std::fill(m_transitions, m_transitions + k_NUM_TRANSITIONS, 0);
}

/// @todo Replace with proper copy constructor. This will crash when copy is NULL.
TileInfo::TileInfo(TileInfo *copy)
{
	*this = *copy;

	if (copy->m_goodActor)
    {
		m_goodActor = new GoodActor(*copy->m_goodActor);
	}
    // else: already set in *this = *copy;
}

TileInfo::TileInfo(CivArchive &archive)
:
    m_riverPiece    (-1),
    m_megaInfo      (0),
    m_terrainType   (0),
    m_transform     (0),
    m_tileNum       (0),
    m_goodActor     (NULL)
{
	Serialize(archive);
}

TileInfo::~TileInfo()
{
	delete m_goodActor;
}

TILEINDEX TileInfo::GetTileNum()
{
	return m_tileNum;
#if 0   // Unreachable code
	if (g_fog_toggle)
		return m_tileNum;
	if ( g_god ) {
		return m_tileNum;
	}

	if (!player_Get(selitem_Get()->GetVisiblePlayer())) return m_tileNum;

	if(player_Get(selitem_Get()->GetVisiblePlayer())->m_hasGlobalRadar) {
		return m_tileNum;
	} else if (player_Get(selitem_Get()->GetVisiblePlayer()) &&
		player_Get(selitem_Get()->GetVisiblePlayer())->GetDeepOceanVisible()) {
		return m_tileNum;
	} else {
		if(g_theTerrainDB->Get(m_terrainType)->GetMovementTypeSea() &&
			m_terrainType != TERRAIN_WATER_BEACH) {
			return k_OCEAN_BEFORE_OCEANOGRAPHY_BASE_TILE;
		} else {
			return m_tileNum;
		}
	}
#endif  // unreachable code
}

void TileInfo::SetGoodActor(sint32 index, MapPoint const & pos)
{
    delete m_goodActor;
	m_goodActor = new GoodActor(index, pos);

	if (profiledb_Get()->IsGoodAnim())
	{
		m_goodActor->FullLoad();
	}
}

void TileInfo::DeleteGoodActor()
{
	delete m_goodActor;
	m_goodActor = NULL;
}

void TileInfo::Serialize(CivArchive &archive)
{
	sint32 hasGoodActor;

	if(archive.IsStoring()) {
		DPRINTF(k_DBG_GRAPHICS, ("%d,%d,%d,%d, %d,%d,%d,%d\n",
								 m_riverPiece, m_megaInfo,
								 m_terrainType, m_tileNum,
								 m_transitions[0], m_transitions[1],
								 m_transitions[2], m_transitions[3]));
		archive << m_riverPiece;
		archive << m_megaInfo;
		archive << m_terrainType;
		archive << m_tileNum;
		archive << m_transform;
		for (size_t i = 0; i < k_NUM_TRANSITIONS; i++)
        {
			archive << m_transitions[i];
		}
		hasGoodActor = (m_goodActor != NULL);
		archive << hasGoodActor;
		if(m_goodActor)
			m_goodActor->Serialize(archive);
	} else {
		archive >> m_riverPiece;
		archive >> m_megaInfo;
		archive >> m_terrainType;
		archive >> m_tileNum;
		archive >> m_transform;
		for (size_t i = 0; i < k_NUM_TRANSITIONS; i++)
        {
			archive >> m_transitions[i];
		}
		archive >> hasGoodActor;

        delete m_goodActor;
        m_goodActor = (hasGoodActor) ? new GoodActor(archive) : NULL;
	}
}
