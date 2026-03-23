#define _GNU_SOURCE
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <pthread.h>
#include <sched.h>
#include <x86intrin.h>

typedef void (*cell_fn_t)(void);

typedef struct {
    uint8_t *base;
    size_t cell_size;
    size_t shadow_off;
    size_t zero_off;
    size_t mut_off;
    uint8_t *shadows;
    cell_fn_t cells[3];
} Chain;

typedef struct {
    Chain *chain;
    int cpu;
    int passes;
    uint64_t *deltas;
} WorkerArgs;

static void emit8(uint8_t **p, uint8_t v) { *(*p)++ = v; }
static void emit32(uint8_t **p, int32_t v) { memcpy(*p, &v, 4); *p += 4; }
static inline uint64_t rdtsc64(void) { return __rdtsc(); }

static void dump_state(FILE *f, const char *label, Chain *c) {
    fprintf(f, "%s mut=%02x %02x %02x shadow=%02x %02x %02x\n",
            label,
            c->base[c->mut_off + 0 * c->cell_size],
            c->base[c->mut_off + 1 * c->cell_size],
            c->base[c->mut_off + 2 * c->cell_size],
            c->shadows[0], c->shadows[1], c->shadows[2]);
}

static Chain *build_chain(void) {
    const size_t cell_size = 36;
    const size_t total = 3 * cell_size + 3 + 4;
    uint8_t *mem = mmap(NULL, total, PROT_READ | PROT_WRITE | PROT_EXEC,
                        MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (mem == MAP_FAILED) return NULL;
    memset(mem, 0, total);

    Chain *c = calloc(1, sizeof(Chain));
    c->base = mem;
    c->cell_size = cell_size;
    c->shadow_off = 3 * cell_size;
    c->zero_off = c->shadow_off + 3;
    c->shadows = mem + c->shadow_off;
    memset(mem + c->zero_off, 0, 4);

    for (int i = 0; i < 3; ++i) {
        uint8_t *cell = mem + i * cell_size;
        uint8_t *p = cell;
        int next = (i + 1) % 3;

        // bsr eax, dword ptr [rip+zero]  ; explicit null host every time
        emit8(&p, 0x0F); emit8(&p, 0xBD); emit8(&p, 0x05);
        int32_t zero_disp = (int32_t)((mem + c->zero_off) - (p + 4));
        emit32(&p, zero_disp);
        // setz dl
        emit8(&p, 0x0F); emit8(&p, 0x94); emit8(&p, 0xC2);

        // J limb: tentative successor pressure on next shadow
        emit8(&p, 0xF0); emit8(&p, 0x30); emit8(&p, 0x15);
        int32_t shadow_disp = (int32_t)((mem + c->shadow_off + next) - (p + 4));
        emit32(&p, shadow_disp);

        // M limb: only commit when repeated pressure has brought shadow back to zero
        emit8(&p, 0x80); emit8(&p, 0x3D);
        shadow_disp = (int32_t)((mem + c->shadow_off + next) - (p + 4));
        emit32(&p, shadow_disp);
        emit8(&p, 0x00);

        emit8(&p, 0x75); emit8(&p, 0x07); // jne skip_commit

        // commit next cell low opcode byte on persistence only
        emit8(&p, 0xF0); emit8(&p, 0x30); emit8(&p, 0x15);
        int32_t mut_disp = (int32_t)((mem + next * cell_size + 33) - (p + 4));
        emit32(&p, mut_disp);

        // mutable executed pair: 00 C0 <-> 01 C0
        size_t mut_here = (size_t)(p - cell);
        if (i == 0) c->mut_off = mut_here;
        emit8(&p, 0x00);
        emit8(&p, 0xC0);
        emit8(&p, 0xC3); // ret

        if ((size_t)(p - cell) != cell_size) {
            fprintf(stderr, "cell size mismatch: %zu\n", (size_t)(p - cell));
            munmap(mem, total);
            free(c);
            return NULL;
        }
        c->cells[i] = (cell_fn_t)cell;
    }
    return c;
}

static void free_chain(Chain *c) {
    if (!c) return;
    munmap(c->base, 3 * c->cell_size + 3 + 4);
    free(c);
}

static inline void run_pass(Chain *c) {
    c->cells[0]();
    c->cells[1]();
    c->cells[2]();
}

static void *worker_main(void *arg_) {
    WorkerArgs *arg = (WorkerArgs *)arg_;
    cpu_set_t set;
    CPU_ZERO(&set);
    CPU_SET(arg->cpu, &set);
    pthread_setaffinity_np(pthread_self(), sizeof(set), &set);

    for (int i = 0; i < arg->passes; ++i) {
        uint64_t t0 = rdtsc64();
        run_pass(arg->chain);
        uint64_t t1 = rdtsc64();
        arg->deltas[i] = t1 - t0;
    }
    return NULL;
}

static double mean_u64(const uint64_t *x, int n) {
    long double s = 0.0;
    for (int i = 0; i < n; ++i) s += (long double)x[i];
    return (double)(s / (long double)n);
}

int main(void) {
    FILE *log = fopen("/mnt/data/test_jm_persistence_orbit.log", "w");
    if (!log) return 1;

    Chain *single = build_chain();
    if (!single) {
        fprintf(log, "build_failed\n");
        fclose(log);
        return 1;
    }
    dump_state(log, "initial", single);
    for (int p = 1; p <= 8; ++p) {
        run_pass(single);
        char label[32];
        snprintf(label, sizeof(label), "pass_%d", p);
        dump_state(log, label, single);
    }
    fclose(log);

    const int passes = 20000;
    uint64_t *shared0 = calloc((size_t)passes, sizeof(uint64_t));
    uint64_t *shared1 = calloc((size_t)passes, sizeof(uint64_t));
    uint64_t *sep0 = calloc((size_t)passes, sizeof(uint64_t));
    uint64_t *sep1 = calloc((size_t)passes, sizeof(uint64_t));
    Chain *shared_chain = build_chain();
    Chain *sep_chain0 = build_chain();
    Chain *sep_chain1 = build_chain();
    if (!shared0 || !shared1 || !sep0 || !sep1 || !shared_chain || !sep_chain0 || !sep_chain1) return 1;

    WorkerArgs a0 = {.chain = shared_chain, .cpu = 0, .passes = passes, .deltas = shared0};
    WorkerArgs a1 = {.chain = shared_chain, .cpu = 1, .passes = passes, .deltas = shared1};
    pthread_t t0, t1;
    pthread_create(&t0, NULL, worker_main, &a0);
    pthread_create(&t1, NULL, worker_main, &a1);
    pthread_join(t0, NULL);
    pthread_join(t1, NULL);

    WorkerArgs b0 = {.chain = sep_chain0, .cpu = 0, .passes = passes, .deltas = sep0};
    WorkerArgs b1 = {.chain = sep_chain1, .cpu = 1, .passes = passes, .deltas = sep1};
    pthread_create(&t0, NULL, worker_main, &b0);
    pthread_create(&t1, NULL, worker_main, &b1);
    pthread_join(t0, NULL);
    pthread_join(t1, NULL);

    FILE *sum = fopen("/mnt/data/jm_persistence_orbit_summary.txt", "w");
    fprintf(sum, "single-thread phase trace follows test_jm_persistence_orbit.log\n");
    fprintf(sum, "shared mean core0 %.6f\n", mean_u64(shared0, passes));
    fprintf(sum, "shared mean core1 %.6f\n", mean_u64(shared1, passes));
    fprintf(sum, "separate mean core0 %.6f\n", mean_u64(sep0, passes));
    fprintf(sum, "separate mean core1 %.6f\n", mean_u64(sep1, passes));
    fprintf(sum, "shared/separate core0 %.12f\n", mean_u64(shared0, passes) / mean_u64(sep0, passes));
    fprintf(sum, "shared/separate core1 %.12f\n", mean_u64(shared1, passes) / mean_u64(sep1, passes));
    fprintf(sum, "shared core0/core1 %.12f\n", mean_u64(shared0, passes) / mean_u64(shared1, passes));
    fprintf(sum, "separate core0/core1 %.12f\n", mean_u64(sep0, passes) / mean_u64(sep1, passes));
    dump_state(sum, "shared_final", shared_chain);
    dump_state(sum, "separate0_final", sep_chain0);
    dump_state(sum, "separate1_final", sep_chain1);
    fclose(sum);

    free_chain(single);
    free_chain(shared_chain);
    free_chain(sep_chain0);
    free_chain(sep_chain1);
    free(shared0); free(shared1); free(sep0); free(sep1);
    return 0;
}
