
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
#include <unistd.h>
#include <x86intrin.h>

#ifndef MAP_ANONYMOUS
#define MAP_ANONYMOUS MAP_ANON
#endif

typedef void (*cell_fn_t)(void);

enum { ITERS = 20000, NUM_CELLS = 3 };

typedef struct {
    int cpu;
    cell_fn_t fn;
    uint64_t *deltas;
    uint64_t *starts;
    pthread_barrier_t *barrier;
} worker_args_t;

static void pin_to_cpu(int cpu) {
    cpu_set_t set;
    CPU_ZERO(&set);
    CPU_SET(cpu, &set);
    if (sched_setaffinity(0, sizeof(set), &set) != 0) {
        perror("sched_setaffinity");
    }
}

static size_t cell_size_for_density(int density) {
    return (size_t)(density * 3 + 8);
}

static uint8_t *build_chain_region(int density) {
    size_t cell_size = cell_size_for_density(density);
    size_t code_size = NUM_CELLS * cell_size + 1;
    uint8_t *buf = mmap(NULL, 4096, PROT_READ | PROT_WRITE | PROT_EXEC,
                        MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (buf == MAP_FAILED) {
        perror("mmap");
        exit(1);
    }
    memset(buf, 0x90, 4096);

    for (int i = 0; i < NUM_CELLS; ++i) {
        size_t base = (size_t)i * cell_size;
        size_t p = base;
        for (int d = 0; d < density; ++d) {
            buf[p + 0] = 0x0F; buf[p + 1] = 0xBD; buf[p + 2] = 0xC0;
            p += 3;
        }
        buf[p + 0] = 0xF0;
        buf[p + 1] = 0x0F;
        buf[p + 2] = 0xB0;
        buf[p + 3] = 0x15;
        size_t target = (size_t)((i + 1) % NUM_CELLS) * cell_size;
        int32_t disp = (int32_t)(target - (p + 8));
        memcpy(&buf[p + 4], &disp, sizeof(disp));
    }
    buf[code_size - 1] = 0xC3;
    __builtin___clear_cache((char *)buf, (char *)buf + code_size);
    return buf;
}

static void *worker(void *argp) {
    worker_args_t *arg = (worker_args_t *)argp;
    pin_to_cpu(arg->cpu);
    unsigned aux = 0;
    for (int i = 0; i < ITERS; ++i) {
        pthread_barrier_wait(arg->barrier);
        uint64_t t0 = __rdtscp(&aux);
        asm volatile(
            "xor %%eax, %%eax\n\t"
            "mov $1, %%edx\n\t"
            "call *%0\n\t"
            :
            : "r"(arg->fn)
            : "rax", "rdx", "rcx", "rsi", "rdi", "r8", "r9", "r10", "r11", "cc", "memory");
        uint64_t t1 = __rdtscp(&aux);
        arg->starts[i] = t0;
        arg->deltas[i] = t1 - t0;
    }
    return NULL;
}

static void run_case(cell_fn_t fn0, cell_fn_t fn1,
                     uint64_t *d0, uint64_t *s0,
                     uint64_t *d1, uint64_t *s1) {
    pthread_barrier_t barrier;
    pthread_barrier_init(&barrier, NULL, 2);
    pthread_t th0, th1;
    worker_args_t a0 = {.cpu = 0, .fn = fn0, .deltas = d0, .starts = s0, .barrier = &barrier};
    worker_args_t a1 = {.cpu = 1, .fn = fn1, .deltas = d1, .starts = s1, .barrier = &barrier};
    if (pthread_create(&th0, NULL, worker, &a0) || pthread_create(&th1, NULL, worker, &a1)) {
        fprintf(stderr, "pthread_create failed\n");
        exit(1);
    }
    pthread_join(th0, NULL);
    pthread_join(th1, NULL);
    pthread_barrier_destroy(&barrier);
}

static double mean_u64(const uint64_t *xs) {
    long double s = 0.0L;
    for (int i = 0; i < ITERS; ++i) s += (long double)xs[i];
    return (double)(s / (long double)ITERS);
}

static double mean_i64(const int64_t *xs) {
    long double s = 0.0L;
    for (int i = 0; i < ITERS; ++i) s += (long double)xs[i];
    return (double)(s / (long double)ITERS);
}

static double std_i64(const int64_t *xs, double mean) {
    long double s = 0.0L;
    for (int i = 0; i < ITERS; ++i) {
        long double d = (long double)xs[i] - mean;
        s += d * d;
    }
    return sqrt((double)(s / (long double)ITERS));
}

static double mean_abs_i64(const int64_t *xs) {
    long double s = 0.0L;
    for (int i = 0; i < ITERS; ++i) s += llabs(xs[i]);
    return (double)(s / (long double)ITERS);
}

static void write_u64_csv(const char *path, const uint64_t *xs) {
    FILE *f = fopen(path, "w");
    if (!f) { perror(path); exit(1); }
    for (int i = 0; i < ITERS; ++i) fprintf(f, "%" PRIu64 "\n", xs[i]);
    fclose(f);
}

static void write_i64_csv(const char *path, const int64_t *xs) {
    FILE *f = fopen(path, "w");
    if (!f) { perror(path); exit(1); }
    for (int i = 0; i < ITERS; ++i) fprintf(f, "%" PRId64 "\n", xs[i]);
    fclose(f);
}

int main(void) {
    FILE *sum = fopen("/mnt/data/chiral_sweep_summary.txt", "w");
    if (!sum) { perror("summary"); return 1; }
    fprintf(sum, "density,shared_mean0,shared_mean1,separate_mean0,separate_mean1,shared_over_sep0,shared_over_sep1,sep_over_shared0,sep_over_shared1,shared_offset_mean,shared_offset_abs_mean,shared_offset_std,separate_offset_mean,separate_offset_abs_mean,separate_offset_std\n");

    for (int density = 1; density <= 4; ++density) {
        uint8_t *shared = build_chain_region(density);
        uint8_t *sep0 = build_chain_region(density);
        uint8_t *sep1 = build_chain_region(density);

        uint64_t *sd0 = calloc(ITERS, sizeof(uint64_t));
        uint64_t *ss0 = calloc(ITERS, sizeof(uint64_t));
        uint64_t *sd1 = calloc(ITERS, sizeof(uint64_t));
        uint64_t *ss1 = calloc(ITERS, sizeof(uint64_t));
        uint64_t *dd0 = calloc(ITERS, sizeof(uint64_t));
        uint64_t *ds0 = calloc(ITERS, sizeof(uint64_t));
        uint64_t *dd1 = calloc(ITERS, sizeof(uint64_t));
        uint64_t *ds1 = calloc(ITERS, sizeof(uint64_t));
        int64_t *shared_off = calloc(ITERS, sizeof(int64_t));
        int64_t *sep_off = calloc(ITERS, sizeof(int64_t));

        if (!sd0||!ss0||!sd1||!ss1||!dd0||!ds0||!dd1||!ds1||!shared_off||!sep_off) {
            fprintf(stderr, "calloc failed\n");
            return 1;
        }

        run_case((cell_fn_t)shared, (cell_fn_t)shared, sd0, ss0, sd1, ss1);
        run_case((cell_fn_t)sep0, (cell_fn_t)sep1, dd0, ds0, dd1, ds1);

        for (int i = 0; i < ITERS; ++i) {
            shared_off[i] = (int64_t)ss1[i] - (int64_t)ss0[i];
            sep_off[i] = (int64_t)ds1[i] - (int64_t)ds0[i];
        }

        double msm0 = mean_u64(sd0), msm1 = mean_u64(sd1);
        double mdm0 = mean_u64(dd0), mdm1 = mean_u64(dd1);
        double so_mean = mean_i64(shared_off);
        double so_abs = mean_abs_i64(shared_off);
        double so_std = std_i64(shared_off, so_mean);
        double do_mean = mean_i64(sep_off);
        double do_abs = mean_abs_i64(sep_off);
        double do_std = std_i64(sep_off, do_mean);

        fprintf(sum, "%d,%.9f,%.9f,%.9f,%.9f,%.12f,%.12f,%.12f,%.12f,%.9f,%.9f,%.9f,%.9f,%.9f,%.9f\n",
                density, msm0, msm1, mdm0, mdm1,
                msm0 / mdm0, msm1 / mdm1, mdm0 / msm0, mdm1 / msm1,
                so_mean, so_abs, so_std, do_mean, do_abs, do_std);

        char path[256];
        snprintf(path, sizeof(path), "/mnt/data/chiral_sweep_d%d_shared_core0.csv", density); write_u64_csv(path, sd0);
        snprintf(path, sizeof(path), "/mnt/data/chiral_sweep_d%d_shared_core1.csv", density); write_u64_csv(path, sd1);
        snprintf(path, sizeof(path), "/mnt/data/chiral_sweep_d%d_separate_core0.csv", density); write_u64_csv(path, dd0);
        snprintf(path, sizeof(path), "/mnt/data/chiral_sweep_d%d_separate_core1.csv", density); write_u64_csv(path, dd1);
        snprintf(path, sizeof(path), "/mnt/data/chiral_sweep_d%d_shared_offset.csv", density); write_i64_csv(path, shared_off);
        snprintf(path, sizeof(path), "/mnt/data/chiral_sweep_d%d_separate_offset.csv", density); write_i64_csv(path, sep_off);

        free(sd0); free(ss0); free(sd1); free(ss1);
        free(dd0); free(ds0); free(dd1); free(ds1);
        free(shared_off); free(sep_off);
        munmap(shared, 4096); munmap(sep0, 4096); munmap(sep1, 4096);
    }
    fclose(sum);
    return 0;
}
