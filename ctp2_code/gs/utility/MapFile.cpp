//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ source
// Description  : Scenario .MAP file handling — JSON format
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
///
/// \file   gs/utility/MapFile.cpp
/// \brief  Scenario .MAP file handling (definitions)
///
/// Phase 0.C-5 / MapFile JSON port.  The original binary chunk format
/// (TERR/TENV/NCTY/UTYP/UNIT/ITYP/IMPS/VISN/ATYP/PADV/HUTS/CIVS) is
/// replaced with a single nlohmann::json document.  Field semantics are
/// preserved 1:1 with the old chunks; the encoding changes:
///   - terrain / terrain_env / vision / huts: dense JSON arrays of ints
///   - unit_types / improvement_types / advance_types: arrays of
///     DB name strings; on load we resolve to current DB indices
///   - cities / units / improvements / advances / civilizations:
///     arrays of structured per-cell or per-player objects
///   - improvements/wonders bitmasks (uint64) are written as hex strings
///     to keep portability with JSON's 53-bit safe integer range

#include "ctp/c3.h"
#include "gs/utility/MapFile.h"

#include <fstream>
#include <sstream>
#include <vector>

#include <nlohmann/json.hpp>

#include "AdvanceRecord.h"              // g_theAdvanceDB
#include "TerrainImprovementRecord.h"   // g_theTerrainImprovementDB
#include "UnitRecord.h"                 // g_theUnitDB
#include "gs/outcom/AICause.h"
#include "gs/world/Cell.h"
#include "gs/world/cellunitlist.h"
#include "gs/gameobj/Civilisation.h"
#include "gs/gameobj/Player.h"          // player_Get()
#include "gs/gameobj/TerrImprove.h"
#include "gs/gameobj/UnitData.h"
#include "gs/gameobj/unitutil.h"
#include "gs/gameobj/Vision.h"
#include "gs/world/World.h"
#include "gs/database/profileDB.h"
#include "gs/database/dbtypes.h"
#include "gs/core/render_observer.h"
#include "gs/newdb/CTPDatabase.h"
#include "gs/database/StrDB.h"          // g_theStringDB

extern sint32 g_isCheatModeOn;
extern void   gameinit_ResetMapSize();

namespace {

constexpr char const * kMagic         = "CTP2-MAP";
constexpr int          kSchemaVersion = 1;

// Serialise a uint64 as a hex string ("0x...") — JSON numbers don't
// reliably round-trip 64-bit integers through stdlib parsers.
std::string u64_to_hex(uint64 v)
{
	std::ostringstream os;
	os << "0x" << std::hex << v;
	return os.str();
}

uint64 hex_to_u64(std::string const & s)
{
	return std::stoull(s, nullptr, 0);
}

// DB id-string lookup mirrors the old SaveDBNames helper: prefer the
// record's GetName() StringDB id; fall back to GetNameText() raw.
template <class T>
std::vector<std::string> collect_db_names(CTPDatabase<T> * db)
{
	std::vector<std::string> names;
	names.reserve(db->NumRecords());
	for (sint32 i = 0; i < db->NumRecords(); ++i)
	{
		char const * id;
		if (db->Get(i)->GetName() < 0)
			id = db->Get(i)->GetNameText();
		else
			id = g_theStringDB->GetIdStr(db->Get(i)->GetName());
		Assert(id);
		names.emplace_back(id ? id : "");
	}
	return names;
}

} // namespace

//----------------------------------------------------------------------------
// Top-level Save / Load
//----------------------------------------------------------------------------

bool MapFile::Save(MBCHAR const * filename)
{
	std::ofstream out(filename);
	if (!out) return false;

	nlohmann::json doc;
	doc["magic"]          = kMagic;
	doc["schema_version"] = kSchemaVersion;

	SaveTerrain      (doc);
	SaveVision       (doc);
	SaveTerrainEnv   (doc);
	SaveUnits        (doc);
	SaveCities       (doc);
	SaveImprovements (doc);
	SaveAdvances     (doc);
	SaveHuts         (doc);
	SaveCivilizations(doc);

	out << doc.dump(2);
	return out.good();
}

