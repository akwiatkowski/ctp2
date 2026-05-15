#!/usr/bin/env bash
# Long-form AI-vs-AI playthrough under ASan + UBSan.
#
# Drives the game through new_game/start_game/enable_autoplay and then
# runs end_turn for N rounds while sanitizers watch. After the run it
# collects ASan/UBSan/Assert hits, deduplicates by source location, and
# writes a Markdown report to /tmp/autoplay-report-<timestamp>.md.
#
# Usage:
#   ctp2_code/test/autoplay_run.sh [turns]
#
# Env overrides:
#   AUTOPLAY_TURNS            — number of end_turn cycles (default 100)
#   AUTOPLAY_TURN_PACE        — seconds to wait between end_turn cmds (default 2.0)
#   AUTOPLAY_TURN_TIMEOUT     — per-end_turn socket timeout (default 60)
#   AUTOPLAY_OVERALL_TIMEOUT  — hard cap on the whole run (default 2400)
#   CTP2_BINARY               — game binary (default: build-sanitized/ctp2)

set -u

# Resolve project root from script location.
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
cd "$PROJECT_ROOT"

TURNS="${AUTOPLAY_TURNS:-${1:-100}}"
PACE="${AUTOPLAY_TURN_PACE:-2.0}"
TURN_TIMEOUT="${AUTOPLAY_TURN_TIMEOUT:-60}"
OVERALL_TIMEOUT="${AUTOPLAY_OVERALL_TIMEOUT:-2400}"
BINARY="${CTP2_BINARY:-$PROJECT_ROOT/build-sanitized/ctp2}"

if [[ ! -x "$BINARY" ]]; then
    echo "ERROR: $BINARY is not executable."
    echo "       Rebuild with: ninja -C build-sanitized ctp2"
    exit 2
fi

# Warn if any tracked source is newer than the binary — silent staleness is
# the easiest way to chase phantom sanitizer hits.
NEWEST_SRC=$(find ctp2_code \( -name '*.cpp' -o -name '*.h' -o -name '*.c' \) \
                  -newer "$BINARY" 2>/dev/null | head -3)
if [[ -n "$NEWEST_SRC" ]]; then
    echo "WARN: sanitized binary is older than source files:"
    echo "$NEWEST_SRC" | sed 's/^/      /'
    echo "      Rebuild before trusting results: ninja -C build-sanitized ctp2"
    echo
fi

# UBSan suppressions file is gitignored. Materialize a default if missing so
# fresh checkouts work without manual setup.
SUPP="$PROJECT_ROOT/ubsan-suppressions.txt"
if [[ ! -f "$SUPP" ]]; then
    cat > "$SUPP" <<'EOF'
# UBSan suppressions for CTP2. Format: <check>:<source-pattern>.
vptr:*aui_win.cpp*
vptr:*aui_ui.cpp*
EOF
    echo "Created default $SUPP"
fi

# Fresh slate for sanitizer logs.
/bin/rm -f /tmp/asan-auto.log.* /tmp/ubsan-auto.log.* /tmp/ctp2-autoplay-game.log

TS=$(date +%Y%m%d-%H%M%S)
REPORT="/tmp/autoplay-report-${TS}.md"

{
    echo "# AI-vs-AI Autoplay Report"
    echo
    echo "- Date: $(date '+%Y-%m-%d %H:%M:%S %Z')"
    echo "- Binary: \`$BINARY\`"
    echo "- Binary mtime: $(stat -f '%Sm' "$BINARY" 2>/dev/null || stat -c '%y' "$BINARY")"
    echo "- Turns requested: $TURNS"
    echo "- Pace: ${PACE}s between end_turn cmds"
    echo "- Overall timeout: ${OVERALL_TIMEOUT}s"
    echo
} | tee "$REPORT"

export CTP2_BINARY="$BINARY"
export CTP2_CWD="$PROJECT_ROOT"
export AUTOPLAY_TURNS="$TURNS"
export AUTOPLAY_TURN_PACE="$PACE"
export AUTOPLAY_TURN_TIMEOUT="$TURN_TIMEOUT"

# halt_on_error=0 keeps execution going past each error so we capture the full
# set, not just the first one. log_path writes per-PID log files we can scrape.
export ASAN_OPTIONS="abort_on_error=0:halt_on_error=0:detect_leaks=0:log_path=/tmp/asan-auto.log"
export UBSAN_OPTIONS="halt_on_error=0:print_stacktrace=1:suppressions=$PROJECT_ROOT/ubsan-suppressions.txt:log_path=/tmp/ubsan-auto.log"

echo "## Driver Output" | tee -a "$REPORT"
echo '```' >> "$REPORT"
t0=$(date +%s)
timeout "$OVERALL_TIMEOUT" python3 "$SCRIPT_DIR/autoplay_test.py" 2>&1 | tee -a "$REPORT"
RUN_EXIT=${PIPESTATUS[0]}
t1=$(date +%s)
echo '```' >> "$REPORT"

WALL=$((t1 - t0))

{
    echo
    echo "- Driver exit code: $RUN_EXIT"
    echo "- Wall time: ${WALL}s"
    echo
} | tee -a "$REPORT"

# --- Coverage ---
ROUNDS=0
if [[ -f /tmp/ctp2-autoplay-game.log ]]; then
    ROUNDS=$(grep -c 'BeginTurnEvent' /tmp/ctp2-autoplay-game.log || echo 0)
