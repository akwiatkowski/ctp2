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
#include "gs/gameobj/Score.h"
#include "gs/gameobj/Regard.h"
#include "gs/gameobj/TaxRate.h"
#include "gs/gameobj/Sci.h"               // Science
#include "gs/gameobj/Readiness.h"         // MilitaryReadiness
#include "gs/gameobj/pollution.h"
#include "gs/gameobj/PollutionConst.h"    // k_MAX_GLOBAL_POLLUTION_RECORD_TURNS
#include "gs/gameobj/WonderTracker.h"
#include "gs/gameobj/AchievementTracker.h"
#include "gs/gameobj/Advances.h"
#include "gs/gameobj/Happy.h"             // HappyTimer (full Happy is D-3+)
#include "gs/gameobj/HappyTracker.h"
#include "gs/gameobj/Exclusions.h"
#include "gs/gameobj/Strengths.h"
#include "gs/gameobj/AgreementData.h"
#include "gs/gameobj/GameObj.h"            // GameObj::Serialize base
#include "gs/gameobj/CivilisationData.h"
#include "gs/gameobj/TradeOfferData.h"
#include "gs/gameobj/BldQue.h"
#include "ctp/ctp2_utils/pointerlist.h"    // PointerList<BuildNode>::Walker
#include "gs/gameobj/FeatTracker.h"
#include "FeatRecord.h"                    // g_theFeatDB (dbgen-built)
#include "BuildingRecord.h"                // g_theBuildingDB (dbgen-built)
#include "gs/gameobj/gaiacontroller.h"
#include "ai/diplomacy/AgreementMatrix.h"
#include "ai/diplomacy/Diplomat.h"
#include "ai/diplomacy/Foreigner.h"
#include "gs/gameobj/citydata.h"
#include "gs/gameobj/UnitTypes.h"           // POP_MAX
#include "gs/gameobj/Player.h"
#include "gs/gameobj/PollutionConst.h"      // already included via pollution.h, kept explicit
#include "gs/gameobj/UnitState.h"
#include "gs/gameobj/Order.h"
#include "CivilisationRecord.h"            // k_MAX_CityName
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

// --- Player-layer leaf bridges (Phase D-1) -----------------------------
// Mechanical: mirror each class's Serialize() field set 1:1 with
// snake_case JSON keys.  These are the simple leaves with no nested
// pointer state — the larger leaves (Strengths' per-category history,
// Exclusions' heap arrays, the Happy* family) land in Phase D-2/D-3.

void to_json(nlohmann::json &j, Score const &s)
{
    j = nlohmann::json{
        {"owner",                s.m_owner},
        {"cities_recaptured",    s.m_cities_recaptured},
        {"opponents_conquered",  s.m_opponents_conquered},
        {"final_score",          s.m_finalScore},
        {"victory_type",         s.m_victory_type},
        {"feats",                s.m_feats},
    };
}

void from_json(nlohmann::json const &j, Score &s)
{
    j.at("owner")               .get_to(s.m_owner);
    j.at("cities_recaptured")   .get_to(s.m_cities_recaptured);
    j.at("opponents_conquered") .get_to(s.m_opponents_conquered);
    j.at("final_score")         .get_to(s.m_finalScore);
    j.at("victory_type")        .get_to(s.m_victory_type);
    j.at("feats")               .get_to(s.m_feats);
}

void to_json(nlohmann::json &j, Regard const &r)
{
    nlohmann::json regard = nlohmann::json::array();
    for (sint32 i = 0; i < k_MAX_PLAYERS; ++i)
    {
        regard.push_back(static_cast<sint32>(r.m_regard[i]));
    }
    j = nlohmann::json{{"regard", std::move(regard)}};
}

void from_json(nlohmann::json const &j, Regard &r)
{
    auto const &regard = j.at("regard");
    if (regard.size() != k_MAX_PLAYERS)
    {
        throw nlohmann::json::other_error::create(
            510, "regard.regard must have exactly k_MAX_PLAYERS entries",
            &j);
    }
    for (sint32 i = 0; i < k_MAX_PLAYERS; ++i)
    {
        r.m_regard[i] = static_cast<REGARD_TYPE>(regard[i].get<sint32>());
    }
}

void to_json(nlohmann::json &j, TaxRate const &t)
{
    j = nlohmann::json{
        {"science",                t.m_science},
        {"science_before_anarchy", t.m_science_before_anarchy},
    };
}

void from_json(nlohmann::json const &j, TaxRate &t)
{
    j.at("science")               .get_to(t.m_science);
    j.at("science_before_anarchy").get_to(t.m_science_before_anarchy);
}

void to_json(nlohmann::json &j, Science const &s)
{
    j = nlohmann::json{{"level", s.m_level}};
}

void from_json(nlohmann::json const &j, Science &s)
{
    j.at("level").get_to(s.m_level);
}

void to_json(nlohmann::json &j, MilitaryReadiness const &r)
{
    j = nlohmann::json{
        {"delta",             r.m_delta},
        {"hp_modifier",       r.m_hp_modifier},
        {"cost",              r.m_cost},
        {"percent_last_turn", r.m_percent_last_turn},
        {"readiness_level",   static_cast<sint32>(r.m_readinessLevel)},
        {"ignore_unsupport",  static_cast<bool>(r.m_ignore_unsupport)},
        {"owner",             r.m_owner},
        {"turn_started",      r.m_turnStarted},
        {"cost_gold",         r.m_costGold},
    };
}

void from_json(nlohmann::json const &j, MilitaryReadiness &r)
{
    j.at("delta")            .get_to(r.m_delta);
    j.at("hp_modifier")      .get_to(r.m_hp_modifier);
    j.at("cost")             .get_to(r.m_cost);
    j.at("percent_last_turn").get_to(r.m_percent_last_turn);
    r.m_readinessLevel = static_cast<READINESS_LEVEL>(
        j.at("readiness_level").get<sint32>());
    r.m_ignore_unsupport = j.at("ignore_unsupport").get<bool>() ? TRUE : FALSE;
    j.at("owner")            .get_to(r.m_owner);
    j.at("turn_started")     .get_to(r.m_turnStarted);
    j.at("cost_gold")        .get_to(r.m_costGold);
}

// --- Phase D-2 leaves ---------------------------------------------------

void to_json(nlohmann::json &j, Pollution const &p)
{
    nlohmann::json history = nlohmann::json::array();
    for (sint32 i = 0; i < k_MAX_GLOBAL_POLLUTION_RECORD_TURNS; ++i)
    {
        history.push_back(p.m_history[i]);
    }
    j = nlohmann::json{
        {"event_trigger_next_round", p.m_eventTriggerNextRound},
        {"event_triggered",          p.m_eventTriggered},
        {"trend",                    p.m_trend},
        {"history",                  std::move(history)},
        {"phase",                    p.m_phase},
        {"gw_phase",                 p.m_gwPhase},
        {"next_level",               p.m_next_level},
    };
}

void from_json(nlohmann::json const &j, Pollution &p)
{
    j.at("event_trigger_next_round").get_to(p.m_eventTriggerNextRound);
    j.at("event_triggered")         .get_to(p.m_eventTriggered);
    j.at("trend")                   .get_to(p.m_trend);

    auto const &history = j.at("history");
    if (static_cast<sint32>(history.size())
        != k_MAX_GLOBAL_POLLUTION_RECORD_TURNS)
    {
        throw nlohmann::json::other_error::create(
            520, "pollution.history must have exactly "
                 "k_MAX_GLOBAL_POLLUTION_RECORD_TURNS entries", &j);
    }
    for (sint32 i = 0; i < k_MAX_GLOBAL_POLLUTION_RECORD_TURNS; ++i)
    {
        history[i].get_to(p.m_history[i]);
    }

    j.at("phase")     .get_to(p.m_phase);
    j.at("gw_phase")  .get_to(p.m_gwPhase);
    j.at("next_level").get_to(p.m_next_level);
}

void to_json(nlohmann::json &j, WonderTracker const &wt)
{
    nlohmann::json building = nlohmann::json::array();
    for (sint32 i = 0; i < k_MAX_PLAYERS; ++i)
    {
        building.push_back(wt.m_buildingWonders[i]);
    }
    j = nlohmann::json{
        {"built_wonders",    wt.m_builtWonders},
        {"building_wonders", std::move(building)},
        {"globe_sat_flags",  wt.m_globeSatFlags},
    };
}

void from_json(nlohmann::json const &j, WonderTracker &wt)
{
    j.at("built_wonders").get_to(wt.m_builtWonders);

    auto const &building = j.at("building_wonders");
    if (static_cast<sint32>(building.size()) != k_MAX_PLAYERS)
    {
        throw nlohmann::json::other_error::create(
            521, "wonder_tracker.building_wonders must have exactly "
                 "k_MAX_PLAYERS entries", &j);
    }
    for (sint32 i = 0; i < k_MAX_PLAYERS; ++i)
    {
        building[i].get_to(wt.m_buildingWonders[i]);
    }
    j.at("globe_sat_flags").get_to(wt.m_globeSatFlags);
}

void to_json(nlohmann::json &j, AchievementTracker const &at)
{
    j = nlohmann::json{{"achievements", at.m_achievements}};
}

void from_json(nlohmann::json const &j, AchievementTracker &at)
{
    j.at("achievements").get_to(at.m_achievements);
}

void to_json(nlohmann::json &j, Advances const &a)
{
    // The three uint8/uint16 arrays are sized by m_size — capture
    // m_size in the JSON so the load side can validate.
    nlohmann::json has_advance       = nlohmann::json::array();
    nlohmann::json can_research      = nlohmann::json::array();
    nlohmann::json turns_since_offered = nlohmann::json::array();
    for (sint32 i = 0; i < a.m_size; ++i)
    {
        has_advance        .push_back(a.m_hasAdvance[i]);
        can_research       .push_back(a.m_canResearch[i]);
        turns_since_offered.push_back(a.m_turnsSinceOffered[i]);
    }
    j = nlohmann::json{
        {"owner",                                 a.m_owner},
        {"size",                                  a.m_size},
        {"researching",                           a.m_researching},
        {"age",                                   a.m_age},
        {"last_advance_enabled_this_many_advances",
                                                  a.m_theLastAdvanceEnabledThisManyAdvances},
        {"total_cost",                            a.m_total_cost},
        {"discovered",                            a.m_discovered},
        {"has_advance",                           std::move(has_advance)},
        {"can_research",                          std::move(can_research)},
        {"turns_since_offered",
                                                  std::move(turns_since_offered)},
    };
}

