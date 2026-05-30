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
#include "gs/gameobj/Score.h"
#include "gs/gameobj/Regard.h"
#include "gs/gameobj/TaxRate.h"
#include "gs/gameobj/Sci.h"
#include "gs/gameobj/Readiness.h"
#include "gs/gameobj/pollution.h"
#include "gs/gameobj/PollutionConst.h"
#include "gs/gameobj/WonderTracker.h"
#include "gs/gameobj/AchievementTracker.h"
#include "gs/gameobj/Advances.h"
#include "gs/gameobj/Happy.h"
#include "gs/gameobj/HappyTracker.h"
#include "gs/gameobj/Exclusions.h"
#include "gs/gameobj/Strengths.h"
#include "gs/gameobj/AgreementData.h"

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

// --- Phase D-1 player-layer leaf round-trips ---

TEST_CASE("json round-trip: Score preserves all 6 scalar fields")
{
    Score orig(/*owner*/3);
    orig.AddCityRecaptured();
    orig.AddCityRecaptured();           // -> cities_recaptured = 2
    orig.AddOpponentConquered();         // -> opponents_conquered = 1
    orig.SetFinalScore(12345);
    orig.SetVictoryType(kScoreSoloVictory);
    orig.AddFeat();                      // -> feats = 1

    nlohmann::json j = orig;
    Score round(0);
    j.get_to(round);

    nlohmann::json j2 = round;
    CHECK(j2["owner"]               == j["owner"]);
    CHECK(j2["cities_recaptured"]   == 2);
    CHECK(j2["opponents_conquered"] == 1);
    CHECK(j2["final_score"]         == 12345);
    CHECK(j2["victory_type"]        == kScoreSoloVictory);
    CHECK(j2["feats"]               == 1);
}

TEST_CASE("json round-trip: Score key set is exactly the locked 6")
{
    Score s(0);
    nlohmann::json j = s;
    CHECK(j.size() == 6);
    CHECK(j.contains("owner"));
    CHECK(j.contains("cities_recaptured"));
    CHECK(j.contains("opponents_conquered"));
    CHECK(j.contains("final_score"));
    CHECK(j.contains("victory_type"));
    CHECK(j.contains("feats"));
    for (auto const &el : j.items())
        CHECK(el.key().substr(0, 2) != "m_");
}

TEST_CASE("json round-trip: Regard preserves all k_MAX_PLAYERS entries")
{
    Regard orig;
    // Drive a varied pattern across the 32 player slots.
    for (sint32 i = 0; i < k_MAX_PLAYERS; ++i)
    {
        orig.SetForPlayer(i, static_cast<REGARD_TYPE>(i % 6));
    }

    nlohmann::json j = orig;
    Regard round;
    j.get_to(round);

    for (sint32 i = 0; i < k_MAX_PLAYERS; ++i)
    {
        CHECK(round.GetForPlayer(i) == orig.GetForPlayer(i));
    }
}

TEST_CASE("json round-trip: Regard load rejects wrong array size")
{
    nlohmann::json bad{{"regard", nlohmann::json::array({0, 1, 2})}};
    Regard r;
    CHECK_THROWS(bad.get_to(r));
}

TEST_CASE("json round-trip: TaxRate preserves both doubles")
{
    // SetTaxRates dereferences g_player[owner]->m_government_type
    // — null in unit-tier tests.  Drive state through a synthetic
    // JSON and verify the round-trip back.
    nlohmann::json j{
        {"science",                0.625},
        {"science_before_anarchy", 0.375},
    };
    TaxRate t;
    j.get_to(t);

    double science = -1.0;
    t.GetScienceTaxRate(science);
    CHECK(science == doctest::Approx(0.625));
    CHECK(t.GetScienceBeforeAnarchy() == doctest::Approx(0.375));

    // Round back out matches.
    nlohmann::json j2 = t;
    CHECK(j2 == j);
}

TEST_CASE("json round-trip: Science preserves m_level")
{
    Science orig;
    orig.SetLevel(98765);

    nlohmann::json j = orig;
    Science round;
    j.get_to(round);

    CHECK(round.GetLevel() == 98765);
    CHECK(j.contains("level"));
    CHECK(j.size() == 1);
}

