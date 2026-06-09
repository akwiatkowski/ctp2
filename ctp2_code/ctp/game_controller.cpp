//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ source
// Description  : UI-free command/query dispatch for the test/automation API.
//
//----------------------------------------------------------------------------
//
// See game_controller.h for the design rationale.  Every handler here operates
// purely on game-state objects and is safe to call in both the interactive and
// headless builds.  Responses are built with nlohmann::json and dumped as a
// single line (no embedded newlines), so they ride the existing
// newline-delimited socket protocol without truncation.
//
//----------------------------------------------------------------------------

#include "ctp/c3.h"
#include "ctp/game_controller.h"

#include <nlohmann/json.hpp>

#include <cstdio>
#include <cstring>
#include <string>

#include "ctp/civapp.h"                       // civapp_Get()->IsGameLoaded()
#include "ctp/ctp2_utils/civlog.h"            // civlog::Get
#include "gs/utility/Globals.h"               // k_MAX_PLAYERS, k_GAME_OBJ_TYPE_*
#include "gs/gameobj/Player.h"                // player_Get, Player
#include "gs/gameobj/Army.h"                  // Army
#include "gs/gameobj/ArmyData.h"              // ArmyData::Settle / CanSettle
#include "gs/gameobj/Unit.h"                  // Unit
#include "gs/gameobj/UnitData.h"              // Unit::GetData / GetCityData
#include "gs/gameobj/CityData.h"              // CityData
#include "gs/gameobj/BldQue.h"                // BuildQueue / BuildNode
#include "gs/gameobj/Vision.h"                // Vision::IsVisible
#include "gs/utility/UnitDynArr.h"            // UnitDynamicArray
#include "gs/fileio/gamefile.h"               // GameFile::SaveGame / RestoreGame
#include "gs/events/GameEventManager.h"       // gevmanager_Get()->Process()
#include "UnitRecord.h"                       // g_theUnitDB, UnitRecord

using json = nlohmann::json;

