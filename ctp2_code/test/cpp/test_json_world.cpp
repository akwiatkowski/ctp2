// test/cpp/test_json_world.cpp
// Phase C-2 integration test for the JSON world block.
//
// Unit-tier tests can't construct a real World — World::AllocateMap
// touches g_theResourceDB, the binary save path depends on the same,
// and World's ctor pulls in the wraparound padding machinery.  The
// integration tier already runs a full headless game; we drive
// ctp2_headless with --json-save and verify the resulting JSON
// contains a populated, well-shaped "world" block.
//
// This locks in:
//   - The world block exists at top level
//   - Map dimensions match the actual world's size
//   - The dense cells[] array has size_x rows of size_y cells each
//   - tile_info_storage has size_x * size_y entries
//   - Each cell has the locked Phase C-1 key set
//   - The deliberately-omitted keys (m_actor / m_goodActor /
//     m_unit_army / etc.) still aren't present

#include "ctp/c3.h"
#include "doctest.h"
#include "headless_test_config.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/wait.h>
#include <fstream>
#include <sstream>
#include <string>
#include <nlohmann/json.hpp>

namespace {

// Run headless with the given args, return exit code; stderr+stdout
// merged into `out` for diagnostic context on failure.
int run_headless(char const *args, std::string &out)
{
    char const *bin = CTP2_HEADLESS_COMMAND;

    char cmd[1024];
    std::snprintf(cmd, sizeof(cmd), "%s %s 2>&1", bin, args);

    FILE *pipe = popen(cmd, "r");
    if (!pipe) return -1;

    char buf[512];
    while (std::fgets(buf, sizeof(buf), pipe)) out += buf;
    int status = pclose(pipe);
    return WEXITSTATUS(status);
}

// Save a JSON file via headless and parse it.  Returns parsed json
// or null on failure; the caller checks .is_null() before reading.
nlohmann::json save_and_parse_json(int seed, int turns, char const *path,
                                   int players = 3)
{
    std::remove(path);
    char args[512];
    std::snprintf(args, sizeof(args),
                  "--new-game --turns %d --players %d --seed %d "
                  "--json-save %s",
                  turns, players, seed, path);
    std::string log;
    int rc = run_headless(args, log);
    if (rc != 0)
    {
        INFO("headless exited non-zero (" << rc << "):\n" << log);
        return nullptr;
    }

    std::ifstream in(path);
    if (!in) { INFO("could not open " << path); return nullptr; }

    nlohmann::json doc;
    try { in >> doc; }
    catch (nlohmann::json::parse_error const &e)
    {
        INFO("JSON parse error: " << e.what());
        return nullptr;
    }
    return doc;
}

}  // namespace

// World JSON is megabytes — keep these tests in the integration tier
// so the unit-tier budget stays tight.
TEST_SUITE_BEGIN("integration");

TEST_CASE("json_save world block: present at top level, dense + well-shaped")
{
    nlohmann::json doc =
        save_and_parse_json(42, 2, "/tmp/ctp2_json_world_shape.json");

    REQUIRE_FALSE(doc.is_null());
    REQUIRE(doc.contains("world"));

    auto const &world = doc["world"];

    // Top-level world keys
    CHECK(world.contains("size_x"));
    CHECK(world.contains("size_y"));
    CHECK(world.contains("is_xwrap"));
    CHECK(world.contains("is_ywrap"));
    CHECK(world.contains("continents_are_numbered"));
    CHECK(world.contains("water_continent_max"));
    CHECK(world.contains("land_continent_max"));
    CHECK(world.contains("tile_info_storage"));
    CHECK(world.contains("cells"));
    CHECK(world.contains("num_civ_starts"));
    CHECK(world.contains("civ_starts"));
    CHECK(world.contains("good_value"));

    // Map dimensions are sane
    int const size_x = world["size_x"].get<int>();
    int const size_y = world["size_y"].get<int>();
    CHECK(size_x > 0);
    CHECK(size_y > 0);
    CHECK(size_x < 1000);  // sanity guard
    CHECK(size_y < 1000);

    // Dense cell array: [size_x][size_y]
    REQUIRE(world["cells"].is_array());
    CHECK(static_cast<int>(world["cells"].size()) == size_x);
    if (size_x > 0)
    {
        CHECK(static_cast<int>(world["cells"][0].size()) == size_y);
    }

    // tile_info_storage: flat row-major
    REQUIRE(world["tile_info_storage"].is_array());
    CHECK(static_cast<int>(world["tile_info_storage"].size())
          == size_x * size_y);

    // good_value: per-resource-record entries
    CHECK(world["good_value"].is_array());
    // Currently 55 in the bundled DB — but don't lock the exact count
    // because the resource DB grows over time.
    CHECK(static_cast<int>(world["good_value"].size()) > 0);
}

