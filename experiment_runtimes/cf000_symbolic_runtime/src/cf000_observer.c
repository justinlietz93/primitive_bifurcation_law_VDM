#include "cf000_observer.h"

#include <stdio.h>
#include <string.h>

void cf000_observe(const cf000_state *s, cf000_observation *o)
{
    int seen_x[CF000_MAX_TERMS];
    int seen_x_n = 0;
    size_t i, j;

    memset(o, 0, sizeof(*o));
    o->step = s->step;
    o->total_terms = s->n_terms;
    o->collisions_resolved = s->collisions_resolved;
    o->axis2_admitted = s->axis2_admitted;

    for (i = 0; i < s->n_terms; ++i) {
        const cf000_term *t = &s->terms[i];
        if (t->axis == CF000_AXIS_1D) o->axis1_terms += 1;
        if (t->axis == CF000_AXIS_2D) o->axis2_terms += 1;
        if (t->axis != CF000_AXIS_ORIGIN && !t->expanded) o->frontier_terms += 1;
        if (t->depth > o->max_depth) o->max_depth = t->depth;

        if (t->axis == CF000_AXIS_1D || t->axis == CF000_AXIS_2D) {
            int found = 0;
            for (j = 0; j < (size_t)seen_x_n; ++j) {
                if (seen_x[j] == t->x) { found = 1; break; }
            }
            if (!found && seen_x_n < CF000_MAX_TERMS) {
                seen_x[seen_x_n++] = t->x;
                o->unique_x += 1;
            }
        }
        if (t->axis == CF000_AXIS_2D) o->occupied_pairs += 1;
    }
}

int cf000_write_snapshot_csv(const char *path, const cf000_state *s)
{
    FILE *fp = fopen(path, "w");
    size_t i;
    if (!fp) return 0;

    fprintf(fp, "id,parent_id,axis,depth,x,y,expanded,word\n");
    for (i = 0; i < s->n_terms; ++i) {
        const cf000_term *t = &s->terms[i];
        const char *axis =
            (t->axis == CF000_AXIS_ORIGIN) ? "origin" :
            (t->axis == CF000_AXIS_1D) ? "1D" : "2D";

        fprintf(fp, "%u,%u,%s,%u,%d,%d,%u,%s\n",
                t->id, t->parent_id, axis, t->depth,
                t->x, t->y, (unsigned)t->expanded, t->word);
    }

    fclose(fp);
    return 1;
}
