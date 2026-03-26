#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

extern unsigned char i_kernel_step(uint64_t *word);

int main(void) {
    const int steps = 4096;
    uint64_t w = 1;
    uint64_t axis = 0;

    FILE *f = fopen("/mnt/data/kernel_trace_liveword.csv", "w");
    if (!f) return 1;
    fprintf(f, "tick,word,bitlen,lowbit,event,axis,x,y,z,w4\n");

    long long x=0, y=0, z=0, w4=0;

    for (int t=0; t<steps; ++t) {
        uint64_t before = w;
        uint64_t lowbit = before & (~before + 1ULL);
        int bitlen = 64 - __builtin_clzll(before);
        unsigned char event = i_kernel_step(&w);
        if (event) axis++;

        switch (axis & 3ULL) {
            case 0: x += (before & 1ULL) ? 1 : -1; break;
            case 1: y += (before & 1ULL) ? 1 : -1; break;
            case 2: z += (before & 1ULL) ? 1 : -1; break;
            default: w4 += (before & 1ULL) ? 1 : -1; break;
        }

        fprintf(f, "%d,%llu,%d,%llu,%u,%llu,%lld,%lld,%lld,%lld\n",
                t,
                (unsigned long long)before,
                bitlen,
                (unsigned long long)lowbit,
                (unsigned)event,
                (unsigned long long)axis,
                x,y,z,w4);
    }

    fclose(f);
    return 0;
}
