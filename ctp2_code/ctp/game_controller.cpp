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
#include "gs/gameobj/Vision.h"                // Vision::IsVisible / IsExplored
#include "gs/world/World.h"                   // world_Get(), GetCell
#include "gs/world/Cell.h"                    // Cell terrain / city / units
#include "gs/utility/UnitDynArr.h"            // UnitDynamicArray
#include "gs/fileio/gamefile.h"               // GameFile::SaveGame / RestoreGame
#include "gs/events/GameEventManager.h"       // gevmanager_Get()->Process()
#include "gs/gameobj/MovePath.h"              // army_QueueMovePath
#include "gs/gameobj/Events.h"                // GEV_ExploreOrder
#include "gs/gameobj/Score.h"                 // Score::GetTotalScore
#include "gs/gameobj/Civilisation.h"          // Civilisation::Get*CivName
#include "gs/utility/TurnCnt.h"               // turn_Get()->GetRound/GetYear
#include "UnitRecord.h"                       // g_theUnitDB, UnitRecord
#include "TerrainRecord.h"                    // g_theTerrainDB, TerrainRecord

using json = nlohmann::json;

namespace game_controller {

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

}  // namespace game_controller

namespace {

using game_controller::HumanPlayer;

auto gc_log = civlog::Get("gamectl");

// ---- response builders --------------------------------------------------

// Game strings reach us in TWO encodings: fresh-game strings come from the
// StringDB in Latin-1 (a Mali city named with an 0xE9 'é' broke
// query_player_cities live — nlohmann::json::dump() throws type_error.316
// on invalid UTF-8), while strings that round-tripped through a JSON save
// come back as valid UTF-8 (json_save transcodes on write but not back on
// load — see plan: the deep fix is load-side UTF-8→Latin-1 so the UI font
// path stays consistent). So: pass valid UTF-8 through untouched, and
// transcode anything else as Latin-1.
bool IsValidUtf8(const unsigned char * p)
{
    while (*p) {
        if (*p < 0x80) { ++p; continue; }
        int extra = (*p >= 0xF0) ? 3 : (*p >= 0xE0) ? 2 : (*p >= 0xC2) ? 1 : -1;
        if (extra < 0) return false;
        ++p;
        for (int i = 0; i < extra; ++i, ++p)
            if ((*p & 0xC0) != 0x80) return false;
    }
    return true;
}

std::string ToUtf8(const char * s)
{
    std::string out;
    if (!s) return out;
    if (IsValidUtf8((const unsigned char *)s)) return s;
    for (const unsigned char * p = (const unsigned char *)s; *p; ++p) {
        if (*p < 0x80) {
            out += (char)*p;
        } else {
            out += (char)(0xC0 | (*p >> 6));
            out += (char)(0x80 | (*p & 0x3F));
        }
    }
    return out;
}

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
        ArmyData * ad = army.AccessData();
        if (army.IsValid() && army.CanSettle() && ad) {
            // CanSettle() only checks the UNIT can settle, not the tile.
            // Settling on a tile that already has a city silently REPLACES
            // it (pop and improvements lost) — refuse instead. Found by the
            // first MCP playtest: a settler stuck on Rome's tile "founded"
            // a fresh pop-1 Rome over the pop-2 original.
            MapPoint at = ad->RetPos();
            Cell * cell = world_Get() ? world_Get()->GetCell(at) : nullptr;
            if (cell && cell->GetCity().IsValid())
                return Err("build_city", "tile_occupied");
            gc_log->info("build_city: settling with army {} of player {}",
                         i, (int)human->GetOwner());
            ad->Settle();
            // Drain the queued GEV_Settle (and any cascade) so the city is
            // actually founded before we return — keeps the driver synchronous.
            if (gevmanager_Get())
                gevmanager_Get()->Process();
            // The settle event can be VETOED downstream (city minimum
            // distance, terrain rules) with no error surfaced — verify the
            // city actually exists instead of reporting blind success.
            cell = world_Get() ? world_Get()->GetCell(at) : nullptr;
            if (!cell || !cell->GetCity().IsValid())
                return Err("build_city", "settle_rejected");
            json result;
            result["pos"] = { {"x", at.x}, {"y", at.y} };
            return Ok("build_city", result);
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

    // Cheapest buildable unit satisfying the keyword's predicate, or -1.
    auto cheapest_buildable = [&](auto && pred) {
        sint32 best = 0x7fffffff, found = -1;
        for (sint32 i = 0; i < g_theUnitDB->NumRecords(); ++i) {
            const UnitRecord * rec = g_theUnitDB->Get(i, gov_type);
            if (!rec || rec->GetCantBuild() || !pred(rec)) continue;
            if (!cd->CanBuildUnit(i)) continue;
            sint32 cost = rec->GetShieldCost();
            if (cost > 0 && cost < best) { best = cost; found = i; }
        }
        return found;
    };

    if (strcmp(keyword, "cheapest_military") == 0) {
        unit_type = cheapest_buildable(
            [](const UnitRecord * r) { return r->GetAttack() > 0.0; });
    } else if (strcmp(keyword, "settler") == 0) {
        unit_type = cheapest_buildable(
            [](const UnitRecord * r) { return r->GetSettle() || r->GetNumCanSettleOn() > 0; });
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

// load_game <path>  — actor recreation happens inside LoadJson, shared with
// the UI and headless batch load paths.
std::string CmdLoadGame(const char * args)
{
    if (!args[0])
        return Err("load_game", "bad_args");
    gc_log->info("load_game: {}", args);
    if (!GameFile::RestoreGame(args))
        return Err("load_game", "load_failed");
    return Ok("load_game");
}

// query_armies — the human's armies with what a player needs to command
// them: index (for move_army/auto_explore), position, movement points left
// this turn, settle capability, and the member units.
std::string QueryArmies()
{
    if (!civapp_Get() || !civapp_Get()->IsGameLoaded())
        return Err("query_armies", "game_not_loaded");

    Player * human = HumanPlayer();
    if (!human)
        return Err("query_armies", "no_human_player");

    json list = json::array();
    DynamicArray<Army> * armies = human->GetAllArmiesList();
    for (sint32 i = 0; armies && i < armies->Num(); ++i) {
        Army army = armies->Access(i);
        ArmyData * ad = army.AccessData();
        if (!army.IsValid() || !ad) continue;

        MapPoint pos = ad->RetPos();
        double moves = 0.0;
        ad->CurMinMovementPoints(moves);

        json units = json::array();
        for (sint32 u = 0; u < ad->Num(); ++u) {
            Unit unit = ad->Access(u);
            if (!unit.IsValid()) continue;
            json j;
            j["type"] = unit.GetType();
            j["name"] = ToUtf8(unit.GetName());
            j["hp"]   = unit.GetHP();
            units.push_back(j);
        }

        json a;
        a["index"]      = i;
        a["pos"]        = { {"x", pos.x}, {"y", pos.y} };
        a["moves_left"] = moves;
        a["can_settle"] = army.CanSettle();
        a["units"]      = units;
        list.push_back(a);
    }

    json result;
    result["armies"] = list;
    return Ok("query_armies", result);
}

// move_army <army_idx> <x> <y> — pathfind and queue a move order, then pump
// events so movement starts immediately. The army walks as far as this
// turn's movement points allow; the rest of the path continues on later
// turns. The result reports where the army actually stands afterwards.
std::string CmdMoveArmy(const char * args)
{
    if (!civapp_Get() || !civapp_Get()->IsGameLoaded())
        return Err("move_army", "game_not_loaded");

    int idx = -1, x = -1, y = -1;
    if (sscanf(args, "%d %d %d", &idx, &x, &y) != 3)
        return Err("move_army", "bad_args");

    Player * human = HumanPlayer();
    if (!human)
        return Err("move_army", "no_human_player");

    DynamicArray<Army> * armies = human->GetAllArmiesList();
    if (!armies || idx < 0 || idx >= armies->Num())
        return Err("move_army", "bad_army_index");

    World * w = world_Get();
    if (!w || x < 0 || x >= w->GetXWidth() || y < 0 || y >= w->GetYHeight())
        return Err("move_army", "bad_destination");

    Army army = armies->Access(idx);
    ArmyData * ad = army.AccessData();
    if (!army.IsValid() || !ad)
        return Err("move_army", "invalid_army");

    MapPoint src = ad->RetPos();
    MapPoint dest((sint16)x, (sint16)y);
    if (!army_QueueMovePath(human->GetOwner(), army, src, dest))
        return Err("move_army", "no_path");

    // A manual order overrides auto-explore — otherwise the explore tick
    // would re-route the army somewhere else next turn.
    for (sint32 u = 0; u < ad->Num(); ++u) {
        Unit unit = ad->Access(u);
        if (UnitData * ud = unit.AccessData()) ud->SetExploring(false);
    }

    // Drain the queued GEV_MoveOrder so the army starts walking now.
    if (gevmanager_Get())
        gevmanager_Get()->Process();

    MapPoint now = ad->RetPos();
    gc_log->info("move_army: army {} ({},{}) -> ({},{}), now at ({},{})",
                 idx, (int)src.x, (int)src.y, x, y, (int)now.x, (int)now.y);

    json result;
    result["army"] = idx;
    result["from"] = { {"x", src.x}, {"y", src.y} };
    result["dest"] = { {"x", x}, {"y", y} };
    result["pos"]  = { {"x", now.x}, {"y", now.y} };
    result["arrived"] = (now.x == x && now.y == y);
    return Ok("move_army", result);
}

// auto_explore <army_idx> — hand the army to the explore order; the per-turn
// hook keeps re-picking new targets, revealing the map without driving every
// step by hand.
std::string CmdAutoExplore(const char * args)
{
    if (!civapp_Get() || !civapp_Get()->IsGameLoaded())
        return Err("auto_explore", "game_not_loaded");

    int idx = -1;
    if (sscanf(args, "%d", &idx) != 1)
        return Err("auto_explore", "bad_args");

    Player * human = HumanPlayer();
    if (!human)
        return Err("auto_explore", "no_human_player");

    DynamicArray<Army> * armies = human->GetAllArmiesList();
    if (!armies || idx < 0 || idx >= armies->Num())
        return Err("auto_explore", "bad_army_index");

    Army army = armies->Access(idx);
    if (!army.IsValid())
        return Err("auto_explore", "invalid_army");

    if (!gevmanager_Get())
        return Err("auto_explore", "no_event_manager");
    gevmanager_Get()->AddEvent(GEV_INSERT_Tail, GEV_ExploreOrder,
                               GEA_Army, army, GEA_End);
    gevmanager_Get()->Process();

    gc_log->info("auto_explore: army {}", idx);
    return Ok("auto_explore");
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
    c["name"] = ToUtf8(u.GetName());

    CityData * cd = u.GetData() ? u.GetData()->GetCityData() : nullptr;
    if (cd) {
        c["population"] = cd->PopCount();
        // Net per-turn yields — what the city panel shows the player.
        json y;
        y["food"]       = cd->GetNetCityFood();
        y["production"] = cd->GetNetCityProduction();
        y["gold"]       = cd->GetNetCityGold();
        y["science"]    = cd->GetScience();
        y["happiness"]  = cd->GetHappiness();
        c["yields"] = y;
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

// query_units — every unit the human can see (fog-of-war filtered via the unit
// visibility bitmask, same mechanism the renderer uses). Includes the human's
// own units. Reports type, position, hp and whether it is a city.
std::string QueryUnits()
{
    if (!civapp_Get() || !civapp_Get()->IsGameLoaded())
        return Err("query_units", "game_not_loaded");

    Player * human = HumanPlayer();
    if (!human)
        return Err("query_units", "no_human_player");
    sint32 vis = human->GetOwner();

    json units = json::array();
    for (sint32 p = 0; p < k_MAX_PLAYERS; ++p) {
        if (!player_Get(p) || !player_Get(p)->m_all_units) continue;
        for (sint32 i = 0; i < player_Get(p)->m_all_units->Num(); ++i) {
            Unit u = player_Get(p)->m_all_units->Access(i);
            if (!u.IsValid()) continue;
            if (!(u.GetVisibility() & (1 << vis))) continue;
            MapPoint pos;
            u.GetPos(pos);
            json j;
            j["owner"]   = p;
            j["type"]    = u.GetType();
            j["name"]    = ToUtf8(u.GetName());
            j["pos"]     = { {"x", pos.x}, {"y", pos.y} };
            j["hp"]      = u.GetHP();
            j["is_city"] = u.IsCity();
            units.push_back(j);
        }
    }

    json result;
    result["visible_player"] = vis;
    result["units"]          = units;
    return Ok("query_units", result);
}

// query_map — terrain, fog state and city markers for every tile the human has
// explored. Only explored tiles are listed (unexplored tiles are omitted
// entirely); the "visible" flag distinguishes currently-seen tiles from
// remembered ones. Units belong to query_units; this stays terrain+cities.
std::string QueryMap()
{
    if (!civapp_Get() || !civapp_Get()->IsGameLoaded())
        return Err("query_map", "game_not_loaded");

    Player * human = HumanPlayer();
    if (!human)
        return Err("query_map", "no_human_player");
    World * w = world_Get();
    if (!w)
        return Err("query_map", "no_world");
    Vision * vis = human->m_vision;

    const sint32 W = w->GetXWidth();
    const sint32 H = w->GetYHeight();
    json tiles = json::array();
    sint32 n_explored = 0, n_visible = 0;

    for (sint32 y = 0; y < H; ++y) {
        for (sint32 x = 0; x < W; ++x) {
            MapPoint pos(x, y);
            if (!(vis && vis->IsExplored(pos))) continue;
            ++n_explored;
            bool visible = vis->IsVisible(pos);
            if (visible) ++n_visible;

            Cell * c = w->GetCell(pos);
            json t;
            t["x"]       = x;
            t["y"]       = y;
            t["terrain"] = c ? c->GetTerrain() : -1;
            t["visible"] = visible;
            if (c) {
                Unit city = c->GetCity();
                if (city.IsValid())
                    t["city"] = (sint32)city.GetOwner();
            }
            tiles.push_back(t);
        }
    }

    json result;
    result["visible_player"] = human->GetOwner();
    result["width"]    = W;
    result["height"]   = H;
    result["explored"] = n_explored;
    result["visible"]  = n_visible;
    result["tiles"]    = tiles;
    return Ok("query_map", result);
}

// ---- admin queries --------------------------------------------------------
//
// Unlike the player-view queries above, these are OMNISCIENT: they report the
// whole game state with no fog-of-war filtering. They exist for the gateway's
// admin panel and debugging — a driver that wants the player's perspective
// must use the query_* family instead.

// One player slot, shared by query_players and query_player so the two can
// never drift apart.
json PlayerJson(sint32 p, Player * pl)
{
    std::string name = ToUtf8(pl->GetLeaderName());
    // Civilisation names come from the game's StringDB via the player's
    // Civilisation object — never synthesized here.
    MBCHAR civ[k_MAX_NAME_LEN]     = {0};
    MBCHAR country[k_MAX_NAME_LEN] = {0};
    Civilisation * c = pl->GetCivilisation();
    if (c && c->AccessData()) {
        c->GetSingularCivName(civ);
        c->GetCountryName(country);
    }
    json j;
    j["id"]         = p;
    j["name"]       = name;
    j["civ"]        = ToUtf8(civ);       // adjective/singular, e.g. "Roman"
    j["country"]    = ToUtf8(country);   // nation, e.g. "Rome"
    j["human"]      = pl->IsHuman();
    j["dead"]       = pl->IsDead();
    j["gold"]       = pl->GetGold();
    j["num_cities"] = pl->GetNumCities();
    j["num_units"]  = pl->m_all_units ? pl->m_all_units->Num() : 0;
    j["num_armies"] = pl->GetAllArmiesList() ? pl->GetAllArmiesList()->Num() : 0;
    j["government"] = pl->GetGovernmentType();
    j["score"]      = pl->m_score ? pl->m_score->GetTotalScore() : 0;
    return j;
}

// query_players — every live player slot with headline stats.
std::string QueryPlayers()
{
    if (!civapp_Get() || !civapp_Get()->IsGameLoaded())
        return Err("query_players", "game_not_loaded");

    json players = json::array();
    for (sint32 p = 0; p < k_MAX_PLAYERS; ++p) {
        Player * pl = player_Get(p);
        if (!pl) continue;
        players.push_back(PlayerJson(p, pl));
    }

    json result;
    result["players"] = players;
    return Ok("query_players", result);
}

// query_player <id> — one player slot (same shape as a query_players entry).
std::string QueryPlayer(const char * args)
{
    if (!civapp_Get() || !civapp_Get()->IsGameLoaded())
        return Err("query_player", "game_not_loaded");

    int id = -1;
    if (sscanf(args, "%d", &id) != 1)
        return Err("query_player", "bad_args");
    if (id < 0 || id >= k_MAX_PLAYERS || !player_Get(id))
        return Err("query_player", "bad_player");

    return Ok("query_player", PlayerJson(id, player_Get(id)));
}

// query_turn — where the clock stands. Session-level accessors on purpose:
// the legacy GetRound/GetYear route through the currently-viewing PLAYER's
// recorded round, which lags the global clock between rounds — exactly the
// idle window in which this query runs.
std::string QueryTurn()
{
    if (!civapp_Get() || !civapp_Get()->IsGameLoaded())
        return Err("query_turn", "game_not_loaded");

    json result;
    result["round"] = turn_Get() ? turn_Get()->GetSessionRound() : 0;
    result["year"]  = turn_Get() ? turn_Get()->GetSessionYear()  : 0;
    return Ok("query_turn", result);
}

// query_player_cities <player_id> — ALL cities of one player (no fog filter).
std::string QueryPlayerCities(const char * args)
{
    if (!civapp_Get() || !civapp_Get()->IsGameLoaded())
        return Err("query_player_cities", "game_not_loaded");

    int id = -1;
    if (sscanf(args, "%d", &id) != 1)
        return Err("query_player_cities", "bad_args");
    if (id < 0 || id >= k_MAX_PLAYERS || !player_Get(id))
        return Err("query_player_cities", "bad_player");

    json cities = json::array();
    UnitDynamicArray * list = player_Get(id)->GetAllCitiesList();
    for (sint32 i = 0; list && i < list->Num(); ++i) {
        Unit u = list->Access(i);
        if (!u.IsValid()) continue;
        cities.push_back(CityJson(id, i, u));
    }

    json result;
    result["owner"]  = id;
    result["cities"] = cities;
    return Ok("query_player_cities", result);
}

// query_terrains — static dictionary mapping terrain ids (as reported by
// query_map) to names, passability and base tile yields. Values come from
// the game's TerrainRecord DB; fetch once and cache client-side.
std::string QueryTerrains()
{
    if (!g_theTerrainDB)
        return Err("query_terrains", "no_terrain_db");

    json list = json::array();
    for (sint32 i = 0; i < g_theTerrainDB->NumRecords(); ++i) {
        const TerrainRecord * t = g_theTerrainDB->Get(i);
        if (!t) continue;
        const TerrainRecord::Modifiers * m = t->GetEnvBase();
        sint32 movement = 0;
        if (m) m->GetMovement(movement);
        json j;
        j["id"]       = i;
        j["name"]     = ToUtf8(t->GetNameText());
        j["internal"] = ToUtf8(t->GetIDText());
        j["land"]     = t->GetMovementTypeLand();
        j["water"]    = t->GetMovementTypeSea() || t->GetMovementTypeShallowWater();
        j["mountain"] = t->GetMovementTypeMountain();
        j["food"]     = m ? m->GetFood()   : 0;
        j["shields"]  = m ? m->GetShield() : 0;
        j["gold"]     = m ? m->GetGold()   : 0;
        j["movement"] = movement;  // cost in 1/100 MP; 0 = record default
        list.push_back(j);
    }

    json result;
    result["terrains"] = list;
    return Ok("query_terrains", result);
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
    if (line == "query_units")                                  return QueryUnits();
    if (line == "query_armies")                                 return QueryArmies();
    if (line.rfind("move_army ", 0) == 0)                       return CmdMoveArmy(line.c_str() + 10);
    if (line.rfind("auto_explore ", 0) == 0)                    return CmdAutoExplore(line.c_str() + 13);
    if (line == "query_map")                                    return QueryMap();
    if (line == "query_players")                                return QueryPlayers();
    if (line.rfind("query_player_cities ", 0) == 0)             return QueryPlayerCities(line.c_str() + 20);
    if (line.rfind("query_player ", 0) == 0)                    return QueryPlayer(line.c_str() + 13);
    if (line == "query_turn")                                   return QueryTurn();
    if (line == "query_terrains")                               return QueryTerrains();

    handled = false;
    return std::string();
}

std::string DispatchSafe(const std::string & line, bool & handled)
{
    try {
        return Dispatch(line, handled);
    } catch (const std::exception & e) {
        gc_log->error("Dispatch threw on '{}': {}", line, e.what());
    } catch (...) {
        gc_log->error("Dispatch threw a non-std exception on '{}'", line);
    }
    handled = true;
    std::string verb = line.substr(0, line.find(' '));
    return Err(verb.c_str(), "exception");
}

}  // namespace game_controller
