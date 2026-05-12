#!/bin/bash
# CTP2 launcher with crash capture
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
LOG="test/crashes/crash_${TIMESTAMP}.log"
mkdir -p test/crashes

echo "Launching CTP2, logging to $LOG"
echo "Play the game. If it crashes, the backtrace will be in $LOG"
./build-sanitized/ctp2 "$@" 2>&1 | tee "$LOG"

EXIT=${PIPESTATUS[0]}
if [ $EXIT -ne 0 ]; then
    echo ""
    echo "========================================"
    echo "GAME CRASHED with exit code $EXIT"
    echo "Crash log: $LOG"
    echo "========================================"
fi
