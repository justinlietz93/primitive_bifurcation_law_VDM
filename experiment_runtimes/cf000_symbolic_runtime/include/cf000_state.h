#ifndef CF000_STATE_H
#define CF000_STATE_H

#include <stddef.h>
#include <stdint.h>

#define CF000_MAX_TERMS 4096
#define CF000_MAX_WORD  64

typedef enum {
    CF000_AXIS_ORIGIN = 0,
    CF000_AXIS_1D     = 1,
    CF000_AXIS_2D     = 2
} cf000_axis;

typedef struct {
    cf000_axis axis;
    uint32_t   id;
    uint32_t   parent_id;
    uint32_t   depth;
    uint8_t    expanded;
    int32_t    x;
    int32_t    y;
    char       word[CF000_MAX_WORD];
} cf000_term;

typedef struct {
    uint64_t   step;
    uint32_t   next_id;
    uint32_t   collisions_resolved;
    uint8_t    axis2_admitted;
    size_t     n_terms;
    cf000_term terms[CF000_MAX_TERMS];
} cf000_state;

void cf000_state_init(cf000_state *s);

#endif
