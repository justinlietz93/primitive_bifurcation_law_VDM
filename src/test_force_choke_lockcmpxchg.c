#define _GNU_SOURCE
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <sched.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <errno.h>
#include <sys/mman.h>

#ifndef SAMPLES
#define SAMPLES 200000
#endif

static inline uint64_t rdtsc_begin(void) {
    unsigned hi, lo;
    __asm__ volatile ("lfence\n\trdtsc" : "=a"(lo), "=d"(hi) :: "memory");
    return ((uint64_t)hi << 32) | lo;
}

static inline uint64_t rdtsc_end(void) {
    unsigned hi, lo;
    __asm__ volatile ("rdtscp\n\tlfence" : "=a"(lo), "=d"(hi) :: "rcx", "memory");
    return ((uint64_t)hi << 32) | lo;
}

static inline void choke_probe(volatile uint8_t *target) {
    // Primitive null-search then atomic choke at shared site.
    // xor eax,eax; bsr eax,eax; mov dl,1; lock cmpxchg [target], dl
    __asm__ volatile (
        "xor %%eax, %%eax\n\t"
        ".byte 0x0f, 0xbd, 0xc0\n\t"   // bsr eax,eax
        "mov $1, %%dl\n\t"
        "lock cmpxchgb %%dl, %0\n\t"
        : "+m"(*target)
        :
        : "rax", "rdx", "cc", "memory");
}

typedef struct {
    int cpu;
    volatile uint8_t *target;
    uint32_t *deltas;
    pthread_barrier_t *barrier;
} worker_args;

static void *worker(void *arg_) {
    worker_args *arg = (worker_args *)arg_;
    cpu_set_t set;
    CPU_ZERO(&set);
    CPU_SET(arg->cpu, &set);
    int aff = pthread_setaffinity_np(pthread_self(), sizeof(set), &set);
    (void)aff;

    pthread_barrier_wait(arg->barrier);
    for (size_t i = 0; i < SAMPLES; ++i) {
        uint64_t t0 = rdtsc_begin();
        choke_probe(arg->target);
        uint64_t t1 = rdtsc_end();
        arg->deltas[i] = (uint32_t)(t1 - t0);
    }
    return NULL;
}

typedef struct {
    double mean;
    uint32_t min;
    uint32_t max;
    uint32_t top_vals[8];
    uint32_t top_counts[8];
    double dom_freq_idx;
    double dom_psd;
} stats_t;

static int cmp_u32(const void *a, const void *b) {
    uint32_t ua = *(const uint32_t *)a, ub = *(const uint32_t *)b;
    return (ua > ub) - (ua < ub);
}

static void summarize(const uint32_t *x, size_t n, stats_t *st) {
    uint64_t sum = 0;
    uint32_t min = UINT32_MAX, max = 0;
    for (size_t i = 0; i < n; ++i) {
        uint32_t v = x[i];
        sum += v;
        if (v < min) min = v;
        if (v > max) max = v;
    }
    st->mean = (double)sum / (double)n;
    st->min = min;
    st->max = max;

    uint32_t *sorted = malloc(n * sizeof(uint32_t));
    memcpy(sorted, x, n * sizeof(uint32_t));
    qsort(sorted, n, sizeof(uint32_t), cmp_u32);

    uint32_t vals[2048];
    uint32_t counts[2048];
    size_t m = 0;
    for (size_t i = 0; i < n;) {
        size_t j = i + 1;
        while (j < n && sorted[j] == sorted[i]) ++j;
        if (m < 2048) {
            vals[m] = sorted[i];
            counts[m] = (uint32_t)(j - i);
            ++m;
        }
        i = j;
    }
    free(sorted);

    for (int k = 0; k < 8; ++k) {
        st->top_vals[k] = 0;
        st->top_counts[k] = 0;
    }
    for (size_t i = 0; i < m; ++i) {
        uint32_t c = counts[i];
        uint32_t v = vals[i];
        for (int k = 0; k < 8; ++k) {
            if (c > st->top_counts[k]) {
                for (int s = 7; s > k; --s) {
                    st->top_counts[s] = st->top_counts[s-1];
                    st->top_vals[s] = st->top_vals[s-1];
                }
                st->top_counts[k] = c;
                st->top_vals[k] = v;
                break;
            }
        }
    }

    // crude DFT on demeaned data for first 4096 frequencies, skipping DC.
    size_t N = n < 32768 ? n : 32768;
    double mean = st->mean;
    double best_p = -1.0;
    size_t best_k = 0;
    size_t kmax = N/2 < 4096 ? N/2 : 4096;
    for (size_t k = 1; k < kmax; ++k) {
        double re = 0.0, im = 0.0;
        for (size_t t = 0; t < N; ++t) {
            double a = -2.0 * M_PI * (double)k * (double)t / (double)N;
            double y = (double)x[t] - mean;
            re += y * cos(a);
            im += y * sin(a);
        }
        double p = re*re + im*im;
        if (p > best_p) {
            best_p = p;
            best_k = k;
        }
    }
    st->dom_freq_idx = (double)best_k / (double)N;
    st->dom_psd = best_p;
}

