//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ source
// Description  : JSON savegame entry points (Phase B: top-level skeleton)
//
//----------------------------------------------------------------------------
//
// Phase B extends the Phase A scaffold (magic + schema_version) with
// the top-level fields covering metadata + the 3 simple gs/ singletons:
//   - saved_at        ISO 8601 timestamp at SaveJson() call
//   - ctp2_build      git short SHA captured at configure time
//   - rng             RandomGenerator state (seed, buffer, indices,
//                     call count)
//   - turn            TurnCount state (round/turn/year + flags)
//   - settings        GameSettings (difficulty, risk, ages, pollution)
//   - selection       Minimal SelectionState — current_player only;
//                     the rest of SelectedItem lands in Phase E once
//                     Army/Unit JSON serialisation exists.
//
// Schema decisions locked in this file:
//   - snake_case JSON keys with `m_` stripped (Decision #2 in the plan)
//   - Hard-break on magic / schema mismatch (Decision #1)
//   - No migrators during migration phases
//
//----------------------------------------------------------------------------

#include "ctp/c3.h"
#include "gs/fileio/json_save.h"

#include "gs/utility/TurnCnt.h"
#include "gs/utility/RandGen.h"
#include "gs/gameobj/GameSettings.h"
#include "gs/world/Cell.h"
#include "gs/world/TileInfo.h"
#include "gs/world/UnseenCell.h"
#include "gs/world/World.h"
#include "gs/fileio/StartingPosition.h"
#include "ResourceRecord.h"               // g_theResourceDB (dbgen-built)
#include "gs/core/player_view.h"          // player_view::CurPlayer

#include <chrono>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

extern TurnCount       *g_turn;
extern GameSettings    *g_theGameSettings;
// g_rand is declared in RandGen.h
// g_theWorld is declared in World.h

// CTP2_BUILD_SHA is injected by meson into config.h (run_command git
// rev-parse --short).  Fall back to "unknown" if config.h hasn't been
// regenerated.
#ifndef CTP2_BUILD_SHA
#define CTP2_BUILD_SHA "unknown"
#endif

namespace {

// ISO 8601 / RFC 3339 UTC timestamp ("2026-05-30T11:00:00Z") for the
// "saved_at" field.  Captured at SaveJson time, never read back into
// game state — purely informational.
std::string iso_utc_now()
{
    auto const  now   = std::chrono::system_clock::now();
    std::time_t const t = std::chrono::system_clock::to_time_t(now);
    std::tm           tm;
    gmtime_r(&t, &tm);

    std::ostringstream out;
    out << std::put_time(&tm, "%Y-%m-%dT%H:%M:%SZ");
    return out.str();
}

}  // namespace

// --- gs/ singleton bridges ----------------------------------------------

void to_json(nlohmann::json &j, GameSettings const &gs)
{
    j = nlohmann::json{
        {"difficulty",      gs.m_difficulty},
        {"risk",            gs.m_risk},
        {"alien_end_game",  static_cast<bool>(gs.m_alienEndGame)},
        {"keep_score",      static_cast<bool>(gs.m_keepScore)},
        {"starting_age",    gs.m_startingAge},
        {"ending_age",      gs.m_endingAge},
        {"pollution",       static_cast<bool>(gs.m_pollution)},
    };
}

void from_json(nlohmann::json const &j, GameSettings &gs)
{
    j.at("difficulty")     .get_to(gs.m_difficulty);
    j.at("risk")           .get_to(gs.m_risk);
    gs.m_alienEndGame      = j.at("alien_end_game").get<bool>() ? TRUE : FALSE;
    gs.m_keepScore         = j.at("keep_score").get<bool>()     ? TRUE : FALSE;
    j.at("starting_age")   .get_to(gs.m_startingAge);
    j.at("ending_age")     .get_to(gs.m_endingAge);
    gs.m_pollution         = j.at("pollution").get<bool>()      ? TRUE : FALSE;
}

