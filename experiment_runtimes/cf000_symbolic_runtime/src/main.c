#include <stdio.h>
#include <stdlib.h>

#include "cf000_state.h"
#include "cf000_core.h"
#include "cf000_observer.h"

static unsigned long long parse_ull(const char *s)
{
    char *end = 0;
    unsigned long long v = strtoull(s, &end, 10);
    if (!s || !*s || (end && *end)) return 0ULL;
    return v;
}

int main(int argc, char **argv)
{
    cf000_state s;
    cf000_observation o;
    FILE *obs;
    unsigned long long steps, sample_every, i;
    const char *prefix;
    char obs_path[1024], snap_path[1024], summary_path[1024];

    if (argc != 4) {
        fprintf(stderr, "usage: %s <steps> <sample_every> <out_prefix>\n", argv[0]);
        return 1;
    }

    steps = parse_ull(argv[1]);
    sample_every = parse_ull(argv[2]);
    prefix = argv[3];

    if (steps == 0ULL || sample_every == 0ULL) {
        fprintf(stderr, "steps and sample_every must be positive integers\n");
        return 1;
    }

    snprintf(obs_path, sizeof(obs_path), "%s_observables.csv", prefix);
    snprintf(snap_path, sizeof(snap_path), "%s_snapshot.csv", prefix);
    snprintf(summary_path, sizeof(summary_path), "%s_summary.txt", prefix);

    obs = fopen(obs_path, "w");
    if (!obs) {
        perror("fopen observables");
        return 1;
    }

    fprintf(obs, "step,total_terms,axis1_terms,axis2_terms,frontier_terms,unique_x,occupied_pairs,max_depth,collisions_resolved,axis2_admitted\n");

    cf000_state_init(&s);
    cf000_observe(&s, &o);
    fprintf(obs, "%llu,%zu,%zu,%zu,%zu,%zu,%zu,%u,%u,%u\n",
            (unsigned long long)o.step, o.total_terms, o.axis1_terms, o.axis2_terms,
            o.frontier_terms, o.unique_x, o.occupied_pairs, o.max_depth,
            o.collisions_resolved, (unsigned)o.axis2_admitted);

    for (i = 0ULL; i < steps; ++i) {
        cf000_update(&s);
        if (s.step % sample_every == 0ULL) {
            cf000_observe(&s, &o);
            fprintf(obs, "%llu,%zu,%zu,%zu,%zu,%zu,%zu,%u,%u,%u\n",
                    (unsigned long long)o.step, o.total_terms, o.axis1_terms, o.axis2_terms,
                    o.frontier_terms, o.unique_x, o.occupied_pairs, o.max_depth,
                    o.collisions_resolved, (unsigned)o.axis2_admitted);
        }
    }

    fclose(obs);

    cf000_observe(&s, &o);
    if (!cf000_write_snapshot_csv(snap_path, &s)) {
        fprintf(stderr, "failed to write snapshot\n");
        return 1;
    }

    {
        FILE *fp = fopen(summary_path, "w");
        if (!fp) {
            perror("fopen summary");
            return 1;
        }
        fprintf(fp, "step=%llu\n", (unsigned long long)o.step);
        fprintf(fp, "total_terms=%zu\n", o.total_terms);
        fprintf(fp, "axis1_terms=%zu\n", o.axis1_terms);
        fprintf(fp, "axis2_terms=%zu\n", o.axis2_terms);
        fprintf(fp, "frontier_terms=%zu\n", o.frontier_terms);
        fprintf(fp, "unique_x=%zu\n", o.unique_x);
        fprintf(fp, "occupied_pairs=%zu\n", o.occupied_pairs);
        fprintf(fp, "max_depth=%u\n", o.max_depth);
        fprintf(fp, "collisions_resolved=%u\n", o.collisions_resolved);
        fprintf(fp, "axis2_admitted=%u\n", (unsigned)o.axis2_admitted);
        fclose(fp);
    }

    return 0;
}
