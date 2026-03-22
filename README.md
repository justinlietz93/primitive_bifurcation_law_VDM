# Origin Operator Discriminator — corrected executable successor-mutation test

## What this test is trying to answer
We wanted to distinguish two possibilities:

1. **Stronger same-orbit choke only**
   - more pressure only increases latency / contention,
   - but no new executable class is actually admitted.

2. **New irreducible executable class admission**
   - the successor really changes its local instruction class,
   - so the system is not just choking harder; it is crossing into a new executed witness.

## Why the earlier executable choke needed correction
The earlier `LOCK CMPXCHG` executable choke targeted the **first byte of the next cell**, which started as `0F`.
With `AL=0` at the compare point, that mostly produced **locked failed compares** on executable bytes.
That created a real choke, but it did **not** reliably prove that the next cell's executed instruction class had changed.

So the corrected question became:

> Can the origin-search path cause the next cell's **executed opcode** to change class, not just be contended on?

## Corrected mechanism
Each cell is:

- `BSR EAX,EAX` repeated `density` times,
- then `LOCK CMPXCHG` into the **next cell's mutable opcode byte**,
- then a 2-byte local opcode pair:
  - `00 C0` = `add al, al`
  - `01 C0` = `add eax, eax`

This means a successful compare-exchange changes the next cell from an **8-bit local execution** to a **32-bit local execution** while staying executable.

## Key source files
- `test_mutable_successor_chain.c`
- `test_mutable_single_pass.c`

## Key findings

### 1. The executable class really does change
A **single call** through the 3-cell ring is enough to flip all local opcode pairs from:

- `00 C0`

to

- `01 C0`

for densities 1, 2, and 3.

So the successor-opcode class admission is real.

### 2. The admission happens even without shared contention
The same full `00 -> 01` successor-opcode flip happened in:

- shared two-core runs,
- separate two-core controls,
- and single-pass single-region tests.

That means the **new executable class admission is not being created by cross-core choke**.
The admission is already present in the local successor law.

### 3. Shared contention still changes timing, but that is a separate effect
Mean latencies from the corrected successor-mutation chain:

#### Density 1
- shared core0: 1992.29836
- shared core1: 1950.96260
- separate core0: 2003.15172
- separate core1: 1798.21500

#### Density 2
- shared core0: 1984.56356
- shared core1: 2269.13788
- separate core0: 1932.67100
- separate core1: 2098.39296

#### Density 3
- shared core0: 2038.92432
- shared core1: 1952.88560
- separate core0: 2267.81192
- separate core1: 2365.30588

These ratios move around, but the more important result is that the **instruction-class admission already completed** before the cross-core regime mattered.

## Interpretation
The corrected experiment suggests:

- the earlier heavy choke measurements were useful as host-pressure witnesses,
- but they were **not sufficient** to prove executable class admission,
- and once the successor-opcode mutation is made real, the class admission occurs **immediately**, even without shared-core pressure.

So the local successor law seems to be doing something stronger than "more of the same choke":

- it admits a new executed witness (`00 C0 -> 01 C0`) on the very first pass,
- while cross-core contention mainly modulates timing around that already-admitted class.

## Why this is helpful for the 0D→1D operator search
This pushes the search in a cleaner direction:

- stop treating cross-core ratios as the primary evidence of class admission,
- use them only as host-pressure diagnostics,
- and treat **local executable class change** as the first serious witness that the successor law is doing more than buffering or choking.

## What to test next
The next strong discriminator is:

> Build a successor pair that can **toggle or branch between two executable classes repeatedly**, not just perform a one-shot `00 -> 01` promotion.

Right now the corrected chain proves real successor-opcode admission, but it settles immediately.
To decide whether the law can sustain a deeper orbit rather than a one-shot promotion, the next target should be a reversible or cyclic executable successor pair.