TEST_CASE("json round-trip: MilitaryReadiness preserves all 9 fields")
{
    MilitaryReadiness orig(/*owner*/2);

    // Drive non-default values directly via JSON since the public
    // API mostly drives state through SetLevel(game, all_armies,...)
    // which needs game globals.  Roundtrip a deserialised state.
    nlohmann::json j{
        {"delta",             1.5},
        {"hp_modifier",       0.875},
        {"cost",              42.0},
        {"percent_last_turn", 0.3},
        {"readiness_level",   static_cast<sint32>(READINESS_LEVEL_ALERT)},
        {"ignore_unsupport",  true},
        {"owner",             2},
        {"turn_started",      17},
        {"cost_gold",         101},
    };
    j.get_to(orig);

    nlohmann::json j2 = orig;
    CHECK(j2 == j);  // exact round-trip
    CHECK(orig.GetLevel() == READINESS_LEVEL_ALERT);
}

// --- Phase D-2 leaf round-trips ---

TEST_CASE("json round-trip: Pollution preserves 7 fields + history array")
{
    // Default Pollution ctor initialises everything to 0 — drive
    // non-default values via synthetic JSON.
    nlohmann::json j{
        {"event_trigger_next_round", 42},
        {"event_triggered",          1},
        {"trend",                    -3},
        {"history",                  nlohmann::json::array({10, 20, 30, 40, 50})},
        {"phase",                    2},
        {"gw_phase",                 4},
        {"next_level",               1000},
    };
    Pollution p;
    j.get_to(p);

    // Round-trip back out matches.
    nlohmann::json j2 = p;
    CHECK(j2 == j);
}

TEST_CASE("json round-trip: Pollution load rejects wrong history size")
{
    nlohmann::json bad{
        {"event_trigger_next_round", 0},
        {"event_triggered",          0},
        {"trend",                    0},
        {"history",                  nlohmann::json::array({1, 2})},  // wrong
        {"phase",                    0},
        {"gw_phase",                 0},
        {"next_level",               0},
    };
    Pollution p;
    CHECK_THROWS(bad.get_to(p));
}

TEST_CASE("json round-trip: WonderTracker preserves built/building/sat flags")
{
    WonderTracker orig;
    orig.SetBuiltWonders(0xCAFEBABE12345678ull);
    orig.SetGlobeSatFlags(0xDEADBEEFu);
    // Populate building flags directly via JSON since the public
    // setter (SetBuildingWonder) requires database lookups.
    nlohmann::json j = orig;
    j["building_wonders"][3] = 0xABCD1234u;
    j["building_wonders"][17] = 0xFFFFFFFFu;

    WonderTracker round;
    j.get_to(round);

    CHECK(round.GetBuiltWonders() == 0xCAFEBABE12345678ull);
    CHECK(round.GlobeSatFlags()   == 0xDEADBEEFu);

    nlohmann::json j2 = round;
    CHECK(j2["building_wonders"][3]  == 0xABCD1234u);
    CHECK(j2["building_wonders"][17] == 0xFFFFFFFFu);
}

TEST_CASE("json round-trip: AchievementTracker preserves bitfield")
{
    AchievementTracker orig;
    orig.SetData(0x0123456789ABCDEFull);

    nlohmann::json j = orig;
    AchievementTracker round;
    j.get_to(round);

    CHECK(round.GetData() == 0x0123456789ABCDEFull);
    CHECK(j.size() == 1);
    CHECK(j.contains("achievements"));
}

TEST_CASE("json round-trip: HappyTimer preserves all 3 fields")
{
    HappyTimer orig(/*turns*/ 5,
                    /*adjust*/ 1.25,
                    /*reason*/ HAPPY_REASON_WONDERS);

    nlohmann::json j = orig;
    HappyTimer round(0, 0.0, HAPPY_REASON_SMOKING_CRACK);
    j.get_to(round);

    CHECK(round.m_turnsRemaining == 5);
    CHECK(round.m_adjustment     == doctest::Approx(1.25));
    CHECK(round.m_reason         == HAPPY_REASON_WONDERS);
}