TEST_CASE("json_save world block: cells have the locked Phase C-1 key set")
{
    nlohmann::json doc =
        save_and_parse_json(42, 2, "/tmp/ctp2_json_world_cell_keys.json");
    REQUIRE_FALSE(doc.is_null());
    REQUIRE(doc.contains("world"));
    REQUIRE(doc["world"]["cells"].is_array());
    REQUIRE_FALSE(doc["world"]["cells"].empty());
    REQUIRE_FALSE(doc["world"]["cells"][0].empty());

    auto const &cell = doc["world"]["cells"][0][0];

    // Phase C-1 lock-in
    CHECK(cell.contains("env"));
    CHECK(cell.contains("zoc"));
    CHECK(cell.contains("move_cost"));
    CHECK(cell.contains("continent_number"));
    CHECK(cell.contains("gf"));
    CHECK(cell.contains("terrain_type"));
    CHECK(cell.contains("city"));
    CHECK(cell.contains("cell_owner"));

    // Pointer-typed nested data deferred to Phase D/E — must not leak
    // into Phase C-2 by accident.
    CHECK_FALSE(cell.contains("unit_army"));
    CHECK_FALSE(cell.contains("objects"));
    REQUIRE(cell.contains("goody_hut"));
    CHECK((cell["goody_hut"].is_null() || cell["goody_hut"].is_object()));
    CHECK_FALSE(cell.contains("jabba"));

    // No m_-prefix leakage (Decision #2 snake_case enforcement).
    for (auto const &el : cell.items())
    {
        CHECK(el.key().substr(0, 2) != "m_");
    }
}

TEST_CASE("json_save world block: tile_info_storage entries omit m_goodActor")
{
    nlohmann::json doc =
        save_and_parse_json(42, 2, "/tmp/ctp2_json_world_tile_keys.json");
    REQUIRE_FALSE(doc.is_null());
    REQUIRE(doc.contains("world"));
    REQUIRE(doc["world"]["tile_info_storage"].is_array());
    REQUIRE_FALSE(doc["world"]["tile_info_storage"].empty());

    auto const &tile = doc["world"]["tile_info_storage"][0];

    // Phase C-1 TileInfo keys
    CHECK(tile.contains("river_piece"));
    CHECK(tile.contains("mega_info"));
    CHECK(tile.contains("terrain_type"));
    CHECK(tile.contains("transform"));
    CHECK(tile.contains("tile_num"));
    CHECK(tile.contains("transitions"));

    // m_goodActor is a UI sprite pointer — NEVER in the JSON.
    CHECK_FALSE(tile.contains("good_actor"));
    CHECK_FALSE(tile.contains("m_goodActor"));
}

TEST_CASE("json_save world block: same seed -> identical world JSON")
{
    // Once Phase E lands UnitData / UnitActor amputation, the cell-
    // level JSON should be deterministic for same-seed runs.  Until
    // then, this is the strongest deterministic surface we can lock.
    // (Pre-A's full-file byte-compare is blocked on gameSetup vtable;
    //  this is the world-block-only equivalent.)
    nlohmann::json a =
        save_and_parse_json(42, 2, "/tmp/ctp2_json_world_seed_a.json");
    nlohmann::json b =
        save_and_parse_json(42, 2, "/tmp/ctp2_json_world_seed_b.json");

    REQUIRE_FALSE(a.is_null());
    REQUIRE_FALSE(b.is_null());

    CHECK(a["world"]["size_x"]    == b["world"]["size_x"]);
    CHECK(a["world"]["size_y"]    == b["world"]["size_y"]);
    CHECK(a["world"]["is_xwrap"]  == b["world"]["is_xwrap"]);
    CHECK(a["world"]["is_ywrap"]  == b["world"]["is_ywrap"]);
    CHECK(a["world"]["cells"]     == b["world"]["cells"]);
    CHECK(a["world"]["tile_info_storage"]
          == b["world"]["tile_info_storage"]);
    CHECK(a["world"]["good_value"] == b["world"]["good_value"]);
}

TEST_SUITE_END;