void from_json(nlohmann::json const &j, Advances &a)
{
    j.at("owner")      .get_to(a.m_owner);
    j.at("size")       .get_to(a.m_size);
    j.at("researching").get_to(a.m_researching);
    j.at("age")        .get_to(a.m_age);
    j.at("last_advance_enabled_this_many_advances")
                       .get_to(a.m_theLastAdvanceEnabledThisManyAdvances);
    j.at("total_cost") .get_to(a.m_total_cost);
    j.at("discovered") .get_to(a.m_discovered);

    auto const &has_advance         = j.at("has_advance");
    auto const &can_research        = j.at("can_research");
    auto const &turns_since_offered = j.at("turns_since_offered");
    sint32 const expected = a.m_size;
    if (static_cast<sint32>(has_advance.size())         != expected
     || static_cast<sint32>(can_research.size())        != expected
     || static_cast<sint32>(turns_since_offered.size()) != expected)
    {
        throw nlohmann::json::other_error::create(
            522, "advances arrays must match m_size", &j);
    }

    // Reallocate to match the new size (binary path does the same).
    delete[] a.m_hasAdvance;
    delete[] a.m_canResearch;
    delete[] a.m_turnsSinceOffered;
    a.m_hasAdvance        = new uint8 [expected];
    a.m_canResearch       = new uint8 [expected];
    a.m_turnsSinceOffered = new uint16[expected];

    for (sint32 i = 0; i < expected; ++i)
    {
        has_advance        [i].get_to(a.m_hasAdvance[i]);
        can_research       [i].get_to(a.m_canResearch[i]);
        turns_since_offered[i].get_to(a.m_turnsSinceOffered[i]);
    }
}

void to_json(nlohmann::json &j, HappyTimer const &ht)
{
    j = nlohmann::json{
        {"turns_remaining", ht.m_turnsRemaining},
        {"adjustment",      ht.m_adjustment},
        {"reason",          static_cast<sint32>(ht.m_reason)},
    };
}

void from_json(nlohmann::json const &j, HappyTimer &ht)
{
    j.at("turns_remaining").get_to(ht.m_turnsRemaining);
    j.at("adjustment")     .get_to(ht.m_adjustment);
    ht.m_reason = static_cast<HAPPY_REASON>(j.at("reason").get<sint32>());
}

// --- Phase D-3 leaves ---------------------------------------------------

void to_json(nlohmann::json &j, HappyTracker const &t)
{
    nlohmann::json amounts = nlohmann::json::array();
    for (sint32 i = 0; i < HAPPY_REASON_MAX; ++i)
    {
        amounts.push_back(t.m_happinessAmounts[i]);
    }
    j = nlohmann::json{{"happiness_amounts", std::move(amounts)}};
    // m_tempSaveHappiness is transient (only used between Save/Restore
    // calls within a single turn pipeline) — omitted from JSON, matching
    // the binary Serialize() at HappyTracker.cpp.
}

void from_json(nlohmann::json const &j, HappyTracker &t)
{
    auto const &amounts = j.at("happiness_amounts");
    if (static_cast<sint32>(amounts.size()) != HAPPY_REASON_MAX)
    {
        throw nlohmann::json::other_error::create(
            530, "happy_tracker.happiness_amounts must have exactly "
                 "HAPPY_REASON_MAX entries", &j);
    }
    for (sint32 i = 0; i < HAPPY_REASON_MAX; ++i)
    {
        amounts[i].get_to(t.m_happinessAmounts[i]);
    }
}

void to_json(nlohmann::json &j, Exclusions const &e)
{
    nlohmann::json units     = nlohmann::json::array();
    nlohmann::json buildings = nlohmann::json::array();
    nlohmann::json wonders   = nlohmann::json::array();
    for (sint32 i = 0; i < e.m_numUnits;     ++i) units    .push_back(e.m_units[i]);
    for (sint32 i = 0; i < e.m_numBuildings; ++i) buildings.push_back(e.m_buildings[i]);
    for (sint32 i = 0; i < e.m_numWonders;   ++i) wonders  .push_back(e.m_wonders[i]);

    j = nlohmann::json{
        {"num_units",     e.m_numUnits},
        {"num_buildings", e.m_numBuildings},
        {"num_wonders",   e.m_numWonders},
        {"units",         std::move(units)},
        {"buildings",     std::move(buildings)},
        {"wonders",       std::move(wonders)},
    };
}

void from_json(nlohmann::json const &j, Exclusions &e)
{
    j.at("num_units")    .get_to(e.m_numUnits);
    j.at("num_buildings").get_to(e.m_numBuildings);
    j.at("num_wonders")  .get_to(e.m_numWonders);

    auto const &units     = j.at("units");
    auto const &buildings = j.at("buildings");
    auto const &wonders   = j.at("wonders");
    if (static_cast<sint32>(units.size())     != e.m_numUnits
     || static_cast<sint32>(buildings.size()) != e.m_numBuildings
     || static_cast<sint32>(wonders.size())   != e.m_numWonders)
    {
        throw nlohmann::json::other_error::create(
            531, "exclusions arrays must match their num_* counts", &j);
    }

    delete[] e.m_units;
    delete[] e.m_buildings;
    delete[] e.m_wonders;
    e.m_units     = new sint32[e.m_numUnits];
    e.m_buildings = new sint32[e.m_numBuildings];
    e.m_wonders   = new sint32[e.m_numWonders];

    for (sint32 i = 0; i < e.m_numUnits;     ++i) units    [i].get_to(e.m_units[i]);
    for (sint32 i = 0; i < e.m_numBuildings; ++i) buildings[i].get_to(e.m_buildings[i]);
    for (sint32 i = 0; i < e.m_numWonders;   ++i) wonders  [i].get_to(e.m_wonders[i]);
}

void to_json(nlohmann::json &j, Strengths const &s)
{
    // m_strengthRecords[STRENGTH_CAT_MAX] is a fixed-size array of
    // SimpleDynamicArray<sint32>.  Serialise as a 2D array indexed
    // first by category, then by per-turn record.
    nlohmann::json records = nlohmann::json::array();
    for (sint32 cat = 0; cat < STRENGTH_CAT_MAX; ++cat)
    {
        nlohmann::json per_cat = nlohmann::json::array();
        sint32 const n = s.m_strengthRecords[cat].Num();
        for (sint32 i = 0; i < n; ++i)
        {
            per_cat.push_back(s.m_strengthRecords[cat][i]);
        }
        records.push_back(std::move(per_cat));
    }
    j = nlohmann::json{
        {"owner",            s.m_owner},
        {"strength_records", std::move(records)},
    };
}

void from_json(nlohmann::json const &j, Strengths &s)
{
    j.at("owner").get_to(s.m_owner);

    auto const &records = j.at("strength_records");
    if (static_cast<sint32>(records.size()) != STRENGTH_CAT_MAX)
    {
        throw nlohmann::json::other_error::create(
            532, "strengths.strength_records must have exactly "
                 "STRENGTH_CAT_MAX entries", &j);
    }
    for (sint32 cat = 0; cat < STRENGTH_CAT_MAX; ++cat)
    {
        s.m_strengthRecords[cat].Clear();
        for (auto const &val : records[cat])
        {
            s.m_strengthRecords[cat].Insert(val.get<sint32>());
        }
    }
}

// --- Phase D-4 leaves ---------------------------------------------------

void to_json(nlohmann::json &j, AgreementData const &a)
{
    // GameObj base: only m_id is serialised (per the comment in
    // GameObj.h:50 "Only serialized member").  m_lesser / m_greater
    // are intrusive-pool pointers — pool-level concern, OMITTED.
    j = nlohmann::json{
        {"id",                a.m_id},
        {"owner",             a.m_owner},
        {"recipient",         a.m_recipient},
        {"third_party",       a.m_thirdParty},
        {"agreement",         static_cast<sint32>(a.m_agreement)},
        {"round",             a.m_round},
        {"expires",           a.m_expires},
        {"owner_pollution",   a.m_ownerPollution},
        {"recipient_pollution", a.m_recipientPollution},
        {"is_broken",         static_cast<bool>(a.m_isBroken)},
        {"target_city",       static_cast<ID const &>(a.m_targetCity)},
    };
}

void from_json(nlohmann::json const &j, AgreementData &a)
{
    j.at("id")                .get_to(a.m_id);
    j.at("owner")             .get_to(a.m_owner);
    j.at("recipient")         .get_to(a.m_recipient);
    j.at("third_party")       .get_to(a.m_thirdParty);
    a.m_agreement = static_cast<AGREEMENT_TYPE>(j.at("agreement").get<sint32>());
    j.at("round")             .get_to(a.m_round);
    j.at("expires")           .get_to(a.m_expires);
    j.at("owner_pollution")   .get_to(a.m_ownerPollution);
    j.at("recipient_pollution").get_to(a.m_recipientPollution);
    a.m_isBroken = j.at("is_broken").get<bool>() ? TRUE : FALSE;
    ID target_id(0);
    j.at("target_city")       .get_to(target_id);
    a.m_targetCity = Unit(target_id.m_id);
}

