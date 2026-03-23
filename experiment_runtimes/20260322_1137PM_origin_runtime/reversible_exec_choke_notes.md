# Reversible Executable Choke Test

## Goal
Test whether the **reversible successor orbit** survives and develops a stronger timing split when the orbit itself is placed under two-core shared executable contention.

## Mechanism
Each of 3 cells executes:

- `BSR EAX,EAX`
- `SETZ DL`
- `LOCK XOR byte ptr [next_cell_mutable_opcode], DL`
- mutable successor pair `00 C0 <-> 01 C0`

The mutable pair is executable, so the successor's opcode class is toggled by the origin-trigger path itself.

## Cases
- **Shared**: both cores execute the same mutable ring
- **Separate**: each core executes its own mutable ring

## Measurements
- per-core mean latency
- shared/separate ratio
- odd/even parity means as a crude orbit-phase split
- final mutable opcode states