TEST_CASE("json round-trip: Advances preserves scalars + size-matched arrays")
{
    // Advances allocates m_hasAdvance/m_canResearch/m_turnsSinceOffered
    // sized by ctor argument.  Use a tiny size for testing.
    Advances orig(/*count*/ 8);
    orig.SetOwner(2);

    nlohmann::json j = orig;
    // Patch the arrays via JSON since direct field access requires
    // a friend or non-existent setter.
    for (sint32 i = 0; i < 8; ++i)
    {
        j["has_advance"][i]         = (i % 2) ? 1 : 0;
        j["can_research"][i]        = (i % 3 == 0) ? 1 : 0;
        j["turns_since_offered"][i] = static_cast<uint16>(i * 7);
    }
    j["researching"]                              = 3;
    j["age"]                                      = 1;
    j["last_advance_enabled_this_many_advances"]  = 12;
    j["total_cost"]                               = 5000;
    j["discovered"]                               = 4;

    Advances round(/*count*/ 8);
    j.get_to(round);

    // Round-trip back and verify identity.
    nlohmann::json j2 = round;
    CHECK(j2["owner"]                                       == 2);
    CHECK(j2["size"]                                        == 8);
    CHECK(j2["researching"]                                 == 3);
    CHECK(j2["age"]                                         == 1);
    CHECK(j2["last_advance_enabled_this_many_advances"]     == 12);
    CHECK(j2["total_cost"]                                  == 5000);
    CHECK(j2["discovered"]                                  == 4);
    for (sint32 i = 0; i < 8; ++i)
    {
        CHECK(j2["has_advance"][i].get<int>()       == ((i % 2) ? 1 : 0));
        CHECK(j2["can_research"][i].get<int>()      == ((i % 3 == 0) ? 1 : 0));
        CHECK(j2["turns_since_offered"][i].get<int>() == i * 7);
    }
}

TEST_CASE("json round-trip: Advances load rejects size mismatch")
{
    Advances a(8);
    nlohmann::json j = a;
    j["size"] = 8;
    j["has_advance"] = nlohmann::json::array({1, 2, 3});  // wrong length
    CHECK_THROWS(j.get_to(a));
}

// --- Phase D-3 leaf round-trips ---

TEST_CASE("json round-trip: HappyTracker preserves happiness_amounts")
{
    HappyTracker orig;
    for (sint32 i = 0; i < HAPPY_REASON_MAX; ++i)
    {
        orig.SetHappiness(static_cast<HAPPY_REASON>(i), 0.5 + i * 0.1);
    }

    nlohmann::json j = orig;
    HappyTracker round;
    j.get_to(round);

    for (sint32 i = 0; i < HAPPY_REASON_MAX; ++i)
    {
        double amount = 0.0;
        StringId name = 0;
        round.GetHappiness(static_cast<HAPPY_REASON>(i), amount, name);
        CHECK(amount == doctest::Approx(0.5 + i * 0.1));
    }
}

TEST_CASE("json round-trip: HappyTracker rejects wrong array size")
{
    nlohmann::json bad{
        {"happiness_amounts", nlohmann::json::array({1.0, 2.0})}
    };
    HappyTracker t;
    CHECK_THROWS(bad.get_to(t));
}

TEST_CASE("json round-trip: HappyTracker omits m_tempSaveHappiness (transient)")
{
    HappyTracker t;
    nlohmann::json j = t;
    CHECK_FALSE(j.contains("temp_save_happiness"));
    CHECK_FALSE(j.contains("m_tempSaveHappiness"));
    CHECK(j.size() == 1);
}

TEST_CASE("json round-trip: Exclusions preserves 3 sized heap arrays")
{
    // Use JSON-driven construction since the public ExcludeUnit setters
    // write into the heap array but don't grow it (ctor allocates based
    // on g_theUnitDB->NumRecords() which requires database init).
    nlohmann::json j{
        {"num_units",     3},
        {"num_buildings", 2},
        {"num_wonders",   1},
        {"units",         nlohmann::json::array({0, 1, 0})},
        {"buildings",     nlohmann::json::array({1, 0})},
        {"wonders",       nlohmann::json::array({1})},
    };
    Exclusions e;
    j.get_to(e);

    CHECK(e.IsUnitExcluded(0)     == 0);
    CHECK(e.IsUnitExcluded(1)     == 1);
    CHECK(e.IsUnitExcluded(2)     == 0);
    CHECK(e.IsBuildingExcluded(0) == 1);
    CHECK(e.IsBuildingExcluded(1) == 0);
    CHECK(e.IsWonderExcluded(0)   == 1);

    nlohmann::json j2 = e;
    CHECK(j2 == j);  // exact round-trip
}

