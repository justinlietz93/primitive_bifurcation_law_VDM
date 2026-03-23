#ifndef AM1_TERM_H
#define AM1_TERM_H

#include <stddef.h>
#include <stdio.h>

typedef enum {
    AM1_POLE_0,
    AM1_POLE_1,
    AM1_ORIGIN,
    AM1_UNRESOLVED,
    AM1_MIRROR,
    AM1_SELF_ATTEMPT,
    AM1_REEXPRESSION,
    AM1_ARTICULATION
} am1_tag;

typedef struct am1_term {
    am1_tag tag;
    int rewritten;
    struct am1_term *left;
    struct am1_term *right;
} am1_term;

am1_term *am1_new(am1_tag tag, am1_term *left, am1_term *right);
am1_term *am1_clone(const am1_term *t);
void am1_free(am1_term *t);
const char *am1_tag_name(am1_tag tag);
void am1_print(FILE *fp, const am1_term *t);

#endif
