#include <stdint.h>
#include <stdio.h>

extern void i_kernel_step(void);
static int popcount64(uint64_t x) { return __builtin_popcountll(x); }
static int bitlen64(uint64_t x) { return x ? 64 - __builtin_clzll(x) : 0; }
int main(void) {
    FILE *f = fopen("/mnt/data/kernel_trace_nocounter_v2.csv", "w");
    if (!f) return 1;
    fprintf(f, "tick,word,popcount,bitlen,event,x,y,z,w\n");
    uint64_t x = 0;
    long long sx = 0, sy = 0, sz = 0, sw = 0;
    for (int tick = 0; tick < 256; ++tick) {
        unsigned long long next;
        unsigned char event;
        asm volatile(
            "mov %[in], %%rax\n\t"
            "call i_kernel_step\n\t"
            "mov %%rax, %[out]\n\t"
            "mov %%bl, %[evt]\n\t"
            : [out] "=r" (next), [evt] "=r" (event)
            : [in] "r" (x)
            : "rax", "rbx", "rcx", "rdx", "cc", "memory"
        );
        x = next;
        sx += (x & 1ULL) ? 1 : -1;
        sy += (x & 2ULL) ? 1 : -1;
        sz += (x & 4ULL) ? 1 : -1;
        sw += (x & 8ULL) ? 1 : -1;
        fprintf(f, "%d,%llu,%d,%d,%u,%lld,%lld,%lld,%lld\n",
                tick, (unsigned long long)x, popcount64(x), bitlen64(x), (unsigned)event, sx, sy, sz, sw);
    }
    fclose(f);
    return 0;
}
