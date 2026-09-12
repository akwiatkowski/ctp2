#!/usr/bin/env bash
# See .ci/README.md for coverage. Failures update state and return nonzero.

set -uo pipefail

CTP2_ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
CI_ROOT="$CTP2_ROOT/.ci"

# Shared ccache store across worktrees (harmless when ccache absent).
export CCACHE_DIR="${CCACHE_DIR:-$HOME/.cache/ccache-ctp2}"
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

export UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1"

START=$(date +%s)

(
    echo "=== tier-d $TS (HEAD $HEAD_SHA) ==="

    if [[ ! -f "$CTP2_ROOT/build-ubsan/build.ninja" ]]; then
        echo "=== meson setup build-ubsan -Db_sanitize=undefined -Db_lundef=false ==="
        ( cd ctp2_code && mise exec -- meson setup ../build-ubsan \
            -Danet=false -Dbuildtype=debugoptimized \
            -Db_sanitize=undefined -Db_lundef=false ) || exit $?
    fi

    mise exec -- meson configure build-ubsan \
        -Danet=false -Dbuildtype=debugoptimized -Db_sanitize=undefined || exit $?

    echo "=== ninja -C build-ubsan ctp2_unit_tests ctp2_headless ==="
    mise exec -- ninja -C build-ubsan ctp2_unit_tests ctp2_headless
    BUILD_RC=$?
    if [[ $BUILD_RC -ne 0 ]]; then
        echo "BUILD FAILED rc=$BUILD_RC"
        exit $BUILD_RC
    fi

    echo "=== ./build-ubsan/ctp2_unit_tests (UBSan) ==="
    mise exec -- ./build-ubsan/ctp2_unit_tests -r=xml --no-version > "$XML_FILE" 2>>"$LOG_FILE"
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

[[ "$STATUS" == green ]]
