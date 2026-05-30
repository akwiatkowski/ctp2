#!/usr/bin/env bash
# daemon.sh — CTP2 local-CI daemon.
#
# Loops every POLL_INTERVAL seconds. Each tick:
#   1. If HEAD moved since last_head, schedule Tier B.
#   2. Else, if `ninja -n ctp2_fast_tests` shows pending work AND debounce
#      window has elapsed, schedule Tier A.
#   3. Run one queued tier (never overlap).
#
# All logging goes to .ci/log/daemon.log. The daemon is single-process —
# only one instance should ever be running. PID lives in .ci/daemon.pid.
#
# Lifecycle (v1, pre-launchd):
#   make ci-start      → starts daemon under nohup
#   make ci-stop       → kills the daemon
#   make ci-status     → cats state.json
#   make ci-watch      → tails daemon.log
#
# Configuration (env vars):
#   POLL_INTERVAL      — seconds between ticks (default 3)
#   TIER_A_DEBOUNCE    — seconds since last Tier A run before another can fire (default 5)
#   ENABLE_TIER_B      — set to 0 to disable Tier B scheduling (default 1)

set -uo pipefail

CTP2_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
CI_ROOT="$CTP2_ROOT/.ci"
LOG_FILE="$CI_ROOT/log/daemon.log"
PID_FILE="$CI_ROOT/daemon.pid"
LAST_HEAD_FILE="$CI_ROOT/last_head"

POLL_INTERVAL="${POLL_INTERVAL:-3}"
TIER_A_DEBOUNCE="${TIER_A_DEBOUNCE:-5}"
ENABLE_TIER_B="${ENABLE_TIER_B:-1}"

mkdir -p "$CI_ROOT/log"

# Refuse to start if another daemon is alive.
if [[ -f "$PID_FILE" ]]; then
    OLD_PID="$(cat "$PID_FILE" 2>/dev/null || true)"
    if [[ -n "$OLD_PID" ]] && kill -0 "$OLD_PID" 2>/dev/null; then
        echo "daemon already running (PID $OLD_PID); refusing to start" >&2
        exit 1
    fi
    rm -f "$PID_FILE"
fi
echo "$$" > "$PID_FILE"

cleanup() {
    rm -f "$PID_FILE"
}
trap cleanup EXIT

log() {
    printf '[%s] %s\n' "$(date '+%Y-%m-%d %H:%M:%S')" "$*" >> "$LOG_FILE"
}

last_tier_a_run=0

log "daemon started (PID $$ POLL=$POLL_INTERVAL TIER_A_DEBOUNCE=$TIER_A_DEBOUNCE ENABLE_TIER_B=$ENABLE_TIER_B)"

while :; do
    cd "$CTP2_ROOT"

    # --- Tier B trigger: HEAD moved? -------------------------------------
    if [[ "$ENABLE_TIER_B" == "1" ]]; then
        current_head="$(git rev-parse HEAD 2>/dev/null || echo none)"
        last_head="$(cat "$LAST_HEAD_FILE" 2>/dev/null || echo none)"
        if [[ "$current_head" != "$last_head" && "$current_head" != "none" ]]; then
            log "HEAD changed: $last_head -> $current_head; running tier-b"
            if [[ -x "$CI_ROOT/tiers/tier-b.sh" ]]; then
                "$CI_ROOT/tiers/tier-b.sh" >> "$LOG_FILE" 2>&1
            else
                log "tier-b.sh not yet installed; skipping (just recording HEAD)"
            fi
            echo "$current_head" > "$LAST_HEAD_FILE"
            last_tier_a_run=$(date +%s)  # treat post-B as fresh; suppress immediate A
        fi
    fi

    # --- Tier A trigger: working tree has pending builds? ---------------
    now=$(date +%s)
    if (( now - last_tier_a_run >= TIER_A_DEBOUNCE )); then
        # ninja -n prints what WOULD build. Empty stdout = nothing to do.
        pending="$(mise exec -- ninja -C build -n ctp2_fast_tests 2>/dev/null \
                   | grep -vE '^ninja:( no work to do| Entering directory)' || true)"
        if [[ -n "$pending" ]]; then
            log "tier-a: pending work detected; running"
            "$CI_ROOT/tiers/tier-a.sh" >> "$LOG_FILE" 2>&1
            last_tier_a_run=$(date +%s)
        fi
    fi

    sleep "$POLL_INTERVAL"
done
