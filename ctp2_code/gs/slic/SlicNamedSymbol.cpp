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
// - Guarded against crash with NULL source argument in strcpy.
//
//----------------------------------------------------------------------------

#include "ctp/c3.h"
#include "gs/slic/SlicNamedSymbol.h"

#include "gs/slic/SlicSymbol.h"
#include "gs/slic/SlicArray.h"
#include "gs/slic/SlicObject.h"
#include "gs/slic/SlicEngine.h"
#include "gs/slic/SlicFunc.h"
#include "gs/database/profileDB.h"

namespace
{
//  Some value that is different from k_NORMAL_FILE (0) and k_TUTORIAL_FILE (1).
    uint8 const k_GENERATED_BY_EXECUTABLE   = static_cast<uint8>(-1);
}

SlicNamedSymbol::SlicNamedSymbol(const char *name, SLIC_SYM type) :
	SlicSymbolData(type)
{
	Init(name);
}

SlicNamedSymbol::SlicNamedSymbol(const char *name) :
	SlicSymbolData(SLIC_SYM_UNDEFINED)
{
	Init(name);
}

SlicNamedSymbol::SlicNamedSymbol(const char *name, SlicArray *array) :
	SlicSymbolData(array)
{
	Init(name);
}

SlicNamedSymbol::SlicNamedSymbol(const char *name, SlicStructDescription *structDesc) :
	SlicSymbolData(structDesc)
{
	Init(name);
}

SlicNamedSymbol::~SlicNamedSymbol()
{
	delete [] m_name;
}

void SlicNamedSymbol::Init(const char *name)
{
    if (name)
    {
	    m_name = new char[strlen(name) + 1];
	    strcpy(m_name, name);
    }
    else
    {
        m_name = nullptr;
    }

	m_fromFile = k_GENERATED_BY_EXECUTABLE;
}

const char *SlicNamedSymbol::GetName() const
{
	return m_name;
}

SlicParameterSymbol::SlicParameterSymbol(const char *name, sint32 index) :
	SlicNamedSymbol(name)
{
	m_parameterIndex = index;
}

BOOL SlicParameterSymbol::GetIntValue(sint32 &value) const
{
	SlicObject *context = slicengine_Get()->GetContext();
	SlicArgList *argList = context->GetArgList();
	if(!argList) {
		if(profiledb_Get() && profiledb_Get()->IsDebugSlic()) {
			c3errors_ErrorDialog("Slic", "Parameter %s used outside function call", GetName());
		}
		return FALSE;
	}
	return argList->GetInt(m_parameterIndex, value);
}

BOOL SlicParameterSymbol::GetPlayer(sint32 &value) const
{
	SlicObject *context = slicengine_Get()->GetContext();
	SlicArgList *argList = context->GetArgList();
	if(!argList) {
		if(profiledb_Get() && profiledb_Get()->IsDebugSlic()) {
			c3errors_ErrorDialog("Slic", "Parameter %s used outside function call", GetName());
		}
		return FALSE;
	}
	return argList->GetPlayer(m_parameterIndex, value);
}

BOOL SlicParameterSymbol::GetPos(MapPoint &pos) const
{
	SlicObject *context = slicengine_Get()->GetContext();
	SlicArgList *argList = context->GetArgList();
	if(!argList) {
		if(profiledb_Get() && profiledb_Get()->IsDebugSlic()) {
			c3errors_ErrorDialog("Slic", "Parameter %s used outside function call", GetName());
		}
		return FALSE;
	}
	return argList->GetPos(m_parameterIndex, pos);
}

BOOL SlicParameterSymbol::GetUnit(Unit &u) const
{
	SlicObject *context = slicengine_Get()->GetContext();
	SlicArgList *argList = context->GetArgList();
	if(!argList) {
		if(profiledb_Get() && profiledb_Get()->IsDebugSlic()) {
			c3errors_ErrorDialog("Slic", "Parameter %s used outside function call", GetName());
		}
		return FALSE;
	}
	return argList->GetUnit(m_parameterIndex, u);
}

BOOL SlicParameterSymbol::GetArmy(Army &a) const
{
	SlicObject *context = slicengine_Get()->GetContext();
	SlicArgList *argList = context->GetArgList();
	if(!argList) {
		if(profiledb_Get() && profiledb_Get()->IsDebugSlic()) {
			c3errors_ErrorDialog("Slic", "Parameter %s used outside function call", GetName());
		}
		return FALSE;
	}
	return argList->GetArmy(m_parameterIndex, a);
}

BOOL SlicParameterSymbol::GetCity(Unit &c) const
{
	SlicObject *context = slicengine_Get()->GetContext();
	SlicArgList *argList = context->GetArgList();
	if(!argList) {
		if(profiledb_Get() && profiledb_Get()->IsDebugSlic()) {
			c3errors_ErrorDialog("SLIC Parameter %s used outside function call", GetName());
		}
		return FALSE;
	}
	return argList->GetCity(m_parameterIndex, c);
}


