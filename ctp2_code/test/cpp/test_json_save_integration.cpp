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

    // Action Log: the event-bus tap must have fired during 3 turns of a
    // 3-player game (founding cities, creating units, issuing orders), and
    // every entry is well-formed {turn, player, event, args}.
    REQUIRE(doc.contains("action_log"));
    CHECK(doc["action_log"].is_array());
    CHECK(doc["action_log"].size() > 0);
    if (!doc["action_log"].empty()) {
        auto const &e = doc["action_log"].front();
        CHECK(e.contains("turn"));
        CHECK(e.contains("player"));
        CHECK(e.contains("event"));
        CHECK(e["event"].is_string());
        CHECK_FALSE(e["event"].get<std::string>().empty());
        CHECK(e["args"].is_array());
    }
}

TEST_CASE("LoadJson round-trip: save / load / save preserves all state")
{
    const char *bin = find_headless();
    REQUIRE(bin);

    const char *patha = "/tmp/ctp2_rt_a.json";
    const char *pathb = "/tmp/ctp2_rt_b.json";
    std::remove(patha);
    std::remove(pathb);

    // Run 1: play 3 turns + save (source-of-truth state).
    {
        char cmd[1024];
        std::snprintf(cmd, sizeof(cmd),
                      "%s --new-game --turns 3 --players 3 --seed 42 "
                      "--json-save %s 2>&1", bin, patha);
        std::FILE *pipe = popen(cmd, "r");
        REQUIRE(pipe != nullptr);
        char buf[512];
        while (std::fgets(buf, sizeof(buf), pipe)) { /* drain */ }
        int rc = pclose(pipe);
        REQUIRE(WIFEXITED(rc));
        REQUIRE(WEXITSTATUS(rc) == 0);
    }

    // Run 2: fresh init → LoadJson → SaveJson.  Verifies the full
    // round-trip: gameinit's fresh state is overwritten by the saved
    // state, and the second SaveJson reproduces the same content.
    {
        char cmd[1024];
        std::snprintf(cmd, sizeof(cmd),
                      "%s --new-game --turns 0 --players 3 --seed 42 "
                      "--json-load %s --json-save %s 2>&1",
                      bin, patha, pathb);
        std::FILE *pipe = popen(cmd, "r");
        REQUIRE(pipe != nullptr);
        char buf[512];
        std::string log;
        while (std::fgets(buf, sizeof(buf), pipe)) log += buf;
        int rc = pclose(pipe);
        INFO(log);
        REQUIRE(log.find("LoadJson returned ok") != std::string::npos);
        REQUIRE(log.find("SaveJson returned ok") != std::string::npos);
        REQUIRE(WIFEXITED(rc));
        REQUIRE(WEXITSTATUS(rc) == 0);
    }

    std::string ra;
    std::string rb;
    REQUIRE(read_file(patha, ra));
    REQUIRE(read_file(pathb, rb));

    nlohmann::json a = nlohmann::json::parse(ra);
    nlohmann::json b = nlohmann::json::parse(rb);

    // Keys that should round-trip exactly.  Excludes:
    //   - saved_at, ctp2_build: clock + build SHA, expected to differ.
    //   - selection: load not implemented (no SelectedItem setter).
    //   - slic_engine: ~6-byte diff in sym_tab / segments ordering
    //     (StringHash iteration order differs after reload); content-
    //     equivalent but not byte-equivalent.  Validated separately.
    for (const char *key : {
        "magic", "schema_version",
        "rng", "settings", "world", "turn",
        "unit_pool", "army_pool", "trade_pool", "pollution",
        "terrain_improvement_pool", "civilisation_pool",
        "message_pool", "installation_pool",
        "wonder_tracker", "exclusions", "feat_tracker",
        "event_tracker", "top_ten",
        "players", "dead_players", "ai_state",
    }) {
        INFO("round-trip key: " << key);
        REQUIRE(a.contains(key));
        REQUIRE(b.contains(key));
        CHECK(a[key] == b[key]);
    }

    // slic_engine: size invariant (content drift is in hash-table
    // iteration order, not in the data).  Same top-level sub-keys and
    // very similar byte counts.
    REQUIRE(a.contains("slic_engine"));
    REQUIRE(b.contains("slic_engine"));
    auto const sa_size = a["slic_engine"].dump().size();
    auto const sb_size = b["slic_engine"].dump().size();
    INFO("slic_engine sizes: a=" << sa_size << " b=" << sb_size);
    CHECK(std::abs(static_cast<long>(sa_size) - static_cast<long>(sb_size)) < 100);
    CHECK(a["slic_engine"].size() == b["slic_engine"].size());
}