void to_json(nlohmann::json &j, TurnCount const &tc)
{
    // m_sliceList is a SimpleDynamicArray<sint32> — its contents
    // belong in Phase E (TurnCount is touched there for the unit
    // pipeline).  Phase B preserves the scalar fields the binary
    // Serialize() touches.
    j = nlohmann::json{
        {"round",                       tc.m_round},
        {"turn",                        tc.m_turn},
        {"year",                        tc.m_year},
        {"simultaneous_mode",           static_cast<bool>(tc.m_simultaneousMode)},
        {"active_players",              tc.m_activePlayers},
        {"last_begin_turn",             tc.m_lastBeginTurn},
        {"is_email",                    static_cast<bool>(tc.m_isEmail)},
        {"is_hot_seat",                 static_cast<bool>(tc.m_isHotSeat)},
        {"happiness_player",            tc.m_happinessPlayer},
        {"sent_game_over_message",      static_cast<bool>(tc.m_sentGameOverMessage)},
        {"sent_game_almost_over_message", static_cast<bool>(tc.m_sentGameAlmostOverMessage)},
    };
}

void from_json(nlohmann::json const &j, TurnCount &tc)
{
    j.at("round")                         .get_to(tc.m_round);
    j.at("turn")                          .get_to(tc.m_turn);
    j.at("year")                          .get_to(tc.m_year);
    tc.m_simultaneousMode = j.at("simultaneous_mode").get<bool>() ? TRUE : FALSE;
    j.at("active_players")                .get_to(tc.m_activePlayers);
    j.at("last_begin_turn")               .get_to(tc.m_lastBeginTurn);
    tc.m_isEmail   = j.at("is_email").get<bool>()   ? TRUE : FALSE;
    tc.m_isHotSeat = j.at("is_hot_seat").get<bool>() ? TRUE : FALSE;
    j.at("happiness_player")              .get_to(tc.m_happinessPlayer);
    tc.m_sentGameOverMessage =
        j.at("sent_game_over_message").get<bool>() ? 1u : 0u;
    tc.m_sentGameAlmostOverMessage =
        j.at("sent_game_almost_over_message").get<bool>() ? 1u : 0u;
}

void to_json(nlohmann::json &j, RandomGenerator const &rng)
{
    // m_firstp / m_secondp are pointers INTO m_buffer; serialise as
    // offsets so the load path can rebuild them from the new buffer's
    // base address.  m_endp is always &m_buffer[56] — no need to
    // persist it.  Matches the binary Serialize() at randgen.cpp:28.
    nlohmann::json buffer = nlohmann::json::array();
    for (sint32 i = 0; i < 56; ++i)
    {
        buffer.push_back(rng.m_buffer[i]);
    }

    j = nlohmann::json{
        {"seed",         rng.m_start_seed},
        {"buffer",       std::move(buffer)},
        {"first_index",  static_cast<sint32>(rng.m_firstp  - rng.m_buffer)},
        {"second_index", static_cast<sint32>(rng.m_secondp - rng.m_buffer)},
        {"call_count",   rng.m_callCount},
    };
}

void from_json(nlohmann::json const &j, RandomGenerator &rng)
{
    j.at("seed").get_to(rng.m_start_seed);

    auto const &buffer = j.at("buffer");
    if (buffer.size() != 56)
    {
        throw nlohmann::json::other_error::create(
            501, "rng.buffer must have exactly 56 entries", &j);
    }
    for (sint32 i = 0; i < 56; ++i)
    {
        rng.m_buffer[i] = buffer[i].get<sint32>();
    }

    rng.m_firstp  = rng.m_buffer + j.at("first_index").get<sint32>();
    rng.m_secondp = rng.m_buffer + j.at("second_index").get<sint32>();
    rng.m_endp    = &(rng.m_buffer[56]);
    j.at("call_count").get_to(rng.m_callCount);
}

// --- World-layer bridges (Phase C-1) ------------------------------------
// Scalar fields only.  Nested pointer-typed data (CellUnitList,
// DynamicArray<ID>, GoodyHut on Cell; PointerList<UnseenInstallationInfo>
// etc. on UnseenCell; GoodActor* on TileInfo) is deferred to Phase D/E
// when the contained types get their own to_json/from_json.