bool MapFile::Load(MBCHAR const * filename)
{
	std::ifstream in(filename);
	if (!in) return false;

	nlohmann::json doc;
	try { in >> doc; }
	catch (nlohmann::json::exception const & e)
	{
		DPRINTF(k_DBG_GAMESTATE,
		        ("Error loading map: JSON parse failed: %s\n", e.what()));
		return false;
	}

	if (!doc.contains("magic") || doc["magic"] != kMagic)
	{
		DPRINTF(k_DBG_GAMESTATE, ("Error loading map: bad/missing magic\n"));
		return false;
	}

	try
	{
		// Order matches the old SaveMap dispatch + type-table-before-refs
		// invariant: each *_types section populates a map consumed by its
		// ref section.
		if (doc.contains("terrain"))           if (!LoadTerrain        (doc)) return false;
		if (doc.contains("terrain_env"))       if (!LoadTerrainEnv     (doc)) return false;
		if (doc.contains("unit_types"))        if (!LoadUnitTypes      (doc)) return false;
		if (doc.contains("units"))             if (!LoadUnits          (doc)) return false;
		if (doc.contains("improvement_types")) if (!LoadImprovementTypes(doc)) return false;
		if (doc.contains("improvements"))      if (!LoadImprovements   (doc)) return false;
		if (doc.contains("cities"))            if (!LoadCities         (doc)) return false;
		if (doc.contains("vision"))            if (!LoadVision         (doc)) return false;
		if (doc.contains("advance_types"))     if (!LoadAdvanceTypes   (doc)) return false;
		if (doc.contains("advances"))          if (!LoadAdvances       (doc)) return false;
		if (doc.contains("huts"))              if (!LoadHuts           (doc)) return false;
		if (doc.contains("civilizations"))     if (!LoadCivilizations  (doc)) return false;
	}
	catch (nlohmann::json::exception const & e)
	{
		DPRINTF(k_DBG_GAMESTATE,
		        ("Error loading map: deserialisation failed: %s\n", e.what()));
		return false;
	}

	return true;
}

//----------------------------------------------------------------------------
// Save side
//----------------------------------------------------------------------------

void MapFile::SaveTerrain(nlohmann::json & doc) const
{
	sint32 const w = world_Get()->GetXWidth();
	sint32 const h = world_Get()->GetYHeight();

	nlohmann::json cells = nlohmann::json::array();
	cells.get_ptr<nlohmann::json::array_t *>()->reserve(static_cast<size_t>(w) * h);
	for (sint32 y = 0; y < h; ++y)
		for (sint32 x = 0; x < w; ++x)
			cells.push_back(static_cast<int>(world_Get()->GetCell(x, y)->GetTerrain()));

	doc["terrain"] = { {"width", w}, {"height", h}, {"cells", std::move(cells)} };
}

void MapFile::SaveTerrainEnv(nlohmann::json & doc) const
{
	sint32 const w = world_Get()->GetXWidth();
	sint32 const h = world_Get()->GetYHeight();

	nlohmann::json cells = nlohmann::json::array();
	cells.get_ptr<nlohmann::json::array_t *>()->reserve(static_cast<size_t>(w) * h);
	for (sint32 y = 0; y < h; ++y)
		for (sint32 x = 0; x < w; ++x)
			cells.push_back(world_Get()->GetCell(x, y)->GetEnv());

	doc["terrain_env"] = { {"width", w}, {"height", h}, {"cells", std::move(cells)} };
}

void MapFile::SaveCities(nlohmann::json & doc) const
{
	nlohmann::json arr = nlohmann::json::array();
	sint32 const w = world_Get()->GetXWidth();
	sint32 const h = world_Get()->GetYHeight();
	for (sint32 y = 0; y < h; ++y)
	{
		for (sint32 x = 0; x < w; ++x)
		{
			Cell * cell = world_Get()->GetCell(x, y);
			if (!cell->HasCity()) continue;

			Unit city = cell->GetCity();
			arr.push_back({
				{"x",            x},
				{"y",            y},
				{"size",         city.PopCount()},
				{"improvements", u64_to_hex(city.GetImprovements())},
				{"wonders",      u64_to_hex(city.GetData()->GetCityData()->GetBuiltWonders())},
				{"owner",        static_cast<int>(city.GetOwner())},
				{"name",         city.GetName() ? city.GetName() : ""},
			});
		}
	}
	doc["cities"] = std::move(arr);
}