fi
{
    echo "## Coverage"
    echo
    echo "- BeginTurnEvents observed: $ROUNDS"
    echo "- Game log: \`/tmp/ctp2-autoplay-game.log\` ($(wc -l < /tmp/ctp2-autoplay-game.log 2>/dev/null || echo 0) lines)"
    echo
} | tee -a "$REPORT"

# --- UBSan summary ---
{
    echo "## UBSan Hits"
    echo
} | tee -a "$REPORT"

if compgen -G "/tmp/ubsan-auto.log.*" > /dev/null; then
    {
        echo "### By source location"
        echo
        echo '| Hits | Location |'
        echo '|-----:|----------|'
        cat /tmp/ubsan-auto.log.* 2>/dev/null \
            | grep -oE '\.\./[^ ]+\.(cpp|h):[0-9]+:[0-9]+' \
            | sort | uniq -c | sort -rn \
            | awk '{printf "| %4d | `%s` |\n", $1, $2}'
        echo
        echo "### By error type"
        echo
        echo '| Hits | Error |'
        echo '|-----:|-------|'
        cat /tmp/ubsan-auto.log.* 2>/dev/null \
            | grep -oE 'runtime error: [^|]+' \
            | sed 's/ *$//' \
            | sort | uniq -c | sort -rn \
            | awk '{n=$1; sub(/^ *[0-9]+ /, ""); printf "| %4d | %s |\n", n, $0}'
        echo
        echo "Raw logs: \`/tmp/ubsan-auto.log.*\`"
    } | tee -a "$REPORT"
else
    echo "_None._" | tee -a "$REPORT"
fi
echo | tee -a "$REPORT"

# --- ASan summary ---
{
    echo "## ASan Hits"
    echo
} | tee -a "$REPORT"

# When ASan and UBSan are linked together (as in a build with both -fsanitize
# flags), their runtimes share output streams — ASan errors can land in the
# UBSan log_path and vice versa. Scan both globs for ASan signatures.
SAN_LOGS=()
compgen -G "/tmp/asan-auto.log.*" > /dev/null && SAN_LOGS+=(/tmp/asan-auto.log.*)
compgen -G "/tmp/ubsan-auto.log.*" > /dev/null && SAN_LOGS+=(/tmp/ubsan-auto.log.*)

if [[ ${#SAN_LOGS[@]} -gt 0 ]] && grep -lE 'AddressSanitizer:' "${SAN_LOGS[@]}" >/dev/null 2>&1; then
    {
        echo '```'
        grep -hE 'AddressSanitizer:|SUMMARY: AddressSanitizer|#[0-9]+ 0x.*UnitActor|#[0-9]+ 0x.*SpriteGroupList' "${SAN_LOGS[@]}" 2>/dev/null | head -60
        echo '```'
        echo
        echo "Source locations in ASan stack traces:"
        grep -hE 'in .* [^/]+\.(cpp|h):[0-9]+' "${SAN_LOGS[@]}" 2>/dev/null \
            | grep -oE '[^ /]+\.(cpp|h):[0-9]+' | sort | uniq -c | sort -rn | head -20 \
            | awk '{printf "  %4d  %s\n", $1, $2}'
        echo
        echo "Raw logs: \`/tmp/{asan,ubsan}-auto.log.*\`"
    } | tee -a "$REPORT"
else
    echo "_None._" | tee -a "$REPORT"
fi
echo | tee -a "$REPORT"

# --- Asserts / aborts in game log ---
{
    echo "## Asserts / Aborts in Game Log"
    echo
} | tee -a "$REPORT"

# BSD grep -c exits 1 on zero matches; pipe to wc -l to get a clean integer
# regardless of match count or missing file.
ASSERT_HITS=$(grep -E '^Assert|^ASSERT|aborted|Abort trap' /tmp/ctp2-autoplay-game.log 2>/dev/null | wc -l | tr -d ' ')
if [[ "$ASSERT_HITS" -gt 0 ]]; then
    {
        echo '```'
        grep -E '^Assert|^ASSERT|aborted|Abort trap' /tmp/ctp2-autoplay-game.log | head -30
        echo '```'
    } | tee -a "$REPORT"
else
    echo "_None._" | tee -a "$REPORT"
fi
echo | tee -a "$REPORT"

# --- Verdict ---
{
    echo "## Verdict"
    echo
    UBSAN_TOTAL=0
    if compgen -G "/tmp/ubsan-auto.log.*" > /dev/null; then
        UBSAN_TOTAL=$(grep -c 'runtime error' /tmp/ubsan-auto.log.* 2>/dev/null | awk -F: '{s+=$NF} END{print s+0}')
    fi
    ASAN_TOTAL=0
    if [[ ${#SAN_LOGS[@]} -gt 0 ]]; then
        ASAN_TOTAL=$(grep -hE 'AddressSanitizer:' "${SAN_LOGS[@]}" 2>/dev/null | wc -l | tr -d ' ')
    fi
    echo "- UBSan: **${UBSAN_TOTAL}** total events"
    echo "- ASan: **${ASAN_TOTAL}** total events"
    echo "- Asserts: **${ASSERT_HITS}**"
    echo "- Driver: exit=${RUN_EXIT}, rounds=${ROUNDS}/${TURNS} requested"
} | tee -a "$REPORT"

echo
echo "Full report: $REPORT"

exit "$RUN_EXIT"