void to_json(nlohmann::json &j, Happy const &h)
{
    // m_timedChanges (std::list<HappyTimer>) -> JSON array of objects
    nlohmann::json timed_changes = nlohmann::json::array();
    for (auto const &timer : h.m_timedChanges)
    {
        timed_changes.push_back(timer);
    }
    j = nlohmann::json{
        {"happiness",            h.m_happiness},
        {"last_captured",        h.m_last_captured},
        {"base",                 h.m_base},
        {"size",                 h.m_size},
        {"pollution",            h.m_pollution},
        {"conquest_distress",    h.m_conquest_distress},
        {"empire_dist",          h.m_empire_dist},
        {"enemy_action",         h.m_enemy_action},
        {"peace",                h.m_peace},
        {"starvation",           h.m_starvation},
        {"workday",              h.m_workday},
        {"wages",                h.m_wages},
        {"rations",              h.m_rations},
        {"martial_law",          h.m_martial_law},
        {"pop_ent",              h.m_pop_ent},
        {"improvement",          h.m_improvement},
        {"wonders",              h.m_wonders},
        {"dist_to_capitol",      h.m_dist_to_capitol},
        {"cost_to_capitol",      h.m_cost_to_capitol},
        {"full_happiness_turns", h.m_fullHappinessTurns},
        {"too_many_cities",      h.m_too_many_cities},
        {"timed",                h.m_timed},
        {"crime",                h.m_crime},
        {"timed_changes",        std::move(timed_changes)},
    };
    // m_pad is alignment padding only — never written to JSON.
    if (h.m_tracker)
    {
        j["tracker"] = *h.m_tracker;
    }
}

void from_json(nlohmann::json const &j, Happy &h)
{
    j.at("happiness")           .get_to(h.m_happiness);
    j.at("last_captured")       .get_to(h.m_last_captured);
    j.at("base")                .get_to(h.m_base);
    j.at("size")                .get_to(h.m_size);
    j.at("pollution")           .get_to(h.m_pollution);
    j.at("conquest_distress")   .get_to(h.m_conquest_distress);
    j.at("empire_dist")         .get_to(h.m_empire_dist);
    j.at("enemy_action")        .get_to(h.m_enemy_action);
    j.at("peace")               .get_to(h.m_peace);
    j.at("starvation")          .get_to(h.m_starvation);
    j.at("workday")             .get_to(h.m_workday);
    j.at("wages")               .get_to(h.m_wages);
    j.at("rations")             .get_to(h.m_rations);
    j.at("martial_law")         .get_to(h.m_martial_law);
    j.at("pop_ent")             .get_to(h.m_pop_ent);
    j.at("improvement")         .get_to(h.m_improvement);
    j.at("wonders")             .get_to(h.m_wonders);
    j.at("dist_to_capitol")     .get_to(h.m_dist_to_capitol);
    j.at("cost_to_capitol")     .get_to(h.m_cost_to_capitol);
    j.at("full_happiness_turns").get_to(h.m_fullHappinessTurns);
    j.at("too_many_cities")     .get_to(h.m_too_many_cities);
    j.at("timed")               .get_to(h.m_timed);
    j.at("crime")               .get_to(h.m_crime);
    h.m_pad = 0;  // alignment field — never carried in JSON

    h.m_timedChanges.clear();
    for (auto const &timer_json : j.at("timed_changes"))
    {
        HappyTimer timer;
        timer_json.get_to(timer);
        h.m_timedChanges.push_back(timer);
    }

    // m_tracker is owned by Happy: delete + reconstruct on load.
    delete h.m_tracker;
    h.m_tracker = nullptr;
    if (j.contains("tracker"))
    {
        h.m_tracker = new HappyTracker();
        j.at("tracker").get_to(*h.m_tracker);
    }
}

// --- Phase D-5 leaves ---------------------------------------------------

void to_json(nlohmann::json &j, CivilisationData const &c)
{
    // m_cityname_count is uint8[500] — store as flat array.
    nlohmann::json cityname_count = nlohmann::json::array();
    for (sint32 i = 0; i < k_MAX_CityName; ++i)
    {
        cityname_count.push_back(c.m_cityname_count[i]);
    }
    // Char buffers: store as plain JSON strings.  The binary path
    // dumps all 512 bytes of each buffer; JSON form is the
    // null-terminated portion, which is what modders actually want
    // to read/edit.
    j = nlohmann::json{
        {"id",                       c.m_id},
        {"owner",                    c.m_owner},
        {"cityname_count",           std::move(cityname_count)},
        {"civ",                      c.m_civ},
        {"gender",                   static_cast<sint32>(c.m_gender)},
        {"city_style",               c.m_cityStyle},
        {"leader_name",              std::string(c.m_leader_name)},
        {"personality_description",  std::string(c.m_personality_description)},
        {"civilisation_name",        std::string(c.m_civilisation_name)},
        {"country_name",             std::string(c.m_country_name)},
        {"singular_name",            std::string(c.m_singular_name)},
    };
}

namespace {
// Copy a std::string into a fixed-size char buffer, zero-filling the
// remainder so we don't carry uninitialised stack/heap bytes (the
// Pre-A SaveExtendedGameInfo lesson).
void load_fixed_string(MBCHAR *dest, std::size_t buf_size,
                       std::string const &src)
{
    std::size_t const n = std::min(src.size(), buf_size - 1);
    std::memcpy(dest, src.data(), n);
    std::memset(dest + n, 0, buf_size - n);
}
}

void from_json(nlohmann::json const &j, CivilisationData &c)
{
    j.at("id")        .get_to(c.m_id);
    j.at("owner")     .get_to(c.m_owner);

    auto const &cityname_count = j.at("cityname_count");
    if (static_cast<sint32>(cityname_count.size()) != k_MAX_CityName)
    {
        throw nlohmann::json::other_error::create(
            540, "civilisation_data.cityname_count must have exactly "
                 "k_MAX_CityName entries", &j);
    }
    for (sint32 i = 0; i < k_MAX_CityName; ++i)
    {
        cityname_count[i].get_to(c.m_cityname_count[i]);
    }

    j.at("civ").get_to(c.m_civ);
    c.m_gender = static_cast<GENDER>(j.at("gender").get<sint32>());
    j.at("city_style").get_to(c.m_cityStyle);

    load_fixed_string(c.m_leader_name,
                      k_MAX_NAME_LEN,
                      j.at("leader_name")            .get<std::string>());
    load_fixed_string(c.m_personality_description,
                      k_MAX_NAME_LEN,
                      j.at("personality_description").get<std::string>());
    load_fixed_string(c.m_civilisation_name,
                      k_MAX_NAME_LEN,
                      j.at("civilisation_name")      .get<std::string>());
    load_fixed_string(c.m_country_name,
                      k_MAX_NAME_LEN,
                      j.at("country_name")           .get<std::string>());
    load_fixed_string(c.m_singular_name,
                      k_MAX_NAME_LEN,
                      j.at("singular_name")          .get<std::string>());
}

void to_json(nlohmann::json &j, TradeOfferData const &t)
{
    j = nlohmann::json{
        {"id",              t.m_id},
        {"owner",           t.m_owner},
        {"from_city",       static_cast<ID const &>(t.m_fromCity)},
        {"offer_type",      static_cast<sint32>(t.m_offerType)},
        {"offer_resource",  t.m_offerResource},
        {"asking_type",     static_cast<sint32>(t.m_askingType)},
        {"asking_resource", t.m_askingResource},
        {"to_city",         static_cast<ID const &>(t.m_toCity)},
    };
}

void from_json(nlohmann::json const &j, TradeOfferData &t)
{
    j.at("id")             .get_to(t.m_id);
    j.at("owner")          .get_to(t.m_owner);
    ID from_id(0);
    j.at("from_city")      .get_to(from_id);
    t.m_fromCity = Unit(from_id.m_id);
    t.m_offerType = static_cast<ROUTE_TYPE>(j.at("offer_type").get<sint32>());
    j.at("offer_resource") .get_to(t.m_offerResource);
    t.m_askingType = static_cast<ROUTE_TYPE>(j.at("asking_type").get<sint32>());
    j.at("asking_resource").get_to(t.m_askingResource);
    ID to_id(0);
    j.at("to_city")        .get_to(to_id);
    t.m_toCity = Unit(to_id.m_id);
}

// Phase D worker batch — BuildNode + BuildQueue
void to_json(nlohmann::json &j, BuildNode const &n)
{
    j = nlohmann::json{
        {"cost",     n.m_cost},
        {"type",     n.m_type},
        {"category", n.m_category},
        {"flags",    n.m_flags},
    };
}

void from_json(nlohmann::json const &j, BuildNode &n)
{
    j.at("cost")    .get_to(n.m_cost);
    j.at("type")    .get_to(n.m_type);
    j.at("category").get_to(n.m_category);
    j.at("flags")   .get_to(n.m_flags);
}

void to_json(nlohmann::json &j, BuildQueue const &q)
{
    nlohmann::json nodes = nlohmann::json::array();
    PointerList<BuildNode>::Walker walk(q.m_list);
    while (walk.IsValid())
    {
        nodes.push_back(*walk.GetObj());
        walk.Next();
    }

    j = nlohmann::json{
        {"owner",           q.m_owner},
        {"city",            static_cast<ID const &>(q.m_city)},
        {"wonder_started",  q.m_wonderStarted},
        {"wonder_stopped",  q.m_wonderStopped},
        {"name",            std::string(q.m_name)},
        {"wonder_complete", q.m_wonderComplete},
        {"nodes",           std::move(nodes)},
    };
}

void from_json(nlohmann::json const &j, BuildQueue &q)
{
    j.at("owner")          .get_to(q.m_owner);
    ID city_id(0);
    j.at("city")           .get_to(city_id);
    q.m_city = Unit(city_id.m_id);
    j.at("wonder_started") .get_to(q.m_wonderStarted);
    j.at("wonder_stopped") .get_to(q.m_wonderStopped);

    // Copy name into the fixed 256-byte buffer with explicit
    // zero-fill of the tail (Pre-A lesson: never carry uninit
    // bytes past the null terminator).
    std::string const name = j.at("name").get<std::string>();
    std::size_t const n    = std::min(name.size(), std::size_t{255});
    std::memcpy(q.m_name, name.data(), n);
    std::memset(q.m_name + n, 0, 256 - n);

    j.at("wonder_complete").get_to(q.m_wonderComplete);

    // Rebuild the PointerList from the JSON array.
    q.m_list->DeleteAll();
    for (auto const &node_json : j.at("nodes"))
    {
        BuildNode *node = new BuildNode;
        node_json.get_to(*node);
        q.m_list->AddTail(node);
    }
}

