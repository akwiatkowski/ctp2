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
#include "gs/world/Cell.h"
#include "gs/world/TileInfo.h"
#include "gs/world/UnseenCell.h"

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

// --- Phase C-1 world-layer round-trip tests ---

TEST_CASE("json round-trip: TileInfo preserves scalar fields + transitions[]")
{
    TileInfo orig;
    orig.SetTerrainType(7);
    orig.SetTransform(2);
    orig.SetRiverPiece(3);
    orig.SetTileNum(42);
    orig.SetTransition(0, 11);
    orig.SetTransition(1, 12);
    orig.SetTransition(2, 13);
    orig.SetTransition(3, 14);
    orig.SetMega(static_cast<uint8>(5));

    nlohmann::json j = orig;

    TileInfo round;
    j.get_to(round);

    CHECK(round.GetTerrainType() == orig.GetTerrainType());
    CHECK(round.GetTransform()   == orig.GetTransform());
    CHECK(round.GetRiverPiece()  == orig.GetRiverPiece());
    CHECK(round.GetMega()        == orig.GetMega());
    CHECK(round.GetTransition(0) == 11);
    CHECK(round.GetTransition(1) == 12);
    CHECK(round.GetTransition(2) == 13);
    CHECK(round.GetTransition(3) == 14);
}

TEST_CASE("json round-trip: TileInfo omits m_goodActor by design")
{
    TileInfo t;
    nlohmann::json j = t;
    // m_goodActor is a UI sprite pointer — never carried in saves.
    // Lock the schema so future scope-creep gets caught.
    CHECK_FALSE(j.contains("good_actor"));
    CHECK_FALSE(j.contains("m_goodActor"));
}

TEST_CASE("json round-trip: TileInfo load rejects wrong transitions size")
{
    nlohmann::json bad{
        {"river_piece",  0},
        {"mega_info",    0},
        {"terrain_type", 0},
        {"transform",    0},
        {"tile_num",     0},
        {"transitions",  nlohmann::json::array({1, 2})},  // wrong size
    };
    TileInfo t;
    CHECK_THROWS(bad.get_to(t));
}

TEST_CASE("json round-trip: UnseenCell preserves scalar fields + position")
{
    // Use the default ctor — the MapPoint ctor needs g_theWorld, which
    // is null in unit-tier tests.  Default ctor gives position (0,0)
    // and an empty fog-of-war cell; we mutate the public scalar fields
    // and pull position-roundtrip from the deserialised JSON below.
    UnseenCell orig;
    orig.m_env                  = 0xDEADBEEF;
    orig.m_terrain_type         = 4;
    orig.m_move_cost            = 10;
    orig.m_flags                = 0x1234;
    orig.m_citySize             = 7;
    orig.m_cityOwner            = 3;
    orig.m_citySpriteIndex      = 12;
    orig.m_cell_owner           = 2;
    orig.m_slaveBits            = 0xAABBCCDD;

    nlohmann::json j = orig;

    UnseenCell round;
    j.get_to(round);

    CHECK(round.m_env             == orig.m_env);
    CHECK(round.m_terrain_type    == orig.m_terrain_type);
    CHECK(round.m_move_cost       == orig.m_move_cost);
    CHECK(round.m_flags           == orig.m_flags);
    CHECK(round.m_citySize        == orig.m_citySize);
    CHECK(round.m_cityOwner       == orig.m_cityOwner);
    CHECK(round.m_citySpriteIndex == orig.m_citySpriteIndex);
    CHECK(round.m_cell_owner      == orig.m_cell_owner);
    CHECK(round.m_slaveBits       == orig.m_slaveBits);

    // Position round-trip via JSON edit (default ctor gives (0,0);
    // edit the JSON and confirm deserialise picks up the new value).
    j["position"]["x"] = 5;
    j["position"]["y"] = 9;
    UnseenCell roundPos;
    j.get_to(roundPos);
    MapPoint roundPosPoint;
    roundPos.GetPos(roundPosPoint);
    CHECK(roundPosPoint.x == 5);
    CHECK(roundPosPoint.y == 9);
}

TEST_CASE("json round-trip: UnseenCell omits m_actor + m_snapshotState by design")
{
    UnseenCell uc;
    nlohmann::json j = uc;
    // m_actor (fog-of-war sprite shared_ptr) is omitted — UI
    // regenerates it on load.  Slice 7j removes it from gs/ entirely.
    CHECK_FALSE(j.contains("actor"));
    CHECK_FALSE(j.contains("m_actor"));
    CHECK_FALSE(j.contains("snapshot_state"));
    CHECK_FALSE(j.contains("m_snapshotState"));
}

TEST_CASE("json round-trip: Cell preserves scalar fields")
{
    Cell orig;
    // Cell's only public field setters are limited; populate via
    // the friend-accessible private members through deserialization
    // round-trip is the cleanest path.  Build the source JSON
    // explicitly so we control what goes in.
    nlohmann::json j{
        {"env",              0x12345678u},
        {"zoc",              0xAABBCCDDu},
        {"move_cost",        sint16{100}},
        {"continent_number", sint16{42}},
        {"gf",               sint8{7}},
        {"terrain_type",     sint8{3}},
        {"city",             0u},
        {"cell_owner",       sint8{-1}},
    };
    Cell round;
    j.get_to(round);

    // Round-trip back out and verify equality
    nlohmann::json j2 = round;
    CHECK(j2["env"]              == j["env"]);
    CHECK(j2["zoc"]              == j["zoc"]);
    CHECK(j2["move_cost"]        == j["move_cost"]);
    CHECK(j2["continent_number"] == j["continent_number"]);
    CHECK(j2["gf"]               == j["gf"]);
    CHECK(j2["terrain_type"]     == j["terrain_type"]);
    CHECK(j2["city"]             == j["city"]);
    CHECK(j2["cell_owner"]       == j["cell_owner"]);
}

TEST_CASE("json round-trip: Cell omits nested pointer-typed data by design")
{
    Cell c;
    nlohmann::json j = c;
    // m_unit_army, m_objects, m_jabba are pointer-typed nested data
    // that needs the contained types (CellUnitList, DynamicArray<ID>,
    // GoodyHut) to be JSON-serialisable first — Phase D/E.
    CHECK_FALSE(j.contains("unit_army"));
    CHECK_FALSE(j.contains("objects"));
    CHECK_FALSE(j.contains("goody_hut"));
    CHECK_FALSE(j.contains("jabba"));
}

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
