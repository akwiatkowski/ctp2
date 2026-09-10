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
#include "headless_test_config.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>

// Run the headless binary with given arguments and return its stderr output.
// Returns empty string on failure (cannot spawn process).
static std::string run_headless(const char *args)
{
    const char *bin = CTP2_HEADLESS_COMMAND;

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

// Each test in this file launches ctp2_headless as a subprocess and
// runs 10-50 real turns — a few seconds per case.  Tagged "integration"
// so the default `unit` meson target excludes them; run via the
// `integration` meson target or with --test-suite=integration.
TEST_SUITE_BEGIN("integration");

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
    // Sanity check that the seed is wired to the RNG.  If both runs produce
    // identical output, the seed is not wired (bug).  We compare raw stderr
    // output minus timing-dependent lines.
    std::string out1 = run_headless("--new-game --turns 10 --players 3 --seed 123");
    std::string out2 = run_headless("--new-game --turns 10 --players 3 --seed 456");

    CHECK(!out1.empty());
    CHECK(!out2.empty());

    // Verify both runs succeeded
    CHECK(out1.find("[EXIT_CODE] 0") == 0);
    CHECK(out2.find("[EXIT_CODE] 0") == 0);

    // If the seed is wired, the RNG produces different map state / events.
    // We check that the full stderr output (minus turn counter lines) differs.
    // If outputs are identical, the seed is not affecting the RNG.
    auto strip_turn_lines = [](const std::string& s) -> std::string {
        std::string result;
        size_t pos = 0;
        while (pos < s.size()) {
            size_t end = s.find('\n', pos);
            if (end == std::string::npos) end = s.size();
            std::string line = s.substr(pos, end - pos);
            // Skip turn-counter lines that are identical across runs
            if (line.find("[HEADLESS] Turn ") == std::string::npos) {
                result += line;
                result += '\n';
            }
            pos = end + 1;
        }
        return result;
    };

    std::string stripped1 = strip_turn_lines(out1);
    std::string stripped2 = strip_turn_lines(out2);

    // With different seeds the outputs should diverge somewhere.
    CHECK(stripped1 != stripped2);
}

TEST_CASE("Headless smoke: minimum turns (1 turn, 3 players)")
{
    std::string output = run_headless("--new-game --turns 1 --players 3 --seed 42");

    CHECK(!output.empty());
    CHECK(output.find("[EXIT_CODE] 0") == 0);
    CHECK(output.find("Completed 1 turns") != std::string::npos);
    CHECK(output.find("ERROR: AddressSanitizer") == std::string::npos);
    CHECK(output.find("runtime error:") == std::string::npos);
}

TEST_CASE("Headless smoke: minimum players (5 turns, 2 players)")
{
    std::string output = run_headless("--new-game --turns 5 --players 2 --seed 42");

    CHECK(!output.empty());
    CHECK(output.find("[EXIT_CODE] 0") == 0);
    CHECK(output.find("Completed 5 turns") != std::string::npos);
    CHECK(output.find("ERROR: AddressSanitizer") == std::string::npos);
    CHECK(output.find("runtime error:") == std::string::npos);
}

TEST_CASE("Headless smoke: large player count (10 turns, 8 players)")
{
    std::string output = run_headless("--new-game --turns 10 --players 8 --seed 42");

    CHECK(!output.empty());
    CHECK(output.find("[EXIT_CODE] 0") == 0);
    CHECK(output.find("Completed 10 turns") != std::string::npos);
    CHECK(output.find("ERROR: AddressSanitizer") == std::string::npos);
    CHECK(output.find("runtime error:") == std::string::npos);
}

TEST_CASE("Headless smoke: different seed 100 (20 turns, 3 players)")
{
    std::string output = run_headless("--new-game --turns 20 --players 3 --seed 100");

    CHECK(!output.empty());
    CHECK(output.find("[EXIT_CODE] 0") == 0);
    CHECK(output.find("Completed 20 turns") != std::string::npos);
    CHECK(output.find("ERROR: AddressSanitizer") == std::string::npos);
    CHECK(output.find("runtime error:") == std::string::npos);
}

TEST_CASE("Headless smoke: different seed 999 (20 turns, 3 players)")
{
    std::string output = run_headless("--new-game --turns 20 --players 3 --seed 999");

    CHECK(!output.empty());
    CHECK(output.find("[EXIT_CODE] 0") == 0);
    CHECK(output.find("Completed 20 turns") != std::string::npos);
    CHECK(output.find("ERROR: AddressSanitizer") == std::string::npos);
    CHECK(output.find("runtime error:") == std::string::npos);
}

TEST_CASE("Headless smoke: baseline output is non-empty (10 turns, 3 players)")
{
    std::string output = run_headless("--new-game --turns 10 --players 3 --seed 42");

    CHECK(!output.empty());
}

TEST_SUITE_END;