// Phase D worker batch — Feat + FeatTracker
void to_json(nlohmann::json &j, Feat const &f)
{
    j = nlohmann::json{
        {"type",   f.m_type},
        {"player", f.m_player},
        {"round",  f.m_round},
    };
}

void from_json(nlohmann::json const &j, Feat &f)
{
    j.at("type")  .get_to(f.m_type);
    j.at("player").get_to(f.m_player);
    j.at("round") .get_to(f.m_round);
}

void to_json(nlohmann::json &j, FeatTracker const &ft)
{
    nlohmann::json active = nlohmann::json::array();
    PointerList<Feat>::Walker walk(ft.m_activeList);
    while (walk.IsValid())
    {
        active.push_back(*walk.GetObj());
        walk.Next();
    }

    nlohmann::json achieved      = nlohmann::json::array();
    nlohmann::json building_feat = nlohmann::json::array();
    sint32 const   feat_count    = g_theFeatDB     ? g_theFeatDB->NumRecords()     : 0;
    sint32 const   bldg_count    = g_theBuildingDB ? g_theBuildingDB->NumRecords() : 0;
    for (sint32 i = 0; i < feat_count; ++i) achieved     .push_back(ft.m_achieved[i]);
    for (sint32 i = 0; i < bldg_count; ++i) building_feat.push_back(ft.m_buildingFeat[i]);

    j = nlohmann::json{
        {"active",        std::move(active)},
        {"achieved",      std::move(achieved)},
        {"building_feat", std::move(building_feat)},
    };
}

void from_json(nlohmann::json const &j, FeatTracker &ft)
{
    // Rebuild m_activeList from the JSON array.
    ft.m_activeList->DeleteAll();
    for (auto const &feat_json : j.at("active"))
    {
        Feat *feat = new Feat(0, 0);  // dummy ctor args; overwritten by JSON
        feat_json.get_to(*feat);
        ft.m_activeList->AddTail(feat);
    }

    // Achieved / building_feat: sized by current DB.  If the JSON
    // counts don't match the live DB, log + skip (matches the
    // binary path's tolerance to DB-size drift).
    auto const &achieved      = j.at("achieved");
    auto const &building_feat = j.at("building_feat");
    sint32 const feat_count   = g_theFeatDB     ? g_theFeatDB->NumRecords()     : 0;
    sint32 const bldg_count   = g_theBuildingDB ? g_theBuildingDB->NumRecords() : 0;

    if (static_cast<sint32>(achieved.size()) == feat_count)
    {
        for (sint32 i = 0; i < feat_count; ++i) achieved[i].get_to(ft.m_achieved[i]);
    }
    if (static_cast<sint32>(building_feat.size()) == bldg_count)
    {
        for (sint32 i = 0; i < bldg_count; ++i) building_feat[i].get_to(ft.m_buildingFeat[i]);
    }
}

// Phase D worker batch — GaiaController
void to_json(nlohmann::json &j, GaiaController const &gc)
{
    j = nlohmann::json{
        {"player_id",         gc.m_playerId},
        {"num_mainframes",    gc.m_numMainframes},
        {"num_satellites",    gc.m_numSatellites},
        {"num_wonders_built", gc.m_numWondersBuilt},
        {"num_towers_built",  gc.m_numTowersBuilt},
        {"percent_coverage",  gc.m_percentCoverage},
        {"completed_turn",    gc.m_completedTurn},
    };
}

void from_json(nlohmann::json const &j, GaiaController &gc)
{
    j.at("player_id")        .get_to(gc.m_playerId);
    j.at("num_mainframes")   .get_to(gc.m_numMainframes);
    j.at("num_satellites")   .get_to(gc.m_numSatellites);
    j.at("num_wonders_built").get_to(gc.m_numWondersBuilt);
    j.at("num_towers_built") .get_to(gc.m_numTowersBuilt);
    j.at("percent_coverage") .get_to(gc.m_percentCoverage);
    j.at("completed_turn")   .get_to(gc.m_completedTurn);
}

// Phase D — Diplomat
//
// Mirrors Diplomat::Save in diplomat.cpp.  Per the binary path, only
// a small subset of the class's ~45 fields are persisted.  The
// remainder (m_motivations, m_strategy, m_diplomacy, m_friendCount,
// m_enemyCount, etc.) are derived/recalculated and OMITTED.
//
// OMITS m_foreigners + paired m_diplomaticStates — Foreigner has
// complex RegardEventList per regard-event type; needs its own bridge
// in a follow-up Phase D session.

void to_json(nlohmann::json &j, Diplomat const &d)
{
    nlohmann::json best_strategic_states = nlohmann::json::array();
    for (auto const &state : d.m_bestStrategicStates)
    {
        best_strategic_states.push_back(state);
    }

    nlohmann::json threats = nlohmann::json::array();
    for (auto const &threat : d.m_threats)
    {
        threats.push_back(threat);
    }

    j = nlohmann::json{
        {"player_id",                        d.m_playerId},
        {"personality_name",                 d.m_personalityName},
        {"best_strategic_states",            std::move(best_strategic_states)},
        {"threats",                          std::move(threats)},
        {"diplomacy_victory_complete_turn",  d.m_diplomcyVictoryCompleteTurn},
        {"nuclear_attack_target",            d.m_nuclearAttackTarget},
        {"last_party",                       d.m_lastParty},
        {"launched_nukes",                   d.m_launchedNukes},
        {"launched_nano_attack",             d.m_launchedNanoAttack},
    };
}

void from_json(nlohmann::json const &j, Diplomat &d)
{
    j.at("player_id")                       .get_to(d.m_playerId);
    j.at("personality_name")                .get_to(d.m_personalityName);

    d.m_bestStrategicStates.clear();
    for (auto const &state_json : j.at("best_strategic_states"))
    {
        AiState state;
        state_json.get_to(state);
        d.m_bestStrategicStates.push_back(state);
    }

    d.m_threats.clear();
    for (auto const &threat_json : j.at("threats"))
    {
        Threat threat;
        threat_json.get_to(threat);
        d.m_threats.push_back(threat);
    }

    j.at("diplomacy_victory_complete_turn") .get_to(d.m_diplomcyVictoryCompleteTurn);
    j.at("nuclear_attack_target")           .get_to(d.m_nuclearAttackTarget);
    j.at("last_party")                      .get_to(d.m_lastParty);
    j.at("launched_nukes")                  .get_to(d.m_launchedNukes);
    j.at("launched_nano_attack")            .get_to(d.m_launchedNanoAttack);
}

// Phase E-1 — UnitState (Phase 1 placeholder)
// Currently serialises unit_id + pos.  Matches the binary Serialize
// body (which writes nothing yet).  As fields migrate from UnitActor
// into UnitState in subsequent UnitActor-split phases, both Serialize
// and this bridge extend in lockstep.

void to_json(nlohmann::json &j, UnitState const &s)
{
    j = nlohmann::json{
        {"unit_id", static_cast<ID const &>(s.m_unit_id)},
        {"pos",     s.m_pos},
    };
}

void from_json(nlohmann::json const &j, UnitState &s)
{
    ID id(0);
    j.at("unit_id").get_to(id);
    s.m_unit_id = Unit(id.m_id);
    j.at("pos").get_to(s.m_pos);
}

// Phase E-1 — Order
// Mirrors Order::Serialize at Order.cpp:146.  Persists scalar fields
// + m_point.  m_path (Path *) and m_gameEventArgs (GameEventArgList *)
// are pointer-typed sub-objects with their own Serialize methods —
// they need their own JSON bridges; deferred to Phase E-2 or later.
// to_json sets them to null; from_json leaves the pointers at nullptr.

void to_json(nlohmann::json &j, Order const &o)
{
    j = nlohmann::json{
        {"order",           static_cast<sint32>(o.m_order)},
        {"round",           o.m_round},
        {"point",           o.m_point},
        {"argument",        o.m_argument},
        {"event_type",      static_cast<sint32>(o.m_eventType)},
        {"path",            nullptr},
        {"game_event_args", nullptr},
    };
}

void from_json(nlohmann::json const &j, Order &o)
{
    o.m_order = static_cast<UNIT_ORDER_TYPE>(j.at("order").get<sint32>());
    j.at("round")   .get_to(o.m_round);
    j.at("point")   .get_to(o.m_point);
    j.at("argument").get_to(o.m_argument);
    o.m_eventType = static_cast<GAME_EVENT>(j.at("event_type").get<sint32>());
    o.m_path          = nullptr;
    o.m_gameEventArgs = nullptr;
}

// Phase D-9 — CityData (largest single class composite)
//
// Mirrors CityData::Serialize at CityData.cpp:624.  The big StoreChunk
// block (m_owner..m_is_rioting, ~91 scalar fields) becomes named
// snake_case keys.  Discrete sub-objects compose their existing
// bridges (Unit/BuildQueue/Happy).
//
// OMITTED with reason:
//   - m_tradeSourceList / m_tradeDestinationList (TradeDynamicArray =
//     DynamicArray<TradeRoute>) — TradeRoute needs its own bridge in
//     Phase F (tail pools).
//   - m_collectingResources / m_buyingResources / m_sellingResources
//     (Resources) — needs its own bridge.
//   - m_ringFood/Prod/Gold/Sizes, NEW_RESOURCE_PROCESS fields,
//     m_bonus* coeffs, m_cityRadiusOp, m_killList, m_tempGoodAdder,
//     m_tempGood/Count, m_sentInefficientMessageAlready: transient
//     state, recomputed during gameplay (matches binary).
//   - m_build_category_before_load_queue, m_scie_lost_to_crime,
//     m_gross_food_before_bonuses, m_gross_prod_before_bonuses,
//     m_happinessAttackedBy: not in binary Serialize either.

