//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ source
// Description  : Slic array variable handling
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
// - Repaired memory leak
//
//----------------------------------------------------------------------------

#include "ctp/c3.h"
#include <memory>
#include "gs/slic/slicif.h"
#include "gs/slic/SlicArray.h"
#include "gs/slic/SlicEngine.h"
#include "gs/slic/SlicSymbol.h"
#include "gs/slic/SlicStack.h"
#include "gs/slic/SlicNamedSymbol.h"
#include "gs/slic/SlicStruct.h"
#include "gs/slic/SlicObject.h"
#include "gs/slic/SlicSegment.h"
#include "gs/slic/SlicFrame.h"
#include "gs/database/profileDB.h"

#define k_DEFAULT_SLICARRAY_SIZE 1

SlicArray::SlicArray(SS_TYPE type, SLIC_SYM varType)
{
	m_type = type;
	m_varType = varType;
	m_allocatedSize = k_DEFAULT_SLICARRAY_SIZE;
	m_arraySize = 0;
	m_sizeIsFixed = false;
	m_array = std::make_unique<SlicStackValue[]>(m_allocatedSize);
	memset(m_array.get(), 0, m_allocatedSize * sizeof(SlicStackValue));
	m_structTemplate = nullptr;
}

SlicArray::SlicArray(SlicStructDescription *aStruct)
{
	m_type = SS_TYPE_SYM;
	m_varType = SLIC_SYM_STRUCT;
	m_allocatedSize = k_DEFAULT_SLICARRAY_SIZE;
	m_arraySize = 0;
	m_sizeIsFixed = false;
	m_array = std::make_unique<SlicStackValue[]>(m_allocatedSize);
	memset(m_array.get(), 0, m_allocatedSize * sizeof(SlicStackValue));
	m_structTemplate = aStruct;
}

SlicArray::~SlicArray()
{
	if (SS_TYPE_SYM == m_type)
    {
        for (size_t i = 0; i < m_allocatedSize; ++i)
        {
		    std::unique_ptr<SlicSymbolData>{m_array[i].m_sym};
        }
	}
}

void SlicArray::FixSize(sint32 size)
{
	if (SS_TYPE_SYM == m_type)
    {
        for (size_t i = 0; i < m_allocatedSize; ++i)
        {
		    std::unique_ptr<SlicSymbolData>{m_array[i].m_sym};
        }
	}

	m_allocatedSize = static_cast<uint32>(size);
    m_arraySize     = size;
	m_array         = std::make_unique<SlicStackValue[]>(m_allocatedSize);
	memset(m_array.get(), 0, m_allocatedSize * sizeof(SlicStackValue));
	m_sizeIsFixed   = true;
}

void SlicArray::SetType(SS_TYPE type, SLIC_SYM varType)
{
	Assert(m_arraySize == 0 || m_sizeIsFixed);
	m_type = type;
	m_varType = varType;
}

BOOL SlicArray::Lookup(sint32 index, SS_TYPE &type, SlicStackValue &value)
{

	type = m_type;

	if(index < 0 || index >= m_arraySize) {
		if(slicengine_Get()->GetContext() && slicengine_Get()->GetContext()->GetSegment() &&
			slicengine_Get()->GetContext()->GetFrame()) {
			if(profiledb_Get() && profiledb_Get()->IsDebugSlic()) {
				c3errors_ErrorDialog("SLIC", "%s:%d: Array index %d out of bounds",
									 slicengine_Get()->GetContext()->GetSegment()->GetFilename(),
									 slicengine_Get()->GetContext()->GetFrame()->GetCurrentLine(),
									 index);
			}
		} else {
			if(profiledb_Get() && profiledb_Get()->IsDebugSlic()) {
				c3errors_ErrorDialog("SLIC", "Array index %d out of bounds",
									 index);
			}
		}
		return FALSE;
	}

	if(m_type == SS_TYPE_SYM && m_array[index].m_sym == nullptr) {

		if(m_structTemplate) {
			m_array[index].m_sym = m_structTemplate->CreateInstance();
		} else {
			m_array[index].m_sym = std::make_unique<SlicSymbolData>(m_varType).release();
		}
	}

	value = m_array[index];
	return TRUE;
}

BOOL SlicArray::Insert(sint32 untestedIndex, SS_TYPE type, SlicStackValue value)
{
	switch(m_type) {
		case SS_TYPE_VAR:
		{

			Assert(type == SS_TYPE_VAR || type == SS_TYPE_SYM);
			if(type != SS_TYPE_VAR && type != SS_TYPE_SYM) {
				return FALSE;
			}


			SlicSymbolData *sym;
			if(type == SS_TYPE_VAR)
				sym = slicengine_Get()->GetSymbol(value.m_int);
			else
				sym = value.m_sym;

			if(!sym || (sym->GetType() != m_varType)) {
				return FALSE;
			}
			break;
		}
		case SS_TYPE_INT:

			if(type != SS_TYPE_INT) {
				SlicSymbolData *sym = value.m_sym;
				if(!sym) {
					return FALSE;
				}
				type = SS_TYPE_INT;
				sym->GetIntValue(value.m_int);
			}
			break;
		case SS_TYPE_SYM:

			break;
		default:

			Assert(FALSE);
			return FALSE;
	}

	if (untestedIndex < 0)
		return FALSE;

	size_t const    index = static_cast<size_t>(untestedIndex);

	if (index >= m_allocatedSize)
    {
		uint32 const oldAllocated = m_allocatedSize;
		while (index >= m_allocatedSize)
        {
			m_allocatedSize *= 2;
		}

		auto newArray = std::make_unique<SlicStackValue[]>(m_allocatedSize);
		memset(&newArray[oldAllocated], 0,
			   (m_allocatedSize - oldAllocated) * sizeof(SlicStackValue));

		memcpy(newArray.get(), m_array.get(), oldAllocated * sizeof(SlicStackValue));
		m_array = std::move(newArray);
	}

	if (index >= static_cast<size_t>(m_arraySize))
	{
		if (m_sizeIsFixed)
		{

			return FALSE;
		}

		if (index > static_cast<size_t>(m_arraySize))
		{
			memset(&m_array[m_arraySize], 0, (index - m_arraySize) * sizeof(SlicStackValue));
		}
		m_arraySize = index + 1;
	}

	if (m_type == SS_TYPE_SYM)
    {
        // Create a new symbol if one does not exist yet.
	    if (!m_array[index].m_sym)
        {
			m_array[index].m_sym = (m_structTemplate)
                                   ? m_structTemplate->CreateInstance()
                                   : std::make_unique<SlicSymbolData>(m_varType).release();
		}

        return m_array[index].m_sym &&
               m_array[index].m_sym->SetValueFromStackValue(type, value);
	}
    else
    {
		m_array[index] = value;
	}

	return TRUE;
}

void SlicArray::Prune(sint32 size)
{
	if (m_sizeIsFixed)
		return;

	if (m_type == SS_TYPE_SYM)
    {
		for (sint32 i = size; i < m_arraySize; i++)
        {
			std::unique_ptr<SlicSymbolData>{m_array[i].m_sym};
			m_array[i].m_sym = nullptr;
		}
	}

	m_arraySize = size;
}
