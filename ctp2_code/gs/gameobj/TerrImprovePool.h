#ifdef HAVE_PRAGMA_ONCE
#pragma once
#endif
#ifndef _TERRIMPROVEPOOL_H_
#define _TERRIMPROVEPOOL_H_

#include "gs/gameobj/ObjPool.h"

#include "gs/gameobj/TerrImprove.h"

#include <nlohmann/json.hpp>

class CivArchive;

class TerrainImprovementPool : public ObjPool
{
private:
public:
	TerrainImprovementPool();
	TerrainImprovementPool(CivArchive &archive);

	TerrainImprovementData *AccessTerrainImprovement(const TerrainImprovement id)
	{
		return (TerrainImprovementData*)Access(id);
	}

	TerrainImprovementData *GetTerrainImprovement(const TerrainImprovement id)
	{
		return (TerrainImprovementData*)Get(id);
	}

	TerrainImprovement Create(sint32 owner,
							  MapPoint const & pnt,
							  sint32 type,
							  sint32 extraData);
	void Remove(TerrainImprovement id);
	BOOL HasImprovement(const MapPoint &point,
						TERRAIN_IMPROVEMENT type,
						sint32 extraData);
	BOOL HasAnyImprovement(const MapPoint &point) ;
	BOOL CanHaveImprovement(const MapPoint &point,
							TERRAIN_IMPROVEMENT type,
							sint32 extraData);

	void Serialize(CivArchive &archive) override;

	// JSON bridge — mirrors TerrainImprovementPool::Serialize.  Persists
	// ObjPool key counter + every live TerrainImprovementData entry.
	friend void to_json(nlohmann::json &j, TerrainImprovementPool const &p);
	friend void from_json(nlohmann::json const &j, TerrainImprovementPool &p);
};

// g_theTerrainImprovementPool's lifecycle (new / archive-load / Cleanup)
// lives in gs/utility/gameinit.cpp; the variable is now file-scope
// `static` there.  External readers go through terrimprovepool_Get().
TerrainImprovementPool * terrimprovepool_Get();
void                     terrimprovepool_Set(TerrainImprovementPool *p);
#endif