void to_json(nlohmann::json &j, Cell const &c)
{
    j = nlohmann::json{
        {"env",              c.m_env},
        {"zoc",              c.m_zoc},
        {"move_cost",        c.m_move_cost},
#ifdef BATTLE_FLAGS
        {"battle_flags",     c.m_battleFlags},
#endif
        {"continent_number", c.m_continent_number},
        {"gf",               c.m_gf},
        {"terrain_type",     c.m_terrain_type},
        // m_city is a Unit (ID derivative — wraps a uint32 id).  Serialise
        // through the ID base.  An empty city has id 0.
        {"city",             static_cast<ID const &>(c.m_city)},
        {"cell_owner",       c.m_cellOwner},
    };
}

void from_json(nlohmann::json const &j, Cell &c)
{
    j.at("env")             .get_to(c.m_env);
    j.at("zoc")             .get_to(c.m_zoc);
    j.at("move_cost")       .get_to(c.m_move_cost);
#ifdef BATTLE_FLAGS
    if (j.contains("battle_flags"))
    {
        j.at("battle_flags").get_to(c.m_battleFlags);
    }
#endif
    j.at("continent_number").get_to(c.m_continent_number);
    j.at("gf")              .get_to(c.m_gf);
    j.at("terrain_type")    .get_to(c.m_terrain_type);
    ID city_id(0);
    j.at("city")            .get_to(city_id);
    c.m_city = Unit(city_id.m_id);
    j.at("cell_owner")      .get_to(c.m_cellOwner);
}

void to_json(nlohmann::json &j, TileInfo const &t)
{
    nlohmann::json transitions = nlohmann::json::array();
    for (sint32 i = 0; i < k_NUM_TRANSITIONS; ++i)
    {
        transitions.push_back(t.m_transitions[i]);
    }
    j = nlohmann::json{
        {"river_piece",  t.m_riverPiece},
        {"mega_info",    t.m_megaInfo},
        {"terrain_type", t.m_terrainType},
        {"transform",    t.m_transform},
        {"tile_num",     t.m_tileNum},
        {"transitions",  std::move(transitions)},
    };
}

void from_json(nlohmann::json const &j, TileInfo &t)
{
    j.at("river_piece") .get_to(t.m_riverPiece);
    j.at("mega_info")   .get_to(t.m_megaInfo);
    j.at("terrain_type").get_to(t.m_terrainType);
    j.at("transform")   .get_to(t.m_transform);
    j.at("tile_num")    .get_to(t.m_tileNum);

    auto const &transitions = j.at("transitions");
    if (transitions.size() != k_NUM_TRANSITIONS)
    {
        throw nlohmann::json::other_error::create(
            502, "tile_info.transitions must have exactly "
                 "k_NUM_TRANSITIONS entries", &j);
    }
    for (sint32 i = 0; i < k_NUM_TRANSITIONS; ++i)
    {
        transitions[i].get_to(t.m_transitions[i]);
    }
    // m_goodActor is a UI sprite pointer — never carried in saves.
    // The UI regenerates it on load via the goodactor_factory observer.
}

void to_json(nlohmann::json &j, UnseenCell const &uc)
{
    j = nlohmann::json{
        {"env",                    uc.m_env},
        {"terrain_type",           uc.m_terrain_type},
        {"move_cost",              uc.m_move_cost},
        {"flags",                  uc.m_flags},
        {"bio_infected_owner",     uc.m_bioInfectedOwner},
        {"nano_infected_owner",    uc.m_nanoInfectedOwner},
        {"converted_owner",        uc.m_convertedOwner},
        {"franchise_owner",        uc.m_franchiseOwner},
        {"injoined_owner",         uc.m_injoinedOwner},
        {"happiness_attack_owner", uc.m_happinessAttackOwner},
        {"city_size",              uc.m_citySize},
        {"city_owner",             uc.m_cityOwner},
        {"city_sprite_index",      uc.m_citySpriteIndex},
        {"cell_owner",             uc.m_cell_owner},
        {"slave_bits",             uc.m_slaveBits},
#ifdef BATTLE_FLAGS
        {"battle_flags",           uc.m_battleFlags},
#endif
        {"position",               uc.m_point},
        {"visible_city_owner",     uc.m_visibleCityOwner},
    };
    // m_actor, m_snapshotState, m_installations, m_improvements,
    // m_cityName, m_tileInfo, m_poolIndex: OMITTED — see header.
}

