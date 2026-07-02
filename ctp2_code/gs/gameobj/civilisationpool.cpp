//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ source
// Description  : Civilisation pool
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
// - Prevent assigning the same civilisation index twice.
// - Recycle civilisation indices to prevent a game crash.
// - Replaced old civilisation database by new one. (Aug 20th 2005 Martin G�hmann)
// - Replaced CIV_INDEX by sint32. (2-Jan-2008 Martin G�hmann)
//
//----------------------------------------------------------------------------

#include "ctp/c3.h"
#include "ctp/ctp2_utils/c3errors.h"
#include "gs/gameobj/Unit.h"
#include "gs/gameobj/Player.h"
#include "gs/database/StrDB.h"
#include "CivilisationRecord.h"
#include "gs/gameobj/CivilisationPool.h"
#include "gs/utility/RandGen.h"
#include "gs/utility/SimpleDynArr.h"
#include "gs/database/profileDB.h"
#include "net/general/network.h"
#include "gs/utility/Globals.h"

;










CivilisationPool::CivilisationPool() : ObjPool(k_BIT_GAME_OBJ_TYPE_CIVILISATION)
{
	m_usedCivs = new SimpleDynamicArray<sint32>;
}









CivilisationPool::~CivilisationPool()
{
	delete m_usedCivs;
}












Civilisation CivilisationPool::Create(const PLAYER_INDEX owner, sint32 requiredCiv, GENDER gender)
{
	sint32 const	numCivs	= g_theCivilisationDB->NumRecords();
	sint32		civ		= requiredCiv;

	if (numCivs <= 0)
	{
		c3errors_FatalDialogFromDB("CIVILIZATION_ERROR", "CIVILIZATION_NO_MORE_CIVS_AVAILABLE");
	}

	if (CIV_INDEX_RANDOM == civ)
	{
		if (profiledb_Get()->IsNonRandomCivs())
		{
			civ = owner;
		}
		else
		{
			civ = rand_ptr()->Next(numCivs);
		}
	}

	for (sint32 c = 0; m_usedCivs->IsPresent(civ) && (c < numCivs); ++c)
	{
		civ = (civ + 1 < numCivs) ? civ + 1 : 1;
	}

	if (civ >= numCivs)
	{
		c3errors_FatalDialogFromDB("CIVILIZATION_ERROR", "CIVILIZATION_NO_MORE_CIVS_AVAILABLE");
		civ = CIV_INDEX_VANDALS;
	}

	Assert((civ >= CIV_INDEX_VANDALS) && (civ < numCivs));

	Civilisation newCivilisation(NewKey(k_BIT_GAME_OBJ_TYPE_CIVILISATION));

	if (gender == GENDER_RANDOM) {
		gender = (GENDER)(rand_ptr()->Next() % 2);
	}

	CivilisationData *	newData = new CivilisationData(newCivilisation, owner, civ, gender);

	m_usedCivs->Insert(civ);

	StringId	strId = (gender == GENDER_MALE)
						? g_theCivilisationDB->Get(civ)->GetLeaderNameMale()
						: g_theCivilisationDB->Get(civ)->GetLeaderNameFemale();

	newData->SetLeaderName(stringdb_Get()->GetNameStr(strId));

	strId = g_theCivilisationDB->Get(civ)->GetPersonalityDescription();
	newData->SetPersonalityDescription(stringdb_Get()->GetNameStr(strId));

	strId = g_theCivilisationDB->Get(civ)->GetPluralCivName();
	newData->SetPluralCivName(stringdb_Get()->GetNameStr(strId));
	strId = g_theCivilisationDB->Get(civ)->GetCountryName();
	newData->SetCountryName(stringdb_Get()->GetNameStr(strId));
	strId = g_theCivilisationDB->Get(civ)->GetSingularCivName();
	newData->SetSingularCivName(stringdb_Get()->GetNameStr(strId));

	newData->SetCityStyle(g_theCivilisationDB->Get(civ)->GetCityStyleIndex());

	Insert(newData);
	DPRINTF(k_DBG_INFO, ("Civilisation %d is in use\n", civ));

	if(network_Get().IsHost()) {
		network_Get().Enqueue(newData);
	}

	return (newCivilisation);
}

//----------------------------------------------------------------------------
//
// Name       : CivilisationPool::Release
//
// Description: Release a civilisation index for reuse.
//
// Parameters : civ:       Civilisation index to release
//
// Globals    : -
//
// Returns    : -
//
// Remark(s)  : -
//
//----------------------------------------------------------------------------
void CivilisationPool::Release(sint32 const & civ)
{
	sint32 const	usedCount = m_usedCivs->Num();

	for (sint32 i = 0; i < usedCount; ++i)
	{
		if (civ == m_usedCivs->Access(i))
		{
			m_usedCivs->DelIndex(i);
			return;
		}
	}
}
