//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ header
// Description  : Tile information.
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
// - Standardized casts and removed some warnings in .NET.
//   - April 23rd 2005 Martin G�hmann
//
//----------------------------------------------------------------------------
//
/// \file   TileInfo.h
/// \brief  Tile information (declarations)

#if defined(HAVE_PRAGMA_ONCE)
#pragma once
#endif

#ifndef TILEINFO_H__
#define TILEINFO_H__

//----------------------------------------------------------------------------
// Library dependencies
//----------------------------------------------------------------------------

// None

//----------------------------------------------------------------------------
// Export overview
//----------------------------------------------------------------------------

class TileInfo;

#define k_NUM_TRANSITIONS	4

#define k_MASK_MEGA_NEXT	0x0F
#define k_MASK_MEGA_LAST	0xF0
#define k_SHIFT_MEGA_NEXT	0
#define k_SHIFT_MEGA_LAST	4

//----------------------------------------------------------------------------
// Project dependencies
//----------------------------------------------------------------------------

#include "os/include/ctp2_inttypes.h"
#include "gs/world/MapPoint.h"

class GoodActor;
class CivArchive;

//----------------------------------------------------------------------------
// Class declarations
//----------------------------------------------------------------------------

#include <nlohmann/json.hpp>

typedef uint16 TILEINDEX;

class TileInfo
{
public:
	TileInfo();
	TileInfo(TileInfo *copy);
	~TileInfo();

	uint8 GetTerrainType() const { return m_terrainType; }
	void SetTerrainType(uint8 type) { m_terrainType = type; }

	uint8 GetTransform() const { return m_transform; }
	void SetTransform(uint8 trans) { m_transform = trans; }

	TILEINDEX GetTileNum();

	void SetTileNum(TILEINDEX index) { m_tileNum = index; }

	sint16 GetRiverPiece() const { return m_riverPiece; }
	void SetRiverPiece(sint16 river) { m_riverPiece = static_cast<sint8>(river); }

	void SetGoodActor(sint32 index, MapPoint const & pos);
	GoodActor * GetGoodActor() const {return m_goodActor;}
	bool HasGoodActor() const { return m_goodActor != nullptr; }
	void DeleteGoodActor();

	void SetTransition(uint16 transNum, uint16 index) { m_transitions[transNum] = (sint8)index; }
	uint16 GetTransition(uint16 transNum) { return m_transitions[transNum]; }

	sint32 GetNextMega() { return static_cast<sint32>((m_megaInfo & k_MASK_MEGA_NEXT) >> k_SHIFT_MEGA_NEXT); }
	sint32 GetLastMega() { return static_cast<sint32>((m_megaInfo & k_MASK_MEGA_LAST) >> k_SHIFT_MEGA_LAST); }
	bool IsMega() const { return m_megaInfo != 0; }
	void SetNextMega(sint32 dir) { m_megaInfo = static_cast<sint8>((m_megaInfo & k_MASK_MEGA_NEXT) | ((dir << k_SHIFT_MEGA_NEXT) & k_MASK_MEGA_NEXT));}
	void SetLastMega(sint32 dir) { m_megaInfo = static_cast<sint8>((m_megaInfo & k_MASK_MEGA_LAST) | ((dir << k_SHIFT_MEGA_LAST) & k_MASK_MEGA_LAST));}

	uint8 GetMega() { return m_megaInfo; }
	void SetMega(uint8 m) { m_megaInfo = m; }

private:

	sint8			m_riverPiece;
	uint8			m_megaInfo;
	uint8			m_terrainType;
	uint8           m_transform;

	TILEINDEX		m_tileNum;

	uint8			m_transitions[k_NUM_TRANSITIONS];

	GoodActor		*m_goodActor;

	friend class NetCellList;
	friend class NetCell;
	friend class NetUnseenCell;

	// JSON savegame bridges (json_save.cpp).  Scalar fields only —
	// m_goodActor (UI sprite pointer) is omitted from the JSON schema
	// by design; the UI regenerates it from terrain + good_value on
	// load, mirroring the UnseenCell::m_actor handling.
	friend void to_json(nlohmann::json &j, TileInfo const &t);
	friend void from_json(nlohmann::json const &j, TileInfo &t);
};

#endif