void MapFile::SaveUnits(nlohmann::json & doc) const
{
	doc["unit_types"] = collect_db_names(g_theUnitDB);

	nlohmann::json cells = nlohmann::json::array();
	sint32 const w = world_Get()->GetXWidth();
	sint32 const h = world_Get()->GetYHeight();
	for (sint32 y = 0; y < h; ++y)
	{
		for (sint32 x = 0; x < w; ++x)
		{
			CellUnitList * units = world_Get()->GetCell(x, y)->UnitArmy();
			if (!units) continue;

			nlohmann::json stack = nlohmann::json::array();
			for (sint32 i = 0; i < units->Num(); ++i)
			{
				stack.push_back({
					{"owner", static_cast<int>(units->Access(i).GetOwner())},
					{"type",  static_cast<int>(units->Access(i).GetType())},
				});
			}
			cells.push_back({{"x", x}, {"y", y}, {"stack", std::move(stack)}});
		}
	}
	doc["units"] = std::move(cells);
}

void MapFile::SaveImprovements(nlohmann::json & doc) const
{
	doc["improvement_types"] = collect_db_names(g_theTerrainImprovementDB);

	nlohmann::json cells = nlohmann::json::array();
	sint32 const w = world_Get()->GetXWidth();
	sint32 const h = world_Get()->GetYHeight();
	for (sint32 y = 0; y < h; ++y)
	{
		for (sint32 x = 0; x < w; ++x)
		{
			Cell * cell = world_Get()->GetCell(x, y);
			if (cell->GetNumDBImprovements() <= 0) continue;

			nlohmann::json types = nlohmann::json::array();
			for (sint32 i = 0; i < cell->GetNumDBImprovements(); ++i)
				types.push_back(cell->GetDBImprovement(i));

			cells.push_back({{"x", x}, {"y", y}, {"types", std::move(types)}});
		}
	}
	doc["improvements"] = std::move(cells);
}

void MapFile::SaveVision(nlohmann::json & doc) const
{
	sint32 const w = world_Get()->GetXWidth();
	sint32 const h = world_Get()->GetYHeight();

	nlohmann::json arr = nlohmann::json::array();
	for (sint32 p = 0; p < k_MAX_PLAYERS; ++p)
	{
		if (!player_Get(p)) continue;

		nlohmann::json fog = nlohmann::json::array();
		fog.get_ptr<nlohmann::json::array_t *>()->reserve(static_cast<size_t>(w) * h);
		for (sint32 x = 0; x < w; ++x)
			for (sint32 y = 0; y < h; ++y)
				fog.push_back(player_Get(p)->m_vision->m_array[x][y]);

		arr.push_back({
			{"player", p}, {"width", w}, {"height", h}, {"fog", std::move(fog)},
		});
	}
	doc["vision"] = std::move(arr);
}

void MapFile::SaveAdvances(nlohmann::json & doc) const
{
	doc["advance_types"] = collect_db_names(g_theAdvanceDB);

	nlohmann::json arr = nlohmann::json::array();
	sint32 const numAdv = g_theAdvanceDB->NumRecords();
	for (sint32 p = 0; p < k_MAX_PLAYERS; ++p)
	{
		if (!player_Get(p)) continue;

		nlohmann::json has = nlohmann::json::array();
		has.get_ptr<nlohmann::json::array_t *>()->reserve(numAdv);
		for (sint32 a = 0; a < numAdv; ++a)
			has.push_back(static_cast<bool>(player_Get(p)->HasAdvance(a)));

		arr.push_back({{"player", p}, {"has", std::move(has)}});
	}
	doc["advances"] = std::move(arr);
}

void MapFile::SaveHuts(nlohmann::json & doc) const
{
	sint32 const w = world_Get()->GetXWidth();
	sint32 const h = world_Get()->GetYHeight();

	nlohmann::json cells = nlohmann::json::array();
	cells.get_ptr<nlohmann::json::array_t *>()->reserve(static_cast<size_t>(w) * h);
	for (sint32 y = 0; y < h; ++y)
	{
		for (sint32 x = 0; x < w; ++x)
		{
			MapPoint mp(x, y);
			cells.push_back(static_cast<bool>(world_Get()->GetGoodyHut(mp)));
		}
	}
	doc["huts"] = { {"width", w}, {"height", h}, {"cells", std::move(cells)} };
}

