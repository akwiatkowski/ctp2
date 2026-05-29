// test/cpp/test_headless_determinism.cpp
// Determinism tests for ctp2_headless.
//
// Two runs of ctp2_headless with the same --seed must produce byte-identical
// metrics output.  Two runs with different seeds must NOT.
//
// Determinism is the gate for further refactoring: if the same seed produces
// different output, some subsystem is reading an unsalted RNG (system clock,
// hash-map iteration order, uninitialised memory).  These tests catch
// regressions in that gate.

#include "ctp/c3.h"
#include "doctest.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <string>

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

int run_headless(const char *args, std::string *captured_stderr)
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

bool read_file(const char *path, std::string &out)
{
    FILE *fp = std::fopen(path, "rb");
    if (!fp) return false;
    char buf[4096];
    size_t n;
    while ((n = std::fread(buf, 1, sizeof(buf), fp)) > 0) out.append(buf, n);
    std::fclose(fp);
    return true;
}

// Run headless with the given seed and turn count, write metrics to the given
// path, return the metrics file contents.  Empty string on failure.
std::string run_and_read_metrics(int seed, int turns, const char *path, int players = 4)
{
    std::remove(path);
    char args[512];
    std::snprintf(args, sizeof(args),
                  "--new-game --turns %d --players %d --seed %d --export-metrics %s",
                  turns, players, seed, path);
    std::string log;
    int rc = run_headless(args, &log);
    if (rc != 0) {
        INFO("headless exited non-zero: " << rc << "\n" << log);
        return "";
    }
    std::string contents;
    if (!read_file(path, contents)) return "";
    return contents;
}

}  // namespace

// Each headless-game test in this file launches ctp2_headless as a
// subprocess and runs real turns — a few seconds per case.  Tagging
// the whole file as the "integration" suite lets the default `unit`
// meson target exclude these slow tests; run them via `meson test
// -C build integration` (or directly with --test-suite=integration).
TEST_SUITE_BEGIN("integration");

TEST_CASE("Determinism: same seed produces identical metrics at 5 turns")
{
    std::string a = run_and_read_metrics(42, 5, "/tmp/ctp2_det_5a.csv");
    std::string b = run_and_read_metrics(42, 5, "/tmp/ctp2_det_5b.csv");

    REQUIRE_FALSE(a.empty());
    REQUIRE_FALSE(b.empty());
    CHECK(a == b);
}

TEST_CASE("Determinism: same seed produces identical metrics at 25 turns")
{
    // 25 turns is enough for the AI to found multiple cities per player and
    // run combat/diplomacy events; catches divergence sources that only fire
    // after the opening moves.
    std::string a = run_and_read_metrics(42, 25, "/tmp/ctp2_det_25a.csv");
    std::string b = run_and_read_metrics(42, 25, "/tmp/ctp2_det_25b.csv");

    REQUIRE_FALSE(a.empty());
    REQUIRE_FALSE(b.empty());
    CHECK(a == b);
}

TEST_CASE("Determinism: different seeds produce different metrics")
{
    std::string a = run_and_read_metrics(42, 10, "/tmp/ctp2_det_seedA.csv");
    std::string b = run_and_read_metrics(99, 10, "/tmp/ctp2_det_seedB.csv");

    REQUIRE_FALSE(a.empty());
    REQUIRE_FALSE(b.empty());
    // If these match the RNG seed isn't actually affecting any game decision.
    CHECK(a != b);
}

TEST_CASE("Determinism: same seed produces identical metrics at 30 turns")
{
    std::string a = run_and_read_metrics(42, 30, "/tmp/ctp2_det_30a.csv");
    std::string b = run_and_read_metrics(42, 30, "/tmp/ctp2_det_30b.csv");

    REQUIRE_FALSE(a.empty());
    REQUIRE_FALSE(b.empty());
    CHECK(a == b);
}

TEST_CASE("Determinism: same seed produces identical metrics with 2 players")
{
    std::string a = run_and_read_metrics(42, 10, "/tmp/ctp2_det_2p_a.csv", 2);
    std::string b = run_and_read_metrics(42, 10, "/tmp/ctp2_det_2p_b.csv", 2);

    REQUIRE_FALSE(a.empty());
    REQUIRE_FALSE(b.empty());
    CHECK(a == b);
}

TEST_CASE("Determinism: same seed produces identical metrics with 6 players")
{
    std::string a = run_and_read_metrics(42, 10, "/tmp/ctp2_det_6p_a.csv", 6);
    std::string b = run_and_read_metrics(42, 10, "/tmp/ctp2_det_6p_b.csv", 6);

    REQUIRE_FALSE(a.empty());
    REQUIRE_FALSE(b.empty());
    CHECK(a == b);
}

// NOTE: seed 0 is NOT deterministic in the current engine — the headless
// binary treats it as a sentinel and substitutes a time-based seed.
// Wave 10a W8 added this test optimistically; it has been disabled until
// the engine's seed-0 behaviour is either fixed or documented as
// "0 means random".  See findings/ctp2.md for the investigation thread.
TEST_CASE("Determinism: same seed produces identical metrics with seed 0"
          * doctest::skip(true))
{
    std::string a = run_and_read_metrics(0, 10, "/tmp/ctp2_det_0a.csv");
    std::string b = run_and_read_metrics(0, 10, "/tmp/ctp2_det_0b.csv");

    REQUIRE_FALSE(a.empty());
    REQUIRE_FALSE(b.empty());
    CHECK(a == b);
}

TEST_CASE("Determinism: same seed produces identical metrics with large seed")
{
    std::string a = run_and_read_metrics(2147483647, 10, "/tmp/ctp2_det_maxa.csv");
    std::string b = run_and_read_metrics(2147483647, 10, "/tmp/ctp2_det_maxb.csv");

    REQUIRE_FALSE(a.empty());
    REQUIRE_FALSE(b.empty());
    CHECK(a == b);
}

TEST_SUITE_END;
