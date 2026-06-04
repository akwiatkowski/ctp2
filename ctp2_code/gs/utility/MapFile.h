//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ header
// Description  : Scenario .MAP file handling
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
/// \file   gs/utility/MapFile.h
/// \brief  Scenario .MAP file handling (declarations)
///
/// Phase 0.C-5 / MapFile JSON port: the legacy CivArchive-based binary
/// format is replaced by nlohmann::json.  Scenario .MAP files written
/// before this change are not loadable.  Single-purpose: the scenario
/// editor + slicfunc map-load/save triggers.

#ifdef HAVE_PRAGMA_ONCE
#pragma once
#endif

#ifndef __MAPFILE_H__
#define __MAPFILE_H__

#include <vector>

#include <nlohmann/json.hpp>

#include "ctp/c3types.h"        // MBCHAR, sintN, uintN

class MapFile
{
public:
	MapFile() = default;
	~MapFile() = default;

	bool Load(MBCHAR const * filename);
	bool Save(MBCHAR const * filename);

private:
	// Type-index remap tables.  The save stores types by DB-name (string);
	// on load we resolve names back to current DB indices and stash the
	// per-index mapping here so the later cell/player sections can rewrite
	// the indices.  Populated by Load{Unit,Improvement,Advance}Types,
	// consumed by the corresponding ref-section loaders.
	std::vector<sint32> m_unitTypeMap;
	std::vector<sint32> m_improvementTypeMap;
	std::vector<sint32> m_advanceTypeMap;

	// Save helpers — each builds a sub-object into the top-level doc.
	void SaveTerrain      (nlohmann::json & doc) const;
	void SaveTerrainEnv   (nlohmann::json & doc) const;
	void SaveCities       (nlohmann::json & doc) const;
	void SaveUnits        (nlohmann::json & doc) const;
	void SaveImprovements (nlohmann::json & doc) const;
	void SaveVision       (nlohmann::json & doc) const;
	void SaveAdvances     (nlohmann::json & doc) const;
	void SaveHuts         (nlohmann::json & doc) const;
	void SaveCivilizations(nlohmann::json & doc) const;

	// Load helpers — each consumes a sub-object from the top-level doc.
	// Type-table loaders must run before any ref-loader that uses the map.
	bool LoadTerrain      (nlohmann::json const & doc);
	bool LoadTerrainEnv   (nlohmann::json const & doc);
	bool LoadUnitTypes    (nlohmann::json const & doc);
	bool LoadUnits        (nlohmann::json const & doc);
	bool LoadImprovementTypes(nlohmann::json const & doc);
	bool LoadImprovements (nlohmann::json const & doc);
	bool LoadCities       (nlohmann::json const & doc);
	bool LoadVision       (nlohmann::json const & doc);
	bool LoadAdvanceTypes (nlohmann::json const & doc);
	bool LoadAdvances     (nlohmann::json const & doc);
	bool LoadHuts         (nlohmann::json const & doc);
	bool LoadCivilizations(nlohmann::json const & doc);
};

#endif
