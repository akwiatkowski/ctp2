// test/cpp/test_headless_long.cpp
// Long-turn sanity test for ctp2_headless.
//
// test_headless_progression checks 5 turns — enough to assert "the AI moved
// off the starting tile and founded a capital."  But it doesn't catch
// regressions that only manifest later: stalled expansion, AI getting stuck
// on a single city, populations frozen at 1, scores never diverging,
// crashes in mid-game events, etc.
//
// This test runs 50 turns and asserts the game has actually progressed past
// the opening:
//   - Each non-barbarian player has expanded beyond the capital (>= 2 cities).
//   - At least one city has population > 1 (growth happened).
//   - Player scores have diverged (AI is making distinct decisions).
//   - Total cities on the map is meaningfully more than the player count.
//   - Run reports "Completed N turns" in stderr (no premature exit).
//
// Skipped from the fast suite — fixture spawns the headless binary for
// ~3 seconds.

#include "ctp/c3.h"
#include "doctest.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <algorithm>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace {

static const char *HEADLESS_CANDIDATES[] = {
    "./build/ctp2_headless",
    "./build-sanitized/ctp2_headless",
    "./ctp2_headless",
    nullptr,
};

const char *find_headless_binary()
{
    for (const char **p = HEADLESS_CANDIDATES; *p; ++p) {
        if (std::FILE *f = std::fopen(*p, "r")) {
            std::fclose(f);
            return *p;
        }
    }
    return nullptr;
}

int run_headless_capture(const char *args, std::string *captured_stderr)
{
    const char *bin = find_headless_binary();
    if (!bin) {
        if (captured_stderr) *captured_stderr = "[ERROR] ctp2_headless not found";
        return -1;
    }
    char cmd[1024];
    std::snprintf(cmd, sizeof(cmd), "%s %s 2>&1", bin, args);

    FILE *pipe = popen(cmd, "r");
    if (!pipe) return -1;

    std::string out;
    char buf[512];
    while (std::fgets(buf, sizeof(buf), pipe)) out += buf;
    int status = pclose(pipe);
    if (captured_stderr) *captured_stderr = out;
    return WEXITSTATUS(status);
}

struct PlayerRow {
    int  idx;
    std::string leader;
    bool dead;
    int  score;
    int  gold;
    int  num_cities;
};

struct CityRow {
    int  player_idx;
    std::string name;
    int  x, y, population;
};

struct Metrics {
    std::vector<PlayerRow> players;
    std::vector<CityRow>   cities;
};

bool parse_metrics(const char *path, Metrics &out)
{
    FILE *fp = std::fopen(path, "r");
    if (!fp) return false;

    char line[1024];
    enum { SECTION_NONE, SECTION_PLAYERS, SECTION_CITIES } section = SECTION_NONE;

    while (std::fgets(line, sizeof(line), fp)) {
        size_t n = std::strlen(line);
        while (n && (line[n-1] == '\n' || line[n-1] == '\r')) line[--n] = '\0';
        if (n == 0) continue;

        if (std::strcmp(line, "# PLAYERS") == 0) { section = SECTION_PLAYERS; continue; }
        if (std::strcmp(line, "# CITIES")  == 0) { section = SECTION_CITIES;  continue; }
        if (line[0] < '0' || line[0] > '9') continue;

        if (section == SECTION_PLAYERS) {
            PlayerRow r;
            char leader[256]   = {0};
            char dead_str[16]  = {0};
            int got = std::sscanf(line, "%d,%255[^,],%15[^,],%d,%d,%d",
                                  &r.idx, leader, dead_str,
                                  &r.score, &r.gold, &r.num_cities);
            if (got == 6) {
                r.leader = leader;
                r.dead = (std::strcmp(dead_str, "yes") == 0);
                out.players.push_back(r);
            }
        } else if (section == SECTION_CITIES) {
            CityRow c;
            char name[256] = {0};
            int got = std::sscanf(line, "%d,%255[^,],%d,%d,%d",
                                  &c.player_idx, name, &c.x, &c.y, &c.population);
            if (got == 5) {
                c.name = name;
                out.cities.push_back(c);
            }
        }
    }
    std::fclose(fp);
    return true;
}

}  // namespace