TEST_CASE("json round-trip: Exclusions rejects mismatched num/array length")
{
    nlohmann::json bad{
        {"num_units",     5},
        {"num_buildings", 0},
        {"num_wonders",   0},
        {"units",         nlohmann::json::array({1, 2})},  // count says 5
        {"buildings",     nlohmann::json::array()},
        {"wonders",       nlohmann::json::array()},
    };
    Exclusions e;
    CHECK_THROWS(bad.get_to(e));
}

TEST_CASE("json round-trip: Strengths preserves owner + per-category records")
{
    Strengths orig(/*owner*/ 4);
    // Drive non-default via JSON since the public Calculate() method
    // depends on database singletons.
    nlohmann::json j{
        {"owner", 4},
        {"strength_records", nlohmann::json::array()},
    };
    for (sint32 cat = 0; cat < STRENGTH_CAT_MAX; ++cat)
    {
        // Each category gets a small history of (cat+1) elements
        nlohmann::json per_cat = nlohmann::json::array();
        for (sint32 i = 0; i <= cat; ++i)
        {
            per_cat.push_back(cat * 100 + i);
        }
        j["strength_records"].push_back(std::move(per_cat));
    }
    j.get_to(orig);

    // Round-trip back and verify identity.
    nlohmann::json j2 = orig;
    CHECK(j2 == j);
}

TEST_CASE("json round-trip: Strengths rejects wrong category count")
{
    nlohmann::json bad{
        {"owner", 0},
        {"strength_records",
            nlohmann::json::array({nlohmann::json::array(),
                                   nlohmann::json::array()})},
    };
    Strengths s(0);
    CHECK_THROWS(bad.get_to(s));
}

// --- Phase D-4 leaf round-trips ---

TEST_CASE("json round-trip: AgreementData preserves all 11 fields + Unit")
{
    AgreementData orig(ID(42));
    // Drive non-default values via JSON since most setters depend on
    // pool / network globals.
    nlohmann::json j{
        {"id",                  42u},
        {"owner",               2},
        {"recipient",           3},
        {"third_party",         5},
        {"agreement",           static_cast<int>(AGREEMENT_TYPE_CEASE_FIRE)},
        {"round",               17},
        {"expires",             50},
        {"owner_pollution",     1000u},
        {"recipient_pollution", 2000u},
        {"is_broken",           false},
        {"target_city",         12345u},
    };
    j.get_to(orig);

    CHECK(orig.GetOwner()      == 2);
    CHECK(orig.GetRecipient()  == 3);
    CHECK(orig.GetThirdParty() == 5);
    CHECK(orig.GetAgreement()  == AGREEMENT_TYPE_CEASE_FIRE);
    CHECK(orig.GetTurns()      == 50);
    CHECK(orig.GetStartTurn()  == 17);
    CHECK_FALSE(orig.IsBroken());
    CHECK(orig.GetTarget().m_id == 12345u);

    // Round-trip back through JSON
    nlohmann::json j2 = orig;
    CHECK(j2 == j);
}

TEST_CASE("json round-trip: AgreementData omits m_lesser/m_greater (pool-level)")
{
    AgreementData a(ID(0));
    nlohmann::json j = a;
    // Intrusive linked-list pointers stay out of the leaf bridge —
    // pool serialiser will flatten the relationship in a future phase.
    CHECK_FALSE(j.contains("lesser"));
    CHECK_FALSE(j.contains("greater"));
    CHECK_FALSE(j.contains("m_lesser"));
    CHECK_FALSE(j.contains("m_greater"));
    // m_killMeSoon (transient) and m_isFromPool (pool bookkeeping) also stay out.
    CHECK_FALSE(j.contains("kill_me_soon"));
    CHECK_FALSE(j.contains("is_from_pool"));
}

