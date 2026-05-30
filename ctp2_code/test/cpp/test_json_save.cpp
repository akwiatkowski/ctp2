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