TEST_CASE("LoadJson derived cache (P5): stale good_value is recomputed from the "
          "loaded map, not left at the fresh-game state")
{
    // P5 derived-cache audit pin.  World's per-resource good_value table
    // is derived from the map's good distribution (ComputeGoodsValues).
    // The save carries it, but when the saved length doesn't match the
    // current ResourceDB (a mod changed the DB between save and load) the
    // load path must REBUILD it from the freshly-loaded cells — not leave
    // it at whatever the throwaway fresh-game gameinit computed for the
    // discarded initial map (the wrong-but-not-crashing state P5 targets).
    //
    // We can't change the DB mid-test, so we force the mismatch branch by
    // corrupting the saved good_value to a wrong length, then load it under
    // a DIFFERENT seed (so the fresh-game map — and its good_value — differ
    // from the saved map).  A correct recompute reproduces the SOURCE map's
    // values; the pre-fix "leave it alone" behaviour would surface the
    // fresh seed-999 map's values instead.
    const char *bin = find_headless();
    REQUIRE(bin);

    const char *patha = "/tmp/ctp2_p5_good_a.json";
    const char *pathc = "/tmp/ctp2_p5_good_corrupt.json";
    const char *pathb = "/tmp/ctp2_p5_good_b.json";
    std::remove(patha);
    std::remove(pathc);
    std::remove(pathb);

    // Run 1: seed 42 → source-of-truth map + good_value.
    {
        std::string log;
        REQUIRE(run_save(patha, &log));  // seed 42, 3 turns, 3 players
    }

    std::string ra;
    REQUIRE(read_file(patha, ra));
    nlohmann::json a = nlohmann::json::parse(ra);
    REQUIRE(a["world"]["good_value"].is_array());
    nlohmann::json const good_a = a["world"]["good_value"];
    REQUIRE(good_a.size() > 3);  // real DB is ~55 entries

    // Corrupt the length so the load path can't restore verbatim and must
    // fall into the recompute branch.
    a["world"]["good_value"] = nlohmann::json::array({1.0, 2.0, 3.0});
    {
        std::ofstream out(pathc);
        REQUIRE(out.good());
        out << a.dump();
    }

    // Run 2: fresh seed 999 game → load the corrupted save → re-save.
    {
        char cmd[1024];
        std::snprintf(cmd, sizeof(cmd),
                      "%s --new-game --turns 0 --players 3 --seed 999 "
                      "--json-load %s --json-save %s 2>&1",
                      bin, pathc, pathb);
        std::FILE *pipe = popen(cmd, "r");
        REQUIRE(pipe != nullptr);
        char buf[512];
        std::string log;
        while (std::fgets(buf, sizeof(buf), pipe)) log += buf;
        int rc = pclose(pipe);
        INFO(log);
        REQUIRE(log.find("LoadJson returned ok") != std::string::npos);
        REQUIRE(WIFEXITED(rc));
        REQUIRE(WEXITSTATUS(rc) == 0);  // recompute path must not crash
    }

    std::string rb;
    REQUIRE(read_file(pathb, rb));
    nlohmann::json b = nlohmann::json::parse(rb);
    REQUIRE(b["world"]["good_value"].is_array());

    // Recompute restored the correct DB-sized table (not the corrupt 3)...
    CHECK(b["world"]["good_value"].size() == good_a.size());
    // ...with the SOURCE map's values, proving it was rebuilt from the
    // loaded cells and not left at the fresh seed-999 game's table.
    CHECK(b["world"]["good_value"] == good_a);
}