namespace {

auto gc_log = civlog::Get("gamectl");

// ---- response builders --------------------------------------------------

// {"status":"ok","cmd":"<verb>","result":{...}}  (result omitted if null)
std::string Ok(const char * verb, const json & result = json())
{
    json r;
    r["status"] = "ok";
    r["cmd"]    = verb;
    if (!result.is_null())
        r["result"] = result;
    return r.dump();
}

// {"status":"error","cmd":"<verb>","detail":"<code>"}
std::string Err(const char * verb, const char * code)
{
    json r;
    r["status"] = "error";
    r["cmd"]    = verb;
    r["detail"] = code;
    return r.dump();
}

// The "visible player" whose viewpoint queries report — the (single) human.
// We deliberately do NOT use selitem_Get() here: it is a UI singleton that may
// be absent/empty headless.  The human player's own vision is the correct,
// build-independent source for "what the player can see".
Player * HumanPlayer()
{
    for (sint32 p = 0; p < k_MAX_PLAYERS; ++p) {
        if (player_Get(p) && player_Get(p)->IsHuman())
            return player_Get(p);
    }
    return nullptr;
}

// ---- commands -----------------------------------------------------------

// Found a city with the human player's first settler-capable army.  Settle()
// only queues a GEV_Settle event, so we pump the event manager to make the
// command synchronous: the city exists by the time we respond.
std::string CmdBuildCity()
{
    if (!civapp_Get() || !civapp_Get()->IsGameLoaded())
        return Err("build_city", "game_not_loaded");

    Player * human = HumanPlayer();
    if (!human)
        return Err("build_city", "no_human_player");

    DynamicArray<Army> * armies = human->GetAllArmiesList();
    for (sint32 i = 0; i < armies->Num(); ++i) {
        Army army = armies->Access(i);
        if (army.IsValid() && army.CanSettle()) {
            gc_log->info("build_city: settling with army {} of player {}",
                         i, (int)human->GetOwner());
            army.AccessData()->Settle();
            // Drain the queued GEV_Settle (and any cascade) so the city is
            // actually founded before we return — keeps the driver synchronous.
            if (gevmanager_Get())
                gevmanager_Get()->Process();
            return Ok("build_city");
        }
    }
    return Err("build_city", "no_settler_found");
}

// set_production <city_idx> <unit_keyword>
// unit_keyword: a numeric unit type, or "cheapest_military" / "settler".
std::string CmdSetProduction(const char * args)
{
    if (!civapp_Get() || !civapp_Get()->IsGameLoaded())
        return Err("set_production", "game_not_loaded");

    int  city_idx = 0;
    char keyword[64];
    if (sscanf(args, "%d %63s", &city_idx, keyword) != 2)
        return Err("set_production", "bad_args");

    Player * human = HumanPlayer();
    if (!human)
        return Err("set_production", "no_human_player");
    if (city_idx < 0 || city_idx >= human->GetAllCitiesList()->Num())
        return Err("set_production", "bad_city_index");

    Unit city = human->GetAllCitiesList()->Access(city_idx);
    if (!city.IsValid() || !city.GetData()->GetCityData())
        return Err("set_production", "invalid_city");

    CityData * cd       = city.GetData()->GetCityData();
    sint32     gov_type = human->GetGovernmentType();
    sint32     unit_type = -1;

    if (strcmp(keyword, "cheapest_military") == 0) {
        sint32 best = 0x7fffffff;
        for (sint32 i = 0; i < g_theUnitDB->NumRecords(); ++i) {
            const UnitRecord * rec = g_theUnitDB->Get(i, gov_type);
            if (!rec || rec->GetCantBuild() || rec->GetAttack() <= 0.0) continue;
            if (!cd->CanBuildUnit(i)) continue;
            sint32 cost = rec->GetShieldCost();
            if (cost > 0 && cost < best) { best = cost; unit_type = i; }
        }
    } else if (strcmp(keyword, "settler") == 0) {
        sint32 best = 0x7fffffff;
        for (sint32 i = 0; i < g_theUnitDB->NumRecords(); ++i) {
            const UnitRecord * rec = g_theUnitDB->Get(i, gov_type);
            if (!rec || rec->GetCantBuild()) continue;
            if (!rec->GetSettle() && rec->GetNumCanSettleOn() <= 0) continue;
            if (!cd->CanBuildUnit(i)) continue;
            sint32 cost = rec->GetShieldCost();
            if (cost > 0 && cost < best) { best = cost; unit_type = i; }
        }
    } else {
        unit_type = atoi(keyword);
    }

    if (unit_type < 0 || unit_type >= g_theUnitDB->NumRecords())
        return Err("set_production", "unit_not_found");
    if (!cd->CanBuildUnit(unit_type))
        return Err("set_production", "cannot_build_unit");

    gc_log->info("set_production: city {} -> unit {}", city_idx, (int)unit_type);
    cd->BuildUnit(unit_type);

    json result;
    result["city"]     = city_idx;
    result["category"] = k_GAME_OBJ_TYPE_UNIT;
    result["type"]     = unit_type;
    return Ok("set_production", result);
}

// save_game <path>  — binary or JSON depending on extension (GameFile decides).
std::string CmdSaveGame(const char * args)
{
    if (!civapp_Get() || !civapp_Get()->IsGameLoaded())
        return Err("save_game", "game_not_loaded");
    if (!args[0])
        return Err("save_game", "bad_args");
    gc_log->info("save_game: {}", args);
    GameFile::SaveGame(args, nullptr);
    return Ok("save_game");
}

// load_game <path>
std::string CmdLoadGame(const char * args)
{
    if (!args[0])
        return Err("load_game", "bad_args");
    gc_log->info("load_game: {}", args);
    GameFile::RestoreGame(args);
    return Ok("load_game");
}

// ---- queries ------------------------------------------------------------

// Describe one city for the human's viewpoint.
json CityJson(sint32 owner, sint32 city_idx, const Unit & u)
{
    json c;
    c["owner"] = owner;
    c["index"] = city_idx;
    MapPoint pos;
    u.GetPos(pos);
    c["pos"] = { {"x", pos.x}, {"y", pos.y} };
    const char * name = u.GetName();
    c["name"] = name ? name : "";

    CityData * cd = u.GetData() ? u.GetData()->GetCityData() : nullptr;
    if (cd) {
        c["population"] = cd->PopCount();
        BuildNode * head = cd->GetBuildQueue() ? cd->GetBuildQueue()->GetHead() : nullptr;
        if (head) {
            c["building"] = { {"category", head->m_category},
                              {"type",     head->m_type},
                              {"cost",     head->m_cost} };
        } else {
            c["building"] = nullptr;
        }
    }
    return c;
}

// query_cities — every city visible to the human (fog-of-war filtered).
std::string QueryCities()
{
    if (!civapp_Get() || !civapp_Get()->IsGameLoaded())
        return Err("query_cities", "game_not_loaded");

    Player * human = HumanPlayer();
    if (!human)
        return Err("query_cities", "no_human_player");

    json cities = json::array();
    for (sint32 p = 0; p < k_MAX_PLAYERS; ++p) {
        if (!player_Get(p)) continue;
        UnitDynamicArray * list = player_Get(p)->GetAllCitiesList();
        if (!list) continue;
        for (sint32 i = 0; i < list->Num(); ++i) {
            Unit u = list->Access(i);
            if (!u.IsValid()) continue;
            MapPoint pos;
            u.GetPos(pos);
            // Only what the human can actually see.  A player always sees its
            // own cities; enemy cities only when their tile is visible.
            bool visible = (p == human->GetOwner()) ||
                           (human->m_vision && human->m_vision->IsVisible(pos));
            if (!visible) continue;
            cities.push_back(CityJson(p, i, u));
        }
    }

    json result;
    result["visible_player"] = human->GetOwner();
    result["cities"]         = cities;
    return Ok("query_cities", result);
}

// query_city <city_idx>  — detail for one of the human's cities, including the
// buildable-unit affordance list ("what can I do here").
std::string QueryCity(const char * args)
{
    if (!civapp_Get() || !civapp_Get()->IsGameLoaded())
        return Err("query_city", "game_not_loaded");

    int city_idx = 0;
    if (sscanf(args, "%d", &city_idx) != 1)
        return Err("query_city", "bad_args");

    Player * human = HumanPlayer();
    if (!human)
        return Err("query_city", "no_human_player");
    if (city_idx < 0 || city_idx >= human->GetAllCitiesList()->Num())
        return Err("query_city", "bad_city_index");

    Unit u = human->GetAllCitiesList()->Access(city_idx);
    if (!u.IsValid() || !u.GetData() || !u.GetData()->GetCityData())
        return Err("query_city", "invalid_city");

    json result   = CityJson(human->GetOwner(), city_idx, u);
    CityData * cd = u.GetData()->GetCityData();
    sint32 gov    = human->GetGovernmentType();

    // Buildable units: the affordance set a player would see in the build menu.
    json buildable = json::array();
    for (sint32 i = 0; i < g_theUnitDB->NumRecords(); ++i) {
        if (!cd->CanBuildUnit(i)) continue;
        const UnitRecord * rec = g_theUnitDB->Get(i, gov);
        json item;
        item["category"] = k_GAME_OBJ_TYPE_UNIT;
        item["type"]     = i;
        item["cost"]     = rec ? rec->GetShieldCost() : 0;
        buildable.push_back(item);
    }
    result["buildable"] = buildable;
    return Ok("query_city", result);
}

}  // namespace

namespace game_controller {

std::string Dispatch(const std::string & line, bool & handled)
{
    handled = true;

    if (line == "build_city")                                  return CmdBuildCity();
    if (line.rfind("set_production ", 0) == 0)                  return CmdSetProduction(line.c_str() + 15);
    if (line.rfind("save_game ", 0) == 0)                       return CmdSaveGame(line.c_str() + 10);
    if (line.rfind("load_game ", 0) == 0)                       return CmdLoadGame(line.c_str() + 10);
    if (line == "query_cities")                                 return QueryCities();
    if (line.rfind("query_city ", 0) == 0)                      return QueryCity(line.c_str() + 11);
    if (line == "query_city")                                   return QueryCity("");

    handled = false;
    return std::string();
}

}  // namespace game_controller
