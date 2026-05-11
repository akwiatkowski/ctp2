#!/bin/bash
# run-with-debugger.sh — run CTP2 under lldb with auto-backtrace on crash
set -e

BUILDDIR="/Users/olek/projects/llm/games/ctp2/build"
EXEDIR="$BUILDDIR/ctp2_code"

# Write lldb commands
LLDB_SCRIPT=$(mktemp)
trap "rm -f $LLDB_SCRIPT" EXIT

cat > "$LLDB_SCRIPT" << 'EOF'
settings set target.load-script-from-symbol-file true
breakpoint set -n exit -o true -C "bt all"
run
bt all
thread backtrace all
quit
EOF

echo "=== Starting CTP2 under lldb ==="
echo "Click 'Next Turn' to reproduce the crash."
echo ""

lldb -s "$LLDB_SCRIPT" -- "$EXEDIR/ctp2"
