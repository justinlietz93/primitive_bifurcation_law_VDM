#ifndef AM1_CORE_H
#define AM1_CORE_H
#include "am1_term.h"
am1_term *am1_initial_state(void);
void am1_step(am1_term **root);
#endif
