#ifndef AM1_OBSERVER_H
#define AM1_OBSERVER_H
#include "am1_term.h"
#include <stddef.h>
typedef struct {
    size_t total_terms;
    size_t pole_0_terms;
    size_t pole_1_terms;
    size_t origin_terms;
    size_t unresolved_terms;
    size_t mirror_terms;
    size_t self_attempt_terms;
    size_t reexpression_terms;
    size_t articulation_terms;
} am1_observation;
void am1_observe(const am1_term *root, am1_observation *o);
#endif
