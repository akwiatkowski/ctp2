// test/cpp/test_headless_progression.cpp
// Sanity tests that headless game-state actually evolves over time.
//
// Existing tests (test_headless_smoke) check the lifecycle wrapper:
//   process exits cleanly, "Turn N" lines log, no sanitizer errors.
// That's necessary but not sufficient.  If the AI never makes decisions,
// turns tick but nothing happens — barbarians stay 0/500, every other
// player stays at score 500 with no cities, forever.
//
// These tests parse the --export-metrics CSV and assert the simulation
// actually progresses: every non-barbarian player should have at least
// one founded city by turn 5 (the first AI settler typically founds
// a capital on turn 1-2 in CTP2).

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

int run_headless_capture(const char *args, std::string *captured_stderr)
{
    const char *bin = CTP2_HEADLESS_COMMAND;

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
        // Strip trailing newline.
        size_t n = std::strlen(line);
        while (n && (line[n-1] == '\n' || line[n-1] == '\r')) line[--n] = '\0';
        if (n == 0) continue;

        if (std::strcmp(line, "# PLAYERS") == 0) { section = SECTION_PLAYERS; continue; }
        if (std::strcmp(line, "# CITIES")  == 0) { section = SECTION_CITIES;  continue; }
        // Header rows start with text fields, not digits.
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

// Each test in this file launches ctp2_headless as a subprocess and
// runs real turns — seconds per case.  Tagged "integration" so the
// default `unit` meson target excludes them; run via the `integration`
// meson target or with --test-suite=integration.
TEST_SUITE_BEGIN("integration");

TEST_CASE("Headless progression: metrics export contains all players")
{
    const char *metrics = "/tmp/ctp2_test_prog_players.csv";
    std::remove(metrics);

    std::string out;
    int rc = run_headless_capture(
        "--new-game --turns 5 --players 5 --seed 42 "
        "--export-metrics /tmp/ctp2_test_prog_players.csv",
        &out);
    CAPTURE(out);
    REQUIRE(rc == 0);

    Metrics m;
    REQUIRE(parse_metrics(metrics, m));

    // Expect at least the 5 AI players + barbarian (idx 0). CTP2 always
    // creates the barbarian slot, so we get up to k_MAX_PLAYERS rows but
    // typically 6 active.
    CHECK(m.players.size() >= 5);

    // Each player has a non-empty leader name.
    for (const auto &p : m.players) {
        INFO("player " << p.idx << " leader='" << p.leader << "'");
        CHECK(!p.leader.empty());
    }
}

TEST_CASE("Headless progression: every non-barbarian player has a city by turn 5")
{
    // CTP2 starts each non-barbarian player with a Settler unit and an AI
    // that founds the capital on turn 1 or 2.  By turn 5 every player
    // should have at least one city.  If they don't, the AI/turn pipeline
    // is broken (turns advance but no decisions are dispatched).
    const char *metrics = "/tmp/ctp2_test_prog_cities.csv";
    std::remove(metrics);

    std::string out;
    int rc = run_headless_capture(
        "--new-game --turns 5 --players 5 --seed 42 "
        "--export-metrics /tmp/ctp2_test_prog_cities.csv",
        &out);
    CAPTURE(out);
    REQUIRE(rc == 0);

    Metrics m;
    REQUIRE(parse_metrics(metrics, m));

    // Build per-player city count map.
    std::map<int, int> cities_by_player;
    for (const auto &c : m.cities) cities_by_player[c.player_idx]++;

    // Barbarians (player 0 in CTP2) don't found cities — exclude.
    int non_barbarian_players_checked = 0;
    for (const auto &p : m.players) {
        if (p.idx == 0) continue;            // barbarians
        if (p.dead)    continue;
        non_barbarian_players_checked++;

        int n = cities_by_player[p.idx];
        INFO("player " << p.idx << " leader='" << p.leader
             << "' num_cities(reported)=" << p.num_cities
             << " num_cities(actual)=" << n);
        CHECK(n >= 1);
    }
    // Sanity: we actually checked something.
    CHECK(non_barbarian_players_checked >= 1);
}

TEST_SUITE_END;
