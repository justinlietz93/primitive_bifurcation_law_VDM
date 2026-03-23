#include "cf000_state.h"

#include <string.h>

void cf000_state_init(cf000_state *s)
{
    memset(s, 0, sizeof(*s));
    s->step = 0;
    s->next_id = 1;
    s->collisions_resolved = 0;
    s->axis2_admitted = 0;
    s->n_terms = 1;

    s->terms[0].axis = CF000_AXIS_ORIGIN;
    s->terms[0].id = s->next_id++;
    s->terms[0].parent_id = 0;
    s->terms[0].depth = 0;
    s->terms[0].expanded = 0;
    s->terms[0].x = 0;
    s->terms[0].y = 0;
    strcpy(s->terms[0].word, "w");
}