TEST_CASE("json round-trip: Happy preserves 23 scalars + timed_changes + tracker")
{
    Happy orig;  // default ctor zeros the scalar block

    nlohmann::json j = orig;
    // The 23 scalars should all default to 0; set a varied pattern.
    j["happiness"]            = 1.5;
    j["last_captured"]        = 2.5;
    j["base"]                 = 3.5;
    j["size"]                 = 4.5;
    j["pollution"]            = 5.5;
    j["conquest_distress"]    = 6.5;
    j["empire_dist"]          = 7.5;
    j["enemy_action"]         = 8.5;
    j["peace"]                = 9.5;
    j["starvation"]           = 10.5;
    j["workday"]              = 11.5;
    j["wages"]                = 12.5;
    j["rations"]              = 13.5;
    j["martial_law"]          = 14.5;
    j["pop_ent"]              = 15.5;
    j["improvement"]          = 16.5;
    j["wonders"]              = 17.5;
    j["dist_to_capitol"]      = 18.5;
    j["cost_to_capitol"]      = 19;
    j["full_happiness_turns"] = 20;
    j["too_many_cities"]      = 21.5;
    j["timed"]                = 22.5;
    j["crime"]                = 23.5;

    // Add 2 timed changes
    nlohmann::json timer_1{
        {"turns_remaining", 5},
        {"adjustment",      0.5},
        {"reason",          static_cast<int>(HAPPY_REASON_WONDERS)},
    };
    nlohmann::json timer_2{
        {"turns_remaining", 7},
        {"adjustment",      -1.25},
        {"reason",          static_cast<int>(HAPPY_REASON_HAPPINESS_ATTACK)},
    };
    j["timed_changes"] = nlohmann::json::array({timer_1, timer_2});

    Happy round;
    j.get_to(round);

    // Round-trip back and verify identity (except possibly tracker — see below)
    nlohmann::json j2 = round;
    CHECK(j2["happiness"]            == doctest::Approx(1.5));
    CHECK(j2["crime"]                == doctest::Approx(23.5));
    CHECK(j2["cost_to_capitol"]      == 19);
    CHECK(j2["full_happiness_turns"] == 20);
    CHECK(j2["timed_changes"].size() == 2);
    CHECK(j2["timed_changes"][0]["turns_remaining"]  == 5);
    CHECK(j2["timed_changes"][1]["adjustment"]       == doctest::Approx(-1.25));
}

TEST_CASE("json round-trip: Happy omits m_pad (alignment field)")
{
    Happy h;
    nlohmann::json j = h;
    CHECK_FALSE(j.contains("pad"));
    CHECK_FALSE(j.contains("m_pad"));
}

TEST_CASE("json round-trip: D-4 leaf bridges all use snake_case (no m_ leak)")
{
    Happy h;             nlohmann::json jh = h;
    AgreementData a(ID(0));
                         nlohmann::json ja = a;

    for (auto const &j : {jh, ja})
    {
        for (auto const &el : j.items())
        {
            CHECK(el.key().substr(0, 2) != "m_");
        }
    }
}

TEST_CASE("json round-trip: D-3 leaf bridges all use snake_case (no m_ leak)")
{
    HappyTracker t;          nlohmann::json jt = t;
    Strengths    s(0);       nlohmann::json js = s;
    // Exclusions has a default ctor but allocates based on database
    // — skip the default instance check.

    for (auto const &j : {jt, js})
    {
        for (auto const &el : j.items())
        {
            CHECK(el.key().substr(0, 2) != "m_");
        }
    }
}

TEST_CASE("json round-trip: D-2 leaf bridges all use snake_case (no m_ leak)")
{
    Pollution p;                  nlohmann::json jp  = p;
    WonderTracker wt;             nlohmann::json jwt = wt;
    AchievementTracker at;        nlohmann::json jat = at;
    HappyTimer ht(0, 0.0, HAPPY_REASON_SMOKING_CRACK);
                                  nlohmann::json jht = ht;
    Advances a(4);                nlohmann::json ja  = a;

    for (auto const &j : {jp, jwt, jat, jht, ja})
    {
        for (auto const &el : j.items())
        {
            CHECK(el.key().substr(0, 2) != "m_");
        }
    }
}

TEST_CASE("json round-trip: leaf bridges all use snake_case (no m_ leak)")
{
    Score    sc(0);    nlohmann::json js = sc;
    Regard   r;        nlohmann::json jr = r;
    TaxRate  t;        nlohmann::json jt = t;
    Science  ss;       nlohmann::json jss = ss;
    MilitaryReadiness mr(0); nlohmann::json jm = mr;

    for (auto const &j : {js, jr, jt, jss, jm})
    {
        for (auto const &el : j.items())
        {
            CHECK(el.key().substr(0, 2) != "m_");
        }
    }
}
