#include "am1_term.h"
#include <stdlib.h>
am1_term *am1_new(am1_tag tag, am1_term *left, am1_term *right){ am1_term *t=(am1_term*)malloc(sizeof(am1_term)); if(!t) return NULL; t->tag=tag; t->rewritten=0; t->left=left; t->right=right; return t; }
am1_term *am1_clone(const am1_term *t){ am1_term *u; if(!t) return NULL; u=am1_new(t->tag, am1_clone(t->left), am1_clone(t->right)); if(!u) return NULL; u->rewritten=t->rewritten; return u; }
void am1_free(am1_term *t){ if(!t) return; am1_free(t->left); am1_free(t->right); free(t); }
const char *am1_tag_name(am1_tag tag){ switch(tag){ case AM1_POLE_0:return "0"; case AM1_POLE_1:return "1"; case AM1_ORIGIN:return "Orig"; case AM1_UNRESOLVED:return "Unres"; case AM1_MIRROR:return "Mirror"; case AM1_SELF_ATTEMPT:return "Look"; case AM1_REEXPRESSION:return "Re"; case AM1_ARTICULATION:return "Art"; default:return "?"; }}
void am1_print(FILE *fp, const am1_term *t){ if(!t){ fputs("∅", fp); return; } switch(t->tag){ case AM1_POLE_0: case AM1_POLE_1: case AM1_ORIGIN: fputs(am1_tag_name(t->tag), fp); return; case AM1_MIRROR: fputs("Mirror(", fp); am1_print(fp, t->left); fputc(')', fp); return; default: fputs(am1_tag_name(t->tag), fp); fputc('(', fp); am1_print(fp, t->left); if(t->right){ fputs(", ", fp); am1_print(fp, t->right);} fputc(')', fp); return; }}
