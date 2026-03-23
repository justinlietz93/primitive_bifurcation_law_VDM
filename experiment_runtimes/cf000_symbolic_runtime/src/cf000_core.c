#include "cf000_core.h"

#include <string.h>

static int32_t cf000_balance(const char *word)
{
    int32_t x = 0;
    const char *p = word;
    while (*p) {
        if (*p == 'R') x += 1;
        else if (*p == 'L') x -= 1;
        ++p;
    }
    return x;
}

static size_t cf000_next_free_y(const cf000_state *s, int32_t x)
{
    size_t max_y = 0;
    size_t i;
    for (i = 0; i < s->n_terms; ++i) {
        const cf000_term *t = &s->terms[i];
        if (t->axis == CF000_AXIS_2D && t->x == x && (size_t)t->y >= max_y)
            max_y = (size_t)t->y + 1;
    }
    return max_y;
}

static int cf000_axis1_collision(const cf000_state *s, int32_t x, const char *word)
{
    size_t i;
    for (i = 0; i < s->n_terms; ++i) {
        const cf000_term *t = &s->terms[i];
        if (t->axis == CF000_AXIS_1D && t->x == x && strcmp(t->word, word) != 0)
            return 1;
    }
    return 0;
}

static int cf000_term_exists(const cf000_state *s, cf000_axis axis, const char *word, int32_t x, int32_t y)
{
    size_t i;
    for (i = 0; i < s->n_terms; ++i) {
        const cf000_term *t = &s->terms[i];
        if (t->axis == axis && t->x == x && t->y == y && strcmp(t->word, word) == 0)
            return 1;
    }
    return 0;
}

static void cf000_append_term(cf000_state *s,
                              cf000_axis axis,
                              uint32_t parent_id,
                              uint32_t depth,
                              const char *word,
                              int32_t x,
                              int32_t y)
{
    cf000_term *t;
    if (s->n_terms >= CF000_MAX_TERMS) return;
    if (cf000_term_exists(s, axis, word, x, y)) return;

    t = &s->terms[s->n_terms++];
    t->axis = axis;
    t->id = s->next_id++;
    t->parent_id = parent_id;
    t->depth = depth;
    t->expanded = 0;
    t->x = x;
    t->y = y;
    strncpy(t->word, word, CF000_MAX_WORD - 1);
    t->word[CF000_MAX_WORD - 1] = '\0';
}

static void cf000_realize_origin(cf000_state *s)
{
    cf000_term *w = &s->terms[0];
    w->expanded = 1;
    cf000_append_term(s, CF000_AXIS_1D, w->id, 1, "L", -1, 0);
    cf000_append_term(s, CF000_AXIS_1D, w->id, 1, "R",  1, 0);
}

static cf000_term *cf000_pick_frontier(cf000_state *s)
{
    size_t i;
    cf000_term *best = NULL;
    for (i = 0; i < s->n_terms; ++i) {
        cf000_term *t = &s->terms[i];
        if (t->axis == CF000_AXIS_ORIGIN || t->expanded) continue;
        if (!best) {
            best = t;
            continue;
        }
        if (t->depth < best->depth) best = t;
        else if (t->depth == best->depth && t->id < best->id) best = t;
    }
    return best;
}

static void cf000_emit_children(cf000_state *s, cf000_term *parent)
{
    char left[CF000_MAX_WORD];
    char right[CF000_MAX_WORD];
    int32_t xl, xr;
    size_t yl, yr;

    if (strlen(parent->word) + 1 >= CF000_MAX_WORD) {
        parent->expanded = 1;
        return;
    }

    strcpy(left, parent->word);
    strcat(left, "L");
    strcpy(right, parent->word);
    strcat(right, "R");

    xl = cf000_balance(left);
    xr = cf000_balance(right);

    if (cf000_axis1_collision(s, xl, left)) {
        yl = cf000_next_free_y(s, xl);
        s->axis2_admitted = 1;
        s->collisions_resolved += 1;
        cf000_append_term(s, CF000_AXIS_2D, parent->id, parent->depth + 1, left, xl, (int32_t)yl);
    } else {
        cf000_append_term(s, CF000_AXIS_1D, parent->id, parent->depth + 1, left, xl, 0);
    }

    if (cf000_axis1_collision(s, xr, right)) {
        yr = cf000_next_free_y(s, xr);
        s->axis2_admitted = 1;
        s->collisions_resolved += 1;
        cf000_append_term(s, CF000_AXIS_2D, parent->id, parent->depth + 1, right, xr, (int32_t)yr);
    } else {
        cf000_append_term(s, CF000_AXIS_1D, parent->id, parent->depth + 1, right, xr, 0);
    }

    parent->expanded = 1;
}

void cf000_update(cf000_state *s)
{
    cf000_term *frontier;

    if (s->n_terms == 1 && s->terms[0].axis == CF000_AXIS_ORIGIN && !s->terms[0].expanded) {
        cf000_realize_origin(s);
        s->step += 1;
        return;
    }

    frontier = cf000_pick_frontier(s);
    if (!frontier) {
        s->step += 1;
        return;
    }

    cf000_emit_children(s, frontier);
    s->step += 1;
}
