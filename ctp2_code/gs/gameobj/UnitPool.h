//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ header
// Description  : The unit pool
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
// - None
//
//----------------------------------------------------------------------------

#ifdef HAVE_PRAGMA_ONCE
#pragma once
#endif

#ifndef __UNITPOOL_H__
#define __UNITPOOL_H__ 1

class UnitPool;

#include "gs/gameobj/ObjPool.h"

#include "gs/gameobj/Unit.h"

#include <nlohmann/json.hpp>

class UnitData;
class CivArchive;
class MapPoint;
class UnitData;
class UnitRecord;

#define k_UNITPOOL_VERSION_MAJOR	0
#define k_UNITPOOL_VERSION_MINOR	0

class UnitPool : public ObjPool
{
public:
	UnitPool();
	UnitPool(CivArchive &archive) ;

	Unit Create (sint32 t, const PLAYER_INDEX owner, const MapPoint &pos, const Unit hc, std::shared_ptr<UnitActor> actor = std::shared_ptr<UnitActor>());
	Unit Create (sint32 t, const PLAYER_INDEX owner, const MapPoint &actor_pos);

	UnitData * AccessUnit(uint32 id) {  return (UnitData * ) Access(id); };
	UnitData * GetUnit(uint32 id) const { return (UnitData *) Get(id); };

	const UnitRecord * GetDBRec(const Unit id) const;

	void Serialize(CivArchive &archive) override ;
	void RebuildQuadTree();

	// JSON bridge — mirrors UnitPool::Serialize at UnitPool.cpp:129.
	// Persists ObjPool key counter + every live UnitData entry.
	// Implementation in json_save.cpp.
	friend void to_json(nlohmann::json &j, UnitPool const &p);
	friend void from_json(nlohmann::json const &j, UnitPool &p);
};

// g_theUnitPool is now file-static in gameinit.cpp; external access goes
// through the accessor pair below.
UnitPool * unitpool_Get();
void       unitpool_Set(UnitPool *p);

uint32 UnitPool_UnitPool_GetVersion() ;
#else

class UnitPool;

UnitPool * unitpool_Get(void);
void       unitpool_Set(UnitPool *p);

#endif