void to_json(nlohmann::json &j, CityData const &c)
{
    nlohmann::json num_specialists      = nlohmann::json::array();
    nlohmann::json specialist_db_index  = nlohmann::json::array();
    for (sint32 i = 0; i < POP_MAX; ++i)
    {
        num_specialists    .push_back(c.m_numSpecialists[i]);
        specialist_db_index.push_back(c.m_specialistDBIndex[i]);
    }

    nlohmann::json distance_to_good = nlohmann::json::array();
    if (c.m_distanceToGood && g_theResourceDB)
    {
        for (sint32 i = 0; i < g_theResourceDB->NumRecords(); ++i)
        {
            distance_to_good.push_back(c.m_distanceToGood[i]);
        }
    }

    j = nlohmann::json{
        // StoreChunk block (m_owner..m_is_rioting)
        {"owner",                            c.m_owner},
        {"slave_bits",                       c.m_slaveBits},
        {"accumulated_food",                 c.m_accumulated_food},
        {"shieldstore",                      c.m_shieldstore},
        {"shieldstore_at_begin_turn",        c.m_shieldstore_at_begin_turn},
        {"build_category_at_begin_turn",     c.m_build_category_at_begin_turn},
        {"net_gold",                         c.m_net_gold},
        {"gold_lost_to_crime",               c.m_gold_lost_to_crime},
        {"gross_gold",                       c.m_gross_gold},
        {"gold_from_trade_routes",           c.m_goldFromTradeRoutes},
        {"gold_lost_to_piracy",              c.m_goldLostToPiracy},
        {"science",                          c.m_science},
        {"luxury",                           c.m_luxury},
        {"city_attitude",                    static_cast<sint32>(c.m_city_attitude)},
        {"collected_production_this_turn",   c.m_collected_production_this_turn},
        {"gross_production",                 c.m_gross_production},
        {"net_production",                   c.m_net_production},
        {"production_lost_to_crime",         c.m_production_lost_to_crime},
        {"built_improvements",               c.m_built_improvements},
        {"built_wonders",                    c.m_builtWonders},
        {"food_delta",                       c.m_food_delta},
        {"gross_food",                       c.m_gross_food},
        {"net_food",                         c.m_net_food},
        {"food_lost_to_crime",               c.m_food_lost_to_crime},
        {"food_consumed_this_turn",          c.m_food_consumed_this_turn},
        {"total_pollution",                  c.m_total_pollution},
        {"city_population_pollution",        c.m_cityPopulationPollution},
        {"city_industrial_pollution",        c.m_cityIndustrialPollution},
        {"food_vat_pollution",               c.m_foodVatPollution},
        {"city_pollution_cleaner",           c.m_cityPollutionCleaner},
        {"contribute_materials",             static_cast<bool>(c.m_contribute_materials)},
        {"contribute_military",              static_cast<bool>(c.m_contribute_military)},
        {"captured_this_turn",               static_cast<bool>(c.m_capturedThisTurn)},
        {"spied_upon",                       c.m_spied_upon},
        {"walls_nullified",                  static_cast<bool>(c.m_walls_nullified)},
        {"franchise_owner",                  c.m_franchise_owner},
        {"franchise_turns_remaining",        c.m_franchiseTurnsRemaining},
        {"watchful_turns",                   c.m_watchfulTurns},
        {"bio_infection_turns",              c.m_bioInfectionTurns},
        {"bio_infected_by",                  c.m_bioInfectedBy},
        {"nano_infection_turns",             c.m_nanoInfectionTurns},
        {"nano_infected_by",                 c.m_nanoInfectedBy},
        {"converted_to",                     c.m_convertedTo},
        {"converted_gold",                   c.m_convertedGold},
        {"converted_by",                     static_cast<sint32>(c.m_convertedBy)},
        {"terrain_was_polluted",             static_cast<bool>(c.m_terrainWasPolluted)},
        {"happiness_attacked",               static_cast<bool>(c.m_happinessAttacked)},
        {"terrain_improvement_was_built",    static_cast<bool>(c.m_terrainImprovementWasBuilt)},
        {"improvement_was_built",            static_cast<bool>(c.m_improvementWasBuilt)},
        {"is_injoined",                      static_cast<bool>(c.m_isInjoined)},
        {"injoined_by",                      c.m_injoinedBy},
        {"airport_last_used",                c.m_airportLastUsed},
        {"founder",                          c.m_founder},
        {"wages_paid",                       c.m_wages_paid},
        {"pw_from_infrastructure",           c.m_pw_from_infrastructure},
        {"gold_from_capitalization",         c.m_gold_from_capitalization},
        {"build_infrastructure",             static_cast<bool>(c.m_buildInfrastructure)},
        {"build_capitalization",             static_cast<bool>(c.m_buildCapitalization)},
        {"paid_for_buy_front",               static_cast<bool>(c.m_paidForBuyFront)},
        {"do_uprising",                      static_cast<sint32>(c.m_doUprising)},
        {"turn_founded",                     c.m_turnFounded},
        {"production_lost_to_franchise",     c.m_productionLostToFranchise},
        {"probe_recovered_here",             c.m_probeRecoveredHere},
        {"last_celebration_msg",             c.m_lastCelebrationMsg},
        {"already_sold_a_building",          c.m_alreadySoldABuilding},
        {"population",                       c.m_population},
        {"partial_population",               c.m_partialPopulation},
        {"num_specialists",                  std::move(num_specialists)},
        {"specialist_db_index",              std::move(specialist_db_index)},
        {"size_index",                       c.m_sizeIndex},
        {"worker_full_utilization_index",    c.m_workerFullUtilizationIndex},
        {"worker_partial_utilization_index", c.m_workerPartialUtilizationIndex},
        {"use_governor",                     static_cast<bool>(c.m_useGovernor)},
        {"build_list_sequence_index",        c.m_buildListSequenceIndex},
        {"garrison_other_cities",            c.m_garrisonOtherCities},
        {"garrison_complete",                static_cast<bool>(c.m_garrisonComplete)},
        {"current_garrison",                 c.m_currentGarrison},
        {"needed_garrison",                  c.m_neededGarrison},
        {"current_garrison_strength",        c.m_currentGarrisonStrength},
        {"needed_garrison_strength",         c.m_neededGarrisonStrength},
        {"sell_building",                    c.m_sellBuilding},
        {"buy_front",                        c.m_buyFront},
        {"max_food_from_terrain",            c.m_max_food_from_terrain},
        {"max_prod_from_terrain",            c.m_max_prod_from_terrain},
        {"max_gold_from_terrain",            c.m_max_gold_from_terrain},
        {"growth_rate",                      c.m_growth_rate},
        {"overcrowding_coeff",               c.m_overcrowdingCoeff},
        {"starvation_turns",                 c.m_starvation_turns},
        {"city_style",                       c.m_cityStyle},
        {"position",                         c.m_pos},
        {"is_rioting",                       static_cast<bool>(c.m_is_rioting)},
        // Post-StoreChunk persisted fields
        {"min_turns_revolt",                 c.m_min_turns_revolt},
        {"home_city",                        static_cast<ID const &>(c.m_home_city)},
        {"build_queue",                      c.m_build_queue},
        {"happy",                            c.m_happy ? nlohmann::json(*c.m_happy)
                                                       : nlohmann::json(nullptr)},
        {"name",                             std::string(c.m_name)},
        {"distance_to_good",                 std::move(distance_to_good)},
        {"defensive_bonus",                  c.m_defensiveBonus},
    };
}