void from_json(nlohmann::json const &j, UnseenCell &uc)
{
    j.at("env")                   .get_to(uc.m_env);
    j.at("terrain_type")          .get_to(uc.m_terrain_type);
    j.at("move_cost")             .get_to(uc.m_move_cost);
    j.at("flags")                 .get_to(uc.m_flags);
    j.at("bio_infected_owner")    .get_to(uc.m_bioInfectedOwner);
    j.at("nano_infected_owner")   .get_to(uc.m_nanoInfectedOwner);
    j.at("converted_owner")       .get_to(uc.m_convertedOwner);
    j.at("franchise_owner")       .get_to(uc.m_franchiseOwner);
    j.at("injoined_owner")        .get_to(uc.m_injoinedOwner);
    j.at("happiness_attack_owner").get_to(uc.m_happinessAttackOwner);
    j.at("city_size")             .get_to(uc.m_citySize);
    j.at("city_owner")            .get_to(uc.m_cityOwner);
    j.at("city_sprite_index")     .get_to(uc.m_citySpriteIndex);
    j.at("cell_owner")            .get_to(uc.m_cell_owner);
    j.at("slave_bits")            .get_to(uc.m_slaveBits);
#ifdef BATTLE_FLAGS
    if (j.contains("battle_flags"))
    {
        j.at("battle_flags")      .get_to(uc.m_battleFlags);
    }
#endif
    j.at("position")              .get_to(uc.m_point);
    j.at("visible_city_owner")    .get_to(uc.m_visibleCityOwner);
}

// --- StartingPosition bridge (used inside World) ---

void to_json(nlohmann::json &j, StartingPosition const &sp)
{
    j = nlohmann::json{
        {"position",  sp.point},
        {"civ_index", sp.civIndex},
    };
}

void from_json(nlohmann::json const &j, StartingPosition &sp)
{
    j.at("position") .get_to(sp.point);
    j.at("civ_index").get_to(sp.civIndex);
}

// --- World bridge (Phase C-2) ------------------------------------------
// Mirrors World::Serialize at wldgen.cpp:2343.  Dense per-cell array
// per the JSON migration plan ("Recommend dense for v1 — sparse is an
// optimization for later").  Map dimensions, wrap flags, continent
// metadata, the dense cell + tile-info arrays, civ starts, and the
// per-good-record value table all round-trip.  Transient bookkeeping
// (m_radiusOp, m_distanceQueue, etc.) and derived continent arrays
// (m_water_next_too_land etc.) are OMITTED — they're rebuilt during
// gameplay or recomputed by ComputeGoodsValues().

extern sint32 g_numGoods;   // wldgen.cpp:2326

