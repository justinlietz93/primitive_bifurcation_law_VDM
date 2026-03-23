# 3D falsifier check from `test.log`
Observed unique `mut` states from the log:
- `000`
- `011`
- `110`
- `101`

Observed cycle:

`000 -> 011 -> 110 -> 101 -> 000`

## Pairwise Hamming distances
- d(`000`, `011`) = 2
- d(`000`, `110`) = 2
- d(`000`, `101`) = 2
- d(`011`, `110`) = 2
- d(`011`, `101`) = 2
- d(`110`, `101`) = 2

All six pairwise distances are exactly 2.

## Hard falsifier
A set of 4 binary states with all pairwise Hamming distances equal to 2 **does not exist** in 1 bit or 2 bits. Brute-force check:
- dimension 1: does NOT exist
- dimension 2: does NOT exist
- dimension 3: exists

So if we require the **full pairwise Hamming geometry** seen in the log, the carrier needs at least **3 binary coordinates**.

## Important nuance
The four states also satisfy the parity constraint `x ⊕ y ⊕ z = 0`, so inside the 3-bit cube they lie on the even-parity subset. Over GF(2) their affine rank is 2.
- affine GF(2) rank = 2

That means:
- **extrinsic carrier dimension**: 3 observed binary coordinates are needed to realize the full equidistant geometry
- **intrinsic constrained manifold**: the visited subset is 2D (a parity plane / 2D affine subspace) inside that 3-bit carrier

## What this means
If the question is **"are we already using a 3-coordinate carrier?"**, the log says **yes**.
If the question is **"has the orbit escaped a 2D constrained manifold?"**, the log says **not yet**.
So the strongest current statement is:
> the runtime is already expressed on a 3-bit carrier, but the observed orbit is confined to a 2D parity-constrained subset of that carrier.
