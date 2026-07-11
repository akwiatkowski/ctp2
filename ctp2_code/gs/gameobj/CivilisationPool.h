//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ header
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
// - Recycle civilisation indices to prevent a game crash.
// - Replaced CIV_INDEX by sint32. (2-Jan-2008 Martin G�hmann)
//
//----------------------------------------------------------------------------

#if defined(HAVE_PRAGMA_ONCE)
#pragma once
#endif

#ifndef __CIVILISATIONPOOL_H__
#define __CIVILISATIONPOOL_H__

class CivilisationPool;

#include "gs/gameobj/ObjPool.h"
#include "gs/gameobj/GameObj_types.h"
#include "gs/gameobj/Civilisation.h"

#include <nlohmann/json.hpp>
#include <memory>

template <class T> class SimpleDynamicArray;

class CivilisationPool : public ObjPool
{
public:
	std::unique_ptr<SimpleDynamicArray<sint32>> m_usedCivs;

	CivilisationPool() ;
	~CivilisationPool() override ;

	CivilisationData* AccessData(const Civilisation id) { return ((CivilisationData*)Access(id)) ; }

	CivilisationData* GetData(const Civilisation id) const { return ((CivilisationData*)Get(id)) ; }

	Civilisation Create(const PLAYER_INDEX owner, sint32 civ, GENDER gender) ;
	void Release(sint32 const & civ);

	// JSON bridge — mirrors CivilisationPool::Serialize.  Persists
	// ObjPool key counter + every live CivilisationData entry +
	// m_usedCivs SimpleDynamicArray<sint32>.  Implementation in
	// json_save.cpp.
	friend void to_json(nlohmann::json &j, CivilisationPool const &p);
	friend void from_json(nlohmann::json const &j, CivilisationPool &p);
};

// Lifecycle in gs/utility/gameinit.cpp; backing static there.  Tests
// re-allocate via civilisationpool_Set() to bypass full game init.
CivilisationPool * civilisationpool_Get();
void               civilisationpool_Set(CivilisationPool *p);

#endif
