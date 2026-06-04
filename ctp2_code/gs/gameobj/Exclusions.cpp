//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ source
// Description  : Lists of things that may not be built.
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
//----------------------------------------------------------------------------
//
// Modifications from the original Activision code:
//
// - Cut and paste errors corrected.
//
//----------------------------------------------------------------------------

#include "ctp/c3.h"
#include "gs/gameobj/Exclusions.h"

#include <algorithm>

#include "BuildingRecord.h"
#include "WonderRecord.h"
#include "UnitRecord.h"

static Exclusions *g_exclusions = nullptr;

Exclusions * exclusions_Get()          { return g_exclusions; }
void         exclusions_Set(Exclusions *p) { g_exclusions = p; }

Exclusions::Exclusions()
{
	m_numUnits = g_theUnitDB->NumRecords();
	m_numBuildings = g_theBuildingDB->NumRecords();
	m_numWonders = g_theWonderDB->NumRecords();

	m_units = new sint32[g_theUnitDB->NumRecords()];
	memset(m_units, 0, sizeof(sint32) * m_numUnits);
	m_buildings = new sint32[g_theBuildingDB->NumRecords()];
	memset(m_buildings, 0, sizeof(sint32) * m_numBuildings);
	m_wonders = new sint32[g_theWonderDB->NumRecords()];
	memset(m_wonders, 0, sizeof(sint32) * m_numWonders);
}

Exclusions::~Exclusions()
{
	delete [] m_units;
	delete [] m_buildings;
	delete [] m_wonders;
}