void from_json(nlohmann::json const &j, CityData &c)
{
    // StoreChunk fields
    j.at("owner")                           .get_to(c.m_owner);
    j.at("slave_bits")                      .get_to(c.m_slaveBits);
    j.at("accumulated_food")                .get_to(c.m_accumulated_food);
    j.at("shieldstore")                     .get_to(c.m_shieldstore);
    j.at("shieldstore_at_begin_turn")       .get_to(c.m_shieldstore_at_begin_turn);
    j.at("build_category_at_begin_turn")    .get_to(c.m_build_category_at_begin_turn);
    j.at("net_gold")                        .get_to(c.m_net_gold);
    j.at("gold_lost_to_crime")              .get_to(c.m_gold_lost_to_crime);
    j.at("gross_gold")                      .get_to(c.m_gross_gold);
    j.at("gold_from_trade_routes")          .get_to(c.m_goldFromTradeRoutes);
    j.at("gold_lost_to_piracy")             .get_to(c.m_goldLostToPiracy);
    j.at("science")                         .get_to(c.m_science);
    j.at("luxury")                          .get_to(c.m_luxury);
    c.m_city_attitude = static_cast<CITY_ATTITUDE>(j.at("city_attitude").get<sint32>());
    j.at("collected_production_this_turn")  .get_to(c.m_collected_production_this_turn);
    j.at("gross_production")                .get_to(c.m_gross_production);
    j.at("net_production")                  .get_to(c.m_net_production);
    j.at("production_lost_to_crime")        .get_to(c.m_production_lost_to_crime);
    j.at("built_improvements")              .get_to(c.m_built_improvements);
    j.at("built_wonders")                   .get_to(c.m_builtWonders);
    j.at("food_delta")                      .get_to(c.m_food_delta);
    j.at("gross_food")                      .get_to(c.m_gross_food);
    j.at("net_food")                        .get_to(c.m_net_food);
    j.at("food_lost_to_crime")              .get_to(c.m_food_lost_to_crime);
    j.at("food_consumed_this_turn")         .get_to(c.m_food_consumed_this_turn);
    j.at("total_pollution")                 .get_to(c.m_total_pollution);
    j.at("city_population_pollution")       .get_to(c.m_cityPopulationPollution);
    j.at("city_industrial_pollution")       .get_to(c.m_cityIndustrialPollution);
    j.at("food_vat_pollution")              .get_to(c.m_foodVatPollution);
    j.at("city_pollution_cleaner")          .get_to(c.m_cityPollutionCleaner);
    c.m_contribute_materials = j.at("contribute_materials").get<bool>() ? TRUE : FALSE;
    c.m_contribute_military  = j.at("contribute_military") .get<bool>() ? TRUE : FALSE;
    c.m_capturedThisTurn     = j.at("captured_this_turn")  .get<bool>() ? TRUE : FALSE;
    j.at("spied_upon")                      .get_to(c.m_spied_upon);
    c.m_walls_nullified = j.at("walls_nullified").get<bool>() ? TRUE : FALSE;
    j.at("franchise_owner")                 .get_to(c.m_franchise_owner);
    j.at("franchise_turns_remaining")       .get_to(c.m_franchiseTurnsRemaining);
    j.at("watchful_turns")                  .get_to(c.m_watchfulTurns);
    j.at("bio_infection_turns")             .get_to(c.m_bioInfectionTurns);
    j.at("bio_infected_by")                 .get_to(c.m_bioInfectedBy);
    j.at("nano_infection_turns")            .get_to(c.m_nanoInfectionTurns);
    j.at("nano_infected_by")                .get_to(c.m_nanoInfectedBy);
    j.at("converted_to")                    .get_to(c.m_convertedTo);
    j.at("converted_gold")                  .get_to(c.m_convertedGold);
    c.m_convertedBy = static_cast<CONVERTED_BY>(j.at("converted_by").get<sint32>());
    c.m_terrainWasPolluted          = j.at("terrain_was_polluted")         .get<bool>() ? TRUE : FALSE;
    c.m_happinessAttacked           = j.at("happiness_attacked")           .get<bool>() ? TRUE : FALSE;
    c.m_terrainImprovementWasBuilt  = j.at("terrain_improvement_was_built").get<bool>() ? TRUE : FALSE;
    c.m_improvementWasBuilt         = j.at("improvement_was_built")        .get<bool>() ? TRUE : FALSE;
    c.m_isInjoined                  = j.at("is_injoined")                  .get<bool>() ? TRUE : FALSE;
    j.at("injoined_by")                     .get_to(c.m_injoinedBy);
    j.at("airport_last_used")               .get_to(c.m_airportLastUsed);
    j.at("founder")                         .get_to(c.m_founder);
    j.at("wages_paid")                      .get_to(c.m_wages_paid);
    j.at("pw_from_infrastructure")          .get_to(c.m_pw_from_infrastructure);
    j.at("gold_from_capitalization")        .get_to(c.m_gold_from_capitalization);
    c.m_buildInfrastructure  = j.at("build_infrastructure") .get<bool>() ? TRUE : FALSE;
    c.m_buildCapitalization  = j.at("build_capitalization") .get<bool>() ? TRUE : FALSE;
    c.m_paidForBuyFront      = j.at("paid_for_buy_front")   .get<bool>() ? TRUE : FALSE;
    c.m_doUprising = static_cast<UPRISING_CAUSE>(j.at("do_uprising").get<sint32>());
    j.at("turn_founded")                    .get_to(c.m_turnFounded);
    j.at("production_lost_to_franchise")    .get_to(c.m_productionLostToFranchise);
    j.at("probe_recovered_here")            .get_to(c.m_probeRecoveredHere);
    j.at("last_celebration_msg")            .get_to(c.m_lastCelebrationMsg);
    j.at("already_sold_a_building")         .get_to(c.m_alreadySoldABuilding);
    j.at("population")                      .get_to(c.m_population);
    j.at("partial_population")              .get_to(c.m_partialPopulation);

    auto const &num_specialists     = j.at("num_specialists");
    auto const &specialist_db_index = j.at("specialist_db_index");
    if (static_cast<sint32>(num_specialists.size()) != POP_MAX
     || static_cast<sint32>(specialist_db_index.size()) != POP_MAX)
    {
        throw nlohmann::json::other_error::create(
            560, "city_data.num_specialists and specialist_db_index must "
                 "have exactly POP_MAX entries", &j);
    }
    for (sint32 i = 0; i < POP_MAX; ++i)
    {
        num_specialists[i]    .get_to(c.m_numSpecialists[i]);
        specialist_db_index[i].get_to(c.m_specialistDBIndex[i]);
    }

    j.at("size_index")                      .get_to(c.m_sizeIndex);
    j.at("worker_full_utilization_index")   .get_to(c.m_workerFullUtilizationIndex);
    j.at("worker_partial_utilization_index").get_to(c.m_workerPartialUtilizationIndex);
    c.m_useGovernor = j.at("use_governor").get<bool>() ? TRUE : FALSE;
    j.at("build_list_sequence_index")       .get_to(c.m_buildListSequenceIndex);
    j.at("garrison_other_cities")           .get_to(c.m_garrisonOtherCities);
    c.m_garrisonComplete = j.at("garrison_complete").get<bool>() ? TRUE : FALSE;
    j.at("current_garrison")                .get_to(c.m_currentGarrison);
    j.at("needed_garrison")                 .get_to(c.m_neededGarrison);
    j.at("current_garrison_strength")       .get_to(c.m_currentGarrisonStrength);
    j.at("needed_garrison_strength")        .get_to(c.m_neededGarrisonStrength);
    j.at("sell_building")                   .get_to(c.m_sellBuilding);
    j.at("buy_front")                       .get_to(c.m_buyFront);
    j.at("max_food_from_terrain")           .get_to(c.m_max_food_from_terrain);
    j.at("max_prod_from_terrain")           .get_to(c.m_max_prod_from_terrain);
    j.at("max_gold_from_terrain")           .get_to(c.m_max_gold_from_terrain);
    j.at("growth_rate")                     .get_to(c.m_growth_rate);
    j.at("overcrowding_coeff")              .get_to(c.m_overcrowdingCoeff);
    j.at("starvation_turns")                .get_to(c.m_starvation_turns);
    j.at("city_style")                      .get_to(c.m_cityStyle);
    j.at("position")                        .get_to(c.m_pos);
    c.m_is_rioting = j.at("is_rioting").get<bool>() ? TRUE : FALSE;

    // Post-StoreChunk persisted fields
    j.at("min_turns_revolt").get_to(c.m_min_turns_revolt);

    ID home_city_id(0);
    j.at("home_city")       .get_to(home_city_id);
    c.m_home_city = Unit(home_city_id.m_id);

    j.at("build_queue")     .get_to(c.m_build_queue);

    if (!j.at("happy").is_null())
    {
        if (!c.m_happy) c.m_happy = new Happy();
        j.at("happy").get_to(*c.m_happy);
    }

    // m_name: fixed k_MAX_NAME_LEN buffer with Pre-A zero-fill discipline.
    {
        std::string const name = j.at("name").get<std::string>();
        std::size_t const n    = std::min(name.size(), std::size_t{k_MAX_NAME_LEN - 1});
        std::memcpy(c.m_name, name.data(), n);
        std::memset(c.m_name + n, 0, k_MAX_NAME_LEN - n);
    }

    // m_distanceToGood: variable-length, sized by current ResourceDB.
    auto const &distance_to_good = j.at("distance_to_good");
    if (g_theResourceDB
        && static_cast<sint32>(distance_to_good.size()) == g_theResourceDB->NumRecords())
    {
        delete[] c.m_distanceToGood;
        c.m_distanceToGood = new sint32[g_theResourceDB->NumRecords()];
        for (sint32 i = 0; i < g_theResourceDB->NumRecords(); ++i)
        {
            distance_to_good[i].get_to(c.m_distanceToGood[i]);
        }
    }

    j.at("defensive_bonus").get_to(c.m_defensiveBonus);
}

// Phase D-9 — Player (final composite)
//
// Mirrors Player::Serialize at Player.cpp:699.  The big StoreChunk
// block (m_owner..m_broken_alliances_and_cease_fires, ~55 scalars
// + 6 arrays) becomes named fields.  m_goodSalePrices is a
// variable-length array sized by ResourceDB.  Composed sub-objects
// use existing bridges where available.
//
// OMITTED with reason (Phase F pool work):
//   - m_all_armies, m_all_cities, m_all_units, m_traderUnits
//     (UnitDynamicArray / DynamicArray<Army>) — pool-level
//     concern; ArmyPool/UnitPool bridges in Phase E/F.
//   - m_gold (Gold), m_difficulty (Difficulty), m_vision (Vision),
//     m_tradeOffers (TradeOfferPool), m_terrainImprovements
//     (TerrainImprovementPool), m_materialPool (MaterialPool),
//     m_messages (MessagePool), m_allRadarInstallations,
//     m_allInstallations (InstallationPool), m_requests, m_agreed —
//     each needs its own bridge in Phase F.
//   - m_capitol (Unit*) — serialised as Unit ID (already bridgeable
//     via ID base).
//
// INCLUDED via existing bridges:
//   - m_science (Science), m_tax_rate (TaxRate), m_advances (Advances),
//     m_global_happiness (Happy), m_readiness (MilitaryReadiness),
//     m_regard (Regard), m_strengths (Strengths).

