#include "ctp/c3.h"

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
	ArmyData *newData;
	Army newArmy(NewKey(k_BIT_GAME_OBJ_TYPE_ARMY));

	newData = new ArmyData(newArmy, units);
	Insert(newData);
	return newArmy;
}

Army ArmyPool::Create(CellUnitList &units)
{
	ArmyData *newData;
	Army newArmy(NewKey(k_BIT_GAME_OBJ_TYPE_ARMY));

	newData = new ArmyData(newArmy, units);
	Insert(newData);
	return newArmy;
}

Army ArmyPool::Create(const Unit &u)
{
	ArmyData *newData;
	Army newArmy(NewKey(k_BIT_GAME_OBJ_TYPE_ARMY));

	Unit bleah(u);
	newData = new ArmyData(newArmy, bleah);
	Insert(newData);
	return newArmy;
}

Army ArmyPool::Create()
{
	ArmyData *newData;
	Army newArmy(NewKey(k_BIT_GAME_OBJ_TYPE_ARMY));
	newData = new ArmyData(newArmy);
	Insert(newData);
	return newArmy;
}

void ArmyPool::Remove(Army army)
{
	Del(army);
}


