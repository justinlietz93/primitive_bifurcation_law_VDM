#define _GNU_SOURCE
#include <errno.h>
#include <inttypes.h>
#include <math.h>
#include <pthread.h>
#include <sched.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <x86intrin.h>

#ifndef MAP_ANONYMOUS
#define MAP_ANONYMOUS MAP_ANON
#endif

typedef void (*cell_fn_t)(void);

enum { ITERS = 100000 };
enum { CELL_SIZE = 17, NUM_CELLS = 3, CODE_SIZE = NUM_CELLS * CELL_SIZE + 1 };

typedef struct {
    int cpu;
    cell_fn_t fn;
    uint64_t *deltas;
} worker_args_t;

static void pin_to_cpu(int cpu) {
    cpu_set_t set;
    CPU_ZERO(&set);
    CPU_SET(cpu, &set);
    int rc = sched_setaffinity(0, sizeof(set), &set);
    if (rc != 0) {
        perror("sched_setaffinity");
    }
}

static uint8_t *build_chain_region(void) {
    uint8_t *buf = mmap(NULL, 4096, PROT_READ | PROT_WRITE | PROT_EXEC,
                        MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (buf == MAP_FAILED) {
        perror("mmap");
        exit(1);
    }
    memset(buf, 0x90, 4096);

    for (int i = 0; i < NUM_CELLS; ++i) {
        size_t base = (size_t)i * CELL_SIZE;
        // 3x BSR EAX,EAX
        buf[base + 0] = 0x0F; buf[base + 1] = 0xBD; buf[base + 2] = 0xC0;
        buf[base + 3] = 0x0F; buf[base + 4] = 0xBD; buf[base + 5] = 0xC0;
        buf[base + 6] = 0x0F; buf[base + 7] = 0xBD; buf[base + 8] = 0xC0;
        // LOCK CMPXCHG byte ptr [rip+disp32], dl
        buf[base + 9]  = 0xF0;
        buf[base + 10] = 0x0F;
        buf[base + 11] = 0xB0;
        buf[base + 12] = 0x15;

        size_t target = (size_t)((i + 1) % NUM_CELLS) * CELL_SIZE; // next cell start, wrapping
        int32_t disp = (int32_t)(target - (base + CELL_SIZE));
        memcpy(&buf[base + 13], &disp, sizeof(disp));
    }
    buf[NUM_CELLS * CELL_SIZE] = 0xC3; // ret

    __builtin___clear_cache((char *)buf, (char *)buf + CODE_SIZE);
    return buf;
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
            : "rax", "rdx", "rcx", "rsi", "rdi", "r8", "r9", "r10", "r11", "cc", "memory");
        uint64_t t1 = __rdtscp(&aux);
        arg->deltas[i] = t1 - t0;
    }
    return NULL;
}

static void run_case(const char *label, cell_fn_t fn0, cell_fn_t fn1,
                     uint64_t *out0, uint64_t *out1) {
    pthread_t th0, th1;
    worker_args_t a0 = {.cpu = 0, .fn = fn0, .deltas = out0};
    worker_args_t a1 = {.cpu = 1, .fn = fn1, .deltas = out1};

    int rc0 = pthread_create(&th0, NULL, worker, &a0);
    int rc1 = pthread_create(&th1, NULL, worker, &a1);
    if (rc0 || rc1) {
        fprintf(stderr, "pthread_create failed for %s\n", label);
        exit(1);
    }
    pthread_join(th0, NULL);
    pthread_join(th1, NULL);
}

static void write_csv(const char *path, const uint64_t *xs) {
    FILE *f = fopen(path, "w");
    if (!f) { perror(path); exit(1); }
    for (int i = 0; i < ITERS; ++i) fprintf(f, "%" PRIu64 "\n", xs[i]);
    fclose(f);
}

static double mean_u64(const uint64_t *xs) {
    long double s = 0.0L;
    for (int i = 0; i < ITERS; ++i) s += (long double)xs[i];
    return (double)(s / (long double)ITERS);
}

int main(void) {
    uint8_t *shared = build_chain_region();
    uint8_t *sep0 = build_chain_region();
    uint8_t *sep1 = build_chain_region();

    uint64_t *shared0 = calloc(ITERS, sizeof(uint64_t));
    uint64_t *shared1 = calloc(ITERS, sizeof(uint64_t));
    uint64_t *sep0d = calloc(ITERS, sizeof(uint64_t));
    uint64_t *sep1d = calloc(ITERS, sizeof(uint64_t));
    if (!shared0 || !shared1 || !sep0d || !sep1d) {
        fprintf(stderr, "calloc failed\n");
        return 1;
    }

    run_case("shared", (cell_fn_t)shared, (cell_fn_t)shared, shared0, shared1);
    run_case("separate", (cell_fn_t)sep0, (cell_fn_t)sep1, sep0d, sep1d);

    double m_shared0 = mean_u64(shared0);
    double m_shared1 = mean_u64(shared1);
    double m_sep0 = mean_u64(sep0d);
    double m_sep1 = mean_u64(sep1d);

    printf("shared   mean0=%.6f mean1=%.6f\n", m_shared0, m_shared1);
    printf("separate mean0=%.6f mean1=%.6f\n", m_sep0, m_sep1);
    printf("ratio sep/shared core0=%.9f\n", m_sep0 / m_shared0);
    printf("ratio sep/shared core1=%.9f\n", m_sep1 / m_shared1);

    write_csv("/mnt/data/hpec_shared_core0.csv", shared0);
    write_csv("/mnt/data/hpec_shared_core1.csv", shared1);
    write_csv("/mnt/data/hpec_separate_core0.csv", sep0d);
    write_csv("/mnt/data/hpec_separate_core1.csv", sep1d);

    FILE *sum = fopen("/mnt/data/hpec_summary.txt", "w");
    if (!sum) { perror("summary"); return 1; }
    fprintf(sum, "shared   mean0=%.9f mean1=%.9f\n", m_shared0, m_shared1);
    fprintf(sum, "separate mean0=%.9f mean1=%.9f\n", m_sep0, m_sep1);
    fprintf(sum, "ratio_sep_shared_core0=%.12f\n", m_sep0 / m_shared0);
    fprintf(sum, "ratio_sep_shared_core1=%.12f\n", m_sep1 / m_shared1);
    fclose(sum);

    return 0;
}
