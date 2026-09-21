#ifdef HAVE_PRAGMA_ONCE
#pragma once
#endif
#ifndef __SLIC_BUILTIN_H__
#define __SLIC_BUILTIN_H__

#include "gs/slic/SlicStack.h"
#include "gs/slic/SlicStruct.h"
#include "gs/slic/SlicBuiltinEnum.h"
#include "gs/slic/SlicSymbol.h"

#include <memory>

#define SLICSTRUCT(name, type) \
class SlicStruct_##name : public SlicStructDescription {\
public:\
    SlicStruct_##name();\
    std::unique_ptr<SlicSymbolData> CreateDataSymbol() override { return std::make_unique<SlicSymbolData>(type); }\
};







class SlicStruct_Global : public SlicStructDescription
{
public:
	SlicStruct_Global();
	std::unique_ptr<SlicSymbolData> CreateDataSymbol() override { return nullptr; }
};

SLICSTRUCT(Unit, SLIC_SYM_UNIT);
SLICSTRUCT(City, SLIC_SYM_CITY);
SLICSTRUCT(Player, SLIC_SYM_IVAR);
SLICSTRUCT(Army, SLIC_SYM_ARMY);
SLICSTRUCT(Location, SLIC_SYM_LOCATION);
SLICSTRUCT(Government, SLIC_SYM_IVAR);
SLICSTRUCT(Advance, SLIC_SYM_IVAR);
SLICSTRUCT(Action, SLIC_SYM_STRING);
SLICSTRUCT(Improvement, SLIC_SYM_IMPROVEMENT);
SLICSTRUCT(Value, SLIC_SYM_IVAR);
SLICSTRUCT(Building, SLIC_SYM_IVAR);
SLICSTRUCT(Wonder, SLIC_SYM_IVAR);
SLICSTRUCT(UnitRecord, SLIC_SYM_IVAR);
SLICSTRUCT(Gold, SLIC_SYM_IVAR);
SLICSTRUCT(Good, SLIC_SYM_IVAR);


#endif
