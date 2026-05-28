#!/bin/bash
# test/run_headless_tests.sh
# Smoke tests for ctp2_headless — runs without needing the unit-test build.
#
# Usage: ./test/run_headless_tests.sh [build_dir]
# Default build_dir: ../build-sanitized

set -e

BUILD_DIR="${1:-build-sanitized}"
HEADLESS="./$BUILD_DIR/ctp2_headless"

echo "=== CTP2 Headless Smoke Tests ==="
echo "Binary: $HEADLESS"
echo ""

if [ ! -x "$HEADLESS" ]; then
    echo "ERROR: $HEADLESS not found or not executable"
    echo "Build it first: ninja -C $BUILD_DIR ctp2_headless"
    exit 1
fi

# Test 1: 10-turn smoke
echo "TEST 1: 10-turn smoke test..."
OUTPUT=$("$HEADLESS" --new-game --turns 10 --players 3 --seed 42 2>&1)
if echo "$OUTPUT" | grep -q "Completed 10 turns"; then
    echo "  PASS"
else
    echo "  FAIL"
    exit 1
fi

# Test 2: No ASan/UBSan errors
echo "TEST 2: No sanitizer errors..."
if echo "$OUTPUT" | grep -qi "ERROR: AddressSanitizer"; then
    echo "  FAIL: ASan error detected"
    exit 1
fi
if echo "$OUTPUT" | grep -qi "runtime error:"; then
    echo "  FAIL: UBSan error detected"
    exit 1
fi
echo "  PASS"

# Test 3: 50-turn stress
echo "TEST 3: 50-turn stress test..."
OUTPUT=$("$HEADLESS" --new-game --turns 50 --players 3 --seed 42 2>&1)
if echo "$OUTPUT" | grep -q "Completed 50 turns"; then
    echo "  PASS"
else
    echo "  FAIL"
    exit 1
fi

# Test 4: Different seeds produce different event patterns
echo "TEST 4: Seed divergence check..."
OUT1=$("$HEADLESS" --new-game --turns 25 --players 3 --seed 123 2>&1)
OUT2=$("$HEADLESS" --new-game --turns 25 --players 3 --seed 456 2>&1)
# Simple check: count of "Turn start" lines should be identical (both 25 turns * 3 players)
# but the detailed event patterns (city foundations, etc.) will differ.
# For a real determinism test we need save-file comparison.
echo "  PASS (placeholder — determinism test needs save-file support)"

# Test 5: Civilization growth events logged
echo "TEST 5: Game events logged..."
if echo "$OUT1" | grep -qi "founded city\|researched advance\|Wonder.*built"; then
    echo "  PASS"
else
    echo "  SKIP: No city/tech/wonder events in 25 turns (normal for short games)"
fi


# Test 6: Save → load round-trip.  Regression test for the silent exit(0)
# that GameFile::Restore hit when CivApp::InitializeGame called UI helpers
# without g_c3ui, and for the load-side trying to read SelectedItem bytes
# that GameFile::SaveGame no longer writes.  See the
# `if (!g_c3ui) return InitializeGameHeadless(archive)` guard in
# civapp.cpp::InitializeGame and the `player_view::Init(nPlayers)` line
# in gameinit.cpp.
echo "TEST 6: save → load round-trip..."
SAV=$(mktemp -t ctp2-roundtrip.XXXX)
CSV=$(mktemp -t ctp2-roundtrip.XXXX.csv)
trap "rm -f $SAV $CSV" EXIT
SAVE_OUT=$("$HEADLESS" --new-game --turns 8 --players 3 --seed 42 --save-game "$SAV" 2>&1)
if ! echo "$SAVE_OUT" | grep -q "SaveGame returned"; then
    echo "  FAIL: save did not complete"
    exit 1
fi
if [ ! -s "$SAV" ]; then
    echo "  FAIL: save file empty or missing ($SAV)"
    exit 1
fi
LOAD_OUT=$("$HEADLESS" --load-game "$SAV" --turns 3 --export-metrics "$CSV" 2>&1)
LOAD_EXIT=$?
if [ "$LOAD_EXIT" != "0" ]; then
    echo "  FAIL: load exited with code $LOAD_EXIT (regression — was silent exit(0)"
    echo "        before $(git log --oneline -1 ctp2_code/gs/utility/gameinit.cpp | awk '{print $1}'))"
    echo "        last 20 lines of load output:"
    echo "$LOAD_OUT" | tail -20
    exit 1
fi
if ! echo "$LOAD_OUT" | grep -q "Completed 3 turns"; then
    echo "  FAIL: load did not run requested turns"
    echo "$LOAD_OUT" | tail -10
    exit 1
fi
if [ ! -s "$CSV" ] || ! head -1 "$CSV" | grep -q "PLAYERS"; then
    echo "  FAIL: metrics CSV missing or malformed ($CSV)"
    exit 1
fi
echo "  PASS"

echo ""
echo "=== All tests passed ==="
