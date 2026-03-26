#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct {
    uint64_t u;
    uint64_t du;
    uint64_t axis;
} State;

extern uint32_t i_kernel_step(State *s);

int main(void) {
    FILE *f = fopen("/mnt/data/kernel_trace_trigger.csv", "w");
    if (!f) return 1;
    fprintf(f, "tick,u,du,axis,event,x,y,z,w\n");

    State s = {0, (1ULL<<32)/8, 0};
    long long coord[4] = {0,0,0,0};

    for (uint64_t tick=0; tick<128; ++tick) {
        uint32_t event = i_kernel_step(&s);
        uint64_t active = s.axis & 3ULL;
        if (event) {
            coord[active] += 1;
        } else {
            coord[active] += 1;
        }
        fprintf(f, "%llu,%llu,%llu,%llu,%u,%lld,%lld,%lld,%lld\n",
                (unsigned long long)tick,
                (unsigned long long)s.u,
                (unsigned long long)s.du,
                (unsigned long long)s.axis,
                event,
                coord[0], coord[1], coord[2], coord[3]);
    }
    fclose(f);
    return 0;
}
