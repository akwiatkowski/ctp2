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

#include <algorithm>        // std::fill, std::copy
#include <memory>           // std::make_unique
#include "gfx/spritesys/GoodActor.h"
#include "gs/database/profileDB.h"      // profiledb_Get()

TileInfo::TileInfo()
:
    m_riverPiece    (-1),
    m_megaInfo      (0),
    m_terrainType   (0),
    m_transform     (static_cast<uint8>(rand() % 256)),
    m_tileNum       (0),
    m_goodActor     (nullptr)
{
    std::fill(m_transitions, m_transitions + k_NUM_TRANSITIONS, 0);
}

/// @todo Replace with proper copy constructor. This will crash when copy is NULL.
TileInfo::TileInfo(TileInfo *copy)
{
	*this = *copy;
}

TileInfo &TileInfo::operator=(const TileInfo &other)
{
	if (this == &other) return *this;

	m_riverPiece  = other.m_riverPiece;
	m_megaInfo    = other.m_megaInfo;
	m_terrainType = other.m_terrainType;
	m_transform   = other.m_transform;
	m_tileNum     = other.m_tileNum;
	std::copy(other.m_transitions, other.m_transitions + k_NUM_TRANSITIONS,
	          m_transitions);
	m_goodActor   = other.m_goodActor
	                ? std::make_unique<GoodActor>(*other.m_goodActor)
	                : nullptr;
	return *this;
}

TileInfo::~TileInfo()
{
}

TILEINDEX TileInfo::GetTileNum()
{
	return m_tileNum;
}

void TileInfo::SetGoodActor(sint32 index, MapPoint const & pos)
{
	m_goodActor = std::make_unique<GoodActor>(index, pos);

	if (profiledb_Get()->IsGoodAnim())
	{
		m_goodActor->FullLoad();
	}
}

void TileInfo::DeleteGoodActor()
{
	m_goodActor.reset();
}


