#!/usr/bin/env bash
# tier-b.sh — commit tier. Build ctp2 + ctp2_headless + ctp2_unit_tests,
# run unit suite, parse, update state.
#
# Triggered by the daemon when HEAD moves. Always exits 0; failure is in
# .ci/state.json + STATUS_RED.

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

{
    echo "=== tier-b $TS (HEAD $HEAD_SHA) ==="
    echo "=== ninja -C build ctp2 ctp2_headless ctp2_unit_tests ==="
    mise exec -- ninja -C build ctp2 ctp2_headless ctp2_unit_tests
    BUILD_RC=$?
    if [[ $BUILD_RC -ne 0 ]]; then
        echo "BUILD FAILED rc=$BUILD_RC"
        exit $BUILD_RC
    fi

    echo "=== ./build/ctp2_unit_tests ==="
    ./build/ctp2_unit_tests -r=xml --no-version > "$XML_FILE" 2>>"$LOG_FILE"
    UNIT_RC=$?

    # Scenario tier: campaign-save fixtures replayed through serve mode
    # (~12s total). These caught a real reload bug on their first run;
    # every promoted repro lands here automatically via the meson suite.
    echo "=== meson test scenario + integration (headless) ==="
    mise exec -- meson test -C build \
        scenario-load-stress scenario-path-resume \
        slice-headless expansion-headless
    SCENARIO_RC=$?
    if [[ $UNIT_RC -ne 0 || $SCENARIO_RC -ne 0 ]]; then
        exit 1
    fi
} >>"$LOG_FILE" 2>&1
RUN_RC=$?

END=$(date +%s)
DURATION=$((END - START))

if [[ $RUN_RC -ne 0 && ! -s "$XML_FILE" ]]; then
    cat > "$RESULT_JSON" <<EOF
{
  "tests": { "passed": 0, "failed": 1, "skipped": 0 },
  "failures": [
    {
      "test": "tier-b build/run",
      "file": "$LOG_FILE",
      "line": 0,
      "type": "BUILD",
      "original": "ninja ctp2 ctp2_headless ctp2_unit_tests + run",
      "expanded": "exit code $RUN_RC",
      "message": "tier-b failed before test run (build error or runner crash; see context_path)",
      "info": []
    }
  ]
}
EOF
    STATUS=red
else
    mise exec -- python3 "$CI_ROOT/parse_doctest_xml.py" "$XML_FILE" > "$RESULT_JSON" 2>>"$LOG_FILE"
    FAILED=$(mise exec -- python3 -c "import json,sys; print(json.load(open('$RESULT_JSON'))['tests']['failed'])")
    if [[ "$FAILED" == "0" ]]; then
        STATUS=green
    else
        STATUS=red
    fi
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

exit 0
