#!/usr/bin/env bash
# See .ci/README.md for coverage. Failures update state and return nonzero.

set -uo pipefail

CTP2_ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
CI_ROOT="$CTP2_ROOT/.ci"
LOG_DIR="$CI_ROOT/log"
TS="$(date -u +%Y%m%d-%H%M%S)"
LOG_FILE="$LOG_DIR/tier-c-$TS.log"
XML_FILE="$LOG_DIR/tier-c-$TS.xml"
RESULT_JSON="$LOG_DIR/tier-c-$TS.json"

mkdir -p "$LOG_DIR"

cd "$CTP2_ROOT"

HEAD_SHA="$(git rev-parse --short HEAD 2>/dev/null || echo working)"
HEAD_SUBJECT="$(git log -1 --pretty=%s 2>/dev/null || echo '')"
HEAD_BRANCH="$(git rev-parse --abbrev-ref HEAD 2>/dev/null || echo '')"

mise exec -- python3 "$CI_ROOT/update_state.py" \
    --status running --running-tier C \
    --head-sha "$HEAD_SHA" --head-subject "$HEAD_SUBJECT" --head-branch "$HEAD_BRANCH" \
    >/dev/null

# Native ASan currently hangs before main on macOS 26, even for a trivial
# executable. Keep UBSan active there; Linux runs address + undefined.
if [[ "$(uname -s)" == Darwin ]]; then
    SANITIZER=undefined
    SANITIZER_BUILD=build-ubsan
else
    SANITIZER=address,undefined
    SANITIZER_BUILD=build-sanitized
fi
export UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1"
export ASAN_OPTIONS="halt_on_error=1:abort_on_error=1:detect_leaks=0"

START=$(date +%s)
(
    echo "=== tier-c $TS (HEAD $HEAD_SHA; $SANITIZER) ==="
    if [[ -f "$SANITIZER_BUILD/build.ninja" ]]; then
        mise exec -- meson configure "$SANITIZER_BUILD" \
            -Danet=false -Dbuildtype=debugoptimized -Db_sanitize="$SANITIZER" || exit $?
    else
        mise exec -- meson setup "$SANITIZER_BUILD" ctp2_code \
            -Danet=false -Dbuildtype=debugoptimized \
            -Db_sanitize="$SANITIZER" -Db_lundef=false || exit $?
    fi
    mise exec -- ninja -C "$SANITIZER_BUILD" ctp2_unit_tests ctp2_headless || exit $?
    mise exec -- "./$SANITIZER_BUILD/ctp2_unit_tests" -r=xml --no-version > "$XML_FILE"
    UNIT_RC=$?
    mise exec -- meson test -C "$SANITIZER_BUILD" --no-rebuild \
        --suite scenario --no-suite marathon --print-errorlogs || exit $?
    mise exec -- python3 ctp2_code/test/scenario_long_game.py \
        "$SANITIZER_BUILD/ctp2_headless" --rounds 500 || exit $?
    exit "$UNIT_RC"
) >>"$LOG_FILE" 2>&1
RUN_RC=$?

END=$(date +%s)
DURATION=$((END - START))

if mise exec -- python3 "$CI_ROOT/parse_doctest_xml.py" "$XML_FILE" \
    --exit-code "$RUN_RC" > "$RESULT_JSON" 2>>"$LOG_FILE"; then
    STATUS=green
else
    STATUS=red
fi

mise exec -- python3 "$CI_ROOT/update_state.py" \
    --tier c --status "$STATUS" --duration-s "$DURATION" \
    --result "$RESULT_JSON" \
    --head-sha "$HEAD_SHA" --head-subject "$HEAD_SUBJECT" --head-branch "$HEAD_BRANCH" \
    --context-path "$LOG_FILE" \
    --clear-running \
    >/dev/null

# Metrics: track tier-c duration over time.  ASan builds are typically
# 2-3x slower than tier-b; this view tells us if sanitizer overhead is
# drifting.
METRICS_CSV="$CI_ROOT/metrics/tier-c-times.csv"
mkdir -p "$(dirname "$METRICS_CSV")"
if [[ ! -s "$METRICS_CSV" ]]; then
    echo "timestamp,head_sha,status,duration_s" > "$METRICS_CSV"
fi
echo "$(date -u +%Y-%m-%dT%H:%M:%SZ),$HEAD_SHA,$STATUS,$DURATION" >> "$METRICS_CSV"

[[ "$STATUS" == green ]]
