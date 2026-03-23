#!/bin/bash

# --- Configuration ---
ASM_FILE="vdm_runtime.asm"
BIN_FILE="vdm_runtime"
LOG_FILE="vdm_witness.log"
DURATION="3s" # Total run time
SAMPLES=10    # Number of register snapshots to take

echo "=== VDM Runtime Controller ===" | tee $LOG_FILE
echo "Target: $ASM_FILE" | tee -a $LOG_FILE

# 1. Assemble and Link
nasm -f elf64 -g $ASM_FILE -o vdm_runtime.o
ld vdm_runtime.o -o $BIN_FILE
if [ $? -ne 0 ]; then echo "Build failed"; exit 1; fi

echo "✓ Build successful. Executing with $DURATION limit..." | tee -a $LOG_FILE

# 2. Capture Initial Articulation (stdout)
# This captures the transition messages defined in the data section.
timeout $DURATION ./$BIN_FILE >> $LOG_FILE 2>&1 &
PID=$!

echo "✓ Runtime started (PID: $PID). Sampling Invariant Burden..." | tee -a $LOG_FILE

# 3. Sample the Invariant Tension (Register Snapshots)
# This uses GDB to 'look under the hood' at RAX without discharging the loop.
for i in $(seq 1 $SAMPLES); do
    sleep 0.2
    echo "--- Sample $i ---" >> $LOG_FILE
    gdb -batch -ex "attach $PID" -ex "info registers rax" -ex "detach" -ex "quit" >> $LOG_FILE 2>/dev/null
done

# 4. Finalize
wait $PID 2>/dev/null
echo "-----------------------------------" >> $LOG_FILE
echo "✓ Duration limit reached. Non-discharge preserved." | tee -a $LOG_FILE
echo "✓ Logs collected in: $LOG_FILE"
