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
#include "gs/gameobj/Gold.h"
#include "gs/gameobj/Diffcly.h"
#include "gs/gameobj/MaterialPool.h"
#include "gs/gameobj/DiplomaticRequestData.h"
#include "gs/gameobj/DiplomaticRequestPool.h"
#include "gs/gameobj/AgreementPool.h"
#include "gs/gameobj/TradeOfferPool.h"
#include "gs/gameobj/Vision.h"
#include "gs/world/UnseenCellQuadTree.h"
#include "gs/world/UnseenCell.h"
#include "gs/gameobj/Agreement.h"
#include "gs/gameobj/DiplomaticRequest.h"
#include "gs/gameobj/installation.h"
#include "gs/gameobj/TerrImprove.h"
#include "gs/gameobj/message.h"
#include "gs/gameobj/Sci.h"               // Science
#include "gs/gameobj/Readiness.h"         // MilitaryReadiness
#include "gs/gameobj/pollution.h"
#include "gs/gameobj/PollutionConst.h"    // k_MAX_GLOBAL_POLLUTION_RECORD_TURNS
#include "gs/gameobj/WonderTracker.h"
#include "gs/gameobj/AchievementTracker.h"
#include "gs/fileio/action_log.h"          // action_log carrier round-trip
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
#include "ai/ctpai.h"                  // CtpAi::Resize (LoadJson tail)
#include "ai/diplomacy/AgreementMatrix.h"
#include "ai/diplomacy/Diplomat.h"
#include "ai/diplomacy/Foreigner.h"
#include "gs/gameobj/citydata.h"
#include "gs/gameobj/UnitTypes.h"           // POP_MAX
#include "gs/gameobj/player.h"
#include "gs/gameobj/PollutionConst.h"      // already included via pollution.h, kept explicit
#include "gs/gameobj/UnitState.h"
#include "gs/gameobj/Order.h"
#include "gs/gameobj/ArmyData.h"
#include "gs/gameobj/ArmyPool.h"
#include "gs/gameobj/UnitData.h"
#include "gs/gameobj/UnitPool.h"
#include "gs/gameobj/CivilisationPool.h"
#include "gs/gameobj/TopTen.h"
#include "gs/gameobj/EndGame.h"
#include "gs/gameobj/installationpool.h"
#include "gs/gameobj/installationdata.h"
#include "gs/gameobj/TradePool.h"
#include "gs/gameobj/TradeRouteData.h"
#include "gs/utility/TradeDynArr.h"
#include "gs/gameobj/TerrImprovePool.h"
#include "gs/gameobj/TerrImproveData.h"
#include "gs/slic/SlicConst.h"
#include "gs/slic/SlicRecord.h"
#include "gs/slic/SlicEngine.h"       // slicengine_Get() for segment lookup
#include "gs/slic/SlicSegment.h"
#include "gs/slic/SlicSymbol.h"
#include "gs/slic/SlicNamedSymbol.h"
#include "gs/slic/SlicArray.h"
#include "gs/slic/SlicStruct.h"   // SlicStructDescription::GetType()
#include "gs/slic/SlicSymTab.h"
#include "gs/slic/SlicContext.h"
#include "gs/slic/SlicObject.h"
#include "gs/slic/SlicFrame.h"
#include "gs/slic/SlicFunc.h"
#include "gs/slic/SlicEngine.h"
#include "gs/slic/SlicButton.h"
#include "gs/slic/SlicEyePoint.h"
#include "gs/gameobj/MessageData.h"
#include "gs/gameobj/MessagePool.h"
#include "gs/gameobj/EventTracker.h"
#include "gs/gameobj/installationtree.h"     // installation_tree_Get() (G-4)
#include "gs/utility/QuadTree.h"              // unit_tree_Get() (G-4)
#include "ctp/ctp2_utils/pointerlist.h"
#include "gs/events/GameEventManager.h"   // gevmanager_Get() (for SlicSegment hook)
#include "gs/utility/SimpleDynArr.h"
#include "gs/gameobj/Unit.h"
#include "gs/gameobj/Army.h"
#include "gs/gameobj/TradeOffer.h"
#include "robot/pathing/Path.h"
#include "gs/database/EndGameDB.h"          // endgamedb_Get()->m_nRec
#include "gs/utility/SimpleDynArr.h"
#include "gs/core/game_observer.h"          // NotifyUnitSpawned
#include "gs/world/cellunitlist.h"          // CellUnitList (ArmyData base)
#include "gs/utility/UnitDynArr.h"          // UnitDynamicArray
#include "ctp/ctp2_utils/BitMask.h"        // BitMask (m_roundTheWorldMask)
#include "CivilisationRecord.h"            // k_MAX_CityName
#include "gs/core/player_view.h"          // player_view::CurPlayer

#include <chrono>
#include <cstring>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>


#include "gs/gameobj/GameSettings.h"   // gamesettings_Get()
extern PointerList<Player>   *g_deadPlayer;
// rand_ptr() declared in RandGen.h.  world_Get() in World.h.
// g_theArmyPool / tradepool_Get() / slicengine_Get() /
// terrimprovepool_Get() / civilisationpool_Get() / messagepool_Get() /
// installationpool_Get() / wonder_tracker_Get() / exclusions_Get() / feattracker_Get() /
// are extern'd by their respective headers (already included above).

// CTP2_BUILD_SHA is injected by meson into config.h (run_command git
// rev-parse --short).  Fall back to "unknown" if config.h hasn't been
// regenerated.
#ifndef CTP2_BUILD_SHA
#define CTP2_BUILD_SHA "unknown"
#endif

// --- save-file string codec (public: unit-tested) ----------------------

// Sanitise a NUL-terminated C string into a UTF-8-clean std::string.
// CTP2's leader/civ/country names are stored as fixed char[] buffers
// originally populated from ISO-8859-1 or Windows-1252 sources (CTP2
// shipped before UTF-8 was the default).  nlohmann::json::dump() throws
// type_error 316 on invalid UTF-8, which historically crashed autosaves
// for games whose civs have names containing extended characters (e.g.
// "Czechosłowacja" — 0xB3 in slot 6).
//
// Strategy: bytes ≤ 0x7F are ASCII (UTF-8 by definition).  For any
// byte ≥ 0x80, expand to a two-byte UTF-8 sequence treating the byte
// as a U+00xx Latin-1 code point.  Lossless for the common case where
// game data is Latin-1 encoded; never produces invalid UTF-8 even for
// arbitrary binary garbage.
std::string utf8_safe(MBCHAR const *src)
{
    if (!src) return {};
    std::string out;
    while (*src)
    {
        unsigned char const b = static_cast<unsigned char>(*src++);
        if (b < 0x80) {
            out.push_back(static_cast<char>(b));
        } else {
            // U+0080..U+00FF → two-byte UTF-8: 110xxxxx 10xxxxxx
            out.push_back(static_cast<char>(0xC0 | (b >> 6)));
            out.push_back(static_cast<char>(0x80 | (b & 0x3F)));
        }
    }
    return out;
}

// Inverse of utf8_safe for the LOAD side.  Game strings live in memory
// as Latin-1 (StringDB, fixed char[] name buffers, the UI font path);
// saves store them as UTF-8.  Without this, a loaded game carried UTF-8
// bytes into Latin-1 contexts: the UI rendered mojibake and every
// re-save double-encoded.  Two-byte sequences for U+0080..U+00FF decode
// to the original byte; anything outside Latin-1 (or invalid UTF-8)
// becomes '?' rather than garbage.
std::string latin1_safe(std::string const &utf8)
{
    std::string out;
    out.reserve(utf8.size());
    for (std::size_t i = 0; i < utf8.size(); )
    {
        unsigned char const b = static_cast<unsigned char>(utf8[i]);
        if (b < 0x80) {
            out.push_back(static_cast<char>(b));
            ++i;
        } else if ((b == 0xC2 || b == 0xC3) && i + 1 < utf8.size() &&
                   (static_cast<unsigned char>(utf8[i + 1]) & 0xC0) == 0x80) {
            out.push_back(static_cast<char>(((b & 0x03) << 6) |
                          (static_cast<unsigned char>(utf8[i + 1]) & 0x3F)));
            i += 2;
        } else {
            // Outside Latin-1 (or invalid): skip the whole sequence.
            int extra = (b >= 0xF0) ? 3 : (b >= 0xE0) ? 2 : (b >= 0xC2) ? 1 : 0;
            out.push_back('?');
            i += 1 + (std::size_t)extra;
        }
    }
    return out;
}

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
    for (int i : rng.m_buffer)
    {
        buffer.push_back(i);
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
    // Restore m_city RAW — m_env (restored above, serialised in full) is
    // the authority on the city bits.  SetCity here was wrong twice over:
    // it re-derived env bits that were already correct (breaking exact
    // round-trips), and it promoted city-RADIUS cells (m_city = owning
    // city with only k_BIT_ENV_CITY_RADIUS set) to full CITY tiles on
    // every load (SetCity sets k_BIT_ENV_CITY for any non-zero id).
    c.m_city = Unit(city_id.m_id);
    j.at("cell_owner")      .get_to(c.m_cellOwner);
}

