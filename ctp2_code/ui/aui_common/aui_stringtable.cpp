//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ source
// Description  : String table user interface object
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
// - Memory leaks repaired.
//
//----------------------------------------------------------------------------

#include "ctp/c3.h"
#include "ui/aui_common/aui_stringtable.h"

#include <algorithm>
#include "ui/aui_common/aui_ldl.h"
#include "ui/aui_common/aui_ui.h"
#include "gs/database/StrDB.h"              // stringdb_Get()

#include "ui/ldl/ldl_data.hpp"

#define k_AUI_STRINGTABLE_LDL_NUMSTRINGS		"numstrings"
#define k_AUI_STRINGTABLE_LDL_STRING			"string"
#define k_AUI_STRINGTABLE_LDL_NODATABASE		"nodatabase"

aui_StringTable::aui_StringTable
(
	AUI_ERRCODE *   retval,
	size_t          numStrings
)
:
	m_Strings       (numStrings)
{
    *retval = AUI_ERRCODE_OK;
}

aui_StringTable::aui_StringTable
(
	AUI_ERRCODE *   retval,
	MBCHAR const *  ldlBlock
)
:
	m_Strings       ()
{
    ldl_datablock * block = aui_Ldl::FindDataBlock(ldlBlock);
	Assert(block);
	if (!block)
    {
        *retval = AUI_ERRCODE_LDLFINDDATABLOCKFAILED;
        return;
    }

	m_Strings.resize(FindNumStringsFromLdl(block));

    MBCHAR temp[k_AUI_LDL_MAXBLOCK + 1];

	if ( block->GetBool(k_AUI_STRINGTABLE_LDL_NODATABASE) )
    {
		for (size_t i = 0; i < m_Strings.size(); ++i)
		{
			snprintf(temp, sizeof(temp), "%s%d", k_AUI_STRINGTABLE_LDL_STRING, i);
			SetString(block->GetString(temp), i);
		}
	}
	else
    {
		for (size_t i = 0; i < m_Strings.size(); ++i)
		{
			snprintf(temp, sizeof(temp), "%s%d", k_AUI_STRINGTABLE_LDL_STRING, i);
			SetString(stringdb_Get()->GetNameStr(block->GetString(temp)), i);
		}
	}

	*retval = AUI_ERRCODE_OK;
}

aui_StringTable::~aui_StringTable()
{
    m_Strings.clear();
}

size_t aui_StringTable::FindNumStringsFromLdl(ldl_datablock * block)
{
    sint32  stringCount = 0;

	if (ATTRIBUTE_TYPE_INT ==
            block->GetAttributeType(k_AUI_STRINGTABLE_LDL_NUMSTRINGS)
       )
    {
        // Use the numstrings entry
		stringCount = block->GetInt(k_AUI_STRINGTABLE_LDL_NUMSTRINGS);
        Assert(stringCount >= 0);
    }
    else
    {
        // Look for string0, string1, etc. entries and count
	    MBCHAR  temp[k_AUI_LDL_MAXBLOCK + 1];
	    bool    isAtEnd = false;

        while (!isAtEnd)
        {
		    snprintf(temp, sizeof(temp), "%s%d", k_AUI_STRINGTABLE_LDL_STRING, stringCount);
            if (block->GetString(temp))
            {
                ++stringCount;
            }
            else
            {
                isAtEnd = true;
            }
	    }
    }

	return static_cast<size_t>(stringCount);
}






MBCHAR * aui_StringTable::GetString( sint32 index )
{
	Assert(index >= 0 && static_cast<size_t>(index) < m_Strings.size());
	if (index < 0 || static_cast<size_t>(index) >= m_Strings.size()) return nullptr;

	return m_Strings[index].data();
}


AUI_ERRCODE aui_StringTable::SetString(const MBCHAR *text, sint32 index)
{
	Assert(index >= 0 && static_cast<size_t>(index) < m_Strings.size());
	if (index < 0 || static_cast<size_t>(index) >= m_Strings.size())
        return AUI_ERRCODE_INVALIDPARAM;

	m_Strings[index] = text ? text : "";

	return AUI_ERRCODE_OK;
}
