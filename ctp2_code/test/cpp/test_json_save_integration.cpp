// test/cpp/test_json_save_integration.cpp
//
// Integration smoke test for the composite SaveJson path (Phase F-18).
// Launches ctp2_headless --json-save against a fresh game, then verifies
// the resulting JSON document has every expected top-level key.
//
// LoadJson is intentionally NOT exercised here — the symmetric load path
// lands in a follow-up session that introduces nlohmann/json constructors
// for the singletons that currently only accept CivArchive&.  This file
// proves that the Save side of the round-trip is producing a structurally
// complete document.
//
// Runs in the "integration" suite — opt-in via:
//   meson test -C build integration
// or:
//   ./build/ctp2_fast_tests --test-suite=integration
//
// See ~/projects/claude/plans/ctp2-json-savegame.md.

#include "ctp/c3.h"
#include "doctest.h"
#include <nlohmann/json.hpp>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>
#include <sys/wait.h>

namespace {

static const char *HEADLESS_CANDIDATES[] = {
    "./build/ctp2_headless",
    "./build-sanitized/ctp2_headless",
    "./ctp2_headless",
    nullptr,
};

const char *find_headless()
{
    for (const char **p = HEADLESS_CANDIDATES; *p; ++p) {
        if (std::FILE *f = std::fopen(*p, "r")) {
            std::fclose(f);
            return *p;
        }
    }
    return nullptr;
}

bool run_save(const char *json_path, std::string *log)
{
    const char *bin = find_headless();
    if (!bin) {
        if (log) *log = "[ERROR] ctp2_headless not found";
        return false;
    }
    char cmd[1024];
    std::snprintf(cmd, sizeof(cmd),
                  "%s --new-game --turns 3 --players 3 --seed 42 "
                  "--json-save %s 2>&1",
                  bin, json_path);
    std::FILE *pipe = popen(cmd, "r");
    if (!pipe) return false;
    char buf[512];
    std::string out;
    while (std::fgets(buf, sizeof(buf), pipe)) out += buf;
    int status = pclose(pipe);
    if (log) *log = out;
    return WIFEXITED(status) && WEXITSTATUS(status) == 0;
}

bool read_file(const char *path, std::string &out)
{
    std::ifstream in(path);
    if (!in) return false;
    std::ostringstream ss;
    ss << in.rdbuf();
    out = ss.str();
    return true;
}

}  // namespace

TEST_SUITE_BEGIN("integration");

TEST_CASE("SaveJson composite: full game state writes all expected top-level keys")
{
    const char *path = "/tmp/ctp2_json_save_integ.json";
    std::remove(path);

    std::string log;
    REQUIRE(run_save(path, &log));

    std::string raw;
    REQUIRE(read_file(path, raw));
    CHECK(raw.size() > 1024);  // > 1 KB — anything smaller means we lost a section

    nlohmann::json doc;
    REQUIRE_NOTHROW(doc = nlohmann::json::parse(raw));

    // Header
    CHECK(doc["magic"] == "CTP2-JSON");
    CHECK(doc["schema_version"] == 1);
    CHECK(doc.contains("saved_at"));
    CHECK(doc.contains("ctp2_build"));

    // Core singletons
    for (const char *key : {"rng", "settings", "world", "turn", "selection"}) {
        INFO("missing top-level key: " << key);
        CHECK(doc.contains(key));
    }

    // Pools (Phase E + F bridges, wired into SaveJson in F-18)
    for (const char *key : {
        "unit_pool", "army_pool", "trade_pool", "pollution",
        "slic_engine", "terrain_improvement_pool", "civilisation_pool",
        "message_pool", "installation_pool"
    }) {
        INFO("missing pool key: " << key);
        CHECK(doc.contains(key));
    }

    // Trackers
    for (const char *key : {
        "wonder_tracker", "exclusions", "feat_tracker", "event_tracker",
        "top_ten"
    }) {
        INFO("missing tracker key: " << key);
        CHECK(doc.contains(key));
    }

    // Player array — k_MAX_PLAYERS slots, 3 alive matching --players 3
    REQUIRE(doc.contains("players"));
    CHECK(doc["players"].is_array());
    CHECK(doc["players"].size() == 32);
    int alive = 0;
    for (auto const &slot : doc["players"]) if (slot.value("alive", false)) ++alive;
    CHECK(alive == 3);

    // Dead-player list (empty in turn 3 of a fresh game)
    REQUIRE(doc.contains("dead_players"));
    CHECK(doc["dead_players"].is_array());

    // AI state composite (Phase F-18)
    REQUIRE(doc.contains("ai_state"));
    CHECK(doc["ai_state"].contains("diplomat_next_id"));
    CHECK(doc["ai_state"].contains("agreements"));
    CHECK(doc["ai_state"]["diplomats"].is_array());
    CHECK(doc["ai_state"]["diplomats"].size() == 3);
}

TEST_SUITE_END;
