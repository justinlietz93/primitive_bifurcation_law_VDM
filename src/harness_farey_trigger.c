#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

typedef struct {
    uint64_t A;
    uint64_t theta_tick;
    uint64_t kappa;
    uint64_t u;
    uint64_t v;
    uint64_t event;
} State;
extern int i_kernel_step(State *s);
int main(int argc, char **argv) {
    uint64_t steps = 512;
    if (argc > 1) steps = strtoull(argv[1], NULL, 10);
    State s = {0,0,0,1,2,0};
    FILE *f = fopen("/mnt/data/kernel_trace_farey_trigger.csv", "w");
    if (!f) return 1;
    fprintf(f, "tick,A,theta_tick,kappa,u,v,r_num,r_den,event,cL_num,cL_den,cR_num,cR_den\n");
    for (uint64_t t=0; t<steps; ++t) {
        i_kernel_step(&s);
        unsigned long long rden = (unsigned long long)(s.u * s.v);
        long long cl_num = (long long)(s.theta_tick * rden - 8ULL);
        long long cr_num = (long long)(s.theta_tick * rden + 8ULL);
        unsigned long long cden = 8ULL * rden;
        fprintf(f, "%" PRIu64 ",%" PRIu64 ",%" PRIu64 ",%" PRIu64 ",%" PRIu64 ",%" PRIu64 ",1,%llu,%" PRIu64 ",%lld,%llu,%lld,%llu\n",
            t, s.A, s.theta_tick, s.kappa, s.u, s.v, rden, s.event, cl_num, cden, cr_num, cden);
    }
    fclose(f);
    return 0;
}
