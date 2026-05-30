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
