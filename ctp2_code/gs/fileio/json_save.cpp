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
