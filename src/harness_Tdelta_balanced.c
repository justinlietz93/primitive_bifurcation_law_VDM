#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct {
    uint64_t A;
    uint64_t q;
    uint64_t u;
    uint64_t v;
    uint64_t floor_den;
    uint64_t event;
    uint64_t carried_den_at_transfer;
} State;

extern int i_kernel_step(State *s);

int main(void) {
    FILE *f = fopen("/mnt/data/kernel_trace_Tdelta_balanced.csv", "w");
    if (!f) return 1;
    fprintf(f, "tick,A,q,u,v,event,r_den,width_over_pi,x,y,z\n");
    State s = {0,0,1,1,4096,0,1};
    for (uint64_t tick = 0; tick < 2048; ++tick) {
        int ev = i_kernel_step(&s);
        uint64_t rden = s.u * s.v;
        double width_over_pi = 2.0 / (double)rden;
        long long x = (long long)s.q;
        long long y = (long long)s.A;
        long long z = (long long)((s.u + s.v) & 8191ULL);
        fprintf(f, "%llu,%llu,%llu,%llu,%llu,%d,%llu,%.17g,%lld,%lld,%lld\n",
            (unsigned long long)tick,
            (unsigned long long)s.A,
            (unsigned long long)s.q,
            (unsigned long long)s.u,
            (unsigned long long)s.v,
            ev,
            (unsigned long long)rden,
            width_over_pi,
            x,y,z);
    }
    fclose(f);
    return 0;
}
