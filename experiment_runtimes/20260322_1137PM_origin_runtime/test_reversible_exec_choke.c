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

enum { ITERS = 100000, NUM_CELLS = 3 };

typedef struct {
    int cpu;
    cell_fn_t fn;
    uint64_t *deltas;
} worker_args_t;

typedef struct {
    int cell_size;
    int code_size;
    int mutable_off;
    uint8_t *buf;
} region_t;

static void pin_to_cpu(int cpu) {
    cpu_set_t set;
    CPU_ZERO(&set);
    CPU_SET(cpu, &set);
    if (sched_setaffinity(0, sizeof(set), &set) != 0) perror("sched_setaffinity");
}

static region_t build_region(void) {
    region_t r = {0};
    // bsr eax,eax (3) + setz dl (3) + lock xor byte ptr [rip+disp32], dl (7) + mutable pair (2)
    r.cell_size = 3 + 3 + 7 + 2;
    r.code_size = NUM_CELLS * r.cell_size + 1;
    r.mutable_off = 3 + 3 + 7;
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
        // BSR EAX,EAX
        r.buf[p++] = 0x0F; r.buf[p++] = 0xBD; r.buf[p++] = 0xC0;
        // SETZ DL
        r.buf[p++] = 0x0F; r.buf[p++] = 0x94; r.buf[p++] = 0xC2;
        // LOCK XOR byte ptr [rip+disp32], dl
        r.buf[p++] = 0xF0; r.buf[p++] = 0x30; r.buf[p++] = 0x15;
        int next = (i + 1) % NUM_CELLS;
        int target = next * r.cell_size + r.mutable_off;
        int next_ip = p + 4;
        int32_t disp = (int32_t)(target - next_ip);
        memcpy(&r.buf[p], &disp, 4); p += 4;
        // mutable executed successor pair
        r.buf[p++] = 0x00; r.buf[p++] = 0xC0; // 00 C0 = add al,al ; 01 C0 = add eax,eax
    }
    r.buf[NUM_CELLS * r.cell_size] = 0xC3;
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

static double parity_mean(const uint64_t *xs, int parity) {
    long double s = 0.0L;
    int n = 0;
    for (int i = parity; i < ITERS; i += 2) { s += (long double)xs[i]; ++n; }
    return n ? (double)(s / (long double)n) : 0.0;
}

static void write_csv(const char *path, const uint64_t *xs) {
    FILE *f = fopen(path, "w");
    if (!f) { perror(path); exit(1); }
    for (int i = 0; i < ITERS; ++i) fprintf(f, "%" PRIu64 "\n", xs[i]);
    fclose(f);
}

static void dump_state(FILE *f, const char *label, const region_t *r) {
    fprintf(f, "%s", label);
    for (int i = 0; i < NUM_CELLS; ++i) {
        int off = i * r->cell_size + r->mutable_off;
        fprintf(f, " %02x", r->buf[off]);
    }
    fprintf(f, "\n");
}

