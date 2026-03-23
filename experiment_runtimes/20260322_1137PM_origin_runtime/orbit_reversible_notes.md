# Reversible Executable Successor Orbit Test

## Goal
Test whether the origin-driven successor mutation can sustain a **cyclic executable class-change orbit** rather than a one-shot promotion.

## Hypothesis
A cell can use the zero-trigger path

- `BSR EAX,EAX`
- `SETZ DL`
- `LOCK XOR [next_cell_mutable_opcode], DL`

so that entering one cell toggles the first byte of the next cell's mutable opcode pair between:

- `00 C0` = `add al, al`
- `01 C0` = `add eax, eax`

This gives a reversible successor pair. If the mechanism is real, repeated passes through a 3-cell ring should alternate the successor class globally:

- pass 0: `00 00 00`
- pass 1: `01 01 01`
- pass 2: `00 00 00`
- ...

## Why this matters
The earlier mutable-successor test showed **real executable class admission**, but it was one-shot. This test asks whether the origin can drive a **stable orbit** in the successor itself.

## What is measured
1. **Pass-by-pass opcode states** for the 3 mutable successor bytes.
2. **Pass-by-pass latency** to see whether the class orbit has a timing footprint.
3. A long-run odd/even split to see whether the two orbit phases remain distinguishable under repetition.

## Interpretation rule
- If the mutable opcode states alternate cleanly, the successor pair is genuinely reversible.
- If odd/even passes show different mean latency, the orbit is not just stored-state alternation but an executed timing-class alternation.
