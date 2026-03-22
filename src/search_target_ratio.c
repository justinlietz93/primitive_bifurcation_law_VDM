#define _GNU_SOURCE
#include <inttypes.h>
#include <pthread.h>
#include <sched.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <x86intrin.h>
#include <math.h>

#ifndef MAP_ANONYMOUS
#define MAP_ANONYMOUS MAP_ANON
#endif

typedef void (*cell_fn_t)(void);

enum { ITERS = 30000, NUM_CELLS = 3, MAX_CELL_SIZE = 32 };

typedef struct { int cpu; cell_fn_t fn; uint64_t *deltas; } worker_args_t;

typedef struct {
    int d[NUM_CELLS];
    double m_shared0, m_shared1, m_sep0, m_sep1;
    double ratio0, ratio1; // shared/separate
    double score;
} result_t;

static void pin_to_cpu(int cpu) {
    cpu_set_t set; CPU_ZERO(&set); CPU_SET(cpu, &set);
    if (sched_setaffinity(0, sizeof(set), &set) != 0) perror("sched_setaffinity");
}

static size_t cell_size_for_density(int dens) {
    return (size_t)(3*dens + 8); // dens*BSR + lockcmpxchg rip32
}

static size_t total_code_size(const int d[NUM_CELLS]) {
    size_t total = 1; for (int i=0;i<NUM_CELLS;i++) total += cell_size_for_density(d[i]); return total;
}

static uint8_t *build_chain_region(const int d[NUM_CELLS]) {
    uint8_t *buf = mmap(NULL, 4096, PROT_READ|PROT_WRITE|PROT_EXEC, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
    if (buf == MAP_FAILED) { perror("mmap"); exit(1); }
    memset(buf, 0x90, 4096);
    size_t bases[NUM_CELLS];
    size_t off = 0;
    for (int i=0;i<NUM_CELLS;i++) { bases[i] = off; off += cell_size_for_density(d[i]); }
    for (int i=0;i<NUM_CELLS;i++) {
        size_t base = bases[i];
        size_t p = base;
        for (int k=0;k<d[i];k++) { buf[p++] = 0x0F; buf[p++] = 0xBD; buf[p++] = 0xC0; }
        buf[p++] = 0xF0; buf[p++] = 0x0F; buf[p++] = 0xB0; buf[p++] = 0x15;
        size_t target = bases[(i+1)%NUM_CELLS];
        int32_t disp = (int32_t)(target - (p + 4));
        memcpy(&buf[p], &disp, 4); p += 4;
    }
    buf[off] = 0xC3;
    __builtin___clear_cache((char*)buf, (char*)buf + off + 1);
    return buf;
}

static void *worker(void *argp) {
    worker_args_t *arg = (worker_args_t*)argp; pin_to_cpu(arg->cpu); unsigned aux = 0;
    for (int i=0;i<ITERS;i++) {
        uint64_t t0 = __rdtscp(&aux);
        asm volatile(
            "xor %%eax, %%eax\n\t"
            "mov $1, %%edx\n\t"
            "call *%0\n\t"
            : : "r"(arg->fn)
            : "rax","rdx","rcx","rsi","rdi","r8","r9","r10","r11","cc","memory");
        uint64_t t1 = __rdtscp(&aux);
        arg->deltas[i] = t1 - t0;
    }
    return NULL;
}

static void run_case(cell_fn_t fn0, cell_fn_t fn1, uint64_t *out0, uint64_t *out1) {
    pthread_t th0, th1; worker_args_t a0={.cpu=0,.fn=fn0,.deltas=out0}, a1={.cpu=1,.fn=fn1,.deltas=out1};
    if (pthread_create(&th0,NULL,worker,&a0) || pthread_create(&th1,NULL,worker,&a1)) { fprintf(stderr,"pthread_create failed\n"); exit(1);} 
    pthread_join(th0,NULL); pthread_join(th1,NULL);
}

static double mean_u64(const uint64_t *xs) { long double s=0; for (int i=0;i<ITERS;i++) s += (long double)xs[i]; return (double)(s/(long double)ITERS); }

int main(void) {
    FILE *f = fopen("/mnt/data/search_target_ratio_results.csv", "w");
    if (!f) { perror("open csv"); return 1; }
    fprintf(f, "d0,d1,d2,shared0,shared1,sep0,sep1,ratio0,ratio1,score\n");
    result_t best = {.score = 1e300};
    for (int d0=1; d0<=4; d0++) for (int d1=1; d1<=4; d1++) for (int d2=1; d2<=4; d2++) {
        int d[3] = {d0,d1,d2};
        uint8_t *shared = build_chain_region(d);
        uint8_t *sep0 = build_chain_region(d);
        uint8_t *sep1 = build_chain_region(d);
        uint64_t *sh0 = calloc(ITERS,sizeof(uint64_t)), *sh1 = calloc(ITERS,sizeof(uint64_t));
        uint64_t *sp0 = calloc(ITERS,sizeof(uint64_t)), *sp1 = calloc(ITERS,sizeof(uint64_t));
        if (!sh0||!sh1||!sp0||!sp1) { fprintf(stderr,"calloc failed\n"); return 1; }
        run_case((cell_fn_t)shared,(cell_fn_t)shared,sh0,sh1);
        run_case((cell_fn_t)sep0,(cell_fn_t)sep1,sp0,sp1);
        double msh0=mean_u64(sh0), msh1=mean_u64(sh1), msp0=mean_u64(sp0), msp1=mean_u64(sp1);
        double r0 = msh0/msp0, r1 = msh1/msp1;
        double avg = 0.5*(r0+r1); double asym = fabs(r0-r1); double score = fabs(avg-2.15) + 0.5*asym;
        fprintf(f, "%d,%d,%d,%.6f,%.6f,%.6f,%.6f,%.9f,%.9f,%.9f\n", d0,d1,d2,msh0,msh1,msp0,msp1,r0,r1,score);
        if (score < best.score) {
            best.d[0]=d0; best.d[1]=d1; best.d[2]=d2; best.m_shared0=msh0; best.m_shared1=msh1; best.m_sep0=msp0; best.m_sep1=msp1; best.ratio0=r0; best.ratio1=r1; best.score=score;
        }
        munmap(shared,4096); munmap(sep0,4096); munmap(sep1,4096);
        free(sh0); free(sh1); free(sp0); free(sp1);
    }
    fclose(f);
    FILE *bestf = fopen("/mnt/data/search_target_ratio_best.txt", "w");
    fprintf(bestf, "best pattern=%d,%d,%d\nshared0=%.6f shared1=%.6f\nsep0=%.6f sep1=%.6f\nratio0=%.12f ratio1=%.12f\nscore=%.12f\n", best.d[0],best.d[1],best.d[2],best.m_shared0,best.m_shared1,best.m_sep0,best.m_sep1,best.ratio0,best.ratio1,best.score);
    fclose(bestf);
    return 0;
}
