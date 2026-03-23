#include "am1_observer.h"
#include <string.h>
static void rec(const am1_term *t, am1_observation *o){ if(!t) return; o->total_terms += 1; switch(t->tag){ case AM1_POLE_0:o->pole_0_terms += 1; break; case AM1_POLE_1:o->pole_1_terms += 1; break; case AM1_ORIGIN:o->origin_terms += 1; break; case AM1_UNRESOLVED:o->unresolved_terms += 1; break; case AM1_MIRROR:o->mirror_terms += 1; break; case AM1_SELF_ATTEMPT:o->self_attempt_terms += 1; break; case AM1_REEXPRESSION:o->reexpression_terms += 1; break; case AM1_ARTICULATION:o->articulation_terms += 1; break; default: break; } rec(t->left,o); rec(t->right,o); }
void am1_observe(const am1_term *root, am1_observation *o){ memset(o,0,sizeof(*o)); rec(root,o); }
