#!/usr/bin/env bash
# See .ci/README.md for coverage. Failures update state and return nonzero.

set -uo pipefail

CTP2_ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
CI_ROOT="$CTP2_ROOT/.ci"
LOG_DIR="$CI_ROOT/log"
TS="$(date -u +%Y%m%d-%H%M%S)"
LOG_FILE="$LOG_DIR/tier-b-$TS.log"
XML_FILE="$LOG_DIR/tier-b-$TS.xml"
RESULT_JSON="$LOG_DIR/tier-b-$TS.json"

mkdir -p "$LOG_DIR"

cd "$CTP2_ROOT"

HEAD_SHA="$(git rev-parse --short HEAD 2>/dev/null || echo working)"
HEAD_SUBJECT="$(git log -1 --pretty=%s 2>/dev/null || echo '')"
HEAD_BRANCH="$(git rev-parse --abbrev-ref HEAD 2>/dev/null || echo '')"

mise exec -- python3 "$CI_ROOT/update_state.py" \
    --status running --running-tier B \
    --head-sha "$HEAD_SHA" --head-subject "$HEAD_SUBJECT" --head-branch "$HEAD_BRANCH" \
    >/dev/null

START=$(date +%s)

(
    echo "=== tier-b $TS (HEAD $HEAD_SHA) ==="
    echo "=== ninja -C build ctp2 ctp2_headless ctp2_unit_tests ctp2_fast_tests ==="
    mise exec -- ninja -C build ctp2 ctp2_headless ctp2_unit_tests ctp2_fast_tests
    BUILD_RC=$?
    if [[ $BUILD_RC -ne 0 ]]; then
        echo "BUILD FAILED rc=$BUILD_RC"
        exit $BUILD_RC
    fi

    echo "=== ./build/ctp2_unit_tests ==="
    mise exec -- ./build/ctp2_unit_tests -r=xml --no-version --test-suite-exclude=integration > "$XML_FILE" 2>>"$LOG_FILE"
    UNIT_RC=$?

    echo "=== current scenarios, fast tests, installation and save replays ==="
    mise exec -- meson test -C build --no-rebuild --print-errorlogs \
        --suite scenario --no-suite marathon || exit $?
    mise exec -- meson test -C build --no-rebuild --print-errorlogs \
        fast asset-installer client-harness ci-results cli-save-resume cli-save-replay cli-save-replay-long
    SCENARIO_RC=$?
    if [[ $UNIT_RC -ne 0 || $SCENARIO_RC -ne 0 ]]; then
        exit 1
    fi
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
    --tier b --status "$STATUS" --duration-s "$DURATION" \
    --result "$RESULT_JSON" \
    --head-sha "$HEAD_SHA" --head-subject "$HEAD_SUBJECT" --head-branch "$HEAD_BRANCH" \
    --context-path "$LOG_FILE" \
    --clear-running \
    >/dev/null

# Append tier-b duration to metrics history for the "is CI getting slow?" view.
METRICS_CSV="$CI_ROOT/metrics/tier-b-times.csv"
mkdir -p "$(dirname "$METRICS_CSV")"
[[ -s "$METRICS_CSV" ]] || echo "timestamp,head_sha,status,duration_s" > "$METRICS_CSV"
echo "$(date -u +%FT%TZ),$HEAD_SHA,$STATUS,$DURATION" >> "$METRICS_CSV"

# Retain last 30 tier-b log+xml+json triplets.
ls -t "$LOG_DIR"/tier-b-*.log  2>/dev/null | tail -n +31 | xargs rm -f 2>/dev/null
ls -t "$LOG_DIR"/tier-b-*.xml  2>/dev/null | tail -n +31 | xargs rm -f 2>/dev/null
ls -t "$LOG_DIR"/tier-b-*.json 2>/dev/null | tail -n +31 | xargs rm -f 2>/dev/null

[[ "$STATUS" == green ]]
