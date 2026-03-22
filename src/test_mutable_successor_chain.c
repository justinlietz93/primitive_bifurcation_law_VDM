#define _GNU_SOURCE
#include <errno.h>
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

#ifndef MAP_ANONYMOUS
#define MAP_ANONYMOUS MAP_ANON
#endif

typedef void (*cell_fn_t)(void);

enum { ITERS = 50000, NUM_CELLS = 3, MAX_DENSITY = 4 };

typedef struct {
    int cpu;
    cell_fn_t fn;
    uint64_t *deltas;
} worker_args_t;

typedef struct {
    int density;
    int cell_size;
    int code_size;
    int local_opcode_off;
    uint8_t *buf;
} region_t;

static void pin_to_cpu(int cpu) {
    cpu_set_t set;
    CPU_ZERO(&set);
    CPU_SET(cpu, &set);
    if (sched_setaffinity(0, sizeof(set), &set) != 0) perror("sched_setaffinity");
}

static region_t build_region(int density) {
    region_t r = {0};
    r.density = density;
    r.cell_size = density * 3 + 8 + 2; // BSR*d + lock cmpxchg + mutable opcode pair
    r.code_size = NUM_CELLS * r.cell_size + 1; // +ret
    r.local_opcode_off = density * 3 + 8;
    r.buf = mmap(NULL, 4096, PROT_READ | PROT_WRITE | PROT_EXEC,
                 MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (r.buf == MAP_FAILED) {
        perror("mmap");
        exit(1);
    }
    memset(r.buf, 0x90, 4096);
    for (int i = 0; i < NUM_CELLS; ++i) {
        int base = i * r.cell_size;
        int p = base;
        for (int k = 0; k < density; ++k) {
            r.buf[p++] = 0x0F; r.buf[p++] = 0xBD; r.buf[p++] = 0xC0; // bsr eax,eax
        }
        r.buf[p++] = 0xF0; r.buf[p++] = 0x0F; r.buf[p++] = 0xB0; r.buf[p++] = 0x15; // lock cmpxchg byte ptr [rip+disp32], dl
        int next = (i + 1) % NUM_CELLS;
        int target = next * r.cell_size + r.local_opcode_off;
        int next_ip = p + 4; // after disp32
        int32_t disp = (int32_t)(target - next_ip);
        memcpy(&r.buf[p], &disp, 4); p += 4;
        r.buf[p++] = 0x00; r.buf[p++] = 0xC0; // mutable opcode: 00 C0 = add al,al ; 01 C0 = add eax,eax
    }
    r.buf[NUM_CELLS * r.cell_size] = 0xC3; // ret
    __builtin___clear_cache((char *)r.buf, (char *)r.buf + r.code_size);
    return r;
}

static void *worker(void *argp) {
    worker_args_t *arg = (worker_args_t *)argp;
    pin_to_cpu(arg->cpu);
    unsigned aux = 0;
    for (int i = 0; i < ITERS; ++i) {
        uint64_t t0 = __rdtscp(&aux);
        asm volatile(
            "xor %%eax, %%eax\n\t"
            "mov $1, %%edx\n\t"
            "call *%0\n\t"
            :
            : "r"(arg->fn)
            : "rax","rdx","rcx","rsi","rdi","r8","r9","r10","r11","cc","memory");
        uint64_t t1 = __rdtscp(&aux);
        arg->deltas[i] = t1 - t0;
    }
    return NULL;
}

static void run_case(cell_fn_t fn0, cell_fn_t fn1, uint64_t *out0, uint64_t *out1) {
    pthread_t th0, th1;
    worker_args_t a0 = {.cpu = 0, .fn = fn0, .deltas = out0};
    worker_args_t a1 = {.cpu = 1, .fn = fn1, .deltas = out1};
    if (pthread_create(&th0, NULL, worker, &a0) || pthread_create(&th1, NULL, worker, &a1)) {
        fprintf(stderr, "pthread_create failed\n");
        exit(1);
    }
    pthread_join(th0, NULL);
    pthread_join(th1, NULL);
}

static double mean_u64(const uint64_t *xs) {
    long double s = 0.0L;
    for (int i = 0; i < ITERS; ++i) s += (long double)xs[i];
    return (double)(s / (long double)ITERS);
}

static void dump_region(FILE *f, const char *label, const region_t *r) {
    fprintf(f, "%s density=%d cell_size=%d local_opcode_off=%d\n", label, r->density, r->cell_size, r->local_opcode_off);
    for (int i = 0; i < NUM_CELLS; ++i) {
        int off = i * r->cell_size + r->local_opcode_off;
        fprintf(f, "  cell%d opcode=%02x %02x\n", i, r->buf[off], r->buf[off+1]);
    }
}

static void write_csv(const char *path, const uint64_t *xs) {
    FILE *f = fopen(path, "w");
    if (!f) { perror(path); exit(1);} 
    for (int i = 0; i < ITERS; ++i) fprintf(f, "%" PRIu64 "\n", xs[i]);
    fclose(f);
}

int main(void) {
    FILE *sum = fopen("/mnt/data/mutable_successor_summary.txt", "w");
    if (!sum) { perror("summary"); return 1; }
    fprintf(sum, "Experiment: corrected executable successor-mutation chain\n");
    fprintf(sum, "Goal: distinguish real executable class admission from mere lock-choke.\n");
    fprintf(sum, "Mechanism: each cell does BSR^density, then LOCK CMPXCHG into the next cell's mutable opcode byte.\n");
    fprintf(sum, "Mutable opcode pair: 00 C0 (add al,al) <-> 01 C0 (add eax,eax).\n");
    fprintf(sum, "A successful cmpxchg turns the next cell from 8-bit local execution to 32-bit local execution.\n\n");

    uint64_t *shared0 = calloc(ITERS, sizeof(uint64_t));
    uint64_t *shared1 = calloc(ITERS, sizeof(uint64_t));
    uint64_t *sep0 = calloc(ITERS, sizeof(uint64_t));
    uint64_t *sep1 = calloc(ITERS, sizeof(uint64_t));
    if (!shared0 || !shared1 || !sep0 || !sep1) { fprintf(stderr, "calloc failed\n"); return 1; }

    for (int density = 1; density <= 3; ++density) {
        region_t shared = build_region(density);
        region_t sepA = build_region(density);
        region_t sepB = build_region(density);

        run_case((cell_fn_t)shared.buf, (cell_fn_t)shared.buf, shared0, shared1);
        run_case((cell_fn_t)sepA.buf, (cell_fn_t)sepB.buf, sep0, sep1);

        double m_sh0 = mean_u64(shared0), m_sh1 = mean_u64(shared1);
        double m_sp0 = mean_u64(sep0), m_sp1 = mean_u64(sep1);
        fprintf(sum, "density=%d\n", density);
        fprintf(sum, "  shared_mean_core0=%.9f\n", m_sh0);
        fprintf(sum, "  shared_mean_core1=%.9f\n", m_sh1);
        fprintf(sum, "  separate_mean_core0=%.9f\n", m_sp0);
        fprintf(sum, "  separate_mean_core1=%.9f\n", m_sp1);
        fprintf(sum, "  ratio_shared_separate_core0=%.12f\n", m_sh0 / m_sp0);
        fprintf(sum, "  ratio_shared_separate_core1=%.12f\n", m_sh1 / m_sp1);
        dump_region(sum, "  shared_region_final", &shared);
        dump_region(sum, "  separate_regionA_final", &sepA);
        dump_region(sum, "  separate_regionB_final", &sepB);
        fprintf(sum, "\n");

        char path[256];
        snprintf(path, sizeof(path), "/mnt/data/mutable_d%d_shared_core0.csv", density);
        write_csv(path, shared0);
        snprintf(path, sizeof(path), "/mnt/data/mutable_d%d_shared_core1.csv", density);
        write_csv(path, shared1);
        snprintf(path, sizeof(path), "/mnt/data/mutable_d%d_sep_core0.csv", density);
        write_csv(path, sep0);
        snprintf(path, sizeof(path), "/mnt/data/mutable_d%d_sep_core1.csv", density);
        write_csv(path, sep1);

        munmap(shared.buf, 4096);
        munmap(sepA.buf, 4096);
        munmap(sepB.buf, 4096);
    }
    fclose(sum);
    return 0;
}