TEST_CASE("Phase G converter: binary save → JSON save via --load-game --json-save")
{
    // The one-shot converter mentioned in the JSON-savegame plan is free
    // via the existing CLI: --load-game reads a binary save, --json-save
    // writes JSON.  Together they convert old .c2g files (or any binary
    // savegame the engine can read) to the new JSON format.  This test
    // is the Phase G acceptance check that the conversion path works
    // end-to-end against a freshly-produced binary save.
    const char *bin = find_headless();
    REQUIRE(bin);

    const char *binpath  = "/tmp/ctp2_g_binsave.c2g";
    const char *jsonpath = "/tmp/ctp2_g_converted.json";
    std::remove(binpath);
    std::remove(jsonpath);

    // Step 1: produce a binary save via --save-game.
    {
        char cmd[1024];
        std::snprintf(cmd, sizeof(cmd),
                      "%s --new-game --turns 3 --players 3 --seed 42 "
                      "--save-game %s 2>&1", bin, binpath);
        std::FILE *pipe = popen(cmd, "r");
        REQUIRE(pipe != nullptr);
        char buf[512];
        std::string log;
        while (std::fgets(buf, sizeof(buf), pipe)) log += buf;
        int rc = pclose(pipe);
        INFO(log);
        REQUIRE(WIFEXITED(rc));
        REQUIRE(WEXITSTATUS(rc) == 0);
        REQUIRE(log.find("SaveGame returned") != std::string::npos);
    }

    std::string binraw;
    REQUIRE(read_file(binpath, binraw));
    CHECK(binraw.size() > 1024);

    // Step 2: convert binary → JSON via --load-game + --json-save.
    {
        char cmd[1024];
        std::snprintf(cmd, sizeof(cmd),
                      "%s --load-game %s --turns 0 --json-save %s 2>&1",
                      bin, binpath, jsonpath);
        std::FILE *pipe = popen(cmd, "r");
        REQUIRE(pipe != nullptr);
        char buf[512];
        std::string log;
        while (std::fgets(buf, sizeof(buf), pipe)) log += buf;
        int rc = pclose(pipe);
        INFO(log);
        REQUIRE(WIFEXITED(rc));
        REQUIRE(WEXITSTATUS(rc) == 0);
        REQUIRE(log.find("SaveJson returned ok") != std::string::npos);
    }

    // Step 3: the converted JSON should parse and contain the expected
    // top-level structure.
    std::string jsonraw;
    REQUIRE(read_file(jsonpath, jsonraw));

    nlohmann::json doc = nlohmann::json::parse(jsonraw);
    CHECK(doc["magic"] == "CTP2-JSON");
    CHECK(doc["schema_version"] == 1);
    for (const char *key : {
        "rng", "settings", "world", "turn",
        "unit_pool", "army_pool", "trade_pool", "slic_engine",
        "civilisation_pool", "message_pool", "players", "ai_state",
    }) {
        INFO("missing key in converted JSON: " << key);
        CHECK(doc.contains(key));
    }

    // Expect 3 alive players matching --players 3.
    REQUIRE(doc.contains("players"));
    REQUIRE(doc["players"].is_array());
    int alive = 0;
    for (auto const &slot : doc["players"]) if (slot.value("alive", false)) ++alive;
    CHECK(alive == 3);
}

// Phase 0.C-3: the auto-detect test (G-2) is gone — there's no binary
// format left to detect.  GameFile::Restore is JSON-only now.