int main(void) {
    FILE *notes = fopen("/mnt/data/reversible_exec_choke_notes.md", "w");
    if (!notes) { perror("notes"); return 1; }
    fprintf(notes,
"# Reversible Executable Choke Test\n\n"
"## Goal\n"
"Test whether the **reversible successor orbit** survives and develops a stronger timing split when the orbit itself is placed under two-core shared executable contention.\n\n"
"## Mechanism\n"
"Each of 3 cells executes:\n\n"
"- `BSR EAX,EAX`\n"
"- `SETZ DL`\n"
"- `LOCK XOR byte ptr [next_cell_mutable_opcode], DL`\n"
"- mutable successor pair `00 C0 <-> 01 C0`\n\n"
"The mutable pair is executable, so the successor's opcode class is toggled by the origin-trigger path itself.\n\n"
"## Cases\n"
"- **Shared**: both cores execute the same mutable ring\n"
"- **Separate**: each core executes its own mutable ring\n\n"
"## Measurements\n"
"- per-core mean latency\n"
"- shared/separate ratio\n"
"- odd/even parity means as a crude orbit-phase split\n"
"- final mutable opcode states\n");
    fclose(notes);

    region_t shared = build_region();
    region_t sepA = build_region();
    region_t sepB = build_region();

    uint64_t *shared0 = calloc(ITERS, sizeof(uint64_t));
    uint64_t *shared1 = calloc(ITERS, sizeof(uint64_t));
    uint64_t *sep0 = calloc(ITERS, sizeof(uint64_t));
    uint64_t *sep1 = calloc(ITERS, sizeof(uint64_t));
    if (!shared0 || !shared1 || !sep0 || !sep1) { fprintf(stderr, "calloc failed\n"); return 1; }

    run_case((cell_fn_t)shared.buf, (cell_fn_t)shared.buf, shared0, shared1);
    run_case((cell_fn_t)sepA.buf, (cell_fn_t)sepB.buf, sep0, sep1);

    double m_sh0 = mean_u64(shared0), m_sh1 = mean_u64(shared1);
    double m_sp0 = mean_u64(sep0), m_sp1 = mean_u64(sep1);
    double sh0_odd = parity_mean(shared0, 0), sh0_even = parity_mean(shared0, 1);
    double sh1_odd = parity_mean(shared1, 0), sh1_even = parity_mean(shared1, 1);
    double sp0_odd = parity_mean(sep0, 0), sp0_even = parity_mean(sep0, 1);
    double sp1_odd = parity_mean(sep1, 0), sp1_even = parity_mean(sep1, 1);

    FILE *sum = fopen("/mnt/data/reversible_exec_choke_summary.txt", "w");
    if (!sum) { perror("summary"); return 1; }
    fprintf(sum, "Reversible executable choke summary\n");
    fprintf(sum, "shared_mean_core0=%.9f\n", m_sh0);
    fprintf(sum, "shared_mean_core1=%.9f\n", m_sh1);
    fprintf(sum, "separate_mean_core0=%.9f\n", m_sp0);
    fprintf(sum, "separate_mean_core1=%.9f\n", m_sp1);
    fprintf(sum, "ratio_shared_separate_core0=%.12f\n", m_sh0 / m_sp0);
    fprintf(sum, "ratio_shared_separate_core1=%.12f\n", m_sh1 / m_sp1);
    fprintf(sum, "ratio_core0_over_core1_shared=%.12f\n", m_sh0 / m_sh1);
    fprintf(sum, "ratio_core0_over_core1_separate=%.12f\n", m_sp0 / m_sp1);
    fprintf(sum, "shared_core0_odd=%.9f even=%.9f ratio=%.12f\n", sh0_odd, sh0_even, sh0_odd / sh0_even);
    fprintf(sum, "shared_core1_odd=%.9f even=%.9f ratio=%.12f\n", sh1_odd, sh1_even, sh1_odd / sh1_even);
    fprintf(sum, "separate_core0_odd=%.9f even=%.9f ratio=%.12f\n", sp0_odd, sp0_even, sp0_odd / sp0_even);
    fprintf(sum, "separate_core1_odd=%.9f even=%.9f ratio=%.12f\n", sp1_odd, sp1_even, sp1_odd / sp1_even);
    dump_state(sum, "shared_final_state:", &shared);
    dump_state(sum, "separate_A_final_state:", &sepA);
    dump_state(sum, "separate_B_final_state:", &sepB);
    fclose(sum);

    FILE *log = fopen("/mnt/data/test_reversible_exec_choke.log", "w");
    if (!log) { perror("log"); return 1; }
    fprintf(log, "Reversible executable choke test\n");
    fprintf(log, "Mutable pair: 00 C0 <-> 01 C0\n");
    fprintf(log, "shared_mean_core0=%.9f\n", m_sh0);
    fprintf(log, "shared_mean_core1=%.9f\n", m_sh1);
    fprintf(log, "separate_mean_core0=%.9f\n", m_sp0);
    fprintf(log, "separate_mean_core1=%.9f\n", m_sp1);
    dump_state(log, "shared_final_state:", &shared);
    dump_state(log, "separate_A_final_state:", &sepA);
    dump_state(log, "separate_B_final_state:", &sepB);
    fclose(log);

    write_csv("/mnt/data/reversible_exec_choke_shared_core0.csv", shared0);
    write_csv("/mnt/data/reversible_exec_choke_shared_core1.csv", shared1);
    write_csv("/mnt/data/reversible_exec_choke_separate_core0.csv", sep0);
    write_csv("/mnt/data/reversible_exec_choke_separate_core1.csv", sep1);

    printf("shared   mean0=%.6f mean1=%.6f\n", m_sh0, m_sh1);
    printf("separate mean0=%.6f mean1=%.6f\n", m_sp0, m_sp1);
    printf("ratio shared/separate core0=%.9f\n", m_sh0 / m_sp0);
    printf("ratio shared/separate core1=%.9f\n", m_sh1 / m_sp1);
    dump_state(stdout, "shared_final_state:", &shared);
    dump_state(stdout, "separate_A_final_state:", &sepA);
    dump_state(stdout, "separate_B_final_state:", &sepB);

    return 0;
}