void to_json(nlohmann::json &j, World const &w)
{
    sint32 const width  = w.m_size.x;
    sint32 const height = w.m_size.y;
    sint32 const len    = width * height;

    // tile_info_storage: flat row-major.
    nlohmann::json tile_info_storage = nlohmann::json::array();
    for (sint32 i = 0; i < len; ++i)
    {
        tile_info_storage.push_back(w.m_tileInfoStorage[i]);
    }

    // cells: nested [x][y] arrays so the schema makes the 2D shape
    // explicit to modders + tools.  Plan recommends dense; this
    // also keeps load-time bounds checking simple.
    nlohmann::json cells = nlohmann::json::array();
    for (sint32 x = 0; x < width; ++x)
    {
        nlohmann::json column = nlohmann::json::array();
        for (sint32 y = 0; y < height; ++y)
        {
            column.push_back(*w.m_map[x][y]);
        }
        cells.push_back(std::move(column));
    }

    // civ_starts (only first num_civ_starts entries carry data).
    nlohmann::json civ_starts = nlohmann::json::array();
    for (sint32 i = 0; i < w.m_num_civ_starts; ++i)
    {
        civ_starts.push_back(w.m_civ_starts[i]);
    }

    // good_value: per-resource-record value.  Database-sized; the
    // load side recomputes if the DB grew.
    nlohmann::json good_value = nlohmann::json::array();
    if (w.m_goodValue && g_theResourceDB)
    {
        sint32 const n = g_theResourceDB->NumRecords();
        for (sint32 i = 0; i < n; ++i)
        {
            good_value.push_back(w.m_goodValue[i]);
        }
    }

    j = nlohmann::json{
        {"size_x",                 width},
        {"size_y",                 height},
        {"is_xwrap",               static_cast<bool>(w.m_isXwrap)},
        {"is_ywrap",               static_cast<bool>(w.m_isYwrap)},
        {"continents_are_numbered", static_cast<bool>(w.m_continents_are_numbered)},
        {"water_continent_max",    w.m_water_continent_max},
        {"land_continent_max",     w.m_land_continent_max},
        {"tile_info_storage",      std::move(tile_info_storage)},
        {"cells",                  std::move(cells)},
        {"num_civ_starts",         w.m_num_civ_starts},
        {"civ_starts",             std::move(civ_starts)},
        {"good_value",             std::move(good_value)},
    };
}

void from_json(nlohmann::json const &j, World &w)
{
    // Free any existing map state and reallocate at the saved size.
    // Matches the binary load path at wldgen.cpp:2385.
    w.FreeMap();

    sint32 size_x  = j.at("size_x").get<sint32>();
    sint32 size_y  = j.at("size_y").get<sint32>();
    bool   xwrap   = j.at("is_xwrap").get<bool>();
    bool   ywrap   = j.at("is_ywrap").get<bool>();
    w.m_isXwrap    = xwrap ? 1 : 0;
    w.m_isYwrap    = ywrap ? 1 : 0;
    w.m_continents_are_numbered =
        j.at("continents_are_numbered").get<bool>() ? TRUE : FALSE;
    j.at("water_continent_max").get_to(w.m_water_continent_max);
    j.at("land_continent_max") .get_to(w.m_land_continent_max);

    w.m_size = MapPoint(size_x, size_y);
    w.AllocateMap();

    auto const &tile_info_storage = j.at("tile_info_storage");
    sint32 const len = size_x * size_y;
    if (static_cast<sint32>(tile_info_storage.size()) != len)
    {
        throw nlohmann::json::other_error::create(
            503, "world.tile_info_storage size mismatch with size_x*size_y",
            &j);
    }
    for (sint32 i = 0; i < len; ++i)
    {
        tile_info_storage[i].get_to(w.m_tileInfoStorage[i]);
    }

    auto const &cells = j.at("cells");
    if (static_cast<sint32>(cells.size()) != size_x)
    {
        throw nlohmann::json::other_error::create(
            504, "world.cells outer length mismatch with size_x", &j);
    }
    for (sint32 x = 0; x < size_x; ++x)
    {
        auto const &column = cells[x];
        if (static_cast<sint32>(column.size()) != size_y)
        {
            throw nlohmann::json::other_error::create(
                505, "world.cells inner length mismatch with size_y", &j);
        }
        for (sint32 y = 0; y < size_y; ++y)
        {
            column[y].get_to(*w.m_map[x][y]);
        }
    }

    j.at("num_civ_starts").get_to(w.m_num_civ_starts);
    auto const &civ_starts = j.at("civ_starts");
    for (sint32 i = 0; i < w.m_num_civ_starts
                       && i < static_cast<sint32>(civ_starts.size()); ++i)
    {
        civ_starts[i].get_to(w.m_civ_starts[i]);
    }

    // good_value: re-allocate based on the saved count.  If the
    // current database doesn't match, the load side discards (matches
    // wldgen.cpp:2418 — would normally call ComputeGoodsValues).
    auto const &good_value = j.at("good_value");
    sint32 const n_goods = static_cast<sint32>(good_value.size());
    if (g_theResourceDB && n_goods == g_theResourceDB->NumRecords())
    {
        delete[] w.m_goodValue;
        w.m_goodValue = new double[n_goods];
        for (sint32 i = 0; i < n_goods; ++i)
        {
            good_value[i].get_to(w.m_goodValue[i]);
        }
        g_numGoods = n_goods;
    }
    // Database size changed: the binary path calls
    // ComputeGoodsValues() here.  Phase C-2 leaves m_goodValue alone
    // for the JSON path — Phase D's player + city deps will revisit.
}

