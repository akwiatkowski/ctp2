//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ source
// Description  : Multiplayer units list
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
// - Initialized local variables. (Sep 9th 2005 Martin G�hmann)
//
//----------------------------------------------------------------------------
#include "ctp/c3.h"
#include <algorithm>

#include "ui/netshell/ns_units.h"

#include "ui/aui_common/aui_stringtable.h"
#include "gs/database/StrDB.h"              // stringdb_Get()
#include "UnitRecord.h"         // g_theUnitDB

static ns_Units *g_nsUnits = nullptr;

ns_Units * nsunits_Get()         { return g_nsUnits; }
void       nsunits_Set(ns_Units *p)  { g_nsUnits = p; }

ns_Units::ns_Units()
{
	Assert(g_theUnitDB->NumRecords() <= k_UNITS_MAX);
    sint32      numUnits    =
        std::min<sint32>(k_UNITS_MAX, g_theUnitDB->NumRecords());
	m_noIndex.resize(numUnits);

	AUI_ERRCODE errcode     = AUI_ERRCODE_OK;
	m_stringtable = std::make_unique<aui_StringTable>(&errcode, numUnits);
	Assert(AUI_NEWOK(m_stringtable, errcode));
	if (!AUI_NEWOK(m_stringtable,errcode)) return;

	for (sint32 i = 0; i < numUnits; i++)
	{
		StringId stringNum = g_theUnitDB->GetName(i);
		m_stringtable->SetString(stringdb_Get()->GetNameStr(stringNum), i);

		m_noIndex[i] = g_theUnitDB->Get(i)->GetNoIndex();
	}
}

ns_Units::~ns_Units()
{
	m_stringtable.reset();
	// m_noIndex is std::vector, auto-freed
}