void to_json(nlohmann::json &j, Player const &p)
{
    // Per-player arrays (k_MAX_PLAYERS = 32 entries each)
    nlohmann::json diplomatic_state       = nlohmann::json::array();
    nlohmann::json patience               = nlohmann::json::array();
    nlohmann::json sent_requests_this_turn = nlohmann::json::array();
    nlohmann::json last_attacked          = nlohmann::json::array();
    for (sint32 i = 0; i < k_MAX_PLAYERS; ++i)
    {
        diplomatic_state       .push_back(static_cast<sint32>(p.m_diplomatic_state[i]));
        patience               .push_back(p.m_patience[i]);
        sent_requests_this_turn.push_back(p.m_sent_requests_this_turn[i]);
        last_attacked          .push_back(p.m_last_attacked[i]);
    }

    // Pollution history arrays
    nlohmann::json pollution_history  = nlohmann::json::array();
    nlohmann::json event_pollution    = nlohmann::json::array();
    for (sint32 i = 0; i < k_MAX_POLLUTION_HISTORY; ++i)
    {
        pollution_history.push_back(p.m_pollution_history[i]);
    }
    for (sint32 i = 0; i < k_MAX_EVENT_POLLUTION_TURNS; ++i)
    {
        event_pollution.push_back(p.m_event_pollution[i]);
    }

    // m_goodSalePrices (variable-length, sized by ResourceDB)
    nlohmann::json good_sale_prices = nlohmann::json::array();
    if (p.m_goodSalePrices && g_theResourceDB)
    {
        for (sint32 i = 0; i < g_theResourceDB->NumRecords(); ++i)
        {
            good_sale_prices.push_back(p.m_goodSalePrices[i]);
        }
    }

    j = nlohmann::json{
        // StoreChunk block
        {"owner",                              p.m_owner},
        {"player_type",                        static_cast<sint32>(p.m_playerType)},
        {"diplomatic_mute",                    p.m_diplomatic_mute},
        {"mask_alliance",                      p.mask_alliance},
        {"mask_hostile",                       p.m_mask_hostile},
        {"diplomatic_state",                   std::move(diplomatic_state)},
        {"government_type",                    p.m_government_type},
        {"trade_transport_points",             p.m_tradeTransportPoints},
        {"used_trade_transport_points",        p.m_usedTradeTransportPoints},
        {"pollution_history",                  std::move(pollution_history)},
        {"event_pollution",                    std::move(event_pollution)},
        {"terrain_pollution",                  static_cast<bool>(p.m_terrainPollution)},
        {"deep_ocean_visible",                 static_cast<bool>(p.m_deepOceanVisible)},
        {"patience",                           std::move(patience)},
        {"sent_requests_this_turn",            std::move(sent_requests_this_turn)},
        {"materials_tax",                      p.m_materialsTax},
        {"home_lost_unit_count",               p.m_home_lost_unit_count},
        {"oversea_lost_unit_count",            p.m_oversea_lost_unit_count},
        {"built_wonders",                      p.m_builtWonders},
        {"wonder_buildings",                   p.m_wonderBuildings},
        {"income_percent",                     p.m_income_Percent},
        {"embassies",                          p.m_embassies},
        {"production_from_franchises",         p.m_productionFromFranchises},
        {"assasination_modifier",              p.m_assasinationModifier},
        {"assasination_timer",                 p.m_assasinationTimer},
        {"is_dead",                            static_cast<bool>(p.m_isDead)},
        {"first_city",                         static_cast<bool>(p.m_first_city)},
        {"total_armies_created",               p.m_totalArmiesCreated},
        {"has_used_city_view",                 static_cast<bool>(p.m_hasUsedCityView)},
        {"has_used_work_view",                 static_cast<bool>(p.m_hasUsedWorkView)},
        {"has_used_production_controls",       static_cast<bool>(p.m_hasUsedProductionControls)},
        {"total_production",                   p.m_total_production},
        {"is_turn_over",                       static_cast<bool>(p.m_is_turn_over)},
        {"end_turn_soon",                      static_cast<bool>(p.m_end_turn_soon)},
        {"power_points",                       p.m_powerPoints},
        {"last_action_cost",                   p.m_lastActionCost},
        {"setup_center",                       p.m_setupCenter},
        {"setup_radius",                       p.m_setupRadius},
        {"done_setting_up",                    static_cast<bool>(p.m_doneSettingUp)},
        {"contacted_players",                  p.m_contactedPlayers},
        {"ending_turn",                        static_cast<bool>(p.m_endingTurn)},
        {"set_government_type",                p.m_set_government_type},
        {"change_government_turn",             p.m_change_government_turn},
        {"changed_government_this_turn",       static_cast<bool>(p.m_changed_government_this_turn)},
        {"pop_science",                        p.m_pop_science},
        {"num_revolted",                       p.m_num_revolted},
        {"can_build_capitalization",           static_cast<bool>(p.m_can_build_capitalization)},
        {"can_build_infrastructure",           static_cast<bool>(p.m_can_build_infrastructure)},
        {"last_attacked",                      std::move(last_attacked)},
        {"can_use_terra_tab",                  static_cast<bool>(p.m_can_use_terra_tab)},
        {"can_use_space_tab",                  static_cast<bool>(p.m_can_use_space_tab)},
        {"can_use_sea_tab",                    static_cast<bool>(p.m_can_use_sea_tab)},
        {"can_use_space_button",               static_cast<bool>(p.m_can_use_space_button)},
        // m_networkGuid: skip (binary writes the bytes but they're
        // either zero or process-dependent — the Pre-A SaveExtendedGameInfo
        // lesson said GUIDs are non-deterministic; carry-through here
        // would defeat Pre-A's same-seed test if it ever gets re-enabled).
        // Recorded as 0 on save; reconstructed on load via the binary
        // path's m_networkGuid memset(0) initialisation.
        {"network_id",                         p.m_networkId},
        {"network_group",                      p.m_networkGroup},
        {"civ_revolting_cities_should_join",   p.m_civRevoltingCitiesShouldJoin},
        {"has_won_the_game",                   static_cast<bool>(p.m_hasWonTheGame)},
        {"has_lost_the_game",                  static_cast<bool>(p.m_hasLostTheGame)},
        {"disable_choose_research",            static_cast<bool>(p.m_disableChooseResearch)},
        {"open_for_network",                   static_cast<bool>(p.m_openForNetwork)},
        {"virtual_gold_spent",                 p.m_virtualGoldSpent},
        {"current_round",                      p.m_current_round},
        {"max_city_count",                     p.m_maxCityCount},
        {"age",                                p.m_age},
        {"research_goal",                      p.m_researchGoal},
        {"broken_alliances_and_cease_fires",   p.m_broken_alliances_and_cease_fires},
        // m_goodSalePrices + composed sub-bridges
        {"good_sale_prices",                   std::move(good_sale_prices)},
        // Composed sub-objects with existing bridges
        {"science",          p.m_science          ? nlohmann::json(*p.m_science)         : nlohmann::json(nullptr)},
        {"tax_rate",         p.m_tax_rate         ? nlohmann::json(*p.m_tax_rate)        : nlohmann::json(nullptr)},
        {"advances",         p.m_advances         ? nlohmann::json(*p.m_advances)        : nlohmann::json(nullptr)},
        // m_global_happiness is PlayerHappiness, not Happy — needs
        // its own bridge in Phase F.  Deferred.
        {"global_happiness", nullptr},
        {"readiness",        p.m_readiness        ? nlohmann::json(*p.m_readiness)       : nlohmann::json(nullptr)},
        {"regard",           p.m_regard           ? nlohmann::json(*p.m_regard)          : nlohmann::json(nullptr)},
        {"strengths",        p.m_strengths        ? nlohmann::json(*p.m_strengths)       : nlohmann::json(nullptr)},
        // m_capitol via ID
        {"capitol",          p.m_capitol          ? nlohmann::json(static_cast<ID const &>(*p.m_capitol)) : nlohmann::json(nullptr)},
    };
}

