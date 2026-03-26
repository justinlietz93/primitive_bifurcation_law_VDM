#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct {
    uint64_t step;
    uint64_t u;
    uint64_t v;
    uint64_t uv;
    uint64_t hit_floor;
} Row;

extern uint64_t balanced_trace_to_floor(uint64_t u0, uint64_t v0, uint64_t floor_den, Row *out_rows, uint64_t max_rows);

static void write_csv(const char *path, const Row *rows, uint64_t n) {
    FILE *f = fopen(path, "w");
    if (!f) exit(1);
    fprintf(f, "step,u,v,uv,hit_floor\n");
    for (uint64_t i = 0; i < n; ++i) {
        fprintf(f, "%llu,%llu,%llu,%llu,%llu\n",
                (unsigned long long)rows[i].step,
                (unsigned long long)rows[i].u,
                (unsigned long long)rows[i].v,
                (unsigned long long)rows[i].uv,
                (unsigned long long)rows[i].hit_floor);
    }
    fclose(f);
}

int main(void) {
    const uint64_t floor_den = 4096;
    Row rows11[64] = {0};
    Row rows12[64] = {0};

    uint64_t n11 = balanced_trace_to_floor(1, 1, floor_den, rows11, 64);
    uint64_t n12 = balanced_trace_to_floor(1, 2, floor_den, rows12, 64);

    write_csv("/mnt/data/farey_balanced_anchor_trace_11_asm.csv", rows11, n11);
    write_csv("/mnt/data/farey_balanced_anchor_trace_12_asm.csv", rows12, n12);

    FILE *txt = fopen("/mnt/data/farey_balanced_anchor_trace_asm.txt", "w");
    if (!txt) return 1;

    fprintf(txt, "(1,1) trace to floor uv >= %llu\n", (unsigned long long)floor_den);
    for (uint64_t i = 0; i < n11; ++i) {
        fprintf(txt, "step %llu: (%llu,%llu) uv=%llu%s\n",
                (unsigned long long)rows11[i].step,
                (unsigned long long)rows11[i].u,
                (unsigned long long)rows11[i].v,
                (unsigned long long)rows11[i].uv,
                rows11[i].hit_floor ? "  <-- floor hit" : "");
    }

    fprintf(txt, "\n(1,2) trace to floor uv >= %llu\n", (unsigned long long)floor_den);
    for (uint64_t i = 0; i < n12; ++i) {
        fprintf(txt, "step %llu: (%llu,%llu) uv=%llu%s\n",
                (unsigned long long)rows12[i].step,
                (unsigned long long)rows12[i].u,
                (unsigned long long)rows12[i].v,
                (unsigned long long)rows12[i].uv,
                rows12[i].hit_floor ? "  <-- floor hit" : "");
    }

    fprintf(txt, "\nAnchor from both smallest lifts: (%llu,%llu), uv=%llu\n",
            (unsigned long long)rows11[n11 - 1].u,
            (unsigned long long)rows11[n11 - 1].v,
            (unsigned long long)rows11[n11 - 1].uv);
    fclose(txt);
    return 0;
}