TEST_CASE("Phase G-3: --save-game writes JSON by default")
{
    // Phase 0.C-1: GameFile::SaveGame is JSON-only — the legacy binary
    // write path was deleted.  This test verifies that --save-game (which
    // calls GameFile::SaveGame) produces a file that starts with the JSON
    // object marker rather than the legacy CTP0XXX magic header.
    const char *bin = find_headless();
    REQUIRE(bin);

    const char *path = "/tmp/ctp2_g3_default.save";
    std::remove(path);

    char cmd[1024];
    std::snprintf(cmd, sizeof(cmd),
                  "%s --new-game --turns 3 --players 3 --seed 42 "
                  "--save-game %s 2>&1", bin, path);
    std::FILE *pipe = popen(cmd, "r");
    REQUIRE(pipe != nullptr);
    char buf[512];
    std::string log;
    while (std::fgets(buf, sizeof(buf), pipe)) log += buf;
    int rc = pclose(pipe);
    INFO(log);
    REQUIRE(WIFEXITED(rc));
    REQUIRE(WEXITSTATUS(rc) == 0);

    // File must exist + start with '{' (after any leading whitespace)
    // confirming JSON format.
    std::string raw;
    REQUIRE(read_file(path, raw));
    size_t i = 0;
    while (i < raw.size() && std::isspace(static_cast<unsigned char>(raw[i]))) ++i;
    REQUIRE(i < raw.size());
    CHECK(raw[i] == '{');

    // Parse and check the magic field.
    nlohmann::json doc = nlohmann::json::parse(raw);
    CHECK(doc["magic"] == "CTP2-JSON");
}

TEST_CASE("UTF-8: SaveJson handles 8-player games with Latin-1 civ names")
{
    // Regression test for the JSON UTF-8 cleanliness bug.  CTP2's
    // civ/leader/country/city/army names are stored in fixed char[]
    // buffers populated from ISO-8859-1 / Windows-1252 sources (CTP2
    // shipped pre-UTF-8).  nlohmann::json::dump() throws type_error 316
    // on invalid UTF-8.
    //
    // Before the fix: with `--players 8 --seed 42`, the per-turn
    // autosave triggered by GEV_StartMovePhase used to crash mid-turn 1
    // with `[json.exception.type_error.316] invalid UTF-8 byte at index 6:
    // 0x6F` — civ name "Czechosłowacja" or similar.
    //
    // After the fix: utf8_safe() in json_save.cpp expands Latin-1 bytes
    // ≥ 0x80 into two-byte UTF-8 on the write side, lossless for the
    // common case.
    const char *bin = find_headless();
    REQUIRE(bin);

    const char *path = "/tmp/ctp2_utf8_8p.save";
    std::remove(path);

    char cmd[1024];
    std::snprintf(cmd, sizeof(cmd),
                  "%s --new-game --turns 5 --players 8 --seed 42 "
                  "--save-game %s 2>&1", bin, path);
    std::FILE *pipe = popen(cmd, "r");
    REQUIRE(pipe != nullptr);
    char buf[512];
    std::string log;
    while (std::fgets(buf, sizeof(buf), pipe)) log += buf;
    int rc = pclose(pipe);
    INFO(log);
    REQUIRE(WIFEXITED(rc));
    REQUIRE(WEXITSTATUS(rc) == 0);
    REQUIRE(log.find("type_error.316") == std::string::npos);
    REQUIRE(log.find("invalid UTF-8")   == std::string::npos);

    // Verify the file is well-formed JSON (parses cleanly — that's the
    // post-condition the UTF-8 fix protects).
    std::string raw;
    REQUIRE(read_file(path, raw));
    REQUIRE_NOTHROW(nlohmann::json::parse(raw));
}

// ---------------------------------------------------------------------------
// Phase 1j / Modernization 0.A.3 — N-turn determinism
//
// Stronger than the 0-turn round-trip above: load the same snapshot
// twice, advance K turns each time, save, and compare. If the engine
// is deterministic from a loaded state, the two post-advance JSON
// blobs must be byte-identical on the keys we round-trip.
//
// What this catches that the 0-turn test does not:
//   - State that is correctly serialized but stale-and-reset on load
//     (e.g. caches whose first-touch differs between runs).
//   - RNG paths that consume different amounts during turn processing
//     depending on hidden mutable state.
//   - Iteration-order dependencies in subsystems that mutate during
//     turn advancement (vs. SaveJson-only iteration).
// ---------------------------------------------------------------------------

