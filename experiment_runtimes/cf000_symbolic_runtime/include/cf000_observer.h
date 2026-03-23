#ifndef CF000_OBSERVER_H
#define CF000_OBSERVER_H

#include "cf000_state.h"

typedef struct {
    uint64_t step;
    size_t   total_terms;
    size_t   axis1_terms;
    size_t   axis2_terms;
    size_t   frontier_terms;
    size_t   unique_x;
    size_t   occupied_pairs;
    uint32_t max_depth;
    uint32_t collisions_resolved;
    uint8_t  axis2_admitted;
} cf000_observation;

void cf000_observe(const cf000_state *s, cf000_observation *o);
int  cf000_write_snapshot_csv(const char *path, const cf000_state *s);

#endif
