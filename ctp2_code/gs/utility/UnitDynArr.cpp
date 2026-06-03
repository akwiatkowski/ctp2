#include "ctp/c3.h"

#include "gs/utility/UnitDynArr.h"
#include "gs/world/MapPoint.h"


UnitDynamicArray::UnitDynamicArray() : DynamicArray<Unit>()

{
    }

UnitDynamicArray::UnitDynamicArray(const sint32 size) : DynamicArray<Unit>(size)

{
    }

UnitDynamicArray::UnitDynamicArray (const DynamicArray<Unit> &copyme) :
DynamicArray<Unit> (copyme)

{
    }
