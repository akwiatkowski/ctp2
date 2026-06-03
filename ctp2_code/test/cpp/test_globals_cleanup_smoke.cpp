// test/cpp/test_globals_cleanup_smoke.cpp
//
// Headless stability smoke test for the globals-cleanup batch (refactor-1..7).
//
// Seven sibling workers are demoting low-fanout UI/debug globals from `extern`
// to file-scope `static`.  Each change is small and locally verified, but the
// cumulative effect on startup / turn-loop behaviour deserves an explicit test.
//
// This file complements test_headless_determinism.cpp (which proves same-seed
// runs are byte-identical) by exercising three specific codepaths that touch the
// static-ified globals:
//
//   1. Splash-path smoke       — g_splash_buf / g_splash_cur are written during
//                                gameinit's progress reporting.
//   2. Slic-debug + JSON-save  — g_segmentList / g_sourceList / g_watchList
//                                linkage must still resolve even though the debug
//                                windows are never opened in headless mode.
//   3. Determinism re-check    — catches subtle init-order issues that can arise
//                                when a variable changes from extern to static.
//
// Runs in the "integration" suite so the default `unit` target stays fast.
//   meson test -C build integration
// or:
//   ./build/ctp2_unit_tests --test-suite=integration --test-case='*globals_cleanup_smoke*'

#include "ctp/c3.h"
#include "doctest.h"

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

// Run ctp2_headless with the given CLI args.  Captures merged stdout+stderr
// into `log` (if non-null).  Returns true iff the process exits cleanly (0).
bool run_headless_raw(const char *args, std::string *log)
{
    const char *bin = find_headless();
    if (!bin) {
        if (log) *log = "[ERROR] ctp2_headless not found";
        return false;
    }

    char cmd[1024];
    std::snprintf(cmd, sizeof(cmd), "%s %s 2>&1", bin, args);

    FILE *pipe = ::popen(cmd, "r");
    if (!pipe) return false;

    char buf[512];
    std::string out;
    while (std::fgets(buf, sizeof(buf), pipe)) out += buf;

    int status = ::pclose(pipe);
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

TEST_CASE("globals_cleanup_smoke: splash-path smoke (--turns 1 --players 2)")
{
    // g_splash_buf and g_splash_cur are written during gameinit's progress
    // reporting.  A minimal 1-turn, 2-player run is enough to prove the splash
    // code still executes and the static-ified globals don't crash on write.
    std::string log;
    REQUIRE(run_headless_raw(
        "--new-game --turns 1 --players 2 --seed 7", &log));

    // Sanity: the engine should have run at least one turn and reported it.
    CHECK(log.find("Turn 1") != std::string::npos);
}

TEST_CASE("globals_cleanup_smoke: slic-debug linkage + JSON-save (--turns 3 --players 3)")
{
    // g_segmentList, g_sourceList, and g_watchList are slic-debug globals that
    // are only instantiated when the slic debug windows are opened.  In headless
    // mode those windows are never created, but the symbols must still link and
    // any destructor-order issues must not blow up on exit.  Running through the
    // JSON-save path also exercises persistent-state serialization near the slic
    // engine, increasing coverage of the affected TUs.
    const char *path = "/tmp/ctp2_gc_slic.json";
    std::remove(path);

    std::string log;
    REQUIRE(run_headless_raw(
        "--new-game --turns 3 --players 3 --seed 42 --json-save "
        "/tmp/ctp2_gc_slic.json", &log));

    // Verify the save was produced.
    std::string raw;
    REQUIRE(read_file(path, raw));
    CHECK(raw.size() > 1024);
}

TEST_CASE("globals_cleanup_smoke: determinism re-check (--turns 5 --players 4 --seed 42)")
{
    // When a global changes from extern to static, its initialisation order
    // across translation units can shift.  If any game subsystem implicitly
    // depends on that order (e.g. a constructor reading another static that
    // hasn't run yet), determinism can break even though the *same* seed is
    // used.  Running twice and byte-comparing the metrics CSV catches this.
    const char *path_a = "/tmp/ctp2_gc_det_a.csv";
    const char *path_b = "/tmp/ctp2_gc_det_b.csv";
    std::remove(path_a);
    std::remove(path_b);

    std::string log_a;
    std::string log_b;
    REQUIRE(run_headless_raw(
        "--new-game --turns 5 --players 4 --seed 42 --export-metrics "
        "/tmp/ctp2_gc_det_a.csv", &log_a));
    REQUIRE(run_headless_raw(
        "--new-game --turns 5 --players 4 --seed 42 --export-metrics "
        "/tmp/ctp2_gc_det_b.csv", &log_b));

    std::string csv_a;
    std::string csv_b;
    REQUIRE(read_file(path_a, csv_a));
    REQUIRE(read_file(path_b, csv_b));
    REQUIRE_FALSE(csv_a.empty());
    REQUIRE_FALSE(csv_b.empty());
    CHECK(csv_a == csv_b);
}

TEST_SUITE_END;
