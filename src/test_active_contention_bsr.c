#define _GNU_SOURCE
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <pthread.h>
#include <sched.h>
#include <stdatomic.h>
#include <x86intrin.h>
#include <unistd.h>
#include <string.h>

#ifndef SAMPLES
#define SAMPLES 50000
#endif

typedef struct {
    int cpu;
    volatile uint8_t *target;
    uint32_t *deltas;
    pthread_barrier_t *bar;
    atomic_int *go;
} worker_arg_t;

static inline uint32_t run_probe(volatile uint8_t *target) {
    uint32_t aux0, aux1;
    uint64_t t0 = __rdtscp(&aux0);
    __asm__ volatile(
        "xor %%eax, %%eax\n\t"
        "bsr %%eax, %%eax\n\t"
        "setz (%%rdi)\n\t"
        :
        : "D"(target)
        : "rax", "cc", "memory");
    uint64_t t1 = __rdtscp(&aux1);
    return (uint32_t)(t1 - t0);
}

void *worker(void *vp) {
    worker_arg_t *arg = (worker_arg_t *)vp;
    cpu_set_t set;
    CPU_ZERO(&set);
    CPU_SET(arg->cpu, &set);
    if (pthread_setaffinity_np(pthread_self(), sizeof(set), &set) != 0) {
        perror("pthread_setaffinity_np");
    }

    while (atomic_load_explicit(arg->go, memory_order_acquire) == 0) {
        __asm__ volatile("pause");
    }

    for (int i = 0; i < SAMPLES; ++i) {
        pthread_barrier_wait(arg->bar);
        arg->deltas[i] = run_probe(arg->target);
        pthread_barrier_wait(arg->bar);
    }
    return NULL;
}

static void run_case(const char *label, volatile uint8_t *target_a, volatile uint8_t *target_b,
                     const char *csv_a, const char *csv_b, const char *summary_path) {
    uint32_t *a = aligned_alloc(64, SAMPLES * sizeof(uint32_t));
    uint32_t *b = aligned_alloc(64, SAMPLES * sizeof(uint32_t));
    if (!a || !b) {
        perror("aligned_alloc");
        exit(1);
    }
    pthread_barrier_t bar;
    pthread_barrier_init(&bar, NULL, 2);
    atomic_int go = 0;

    pthread_t ta, tb;
    worker_arg_t aa = {.cpu = 0, .target = target_a, .deltas = a, .bar = &bar, .go = &go};
    worker_arg_t bb = {.cpu = 1, .target = target_b, .deltas = b, .bar = &bar, .go = &go};

    if (pthread_create(&ta, NULL, worker, &aa) != 0) { perror("pthread_create a"); exit(1); }
    if (pthread_create(&tb, NULL, worker, &bb) != 0) { perror("pthread_create b"); exit(1); }

    usleep(50000);
    atomic_store_explicit(&go, 1, memory_order_release);

    pthread_join(ta, NULL);
    pthread_join(tb, NULL);

    FILE *fa = fopen(csv_a, "w");
    FILE *fb = fopen(csv_b, "w");
    if (!fa || !fb) { perror("fopen csv"); exit(1); }
    fprintf(fa, "idx,delta\n");
    fprintf(fb, "idx,delta\n");
    for (int i = 0; i < SAMPLES; ++i) {
        fprintf(fa, "%d,%u\n", i, a[i]);
        fprintf(fb, "%d,%u\n", i, b[i]);
    }
    fclose(fa);
    fclose(fb);

    uint64_t suma = 0, sumb = 0;
    uint32_t mina = UINT32_MAX, minb = UINT32_MAX, maxa = 0, maxb = 0;
    for (int i = 0; i < SAMPLES; ++i) {
        uint32_t da = a[i], db = b[i];
        suma += da; sumb += db;
        if (da < mina) mina = da; if (da > maxa) maxa = da;
        if (db < minb) minb = db; if (db > maxb) maxb = db;
    }

    FILE *fs = fopen(summary_path, strcmp(label, "shared") == 0 ? "w" : "a");
    if (!fs) { perror("fopen summary"); exit(1); }
    fprintf(fs, "%s mean0=%.3f min0=%u max0=%u mean1=%.3f min1=%u max1=%u\n",
            label, (double)suma/SAMPLES, mina, maxa, (double)sumb/SAMPLES, minb, maxb);
    fclose(fs);

    pthread_barrier_destroy(&bar);
    free(a); free(b);
}

int main(void) {
    long ncpu = sysconf(_SC_NPROCESSORS_ONLN);
    printf("online_cpus=%ld\n", ncpu);
    if (ncpu < 2) {
        fprintf(stderr, "need at least 2 CPUs\n");
        return 1;
    }

    static volatile uint8_t shared_target = 0;
    static volatile uint8_t sep_target_a = 0;
    static volatile uint8_t sep_target_b = 0;

    run_case("shared", &shared_target, &shared_target,
             "/mnt/data/active_contention_shared_core0.csv",
             "/mnt/data/active_contention_shared_core1.csv",
             "/mnt/data/active_contention_summary.txt");
    run_case("separate", &sep_target_a, &sep_target_b,
             "/mnt/data/active_contention_separate_core0.csv",
             "/mnt/data/active_contention_separate_core1.csv",
             "/mnt/data/active_contention_summary.txt");

    printf("shared_target=%u sep_target_a=%u sep_target_b=%u\n",
           shared_target, sep_target_a, sep_target_b);
    return 0;
}
