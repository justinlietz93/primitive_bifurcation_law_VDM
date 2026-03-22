#define _GNU_SOURCE
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <pthread.h>
#include <sched.h>
#include <stdatomic.h>
#include <x86intrin.h>
#include <unistd.h>

#ifndef SAMPLES
#define SAMPLES 200000
#endif

typedef struct {
    int cpu;
    uint32_t *deltas;
    atomic_int *go;
} worker_arg_t;

static inline uint32_t run_probe(void) {
    uint32_t aux0, aux1;
    uint64_t t0 = __rdtscp(&aux0);
    __asm__ volatile(
        "xor %%eax, %%eax\n\t"
        "bsr %%eax, %%eax\n\t"
        "setz %%bl\n\t"
        : : : "rax", "rbx", "cc");
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
        arg->deltas[i] = run_probe();
    }
    return NULL;
}

int main(void) {
    uint32_t *a = aligned_alloc(64, SAMPLES * sizeof(uint32_t));
    uint32_t *b = aligned_alloc(64, SAMPLES * sizeof(uint32_t));
    if (!a || !b) {
        perror("alloc");
        return 1;
    }

    atomic_int go = 0;
    pthread_t ta, tb;
    worker_arg_t aa = {.cpu = 0, .deltas = a, .go = &go};
    worker_arg_t bb = {.cpu = 1, .deltas = b, .go = &go};

    if (pthread_create(&ta, NULL, worker, &aa) != 0) { perror("pthread_create a"); return 1; }
    if (pthread_create(&tb, NULL, worker, &bb) != 0) { perror("pthread_create b"); return 1; }

    usleep(50000);
    atomic_store_explicit(&go, 1, memory_order_release);

    pthread_join(ta, NULL);
    pthread_join(tb, NULL);

    FILE *fa = fopen("/mnt/data/dualcore_bsr_core0.csv", "w");
    FILE *fb = fopen("/mnt/data/dualcore_bsr_core1.csv", "w");
    if (!fa || !fb) { perror("fopen"); return 1; }
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

    printf("core0 mean=%.3f min=%u max=%u\n", (double)suma / SAMPLES, mina, maxa);
    printf("core1 mean=%.3f min=%u max=%u\n", (double)sumb / SAMPLES, minb, maxb);

    free(a); free(b);
    return 0;
}
