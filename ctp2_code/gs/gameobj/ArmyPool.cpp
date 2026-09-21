#include "ctp/c3.h"
#include <memory>

#include "gs/gameobj/ArmyPool.h"
#include "gs/gameobj/Army.h"
#include "gs/gameobj/ArmyData.h"
#include "gs/utility/Globals.h"

ArmyPool::ArmyPool() : ObjPool(k_BIT_GAME_OBJ_TYPE_ARMY)
{
}

ArmyPool::~ArmyPool()
= default;

Army ArmyPool::Create(UnitDynamicArray &units)
{
	Army newArmy(NewKey(k_BIT_GAME_OBJ_TYPE_ARMY));
	auto newData = std::make_unique<ArmyData>(newArmy, units);
	Insert(newData.release());
	return newArmy;
}

Army ArmyPool::Create(CellUnitList &units)
{
	Army newArmy(NewKey(k_BIT_GAME_OBJ_TYPE_ARMY));
	auto newData = std::make_unique<ArmyData>(newArmy, units);
	Insert(newData.release());
	return newArmy;
}

Army ArmyPool::Create(const Unit &u)
{
	Army newArmy(NewKey(k_BIT_GAME_OBJ_TYPE_ARMY));
	Unit bleah(u);
	auto newData = std::make_unique<ArmyData>(newArmy, bleah);
	Insert(newData.release());
	return newArmy;
}

Army ArmyPool::Create()
{
	Army newArmy(NewKey(k_BIT_GAME_OBJ_TYPE_ARMY));
	auto newData = std::make_unique<ArmyData>(newArmy);
	Insert(newData.release());
	return newArmy;
}

void ArmyPool::Remove(Army army)
{
	Del(army);
}