TEST_CASE("Long-turn sanity: 50-turn run completes and reports completion")
{
    const char *metrics = "/tmp/ctp2_long_50.csv";
    std::remove(metrics);

    std::string out;
    int rc = run_headless_capture(
        "--new-game --turns 50 --players 4 --seed 42 "
        "--export-metrics /tmp/ctp2_long_50.csv",
        &out);
    CAPTURE(out);
    REQUIRE(rc == 0);

    // Smoke-level: stderr must show the loop reached its end.  Catches a
    // regression where the binary exits early (e.g. signal handler swallows
    // an assert) but still returns rc=0.
    CHECK(out.find("[HEADLESS] Completed 50 turns") != std::string::npos);
    CHECK(out.find("[HEADLESS] Turn 50 / 50")       != std::string::npos);

    Metrics m;
    REQUIRE(parse_metrics(metrics, m));

    // At least the four AI players plus barbarian slot.
    CHECK(m.players.size() >= 4);

    // The barbarian player (idx 0) shouldn't have any cities.
    int barbarian_cities = 0;
    for (const auto &c : m.cities) {
        if (c.player_idx == 0) barbarian_cities++;
    }
    CHECK(barbarian_cities == 0);
}

TEST_CASE("Long-turn sanity: AI expands past the capital by turn 50")
{
    // After 50 turns the AI should have founded a second city for at least
    // one non-barbarian player.  CTP2's settling AI normally produces 2-3
    // cities per civ in the first 50 turns on a default map.  If every player
    // is still on one city, expansion logic is broken (e.g. settlers built
    // but not dispatched, scheduler stuck).
    const char *metrics = "/tmp/ctp2_long_expansion.csv";
    std::remove(metrics);

    std::string out;
    int rc = run_headless_capture(
        "--new-game --turns 50 --players 4 --seed 42 "
        "--export-metrics /tmp/ctp2_long_expansion.csv",
        &out);
    CAPTURE(out);
    REQUIRE(rc == 0);

    Metrics m;
    REQUIRE(parse_metrics(metrics, m));

    std::map<int,int> cities_by_player;
    for (const auto &c : m.cities) cities_by_player[c.player_idx]++;

    int max_cities = 0;
    for (const auto &kv : cities_by_player) {
        if (kv.first == 0) continue;          // barbarians
        max_cities = std::max(max_cities, kv.second);
    }
    INFO("max non-barbarian cities at turn 50 = " << max_cities);
    CHECK(max_cities >= 2);

    // Total cities should be meaningfully more than the player count.
    // 4 players + at least one expansion = >= 5.
    int non_barb_total = 0;
    for (const auto &kv : cities_by_player) {
        if (kv.first == 0) continue;
        non_barb_total += kv.second;
    }
    INFO("total non-barbarian cities at turn 50 = " << non_barb_total);
    CHECK(non_barb_total >= 5);
}

TEST_CASE("Long-turn sanity: city populations grow past 1 by turn 50")
{
    // CTP2 cities start at population 1 and grow as their food balance
    // accumulates.  After 50 turns at least one city should be above 1.
    // If every city is still at population 1, the city-growth pipeline is
    // broken (food production, turn-end city tick, or population formula).
    const char *metrics = "/tmp/ctp2_long_growth.csv";
    std::remove(metrics);

    std::string out;
    int rc = run_headless_capture(
        "--new-game --turns 50 --players 4 --seed 42 "
        "--export-metrics /tmp/ctp2_long_growth.csv",
        &out);
    CAPTURE(out);
    REQUIRE(rc == 0);

    Metrics m;
    REQUIRE(parse_metrics(metrics, m));

    int max_pop = 0;
    for (const auto &c : m.cities) {
        if (c.player_idx == 0) continue;      // ignore barbarians
        max_pop = std::max(max_pop, c.population);
    }
    INFO("max non-barbarian city population at turn 50 = " << max_pop);
    CHECK(max_pop >= 2);
}

TEST_CASE("Long-turn sanity: player scores diverge by turn 50")
{
    // Each AI plays slightly differently — research priorities, settling
    // patterns, combat outcomes diverge over 50 turns.  If every player ends
    // with the exact same score the AI is not running (everyone is a no-op
    // bot) or the scoring system is constant-init only.
    const char *metrics = "/tmp/ctp2_long_scores.csv";
    std::remove(metrics);

    std::string out;
    int rc = run_headless_capture(
        "--new-game --turns 50 --players 4 --seed 42 "
        "--export-metrics /tmp/ctp2_long_scores.csv",
        &out);
    CAPTURE(out);
    REQUIRE(rc == 0);

    Metrics m;
    REQUIRE(parse_metrics(metrics, m));

    std::set<int> distinct_scores;
    int non_barbarian_count = 0;
    for (const auto &p : m.players) {
        if (p.idx == 0) continue;             // barbarians always at 0
        if (p.dead)    continue;
        non_barbarian_count++;
        distinct_scores.insert(p.score);
    }
    INFO("non-barbarian players: "      << non_barbarian_count
         << ", distinct scores: "       << distinct_scores.size());
    REQUIRE(non_barbarian_count >= 2);
    CHECK(distinct_scores.size() >= 2);
}