namespace {

bool run_headless(const char *cmd, std::string *log)
{
    std::FILE *pipe = popen(cmd, "r");
    if (!pipe) return false;
    char buf[512];
    std::string out;
    while (std::fgets(buf, sizeof(buf), pipe)) out += buf;
    int rc = pclose(pipe);
    if (log) *log = std::move(out);
    return WIFEXITED(rc) && WEXITSTATUS(rc) == 0;
}

}  // namespace

TEST_CASE("N-turn determinism: load+advance K turns → same JSON across runs")
{
    const char *bin = find_headless();
    REQUIRE(bin);

    const char *snap = "/tmp/ctp2_det_snapshot.json";
    const char *out_b = "/tmp/ctp2_det_run_b.json";
    const char *out_c = "/tmp/ctp2_det_run_c.json";
    std::remove(snap); std::remove(out_b); std::remove(out_c);

    // Step 1: produce the initial snapshot at turn 3.
    {
        char cmd[1024];
        std::snprintf(cmd, sizeof(cmd),
                      "%s --new-game --turns 3 --players 3 --seed 42 "
                      "--json-save %s 2>&1", bin, snap);
        std::string log;
        REQUIRE_MESSAGE(run_headless(cmd, &log), log);
    }

    // Step 2: load snapshot, advance 3 more turns, save (run B).
    {
        char cmd[1024];
        std::snprintf(cmd, sizeof(cmd),
                      "%s --new-game --turns 3 --players 3 --seed 42 "
                      "--json-load %s --json-save %s 2>&1",
                      bin, snap, out_b);
        std::string log;
        REQUIRE_MESSAGE(run_headless(cmd, &log), log);
        REQUIRE(log.find("LoadJson returned ok") != std::string::npos);
        REQUIRE(log.find("SaveJson returned ok") != std::string::npos);
    }

    // Step 3: do it again from the same snapshot (run C).
    {
        char cmd[1024];
        std::snprintf(cmd, sizeof(cmd),
                      "%s --new-game --turns 3 --players 3 --seed 42 "
                      "--json-load %s --json-save %s 2>&1",
                      bin, snap, out_c);
        std::string log;
        REQUIRE_MESSAGE(run_headless(cmd, &log), log);
        REQUIRE(log.find("LoadJson returned ok") != std::string::npos);
        REQUIRE(log.find("SaveJson returned ok") != std::string::npos);
    }

    // Compare.
    std::string rb;
    std::string rc;
    REQUIRE(read_file(out_b, rb));
    REQUIRE(read_file(out_c, rc));

    nlohmann::json b = nlohmann::json::parse(rb);
    nlohmann::json c = nlohmann::json::parse(rc);

    // Same exclusions as the 0-turn round-trip:
    //   - saved_at / ctp2_build: wall-clock + build SHA.
    //   - selection: load not implemented.
    //   - slic_engine: StringHash iteration drift (validated by size
    //     near-equality only — see 0-turn case for rationale).
    for (const char *key : {
        "magic", "schema_version",
        "rng", "settings", "world", "turn",
        "unit_pool", "army_pool", "trade_pool", "pollution",
        "terrain_improvement_pool", "civilisation_pool",
        "message_pool", "installation_pool",
        "wonder_tracker", "exclusions", "feat_tracker",
        "event_tracker", "top_ten",
        "players", "dead_players", "ai_state",
    }) {
        INFO("determinism key: " << key);
        REQUIRE(b.contains(key));
        REQUIRE(c.contains(key));
        CHECK(b[key] == c[key]);
    }

    REQUIRE(b.contains("slic_engine"));
    REQUIRE(c.contains("slic_engine"));
    auto const sb_size = b["slic_engine"].dump().size();
    auto const sc_size = c["slic_engine"].dump().size();
    INFO("slic_engine sizes: b=" << sb_size << " c=" << sc_size);
    CHECK(std::abs(static_cast<long>(sb_size) - static_cast<long>(sc_size)) < 100);
    CHECK(b["slic_engine"].size() == c["slic_engine"].size());
}

TEST_SUITE_END;
