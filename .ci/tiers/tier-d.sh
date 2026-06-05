#!/usr/bin/env bash
# tier-d.sh — UBSan tier.  Build ctp2_headless under -Db_sanitize=undefined
# and run the unit + integration suites under UBSan.  Catches semantic
# UB ASan does not: invalid enum loads, signed integer overflow, null
# member calls, alignment violations.
#
# Complements tier-c (ASan).  The two sanitizers find different bugs:
# ASan = spatial (OOB, UAF, double-free), UBSan = semantic (enum cast,
# signed overflow, null deref).  Overlap is small; both are worth
# their compile time.
#
# Triggered by the daemon when tier-b just went green (no point firing
# UBSan on a broken build).  Always exits 0; failure goes to .ci/state.json
# + STATUS_RED with the UBSan summary captured to context_path.

set -uo pipefail

CTP2_ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
CI_ROOT="$CTP2_ROOT/.ci"
LOG_DIR="$CI_ROOT/log"
TS="$(date -u +%Y%m%d-%H%M%S)"
LOG_FILE="$LOG_DIR/tier-d-$TS.log"
XML_FILE="$LOG_DIR/tier-d-$TS.xml"
RESULT_JSON="$LOG_DIR/tier-d-$TS.json"

mkdir -p "$LOG_DIR"

cd "$CTP2_ROOT"

HEAD_SHA="$(git rev-parse --short HEAD 2>/dev/null || echo working)"
HEAD_SUBJECT="$(git log -1 --pretty=%s 2>/dev/null || echo '')"
HEAD_BRANCH="$(git rev-parse --abbrev-ref HEAD 2>/dev/null || echo '')"

mise exec -- python3 "$CI_ROOT/update_state.py" \
    --status running --running-tier D \
    --head-sha "$HEAD_SHA" --head-subject "$HEAD_SUBJECT" --head-branch "$HEAD_BRANCH" \
    >/dev/null

# UBSan output is verbose; tune to fail-fast on first error so the daemon
# captures a focused report.  detect_leaks=0 because the codebase has
# documented singleton leaks (g_theStringDB, etc.) that aren't actionable
# without a larger ownership refactor.
export ASAN_OPTIONS="halt_on_error=1:abort_on_error=1:print_summary=1:print_stacktrace=1:detect_leaks=0:exitcode=1"

START=$(date +%s)

{
    echo "=== tier-d $TS (HEAD $HEAD_SHA) ==="

    if [[ ! -d "$CTP2_ROOT/build-ubsan" ]]; then
        echo "=== meson setup build-ubsan -Db_sanitize=undefined -Db_lundef=false ==="
        ( cd ctp2_code && mise exec -- meson setup ../build-ubsan \
            -Db_sanitize=undefined -Db_lundef=false ) || exit $?
    fi

    echo "=== ninja -C build-ubsan ctp2_unit_tests ctp2_headless ==="
    mise exec -- ninja -C build-ubsan ctp2_unit_tests ctp2_headless
    BUILD_RC=$?
    if [[ $BUILD_RC -ne 0 ]]; then
        echo "BUILD FAILED rc=$BUILD_RC"
        exit $BUILD_RC
    fi

    echo "=== ./build-ubsan/ctp2_unit_tests (UBSan) ==="
    ./build-ubsan/ctp2_unit_tests -r=xml --no-version > "$XML_FILE" 2>>"$LOG_FILE"
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
      "test": "tier-d build/run",
      "file": "$LOG_FILE",
      "line": 0,
      "type": "BUILD",
      "original": "ninja ctp2_unit_tests ctp2_headless (asan) + run",
      "expanded": "exit code $RUN_RC",
      "message": "tier-d failed before test run (asan build error or sanitizer crash; see context_path)",
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
    --tier d --status "$STATUS" --duration-s "$DURATION" \
    --result "$RESULT_JSON" \
    --head-sha "$HEAD_SHA" --head-subject "$HEAD_SUBJECT" --head-branch "$HEAD_BRANCH" \
    --context-path "$LOG_FILE" \
    --clear-running \
    >/dev/null

# Metrics: track tier-d duration over time.  UBSan builds are typically
# 2-3x slower than tier-b; this view tells us if sanitizer overhead is
# drifting.
METRICS_CSV="$CI_ROOT/metrics/tier-d-times.csv"
mkdir -p "$(dirname "$METRICS_CSV")"
if [[ ! -s "$METRICS_CSV" ]]; then
    echo "timestamp,head_sha,status,duration_s" > "$METRICS_CSV"
fi
echo "$(date -u +%Y-%m-%dT%H:%M:%SZ),$HEAD_SHA,$STATUS,$DURATION" >> "$METRICS_CSV"
