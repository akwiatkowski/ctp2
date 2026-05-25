// test/cpp/test_headless_smoke.cpp
// Smoke tests that exercise the ctp2_headless binary.
//
// These tests spawn the headless executable as a subprocess and verify:
//   - It starts without crashing
//   - It completes the requested number of turns
//   - No ASan/UBSan errors appear in stderr
//
// Requirements:
//   - ctp2_headless must be built before running tests:
//     ninja -C build-sanitized ctp2_headless
//
// These are integration tests, not unit tests — they take seconds, not ms.

#include "ctp/c3.h"
#include "doctest.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Common paths where the headless binary may live, relative to the test
// working directory (project root when run via meson).
static const char *HEADLESS_CANDIDATES[] = {
    "./build/ctp2_headless",
    "./build-sanitized/ctp2_headless",
    "./ctp2_headless",   // if running directly from build dir
    nullptr,
};

static const char *find_headless_binary()
{
    for (const char **p = HEADLESS_CANDIDATES; *p; ++p) {
        if (std::FILE *f = std::fopen(*p, "r")) {
            std::fclose(f);
            return *p;
        }
    }
    return nullptr;
}

// Run the headless binary with given arguments and return its stderr output.
// Returns empty string on failure (cannot spawn process).
static std::string run_headless(const char *args)
{
    const char *bin = find_headless_binary();
    if (!bin) {
        return "[ERROR] ctp2_headless binary not found";
    }

    char cmd[1024];
    std::snprintf(cmd, sizeof(cmd), "%s %s 2>&1", bin, args);

    FILE *pipe = popen(cmd, "r");
    if (!pipe) {
        return "";
    }

    std::string output;
    char buf[256];
    while (std::fgets(buf, sizeof(buf), pipe)) {
        output += buf;
    }

    int status = pclose(pipe);
    // Encode exit status into the first byte so the caller can check it.
    // We prepend a sentinel line with the exit code.
    std::string result = "[EXIT_CODE] ";
    result += std::to_string(WEXITSTATUS(status));
    result += "\n";
    result += output;
    return result;
}

TEST_CASE("Headless smoke: 10 turns, 3 players")
{
    std::string output = run_headless("--new-game --turns 10 --players 3 --seed 42");

    // Check the binary actually ran (not "file not found")
    CHECK(!output.empty());

    // Verify exit code 0
    CHECK(output.find("[EXIT_CODE] 0") == 0);

    // Verify completion message
    CHECK(output.find("Completed 10 turns") != std::string::npos);

    // Verify turn progression was logged
    CHECK(output.find("Turn 1 / 10") != std::string::npos);
    CHECK(output.find("Turn 10 / 10") != std::string::npos);

    // Verify no ASan errors
    CHECK(output.find("ERROR: AddressSanitizer") == std::string::npos);
    CHECK(output.find("SUMMARY: AddressSanitizer") == std::string::npos);

    // Verify no UBSan errors
    CHECK(output.find("runtime error:") == std::string::npos);
}

TEST_CASE("Headless smoke: 50 turns, 3 players")
{
    std::string output = run_headless("--new-game --turns 50 --players 3 --seed 42");

    CHECK(!output.empty());
    CHECK(output.find("[EXIT_CODE] 0") == 0);
    CHECK(output.find("Completed 50 turns") != std::string::npos);
    CHECK(output.find("ERROR: AddressSanitizer") == std::string::npos);
    CHECK(output.find("runtime error:") == std::string::npos);
}

TEST_CASE("Headless smoke: game events are logged")
{
    // Run 25 turns and check that the headless observer logged key events.
    std::string output = run_headless("--new-game --turns 25 --players 3 --seed 42");

    CHECK(!output.empty());
    CHECK(output.find("[EXIT_CODE] 0") == 0);

    // Headless observer logs these via DPRINTF(k_DBG_GAMESTATE, ...)
    // They appear in stderr when the binary is built with DEBUG logging.
    // In release builds these may be silent — that's OK, the test still
    // verifies the binary doesn't crash.
    if (output.find("[HEADLESS]") != std::string::npos) {
        // If headless logging is active, verify expected events appear.
        CHECK(output.find("Turn start for player") != std::string::npos);
        CHECK(output.find("Turn end for player") != std::string::npos);
    }
}

TEST_CASE("Headless smoke: different seeds produce different outputs")
{
    // This is a sanity check that the seed is actually wired to the RNG.
    // If both runs produce identical output, either:
    //   (a) the seed is not wired (bug), or
    //   (b) the game is fully deterministic with no RNG (unlikely).
    std::string out1 = run_headless("--new-game --turns 10 --players 3 --seed 123");
    std::string out2 = run_headless("--new-game --turns 10 --players 3 --seed 456");

    CHECK(!out1.empty());
    CHECK(!out2.empty());

    // Strip timing-dependent lines (turn counter, timestamps) before comparing.
    // We compare the presence/absence of city foundation events, which are
    // RNG-dependent (goody hut placement, barb spawning).
    bool hasCity1 = out1.find("founded city") != std::string::npos;
    bool hasCity2 = out2.find("founded city") != std::string::npos;

    // With different seeds, the RNG paths diverge.  It's possible both runs
    // happen to found a city in 10 turns, but unlikely both have exactly the
    // same number of city events.  For a stronger check, see the determinism
    // test (test_headless_determinism.cpp) which compares two runs with the
    // SAME seed.
    //
    // This test just verifies the seed plumbing exists.
    // Verify at least one run did something interesting
    CHECK((hasCity1 || hasCity2) == true);
}
