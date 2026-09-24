//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ header file
// Description  : Declarations for the SettleMap class
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
// - Removed MS version specific code.
// - Standardised <list> import.
// - Moved settle_water argument inside SettleMap::GetSettleTargets. (May 20th 2006 Martin Gühmann)
//
//----------------------------------------------------------------------------

#if defined(HAVE_PRAGMA_ONCE)
#pragma once
#endif

#ifndef __SETTLE_MAP_H__
#define __SETTLE_MAP_H__

#include <list>

class SettleMap;

size_t const    k_minimum_settle_city_size  = 2;
size_t const    k_targets_per_continent     = 25;

#include "robot/aibackdoor/bit_table.h"
#include "ai/mapanalysis/mapgrid.h"
#include "gs/world/MapPoint.h"
#include "gs/gameobj/player.h"
#include "gs/gameobj/Unit.h"

namespace Ctp2 { class Game; }

class SettleMap
{
    friend void to_json(nlohmann::json &j, SettleMap const &s);
    friend void from_json(nlohmann::json const &j, SettleMap &s);
    // Per-game owner: Ctp2::Game holds the instance (was file-static
    // s_settleMap). Ctor public so Game can own it; do not instantiate
    // elsewhere.
public:
	struct SettleTarget
	{
		SettleTarget()
		:   m_value (0.0),
		    m_pos   ()
		{ ; };

		bool operator<(const SettleTarget & rval) const { return ( m_value < rval.m_value ); }
		bool operator>(const SettleTarget & rval) const { return ( m_value > rval.m_value ); }

		double      m_value;
		MapPoint    m_pos;
	};

	typedef std::list<SettleTarget > SettleTargetList;

	// (removed) s_settleMap now lives in Ctp2::Game. Use Ref().
	static SettleMap & Ref();

	void Cleanup();
	void Initialize();

	void HandleCityGrowth(const Unit & city);

	void GetSettleTargets(const PLAYER_INDEX &player,
	                      SettleMap::SettleTargetList & targets) const;

	bool HasSettleTargets(const PLAYER_INDEX &player, bool isWater) const;

	bool CanSettlePos(const MapPoint & rc_pos) const;

	void SetCanSettlePos(const MapPoint & rc_pos, const bool can_settle);

	double GetValue(const MapPoint & rc_pos) const;

	// Game-owned (was file-static s_settleMap). Public so Ctp2::Game can
	// hold it; do not instantiate elsewhere.
	SettleMap();

	double ComputeSettleValue(const MapPoint & pos) const;

	MapGrid<double> m_settleValues;
	Bit_Table m_invalidCells;
};

#endif
