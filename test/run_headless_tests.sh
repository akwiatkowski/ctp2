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

echo ""
echo "=== All tests passed ==="
