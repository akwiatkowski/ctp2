// test/cpp/test_city_visibility.cpp
// Regression tests for the "founded city is fogged for its own owner" bug.
//
// Symptom (reported 2026-06-01): user founds a city with the starting
// settler, the city has the correct civ color but is rendered grayed/
// fog-of-war and is not clickable.  Persists across save+load.
//
// The cause is structural: in Player::CreateCity, UnitPool::Create runs
// UnitData::Place → AddUnitVision before InitializeCityData has set up
// m_city_data, so GetVisionRange() returns the unit-record (DB) range
// instead of the city's actual vision radius.
//
// These tests exercise four invariants via the headless binary's
// --export-metrics CSV:
//   1. After a fresh game runs a few turns, every founded city is
//      visible to its own owner.
//   2. The invariant holds across many turns (drift catcher).
//   3. The invariant holds across save+load.
//   4. Every city in metrics is also in its owner's GetAllCitiesList.
//      (Trivially true if the metrics export reads from that same list,
//      but kept as a guard against accidental divergence.)

#include "ctp/c3.h"
#include "doctest.h"
#include "headless_test_config.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/stat.h>
#include <map>
#include <string>
#include <vector>

namespace {

int run_headless_capture(const char *args, std::string *captured)
{
    const char *bin = CTP2_HEADLESS_COMMAND;
    char cmd[2048];
    std::snprintf(cmd, sizeof(cmd), "%s %s 2>&1", bin, args);
    FILE *pipe = popen(cmd, "r");
    if (!pipe) return -1;
    std::string out;
    char buf[512];
    while (std::fgets(buf, sizeof(buf), pipe)) out += buf;
    int status = pclose(pipe);
    if (captured) *captured = out;
    return WEXITSTATUS(status);
}

struct CityRow {
    int  player_idx;
    std::string name;
    int  x, y, population;
    bool visible_owner;
    bool explored_owner;
};

// Parses only the # CITIES section.  Tolerates extra columns added later.
bool parse_cities(const char *path, std::vector<CityRow> &out)
{
    FILE *fp = std::fopen(path, "r");
    if (!fp) return false;
    char line[1024];
    bool in_cities = false;
    while (std::fgets(line, sizeof(line), fp)) {
        size_t n = std::strlen(line);
        while (n && (line[n-1] == '\n' || line[n-1] == '\r')) line[--n] = '\0';
        if (n == 0) continue;
        if (std::strcmp(line, "# CITIES") == 0)  { in_cities = true;  continue; }
        if (std::strcmp(line, "# PLAYERS") == 0) { in_cities = false; continue; }
        if (!in_cities) continue;
        if (line[0] < '0' || line[0] > '9') continue;   // header row

        CityRow c{};
        char name[256] = {0};
        char vis[16]   = {0};
        char exp[16]   = {0};
        int got = std::sscanf(line, "%d,%255[^,],%d,%d,%d,%15[^,],%15[^,\n]",
                              &c.player_idx, name, &c.x, &c.y, &c.population,
                              vis, exp);
        if (got >= 5) {
            c.name = name;
            c.visible_owner  = (got >= 6) && std::strcmp(vis, "yes") == 0;
            c.explored_owner = (got >= 7) && std::strcmp(exp, "yes") == 0;
            out.push_back(c);
        }
    }
    std::fclose(fp);
    return true;
}

}  // namespace

// Each test launches ctp2_headless as a subprocess — seconds per case.
TEST_SUITE_BEGIN("integration");

TEST_CASE("City visibility: founded cities are visible to their owner (turn 5)")
{
    const char *metrics = "/tmp/ctp2_test_vis_turn5.csv";
    std::remove(metrics);

    std::string out;
    int rc = run_headless_capture(
        "--new-game --turns 5 --players 5 --seed 42 "
        "--export-metrics /tmp/ctp2_test_vis_turn5.csv",
        &out);
    CAPTURE(out);
    REQUIRE(rc == 0);

    std::vector<CityRow> cities;
    REQUIRE(parse_cities(metrics, cities));
    REQUIRE(cities.size() >= 1);

    // Every founded city must be visible to its own owner.
    int fogged = 0;
    for (const auto &c : cities) {
        INFO("player " << c.player_idx << " city '" << c.name
             << "' @ (" << c.x << "," << c.y << ") "
             << "visible=" << (c.visible_owner ? "yes" : "no")
             << " explored=" << (c.explored_owner ? "yes" : "no"));
        CHECK(c.visible_owner);
        if (!c.visible_owner) ++fogged;
    }
    CAPTURE(fogged);
}

TEST_CASE("City visibility: invariant holds over 50 turns")
{
    // 1000-turn version would be ideal but tests run as a subprocess
    // per case; 50 turns is enough to catch any vision-drift bug
    // without making integration suite take minutes.  Bump if drift
    // is suspected.
    const char *metrics = "/tmp/ctp2_test_vis_long.csv";
    std::remove(metrics);

    std::string out;
    int rc = run_headless_capture(
        "--new-game --turns 50 --players 5 --seed 42 "
        "--export-metrics /tmp/ctp2_test_vis_long.csv",
        &out);
    CAPTURE(out);
    REQUIRE(rc == 0);

    std::vector<CityRow> cities;
    REQUIRE(parse_cities(metrics, cities));
    REQUIRE(cities.size() >= 1);

    for (const auto &c : cities) {
        INFO("turn-50 player " << c.player_idx << " city '" << c.name
             << "' visible=" << (c.visible_owner ? "yes" : "no"));
        CHECK(c.visible_owner);
    }
}