void MapFile::SaveCivilizations(nlohmann::json & doc) const
{
	// Always emit k_MAX_PLAYERS entries to keep slot indices stable;
	// empty slots get civ=0 and an empty leader string.
	nlohmann::json arr = nlohmann::json::array();
	for (sint32 i = 0; i < k_MAX_PLAYERS; ++i)
	{
		if (player_Get(i))
		{
			arr.push_back({
				{"civ",    static_cast<uint32>(player_Get(i)->m_civilisation->GetCivilisation())},
				{"leader", player_Get(i)->GetLeaderName() ? player_Get(i)->GetLeaderName() : ""},
			});
		}
		else
		{
			arr.push_back({{"civ", 0}, {"leader", ""}});
		}
	}
	doc["civilizations"] = std::move(arr);
}

//----------------------------------------------------------------------------
// Load side
//----------------------------------------------------------------------------

bool MapFile::LoadTerrain(nlohmann::json const & doc)
{
	// Wipe the current world of units/cities/improvements so the
	// freshly-loaded terrain isn't sitting under stale objects.
	g_isCheatModeOn = TRUE;
	{
		sint32 const w0 = world_Get()->GetXWidth();
		sint32 const h0 = world_Get()->GetYHeight();
		for (sint32 y = 0; y < h0; ++y)
		{
			for (sint32 x = 0; x < w0; ++x)
			{
				Cell * cell = world_Get()->GetCell(x, y);
				while (cell->GetNumUnits() > 0)
					cell->AccessUnit(0).Kill(CAUSE_REMOVE_ARMY_UNKNOWN, -1);
				if (cell->GetCity().m_id != 0)
					cell->GetCity().Kill(CAUSE_REMOVE_ARMY_UNKNOWN, -1);
				while (cell->GetNumImprovements() > 0)
					cell->AccessImprovement(0).Kill();
				while (cell->GetNumDBImprovements() > 0)
					cell->RemoveDBImprovement(cell->GetDBImprovement(0));
			}
		}
	}
	for (sint32 i = 0; i < k_MAX_PLAYERS; ++i)
		if (player_Get(i))
			player_Get(i)->m_vision->SetTheWholeWorldUnexplored();

	render_observer::AddCopyVision();
	render_observer::CatchUp();
	g_isCheatModeOn = FALSE;

	auto const & sec = doc.at("terrain");
	sint32 const w   = sec.at("width").get<sint32>();
	sint32 const h   = sec.at("height").get<sint32>();
	auto const & cells = sec.at("cells");

	bool const yWrapOk = (h % w == 0);
	world_Get()->Reset(w, h,
	                   yWrapOk ? profiledb_Get()->IsYWrap() : FALSE,
	                   profiledb_Get()->IsXWrap());

	if (static_cast<sint32>(cells.size()) != w * h)
	{
		DPRINTF(k_DBG_GAMESTATE, ("Error loading terrain: cell count mismatch\n"));
		return false;
	}

	sint32 idx = 0;
	for (sint32 y = 0; y < h; ++y)
		for (sint32 x = 0; x < w; ++x)
			world_Get()->GetCell(x, y)->SetTerrain(cells[idx++].get<sint32>());

	gameinit_ResetMapSize();
	return true;
}

bool MapFile::LoadTerrainEnv(nlohmann::json const & doc)
{
	auto const & sec = doc.at("terrain_env");
	sint32 const w   = sec.at("width").get<sint32>();
	sint32 const h   = sec.at("height").get<sint32>();
	auto const & cells = sec.at("cells");

	if (static_cast<sint32>(cells.size()) != w * h)
	{
		DPRINTF(k_DBG_GAMESTATE, ("Error loading terrain_env: cell count mismatch\n"));
		return false;
	}

	sint32 idx = 0;
	for (sint32 y = 0; y < h; ++y)
	{
		for (sint32 x = 0; x < w; ++x)
		{
			uint32 env = cells[idx++].get<uint32>();
			env &= ~(k_MASK_ENV_CITY | k_MASK_ENV_CITY_RADIUS);
			world_Get()->GetCell(x, y)->SetEnv(env);
		}
	}

	for (sint32 y = 0; y < h; ++y)
	{
		for (sint32 x = 0; x < w; ++x)
		{
			world_Get()->GetCell(x, y)->CalcMovementType();
			world_Get()->GetCell(x, y)->CalcTerrainMoveCost();
		}
	}
	world_Get()->NumberContinents();
	return true;
}

