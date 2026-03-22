#!/usr/bin/env bash
set -euo pipefail
BIN="/mnt/data/f7d0_origin_loop"
LOG="/mnt/data/f7d0_origin_loop.log"
TRACE="/mnt/data/f7d0_gdb_trace.txt"

: > "$LOG"
: > "$TRACE"

echo 'running timeout smoke test...' | tee -a "$LOG"
set +e
timeout 1s "$BIN" >> "$LOG" 2>&1
status=$?
set -e
printf 'exit_status=%s\n' "$status" | tee -a "$LOG"
if [ "$status" = "124" ]; then
  echo 'timed_out_as_expected=1' | tee -a "$LOG"
else
  echo 'timed_out_as_expected=0' | tee -a "$LOG"
fi

gdb -q -batch \
  -ex 'set pagination off' \
  -ex 'file /mnt/data/f7d0_origin_loop' \
  -ex 'starti' \
  -ex 'set $i=0' \
  -ex 'while $i < 10' \
  -ex 'printf "rip=%#lx eax=%#x\\n", $rip, $eax' \
  -ex 'x/2i $rip' \
  -ex 'si' \
  -ex 'set $i=$i+1' \
  -ex 'end' \
  > "$TRACE" 2>&1

cat "$TRACE" >> "$LOG"
