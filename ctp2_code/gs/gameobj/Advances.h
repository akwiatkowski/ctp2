//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ header
// Description  : Advance (tech) handling
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
// _DEBUG
// Generate extra debug output
//
//----------------------------------------------------------------------------
//
// Modifications from the original Activision code:
//
// - Microsoft specifics marked.
// - Safeguard FindLevel against infinite recursion.
// - Speeded up goody hut advance and unit selection.
// - Added FractionComplete methods. (Feb 4th 2007 Martin G�hmann)
//
//----------------------------------------------------------------------------

#if defined(HAVE_PRAGMA_ONCE)
#pragma once
#endif

#ifndef _ADVANCES_H_
#define _ADVANCES_H_

//----------------------------------------------------------------------------
// Library dependencies
//----------------------------------------------------------------------------

// #include <>

//----------------------------------------------------------------------------
// Export overview
//----------------------------------------------------------------------------

class           Advances;
typedef sint32  AdvanceType;

//----------------------------------------------------------------------------
// Project dependencies
//----------------------------------------------------------------------------

#include "AdvanceRecord.h"  // AdvanceRecord
#include "gs/outcom/AICause.h"	    // CAUSE_SCI
#include "os/include/ctp2_inttypes.h"  // uint8, uint16, sint32
#include "gs/gameobj/player.h"         // PLAYER_INDEX

#include <nlohmann/json.hpp>
#include <vector>

//----------------------------------------------------------------------------
// Declarations
//----------------------------------------------------------------------------

uint32 Advances_Advances_GetVersion();

class Advances
{

private:
//----------------------------------------------------------------------------
// Do not change anything in the types or order of the following variable
// declarations. Doing so will break reading in of save files.
// See the Serialize implementation for more details.
//----------------------------------------------------------------------------

	PLAYER_INDEX m_owner;

	sint32 m_size;
	AdvanceType m_researching;

	sint32	m_age;

	sint32 m_theLastAdvanceEnabledThisManyAdvances;

	sint32 m_total_cost;
	sint32 m_discovered;

//----------------------------------------------------------------------------
// End of fixed variable list. Changing below this line is less dangerous.
//----------------------------------------------------------------------------

	std::vector<uint8>  m_hasAdvance;
	std::vector<uint8>  m_canResearch;
	std::vector<uint16> m_turnsSinceOffered;

	friend class NetInfo;
	friend class NetPlayer;
	friend class NetResearch;
	friend void to_json(nlohmann::json &j, Advances const &a);
	friend void from_json(nlohmann::json const &j, Advances &a);

public:
	Advances(size_t a_Count);
	Advances(Advances const & a_Original);
	virtual ~Advances();

	Advances & operator = (Advances const & a_Original);

	void SetOwner(PLAYER_INDEX o);

	sint32 GetNum() const { return m_size; }

	void UpdateCitySprites(BOOL forceUpdate);
	bool HasAdvance(sint32 index) const;
	void SetHasAdvance(AdvanceType advance, const bool init = false);

	void GrantAdvance();
	void GiveAdvance(AdvanceType adv, CAUSE_SCI cause, BOOL fromClient = FALSE);
	void GiveAdvancePlusPrerequisites(AdvanceType adv);
	void TakeAdvance(AdvanceType adv);
	void InitialAdvance(AdvanceType adv);
	double GetPollutionSizeModifier() const;
	double GetPollutionProductionModifier() const;

	AdvanceType GetResearching() const { return m_researching; }
	void SetResearching(AdvanceType adv);

	sint32 GetCostOfWhatHeKnows() const { return m_total_cost; }

	sint32 GetCost() const;
	sint32 GetCost(const AdvanceType adv) const;

	uint8* CanResearch() const;
	BOOL CanResearch(sint32 advance) const;
	void ResetCanResearch(sint32 justGot);
	uint8* CanAskFor(Advances* otherCivAdvances, sint32 &num) const;
	uint8* CanOffer(Advances* otherCivAdvances, sint32 &num) const;

	sint32 GetDiscovered() const { return m_discovered; }

	sint32 GetMinPrerequisites(sint32 adv) const;
	sint32 GetMinPrerequisites(sint32 adv, sint32 limit) const;
	sint32 FindLevel(AdvanceRecord const * const rec, sint32 const fromLevel = 0) const;

#ifdef _DEBUG
	void DebugDumpTree();
#endif

	sint32 GetProjectedScience() const;
	sint32 TurnsToNextAdvance(AdvanceType adv = -1) const;

	void AddAlienLifeAdvance();

	double FractionComplete(AdvanceType adv) const;
	double FractionComplete() const;
};

#endif
