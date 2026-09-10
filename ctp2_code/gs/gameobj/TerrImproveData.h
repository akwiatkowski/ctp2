//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ header
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
// HAVE_PRAGMA_ONCE
//
//----------------------------------------------------------------------------
//
// Modifications from the original Activision code:
//
// - Restored save game compatibilty. (April 22nd 2006 Martin G�hmann)
//
//----------------------------------------------------------------------------

#ifdef HAVE_PRAGMA_ONCE
#pragma once
#endif
#ifndef _TERRIMPROVEDATA_H_
#define _TERRIMPROVEDATA_H_

#include "gs/gameobj/GameObj.h"
#include "gs/world/MapPoint.h"
#include "gs/utility/gstypes.h"

#include <nlohmann/json.hpp>

typedef sint32 TERRAIN_IMPROVEMENT;

#include "gs/gameobj/ID.h"

class TerrainImprovementData : public GameObj
{
private:
//----------------------------------------------------------------------------
// Do not change anything in the types or order of the following variable
// declarations. Doing so will break reading in of save files.
// See the Serialize implementation for more details.
//----------------------------------------------------------------------------

    sint32 m_owner;
	sint32 m_type;
	MapPoint m_point;
	sint32 m_turnsToComplete;
	TERRAIN_TYPES m_transformType;
	sint32 m_materialCost;
	bool m_isComplete;
	bool m_isBuilding;

//----------------------------------------------------------------------------
// Changing the order below this line should not break anything.
//----------------------------------------------------------------------------

    // sint32 m_materialBonus;

    friend class NetTerrainImprovement;

public:
	TerrainImprovementData(ID id,
						   sint32 owner,
						   MapPoint pnt,
						   sint32 type,
						   sint32 extraData);
	TerrainImprovementData(ID id) :
		GameObj(id.m_id),
		m_owner(-1),
		m_type(0),
		m_point(),
		m_turnsToComplete(0),
		m_transformType(static_cast<TERRAIN_TYPES>(0)),
		m_materialCost(0),
		// Uninitialised bools serialize as non-0/1 integers; loading one
		// back is an invalid bool load and aborts under UBSan
		// halt_on_error (same class as Order::m_eventType).
		m_isComplete(false),
		m_isBuilding(false)
	{}

	sint32 GetType() const { return m_type; }

    sint32 GetOwner() const { return m_owner; }
	MapPoint GetLocation() const { return m_point; }
	sint32 GetCompletion() const { return m_turnsToComplete; }
	BOOL Complete();
	BOOL AddTurn(sint32 turns);
	sint32 PercentComplete() const;

//	sint32 GetBonusProductionExport() const { return m_materialBonus; } //EMOD 4-5-2006
	sint32 GetMaterialCost() const { return m_materialCost; }
	bool IsBuilding() const { return m_isBuilding; }

	void StartBuilding();

	// JSON bridge — mirrors TerrainImprovementData::Serialize.  Persists
	// GameObj id + 8 fields (owner, type, point, turns_to_complete,
	// transform_type, material_cost, is_complete, is_building).  Omits
	// m_lesser/m_greater (intrusive list, pool concern).
	friend void to_json(nlohmann::json &j, TerrainImprovementData const &d);
	friend void from_json(nlohmann::json const &j, TerrainImprovementData &d);
};

#endif