bool MapFile::LoadUnitTypes(nlohmann::json const & doc)
{
	auto const & names = doc.at("unit_types");
	m_unitTypeMap.assign(names.size(), CTPRecord::INDEX_INVALID);

	for (size_t i = 0; i < names.size(); ++i)
	{
		std::string const & name = names[i].get_ref<std::string const &>();
		sint32 strId;
		if (!g_theStringDB->GetStringID(name.c_str(), strId))
		{
			DPRINTF(k_DBG_GAMESTATE,
			        ("WARNING: Unit %s missing from string DB\n", name.c_str()));
		}
		else if (!g_theUnitDB->GetNamedItem(strId, m_unitTypeMap[i]))
		{
			DPRINTF(k_DBG_GAMESTATE,
			        ("WARNING: Unit %s missing from Unit DB\n", name.c_str()));
		}
	}
	return true;
}

bool MapFile::LoadUnits(nlohmann::json const & doc)
{
	for (auto const & cell : doc.at("units"))
	{
		sint32 const x = cell.at("x").get<sint32>();
		sint32 const y = cell.at("y").get<sint32>();
		for (auto const & u : cell.at("stack"))
		{
			sint32 const owner = u.at("owner").get<sint32>();
			sint32 const type  = u.at("type").get<sint32>();
			if (!player_Get(owner))
			{
				DPRINTF(k_DBG_GAMESTATE,
				        ("WARNING: Player %d does not exist, can't create unit\n", owner));
				continue;
			}
			if (type < 0 || static_cast<size_t>(type) >= m_unitTypeMap.size()) continue;
			if (m_unitTypeMap[type] < 0) continue;
			MapPoint pos(x, y);
			player_Get(owner)->CreateUnit(m_unitTypeMap[type], pos, Unit(),
			                              FALSE, CAUSE_NEW_ARMY_CHEAT);
		}
	}
	return true;
}

bool MapFile::LoadImprovementTypes(nlohmann::json const & doc)
{
	auto const & names = doc.at("improvement_types");
	m_improvementTypeMap.assign(names.size(), CTPRecord::INDEX_INVALID);

	for (size_t i = 0; i < names.size(); ++i)
	{
		std::string const & name = names[i].get_ref<std::string const &>();
		if (!g_theTerrainImprovementDB->GetNamedItem(name.c_str(),
		                                             m_improvementTypeMap[i]))
		{
			DPRINTF(k_DBG_GAMESTATE,
			        ("WARNING: Improvement %s missing from DB\n", name.c_str()));
		}
	}
	return true;
}

bool MapFile::LoadImprovements(nlohmann::json const & doc)
{
	for (auto const & cell : doc.at("improvements"))
	{
		sint32 const x = cell.at("x").get<sint32>();
		sint32 const y = cell.at("y").get<sint32>();
		for (auto const & t : cell.at("types"))
		{
			sint32 const type = t.get<sint32>();
			if (type < 0 || static_cast<size_t>(type) >= m_improvementTypeMap.size())
				continue;
			if (m_improvementTypeMap[type] < 0) continue;
			world_Get()->GetCell(x, y)->InsertDBImprovement(m_improvementTypeMap[type]);
		}
	}
	return true;
}

bool MapFile::LoadCities(nlohmann::json const & doc)
{
	for (auto const & c : doc.at("cities"))
	{
		sint32 const x        = c.at("x").get<sint32>();
		sint32 const y        = c.at("y").get<sint32>();
		sint32 const citySize = c.at("size").get<sint32>();
		uint64 const improvements = hex_to_u64(
		    c.at("improvements").get_ref<std::string const &>());
		uint64 const wonders      = hex_to_u64(
		    c.at("wonders").get_ref<std::string const &>());
		sint32 const owner    = c.at("owner").get<sint32>();
		std::string const & name = c.at("name").get_ref<std::string const &>();

		if (!player_Get(owner)) continue;

		sint32 const cityType = world_Get()->IsLand(x, y)
		                         ? unitutil_GetLandCity()
		                         : unitutil_GetSeaCity();
		MapPoint pos(x, y);
		Unit city = player_Get(owner)->CreateCity(cityType, pos,
		                                          CAUSE_NEW_CITY_CHEAT, nullptr, -1);
		if (!city.IsValid()) continue;
		city.CD()->ChangePopulation(citySize - city.CD()->PopCount());
		city.CD()->SetImprovements(improvements);
		city.CD()->SetWonders(wonders);
		if (!name.empty())
			city.CD()->SetName(name.c_str());
		player_Get(owner)->m_builtWonders |= wonders;
	}
	return true;
}