namespace json_save {

bool SaveJson(char const *path)
{
    nlohmann::json doc;
    doc["magic"]          = MAGIC;
    doc["schema_version"] = SCHEMA_VERSION;
    doc["saved_at"]       = iso_utc_now();
    doc["ctp2_build"]     = CTP2_BUILD_SHA;

    if (g_rand)             doc["rng"]      = *g_rand;
    if (g_turn)             doc["turn"]     = *g_turn;
    if (g_theGameSettings)  doc["settings"] = *g_theGameSettings;
    if (g_theWorld)         doc["world"]    = *g_theWorld;

    // Selection is currently a scalar projection of player_view
    // state — Phase E expands it.  Skipped when player_view is not
    // wired (headless before InitializeGameHeadless).
    SelectionState sel;
    sel.current_player = player_view::CurPlayer();
    doc["selection"] = sel;

    std::ofstream out(path);
    if (!out)
    {
        std::cerr << "[json_save] SaveJson: cannot open '" << path
                  << "' for writing\n";
        return false;
    }
    out << doc.dump(2);
    return out.good();
}

bool LoadJson(char const *path)
{
    std::ifstream in(path);
    if (!in)
    {
        std::cerr << "[json_save] LoadJson: cannot open '" << path
                  << "' for reading\n";
        return false;
    }

    nlohmann::json doc;
    try
    {
        in >> doc;
    }
    catch (nlohmann::json::parse_error const &e)
    {
        std::cerr << "[json_save] LoadJson: parse error at '" << path
                  << "': " << e.what() << "\n";
        return false;
    }

    if (!doc.contains("magic") || doc["magic"] != MAGIC)
    {
        std::cerr << "[json_save] LoadJson: bad magic in '" << path
                  << "' (expected \"" << MAGIC << "\")\n";
        return false;
    }
    if (!doc.contains("schema_version")
        || doc["schema_version"].get<int>() != SCHEMA_VERSION)
    {
        std::cerr << "[json_save] LoadJson: schema_version mismatch in '"
                  << path << "' (expected " << SCHEMA_VERSION << ")\n";
        return false;
    }

    // Populate game-state singletons if they exist.  Phase B accepts
    // partial files — a load that's missing "rng" or "turn" only
    // skips those (the round-trip test exercises the full shape).
    try
    {
        if (doc.contains("rng") && g_rand)
        {
            doc.at("rng").get_to(*g_rand);
        }
        if (doc.contains("turn") && g_turn)
        {
            doc.at("turn").get_to(*g_turn);
        }
        if (doc.contains("settings") && g_theGameSettings)
        {
            doc.at("settings").get_to(*g_theGameSettings);
        }
        if (doc.contains("world") && g_theWorld)
        {
            doc.at("world").get_to(*g_theWorld);
        }
        // Selection is currently informational — no public setter
        // for SelectedItem::m_current_player.  Phase E wires a bridge.
    }
    catch (nlohmann::json::exception const &e)
    {
        std::cerr << "[json_save] LoadJson: deserialisation error at '"
                  << path << "': " << e.what() << "\n";
        return false;
    }

    return true;
}

}  // namespace json_save
