//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ header
// Description  : Wonder Tracker
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
// HAVE_PRAGMA_ONCE
//
//----------------------------------------------------------------------------
//
// Modifications from the original Activision code:
//
// - None
//
//----------------------------------------------------------------------------

#ifdef HAVE_PRAGMA_ONCE
#pragma once
#endif
#ifndef __WONDER_TRACKER_H__
#define __WONDER_TRACKER_H__

class CivArchive;
class Unit;

#include "gs/utility/gstypes.h"
#include <nlohmann/json.hpp>

class WonderTracker {
	friend void to_json(nlohmann::json &j, WonderTracker const &wt);
	friend void from_json(nlohmann::json const &j, WonderTracker &wt);

private:
	uint64 m_builtWonders;

	uint64 m_buildingWonders[k_MAX_PLAYERS];
	uint32 m_globeSatFlags;

public:
	WonderTracker();
	WonderTracker(CivArchive &archive) { Serialize(archive); }
	void Serialize(CivArchive &archive);

	uint64 GetBuiltWonders() const { return m_builtWonders; }

	bool HasWonderBeenBuilt(sint32 which);
	PLAYER_INDEX WhoOwnsWonder(sint32 which);
	void AddBuilt(sint32 which);
	void SetBuiltWonders(uint64 built);
	bool GetCityWithWonder(sint32 which, Unit &city);

	void SetBuildingWonder(sint32 which, PLAYER_INDEX who);
	void ClearBuildingWonder(sint32 which, PLAYER_INDEX who);
	bool IsBuildingWonder(sint32 which, PLAYER_INDEX who);

	void RecomputeIsBuilding(const PLAYER_INDEX who);

	uint32 GlobeSatFlags() { return m_globeSatFlags; }
	void SetGlobeSatFlags(uint32 flags) { m_globeSatFlags = flags; }
};

// Lifecycle (new during gameinit + allocated::clear cleanup) lives in
// gs/utility/gameinit.cpp; the variable is file-scope `static` there.
// External readers go through wonder_tracker_Get().
WonderTracker * wonder_tracker_Get();
void            wonder_tracker_Set(WonderTracker *p);

#endif
