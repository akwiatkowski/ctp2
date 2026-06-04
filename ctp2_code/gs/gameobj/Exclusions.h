#ifdef HAVE_PRAGMA_ONCE
#pragma once
#endif
#ifndef __EXCLUSIONS_H__
#define __EXCLUSIONS_H__

#include <nlohmann/json.hpp>

class Exclusions
{
	friend void to_json(nlohmann::json &j, Exclusions const &e);
	friend void from_json(nlohmann::json const &j, Exclusions &e);
private:
	sint32 m_numUnits;
	sint32 m_numBuildings;
	sint32 m_numWonders;

	sint32 *m_units;
	sint32 *m_buildings;
	sint32 *m_wonders;
	friend class NetExclusions;

public:
	Exclusions();
	~Exclusions();

	sint32 IsUnitExcluded(sint32 type) { return m_units[type]; }
	sint32 IsBuildingExcluded(sint32 type) { return m_buildings[type]; }
	sint32 IsWonderExcluded(sint32 type) { return m_wonders[type]; }

	void ExcludeUnit(sint32 type, sint32 exclude) { m_units[type] = exclude; }
	void ExcludeBuilding(sint32 type, sint32 exclude) { m_buildings[type] = exclude; }
	void ExcludeWonder(sint32 type, sint32 exclude) { m_wonders[type] = exclude; }
};

// Lifecycle spans gameinit / civapp / network setup.  Storage is
// file-scope `static` in Exclusions.cpp; readers use exclusions_Get(),
// writers (the net layer swaps it for game-rules setup) use _Set.
Exclusions * exclusions_Get();
void         exclusions_Set(Exclusions *p);
#endif