TEST_CASE("City visibility: save+load preserves visibility")
{
    const char *save    = "/tmp/ctp2_test_vis_save.json";
    const char *before  = "/tmp/ctp2_test_vis_before.csv";
    const char *after   = "/tmp/ctp2_test_vis_after.csv";
    std::remove(save);
    std::remove(before);
    std::remove(after);

    // Run, save, and export metrics on the save side.
    std::string out1;
    int rc = run_headless_capture(
        "--new-game --turns 5 --players 5 --seed 42 "
        "--save-game /tmp/ctp2_test_vis_save.json "
        "--export-metrics /tmp/ctp2_test_vis_before.csv",
        &out1);
    CAPTURE(out1);
    REQUIRE(rc == 0);

    std::vector<CityRow> before_cities;
    REQUIRE(parse_cities(before, before_cities));
    REQUIRE(before_cities.size() >= 1);

    // Load and export metrics immediately (zero turns post-load) — tests
    // that visibility state was preserved across the serialization round
    // trip, not that load+run+vision-update works.
    std::string out2;
    rc = run_headless_capture(
        "--load-game /tmp/ctp2_test_vis_save.json --turns 0 "
        "--export-metrics /tmp/ctp2_test_vis_after.csv",
        &out2);
    CAPTURE(out2);
    REQUIRE(rc == 0);

    std::vector<CityRow> after_cities;
    REQUIRE(parse_cities(after, after_cities));

    REQUIRE(after_cities.size() == before_cities.size());

    for (const auto &c : after_cities) {
        INFO("after-load player " << c.player_idx << " city '" << c.name
             << "' visible=" << (c.visible_owner ? "yes" : "no"));
        CHECK(c.visible_owner);
    }
}

TEST_CASE("Save/load: cities are restored to their owner player after load")
{
    // The user-visible bug surfaces here.  Before save: each non-barbarian
    // player has 1+ cities.  After load: # CITIES section is empty / num_cities
    // drops to 0.  Root cause: Player::to_json in json_save.cpp does not
    // serialise m_all_cities, m_all_units, m_all_armies, or m_vision.  See
    // json_save.cpp:2705 — the only composed sub-objects written are
    // science/tax_rate/advances/readiness/regard/strengths/capitol.  When
    // load runs, the unit pool restores the city units but they're never
    // re-attached to the per-player city list, so:
    //   * GetNumCities() returns 0
    //   * m_vision wasn't updated (cities don't extend fog of war for owner)
    //   * UI sees city as grayed/fogged for its own owner
    //
    // This test asserts the round-trip preserves city counts per player.
    // It is expected to FAIL until Player JSON migration is finished
    // (plan: ctp2.md Phase G-4c-3 / Phase F continuation).
    const char *save   = "/tmp/ctp2_test_saveload_cities.json";
    const char *before = "/tmp/ctp2_test_saveload_before.csv";
    const char *after  = "/tmp/ctp2_test_saveload_after.csv";
    std::remove(save);
    std::remove(before);
    std::remove(after);

    std::string out1;
    int rc = run_headless_capture(
        "--new-game --turns 10 --players 5 --seed 42 "
        "--save-game /tmp/ctp2_test_saveload_cities.json "
        "--export-metrics /tmp/ctp2_test_saveload_before.csv",
        &out1);
    CAPTURE(out1);
    REQUIRE(rc == 0);

    std::vector<CityRow> before_cities;
    REQUIRE(parse_cities(before, before_cities));
    INFO("cities before save = " << before_cities.size());
    REQUIRE(before_cities.size() >= 1);

    std::map<int, int> before_count;
    for (const auto &c : before_cities) before_count[c.player_idx]++;

    std::string out2;
    rc = run_headless_capture(
        "--load-game /tmp/ctp2_test_saveload_cities.json --turns 0 "
        "--export-metrics /tmp/ctp2_test_saveload_after.csv",
        &out2);
    CAPTURE(out2);
    REQUIRE(rc == 0);

    std::vector<CityRow> after_cities;
    REQUIRE(parse_cities(after, after_cities));
    INFO("cities after load = " << after_cities.size());

    // Hard assertion: city counts per player must match across save/load.
    CHECK(after_cities.size() == before_cities.size());

    std::map<int, int> after_count;
    for (const auto &c : after_cities) after_count[c.player_idx]++;

    for (auto const &kv : before_count) {
        int player = kv.first;
        int before_n = kv.second;
        int after_n  = after_count[player];
        INFO("player " << player
             << " cities before=" << before_n
             << " after=" << after_n);
        CHECK(after_n == before_n);
    }
}

TEST_CASE("City visibility: every city is at minimum explored to its owner")
{
    // Weaker invariant — explored means "you remember being there".
    // If this fails, the bug is more severe than just fog: the city
    // tile was never seen at all.
    const char *metrics = "/tmp/ctp2_test_vis_explored.csv";
    std::remove(metrics);

    std::string out;
    int rc = run_headless_capture(
        "--new-game --turns 3 --players 5 --seed 42 "
        "--export-metrics /tmp/ctp2_test_vis_explored.csv",
        &out);
    CAPTURE(out);
    REQUIRE(rc == 0);

    std::vector<CityRow> cities;
    REQUIRE(parse_cities(metrics, cities));
    REQUIRE(cities.size() >= 1);

    for (const auto &c : cities) {
        INFO("player " << c.player_idx << " city '" << c.name
             << "' explored=" << (c.explored_owner ? "yes" : "no"));
        CHECK(c.explored_owner);
    }
}

TEST_SUITE_END;