static void dump_csv(const char *path, const uint32_t *x, size_t n) {
    FILE *f = fopen(path, "w");
    for (size_t i = 0; i < n; ++i) fprintf(f, "%u\n", x[i]);
    fclose(f);
}

static void run_case(const char *label, volatile uint8_t *t0, volatile uint8_t *t1,
                     uint32_t *out0, uint32_t *out1, stats_t *s0, stats_t *s1) {
    pthread_t th0, th1;
    pthread_barrier_t barrier;
    pthread_barrier_init(&barrier, NULL, 3);

    worker_args a0 = {.cpu = 0, .target = t0, .deltas = out0, .barrier = &barrier};
    worker_args a1 = {.cpu = 1, .target = t1, .deltas = out1, .barrier = &barrier};

    pthread_create(&th0, NULL, worker, &a0);
    pthread_create(&th1, NULL, worker, &a1);
    pthread_barrier_wait(&barrier);

    pthread_join(th0, NULL);
    pthread_join(th1, NULL);
    pthread_barrier_destroy(&barrier);

    summarize(out0, SAMPLES, s0);
    summarize(out1, SAMPLES, s1);

    fprintf(stderr, "%s done\n", label);
}

int main(void) {
    volatile uint8_t *page = mmap(NULL, 4096, PROT_READ|PROT_WRITE,
                                  MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
    if (page == MAP_FAILED) {
        perror("mmap");
        return 1;
    }
    volatile uint8_t *shared = page + 64;
    volatile uint8_t *sep0 = page + 128;
    volatile uint8_t *sep1 = page + 192;
    *shared = 0;
    *sep0 = 0;
    *sep1 = 0;

    uint32_t *shared0 = aligned_alloc(64, SAMPLES * sizeof(uint32_t));
    uint32_t *shared1 = aligned_alloc(64, SAMPLES * sizeof(uint32_t));
    uint32_t *sep0d = aligned_alloc(64, SAMPLES * sizeof(uint32_t));
    uint32_t *sep1d = aligned_alloc(64, SAMPLES * sizeof(uint32_t));
    if (!shared0 || !shared1 || !sep0d || !sep1d) {
        fprintf(stderr, "alloc failed\n");
        return 1;
    }

    stats_t sh0, sh1, sp0, sp1;
    run_case("shared", shared, shared, shared0, shared1, &sh0, &sh1);
    *sep0 = 0; *sep1 = 0;
    run_case("separate", sep0, sep1, sep0d, sep1d, &sp0, &sp1);

    FILE *f = fopen("/mnt/data/lockcmpxchg_choke_summary.txt", "w");
    fprintf(f, "LOCK CMPXCHG choke test\n");
    fprintf(f, "shared target final=%u separate finals=%u,%u\n", *shared, *sep0, *sep1);
    fprintf(f, "shared   mean0=%.3f min0=%u max0=%u mean1=%.3f min1=%u max1=%u\n",
            sh0.mean, sh0.min, sh0.max, sh1.mean, sh1.min, sh1.max);
    fprintf(f, "separate mean0=%.3f min0=%u max0=%u mean1=%.3f min1=%u max1=%u\n",
            sp0.mean, sp0.min, sp0.max, sp1.mean, sp1.min, sp1.max);
    fprintf(f, "shared-minus-separate delta mean core0=%.3f core1=%.3f\n",
            sh0.mean - sp0.mean, sh1.mean - sp1.mean);

    fprintf(f, "shared core0 top bins:");
    for (int i = 0; i < 8; ++i) fprintf(f, " %u(%u)", sh0.top_vals[i], sh0.top_counts[i]);
    fprintf(f, "\nshared core1 top bins:");
    for (int i = 0; i < 8; ++i) fprintf(f, " %u(%u)", sh1.top_vals[i], sh1.top_counts[i]);
    fprintf(f, "\nseparate core0 top bins:");
    for (int i = 0; i < 8; ++i) fprintf(f, " %u(%u)", sp0.top_vals[i], sp0.top_counts[i]);
    fprintf(f, "\nseparate core1 top bins:");
    for (int i = 0; i < 8; ++i) fprintf(f, " %u(%u)", sp1.top_vals[i], sp1.top_counts[i]);
    fprintf(f, "\nshared dominant spectrum idx core0=%.9f psd=%.3f core1=%.9f psd=%.3f\n",
            sh0.dom_freq_idx, sh0.dom_psd, sh1.dom_freq_idx, sh1.dom_psd);
    fprintf(f, "separate dominant spectrum idx core0=%.9f psd=%.3f core1=%.9f psd=%.3f\n",
            sp0.dom_freq_idx, sp0.dom_psd, sp1.dom_freq_idx, sp1.dom_psd);
    fclose(f);

    dump_csv("/mnt/data/lockcmpxchg_shared_core0.csv", shared0, SAMPLES);
    dump_csv("/mnt/data/lockcmpxchg_shared_core1.csv", shared1, SAMPLES);
    dump_csv("/mnt/data/lockcmpxchg_separate_core0.csv", sep0d, SAMPLES);
    dump_csv("/mnt/data/lockcmpxchg_separate_core1.csv", sep1d, SAMPLES);

    printf("LOCK CMPXCHG choke test complete\n");
    return 0;
}