void from_json(nlohmann::json const &j, Player &p)
{
    // StoreChunk block
    j.at("owner")                          .get_to(p.m_owner);
    p.m_playerType = static_cast<PLAYER_TYPE>(j.at("player_type").get<sint32>());
    j.at("diplomatic_mute")                .get_to(p.m_diplomatic_mute);
    j.at("mask_alliance")                  .get_to(p.mask_alliance);
    j.at("mask_hostile")                   .get_to(p.m_mask_hostile);

    auto const &diplomatic_state = j.at("diplomatic_state");
    if (static_cast<sint32>(diplomatic_state.size()) != k_MAX_PLAYERS)
        throw nlohmann::json::other_error::create(
            570, "player.diplomatic_state needs k_MAX_PLAYERS entries", &j);
    for (sint32 i = 0; i < k_MAX_PLAYERS; ++i)
    {
        p.m_diplomatic_state[i] =
            static_cast<DIPLOMATIC_STATE>(diplomatic_state[i].get<sint32>());
    }

    j.at("government_type")                .get_to(p.m_government_type);
    j.at("trade_transport_points")         .get_to(p.m_tradeTransportPoints);
    j.at("used_trade_transport_points")    .get_to(p.m_usedTradeTransportPoints);

    auto const &pollution_history = j.at("pollution_history");
    if (static_cast<sint32>(pollution_history.size()) != k_MAX_POLLUTION_HISTORY)
        throw nlohmann::json::other_error::create(
            571, "player.pollution_history size mismatch", &j);
    for (sint32 i = 0; i < k_MAX_POLLUTION_HISTORY; ++i)
        pollution_history[i].get_to(p.m_pollution_history[i]);

    auto const &event_pollution = j.at("event_pollution");
    if (static_cast<sint32>(event_pollution.size()) != k_MAX_EVENT_POLLUTION_TURNS)
        throw nlohmann::json::other_error::create(
            572, "player.event_pollution size mismatch", &j);
    for (sint32 i = 0; i < k_MAX_EVENT_POLLUTION_TURNS; ++i)
        event_pollution[i].get_to(p.m_event_pollution[i]);

    p.m_terrainPollution  = j.at("terrain_pollution") .get<bool>() ? TRUE : FALSE;
    p.m_deepOceanVisible  = j.at("deep_ocean_visible").get<bool>() ? TRUE : FALSE;

    auto const &patience = j.at("patience");
    auto const &sent_requests = j.at("sent_requests_this_turn");
    if (static_cast<sint32>(patience.size()) != k_MAX_PLAYERS
     || static_cast<sint32>(sent_requests.size()) != k_MAX_PLAYERS)
    {
        throw nlohmann::json::other_error::create(
            573, "player.patience / sent_requests_this_turn need "
                 "k_MAX_PLAYERS entries", &j);
    }
    for (sint32 i = 0; i < k_MAX_PLAYERS; ++i)
    {
        patience[i]     .get_to(p.m_patience[i]);
        sent_requests[i].get_to(p.m_sent_requests_this_turn[i]);
    }

    j.at("materials_tax")                  .get_to(p.m_materialsTax);
    j.at("home_lost_unit_count")           .get_to(p.m_home_lost_unit_count);
    j.at("oversea_lost_unit_count")        .get_to(p.m_oversea_lost_unit_count);
    j.at("built_wonders")                  .get_to(p.m_builtWonders);
    j.at("wonder_buildings")               .get_to(p.m_wonderBuildings);
    j.at("income_percent")                 .get_to(p.m_income_Percent);
    j.at("embassies")                      .get_to(p.m_embassies);
    j.at("production_from_franchises")     .get_to(p.m_productionFromFranchises);
    j.at("assasination_modifier")          .get_to(p.m_assasinationModifier);
    j.at("assasination_timer")             .get_to(p.m_assasinationTimer);
    p.m_isDead                              = j.at("is_dead")    .get<bool>() ? TRUE : FALSE;
    p.m_first_city                          = j.at("first_city") .get<bool>() ? TRUE : FALSE;
    j.at("total_armies_created")           .get_to(p.m_totalArmiesCreated);
    p.m_hasUsedCityView          = j.at("has_used_city_view")          .get<bool>() ? TRUE : FALSE;
    p.m_hasUsedWorkView          = j.at("has_used_work_view")          .get<bool>() ? TRUE : FALSE;
    p.m_hasUsedProductionControls= j.at("has_used_production_controls").get<bool>() ? TRUE : FALSE;
    j.at("total_production")               .get_to(p.m_total_production);
    p.m_is_turn_over   = j.at("is_turn_over")  .get<bool>() ? TRUE : FALSE;
    p.m_end_turn_soon  = j.at("end_turn_soon") .get<bool>() ? TRUE : FALSE;
    j.at("power_points")                   .get_to(p.m_powerPoints);
    j.at("last_action_cost")               .get_to(p.m_lastActionCost);
    j.at("setup_center")                   .get_to(p.m_setupCenter);
    j.at("setup_radius")                   .get_to(p.m_setupRadius);
    p.m_doneSettingUp = j.at("done_setting_up").get<bool>() ? TRUE : FALSE;
    j.at("contacted_players")              .get_to(p.m_contactedPlayers);
    p.m_endingTurn = j.at("ending_turn").get<bool>() ? TRUE : FALSE;
    j.at("set_government_type")            .get_to(p.m_set_government_type);
    j.at("change_government_turn")         .get_to(p.m_change_government_turn);
    p.m_changed_government_this_turn = j.at("changed_government_this_turn").get<bool>() ? TRUE : FALSE;
    j.at("pop_science")                    .get_to(p.m_pop_science);
    j.at("num_revolted")                   .get_to(p.m_num_revolted);
    p.m_can_build_capitalization  = j.at("can_build_capitalization") .get<bool>() ? TRUE : FALSE;
    p.m_can_build_infrastructure  = j.at("can_build_infrastructure") .get<bool>() ? TRUE : FALSE;

    auto const &last_attacked = j.at("last_attacked");
    if (static_cast<sint32>(last_attacked.size()) != k_MAX_PLAYERS)
        throw nlohmann::json::other_error::create(
            574, "player.last_attacked size mismatch", &j);
    for (sint32 i = 0; i < k_MAX_PLAYERS; ++i)
        last_attacked[i].get_to(p.m_last_attacked[i]);

    p.m_can_use_terra_tab     = j.at("can_use_terra_tab")    .get<bool>() ? TRUE : FALSE;
    p.m_can_use_space_tab     = j.at("can_use_space_tab")    .get<bool>() ? TRUE : FALSE;
    p.m_can_use_sea_tab       = j.at("can_use_sea_tab")      .get<bool>() ? TRUE : FALSE;
    p.m_can_use_space_button  = j.at("can_use_space_button") .get<bool>() ? TRUE : FALSE;
    // m_networkGuid: leave at zero (see to_json comment).
    memset(&p.m_networkGuid, 0, sizeof(p.m_networkGuid));
    j.at("network_id")                     .get_to(p.m_networkId);
    j.at("network_group")                  .get_to(p.m_networkGroup);
    j.at("civ_revolting_cities_should_join").get_to(p.m_civRevoltingCitiesShouldJoin);
    p.m_hasWonTheGame         = j.at("has_won_the_game")         .get<bool>() ? TRUE : FALSE;
    p.m_hasLostTheGame        = j.at("has_lost_the_game")        .get<bool>() ? TRUE : FALSE;
    p.m_disableChooseResearch = j.at("disable_choose_research")  .get<bool>() ? TRUE : FALSE;
    p.m_openForNetwork        = j.at("open_for_network")         .get<bool>() ? TRUE : FALSE;
    j.at("virtual_gold_spent")             .get_to(p.m_virtualGoldSpent);
    j.at("current_round")                  .get_to(p.m_current_round);
    j.at("max_city_count")                 .get_to(p.m_maxCityCount);
    j.at("age")                            .get_to(p.m_age);
    j.at("research_goal")                  .get_to(p.m_researchGoal);
    j.at("broken_alliances_and_cease_fires").get_to(p.m_broken_alliances_and_cease_fires);

    // m_goodSalePrices: variable-length, sized by ResourceDB
    auto const &good_sale_prices = j.at("good_sale_prices");
    if (p.m_goodSalePrices && g_theResourceDB
        && static_cast<sint32>(good_sale_prices.size()) == g_theResourceDB->NumRecords())
    {
        for (sint32 i = 0; i < g_theResourceDB->NumRecords(); ++i)
            good_sale_prices[i].get_to(p.m_goodSalePrices[i]);
    }

    // Composed sub-objects — null in JSON skips the field on load
    if (!j.at("science")          .is_null() && p.m_science)          j.at("science")         .get_to(*p.m_science);
    if (!j.at("tax_rate")         .is_null() && p.m_tax_rate)         j.at("tax_rate")        .get_to(*p.m_tax_rate);
    if (!j.at("advances")         .is_null() && p.m_advances)         j.at("advances")        .get_to(*p.m_advances);
    // m_global_happiness deferred (see to_json comment)
    if (!j.at("readiness")        .is_null() && p.m_readiness)        j.at("readiness")       .get_to(*p.m_readiness);
    if (!j.at("regard")           .is_null() && p.m_regard)           j.at("regard")          .get_to(*p.m_regard);
    if (!j.at("strengths")        .is_null() && p.m_strengths)        j.at("strengths")       .get_to(*p.m_strengths);

    // m_capitol (Unit*) — null in JSON skips
    if (!j.at("capitol").is_null() && p.m_capitol)
    {
        ID capitol_id(0);
        j.at("capitol").get_to(capitol_id);
        *p.m_capitol = Unit(capitol_id.m_id);
    }
}

// Phase D — Foreigner
void to_json(nlohmann::json &j, Foreigner const &f)
{
    // 2D array of regard events: outer indexed by REGARD_EVENT_TYPE
    // (excluding REGARD_EVENT_ALL — that's a derived total), inner is
    // the list of events for that type.
    nlohmann::json regard_event_list = nlohmann::json::array();
    for (sint32 type = 0; type < REGARD_EVENT_ALL; ++type)
    {
        nlohmann::json events = nlohmann::json::array();
        for (auto const &ev : f.m_regardEventList[type])
        {
            events.push_back(ev);
        }
        regard_event_list.push_back(std::move(events));
    }

    j = nlohmann::json{
        {"trustworthiness",        f.m_trustworthiness},
        {"has_initiative",         f.m_hasInitiative},
        {"last_incursion",         f.m_lastIncursion},
        {"regard_event_list",      std::move(regard_event_list)},
        {"hotwar_attacked_me",     f.m_hotwarAttackedMe},
        {"coldwar_attacked_me",    f.m_coldwarAttackedMe},
        {"greeting_turn",          f.m_greetingTurn},
        {"embargo",                f.m_embargo},
    };
}

void from_json(nlohmann::json const &j, Foreigner &f)
{
    j.at("trustworthiness")    .get_to(f.m_trustworthiness);
    j.at("has_initiative")     .get_to(f.m_hasInitiative);
    j.at("last_incursion")     .get_to(f.m_lastIncursion);

    auto const &regard_event_list = j.at("regard_event_list");
    if (static_cast<sint32>(regard_event_list.size()) != REGARD_EVENT_ALL)
    {
        throw nlohmann::json::other_error::create(
            550, "foreigner.regard_event_list must have exactly "
                 "REGARD_EVENT_ALL entries", &j);
    }
    for (sint32 type = 0; type < REGARD_EVENT_ALL; ++type)
    {
        f.m_regardEventList[type].clear();
        for (auto const &ev_json : regard_event_list[type])
        {
            RegardEvent ev;
            ev_json.get_to(ev);
            f.m_regardEventList[type].push_back(ev);
        }
    }

    j.at("hotwar_attacked_me")  .get_to(f.m_hotwarAttackedMe);
    j.at("coldwar_attacked_me") .get_to(f.m_coldwarAttackedMe);
    j.at("greeting_turn")       .get_to(f.m_greetingTurn);
    j.at("embargo")             .get_to(f.m_embargo);
}

// Phase D — AgreementMatrix
void to_json(nlohmann::json &j, AgreementMatrix const &am)
{
    nlohmann::json agreements = nlohmann::json::array();
    for (auto const &agr : am.m_agreements)
    {
        agreements.push_back(agr);
    }
    j = nlohmann::json{
        {"max_players", am.m_maxPlayers},
        {"agreements", std::move(agreements)},
    };
}

void from_json(nlohmann::json const &j, AgreementMatrix &am)
{
    j.at("max_players").get_to(am.m_maxPlayers);
    auto const &agreements = j.at("agreements");
    am.m_agreements.resize(agreements.size());
    for (std::size_t i = 0; i < agreements.size(); ++i)
    {
        agreements[i].get_to(am.m_agreements[i]);
    }
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
