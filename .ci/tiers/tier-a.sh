#!/usr/bin/env bash
# tier-a.sh — fast tier. Build ctp2_fast_tests, run, parse, update state.
#
# Run by the daemon when `ninja -C build -n ctp2_fast_tests` shows pending
# work and the debounce window has elapsed. Can also be invoked manually:
#
#     .ci/tiers/tier-a.sh
#
# Always exits 0 from the script's perspective — failure is communicated
# via .ci/state.json (status: "red") and the STATUS_RED sentinel.

set -uo pipefail

CTP2_ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
CI_ROOT="$CTP2_ROOT/.ci"
LOG_DIR="$CI_ROOT/log"
TS="$(date -u +%Y%m%d-%H%M%S)"
LOG_FILE="$LOG_DIR/tier-a-$TS.log"
XML_FILE="$LOG_DIR/tier-a-$TS.xml"
RESULT_JSON="$LOG_DIR/tier-a-$TS.json"

mkdir -p "$LOG_DIR"

cd "$CTP2_ROOT"

# Head info — Tier A reflects the working tree, but we still record the
# nearest committed SHA so an agent can locate the run.
HEAD_SHA="$(git rev-parse --short HEAD 2>/dev/null || echo working)"
HEAD_SUBJECT="$(git log -1 --pretty=%s 2>/dev/null || echo '')"
HEAD_BRANCH="$(git rev-parse --abbrev-ref HEAD 2>/dev/null || echo '')"

# Mark "running" so an agent polling sees in-progress state.
mise exec -- python3 "$CI_ROOT/update_state.py" \
    --status running --running-tier A \
    --head-sha "$HEAD_SHA" --head-subject "$HEAD_SUBJECT" --head-branch "$HEAD_BRANCH" \
    >/dev/null

START=$(date +%s)

{
    echo "=== tier-a $TS (HEAD $HEAD_SHA) ==="
    echo "=== ninja -C build ctp2_fast_tests ==="
    mise exec -- ninja -C build ctp2_fast_tests
    BUILD_RC=$?
    if [[ $BUILD_RC -ne 0 ]]; then
        echo "BUILD FAILED rc=$BUILD_RC"
        exit $BUILD_RC
    fi

    echo "=== ./build/ctp2_fast_tests ==="
    ./build/ctp2_fast_tests -r=xml --no-version > "$XML_FILE" 2>>"$LOG_FILE"
} >>"$LOG_FILE" 2>&1
RUN_RC=$?

END=$(date +%s)
DURATION=$((END - START))

if [[ $RUN_RC -ne 0 && ! -s "$XML_FILE" ]]; then
    # Build or runner failure before any XML emitted. Synthesize a failure
    # record so the agent can see "build broken" in structured form.
    cat > "$RESULT_JSON" <<EOF
{
  "tests": { "passed": 0, "failed": 1, "skipped": 0 },
  "failures": [
    {
      "test": "tier-a build/run",
      "file": "$LOG_FILE",
      "line": 0,
      "type": "BUILD",
      "original": "ninja + ctp2_fast_tests",
      "expanded": "exit code $RUN_RC",
      "message": "tier-a failed before test run (build error or runner crash; see context_path)",
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
    --tier a --status "$STATUS" --duration-s "$DURATION" \
    --result "$RESULT_JSON" \
    --head-sha "$HEAD_SHA" --head-subject "$HEAD_SUBJECT" --head-branch "$HEAD_BRANCH" \
    --context-path "$LOG_FILE" \
    --clear-running \
    >/dev/null

# Retain last 50 log + xml + result triplets; older are pruned.
ls -t "$LOG_DIR"/tier-a-*.log 2>/dev/null | tail -n +51 | xargs -I{} rm -f {} {%.log}.xml {%.log}.json 2>/dev/null
# The xargs above is approximate (no shell substring magic across xargs);
# do a second pass on .xml/.json to keep them in lockstep:
ls -t "$LOG_DIR"/tier-a-*.xml  2>/dev/null | tail -n +51 | xargs rm -f 2>/dev/null
ls -t "$LOG_DIR"/tier-a-*.json 2>/dev/null | tail -n +51 | xargs rm -f 2>/dev/null

exit 0