bool MapFile::LoadVision(nlohmann::json const & doc)
{
	for (auto const & entry : doc.at("vision"))
	{
		sint32 const p = entry.at("player").get<sint32>();
		sint32 const w = entry.at("width").get<sint32>();
		sint32 const h = entry.at("height").get<sint32>();
		Assert(w == world_Get()->GetXWidth());
		Assert(h == world_Get()->GetYHeight());

		if (!player_Get(p)) continue;

		auto const & fog = entry.at("fog");
		if (static_cast<sint32>(fog.size()) != w * h)
		{
			DPRINTF(k_DBG_GAMESTATE,
			        ("Error loading vision: fog cell count mismatch\n"));
			return false;
		}

		sint32 idx = 0;
		for (sint32 x = 0; x < w; ++x)
			for (sint32 y = 0; y < h; ++y)
			{
				player_Get(p)->m_vision->m_array[x][y] =
				    static_cast<uint16>(fog[idx++].get<uint32>() & 0x8000);
			}
	}

	render_observer::AddCopyVision();
	render_observer::CatchUp();
	return true;
}

bool MapFile::LoadAdvanceTypes(nlohmann::json const & doc)
{
	auto const & names = doc.at("advance_types");
	m_advanceTypeMap.assign(names.size(), CTPRecord::INDEX_INVALID);

	for (size_t i = 0; i < names.size(); ++i)
	{
		std::string const & name = names[i].get_ref<std::string const &>();
		sint32 strId;
		if (!g_theStringDB->GetStringID(name.c_str(), strId))
		{
			DPRINTF(k_DBG_GAMESTATE,
			        ("WARNING: Advance %s missing from string DB\n", name.c_str()));
		}
		else if (!g_theAdvanceDB->GetNamedItem(strId, m_advanceTypeMap[i]))
		{
			DPRINTF(k_DBG_GAMESTATE,
			        ("WARNING: Advance %s missing from Advance DB\n", name.c_str()));
		}
	}
	return true;
}

bool MapFile::LoadAdvances(nlohmann::json const & doc)
{
	for (auto const & entry : doc.at("advances"))
	{
		sint32 const p = entry.at("player").get<sint32>();
		if (!player_Get(p)) continue;

		Assert(!player_Get(p)->m_disableChooseResearch);
		player_Get(p)->m_disableChooseResearch = TRUE;

		auto const & has = entry.at("has");
		for (size_t i = 0; i < has.size(); ++i)
		{
			if (i >= m_advanceTypeMap.size()) break;
			if (!has[i].get<bool>()) continue;
			if (m_advanceTypeMap[i] < 0) continue;
			player_Get(p)->m_advances->SetHasAdvance(m_advanceTypeMap[i]);
		}
		player_Get(p)->m_disableChooseResearch = FALSE;
	}
	return true;
}

bool MapFile::LoadHuts(nlohmann::json const & doc)
{
	auto const & sec = doc.at("huts");
	sint32 const w   = sec.at("width").get<sint32>();
	sint32 const h   = sec.at("height").get<sint32>();
	auto const & cells = sec.at("cells");

	if (static_cast<sint32>(cells.size()) != w * h)
	{
		DPRINTF(k_DBG_GAMESTATE, ("Error loading huts: cell count mismatch\n"));
		return false;
	}

	sint32 idx = 0;
	for (sint32 y = 0; y < h; ++y)
		for (sint32 x = 0; x < w; ++x)
			if (cells[idx++].get<bool>())
				world_Get()->GetCell(x, y)->CreateGoodyHut();
	return true;
}

bool MapFile::LoadCivilizations(nlohmann::json const & doc)
{
	auto const & arr = doc.at("civilizations");
	sint32 const n = std::min<sint32>(arr.size(), k_MAX_PLAYERS);

	for (sint32 i = 0; i < n; ++i)
	{
		auto const & entry = arr[i];
		uint32 const currNation =
		    entry.value("civ", static_cast<uint32>(0));
		std::string const leader = entry.value("leader", std::string{});

		if (!player_Get(i)) continue;

		player_Get(i)->m_civilisation->ResetCiv(
		    currNation, player_Get(i)->m_civilisation->GetGender());
		if (!leader.empty())
		{
			player_Get(i)->m_civilisation->AccessData()->SetLeaderName(leader.c_str());
			if (i == profiledb_Get()->GetPlayerIndex())
				profiledb_Get()->SetLeaderName(leader.c_str());
		}
	}
	return true;
}
