#!/usr/bin/env bash
# SSH benchmark — measures connection + command execution time using the ssh CLI.
# Usage: ./bench.sh user@host [iterations] [port]

set -euo pipefail

DEST="${1:?Usage: $0 user@host [iterations] [port]}"
ITERATIONS="${2:-10}"
PORT="${3:-22}"

total_ms=0

for i in $(seq 1 "$ITERATIONS"); do
    start=$(python3 -c 'import time; print(int(time.time()*1000))')
    ssh -o StrictHostKeyChecking=no -o LogLevel=ERROR -p "$PORT" "$DEST" ls > /dev/null
    end=$(python3 -c 'import time; print(int(time.time()*1000))')
    elapsed=$((end - start))
    total_ms=$((total_ms + elapsed))
    printf "  run %2d: %d ms\n" "$i" "$elapsed"
done

avg=$((total_ms / ITERATIONS))
echo ""
echo "shell/ssh  — iterations: $ITERATIONS  total: ${total_ms} ms  avg: ${avg} ms"
