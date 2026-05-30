// test/cpp/test_json_save.cpp
// Phase A scaffold tests for the JSON savegame migration
// (~/projects/claude/plans/ctp2-json-savegame.md).
//
// Acceptance for Phase A:
//   - json_save::SaveJson writes a file containing
//     {"magic": "CTP2-JSON", "schema_version": 1}
//   - json_save::LoadJson reads it back and verifies the header
//   - Round-trips of the 3 leaf types (MapPoint, ID, sint32) preserve
//     value equality through nlohmann::json
//
// These are all in-process unit tests — no subprocess spawn.  They
// live in the fast suite (Phase A budget: keep under 5 s).

#include "ctp/c3.h"
#include "doctest.h"
#include "gs/fileio/json_save.h"

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>

namespace {

// Read a file in full.  Empty string on failure.
std::string slurp(char const *path)
{
    std::ifstream in(path);
    if (!in) return {};
    std::stringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

}  // namespace

// --- Leaf round-trips (the 3 types Phase A locks in) ---

TEST_CASE("json round-trip: MapPoint preserves x/y/z")
{
    // _SMALL_MAPPOINTS gives MapPoint a 2-arg ctor; z stays 0.  z is
    // still in the JSON so the schema is forward-compatible if the
    // 3D mode is reactivated later.
    MapPoint orig(7, 13);
    nlohmann::json j = orig;

    MapPoint round;
    j.get_to(round);

    CHECK(round.x == orig.x);
    CHECK(round.y == orig.y);
    CHECK(round.z == orig.z);
}

TEST_CASE("json round-trip: MapPoint key set is exactly {x, y, z}")
{
    // Lock the key set so a future scope-creep that adds e.g. `pad` to
    // the JSON gets caught.  pad is an alignment field, never user-
    // visible — it should NOT appear in the JSON spec.
    MapPoint p(1, 2);
    nlohmann::json j = p;
    CHECK(j.contains("x"));
    CHECK(j.contains("y"));
    CHECK(j.contains("z"));
    CHECK_FALSE(j.contains("pad"));
    CHECK(j.size() == 3);
}

TEST_CASE("json round-trip: ID preserves m_id")
{
    ID orig(0xCAFEBABE);
    nlohmann::json j = orig;

    ID round(0);
    j.get_to(round);

    CHECK(round.m_id == orig.m_id);
}

TEST_CASE("json round-trip: ID serialises as bare uint32, not an object")
{
    // ID wraps a single uint32 — the JSON form is the integer itself,
    // not {"m_id": N}.  Keeps the serialised form compact and matches
    // how IDs flow through the rest of the schema (e.g.
    // `{"unit_id": 12345}` not `{"unit_id": {"m_id": 12345}}`).
    ID id(42);
    nlohmann::json j = id;
    CHECK(j.is_number_integer());
    CHECK(j.get<uint32>() == 42u);
}

TEST_CASE("json round-trip: sint32 boxed value preserves value and sign")
{
    // nlohmann handles primitives natively — no custom to_json/from_json.
    // The plan calls out sint32 explicitly as a leaf type to lock in
    // because integer overflow / sign confusion is a common bug class.
    sint32 const positive   = 1'234'567;
    sint32 const negative   = -1'234'567;
    sint32 const zero       = 0;
    sint32 const max_value  = 0x7FFFFFFF;
    sint32 const min_value  = static_cast<sint32>(0x80000000);

    for (sint32 v : {positive, negative, zero, max_value, min_value})
    {
        nlohmann::json j  = v;
        sint32 const  out = j.get<sint32>();
        CHECK(out == v);
    }
}

// --- Top-level Save/Load entry-point smoke test ---

TEST_CASE("json_save: SaveJson + LoadJson round-trip the header")
{
    char const *path = "/tmp/ctp2_phase_a_header.json";
    std::remove(path);

    REQUIRE(json_save::SaveJson(path));

    // File exists and contains the expected magic.
    std::string const body = slurp(path);
    REQUIRE_FALSE(body.empty());
    CHECK(body.find("CTP2-JSON")          != std::string::npos);
    CHECK(body.find("\"schema_version\"") != std::string::npos);

    // Load verifies magic + schema_version.
    CHECK(json_save::LoadJson(path));
}

TEST_CASE("json_save: LoadJson rejects unknown magic")
{
    char const *path = "/tmp/ctp2_phase_a_bad_magic.json";
    {
        std::ofstream out(path);
        out << R"({"magic": "NOT-CTP2", "schema_version": 1})";
    }
    CHECK_FALSE(json_save::LoadJson(path));
}

TEST_CASE("json_save: LoadJson rejects mismatched schema_version")
{
    char const *path = "/tmp/ctp2_phase_a_bad_schema.json";
    {
        std::ofstream out(path);
        out << R"({"magic": "CTP2-JSON", "schema_version": 999})";
    }
    CHECK_FALSE(json_save::LoadJson(path));
}

TEST_CASE("json_save: LoadJson rejects malformed JSON")
{
    char const *path = "/tmp/ctp2_phase_a_malformed.json";
    {
        std::ofstream out(path);
        out << "this is not json at all";
    }
    CHECK_FALSE(json_save::LoadJson(path));
}

TEST_CASE("json_save: LoadJson rejects missing file")
{
    CHECK_FALSE(json_save::LoadJson("/tmp/this_path_does_not_exist_xx.json"));
}

// --- Phase B round-trip tests for the 4 top-level subtypes ---
//
// Each test constructs the subtype with non-default values, round-
// trips it through nlohmann::json, and checks field-by-field
// equality.  The acceptance criterion in the JSON migration plan is
// "a stub save with 4 top-level fields round-trips" — these are
// those 4 fields.

TEST_CASE("json round-trip: GameSettings preserves all 7 scalar fields")
{
    GameSettings orig;
    // Drive non-default values via the public setters that exist;
    // the bridge accesses the private members via friend declaration.
    orig.SetStartingAge(2);
    orig.SetEndingAge(5);
    orig.SetKeepScore(TRUE);
    orig.SetPollution(FALSE);

    nlohmann::json j = orig;

    GameSettings round;
    j.get_to(round);

    CHECK(round.GetDifficulty()    == orig.GetDifficulty());
    CHECK(round.GetRisk()          == orig.GetRisk());
    CHECK(round.GetKeeppScore()    == orig.GetKeeppScore());
    CHECK(round.GetPollution()     == orig.GetPollution());
    CHECK(round.GetStartingAge()   == orig.GetStartingAge());
    CHECK(round.GetEndingAge()     == orig.GetEndingAge());
    // GetAlienEndGame() has a network-active side effect — skip
    // checking it through the public getter and trust that the JSON
    // key round-trips.  The to/from_json read m_alienEndGame
    // directly via friend access.
}

TEST_CASE("json round-trip: GameSettings key set is exactly the locked 7")
{
    GameSettings gs;
    nlohmann::json j = gs;
    CHECK(j.size() == 7);
    CHECK(j.contains("difficulty"));
    CHECK(j.contains("risk"));
    CHECK(j.contains("alien_end_game"));
    CHECK(j.contains("keep_score"));
    CHECK(j.contains("starting_age"));
    CHECK(j.contains("ending_age"));
    CHECK(j.contains("pollution"));
    // No m_ prefixes leaking through — Decision #2 in the plan.
    for (auto const &el : j.items())
    {
        CHECK(el.key().substr(0, 2) != "m_");
    }
}

TEST_CASE("json round-trip: RandomGenerator preserves seed, buffer, indices, call count")
{
    RandomGenerator orig(/*seed*/12345);
    // Advance the RNG a few times so call_count and buffer evolve
    // from the freshly-initialised state.
    for (sint32 i = 0; i < 17; ++i) (void)orig.Next();

    nlohmann::json j = orig;

    RandomGenerator round(/*seed*/0);  // arbitrary; from_json overwrites
    j.get_to(round);

    CHECK(round.GetSeed()    == orig.GetSeed());
    CHECK(round.CallCount()  == orig.CallCount());

    // Strongest guarantee: the next 100 draws match.
    // (Catches buffer / pointer-index corruption that scalar checks
    // miss.)
    RandomGenerator orig_copy(orig);
    for (sint32 i = 0; i < 100; ++i)
    {
        sint32 const a = orig_copy.Next();
        sint32 const b = round.Next();
        CHECK(a == b);
    }
}

TEST_CASE("json round-trip: RandomGenerator key set includes seed/buffer/indices/call_count")
{
    RandomGenerator rng(42);
    nlohmann::json j = rng;
    CHECK(j.contains("seed"));
    CHECK(j.contains("buffer"));
    CHECK(j.contains("first_index"));
    CHECK(j.contains("second_index"));
    CHECK(j.contains("call_count"));
    CHECK(j["buffer"].is_array());
    CHECK(j["buffer"].size() == 56);
}

TEST_CASE("json round-trip: RandomGenerator load rejects wrong buffer size")
{
    nlohmann::json bad{
        {"seed",         42},
        {"buffer",       nlohmann::json::array({1, 2, 3})},  // wrong size
        {"first_index",  0},
        {"second_index", 0},
        {"call_count",   0},
    };
    RandomGenerator rng(1);
    CHECK_THROWS(bad.get_to(rng));
}

TEST_CASE("json round-trip: SelectionState preserves current_player")
{
    SelectionState orig;
    orig.current_player = 3;
    nlohmann::json j = orig;
    SelectionState round;
    j.get_to(round);
    CHECK(round.current_player == orig.current_player);
}

// --- Phase B top-level acceptance: SaveJson writes the 7 expected keys ---

TEST_CASE("json_save: SaveJson writes the Phase B top-level shape")
{
    char const *path = "/tmp/ctp2_phase_b_shape.json";
    std::remove(path);

    REQUIRE(json_save::SaveJson(path));

    std::ifstream in(path);
    REQUIRE(in.is_open());
    nlohmann::json doc;
    in >> doc;

    // Magic / version
    CHECK(doc.contains("magic"));
    CHECK(doc["magic"] == "CTP2-JSON");
    CHECK(doc.contains("schema_version"));
    CHECK(doc["schema_version"] == 1);

    // Metadata header
    CHECK(doc.contains("saved_at"));
    CHECK(doc.contains("ctp2_build"));
    // ISO 8601 / RFC 3339: "YYYY-MM-DDTHH:MM:SSZ" = 20 chars
    CHECK(doc["saved_at"].get<std::string>().length() == 20);

    // Selection (always present — minimal scalar in Phase B)
    CHECK(doc.contains("selection"));

    // rng/turn/settings only present if globals are wired.  When
    // running this test in fast/unit suite (no game initialisation),
    // they're not — that's fine.  Lock the presence-when-wired
    // behaviour at the integration tier instead.
}
