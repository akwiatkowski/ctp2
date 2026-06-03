#ifdef HAVE_PRAGMA_ONCE
#pragma once
#endif
#ifndef __ARMY_POOL_H__
#define __ARMY_POOL_H__

#include "gs/gameobj/ObjPool.h"
#include "gs/gameobj/Army.h"

#include <nlohmann/json.hpp>

class Army;
class ArmyData;
class UnitDynamicArray;
class Unit;

class ArmyPool : public ObjPool
{










public:
	ArmyData *AccessArmy(const uint32 id)
	{
		return (ArmyData *)Access(id);
	}

	ArmyData *GetArmy(const uint32 id) const
	{
		return (ArmyData *)Get(id);
	}

	ArmyData *AccessArmy(const Army &id)
	{
		return (ArmyData *)Access(id);
	}

	ArmyData *GetArmy(const Army &id) const
	{
		return (ArmyData *)Get(id);
	}

	ArmyPool();
	ArmyPool(CivArchive &archive);
	~ArmyPool() override;

	void Serialize(CivArchive &archive) override;

	Army Create(UnitDynamicArray &units);
	Army Create(CellUnitList &units);
	Army Create(const Unit &u);
	Army Create();

	void Remove(Army army);

	// JSON bridge — mirrors ArmyPool::Serialize at ArmyPool.cpp:67.
	// Captures ObjPool base (m_id_type + m_nObjs key counter) plus
	// every live ArmyData entry.  Implementation in json_save.cpp
	// (needs ArmyData's to_json visible).
	friend void to_json(nlohmann::json &j, ArmyPool const &p);
	friend void from_json(nlohmann::json const &j, ArmyPool &p);
};

// Lifecycle (new / archive-load / Cleanup) lives in
// gs/utility/gameinit.cpp; the variable is now file-scope `static`
// there.  External readers go through armypool_Get().
ArmyPool * armypool_Get();
// Setter returns the previous value; used by Ctp2::Game's adoption path.
ArmyPool * armypool_Set(ArmyPool *p);

#endif
