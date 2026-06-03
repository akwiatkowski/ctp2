//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ header
// Description  : Civilisation data
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
// - #pragma once commented out
// - Import structure modified to allow mingw compilation.
// - Prevent crash when settling in the Alexander scenario.
// - Replaced old civilsation databse by new one. (Aug 21st 2005 Martin G�hmann)
// - Replaced CIV_INDEX by sint32. (2-Jan-2008 Martin G�hmann)
//
//----------------------------------------------------------------------------

#if defined(HAVE_PRAGMA_ONCE)
#pragma once
#endif

#ifndef __CIVILISATION_DATA_H__
#define __CIVILISATION_DATA_H__

//----------------------------------------------------------------------------
// Library imports
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// Exported names
//----------------------------------------------------------------------------

class	CivilisationData;

//----------------------------------------------------------------------------
// Project imports
//----------------------------------------------------------------------------

#include "ctp/c3types.h"            // MBCHAR, sint..., uint...
#include "robot/aibackdoor/civarchive.h"         // CivArchive
#include "gs/gameobj/GameObj_types.h"
#include "CivilisationRecord.h" // k_MAX_CityName
#include "gs/database/dbtypes.h"            // k_MAX_NAME_LEN
#include "gs/gameobj/GameObj.h"            // GAMEOBJ
#include "gs/utility/gstypes.h"            // PLAYER_INDEX
#include "gs/gameobj/ID.h"                 // ID

#include <nlohmann/json.hpp>

//----------------------------------------------------------------------------
// Class declarations
//----------------------------------------------------------------------------

#define k_MAX_NAME_LEN           512 // Why isn't dbtypes.h included
#define k_CAPITAL_UNDEFINED       -1 // From CivilisationRec.h
#define k_CITY_NAME_UNDEFINED     -1 // From CivilisationRec.h

class CivilisationData : public GameObj
{
public:


	PLAYER_INDEX    m_owner;

	uint8           m_cityname_count[k_MAX_CityName];

	sint32          m_civ;

	GENDER          m_gender;

	sint32          m_cityStyle;

	MBCHAR          m_leader_name[k_MAX_NAME_LEN],
		            m_personality_description[k_MAX_NAME_LEN],
		            m_civilisation_name[k_MAX_NAME_LEN],
		            m_country_name[k_MAX_NAME_LEN],
		            m_singular_name[k_MAX_NAME_LEN];










	friend class NetCivilization;
	// JSON bridge — covers scalar fields, m_cityname_count[500],
	// and the 5 char buffers.  OMITS m_lesser/m_greater (intrusive
	// linked list, pool-level concern) and m_killMeSoon/m_isFromPool
	// (transient).  Mirrors the AgreementData pattern from Phase D-4.
	friend void to_json(nlohmann::json &j, CivilisationData const &c);
	friend void from_json(nlohmann::json const &j, CivilisationData &c);

public:
	CivilisationData(const ID &id);
	CivilisationData(const ID &id, PLAYER_INDEX owner, sint32 civ, GENDER gender);
	CivilisationData(CivArchive &archive);

	void Serialize(CivArchive &archive) override;

	PLAYER_INDEX GetOwner() const { return m_owner; }
	sint32 GetCivilisation() const { return m_civ; }
	GENDER GetGender() const { return m_gender; }

	sint32 GetAnyCityName() const;
	sint32 GetCapitalName() const;

	void GetCityName(const sint32 name, MBCHAR *s) const;
	void UseCityName(const sint32 name);
	void ReleaseCityName(const sint32 name);
	sint32 GetUseCount(const sint32 name) const;

	MBCHAR const * GetLeaderName() const;
	void SetLeaderName(const MBCHAR *s);

	void SetPersonalityDescription(const MBCHAR* s);
	MBCHAR* GetPersonalityDescription();

	void GetPluralCivName(MBCHAR *s);
	void SetPluralCivName(const MBCHAR *s);
	void GetCountryName(MBCHAR *s);
	void SetCountryName(const MBCHAR *s);
	void GetSingularCivName(MBCHAR *s);
	void SetSingularCivName(const MBCHAR *s);

	sint32 GetCityStyle() const;
	void SetCityStyle( sint32 cityStyle ) { m_cityStyle = cityStyle; }

	void ResetCiv(sint32 newCivIndex, GENDER gender);

	void ResetStrings();
};

#endif
