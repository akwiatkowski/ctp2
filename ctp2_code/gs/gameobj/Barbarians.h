//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ header
// Description  : Barbarian placement and generation
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
// - Standardizes in Babarian period computation. (25-Jan-2008 Martin G�hmann)
// - Standardized visibility check. (22-Feb-2008 Martin G�hmann)
//
//----------------------------------------------------------------------------

#ifdef HAVE_PRAGMA_ONCE
#pragma once
#endif

#ifndef __BARBARIANS_H__
#define __BARBARIANS_H__


class MapPoint;

typedef sint32 PLAYER_INDEX;

class Barbarians {
public:
	static sint32 ChooseUnitType();
	static bool AddBarbarians(const MapPoint &point, PLAYER_INDEX meat,
	                          bool fromGoodyHut, sint32 currentRound);
	static sint32 ChooseSeaUnitType();
	static bool AddPirates(const MapPoint &point, PLAYER_INDEX meat,
	                       bool fromGoodyHut, sint32 currentRound);
	static void BeginYear(sint32 currentRound);
	static bool InBarbarianPeriod(sint32 currentRound);
	static sint32 IsVisibleToAnyone(MapPoint point);
};

#endif