void to_json(nlohmann::json &j, TileInfo const &t)
{
    nlohmann::json transitions = nlohmann::json::array();
    for (unsigned char m_transition : t.m_transitions)
    {
        transitions.push_back(m_transition);
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

    // good_value: per-resource weighting derived from the map's good
    // distribution.  When the saved count matches the current resource
    // DB, restore it verbatim.  When it doesn't (a mod changed
    // ResourceDB between save and load) — or the save carried no
    // good_value at all — recompute from the freshly-loaded cells via
    // ComputeGoodsValues(), exactly as the map-generation and binary
    // load paths do (wldgen.cpp:824).  Leaving m_goodValue at whatever
    // the throwaway fresh-game gameinit computed was a wrong-but-not-
    // crashing state after load (the P5 derived-cache gap): the value
    // table would reflect the discarded initial map, not the loaded
    // one.  Cells are already restored above, so ComputeGoodsValues has
    // the real good distribution to work from.
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
    else if (g_theResourceDB)
    {
        // Size mismatch or empty saved table: rebuild from the loaded map.
        w.ComputeGoodsValues();
    }
    // (If g_theResourceDB is unavailable there is nothing to compute
    //  against; m_goodValue is left as-is.)

    // Continent-size arrays (m_land_size / m_water_size) are zeroed by
    // AllocateMap.  Without re-populating them, GetLandContinentSize()
    // reads off the end of an empty DynamicArray — caught by ASan as a
    // heap-buffer-overflow in MapAnalysis::BeginTurn after a fresh load
    // (the saved cells have valid continent numbers but the per-
    // continent size cache is empty).  FindContinentSize walks cells +
    // accumulates sizes from existing m_continent_number values; no
    // renumbering, so this is a pure cache rebuild.
    w.FindContinentSize();

    // Same family, second cache: the continent NEIGHBOR arrays
    // (m_land_next_too_water / m_water_next_too_land) are allocated
    // empty by AllocateMap and only filled by FindContinentNeighbors.
    // The first AI transport-goal evaluation after a load walks them
    // (Agent::EstimateTransportUtility -> LandShareWater ->
    // IsLandNextTooWater) and dereferenced the empty array's NULL
    // storage — SIGSEGV caught by the crash reporter while building the
    // capture fixture.  Pure cache rebuild from per-cell continent
    // numbers, exactly like FindContinentSize above (the map-import
    // path at wldgen.cpp does the full NumberContinents for the same
    // reason).
    w.FindContinentNeighbors();
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
    for (auto i : r.m_regard)
    {
        regard.push_back(static_cast<sint32>(i));
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

void to_json(nlohmann::json &j, Gold const &g)
{
    j = nlohmann::json{
        {"level",                  g.m_level},
        {"income_this_turn",       g.m_income_this_turn},
        {"gross_income",           g.m_gross_income},
        {"lost_to_cleric",         g.m_lost_to_cleric},
        {"lost_to_crime",          g.m_lost_to_crime},
        {"lost_to_rushbuy",        g.m_lost_to_rushbuy},
        {"wages_this_turn",        g.m_wages_this_turn},
        {"maintenance_this_turn",  g.m_maintenance_this_turn},
        {"science_this_turn",      g.m_science_this_turn},
        {"level_last_turn",        g.m_level_last_turn},
        {"delta_last_turn",        g.m_delta_last_turn},
        {"consider_for_science",   g.m_consider_for_science},
        {"owner",                  g.m_owner},
    };
}

void from_json(nlohmann::json const &j, Gold &g)
{
    j.at("level")                 .get_to(g.m_level);
    j.at("income_this_turn")      .get_to(g.m_income_this_turn);
    j.at("gross_income")          .get_to(g.m_gross_income);
    j.at("lost_to_cleric")        .get_to(g.m_lost_to_cleric);
    j.at("lost_to_crime")         .get_to(g.m_lost_to_crime);
    j.at("lost_to_rushbuy")       .get_to(g.m_lost_to_rushbuy);
    j.at("wages_this_turn")       .get_to(g.m_wages_this_turn);
    j.at("maintenance_this_turn") .get_to(g.m_maintenance_this_turn);
    j.at("science_this_turn")     .get_to(g.m_science_this_turn);
    j.at("level_last_turn")       .get_to(g.m_level_last_turn);
    j.at("delta_last_turn")       .get_to(g.m_delta_last_turn);
    j.at("consider_for_science")  .get_to(g.m_consider_for_science);
    j.at("owner")                 .get_to(g.m_owner);
}

void to_json(nlohmann::json &j, MaterialPool const &m)
{
    j = nlohmann::json{
        {"level", m.m_level},
        {"owner", m.m_owner},
        {"cap",   m.m_cap},
    };
}

void from_json(nlohmann::json const &j, MaterialPool &m)
{
    j.at("level").get_to(m.m_level);
    j.at("owner").get_to(m.m_owner);
    j.at("cap")  .get_to(m.m_cap);
}

void to_json(nlohmann::json &j, Difficulty const &d)
{
    j = nlohmann::json{
        {"big_city_scale",                  d.m_big_city_scale},
        {"big_city_offset",                 d.m_big_city_offset},
        {"pollution_multiplier",            d.m_pollution_multiplier},
        {"riot_chance",                     d.m_riot_chance},
        {"feats_factor",                    d.m_feats_factor},
        {"advances_factor",                 d.m_advances_factor},
        {"wonders_factor",                  d.m_wonders_factor},
        {"population_factor",               d.m_population_factor},
        {"rank_factor",                     d.m_rank_factor},
        {"allies_factor",                   d.m_allies_factor},
        {"opponents_conquered_factor",      d.m_opponents_conquered_factor},
        {"cities0to30_factor",              d.m_cities0to30_factor},
        {"cities30to100_factor",            d.m_cities30to100_factor},
        {"cities100to500_factor",           d.m_cities100to500_factor},
        {"cities500plus_factor",            d.m_cities500plus_factor},
        {"cities_recaptured_factor",        d.m_cities_recaptured_factor},
        {"allied_victory_bonus",            d.m_allied_victory_bonus},
        {"solo_victory_bonus",              d.m_solo_victory_bonus},
        {"wonder_victory_bonus",            d.m_wonder_victory_bonus},
        {"distance_from_capitol_adjustment", d.m_distance_from_capitol_adjustment},
        {"starvation_effect",               d.m_starvation_effect},
        {"owner",                           d.m_owner},
        {"base_contentment",                d.m_base_contentment},
        {"max_martial_law_units",           d.m_max_martial_law_units},
        {"martial_law_effect",              d.m_martial_law_effect},
        {"content_in_the_field",            d.m_content_in_the_field},
        {"in_the_field_effect",             d.m_in_the_field_effect},
        {"science_handicap",                d.m_science_handicap},
        {"starting_gold",                   d.m_starting_gold},
        {"base_score",                      d.m_base_score},
        {"vision_bonus",                    d.m_vision_bonus},
        {"pad",                             d.m_pad},
    };
}

void from_json(nlohmann::json const &j, Difficulty &d)
{
    j.at("big_city_scale")                  .get_to(d.m_big_city_scale);
    j.at("big_city_offset")                 .get_to(d.m_big_city_offset);
    j.at("pollution_multiplier")            .get_to(d.m_pollution_multiplier);
    j.at("riot_chance")                     .get_to(d.m_riot_chance);
    j.at("feats_factor")                    .get_to(d.m_feats_factor);
    j.at("advances_factor")                 .get_to(d.m_advances_factor);
    j.at("wonders_factor")                  .get_to(d.m_wonders_factor);
    j.at("population_factor")               .get_to(d.m_population_factor);
    j.at("rank_factor")                     .get_to(d.m_rank_factor);
    j.at("allies_factor")                   .get_to(d.m_allies_factor);
    j.at("opponents_conquered_factor")      .get_to(d.m_opponents_conquered_factor);
    j.at("cities0to30_factor")              .get_to(d.m_cities0to30_factor);
    j.at("cities30to100_factor")            .get_to(d.m_cities30to100_factor);
    j.at("cities100to500_factor")           .get_to(d.m_cities100to500_factor);
    j.at("cities500plus_factor")            .get_to(d.m_cities500plus_factor);
    j.at("cities_recaptured_factor")        .get_to(d.m_cities_recaptured_factor);
    j.at("allied_victory_bonus")            .get_to(d.m_allied_victory_bonus);
    j.at("solo_victory_bonus")              .get_to(d.m_solo_victory_bonus);
    j.at("wonder_victory_bonus")            .get_to(d.m_wonder_victory_bonus);
    j.at("distance_from_capitol_adjustment").get_to(d.m_distance_from_capitol_adjustment);
    j.at("starvation_effect")               .get_to(d.m_starvation_effect);
    j.at("owner")                           .get_to(d.m_owner);
    j.at("base_contentment")                .get_to(d.m_base_contentment);
    j.at("max_martial_law_units")           .get_to(d.m_max_martial_law_units);
    j.at("martial_law_effect")              .get_to(d.m_martial_law_effect);
    j.at("content_in_the_field")            .get_to(d.m_content_in_the_field);
    j.at("in_the_field_effect")             .get_to(d.m_in_the_field_effect);
    j.at("science_handicap")                .get_to(d.m_science_handicap);
    j.at("starting_gold")                   .get_to(d.m_starting_gold);
    j.at("base_score")                      .get_to(d.m_base_score);
    j.at("vision_bonus")                    .get_to(d.m_vision_bonus);
    j.at("pad")                             .get_to(d.m_pad);
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
    for (int i : p.m_history)
    {
        history.push_back(i);
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
    for (unsigned long long m_buildingWonder : wt.m_buildingWonders)
    {
        building.push_back(m_buildingWonder);
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

    // Resize vectors to match the new size (binary path does the same).
    a.m_hasAdvance.resize(expected);
    a.m_canResearch.resize(expected);
    a.m_turnsSinceOffered.resize(expected);

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
    for (double m_happinessAmount : t.m_happinessAmounts)
    {
        amounts.push_back(m_happinessAmount);
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

    e.m_units     = std::make_unique<sint32[]>(e.m_numUnits);
    e.m_buildings = std::make_unique<sint32[]>(e.m_numBuildings);
    e.m_wonders   = std::make_unique<sint32[]>(e.m_numWonders);

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
    for (const auto & m_strengthRecord : s.m_strengthRecords)
    {
        nlohmann::json per_cat = nlohmann::json::array();
        sint32 const n = m_strengthRecord.Num();
        for (sint32 i = 0; i < n; ++i)
        {
            per_cat.push_back(m_strengthRecord[i]);
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
    for (unsigned char i : c.m_cityname_count)
    {
        cityname_count.push_back(i);
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
        {"leader_name",              utf8_safe(c.m_leader_name)},
        {"personality_description",  utf8_safe(c.m_personality_description)},
        {"civilisation_name",        utf8_safe(c.m_civilisation_name)},
        {"country_name",             utf8_safe(c.m_country_name)},
        {"singular_name",            utf8_safe(c.m_singular_name)},
    };
}

namespace {
// Copy a std::string into a fixed-size char buffer, zero-filling the
// remainder so we don't carry uninitialised stack/heap bytes (the
// Pre-A SaveExtendedGameInfo lesson).
void load_fixed_string(MBCHAR *dest, std::size_t buf_size,
                       std::string const &src)
{
    // Saves store UTF-8 (utf8_safe on write); the fixed buffers are
    // Latin-1 in memory -- decode back so the UI font path and re-saves
    // stay consistent.
    std::string const decoded = latin1_safe(src);
    std::size_t const n = std::min(decoded.size(), buf_size - 1);
    std::memcpy(dest, decoded.data(), n);
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
        {"name",            utf8_safe(q.m_name)},
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
    // Serialize as JSON booleans (not integers) to preserve save-file
    // compatibility with the pre-refactor bool[] representation. Without the
    // explicit cast nlohmann would emit 0/1 integers for uint8.
    for (sint32 i = 0; i < feat_count; ++i) achieved     .push_back(static_cast<bool>(ft.m_achieved[i]));
    for (sint32 i = 0; i < bldg_count; ++i) building_feat.push_back(static_cast<bool>(ft.m_buildingFeat[i]));

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

    // Accept either JSON booleans (legacy bool[] saves) or integers (post-
    // refactor uint8 saves) so save files remain readable across the refactor.
    auto json_to_uint8 = [](nlohmann::json const &v) -> uint8 {
        if (v.is_boolean()) return v.get<bool>() ? 1 : 0;
        return v.get<sint32>() != 0 ? 1 : 0;
    };
    if (static_cast<sint32>(achieved.size()) == feat_count)
    {
        for (sint32 i = 0; i < feat_count; ++i)
            ft.m_achieved[i] = json_to_uint8(achieved[i]);
    }
    if (static_cast<sint32>(building_feat.size()) == bldg_count)
    {
        for (sint32 i = 0; i < bldg_count; ++i)
            ft.m_buildingFeat[i] = json_to_uint8(building_feat[i]);
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
// + m_point + m_path. m_gameEventArgs (GameEventArgList *) remains
// deferred — it holds transient event args regenerated by the
// event-processing loop. Without m_path, ArmyData::ExecuteMoveOrder
// (the first turn after JSON load) dereferences nullptr and crashes
// (Phase 1j finding, 2026-06-03). The Path JSON bridge itself lives
// further down (search for "Mirrors Path::Serialize").

void to_json(nlohmann::json &j, Order const &o)
{
    j = nlohmann::json{
        {"order",           static_cast<sint32>(o.m_order)},
        {"round",           o.m_round},
        {"point",           o.m_point},
        {"argument",        o.m_argument},
        {"event_type",      static_cast<sint32>(o.m_eventType)},
        {"path",            o.m_path ? nlohmann::json(*o.m_path) : nlohmann::json(nullptr)},
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
    if (j.contains("path") && !j.at("path").is_null()) {
        o.m_path = new Path();
        j.at("path").get_to(*o.m_path);
    } else {
        o.m_path = nullptr;
    }
    o.m_gameEventArgs = nullptr;
}

// Phase E-2 — CellUnitList (base for ArmyData + Cell.m_unit_army)
// Stores only the first m_nElements of m_array as IDs.  m_array's
// fixed k_MAX_ARMY_SIZE slots past m_nElements are not persisted.

void to_json(nlohmann::json &j, CellUnitList const &c)
{
    nlohmann::json units = nlohmann::json::array();
    for (sint32 i = 0; i < c.m_nElements; ++i)
    {
        units.push_back(static_cast<ID const &>(c.m_array[i]));
    }
    j = nlohmann::json{
        {"units",              std::move(units)},
        {"move_intersection",  c.m_moveIntersection},
        {"flags",              c.m_flags},
        {"num_elements",       c.m_nElements},
    };
}

void from_json(nlohmann::json const &j, CellUnitList &c)
{
    auto const &units = j.at("units");
    j.at("num_elements").get_to(c.m_nElements);
    if (static_cast<sint32>(units.size()) != c.m_nElements
        || c.m_nElements > k_MAX_ARMY_SIZE)
    {
        throw nlohmann::json::other_error::create(
            580, "cell_unit_list.units size mismatch with num_elements "
                 "or exceeds k_MAX_ARMY_SIZE", &j);
    }
    for (sint32 i = 0; i < c.m_nElements; ++i)
    {
        ID id(0);
        units[i].get_to(id);
        c.m_array[i] = Unit(id.m_id);
    }
    j.at("move_intersection").get_to(c.m_moveIntersection);
    j.at("flags")            .get_to(c.m_flags);
}

// Phase E-2 — ArmyData
//
// Mirrors ArmyData::Serialize at ArmyData.cpp:494.  Composes GameObj
// id + CellUnitList base + UnitDynamicArray + Order list + scalars
// + variable-length name.  OMITS intrusive list (m_lesser/m_greater)
// + transient state (m_tempKillList, m_killMeSoon, m_debugString,
// m_reentryTurn, m_reentryPos).

void to_json(nlohmann::json &j, ArmyData const &a)
{
    // m_attackedByDefenders is a UnitDynamicArray = DynamicArray<Unit>
    nlohmann::json attacked_by_defenders = nlohmann::json::array();
    if (a.m_attackedByDefenders)
    {
        for (sint32 i = 0; i < a.m_attackedByDefenders->Num(); ++i)
        {
            attacked_by_defenders.push_back(
                static_cast<ID const &>(a.m_attackedByDefenders->Access(i)));
        }
    }

    // m_orders is PointerList<Order>
    nlohmann::json orders = nlohmann::json::array();
    if (a.m_orders)
    {
        PointerList<Order>::Walker walk(a.m_orders);
        while (walk.IsValid())
        {
            orders.push_back(*walk.GetObj());
            walk.Next();
        }
    }

    j = nlohmann::json{
        {"id",                       a.m_id},
        {"cell_unit_list",           static_cast<CellUnitList const &>(a)},
        {"attacked_by_defenders",    std::move(attacked_by_defenders)},
        {"pos",                      a.m_pos},
        {"owner",                    a.m_owner},
        {"killer",                   a.m_killer},
        {"remove_cause",             static_cast<sint32>(a.m_removeCause)},
        {"dont_kill_count",          a.m_dontKillCount},
        {"need_to_kill",             a.m_needToKill},
        {"has_been_added",           a.m_hasBeenAdded},
        {"is_pirating",              a.m_isPirating},
        {"orders",                   std::move(orders)},
        {"name",                     utf8_safe(a.m_name.c_str())},
    };
}

void from_json(nlohmann::json const &j, ArmyData &a)
{
    j.at("id").get_to(a.m_id);
    j.at("cell_unit_list").get_to(static_cast<CellUnitList &>(a));

    if (!a.m_attackedByDefenders) a.m_attackedByDefenders = new UnitDynamicArray;
    a.m_attackedByDefenders->Clear();
    for (auto const &id_json : j.at("attacked_by_defenders"))
    {
        ID id(0);
        id_json.get_to(id);
        a.m_attackedByDefenders->Insert(Unit(id.m_id));
    }

    j.at("pos")              .get_to(a.m_pos);
    j.at("owner")            .get_to(a.m_owner);
    j.at("killer")           .get_to(a.m_killer);
    a.m_removeCause = static_cast<CAUSE_REMOVE_ARMY>(
        j.at("remove_cause").get<sint32>());
    j.at("dont_kill_count")  .get_to(a.m_dontKillCount);
    j.at("need_to_kill")     .get_to(a.m_needToKill);
    j.at("has_been_added")   .get_to(a.m_hasBeenAdded);
    j.at("is_pirating")      .get_to(a.m_isPirating);

    if (!a.m_orders) a.m_orders = new PointerList<Order>;
    a.m_orders->DeleteAll();
    for (auto const &order_json : j.at("orders"))
    {
        Order *order = new Order;
        order_json.get_to(*order);
        a.m_orders->AddTail(order);
    }

    a.m_name = j.at("name").get<std::string>();
}

// Phase E-5 — UnitData
//
// Mirrors UnitData::Serialize at UnitData.cpp:2290.  Captures every
// persisted scalar + value member + nullable sub-objects (cargo list,
// city data, vision array, transport, target city, round-the-world
// mask, explore state).  OMITS gfx/render (m_actor, m_sprite_state),
// pool intrusive list (m_lesser, m_greater), and debug-only m_text.
//
// Slice 7i amputation will eventually remove m_actor entirely — at
// that point the binary path drops it too and this bridge needs no
// change.

void to_json(nlohmann::json &j, UnitData const &u)
{
    nlohmann::json cargo = nlohmann::json::object();
    if (u.m_cargo_list)
    {
        nlohmann::json units = nlohmann::json::array();
        for (sint32 i = 0; i < u.m_cargo_list->Num(); ++i)
            units.push_back(static_cast<ID const &>(u.m_cargo_list->Access(i)));
        cargo = nlohmann::json{{"present", true}, {"units", std::move(units)}};
    }
    else
    {
        cargo = nlohmann::json{{"present", false}};
    }

    j = nlohmann::json{
        {"id",                          u.m_id},
        {"owner",                       static_cast<sint32>(u.m_owner)},
        {"fuel",                        u.m_fuel},
        {"hp",                          u.m_hp},
        {"movement_points",             u.m_movement_points},
        {"type",                        u.m_type},
        {"visibility",                  u.m_visibility},
        {"temp_visibility",             u.m_temp_visibility},
        {"radar_visibility",            u.m_radar_visibility},
        {"ever_visible",                u.m_ever_visible},
        {"flags",                       u.m_flags},
        {"army",                        u.m_army},
        {"pos",                         u.m_pos},
        {"cargo_list",                  std::move(cargo)},
        {"city_data",                   u.m_city_data
                                            ? nlohmann::json(*u.m_city_data)
                                            : nlohmann::json(nullptr)},
        {"state",                       u.m_state},
        {"temp_visibility_array",       u.m_temp_visibility_array},
        {"transport",                   u.m_transport},
        {"round_the_world_mask",        u.m_roundTheWorldMask
                                            ? nlohmann::json(*u.m_roundTheWorldMask)
                                            : nlohmann::json(nullptr)},
        {"target_city",                 u.m_target_city},
        {"is_exploring",                u.m_isExploring},
        {"explore_target",              u.m_exploreTarget},
    };
}

void from_json(nlohmann::json const &j, UnitData &u)
{
    j.at("id")              .get_to(u.m_id);
    u.m_owner = static_cast<PLAYER_INDEX>(j.at("owner").get<sint32>());
    j.at("fuel")            .get_to(u.m_fuel);
    j.at("hp")              .get_to(u.m_hp);
    j.at("movement_points") .get_to(u.m_movement_points);
    j.at("type")            .get_to(u.m_type);
    j.at("visibility")      .get_to(u.m_visibility);
    j.at("temp_visibility") .get_to(u.m_temp_visibility);
    j.at("radar_visibility").get_to(u.m_radar_visibility);
    j.at("ever_visible")    .get_to(u.m_ever_visible);
    j.at("flags")           .get_to(u.m_flags);
    j.at("army")            .get_to(u.m_army);
    j.at("pos")             .get_to(u.m_pos);

    delete u.m_cargo_list;
    u.m_cargo_list = nullptr;
    auto const &cargo = j.at("cargo_list");
    if (cargo.at("present").get<bool>())
    {
        u.m_cargo_list = new UnitDynamicArray;
        for (auto const &id_json : cargo.at("units"))
        {
            ID id(0);
            id_json.get_to(id);
            u.m_cargo_list->Insert(Unit(id.m_id));
        }
    }

    delete u.m_city_data;
    u.m_city_data = nullptr;
    if (!j.at("city_data").is_null())
    {
        // CityData has no default ctor; use the (owner, hc, pos) form
        // with placeholders — from_json overwrites all of these.
        u.m_city_data = new CityData(0, Unit(0), MapPoint(0, 0));
        j.at("city_data").get_to(*u.m_city_data);
    }

    j.at("state")                .get_to(u.m_state);
    j.at("temp_visibility_array").get_to(u.m_temp_visibility_array);
    j.at("transport")            .get_to(u.m_transport);

    delete u.m_roundTheWorldMask;
    u.m_roundTheWorldMask = nullptr;
    if (!j.at("round_the_world_mask").is_null())
    {
        u.m_roundTheWorldMask = new BitMask(1);  // dummy size; replaced by from_json
        j.at("round_the_world_mask").get_to(*u.m_roundTheWorldMask);
    }

    j.at("target_city")  .get_to(u.m_target_city);
    j.at("is_exploring") .get_to(u.m_isExploring);
    j.at("explore_target").get_to(u.m_exploreTarget);
}

// Phase F-4 — InstallationData + InstallationPool

void to_json(nlohmann::json &j, InstallationData const &d)
{
    j = nlohmann::json{
        {"id",                  d.m_id},
        {"owner",               d.m_owner},
        {"type",                d.m_type},
        {"point",               d.m_point},
        {"airfield_last_used",  d.m_airfieldLastUsed},
        {"visibility",          d.m_visibility},
    };
}

void from_json(nlohmann::json const &j, InstallationData &d)
{
    j.at("id")                .get_to(d.m_id);
    j.at("owner")             .get_to(d.m_owner);
    j.at("type")              .get_to(d.m_type);
    j.at("point")             .get_to(d.m_point);
    j.at("airfield_last_used").get_to(d.m_airfieldLastUsed);
    j.at("visibility")        .get_to(d.m_visibility);
}

void to_json(nlohmann::json &j, InstallationPool const &p)
{
    nlohmann::json installations = nlohmann::json::array();
    for (auto i : p.m_table)
    {
        if (i)
            installations.push_back(*reinterpret_cast<InstallationData const *>(i));
    }
    j = nlohmann::json{
        {"next_key",      const_cast<InstallationPool &>(p).HackGetKey()},
        {"installations", std::move(installations)},
    };
}

void from_json(nlohmann::json const &j, InstallationPool &p)
{
    // Drain pre-existing entries (see UnitPool::from_json for rationale).
    for (auto & i : p.m_table)
    {
        while (i)
            p.Del(i);
    }

    p.HackSetKey(j.at("next_key").get<uint32>());
    for (auto const &entry : j.at("installations"))
    {
        InstallationData *data = new InstallationData(ID(0));
        entry.get_to(*data);
        p.Insert(data);
    }
}

// Phase F-5 — Path (embedded in TradeRouteData via m_astarPath).
// Mirrors Path::Serialize at robot/pathing/Path.cpp:315.  Direction is
// a one-byte struct (sint8 dir); serialise as raw int — no per-Direction
// bridge.

void to_json(nlohmann::json &j, Path const &p)
{
    nlohmann::json step = nlohmann::json::array();
    for (sint32 i = 0; i < p.m_step.Num(); ++i)
    {
        step.push_back(p.m_step.Access(i).dir);
    }
    j = nlohmann::json{
        {"current",  p.m_current},
        {"next",     p.m_next},
        {"next_dir", p.m_next_dir},
        {"start",    p.m_start},
        {"step",     std::move(step)},
    };
}

void from_json(nlohmann::json const &j, Path &p)
{
    j.at("current") .get_to(p.m_current);
    j.at("next")    .get_to(p.m_next);
    j.at("next_dir").get_to(p.m_next_dir);
    j.at("start")   .get_to(p.m_start);

    p.m_step.Clear();
    for (auto const &dir_json : j.at("step"))
    {
        Direction d;
        d.dir = dir_json.get<sint8>();
        p.m_step.Insert(d);
    }
}

// Phase F-5 — TradeRouteData
// Mirrors TradeRouteData::Serialize at TradeRouteData.cpp:374.
// OMITS m_lesser/m_greater (intrusive list, pool concern),
// m_piratingArmy + m_dontAdjustPointsWhenKilled (not in binary
// Serialize).

namespace {
nlohmann::json mapPointArrayToJson(DynamicArray<MapPoint> const &arr)
{
    nlohmann::json out = nlohmann::json::array();
    for (sint32 i = 0; i < arr.Num(); ++i)
    {
        out.push_back(arr.Access(i));
    }
    return out;
}

void jsonToMapPointArray(nlohmann::json const &j, DynamicArray<MapPoint> &arr)
{
    arr.Clear();
    for (auto const &mp : j)
    {
        MapPoint p;
        mp.get_to(p);
        arr.Insert(p);
    }
}
}  // namespace

void to_json(nlohmann::json &j, TradeRouteData const &d)
{
    nlohmann::json passes_through = nlohmann::json::array();
    for (unsigned int i : d.m_passesThrough)
    {
        passes_through.push_back(static_cast<bool>(i));
    }

    j = nlohmann::json{
        {"id",                   d.m_id},
        {"transport_cost",       d.m_transportCost},
        {"owner",                d.m_owner},
        {"source_route_type",    static_cast<sint32>(d.m_sourceRouteType)},
        {"source_resource",      d.m_sourceResource},
        {"passes_through",       std::move(passes_through)},
        {"crosses_water",        static_cast<bool>(d.m_crossesWater)},
        {"is_active",            static_cast<bool>(d.m_isActive)},
        {"color",                d.m_color},
        {"outline",              d.m_outline},
        {"selected_index",       d.m_selectedIndex},
        {"path_selection_state", d.m_path_selection_state},
        {"valid",                static_cast<bool>(d.m_valid)},
        {"paying_for",           d.m_payingFor},
        {"gold_in_return",       d.m_gold_in_return},
        {"source_city",          static_cast<ID const &>(d.m_sourceCity)},
        {"destination_city",     static_cast<ID const &>(d.m_destinationCity)},
        {"recip",                static_cast<ID const &>(d.m_recip)},
        {"path",                 mapPointArrayToJson(d.m_path)},
        {"way_points",           mapPointArrayToJson(d.m_wayPoints)},
        {"selected_path",        mapPointArrayToJson(d.m_selectedPath)},
        {"selected_way_points",  mapPointArrayToJson(d.m_selectedWayPoints)},
        {"set_path",             mapPointArrayToJson(d.m_setPath)},
        {"set_way_points",       mapPointArrayToJson(d.m_setWayPoints)},
        {"astar_path",           d.m_astarPath ? nlohmann::json(*d.m_astarPath)
                                               : nlohmann::json(nullptr)},
    };
}

void from_json(nlohmann::json const &j, TradeRouteData &d)
{
    j.at("id")                  .get_to(d.m_id);
    j.at("transport_cost")      .get_to(d.m_transportCost);
    j.at("owner")               .get_to(d.m_owner);
    d.m_sourceRouteType = static_cast<ROUTE_TYPE>(
        j.at("source_route_type").get<sint32>());
    j.at("source_resource")     .get_to(d.m_sourceResource);

    auto const &passes_through = j.at("passes_through");
    if (static_cast<sint32>(passes_through.size()) != k_MAX_PLAYERS)
    {
        throw nlohmann::json::other_error::create(
            581, "trade_route_data.passes_through size mismatch with "
                 "k_MAX_PLAYERS", &j);
    }
    for (sint32 i = 0; i < k_MAX_PLAYERS; ++i)
    {
        d.m_passesThrough[i] = passes_through[i].get<bool>();
    }

    d.m_crossesWater = j.at("crosses_water").get<bool>();
    d.m_isActive     = j.at("is_active")    .get<bool>();
    j.at("color")              .get_to(d.m_color);
    j.at("outline")            .get_to(d.m_outline);
    j.at("selected_index")     .get_to(d.m_selectedIndex);
    j.at("path_selection_state").get_to(d.m_path_selection_state);
    d.m_valid        = j.at("valid")        .get<bool>();
    j.at("paying_for")         .get_to(d.m_payingFor);
    j.at("gold_in_return")     .get_to(d.m_gold_in_return);

    ID id(0);
    j.at("source_city")     .get_to(id);   d.m_sourceCity      = Unit(id.m_id);
    j.at("destination_city").get_to(id);   d.m_destinationCity = Unit(id.m_id);
    j.at("recip")           .get_to(id);   d.m_recip           = TradeRoute(id.m_id);

    jsonToMapPointArray(j.at("path"),                d.m_path);
    jsonToMapPointArray(j.at("way_points"),          d.m_wayPoints);
    jsonToMapPointArray(j.at("selected_path"),       d.m_selectedPath);
    jsonToMapPointArray(j.at("selected_way_points"), d.m_selectedWayPoints);
    jsonToMapPointArray(j.at("set_path"),            d.m_setPath);
    jsonToMapPointArray(j.at("set_way_points"),      d.m_setWayPoints);

    if (!d.m_astarPath) d.m_astarPath = new Path;
    j.at("astar_path").get_to(*d.m_astarPath);
}

// Phase F-5 — TradePool
// Mirrors TradePool::Serialize at TradePool.cpp:121.  Persists every
// live TradeRouteData entry + ObjPool key counter.  m_all_routes (a
// TradeDynamicArray = flat view of m_table) is rebuilt during load.

void to_json(nlohmann::json &j, TradePool const &p)
{
    nlohmann::json routes = nlohmann::json::array();
    for (auto i : p.m_table)
    {
        if (i)
            routes.push_back(*reinterpret_cast<TradeRouteData const *>(i));
    }
    j = nlohmann::json{
        {"next_key", const_cast<TradePool &>(p).HackGetKey()},
        {"routes",   std::move(routes)},
    };
}

void from_json(nlohmann::json const &j, TradePool &p)
{
    // Drain pre-existing entries (see UnitPool::from_json for rationale).
    for (auto & i : p.m_table)
    {
        while (i)
            p.Del(i);
    }

    p.HackSetKey(j.at("next_key").get<uint32>());
    if (p.m_all_routes) p.m_all_routes->Clear();
    for (auto const &entry : j.at("routes"))
    {
        TradeRouteData *data = new TradeRouteData(TradeRoute(0));
        entry.get_to(*data);
        p.Insert(data);
        if (p.m_all_routes) p.m_all_routes->Insert(TradeRoute(data->m_id));
    }
}

// Phase F-6 — TerrainImprovementData + TerrainImprovementPool
// Mirrors TerrImproveData.cpp:201 and TerrImprovePool.cpp:148.

void to_json(nlohmann::json &j, TerrainImprovementData const &d)
{
    j = nlohmann::json{
        {"id",                 d.m_id},
        {"owner",              d.m_owner},
        {"type",               d.m_type},
        {"point",              d.m_point},
        {"turns_to_complete",  d.m_turnsToComplete},
        {"transform_type",     static_cast<sint32>(d.m_transformType)},
        {"material_cost",      d.m_materialCost},
        {"is_complete",        d.m_isComplete},
        {"is_building",        d.m_isBuilding},
    };
}

void from_json(nlohmann::json const &j, TerrainImprovementData &d)
{
    j.at("id")               .get_to(d.m_id);
    j.at("owner")            .get_to(d.m_owner);
    j.at("type")             .get_to(d.m_type);
    j.at("point")            .get_to(d.m_point);
    j.at("turns_to_complete").get_to(d.m_turnsToComplete);
    d.m_transformType = static_cast<TERRAIN_TYPES>(
        j.at("transform_type").get<sint32>());
    j.at("material_cost")    .get_to(d.m_materialCost);
    j.at("is_complete")      .get_to(d.m_isComplete);
    j.at("is_building")      .get_to(d.m_isBuilding);
}

void to_json(nlohmann::json &j, TerrainImprovementPool const &p)
{
    nlohmann::json improvements = nlohmann::json::array();
    for (auto i : p.m_table)
    {
        if (i)
            improvements.push_back(
                *reinterpret_cast<TerrainImprovementData const *>(i));
    }
    j = nlohmann::json{
        {"next_key",     const_cast<TerrainImprovementPool &>(p).HackGetKey()},
        {"improvements", std::move(improvements)},
    };
}

void from_json(nlohmann::json const &j, TerrainImprovementPool &p)
{
    // Drain pre-existing entries (see UnitPool::from_json for rationale).
    for (auto & i : p.m_table)
    {
        while (i)
            p.Del(i);
    }

    p.HackSetKey(j.at("next_key").get<uint32>());
    for (auto const &entry : j.at("improvements"))
    {
        TerrainImprovementData *data = new TerrainImprovementData(ID(0));
        entry.get_to(*data);
        p.Insert(data);
    }
}

// Phase 0.B — DiplomaticRequestData + DiplomaticRequestPool + AgreementPool + TradeOfferPool
//
// All three pools follow the UnitPool / ArmyPool template: store next_key
// + array of live entries; on load drain + restore next_key + reinsert.

void to_json(nlohmann::json &j, DiplomaticRequestData const &d)
{
    j = nlohmann::json{
        {"id",                  d.m_id},
        {"round",               d.m_round},
        {"owner",               d.m_owner},
        {"recipient",           d.m_recipient},
        {"third_party",         d.m_thirdParty},
        {"request",             static_cast<sint32>(d.m_request)},
        {"response",            static_cast<sint32>(d.m_response)},
        {"tone",                d.m_tone},
        {"advance",             d.m_advance},
        {"reciprocal_advance",  d.m_reciprocalAdvance},
        {"target_city",         static_cast<ID const &>(d.m_targetCity)},
        {"reciprocal_city",     static_cast<ID const &>(d.m_reciprocalCity)},
        {"amount",              d.m_amount},
    };
}

void from_json(nlohmann::json const &j, DiplomaticRequestData &d)
{
    j.at("id")                .get_to(d.m_id);
    j.at("round")             .get_to(d.m_round);
    j.at("owner")             .get_to(d.m_owner);
    j.at("recipient")         .get_to(d.m_recipient);
    j.at("third_party")       .get_to(d.m_thirdParty);
    d.m_request  = static_cast<REQUEST_TYPE>(j.at("request").get<sint32>());
    d.m_response = static_cast<REQUEST_RESPONSE_TYPE>(j.at("response").get<sint32>());
    j.at("tone")              .get_to(d.m_tone);
    j.at("advance")           .get_to(d.m_advance);
    j.at("reciprocal_advance").get_to(d.m_reciprocalAdvance);
    ID tmp(0);
    j.at("target_city")       .get_to(tmp);
    d.m_targetCity = Unit(tmp.m_id);
    j.at("reciprocal_city")   .get_to(tmp);
    d.m_reciprocalCity = Unit(tmp.m_id);
    j.at("amount")            .get_to(d.m_amount);
}

void to_json(nlohmann::json &j, DiplomaticRequestPool const &p)
{
    nlohmann::json requests = nlohmann::json::array();
    for (auto i : p.m_table)
    {
        if (i)
            requests.push_back(
                *reinterpret_cast<DiplomaticRequestData const *>(i));
    }
    j = nlohmann::json{
        {"next_key", const_cast<DiplomaticRequestPool &>(p).HackGetKey()},
        {"requests", std::move(requests)},
    };
}

void from_json(nlohmann::json const &j, DiplomaticRequestPool &p)
{
    for (auto & i : p.m_table)
    {
        while (i)
            p.Del(i);
    }
    p.HackSetKey(j.at("next_key").get<uint32>());
    for (auto const &entry : j.at("requests"))
    {
        DiplomaticRequestData *data = new DiplomaticRequestData(ID(0), /*currentRound*/0);
        entry.get_to(*data);
        p.Insert(data);
    }
}

void to_json(nlohmann::json &j, AgreementPool const &p)
{
    nlohmann::json agreements = nlohmann::json::array();
    for (auto i : p.m_table)
    {
        if (i)
            agreements.push_back(
                *reinterpret_cast<AgreementData const *>(i));
    }
    j = nlohmann::json{
        {"next_key",   const_cast<AgreementPool &>(p).HackGetKey()},
        {"agreements", std::move(agreements)},
    };
}

void from_json(nlohmann::json const &j, AgreementPool &p)
{
    for (auto & i : p.m_table)
    {
        while (i)
            p.Del(i);
    }
    p.HackSetKey(j.at("next_key").get<uint32>());
    for (auto const &entry : j.at("agreements"))
    {
        AgreementData *data = new AgreementData(ID(0));
        entry.get_to(*data);
        p.Insert(data);
    }
}

void to_json(nlohmann::json &j, TradeOfferPool const &p)
{
    nlohmann::json offers = nlohmann::json::array();
    for (auto i : p.m_table)
    {
        if (i)
            offers.push_back(
                *reinterpret_cast<TradeOfferData const *>(i));
    }
    j = nlohmann::json{
        {"next_key", const_cast<TradeOfferPool &>(p).HackGetKey()},
        {"offers",   std::move(offers)},
    };
}

void from_json(nlohmann::json const &j, TradeOfferPool &p)
{
    for (auto & i : p.m_table)
    {
        while (i)
            p.Del(i);
    }
    p.HackSetKey(j.at("next_key").get<uint32>());
    for (auto const &entry : j.at("offers"))
    {
        TradeOfferData *data = new TradeOfferData(ID(0));
        entry.get_to(*data);
        p.Insert(data);
    }
}

// Phase 0.B — Vision
//
// Mirrors Vision::Serialize at Vision.cpp:679.  Scalars + flattened
// uint16 array (width*height row-major) + UnseenCell list (via
// existing UnseenCell JSON bridge).  Width/height drive both
// allocation and read sizes, so they're persisted first.

void to_json(nlohmann::json &j, Vision const &v)
{
    sint32 const wh = static_cast<sint32>(v.m_width) * static_cast<sint32>(v.m_height);
    nlohmann::json grid = nlohmann::json::array();
    if (!v.m_array.empty())
    {
        for (sint16 x = 0; x < v.m_width; ++x)
        {
            for (sint16 y = 0; y < v.m_height; ++y)
                grid.push_back(v.m_array[x][y]);
        }
    }

    nlohmann::json unseen_cells = nlohmann::json::array();
    if (v.m_unseenCells)
    {
        DynamicArray<UnseenCellCarton> array;
        v.m_unseenCells->BuildList(array);
        for (sint32 i = 0; i < array.Num(); ++i)
        {
            if (array[i].m_unseenCell)
                unseen_cells.push_back(*array[i].m_unseenCell);
        }
    }

    j = nlohmann::json{
        {"width",         v.m_width},
        {"height",        v.m_height},
        {"owner",         v.m_owner},
        {"xy_conversion", v.m_xyConversion},
        {"is_y_wrap",     static_cast<bool>(v.m_isYwrap)},
        {"am_on_screen",  static_cast<bool>(v.m_amOnScreen)},
        {"grid",          std::move(grid)},
        {"unseen_cells",  std::move(unseen_cells)},
    };
    (void)wh; // explicit size capture available for future schema checks
}

void from_json(nlohmann::json const &j, Vision &v)
{
    // Tear down existing storage (matches Vision::Serialize's load branch).
    v.m_array.clear();
    v.DeleteUnseenCells();
    v.m_unseenCells.reset();

    j.at("width")        .get_to(v.m_width);
    j.at("height")       .get_to(v.m_height);
    j.at("owner")        .get_to(v.m_owner);
    j.at("xy_conversion").get_to(v.m_xyConversion);
    v.m_isYwrap    = j.at("is_y_wrap")   .get<bool>() ? TRUE : FALSE;
    v.m_amOnScreen = j.at("am_on_screen").get<bool>() ? TRUE : FALSE;

    auto const &grid = j.at("grid");
    sint32 expected = static_cast<sint32>(v.m_width) * static_cast<sint32>(v.m_height);
    if (static_cast<sint32>(grid.size()) != expected)
        throw nlohmann::json::other_error::create(
            501, "Vision grid size mismatch", &j);
    v.m_array.assign(v.m_width, std::vector<uint16>(v.m_height));
    sint32 idx = 0;
    for (sint16 x = 0; x < v.m_width; ++x)
    {
        for (sint16 y = 0; y < v.m_height; ++y)
            grid[idx++].get_to(v.m_array[x][y]);
    }

    v.m_unseenCells = std::make_unique<UnseenCellQuadTree>(v.m_width, v.m_height, v.m_isYwrap);
    for (auto const &entry : j.at("unseen_cells"))
    {
        UnseenCell *uc = new UnseenCell(MapPoint(0, 0));
        entry.get_to(*uc);
        UnseenCellCarton carton(uc);
        v.m_unseenCells->Insert(carton);
    }
}

// Phase F-7 — SlicConst (smallest Slic-family leaf, no other Slic deps).
// Mirrors SlicConst::Serialize at gs/slic/SlicConst.cpp:23.  Persists
// the (length-prefixed) name string and integer value.

void to_json(nlohmann::json &j, SlicConst const &c)
{
    j = nlohmann::json{
        {"name",  c.m_name},
        {"value", c.m_value},
    };
}

void from_json(nlohmann::json const &j, SlicConst &c)
{
    j.at("name").get_to(c.m_name);
    j.at("value").get_to(c.m_value);
}

// Phase F-8 — SlicRecord (per-player slic message journal entry).
// Mirrors SlicRecord::Serialize at gs/slic/SlicRecord.cpp:53.  Persists
// owner + title/text strings + the segment's name (resolved via
// slicengine_Get()->GetSegment on load — m_segment stays nullptr when
// slicengine_Get() isn't initialised or the segment name is empty).
//
// JSON shape distinguishes "missing string" from "empty string":
// null when the source pointer was NULL (binary path wrote l=-1),
// otherwise the literal string.

namespace {
nlohmann::json optStringToJson(MBCHAR const *s)
{
    return s ? nlohmann::json(utf8_safe(s)) : nlohmann::json(nullptr);
}

void jsonToOptString(nlohmann::json const &j, MBCHAR *&dest)
{
    delete[] dest;
    if (j.is_null())
    {
        dest = nullptr;
        return;
    }
    std::string s = latin1_safe(j.get<std::string>());
    dest = new MBCHAR[s.size() + 1];
    std::memcpy(dest, s.c_str(), s.size() + 1);
}

// std::string overloads — preserve the same shape (null in JSON ↔ empty
// string), used by classes whose char* fields have been migrated to
// std::string (e.g. MessageData::m_text / m_title).
nlohmann::json optStringToJson(std::string const &s)
{
    return s.empty() ? nlohmann::json(nullptr) : nlohmann::json(utf8_safe(s.c_str()));
}

void jsonToOptString(nlohmann::json const &j, std::string &dest)
{
    if (j.is_null())
    {
        dest.clear();
        return;
    }
    dest = latin1_safe(j.get<std::string>());
}

// A Slic segment/function can exist with a NULL name (seen after
// load_game over a running game: runtime-created Slic state whose
// source segment never had a name bound). std::string(nullptr) is UB
// and aborts under libc++ hardening, so every GetName() result is
// converted through here.
std::string safeName(MBCHAR const *s)
{
    return s ? std::string(s) : std::string();
}
}  // namespace

void to_json(nlohmann::json &j, SlicRecord const &r)
{
    j = nlohmann::json{
        {"owner",        r.m_owner},
        {"title",        optStringToJson(r.m_title)},
        {"text",         optStringToJson(r.m_text)},
        {"segment_name", r.m_segment ? safeName(r.m_segment->GetName())
                                     : std::string()},
    };
}

void from_json(nlohmann::json const &j, SlicRecord &r)
{
    j.at("owner").get_to(r.m_owner);
    jsonToOptString(j.at("title"), r.m_title);
    jsonToOptString(j.at("text"),  r.m_text);

    std::string seg_name = j.at("segment_name").get<std::string>();
    r.m_segment = (slicengine_Get() && !seg_name.empty())
                      ? slicengine_Get()->GetSegment(seg_name.c_str())
                      : nullptr;
}

// Phase F-2 — EndGame
//
// Mirrors EndGame::Serialize.  Scalars + two num-built arrays sized
// by endgamedb_Get()->m_nRec.  (TopTen bridge is inline in TopTen.h
// since it has no DB dependency.)

void to_json(nlohmann::json &j, EndGame const &g)
{
    sint32 const nRec = endgamedb_Get() ? endgamedb_Get()->m_nRec : 0;
    std::vector<sint32> num_built;
    std::vector<sint32> saved_num_built;
    if (g.m_numBuilt)
        num_built.assign(g.m_numBuilt.get(), g.m_numBuilt.get() + nRec);
    if (g.m_savedNumBuilt)
        saved_num_built.assign(g.m_savedNumBuilt.get(), g.m_savedNumBuilt.get() + nRec);

    j = nlohmann::json{
        {"owner",                 g.m_owner},
        {"current_stage",         g.m_currentStage},
        {"saved_current_stage",   g.m_savedCurrentStage},
        {"current_stage_began",   g.m_currentStageBegan},
        {"num_built",             std::move(num_built)},
        {"saved_num_built",       std::move(saved_num_built)},
    };
}

void from_json(nlohmann::json const &j, EndGame &g)
{
    j.at("owner")              .get_to(g.m_owner);
    j.at("current_stage")      .get_to(g.m_currentStage);
    j.at("saved_current_stage").get_to(g.m_savedCurrentStage);
    j.at("current_stage_began").get_to(g.m_currentStageBegan);

    std::vector<sint32> num_built;
    std::vector<sint32> saved_num_built;
    j.at("num_built")      .get_to(num_built);
    j.at("saved_num_built").get_to(saved_num_built);

    if (!num_built.empty()) {
        g.m_numBuilt = std::make_unique<sint32[]>(num_built.size());
        for (size_t i = 0; i < num_built.size(); ++i) g.m_numBuilt[i] = num_built[i];
    } else {
        g.m_numBuilt.reset();
    }
    if (!saved_num_built.empty()) {
        g.m_savedNumBuilt = std::make_unique<sint32[]>(saved_num_built.size());
        for (size_t i = 0; i < saved_num_built.size(); ++i) g.m_savedNumBuilt[i] = saved_num_built[i];
    } else {
        g.m_savedNumBuilt.reset();
    }
}

// Phase F-1 — CivilisationPool
//
// Mirrors CivilisationPool::Serialize.  Persists ObjPool key counter
// + every live CivilisationData entry + m_usedCivs index list.

void to_json(nlohmann::json &j, CivilisationPool const &p)
{
    nlohmann::json civs = nlohmann::json::array();
    for (auto i : p.m_table)
    {
        if (i)
            civs.push_back(*reinterpret_cast<CivilisationData const *>(i));
    }
    nlohmann::json used_civs = nlohmann::json::array();
    if (p.m_usedCivs)
    {
        for (sint32 i = 0; i < p.m_usedCivs->Num(); ++i)
            used_civs.push_back(p.m_usedCivs->Access(i));
    }
    j = nlohmann::json{
        {"next_key",   const_cast<CivilisationPool &>(p).HackGetKey()},
        {"civs",       std::move(civs)},
        {"used_civs",  std::move(used_civs)},
    };
}

void from_json(nlohmann::json const &j, CivilisationPool &p)
{
    // Drain pre-existing entries (see UnitPool::from_json for rationale).
    for (auto & i : p.m_table)
    {
        while (i)
            p.Del(i);
    }

    p.HackSetKey(j.at("next_key").get<uint32>());

    for (auto const &entry : j.at("civs"))
    {
        CivilisationData *data = new CivilisationData(ID(0));
        entry.get_to(*data);
        p.Insert(data);
    }

    if (!p.m_usedCivs) p.m_usedCivs = std::make_unique<SimpleDynamicArray<sint32>>();
    p.m_usedCivs->Clear();
    for (auto const &id : j.at("used_civs"))
    {
        sint32 v = 0;
        id.get_to(v);
        p.m_usedCivs->Insert(v);
    }
}

// Phase E-6 — UnitPool
//
// Mirrors UnitPool::Serialize at UnitPool.cpp:129.  Persists ObjPool
// next-key counter + every live UnitData.  On load, fires
// NotifyUnitSpawned after Insert so observers can see the new unit
// (mirrors binary-path slice 7a behaviour).

void to_json(nlohmann::json &j, UnitPool const &p)
{
    nlohmann::json units = nlohmann::json::array();
    for (auto i : p.m_table)
    {
        if (i)
            units.push_back(*reinterpret_cast<UnitData const *>(i));
    }
    j = nlohmann::json{
        {"next_key", const_cast<UnitPool &>(p).HackGetKey()},
        {"units",    std::move(units)},
    };
}

void from_json(nlohmann::json const &j, UnitPool &p)
{
    // Drain any pre-existing entries so reloading on top of fresh-game
    // state (LoadJson pattern) doesn't double-populate.  Mirrors
    // MessagePool's from_json drain; ObjPool::Del walks the BST root.
    for (auto & i : p.m_table)
    {
        while (i)
            p.Del(i);
    }

    p.HackSetKey(j.at("next_key").get<uint32>());

    for (auto const &entry : j.at("units"))
    {
        UnitData *data = new UnitData(entry);
        p.Insert(data);
        if (gameobservers_Get())
            gameobservers_Get()->NotifyUnitSpawned(Unit(data->m_id), data->GetState());
    }
}

// Phase E-4 — ArmyPool
//
// Mirrors ArmyPool::Serialize at ArmyPool.cpp:67.  Stores
// m_nObjs (next-key counter, accessed via HackGetKey/HackSetKey)
// + array of every live ArmyData entry.  m_id_type is invariant
// (set by ArmyPool ctor) so it's not persisted.

void to_json(nlohmann::json &j, ArmyPool const &p)
{
    nlohmann::json armies = nlohmann::json::array();
    for (auto i : p.m_table)
    {
        if (i)
            armies.push_back(*reinterpret_cast<ArmyData const *>(i));
    }
    j = nlohmann::json{
        {"next_key", const_cast<ArmyPool &>(p).HackGetKey()},
        {"armies",   std::move(armies)},
    };
}

void from_json(nlohmann::json const &j, ArmyPool &p)
{
    // Drain pre-existing entries (see UnitPool::from_json for rationale).
    for (auto & i : p.m_table)
    {
        while (i)
            p.Del(i);
    }

    // Restore next-key counter so subsequent NewKey() calls continue
    // from where the saving game left off.
    p.HackSetKey(j.at("next_key").get<uint32>());

    for (auto const &entry : j.at("armies"))
    {
        ArmyData *data = new ArmyData(Army(0));
        entry.get_to(*data);
        p.Insert(data);
    }
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
    if (!c.m_distanceToGood.empty() && g_theResourceDB)
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
        {"name",                             utf8_safe(c.m_name)},
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
        c.m_distanceToGood.assign(g_theResourceDB->NumRecords(), 0);
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
// INCLUDED as ID arrays (Phase 1j fix, 2026-06-03):
//   - m_all_armies, m_all_cities, m_all_units, m_traderUnits
//     serialised as flat arrays of uint32 GameObj IDs. ArmyPool /
//     UnitPool is restored *before* Player::from_json (see LoadJson
//     order), so the IDs resolve. Without these arrays the AI
//     subsystems (MapAnalysis::BeginTurn, etc.) iterate the
//     fresh-init Player lists and dereference stale IDs that
//     have no backing ArmyData/UnitData post-load.
//
// OMITTED with reason (Phase F pool work):
//   (Phase 0.B completed: m_vision via Vision bridge; m_tradeOffers /
//    m_terrainImprovements / m_messages / m_requests / m_agreed /
//    m_allInstallations / m_allRadarInstallations are emitted as
//    pool-handle ID arrays via ids_from_handles, restored after pool
//    drain via load_handles.)
//   - m_capitol (Unit*) — serialised as Unit ID (already bridgeable
//     via ID base).
//
// INCLUDED via existing bridges:
//   - m_science (Science), m_tax_rate (TaxRate), m_advances (Advances),
//     m_global_happiness (Happy), m_readiness (MilitaryReadiness),
//     m_regard (Regard), m_strengths (Strengths), m_gold (Gold),
//     m_difficulty (Difficulty), m_materialPool (MaterialPool),
//     m_vision (Vision).

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
    for (unsigned int i : p.m_pollution_history)
    {
        pollution_history.push_back(i);
    }
    for (int i : p.m_event_pollution)
    {
        event_pollution.push_back(i);
    }

    // m_goodSalePrices (variable-length, sized by ResourceDB)
    nlohmann::json good_sale_prices = nlohmann::json::array();
    if (!p.m_goodSalePrices.empty() && g_theResourceDB)
    {
        for (sint32 i = 0; i < g_theResourceDB->NumRecords(); ++i)
        {
            good_sale_prices.push_back(p.m_goodSalePrices[i]);
        }
    }

    // Per-player object-id lists. Each entry is a uint32 GameObj id;
    // pool restoration runs before Player::from_json so the ids
    // resolve back to live ArmyData / UnitData.
    auto ids_from_armies = [](DynamicArray<Army> const *a) {
        nlohmann::json arr = nlohmann::json::array();
        if (a) for (sint32 i = 0; i < a->Num(); ++i)
            arr.push_back(static_cast<uint32>(a->Access(i).m_id));
        return arr;
    };
    auto ids_from_units = [](UnitDynamicArray const *u) {
        nlohmann::json arr = nlohmann::json::array();
        if (u) for (sint32 i = 0; i < u->Num(); ++i)
            arr.push_back(static_cast<uint32>(u->Access(i).m_id));
        return arr;
    };
    auto ids_from_handles = [](auto const *arr) {
        nlohmann::json out = nlohmann::json::array();
        if (arr) for (sint32 i = 0; i < arr->Num(); ++i)
            out.push_back(static_cast<uint32>(arr->Access(i).m_id));
        return out;
    };

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
        {"gold",             p.m_gold             ? nlohmann::json(*p.m_gold)            : nlohmann::json(nullptr)},
        {"difficulty",       p.m_difficulty       ? nlohmann::json(*p.m_difficulty)      : nlohmann::json(nullptr)},
        {"material_pool",    p.m_materialPool     ? nlohmann::json(*p.m_materialPool)    : nlohmann::json(nullptr)},
        {"vision",           p.m_vision           ? nlohmann::json(*p.m_vision)          : nlohmann::json(nullptr)},
        // m_capitol via ID
        {"capitol",          p.m_capitol          ? nlohmann::json(static_cast<ID const &>(*p.m_capitol)) : nlohmann::json(nullptr)},
        // Per-player object-id lists (see comment above to_json).
        {"all_armies",       ids_from_armies(p.m_all_armies)},
        {"all_cities",       ids_from_units(p.m_all_cities)},
        {"all_units",        ids_from_units(p.m_all_units)},
        {"trader_units",     ids_from_units(p.m_traderUnits)},
        {"messages",                ids_from_handles(p.m_messages)},
        {"trade_offers",            ids_from_handles(p.m_tradeOffers)},
        {"requests",                ids_from_handles(p.m_requests)},
        {"agreed",                  ids_from_handles(p.m_agreed)},
        {"all_installations",       ids_from_handles(p.m_allInstallations)},
        {"all_radar_installations", ids_from_handles(p.m_allRadarInstallations)},
        {"terrain_improvements",    ids_from_handles(p.m_terrainImprovements)},
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
    if (!p.m_goodSalePrices.empty() && g_theResourceDB
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
    if (!j.at("gold")             .is_null() && p.m_gold)             j.at("gold")            .get_to(*p.m_gold);
    if (!j.at("difficulty")       .is_null() && p.m_difficulty)       j.at("difficulty")      .get_to(*p.m_difficulty);
    if (!j.at("material_pool")    .is_null() && p.m_materialPool)     j.at("material_pool")   .get_to(*p.m_materialPool);
    if (!j.at("vision")           .is_null() && p.m_vision)           j.at("vision")          .get_to(*p.m_vision);

    // m_capitol (Unit*) — null in JSON skips
    if (!j.at("capitol").is_null() && p.m_capitol)
    {
        ID capitol_id(0);
        j.at("capitol").get_to(capitol_id);
        *p.m_capitol = Unit(capitol_id.m_id);
    }

    // Per-player object-id lists (Phase 1j fix). Clear the fresh-init
    // entries from gameinit (they refer to throwaway ArmyData/UnitData
    // that pool-restore replaced), then rebuild from saved ids. Pools
    // were restored earlier in LoadJson so the ids resolve.
    auto load_army_ids = [&](char const *key, DynamicArray<Army> *dst) {
        if (!dst || !j.contains(key)) return;
        dst->Clear();
        auto const &arr = j.at(key);
        for (auto const &v : arr) {
            Army a(v.get<uint32>());
            dst->Insert(a);
        }
    };
    auto load_unit_ids = [&](char const *key, UnitDynamicArray *dst) {
        if (!dst || !j.contains(key)) return;
        dst->Clear();
        auto const &arr = j.at(key);
        for (auto const &v : arr) {
            Unit u(v.get<uint32>());
            dst->Insert(u);
        }
    };
    load_army_ids("all_armies",   p.m_all_armies);
    load_unit_ids("all_cities",   p.m_all_cities);
    load_unit_ids("all_units",    p.m_all_units);
    load_unit_ids("trader_units", p.m_traderUnits);

    // Per-player DynamicArray<Handle> ID arrays for pool-backed handles.
    // Handles construct from uint32. Backing pools are restored earlier
    // in LoadJson, so the IDs resolve to live data on first dereference.
    auto load_handles = [&j]<typename Arr>(char const *key, Arr *dst) {
        if (!dst || !j.contains(key)) return;
        dst->Clear();
        using HandleT = std::remove_reference_t<decltype(dst->Access(0))>;
        for (auto const &v : j.at(key)) {
            HandleT h(v.template get<uint32>());
            dst->Insert(h);
        }
    };
    load_handles("messages",                p.m_messages);
    load_handles("trade_offers",            p.m_tradeOffers);
    load_handles("requests",                p.m_requests);
    load_handles("agreed",                  p.m_agreed);
    load_handles("all_installations",       p.m_allInstallations);
    load_handles("all_radar_installations", p.m_allRadarInstallations);
    load_handles("terrain_improvements",    p.m_terrainImprovements);
}

// Phase D — Foreigner
void to_json(nlohmann::json &j, Foreigner const &f)
{
    // 2D array of regard events: outer indexed by REGARD_EVENT_TYPE
    // (excluding REGARD_EVENT_ALL — that's a derived total), inner is
    // the list of events for that type.
    nlohmann::json regard_event_list = nlohmann::json::array();
    for (const auto & type : f.m_regardEventList)
    {
        nlohmann::json events = nlohmann::json::array();
        for (auto const &ev : type)
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

// Phase F-9 — SlicSymbolData (the 14-case tagged-union heart of the
// Slic data model).  Mirrors SlicSymbolData::Serialize at
// gs/slic/SlicSymbol.cpp:947.  Each variant of the m_val union is
// dispatched on m_type and serialised under a payload key.
//
// JSON shape: every node carries a "type" string (lowercase enum
// name without the SLIC_SYM_ prefix) and the relevant payload key.
//
//   {"type": "ivar",        "int_value": 42}
//   {"type": "svar",        "string_id": 1234}        // StringId
//   {"type": "id"|"ufunc",  "segment_name": "Foo"}    // "" if NULL
//   {"type": "func",        "function_name": "Bar"}   // "" if NULL
//   {"type": "string",      "hard_string": "hi"}      // null if NULL
//   {"type": "city"|"unit"|"army", "object_id": 12345}
//   {"type": "location",    "x": 1, "y": 2, "z": 0}
//   {"type": "player",      "int_value": 3}
//   {"type": "improvement", "improvement_id": 99}
//   {"type": "struct_member"}
//   {"type": "undefined"}
//
// Types that Assert(FALSE) in binary (REGION, COMPLEX_REGION, BUILTIN,
// POP, PATH) throw nlohmann::json::other_error on to_json — they are
// documented as never persisted.
//
// ARRAY and STRUCT variants reference SlicArray / SlicStructInstance
// whose own JSON bridges land in F-11 / F-12.  Until those exist this
// bridge throws for those two variants too.

namespace {

char const *slicSymTypeName(SLIC_SYM t)
{
    switch (t)
    {
        case SLIC_SYM_IVAR:           return "ivar";
#ifdef SLIC_DOUBLES
        case SLIC_SYM_DVAR:           return "dvar";
#endif
        case SLIC_SYM_SVAR:           return "svar";
        case SLIC_SYM_ID:             return "id";
        case SLIC_SYM_FUNC:           return "func";
        case SLIC_SYM_REGION:         return "region";
        case SLIC_SYM_COMPLEX_REGION: return "complex_region";
        case SLIC_SYM_STRING:         return "string";
        case SLIC_SYM_CITY:           return "city";
        case SLIC_SYM_UNIT:           return "unit";
        case SLIC_SYM_ARMY:           return "army";
        case SLIC_SYM_LOCATION:       return "location";
        case SLIC_SYM_ARRAY:          return "array";
        case SLIC_SYM_BUILTIN:        return "builtin";
        case SLIC_SYM_STRUCT:         return "struct";
        case SLIC_SYM_STRUCT_MEMBER:  return "struct_member";
        case SLIC_SYM_PLAYER:         return "player";
        case SLIC_SYM_UFUNC:          return "ufunc";
        case SLIC_SYM_POP:            return "pop";
        case SLIC_SYM_PATH:           return "path";
        case SLIC_SYM_IMPROVEMENT:    return "improvement";
        case SLIC_SYM_UNDEFINED:      return "undefined";
    }
    return "undefined";
}

SLIC_SYM slicSymTypeFromName(std::string const &s)
{
    if (s == "ivar")           return SLIC_SYM_IVAR;
#ifdef SLIC_DOUBLES
    if (s == "dvar")           return SLIC_SYM_DVAR;
#endif
    if (s == "svar")           return SLIC_SYM_SVAR;
    if (s == "id")             return SLIC_SYM_ID;
    if (s == "func")           return SLIC_SYM_FUNC;
    if (s == "region")         return SLIC_SYM_REGION;
    if (s == "complex_region") return SLIC_SYM_COMPLEX_REGION;
    if (s == "string")         return SLIC_SYM_STRING;
    if (s == "city")           return SLIC_SYM_CITY;
    if (s == "unit")           return SLIC_SYM_UNIT;
    if (s == "army")           return SLIC_SYM_ARMY;
    if (s == "location")       return SLIC_SYM_LOCATION;
    if (s == "array")          return SLIC_SYM_ARRAY;
    if (s == "builtin")        return SLIC_SYM_BUILTIN;
    if (s == "struct")         return SLIC_SYM_STRUCT;
    if (s == "struct_member")  return SLIC_SYM_STRUCT_MEMBER;
    if (s == "player")         return SLIC_SYM_PLAYER;
    if (s == "ufunc")          return SLIC_SYM_UFUNC;
    if (s == "pop")            return SLIC_SYM_POP;
    if (s == "path")           return SLIC_SYM_PATH;
    if (s == "improvement")    return SLIC_SYM_IMPROVEMENT;
    if (s == "undefined")      return SLIC_SYM_UNDEFINED;
    throw nlohmann::json::other_error::create(
        501, "SlicSymbolData: unknown type '" + s + "'", nullptr);
}

[[noreturn]] void throwUnsupportedSym(SLIC_SYM t, char const *direction)
{
    throw nlohmann::json::other_error::create(
        501,
        std::string("SlicSymbolData: ") + direction + " not supported for type '"
            + slicSymTypeName(t) + "' (Assert(FALSE) in binary path or pending nested bridge)",
        nullptr);
}

}  // namespace

void to_json(nlohmann::json &j, SlicSymbolData const &s)
{
    j = nlohmann::json::object();
    j["type"] = slicSymTypeName(s.m_type);

    switch (s.m_type)
    {
        case SLIC_SYM_IVAR:
        case SLIC_SYM_PLAYER:
            j["int_value"] = s.m_val.m_int_value;
            break;
#ifdef SLIC_DOUBLES
        case SLIC_SYM_DVAR:
            j["double_value"] = s.m_val.m_double_value;
            break;
#endif
        case SLIC_SYM_SVAR:
            j["string_id"] = s.m_val.m_string_value;
            break;
        case SLIC_SYM_CITY:
            j["object_id"] = s.m_val.m_city_id;
            break;
        case SLIC_SYM_UNIT:
            j["object_id"] = s.m_val.m_unit_id;
            break;
        case SLIC_SYM_ARMY:
            j["object_id"] = s.m_val.m_army_id;
            break;
        case SLIC_SYM_LOCATION:
            j["x"] = s.m_val.m_location.x;
            j["y"] = s.m_val.m_location.y;
            j["z"] = s.m_val.m_location.z;
            break;
        case SLIC_SYM_FUNC:
            j["function_name"] = s.m_val.m_function_object
                                     ? safeName(s.m_val.m_function_object->GetName())
                                     : std::string();
            break;
        case SLIC_SYM_STRING:
            j["hard_string"] = optStringToJson(s.m_val.m_hard_string);
            break;
        case SLIC_SYM_UFUNC:
        case SLIC_SYM_ID:
            j["segment_name"] = s.m_val.m_segment
                                    ? safeName(s.m_val.m_segment->GetName())
                                    : std::string();
            break;
        case SLIC_SYM_IMPROVEMENT:
            // Not persisted by binary Serialize (the case is missing
            // from its switch).  Keep round-trip parity in JSON for
            // diagnostics; semantically the field is a unique-object
            // id like CITY/UNIT/ARMY.
            j["improvement_id"] = s.m_val.m_improvement_id;
            break;
        case SLIC_SYM_STRUCT_MEMBER:
        case SLIC_SYM_UNDEFINED:
            // No payload — binary writes nothing for these.
            break;
        case SLIC_SYM_ARRAY:
            // F-11: SlicArray bridge wires in here.  Defined below
            // (the friend declarations in SlicArray.h pick it up).
            j["array"] = *s.m_val.m_array;
            break;
        case SLIC_SYM_STRUCT:
            // F-12: SlicStructInstance bridge wires in here.
            j["struct"] = *s.m_val.m_struct;
            break;
        case SLIC_SYM_REGION:
        case SLIC_SYM_COMPLEX_REGION:
        case SLIC_SYM_BUILTIN:
        case SLIC_SYM_POP:
        case SLIC_SYM_PATH:
            throwUnsupportedSym(s.m_type, "to_json");
    }
}

void from_json(nlohmann::json const &j, SlicSymbolData &s)
{
    SLIC_SYM const type = slicSymTypeFromName(j.at("type").get<std::string>());
    s.m_type = type;

    // Clear the union to a known state before populating the active
    // variant.  This matches the binary path's behaviour for new
    // SlicSymbolData instances and avoids leaking owned pointers from
    // the previous m_type when from_json is called on a reused object.
    std::memset(&s.m_val, 0, sizeof(s.m_val));

    switch (type)
    {
        case SLIC_SYM_IVAR:
        case SLIC_SYM_PLAYER:
            j.at("int_value").get_to(s.m_val.m_int_value);
            break;
#ifdef SLIC_DOUBLES
        case SLIC_SYM_DVAR:
            j.at("double_value").get_to(s.m_val.m_double_value);
            break;
#endif
        case SLIC_SYM_SVAR:
            j.at("string_id").get_to(s.m_val.m_string_value);
            break;
        case SLIC_SYM_CITY:
            j.at("object_id").get_to(s.m_val.m_city_id);
            break;
        case SLIC_SYM_UNIT:
            j.at("object_id").get_to(s.m_val.m_unit_id);
            break;
        case SLIC_SYM_ARMY:
            j.at("object_id").get_to(s.m_val.m_army_id);
            break;
        case SLIC_SYM_LOCATION:
            j.at("x").get_to(s.m_val.m_location.x);
            j.at("y").get_to(s.m_val.m_location.y);
            j.at("z").get_to(s.m_val.m_location.z);
            break;
        case SLIC_SYM_FUNC:
        {
            std::string name = j.at("function_name").get<std::string>();
            s.m_val.m_function_object =
                (slicengine_Get() && !name.empty())
                    ? slicengine_Get()->GetFunction(name.c_str())
                    : nullptr;
            break;
        }
        case SLIC_SYM_STRING:
            // optStringToJson stored either a string or null; reuse
            // the helper to mint a fresh char[] (m_val was zeroed
            // above, so the helper's `delete[]` is a no-op).
            jsonToOptString(j.at("hard_string"), s.m_val.m_hard_string);
            break;
        case SLIC_SYM_UFUNC:
        case SLIC_SYM_ID:
        {
            std::string name = j.at("segment_name").get<std::string>();
            s.m_val.m_segment = (slicengine_Get() && !name.empty())
                                    ? slicengine_Get()->GetSegment(name.c_str())
                                    : nullptr;
            break;
        }
        case SLIC_SYM_IMPROVEMENT:
            j.at("improvement_id").get_to(s.m_val.m_improvement_id);
            break;
        case SLIC_SYM_STRUCT_MEMBER:
        case SLIC_SYM_UNDEFINED:
            break;
        case SLIC_SYM_ARRAY:
            // m_val.m_array takes ownership; ctor args are placeholders
            // (overwritten by from_json from the JSON's "type"/"var_type").
            s.m_val.m_array = new SlicArray(SS_TYPE_BAD, SLIC_SYM_UNDEFINED);
            j.at("array").get_to(*s.m_val.m_array);
            break;
        case SLIC_SYM_STRUCT:
        {
            // SlicStructInstance has no default ctor — it needs a
            // description.  Resolve via slicengine_Get() using the
            // builtin-tag stored in the nested struct JSON.  Without
            // an engine (test-only path) the symbol falls back to
            // UNDEFINED rather than crashing — full struct round-
            // trips require a live SlicEngine fixture.
            nlohmann::json const &js = j.at("struct");
            SlicStructDescription *desc = nullptr;
            if (slicengine_Get() && js.contains("description")
                && !js.at("description").is_null())
            {
                desc = slicengine_Get()->GetStructDescription(
                    static_cast<SLIC_BUILTIN>(js.at("description").get<int>()));
            }
            if (!desc)
            {
                s.m_type = SLIC_SYM_UNDEFINED;
                break;
            }
            s.m_val.m_struct = new SlicStructInstance(desc);
            js.get_to(*s.m_val.m_struct);
            break;
        }
        case SLIC_SYM_REGION:
        case SLIC_SYM_COMPLEX_REGION:
        case SLIC_SYM_BUILTIN:
        case SLIC_SYM_POP:
        case SLIC_SYM_PATH:
            throwUnsupportedSym(type, "from_json");
    }
}

// Phase F-10 — SlicNamedSymbol / SlicParameterSymbol /
// SlicBuiltinNamedSymbol.  Subclass extensions on SlicSymbolData
// that the binary path post-dispatches via GetSerializeType() at
// SlicSymbol.cpp:1113-1132.  Each adds a flat list of fields after
// the base payload:
//
//   SlicNamedSymbol         + name, index, from_file
//   SlicParameterSymbol     +   parameter_index
//   SlicBuiltinNamedSymbol  +   builtin
//
// JSON adds a "serial_type" discriminator (named / parameter /
// builtin) so the polymorphic factory equivalent of slicsymbol_Load
// (lands with SlicSymTab later) can pick the right concrete type.

void to_json(nlohmann::json &j, SlicNamedSymbol const &s)
{
    to_json(j, static_cast<SlicSymbolData const &>(s));
    j["serial_type"] = "named";
    j["name"]        = s.m_name;
    j["index"]       = s.m_index;
    j["from_file"]   = s.m_fromFile;
}

void from_json(nlohmann::json const &j, SlicNamedSymbol &s)
{
    from_json(j, static_cast<SlicSymbolData &>(s));

    j.at("name").get_to(s.m_name);
    j.at("index").get_to(s.m_index);
    j.at("from_file").get_to(s.m_fromFile);
}

void to_json(nlohmann::json &j, SlicParameterSymbol const &s)
{
    to_json(j, static_cast<SlicNamedSymbol const &>(s));
    j["serial_type"]     = "parameter";
    j["parameter_index"] = s.m_parameterIndex;
}

void from_json(nlohmann::json const &j, SlicParameterSymbol &s)
{
    from_json(j, static_cast<SlicNamedSymbol &>(s));
    j.at("parameter_index").get_to(s.m_parameterIndex);
}

void to_json(nlohmann::json &j, SlicBuiltinNamedSymbol const &s)
{
    to_json(j, static_cast<SlicNamedSymbol const &>(s));
    j["serial_type"] = "builtin";
    j["builtin"]     = static_cast<int>(s.m_builtin);
}

void from_json(nlohmann::json const &j, SlicBuiltinNamedSymbol &s)
{
    from_json(j, static_cast<SlicNamedSymbol &>(s));
    s.m_builtin = static_cast<SLIC_BUILTIN>(j.at("builtin").get<int>());
}

// Phase F-11 — SlicArray (and the polymorphic SlicSymbolData factory
// used by both SlicArray's SYM cells and, later, SlicSymTab).
//
// Binary impl: SlicArray::Serialize at SlicArray.cpp:113 and
// slicsymbol_Load at SlicSymbol.cpp:1135.

namespace {

// Polymorphic to_json companion to loadSlicSymbolFromJson.  Dispatches
// on GetSerializeType() so a SlicSymbolData* whose dynamic type is one
// of the F-10 subclasses serialises through the subclass's bridge.
// nlohmann's ADL alone picks the static type, which would silently
// drop subclass fields.
nlohmann::json storeSlicSymbolToJson(SlicSymbolData *sym)
{
    if (!sym)
        return nullptr;
    switch (sym->GetSerializeType())
    {
        case SLIC_SYM_SERIAL_NAMED:
            return *static_cast<SlicNamedSymbol *>(sym);
        case SLIC_SYM_SERIAL_PARAMETER:
            return *static_cast<SlicParameterSymbol *>(sym);
        case SLIC_SYM_SERIAL_BUILTIN:
            return *static_cast<SlicBuiltinNamedSymbol *>(sym);
        case SLIC_SYM_SERIAL_MEMBER:
            // SlicStructMemberData — F-12.  Binary writes nothing for it
            // (Serialize is a no-op); fall through to generic for now.
        case SLIC_SYM_SERIAL_GENERIC:
        default:
            return *sym;
    }
}

// Mirrors slicsymbol_Load.  Reads "serial_type" from j and news the
// matching concrete subclass, then populates it via from_json.  Returns
// nullptr when j is JSON null.  Caller owns the result.
SlicSymbolData *loadSlicSymbolFromJson(nlohmann::json const &j)
{
    if (j.is_null())
        return nullptr;

    std::string const serial = j.value("serial_type", std::string("generic"));

    if (serial == "named")
    {
        auto *p = new SlicNamedSymbol;
        from_json(j, *p);
        return p;
    }
    if (serial == "parameter")
    {
        auto *p = new SlicParameterSymbol;
        from_json(j, *p);
        return p;
    }
    if (serial == "builtin")
    {
        auto *p = new SlicBuiltinNamedSymbol;
        from_json(j, *p);
        return p;
    }
    // "generic" or missing — base SlicSymbolData.
    auto *p = new SlicSymbolData;
    from_json(j, *p);
    return p;
}

char const *ssTypeName(SS_TYPE t)
{
    switch (t)
    {
        case SS_TYPE_INT: return "int";
        case SS_TYPE_VAR: return "var";
        case SS_TYPE_SYM: return "sym";
        case SS_TYPE_BAD: return "bad";
    }
    return "bad";
}

SS_TYPE ssTypeFromName(std::string const &s)
{
    if (s == "int") return SS_TYPE_INT;
    if (s == "var") return SS_TYPE_VAR;
    if (s == "sym") return SS_TYPE_SYM;
    if (s == "bad") return SS_TYPE_BAD;
    throw nlohmann::json::other_error::create(
        501, "SlicArray: unknown SS_TYPE '" + s + "'", nullptr);
}

}  // namespace

void to_json(nlohmann::json &j, SlicArray const &a)
{
    j = nlohmann::json{
        {"type",           ssTypeName(a.m_type)},
        {"var_type",       slicSymTypeName(a.m_varType)},
        {"allocated_size", a.m_allocatedSize},
        {"size_is_fixed",  a.m_sizeIsFixed},
    };

    if (a.m_varType == SLIC_SYM_STRUCT)
    {
        // Stored as the SLIC_BUILTIN enum integer to match the binary
        // path; resolved on load via slicengine_Get()->GetStructDescription.
        j["struct_template"] = a.m_structTemplate
            ? static_cast<int>(a.m_structTemplate->GetType())
            : -1;
    }

    nlohmann::json elements = nlohmann::json::array();
    if (a.m_type == SS_TYPE_INT)
    {
        for (sint32 i = 0; i < a.m_arraySize; ++i)
            elements.push_back(a.m_array[i].m_int);
    }
    else  // SS_TYPE_SYM (or VAR / BAD — preserve structure even if unused)
    {
        for (sint32 i = 0; i < a.m_arraySize; ++i)
            elements.push_back(storeSlicSymbolToJson(a.m_array[i].m_sym));
    }
    j["elements"] = std::move(elements);
}

void from_json(nlohmann::json const &j, SlicArray &a)
{
    // Drop any existing storage; matches binary load which discards
    // pre-existing m_array (the (CivArchive&) ctor allocates fresh).
    if (a.m_type == SS_TYPE_SYM)
    {
        for (size_t i = 0; i < a.m_allocatedSize; ++i)
            delete a.m_array[i].m_sym;
    }
    a.m_array.reset();

    a.m_type     = ssTypeFromName(j.at("type").get<std::string>());
    a.m_varType  = slicSymTypeFromName(j.at("var_type").get<std::string>());
    j.at("allocated_size").get_to(a.m_allocatedSize);
    j.at("size_is_fixed").get_to(a.m_sizeIsFixed);

    if (a.m_varType == SLIC_SYM_STRUCT)
    {
        SLIC_BUILTIN const which =
            static_cast<SLIC_BUILTIN>(j.value("struct_template", -1));
        a.m_structTemplate = (slicengine_Get() && static_cast<int>(which) >= 0)
                                 ? slicengine_Get()->GetStructDescription(which)
                                 : nullptr;
    }
    else
    {
        a.m_structTemplate = nullptr;
    }

    auto const &elements = j.at("elements");
    a.m_arraySize = static_cast<sint32>(elements.size());

    // allocated_size must cover arraySize; widen if the JSON was hand-
    // edited to a smaller capacity than its element list.
    if (a.m_allocatedSize < static_cast<uint32>(a.m_arraySize))
        a.m_allocatedSize = static_cast<uint32>(a.m_arraySize);
    if (a.m_allocatedSize == 0)
        a.m_allocatedSize = 1;  // matches k_DEFAULT_SLICARRAY_SIZE

    a.m_array = std::make_unique<SlicStackValue[]>(a.m_allocatedSize);
    std::memset(a.m_array.get(), 0, a.m_allocatedSize * sizeof(SlicStackValue));

    if (a.m_type == SS_TYPE_INT)
    {
        for (sint32 i = 0; i < a.m_arraySize; ++i)
            elements[i].get_to(a.m_array[i].m_int);
    }
    else
    {
        for (sint32 i = 0; i < a.m_arraySize; ++i)
            a.m_array[i].m_sym = loadSlicSymbolFromJson(elements[i]);
    }
}

// Phase F-12 — SlicStructInstance (the per-instance state for a
// SLIC_SYM_STRUCT symbol).  Mirrors SlicStructInstance::Serialize at
// SlicStruct.cpp:413.
//
// SlicStructDescription itself isn't persisted — it's compile-time
// metadata reconstructed by slicengine_Get() at startup (slicstruct_Init
// equivalent).  The bridge stores the description's SLIC_BUILTIN tag
// and re-resolves via slicengine_Get()->GetStructDescription on load.
//
// JSON shape:
//   {
//     "description":       <SLIC_BUILTIN int> | null,
//     "members": [ <SlicSymbolData json or null>, ... ],   // members only,
//                                                          // not accessors
//     "data_symbol":       <SlicSymbolData json or null>,
//     "created_data":      true | false,
//     "data_symbol_index": <sint32>     // INDEX_INVALID (-1) by default
//   }

void to_json(nlohmann::json &j, SlicStructInstance const &s)
{
    j = nlohmann::json::object();
    j["description"] = s.m_description
        ? static_cast<int>(s.m_description->GetType())
        : -1;

    nlohmann::json members = nlohmann::json::array();
    sint32 const numMembers = s.m_description ? s.m_description->GetNumMembers() : 0;
    for (sint32 i = 0; i < numMembers; ++i)
        members.push_back(storeSlicSymbolToJson(s.m_members[i]));
    j["members"] = std::move(members);

    j["created_data"]      = s.m_createdData;
    j["data_symbol_index"] = s.m_dataSymbolIndex;

    // Binary stores the data symbol's full state only when m_createdData
    // is true; otherwise it stores just the symbol's index (m_dataSymbol
    // refers to a shared symbol owned elsewhere — typically the symtab).
    if (s.m_dataSymbol && s.m_createdData)
        j["data_symbol"] = storeSlicSymbolToJson(s.m_dataSymbol);
    else
        j["data_symbol"] = nullptr;
}

void from_json(nlohmann::json const &j, SlicStructInstance &s)
{
    // Description was set by the constructor before we got here; the
    // caller (SlicSymbolData::from_json STRUCT case) is responsible
    // for resolving it via slicengine_Get().  Reset member/data slots.
    for (size_t i = 0; i < s.m_validIndexCount; ++i)
    {
        delete s.m_members[i];
        s.m_members[i] = nullptr;
    }
    if (s.m_createdData)
        delete s.m_dataSymbol;
    s.m_dataSymbol = nullptr;

    sint32 const numMembers = s.m_description ? s.m_description->GetNumMembers() : 0;
    auto const &members = j.at("members");
    for (sint32 i = 0; i < numMembers && i < static_cast<sint32>(members.size()); ++i)
    {
        if (members[i].is_null())
            continue;
        s.CreateMember(i);  // allocates m_members[i] as SlicStructMemberData
        // SlicStructMemberData inherits SlicSymbolData; populate only
        // the base state — SERIAL_MEMBER's own Serialize is a no-op.
        from_json(members[i], static_cast<SlicSymbolData &>(*s.m_members[i]));
        s.m_members[i]->SetParent(&s);
    }

    j.at("created_data").get_to(s.m_createdData);
    j.at("data_symbol_index").get_to(s.m_dataSymbolIndex);

    if (s.m_createdData && !j.at("data_symbol").is_null())
    {
        s.m_dataSymbol = loadSlicSymbolFromJson(j.at("data_symbol"));
    }
    else if (!s.m_dataSymbol && s.m_dataSymbolIndex < 0)
    {
        // Mirror the binary path's fallback: create a fresh data symbol
        // when neither persisted form is available.
        s.m_dataSymbol  = s.m_description->CreateDataSymbol();
        s.m_createdData = true;
    }
}

// Phase F-15 — SlicContext (the per-event context object carrying
// the lists of game-state references slic scripts can address as
// "city[0]", "unit[1]", etc.).  Mirrors SlicContext::Serialize at
// SlicContext.cpp:326.  GameEventArgList (m_eventArgs) is transient
// (per-event call frame) and not persisted — same as binary.
//
// JSON shape: one key per list, snake_case, with the count implicit
// from array length.  Null SimpleDynamicArray pointers serialise as
// JSON null (binary writes a 0 hasList flag); empty arrays serialise
// as []. Raw sint32* lists with a separate count serialise as plain
// arrays.

namespace {

template <class T>
nlohmann::json sdaToJson(SimpleDynamicArray<T> const *list)
{
    if (!list)
        return nullptr;
    nlohmann::json out = nlohmann::json::array();
    for (sint32 i = 0; i < list->Num(); ++i)
        out.push_back((*list)[i]);
    return out;
}

template <class T>
SimpleDynamicArray<T> *jsonToSda(nlohmann::json const &j)
{
    if (j.is_null())
        return nullptr;
    auto *out = new SimpleDynamicArray<T>;
    for (auto const &el : j)
    {
        T value;
        el.get_to(value);
        out->Insert(value);
    }
    return out;
}

template <class T>
void rawToVec(nlohmann::json &j, T const *list, sint32 count)
{
    j = nlohmann::json::array();
    for (sint32 i = 0; i < count; ++i)
        j.push_back(list[i]);
}

template <class T>
void jsonToRaw(nlohmann::json const &j, T *&list, sint32 &count)
{
    delete[] list;
    list = nullptr;
    auto v = j.get<std::vector<T>>();
    count = static_cast<sint32>(v.size());
    if (count > 0)
    {
        list = new T[count];
        std::copy(v.begin(), v.end(), list);
    }
}

}  // namespace

void to_json(nlohmann::json &j, SlicContext const &c)
{
    j = nlohmann::json::object();
    j["cities"]       = sdaToJson(c.m_cityList);
    j["units"]        = sdaToJson(c.m_unitList);
    j["armies"]       = sdaToJson(c.m_armyList);
    j["players"]      = sdaToJson(c.m_playerList);
    j["ints"]         = sdaToJson(c.m_intList);
    j["unit_records"] = sdaToJson(c.m_unitRecordList);
    j["locations"]    = sdaToJson(c.m_locationList);
    j["agreements"]   = sdaToJson(c.m_agreementList);
    j["trade_offers"] = sdaToJson(c.m_tradeOffersList);
    j["goods"]        = sdaToJson(c.m_goodList);
    j["governments"]  = sdaToJson(c.m_governmentList);
    j["advances"]     = sdaToJson(c.m_advanceList);

    rawToVec(j["calamities"],     c.m_calamityList,     c.m_numCalamities);
    rawToVec(j["golds"],          c.m_goldList,         c.m_numGolds);
    rawToVec(j["ranks"],          c.m_rankList,         c.m_numRanks);
    rawToVec(j["wonders"],        c.m_wonderList,       c.m_numWonders);
    rawToVec(j["orders"],         c.m_orderList,        c.m_numOrders);
    rawToVec(j["madlib_choices"], c.m_madlibChoiceList, c.m_numMadlibs);
    rawToVec(j["madlib_names"],   c.m_madlibNameList,   c.m_numMadlibs);
    rawToVec(j["attitudes"],      c.m_attitudeList,     c.m_numAttitudes);
    rawToVec(j["ages"],           c.m_ageList,          c.m_numAges);
    rawToVec(j["buildings"],      c.m_buildingList,     c.m_numBuildings);
    rawToVec(j["trade_bids"],     c.m_tradeBidList,     c.m_numTradeBids);

    nlohmann::json actions = nlohmann::json::array();
    for (auto const &action : c.m_actionList)
        actions.push_back(action);
    j["actions"] = std::move(actions);
}

void from_json(nlohmann::json const &j, SlicContext &c)
{
    // Free existing storage — SlicContext::~SlicContext is dtor-only
    // (no Reset).  Mimic it inline.
    delete c.m_cityList;       c.m_cityList = nullptr;
    delete c.m_unitList;       c.m_unitList = nullptr;
    delete c.m_armyList;       c.m_armyList = nullptr;
    delete c.m_playerList;     c.m_playerList = nullptr;
    delete c.m_intList;        c.m_intList = nullptr;
    delete c.m_unitRecordList; c.m_unitRecordList = nullptr;
    delete c.m_locationList;   c.m_locationList = nullptr;
    delete c.m_agreementList;  c.m_agreementList = nullptr;
    delete c.m_tradeOffersList;c.m_tradeOffersList = nullptr;
    delete c.m_goodList;       c.m_goodList = nullptr;
    delete c.m_governmentList; c.m_governmentList = nullptr;
    delete c.m_advanceList;    c.m_advanceList = nullptr;
    c.m_actionList.clear();

    c.m_cityList        = jsonToSda<Unit>(j.at("cities"));
    c.m_unitList        = jsonToSda<Unit>(j.at("units"));
    c.m_armyList        = jsonToSda<Army>(j.at("armies"));
    c.m_playerList      = jsonToSda<sint32>(j.at("players"));
    c.m_intList         = jsonToSda<sint32>(j.at("ints"));
    c.m_unitRecordList  = jsonToSda<sint32>(j.at("unit_records"));
    c.m_locationList    = jsonToSda<MapPoint>(j.at("locations"));
    c.m_agreementList   = jsonToSda<ai::Agreement>(j.at("agreements"));
    c.m_tradeOffersList = jsonToSda<TradeOffer>(j.at("trade_offers"));
    c.m_goodList        = jsonToSda<sint32>(j.at("goods"));
    c.m_governmentList  = jsonToSda<sint32>(j.at("governments"));
    c.m_advanceList     = jsonToSda<sint32>(j.at("advances"));

    jsonToRaw(j.at("calamities"),     c.m_calamityList,     c.m_numCalamities);
    jsonToRaw(j.at("golds"),          c.m_goldList,         c.m_numGolds);
    jsonToRaw(j.at("ranks"),          c.m_rankList,         c.m_numRanks);
    jsonToRaw(j.at("wonders"),        c.m_wonderList,       c.m_numWonders);
    jsonToRaw(j.at("orders"),         c.m_orderList,        c.m_numOrders);
    // madlib_choices and madlib_names share a single count; load both
    // from the choices array's size for safety.
    jsonToRaw(j.at("madlib_choices"), c.m_madlibChoiceList, c.m_numMadlibs);
    {
        sint32 namesCount = 0;
        jsonToRaw(j.at("madlib_names"), c.m_madlibNameList, namesCount);
        // namesCount is overwritten — keep m_numMadlibs from choices.
    }
    jsonToRaw(j.at("attitudes"),      c.m_attitudeList,     c.m_numAttitudes);
    jsonToRaw(j.at("ages"),           c.m_ageList,          c.m_numAges);
    jsonToRaw(j.at("buildings"),      c.m_buildingList,     c.m_numBuildings);
    jsonToRaw(j.at("trade_bids"),     c.m_tradeBidList,     c.m_numTradeBids);

    auto const &actions = j.at("actions");
    c.m_actionList.reserve(actions.size());
    for (auto const &action : actions)
        c.m_actionList.push_back(action.get<std::string>());
}

// Phase F-15b — SlicObject (extends SlicContext with message/event
// metadata).  Mirrors SlicObject::Serialize at slicobject.cpp:541.
//
// Adds to the base SlicContext payload: id, segment (by name),
// numRecipients + recipientList, seconds, the nine sint32 flags
// (default_advance_set/default_advance/aborted/instant_message/
// class/dont_save/close_disabled/is_diplomatic_response/
// use_director), plus the m_request ID.  Transient fields
// (m_refCount, m_frame, m_index, m_argList, m_result) are not
// persisted — matches binary.

void to_json(nlohmann::json &j, SlicObject const &o)
{
    // Embed the base context under a nested key so SlicObject's own
    // fields live at the top level.
    to_json(j, static_cast<SlicContext const &>(o));

    j["kind"]    = "slic_object";
    j["id"]      = o.m_id;
    j["seconds"] = o.m_seconds;

    nlohmann::json recipients = nlohmann::json::array();
    for (sint32 i = 0; i < o.m_numRecipients; ++i)
        recipients.push_back(o.m_recipientList[i]);
    j["recipients"] = std::move(recipients);

    j["segment_name"] = o.m_segment
        ? safeName(o.m_segment->GetName())
        : std::string();

    j["default_advance_set"]     = o.m_defaultAdvanceSet;
    j["default_advance"]         = o.m_defaultAdvance;
    j["aborted"]                 = o.m_aborted;
    j["instant_message"]         = o.m_instantMessage;
    j["class"]                   = o.m_class;
    j["dont_save"]               = o.m_dontSave;
    j["close_disabled"]          = o.m_closeDisabled;
    j["is_diplomatic_response"]  = o.m_isDiplomaticResponse;
    j["use_director"]            = o.m_useDirector;

    j["request"] = o.m_request ? static_cast<uint32>(o.m_request->m_id) : 0u;
}

void from_json(nlohmann::json const &j, SlicObject &o)
{
    from_json(j, static_cast<SlicContext &>(o));

    o.m_refCount = 0;

    o.m_id = j.at("id").get<std::string>();

    j.at("seconds").get_to(o.m_seconds);

    auto recipients = j.at("recipients").get<std::vector<sint32>>();
    delete[] o.m_recipientList;
    o.m_numRecipients = static_cast<sint32>(recipients.size());
    o.m_recipientList = o.m_numRecipients > 0
                           ? new sint32[o.m_numRecipients]
                           : nullptr;
    for (sint32 i = 0; i < o.m_numRecipients; ++i)
        o.m_recipientList[i] = recipients[i];

    std::string segName = j.at("segment_name").get<std::string>();
    o.m_segment = (slicengine_Get() && !segName.empty())
                      ? slicengine_Get()->GetSegment(segName.c_str())
                      : nullptr;
    // m_frame is recreated from m_segment by the binary path; do the
    // same here when possible.
    delete o.m_frame;
    o.m_frame = o.m_segment ? new SlicFrame(o.m_segment) : nullptr;

    j.at("default_advance_set").get_to(o.m_defaultAdvanceSet);
    j.at("default_advance").get_to(o.m_defaultAdvance);
    j.at("aborted").get_to(o.m_aborted);
    j.at("instant_message").get_to(o.m_instantMessage);
    j.at("class").get_to(o.m_class);
    j.at("dont_save").get_to(o.m_dontSave);
    j.at("close_disabled").get_to(o.m_closeDisabled);
    j.at("is_diplomatic_response").get_to(o.m_isDiplomaticResponse);
    j.at("use_director").get_to(o.m_useDirector);

    if (o.m_request)
        o.m_request->m_id = j.value("request", 0u);
}

// Phase F-16 — SlicEngine (top-level slic state composer).  Mirrors
// SlicEngine::Serialize at SlicEngine.cpp:359.
//
// Composes everything F-9..F-15 added.  JSON shape:
//   {
//     "tutorial_player":         <sint32>,
//     "tutorial_active":         <bool>,
//     "segments":                [<SlicSegment>, ...],   // segment hash
//     "constants":               [<SlicConst>, ...],     // const hash
//     "sym_tab":                 <SlicSymTab>,
//     "records":                 [{"player": N, "entries": [<SlicRecord>, ...]}],
//     "timer":                   [sint32, ...k_NUM_TIMERS],
//     "trigger_key":             [int, ...k_MAX_TRIGGER_KEYS],   // MBCHAR cast to int
//     "do_research_on_unblank":  <bool>,
//     "research_owner":          <sint32>,
//     "research_text":           "...",                  // up to 256 chars
//     "current_message":         <uint32 ID>,            // Message handle
//     "disabled_classes":        [sint32, ...]
//   }
//
// Not persisted (matches binary): m_functionHash (rebuilt by
// AddBuiltinFunctions), m_modFunc (rebuilt), m_uiHash (rebuilt by
// segment LinkTriggerSymbols), runtime caches.

// Phase F-14 — SlicSegment (compiled SLIC script segment).  Mirrors
// SlicSegment::Serialize at SlicSegment.cpp:401.  Carries the
// bytecode (m_code), per-player cooldown timers (m_lastShown),
// trigger/parameter indices, and the source filename/UI binding.
//
// JSON shape mirrors the field list one-for-one.  Bytecode is an
// array of uint8 (modders won't hand-edit it but it's necessary for
// scripts that mutate state then save mid-execution).
//
// trigger_symbols / parameter_symbols themselves aren't persisted —
// they're runtime pointers resolved post-load via LinkTriggerSymbols
// / LinkParameterSymbols from the indices.  Same as the binary path.

namespace {

std::vector<uint8> bytesToVec(uint8 const *bytes, size_t n)
{
    return bytes ? std::vector<uint8>(bytes, bytes + n) : std::vector<uint8>();
}

}  // namespace

void to_json(nlohmann::json &j, SlicSegment const &s)
{
    j = nlohmann::json{
        {"type",                static_cast<int>(s.m_type)},
        {"code_size",           s.m_codeSize},
        {"num_trigger_symbols", s.m_num_trigger_symbols},
        {"num_parameters",      s.m_num_parameters},
        {"enabled",             static_cast<bool>(s.m_enabled)},
        {"special_variables",   s.m_specialVariables},
        {"is_alert",            static_cast<bool>(s.m_isAlert)},
        {"is_help",             static_cast<bool>(s.m_isHelp)},
        {"event",               static_cast<int>(s.m_event)},
        {"priority",            static_cast<int>(s.m_priority)},
        {"from_file",           s.m_fromFile},
        {"id",                  s.m_id ? std::string(s.m_id) : std::string()},
        {"code",                bytesToVec(s.m_code, s.m_codeSize)},
        {"last_shown",
            std::vector<sint32>(s.m_lastShown, s.m_lastShown + k_MAX_PLAYERS)},
        {"ui_component",        optStringToJson(s.m_uiComponent)},
        {"filename",            optStringToJson(s.m_filename)},
    };

    nlohmann::json trigIdx = nlohmann::json::array();
    for (sint32 i = 0; i < s.m_num_trigger_symbols; ++i)
        trigIdx.push_back(s.m_trigger_symbols_indices[i]);
    j["trigger_symbol_indices"] = std::move(trigIdx);

    nlohmann::json paramIdx = nlohmann::json::array();
    for (sint32 i = 0; i < s.m_num_parameters; ++i)
    {
        // Binary stores GetIndex() of each linked SlicParameterSymbol;
        // when the symbols haven't been linked yet (fresh load path)
        // it falls back to the index array.
        if (s.m_parameter_symbols)
            paramIdx.push_back(
                static_cast<SlicParameterSymbol *>(s.m_parameter_symbols[i])->GetIndex());
        else if (!s.m_parameter_indices.empty())
            paramIdx.push_back(s.m_parameter_indices[i]);
        else
            paramIdx.push_back(-1);
    }
    j["parameter_indices"] = std::move(paramIdx);
}

void from_json(nlohmann::json const &j, SlicSegment &s)
{
    // Free any prior allocations (matches binary path's fresh-new ctor).
    free(s.m_id);
    free(s.m_code);
    free(s.m_uiComponent);
    free(s.m_filename);
    s.m_id = nullptr;
    s.m_code = nullptr;
    s.m_uiComponent = nullptr;
    s.m_filename = nullptr;
    s.m_trigger_symbols_indices.clear();
    s.m_parameter_indices.clear();
    s.m_trigger_symbols = nullptr;
    s.m_parameter_symbols = nullptr;

    s.m_type            = static_cast<SLIC_OBJECT>(j.at("type").get<int>());
    j.at("code_size").get_to(s.m_codeSize);
    j.at("num_trigger_symbols").get_to(s.m_num_trigger_symbols);
    j.at("num_parameters").get_to(s.m_num_parameters);
    s.m_enabled         = j.at("enabled").get<bool>();
    j.at("special_variables").get_to(s.m_specialVariables);
    s.m_isAlert         = j.at("is_alert").get<bool>();
    s.m_isHelp          = j.at("is_help").get<bool>();
    s.m_event           = static_cast<GAME_EVENT>(j.at("event").get<int>());
    s.m_priority        = static_cast<GAME_EVENT_PRIORITY>(j.at("priority").get<int>());
    j.at("from_file").get_to(s.m_fromFile);

    std::string const id = j.at("id").get<std::string>();
    s.m_id = static_cast<char *>(malloc(id.size() + 1));
    if (s.m_id) std::memcpy(s.m_id, id.c_str(), id.size() + 1);

    auto code = j.at("code").get<std::vector<uint8>>();
    if (s.m_codeSize > 0)
    {
        s.m_code = static_cast<uint8 *>(malloc(s.m_codeSize));
        if (s.m_code && !code.empty())
            std::memcpy(s.m_code, code.data(),
                        std::min<size_t>(code.size(), s.m_codeSize));
    }

    auto trigIdx = j.at("trigger_symbol_indices").get<std::vector<sint32>>();
    if (s.m_num_trigger_symbols > 0)
    {
        s.m_trigger_symbols_indices.resize(s.m_num_trigger_symbols);
        for (sint32 i = 0; i < s.m_num_trigger_symbols
                            && i < static_cast<sint32>(trigIdx.size()); ++i)
            s.m_trigger_symbols_indices[i] = trigIdx[i];
    }

    auto lastShown = j.at("last_shown").get<std::vector<sint32>>();
    for (sint32 i = 0; i < k_MAX_PLAYERS; ++i)
        s.m_lastShown[i] = (i < static_cast<sint32>(lastShown.size()))
                              ? lastShown[i] : 0;

    if (!j.at("ui_component").is_null())
    {
        std::string ui = j.at("ui_component").get<std::string>();
        s.m_uiComponent = static_cast<char *>(malloc(ui.size() + 1));
        if (s.m_uiComponent) std::memcpy(s.m_uiComponent, ui.c_str(), ui.size() + 1);
    }

    auto paramIdx = j.at("parameter_indices").get<std::vector<sint32>>();
    if (s.m_num_parameters > 0)
    {
        s.m_parameter_indices.resize(s.m_num_parameters);
        for (sint32 i = 0; i < s.m_num_parameters
                            && i < static_cast<sint32>(paramIdx.size()); ++i)
            s.m_parameter_indices[i] = paramIdx[i];
    }

    if (!j.at("filename").is_null())
    {
        std::string fn = j.at("filename").get<std::string>();
        s.m_filename = static_cast<char *>(malloc(fn.size() + 1));
        if (s.m_filename) std::memcpy(s.m_filename, fn.c_str(), fn.size() + 1);
    }

    if (s.m_type == SLIC_OBJECT_HANDLEEVENT && gevmanager_Get())
    {
        gevmanager_Get()->AddCallback(s.m_event, s.m_priority, &s);
    }
}

// Phase F-13 — SlicSymTab (global named-symbol table).  Mirrors
// SlicSymTab::Serialize at SlicSymTab.cpp:39.
//
// JSON shape:
//   {
//     "array_size":  N,
//     "num_entries": M,
//     "entries":     [<SlicSymbolData json or null>, ...]   // length M
//   }
//
// Each non-null entry MUST carry serial_type in {named, parameter,
// builtin} — SymTab holds SlicNamedSymbol*, not bare SlicSymbolData.
// On load, entries are constructed via loadSlicSymbolFromJson, added
// to the underlying StringHash, and registered with the engine's
// builtin table when applicable.

void to_json(nlohmann::json &j, SlicSymTab const &t)
{
    nlohmann::json entries = nlohmann::json::array();
    for (sint32 i = 0; i < t.m_numEntries; ++i)
        entries.push_back(storeSlicSymbolToJson(t.m_array[i]));

    j = nlohmann::json{
        {"array_size",  t.m_arraySize},
        {"num_entries", t.m_numEntries},
        {"entries",     std::move(entries)},
    };
}

void from_json(nlohmann::json const &j, SlicSymTab &t)
{
    // Drop the placeholder array allocated by SlicSymTab(sint32) (the
    // only public ctor in the unit-test path).  The (CivArchive&)
    // ctor leaves m_array uninitialised and lets Serialize allocate;
    // we do the same here.
    delete[] t.m_array;

    j.at("array_size").get_to(t.m_arraySize);
    j.at("num_entries").get_to(t.m_numEntries);
    t.m_array = new SlicNamedSymbol *[t.m_arraySize];
    std::fill(t.m_array, t.m_array + t.m_arraySize,
              static_cast<SlicNamedSymbol *>(nullptr));

    auto const &entries = j.at("entries");
    for (sint32 i = 0; i < t.m_numEntries && i < static_cast<sint32>(entries.size()); ++i)
    {
        SlicSymbolData *sym = loadSlicSymbolFromJson(entries[i]);
        if (!sym)
        {
            t.m_array[i] = nullptr;
            continue;
        }
        // Mirror binary: cast to SlicNamedSymbol* (the factory returns
        // one of the F-10 derived classes for serial_type in
        // {named, parameter, builtin}; "generic" would be a malformed
        // symtab entry).
        auto *named = dynamic_cast<SlicNamedSymbol *>(sym);
        if (!named)
        {
            delete sym;
            throw nlohmann::json::other_error::create(
                501,
                "SlicSymTab entry " + std::to_string(i) +
                    " has serial_type 'generic'; expected named/parameter/builtin",
                nullptr);
        }
        t.m_array[i] = named;
        t.StringHash<SlicNamedSymbol>::Add(named);
        if (named->IsBuiltin() && slicengine_Get())
        {
            slicengine_Get()->AddBuiltinSymbol(static_cast<SlicBuiltinNamedSymbol *>(named));
        }
    }
}

// --- Phase F-16 SlicEngine bridge implementation ---------------------

namespace {

template <class T>
nlohmann::json stringHashToJson(StringHash<T> const *hash)
{
    nlohmann::json arr = nlohmann::json::array();
    if (hash)
        hash->ForEach([&arr](T const *obj) { arr.push_back(*obj); });
    return arr;
}

}  // namespace

void to_json(nlohmann::json &j, SlicEngine const &e)
{
    j = nlohmann::json::object();
    j["tutorial_player"] = e.m_tutorialPlayer;
    j["tutorial_active"] = static_cast<bool>(e.m_tutorialActive);

    j["segments"]  = stringHashToJson<SlicSegment>(e.m_segmentHash);
    j["constants"] = stringHashToJson<SlicConst>(e.m_constHash);
    j["sym_tab"]   = *e.m_symTab;

    nlohmann::json records = nlohmann::json::array();
    for (sint32 p = 0; p < k_MAX_PLAYERS; ++p)
    {
        if (!e.m_records[p]) continue;
        nlohmann::json entries = nlohmann::json::array();
        PointerList<SlicRecord>::Walker walk(e.m_records[p]);
        while (walk.IsValid())
        {
            entries.push_back(*walk.GetObj());
            walk.Next();
        }
        records.push_back({{"player", p}, {"entries", std::move(entries)}});
    }
    j["records"] = std::move(records);

    j["timer"]       = std::vector<sint32>(e.m_timer, e.m_timer + k_NUM_TIMERS);

    std::vector<int> triggerKey(k_MAX_TRIGGER_KEYS);
    for (sint32 i = 0; i < k_MAX_TRIGGER_KEYS; ++i)
        triggerKey[i] = static_cast<unsigned char>(e.m_triggerKey[i]);
    j["trigger_key"] = std::move(triggerKey);

    j["do_research_on_unblank"] = static_cast<bool>(e.m_doResearchOnUnblank);
    j["research_owner"]         = e.m_researchOwner;
    // m_researchText is a 256-MBCHAR null-terminated buffer; truncate
    // at the first NUL on store, pad-with-zero on load.
    j["research_text"] = std::string(e.m_researchText,
        strnlen(e.m_researchText, sizeof(e.m_researchText)));

    j["current_message"]  = e.m_currentMessage
        ? static_cast<uint32>(e.m_currentMessage->m_id)
        : 0u;
    j["disabled_classes"] = sdaToJson(e.m_disabledClasses);
}

void from_json(nlohmann::json const &j, SlicEngine &e)
{
    j.at("tutorial_player").get_to(e.m_tutorialPlayer);
    e.m_tutorialActive = j.at("tutorial_active").get<bool>();

    // SymTab: re-create from JSON; the existing SymTab was allocated
    // by the SlicEngine ctor.
    j.at("sym_tab").get_to(*e.m_symTab);

    // Constants: clear the hash and re-add each entry.
    if (e.m_constHash)
    {
        e.m_constHash->Clear();
        for (auto const &cj : j.at("constants"))
        {
            auto *c = new SlicConst("", 0);
            cj.get_to(*c);
            e.m_constHash->Add(c->GetName(), c);
        }
    }

    // Segments: clear the segment hash and re-add each entry.  Each
    // segment registers itself with gevmanager_Get() on load (see F-14).
    //
    // Crucial: SlicSegmentHash owns a fixed-size m_segments[] array
    // sized at SetSize().  StringHash::Clear() drains the hash buckets
    // but leaves m_nextSegment and m_segments untouched — so calling
    // Add() after a Clear() (with the hash already filled from gameinit's
    // fresh-game SlicEngine::Reload) walks past the array bounds.
    // Re-size the segment array to the incoming segment count.
    if (e.m_segmentHash)
    {
        auto const &segments_j = j.at("segments");
        e.m_segmentHash->Clear();
        e.m_segmentHash->SetSize(static_cast<sint32>(segments_j.size()));
        for (auto const &sj : segments_j)
        {
            auto *seg = new SlicSegment;
            sj.get_to(*seg);
            e.m_segmentHash->Add(seg->GetName(), seg);
        }
    }

    // Per-player records.
    for (auto & m_record : e.m_records)
    {
        if (m_record)
        {
            m_record->DeleteAll();
            delete m_record;
            m_record = nullptr;
        }
    }
    for (auto const &rec : j.at("records"))
    {
        sint32 p = rec.at("player").get<sint32>();
        if (p < 0 || p >= k_MAX_PLAYERS) continue;
        e.m_records[p] = new PointerList<SlicRecord>;
        for (auto const &entryJson : rec.at("entries"))
        {
            auto *r = new SlicRecord(0, nullptr, nullptr, nullptr);
            entryJson.get_to(*r);
            e.m_records[p]->AddTail(r);
        }
    }

    auto timer = j.at("timer").get<std::vector<sint32>>();
    for (sint32 i = 0; i < k_NUM_TIMERS; ++i)
        e.m_timer[i] = (i < static_cast<sint32>(timer.size())) ? timer[i] : 0;

    auto triggerKey = j.at("trigger_key").get<std::vector<int>>();
    for (sint32 i = 0; i < k_MAX_TRIGGER_KEYS; ++i)
        e.m_triggerKey[i] = (i < static_cast<sint32>(triggerKey.size()))
                               ? static_cast<MBCHAR>(triggerKey[i])
                               : 0;

    e.m_doResearchOnUnblank = j.at("do_research_on_unblank").get<bool>();
    j.at("research_owner").get_to(e.m_researchOwner);
    std::string text = j.at("research_text").get<std::string>();
    std::memset(e.m_researchText, 0, sizeof(e.m_researchText));
    std::memcpy(e.m_researchText, text.c_str(),
                std::min<size_t>(text.size(), sizeof(e.m_researchText) - 1));

    if (e.m_currentMessage)
        e.m_currentMessage->m_id = j.value("current_message", 0u);

    delete e.m_disabledClasses;
    e.m_disabledClasses = jsonToSda<sint32>(j.at("disabled_classes"));
    // SlicEngine invariants: m_disabledClasses is always non-null.
    if (!e.m_disabledClasses)
        e.m_disabledClasses = new SimpleDynamicArray<sint32>;
}

// Phase F-17a — SlicButton.  Mirrors SlicButton::Serialize at
// SlicButton.cpp:104.

void to_json(nlohmann::json &j, SlicButton const &b)
{
    j = nlohmann::json::object();
    j["name"]         = b.m_name;
    j["is_close_event"] = b.m_isCloseEvent != 0;
    j["code_offset"]  = b.m_codeOffset;
    j["message"]      = b.m_message ? *b.m_message : Message();
    if (b.m_context)
        j["context"]  = *b.m_context;
    else
        j["context"]  = SlicObject();
    j["segment_name"] = b.m_segment ? safeName(b.m_segment->GetName())
                         : b.m_segmentName;
}

void from_json(nlohmann::json const &j, SlicButton &b)
{
    j.at("name").get_to(b.m_name);
    b.m_isCloseEvent = j.at("is_close_event").get<bool>() ? 1 : 0;
    j.at("code_offset").get_to(b.m_codeOffset);

    if (!b.m_message)
        b.m_message = new Message();
    j.at("message").get_to(*b.m_message);

    SlicObject *newContext = new SlicObject();
    j.at("context").get_to(*newContext);
    newContext->AddRef();
    if (b.m_context)
        b.m_context->Release();
    b.m_context = newContext;

    std::string segName = j.at("segment_name").get<std::string>();
    b.m_segmentName.clear();
    b.m_segment = nullptr;
    if (!segName.empty() && slicengine_Get())
    {
        b.m_segment = slicengine_Get()->GetSegment(segName.c_str());
    }
    if (!b.m_segment && !segName.empty())
    {
        b.m_segmentName = segName;
    }
}

// Phase F-17b — SlicEyePoint.  Mirrors SlicEyePoint::Serialize at
// SlicEyePoint.cpp:104.

void to_json(nlohmann::json &j, SlicEyePoint const &e)
{
    j = nlohmann::json{
        {"point",        e.m_point},
        {"name",         optStringToJson(e.m_name)},
        {"message",      e.m_message ? *e.m_message : Message()},
        {"data",         e.m_data},
        {"unit",         static_cast<ID const &>(e.m_unit)},
        {"recipient",    e.m_recipient},
        {"segment_name", e.m_segment ? safeName(e.m_segment->GetName())
                                      : std::string()},
        {"type",         static_cast<int>(e.m_type)},
    };
}

void from_json(nlohmann::json const &j, SlicEyePoint &e)
{
    j.at("point").get_to(e.m_point);
    jsonToOptString(j.at("name"), e.m_name);

    if (!e.m_message)
        e.m_message = new Message();
    j.at("message").get_to(*e.m_message);

    j.at("data").get_to(e.m_data);
    j.at("unit").get_to(static_cast<ID &>(e.m_unit));
    j.at("recipient").get_to(e.m_recipient);

    std::string segName = j.at("segment_name").get<std::string>();
    e.m_segment = (!segName.empty() && slicengine_Get())
                      ? slicengine_Get()->GetSegment(segName.c_str())
                      : nullptr;

    e.m_type = static_cast<EYE_POINT_TYPE>(j.at("type").get<int>());
}

// Phase F-17c — MessageData.  Mirrors MessageData::Serialize at
// messagedata.cpp:367.

void to_json(nlohmann::json &j, MessageData const &m)
{
    nlohmann::json buttons = nlohmann::json::array();
    if (m.m_buttonList)
    {
        PointerList<SlicButton>::Walker walk(m.m_buttonList);
        while (walk.IsValid())
        {
            buttons.push_back(*walk.GetObj());
            walk.Next();
        }
    }

    nlohmann::json eyePoints = nlohmann::json::array();
    if (m.m_eyePoints)
    {
        PointerList<SlicEyePoint>::Walker walk(m.m_eyePoints);
        while (walk.IsValid())
        {
            eyePoints.push_back(*walk.GetObj());
            walk.Next();
        }
    }

    nlohmann::json cities = nlohmann::json::array();
    if (m.m_cityList)
    {
        for (sint32 i = 0; i < m.m_cityList->Num(); ++i)
            cities.push_back(static_cast<ID const &>(m.m_cityList->Access(i)));
    }

    j = nlohmann::json{
        {"id",                     m.m_id},
        {"owner",                  m.m_owner},
        {"sender",                 m.m_sender},
        {"is_read",                m.m_isRead != 0},
        {"msg_type",               m.m_msgType},
        {"msg_selected_type",      m.m_msgSelectedType},
        {"timestamp",              m.m_timestamp},
        {"advance",                m.m_advance},
        {"advance_set",            m.m_advanceSet != 0},
        {"expiration",             m.m_expiration},
        {"is_help_box",            m.m_isHelpBox != 0},
        {"is_alert_box",           m.m_isAlertBox != 0},
        {"is_instant",             m.m_isInstant != 0},
        {"class",                  m.m_class},
        {"close_disabled",         m.m_closeDisabled != 0},
        {"is_diplomatic_response", m.m_isDiplomaticResponse != 0},
        {"use_director",           m.m_useDirector != 0},
        {"caption",                utf8_safe(m.m_caption)},
        {"text",                   optStringToJson(m.m_text)},
        {"title",                  optStringToJson(m.m_title)},
        {"buttons",                std::move(buttons)},
        {"eye_points",             std::move(eyePoints)},
        {"city_list",              std::move(cities)},
        {"request",                m.m_request},
        {"trade_offer",            m.m_tradeOffer},
    };
}

void from_json(nlohmann::json const &j, MessageData &m)
{
    j.at("id").get_to(m.m_id);
    j.at("owner").get_to(m.m_owner);
    j.at("sender").get_to(m.m_sender);
    m.m_isRead = j.at("is_read").get<bool>() ? 1 : 0;
    j.at("msg_type").get_to(m.m_msgType);
    j.at("msg_selected_type").get_to(m.m_msgSelectedType);
    j.at("timestamp").get_to(m.m_timestamp);
    j.at("advance").get_to(m.m_advance);
    m.m_advanceSet = j.at("advance_set").get<bool>() ? 1 : 0;
    j.at("expiration").get_to(m.m_expiration);
    m.m_isHelpBox = j.at("is_help_box").get<bool>() ? 1 : 0;
    m.m_isAlertBox = j.at("is_alert_box").get<bool>() ? 1 : 0;
    m.m_isInstant = j.at("is_instant").get<bool>() ? 1 : 0;
    j.at("class").get_to(m.m_class);
    m.m_closeDisabled = j.at("close_disabled").get<bool>() ? 1 : 0;
    m.m_isDiplomaticResponse = j.at("is_diplomatic_response").get<bool>() ? 1 : 0;
    m.m_useDirector = j.at("use_director").get<bool>() ? 1 : 0;

    std::string caption = j.at("caption").get<std::string>();
    std::fill(m.m_caption, m.m_caption + k_MAX_MSG_LEN, (MBCHAR)0);
    std::memcpy(m.m_caption, caption.c_str(),
                std::min<size_t>(caption.size(), k_MAX_MSG_LEN - 1));

    jsonToOptString(j.at("text"), m.m_text);
    jsonToOptString(j.at("title"), m.m_title);

    if (m.m_buttonList)
    {
        m.m_buttonList->DeleteAll();
        delete m.m_buttonList;
    }
    m.m_buttonList = new PointerList<SlicButton>;
    for (auto const &bj : j.at("buttons"))
    {
        auto *btn = new SlicButton();
        bj.get_to(*btn);
        m.m_buttonList->AddTail(btn);
    }

    if (m.m_eyePoints)
    {
        m.m_eyePoints->DeleteAll();
        delete m.m_eyePoints;
    }
    m.m_eyePoints = new PointerList<SlicEyePoint>;
    for (auto const &ej : j.at("eye_points"))
    {
        auto *eye = new SlicEyePoint();
        ej.get_to(*eye);
        m.m_eyePoints->AddTail(eye);
    }

    if (!m.m_cityList)
        m.m_cityList = new UnitDynamicArray;
    m.m_cityList->Clear();
    for (auto const &cj : j.at("city_list"))
    {
        ID id(0);
        cj.get_to(id);
        m.m_cityList->Insert(id);
    }

    j.at("request").get_to(m.m_request);
    j.at("trade_offer").get_to(m.m_tradeOffer);
}

// Phase F-17d — MessagePool.  Mirrors MessagePool::Serialize at
// MessagePool.cpp:48.

void to_json(nlohmann::json &j, MessagePool const &p)
{
    nlohmann::json messages = nlohmann::json::array();
    for (auto i : p.m_table)
    {
        if (i)
            messages.push_back(*static_cast<MessageData const *>(i));
    }
    j = nlohmann::json{
        {"next_key", const_cast<MessagePool &>(p).HackGetKey()},
        {"messages", std::move(messages)},
    };
}

void from_json(nlohmann::json const &j, MessagePool &p)
{
    // Clear any pre-existing entries so re-deserialization doesn't leak or
    // produce a hybrid pool. Mirrors MessageData::from_json semantics.
    // Each m_table slot is a GameObj BST root (m_lesser/m_greater), so we
    // must drain via Del() — matches ~ObjPool at ObjPool.cpp:48.
    for (auto & i : p.m_table)
    {
        while (i)
        {
            p.Del(i);
        }
    }

    p.HackSetKey(j.at("next_key").get<uint32>());

    for (auto const &entry : j.at("messages"))
    {
        auto *data = new MessageData(ID(0), 0);
        entry.get_to(*data);
        p.Insert(data);
    }
}

// --- CtpAi composite bridge (Phase F-18) --------------------------------
//
// CtpAi::Save(archive) is a thin wrapper over Diplomat::SaveAll, which
// writes three things: s_nextId, AgreementMatrix::s_agreements, and the
// per-player Diplomat vector.  All three already have JSON bridges; this
// helper composes them into a single "ai_state" sub-object so SaveJson /
// LoadJson stay flat.
//
// Composes statics directly rather than introducing a wrapper struct —
// there's no per-instance object to bind to here.

namespace {

nlohmann::json ctpai_state_to_json()
{
    nlohmann::json j;
    j["diplomat_next_id"] = Diplomat::PeekNextId();
    j["agreements"]       = AgreementMatrix::s_agreements;

    nlohmann::json diplomats = nlohmann::json::array();
    size_t const   count     = Diplomat::Count();
    for (size_t i = 0; i < count; ++i)
    {
        diplomats.push_back(Diplomat::GetDiplomat(static_cast<sint32>(i)));
    }
    j["diplomats"] = std::move(diplomats);
    return j;
}

}  // namespace

namespace json_save {

// Compose the full game state into a single JSON document and write to
// `path`.  Mirrors GameFile::Save's binary archive order (gs/fileio/
// GameFile.cpp:336-538) so the JSON top-level keys appear in the same
// logical sequence — useful for diffs against a binary→JSON one-shot
// converter (Phase G).
bool SaveJson(char const *path)
{
    nlohmann::json doc;
    doc["magic"]          = MAGIC;
    doc["schema_version"] = SCHEMA_VERSION;
    doc["saved_at"]       = iso_utc_now();
    doc["ctp2_build"]     = CTP2_BUILD_SHA;

    // --- Core singletons (mirror civrand / settings / world / turn
    // order in GameFile::Save:357-388) --------------------------------
    if (rand_ptr())              doc["rng"]                       = *rand_ptr();
    if (GameSettings *gs = gamesettings_Get()) doc["settings"] = *gs;
    if (world_Get())          doc["world"]                     = *world_Get();
    if (turn_Get())          doc["turn"]                      = *turn_Get();

    // Selection is currently a scalar projection of player_view state.
    // SelectedItem (the full per-player ui-side state) is too coupled
    // to Army/Unit to round-trip until Phase E lands a richer bridge.
    SelectionState sel;
    sel.current_player = player_view::CurPlayer();
    doc["selection"] = sel;

    // --- Object pools (GameFile::Save:393-458) -----------------------
    if (unitpool_Get())               doc["unit_pool"]                  = *unitpool_Get();
    if (ArmyPool *ap = armypool_Get()) doc["army_pool"] = *ap;
    if (TradePool *tp = tradepool_Get()) doc["trade_pool"] = *tp;
    if (Pollution *pol = pollution_Get()) doc["pollution"]             = *pol;
    if (slicengine_Get())                doc["slic_engine"]                = *slicengine_Get();
    if (TerrainImprovementPool *tip = terrimprovepool_Get()) doc["terrain_improvement_pool"] = *tip;
    if (CivilisationPool *cp = civilisationpool_Get()) doc["civilisation_pool"] = *cp;
    if (MessagePool *mp = messagepool_Get()) doc["message_pool"] = *mp;
    if (InstallationPool *ip = installationpool_Get()) doc["installation_pool"] = *ip;
    if (DiplomaticRequestPool *dp = diplomaticrequestpool_Get()) doc["diplomatic_request_pool"] = *dp;
    if (AgreementPool *agp = agreementpool_Get()) doc["agreement_pool"] = *agp;
    if (TradeOfferPool *top = tradeofferpool_Get()) doc["trade_offer_pool"] = *top;

    // --- Trackers + exclusions (GameFile::Save:474-503) --------------
    if (wonder_tracker_Get())  doc["wonder_tracker"]            = *wonder_tracker_Get();
    if (exclusions_Get())        doc["exclusions"]                = *exclusions_Get();
    if (FeatTracker *ft = feattracker_Get()) doc["feat_tracker"] = *ft;
    if (EventTracker *et = eventtracker_Get()) doc["event_tracker"] = *et;

    // Action Log: the engine-native action/event ledger.  Additive + optional
    // on load (no schema bump) — see action_log.h and plans/ctp2-action-log.md.
    doc["action_log"] = action_log::Get();

    // --- TopTen: not written by GameFile::Save (legacy-load-only in the
    // binary path); included in JSON so leaderboard state persists across
    // save/load.  See plan section "Open questions before coding".
    if (topten_Get())         doc["top_ten"]                  = *topten_Get();

    // --- Players (GameFile::Save:508-532) ----------------------------
    // Per-slot {alive, data}.  Dead slots emit alive:false with no data
    // — keeps array indices stable so a future scenario-load can address
    // slot N directly.  Mirrors the playerAlive sentinel byte the binary
    // path writes.
    if (player_arr_Get())
    {
        nlohmann::json players = nlohmann::json::array();
        for (sint32 i = 0; i < k_MAX_PLAYERS; ++i)
        {
            nlohmann::json slot;
            if (player_Get(i))
            {
                slot["alive"] = true;
                slot["data"]  = *player_Get(i);
            }
            else
            {
                slot["alive"] = false;
            }
            players.push_back(std::move(slot));
        }
        doc["players"] = std::move(players);
    }

    if (g_deadPlayer)
    {
        nlohmann::json dead = nlohmann::json::array();
        PointerList<Player>::Walker walk(g_deadPlayer);
        while (walk.IsValid())
        {
            dead.push_back(*walk.GetObj());
            walk.Next();
        }
        doc["dead_players"] = std::move(dead);
    }

    // --- AI state (GameFile::Save:537 → CtpAi::Save) -----------------
    doc["ai_state"] = ctpai_state_to_json();

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
	std::cerr << "[json_save] LoadJson: loading from '" << path << "'\n";

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

    // Populate game-state singletons in place.  Pattern: gameinit_
    // Initialize has already run with archive=NULL (the "fresh game"
    // branch), which constructs default-state instances of every
    // singleton.  LoadJson then overwrites that state field-by-field
    // via each class's from_json.  This avoids the need for
    // T(nlohmann::json const&) ctors on the singletons whose only
    // existing custom ctor is T(CivArchive&).
    //
    // Order mirrors gameinit.cpp:1509-1855 (and GameFile::Save:336-538):
    // World first → TurnCount → UnitPool/ArmyPool (rebuild quadtree)
    // → TradePool (RecreateActors) → Pollution → SlicEngine (PostSerialize)
    // → TerrainImprovementPool → CivilisationPool → MessagePool
    // → InstallationPool (rebuild quadtree) → WonderTracker → Exclusions
    // → FeatTracker → EventTracker → TopTen → Players → DeadPlayers
    // → CtpAi state.
    try
    {
        // Core singletons
        if (doc.contains("rng")      && rand_ptr())             doc.at("rng")     .get_to(*rand_ptr());
        if (GameSettings *gs = gamesettings_Get(); doc.contains("settings") && gs) doc.at("settings").get_to(*gs);
        if (doc.contains("world")    && world_Get())         doc.at("world")   .get_to(*world_Get());
        if (doc.contains("turn")     && turn_Get())         doc.at("turn")    .get_to(*turn_Get());
        // Selection is currently informational — no public setter for
        // SelectedItem::m_current_player.  Phase E will wire a bridge.

        // Pools — most append-on-from_json (no implicit Clear).  The
        // fresh-game gameinit path leaves these empty, so appending is
        // equivalent to overwriting.  Pools whose from_json *does*
        // clear pre-existing entries (MessagePool) handle it themselves.
        if (doc.contains("unit_pool")          && unitpool_Get())               doc.at("unit_pool")               .get_to(*unitpool_Get());
        if (ArmyPool *ap = armypool_Get(); doc.contains("army_pool") && ap) doc.at("army_pool").get_to(*ap);
        if (TradePool *tp = tradepool_Get(); doc.contains("trade_pool") && tp) doc.at("trade_pool").get_to(*tp);
        if (Pollution *pol = pollution_Get(); doc.contains("pollution") && pol) doc.at("pollution").get_to(*pol);
        if (doc.contains("slic_engine")        && slicengine_Get())                doc.at("slic_engine")             .get_to(*slicengine_Get());
        if (TerrainImprovementPool *tip = terrimprovepool_Get(); doc.contains("terrain_improvement_pool") && tip) doc.at("terrain_improvement_pool").get_to(*tip);
        if (CivilisationPool *cp = civilisationpool_Get(); doc.contains("civilisation_pool") && cp) doc.at("civilisation_pool").get_to(*cp);
        if (MessagePool *mp = messagepool_Get(); doc.contains("message_pool") && mp) doc.at("message_pool").get_to(*mp);
        if (InstallationPool *ip = installationpool_Get(); doc.contains("installation_pool") && ip) doc.at("installation_pool").get_to(*ip);
        if (DiplomaticRequestPool *dp = diplomaticrequestpool_Get(); doc.contains("diplomatic_request_pool") && dp) doc.at("diplomatic_request_pool").get_to(*dp);
        if (AgreementPool *agp = agreementpool_Get(); doc.contains("agreement_pool") && agp) doc.at("agreement_pool").get_to(*agp);
        if (TradeOfferPool *top = tradeofferpool_Get(); doc.contains("trade_offer_pool") && top) doc.at("trade_offer_pool").get_to(*top);

        // Trackers
        if (doc.contains("wonder_tracker") && wonder_tracker_Get()) doc.at("wonder_tracker").get_to(*wonder_tracker_Get());
        if (doc.contains("exclusions")     && exclusions_Get())       doc.at("exclusions")    .get_to(*exclusions_Get());
        if (FeatTracker *ft = feattracker_Get(); doc.contains("feat_tracker") && ft) doc.at("feat_tracker").get_to(*ft);
        if (EventTracker *et = eventtracker_Get(); doc.contains("event_tracker") && et) doc.at("event_tracker").get_to(*et);
        if (doc.contains("top_ten")        && topten_Get())       doc.at("top_ten")       .get_to(*topten_Get());

        // Action Log: optional-on-load.  Old saves with no action_log key
        // reset the ledger to empty rather than carrying a prior game's log.
        if (doc.contains("action_log")) action_log::Set(doc.at("action_log"));
        else                            action_log::Clear();

        // Post-load fixups that mirror gameinit_Initialize's archive
        // branch (gameinit.cpp:1623-1639, 1677, 1761).  These rebuild
        // observer/derived state that the bridges don't carry.
        // Clear the spatial indices before rebuild — gameinit's fresh-
        // game branch populated them with units/installations from the
        // throwaway initial state, and those references are now stale
        // (pool drains in from_json deleted the backing UnitData /
        // InstallationData).  Walking the stale tree during Insert
        // crashes (intermittent SIGSEGV in UnitData::GetPos via a
        // null-this dereference; ASAN-confirmed).
        if (unit_tree_Get())         unit_tree_Get()->Clear();
        if (installation_tree_Get()) installation_tree_Get()->Clear();
        if (unitpool_Get())         unitpool_Get()->RebuildQuadTree();
        if (InstallationPool *ip = installationpool_Get()) ip->RebuildQuadTree();
        if (TradePool *tp = tradepool_Get()) tp->RecreateActors();
        if (slicengine_Get())          slicengine_Get()->PostSerialize();

        // Players: per-slot in-place from_json (F-20).
        //
        // gameinit_Initialize allocates Players based on
        // ProfileDB::GetNPlayers(), which on --load-game inherits from
        // userprofile.txt / profile.txt (NumPlayers=6 by default), not
        // from the save.  So before restoring data, we delete any slot
        // the save marks dead — otherwise the in-memory state drifts
        // from the save (extra Players appear out of thin air).
        if (doc.contains("players") && player_arr_Get())
        {
            Player **g_players = player_arr_Get();
            auto const &players = doc.at("players");
            sint32 const n = std::min(static_cast<sint32>(players.size()),
                                      static_cast<sint32>(k_MAX_PLAYERS));
            for (sint32 i = 0; i < n; ++i)
            {
                auto const &slot = players[i];
                bool const alive = slot.value("alive", false);
                if (!alive)
                {
                    // Save says this slot is dead — clear any Player
                    // that gameinit speculatively allocated.
                    delete g_players[i];
                    g_players[i] = nullptr;
                }
                else if (g_players[i] && slot.contains("data"))
                {
                    slot.at("data").get_to(*g_players[i]);
                }
            }
            // Slots past the save's player array (rare: save had fewer
            // total slots than k_MAX_PLAYERS) — treat as dead.
            for (sint32 i = n; i < k_MAX_PLAYERS; ++i)
            {
                delete g_players[i];
                g_players[i] = nullptr;
            }
        }

        // AI state (CtpAi::Save mirror): Resize the diplomat vector to
        // the saved count, then per-slot from_json.
        if (doc.contains("ai_state"))
        {
            auto const &ai = doc.at("ai_state");
            if (ai.contains("diplomats") && ai["diplomats"].is_array())
            {
                sint32 const n = static_cast<sint32>(ai["diplomats"].size());
                Diplomat::ResizeAll(n);
                for (sint32 i = 0; i < n; ++i)
                    ai["diplomats"][i].get_to(Diplomat::GetDiplomat(i));
            }
            if (ai.contains("diplomat_next_id") && Diplomat::Count() > 0)
            {
                // SetNextId is non-static but writes the static s_nextId;
                // any instance works.  Slot 0 always exists post-Resize.
                Diplomat::GetDiplomat(0).SetNextId(
                    ai["diplomat_next_id"].get<sint32>());
            }
            if (ai.contains("agreements"))
                ai["agreements"].get_to(AgreementMatrix::s_agreements);
        }

        // Re-sync per-player AI structures with the post-load g_player
        // state.  gameinit_Initialize sized MapAnalysis/Scheduler/
        // Governor/Diplomat to ProfileDB::NumPlayers (6 by default from
        // profile.txt) BEFORE LoadJson ran; after we delete the dead
        // slots above, MapAnalysis::m_threatGrid still has 6 entries
        // but Diplomat has been shrunk to the save's player count.
        // ASan + libc++ hardening catch the mismatch when
        // MapAnalysis::GetEnemyGrid iterates m_threatGrid.size() and
        // queries Diplomat::ComputeEffectiveRegard with an opponent id
        // that's beyond m_foreigners.  CtpAi::Resize walks g_player to
        // recompute s_maxPlayers and resizes all dependent subsystems
        // consistently.  Gated on world_Get() because some unit-test
        // fixtures call LoadJson without going through gameinit, leaving
        // world_Get() == nullptr (MapAnalysis::Resize would deref it).
        if (world_Get())
            CtpAi::Resize();

        // Units/cities were restored without gfx state (UnitData's
        // from_json intentionally leaves m_actor null).  Recreate the
        // actors here so EVERY load entry point — the UI load dialog,
        // headless --load-game / --json-load, and the test-API
        // load_game command — gets actors without its own patch-up
        // call.  Runs after the player loop so RecreateGfxState
        // resolves unit records against the restored governments.
        // Same world_Get() gate as CtpAi::Resize above: unit-test
        // fixtures may call LoadJson without gameinit (no unit DB).
        if (world_Get() && unitpool_Get())
            unitpool_Get()->RecreateActors();
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
