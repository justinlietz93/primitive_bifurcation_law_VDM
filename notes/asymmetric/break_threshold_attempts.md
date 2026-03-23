# Break-threshold attempt summary

I pushed three asymmetric successor variants after the symmetric reversible orbit.

## Variants tried

1. **Direct successor-targeted 16-bit asymmetric pair**
   - mutable pair: `00 C0 <-> F7 D0`
   - mutation: `LOCK XOR word ptr [next_mutable_pair], 0x10F7`
   - result: immediate **segmentation fault** before a stable pass trace could be recorded

2. **Deferred self-targeted 16-bit asymmetric pair**
   - mutable pair: `00 C0 <-> F7 D0`
   - mutation: `LOCK XOR word ptr [self_mutable_pair], 0x10F7`
   - result: immediate **segmentation fault** before a stable pass trace could be recorded

3. **Byte-asymmetric successor pair**
   - mutable pair: `00 C0 <-> 28 C0` (`add al,al` <-> `sub al,al`)
   - mutation: `LOCK XOR byte ptr [next_mutable_opcode], 0x28`
   - result: immediate **segmentation fault** before a stable pass trace could be recorded

## What this means

The earlier symmetric reversible orbit (`00 C0 <-> 01 C0`) stayed executable and stable.

As soon as the successor mutation becomes **asymmetric enough to change execution class more aggressively**, this VM stops yielding a soft timing split and instead hits a **hard fetch/execution break**.

That is useful information:

- the reversible orbit is real
- the mild symmetric pair is still inside the host's stable manifold
- the stronger asymmetric successor mutations cross a real break threshold on this host

## Best current interpretation

This is the first clean sign that we are no longer just measuring "more delay." The host stops tolerating the successor mutation as a smooth orbit and instead fails fast.

On this VM, that means the next good move is **not** another asymmetry variant in the same style. The next good move is to reproduce this on bare-metal AMD, where instruction-fetch and cache-coherency behavior will be much less contaminated by the hypervisor.
