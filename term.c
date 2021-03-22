#include "term.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Forward declarations for static functions
static int var_cmp(const TermNode *t1, const TermNode *t2);
static void add_coeff(TermNode *dest, const TermNode *src);
static void mul_coeff(TermNode *dest, const TermNode *src);

static TermNode *term_dup(const TermNode *t);
static TermNode *var_dup(const TermNode *v);
static TermNode *poly_dup(const TermNode *p);

static void mul_var(TermNode **dest, TermNode *src);

static void free_term(TermNode *t);
static void print_var(const TermNode *v);

TermNode *icoeff_term(long val)
{
	TermNode *term = malloc(sizeof *term);
	*term = (TermNode){ICOEFF_TERM, .hd.ival = val, .u.vars = NULL, NULL};
	return term;
}

TermNode *rcoeff_term(double val)
{
	TermNode *term = malloc(sizeof *term);
	*term = (TermNode){RCOEFF_TERM, .hd.rval = val, .u.vars = NULL, NULL};
	return term;
}

TermNode *var_term(char *name, int pow)
{
	TermNode *term = malloc(sizeof *term);
	char *s = malloc(strlen(name) + 1);
	strcpy(s, name);
	*term = (TermNode){VAR_TERM, .hd.name = s, .u.pow = pow, NULL};
	return term;
}

// First prioritize reverse-lexicographically, then prioritize higher orders.
// `t1` and `t2` must be `VARM_TERM`s or `NULL`s.
static int var_cmp(const TermNode *t1, const TermNode *t2)
{
	if (!t1 && !t2) {
		return 0;
	} else if (t1 && !t2) {
		return 1;
	} else if (!t1 && t2) {
		return -1;
	}
	// `t1->type` and `t2->type` must both be `VAR_TERM`s.
	int cmp = strcmp(t1->hd.name, t2->hd.name);
	if (cmp) {
		// Prioritize reverse-lexicographically, e.g., 'x' > 'y' > 'z'.
		return -cmp;
	}
	// `t1` and `t2` start with a same variable term.
	cmp = t1->u.pow - t2->u.pow;
	if (cmp) {
		return cmp;
	}
	return var_cmp(t1->next, t2->next);
}

static void add_coeff(TermNode *dest, const TermNode *src)
{
	switch (dest->type) {
	case ICOEFF_TERM:
		switch (src->type) {
		case ICOEFF_TERM:
			dest->hd.ival += src->hd.ival;
			return;
		case RCOEFF_TERM:
			dest->type = RCOEFF_TERM;
			dest->hd.rval = dest->hd.ival + src->hd.rval;
			return;
		default:
			fprintf(stderr, "unexpected node type %d\n", src->type);
			abort();
		}
	case RCOEFF_TERM:
		switch (src->type) {
		case ICOEFF_TERM:
			dest->hd.rval += src->hd.ival;
			return;
		case RCOEFF_TERM:
			dest->hd.rval += src->hd.rval;
			return;
		default:
			fprintf(stderr, "unexpected node type %d\n", src->type);
			abort();
		}
	default:
		fprintf(stderr, "unexpected node type %d\n", dest->type);
		abort();
	}
}

static void mul_coeff(TermNode *dest, const TermNode *src)
{
	switch (dest->type) {
	case ICOEFF_TERM:
		switch (src->type) {
		case ICOEFF_TERM:
			dest->hd.ival *= src->hd.ival;
			return;
		case RCOEFF_TERM:
			dest->type = RCOEFF_TERM;
			dest->hd.rval = dest->hd.ival * src->hd.rval;
			return;
		default:
			fprintf(stderr, "unexpected node type %d\n", src->type);
			abort();
		}
	case RCOEFF_TERM:
		switch (src->type) {
		case ICOEFF_TERM:
			dest->hd.rval *= src->hd.ival;
			return;
		case RCOEFF_TERM:
			dest->hd.rval *= src->hd.rval;
			return;
		default:
			fprintf(stderr, "unexpected node type %d\n", src->type);
			abort();
		}
	default:
		fprintf(stderr, "unexpected node type %d\n", dest->type);
		abort();
	}
}

// Add `src` to `dest`.
// This is essentially merging two linked lists.
// Argument passed to `src` must not be used after `add_poly` is called.
// `TermNode`s composing the polynomial represented by `src` are either rewired
// to `dest` accordingly or completely released from memory.
void add_poly(TermNode **dest, TermNode *src)
{
	// `*p` is the head pointer initially; `next` of a `TermNode`, if
	// traversed.
	// The double-pointer allows a uniform handling of both the head
	// pointer and the `next` pointer, by storing an address of a pointer.
	TermNode **p = dest;
	while (src) {
		if (!*p) {
			// Link rest of `src` at the end.
			*p = src;
			return;
		}
		int cmp = var_cmp((*p)->u.vars, src->u.vars);
		if (cmp > 0) {
			p = &(*p)->next;
		} else if (cmp < 0) {
			// Rewire `*src` to the `**p`-side.
			TermNode *tmp = src->next;
			src->next = *p;
			*p = src;
			src = tmp;
		} else {
			// Increase the coefficient of `**p` and release `src`.
			add_coeff(*p, src);
			p = &(*p)->next;
			TermNode *tmp = src;
			src = src->next;
			free_term(tmp);
		}
	}
}

static TermNode *term_dup(const TermNode *t)
{
	switch (t->type) {
	case ICOEFF_TERM:
		return icoeff_term(t->hd.ival);
	case RCOEFF_TERM:
		return rcoeff_term(t->hd.rval);
	case VAR_TERM:
		return var_term(t->hd.name, t->u.pow);
	default:
		fprintf(stderr, "unexpected node type %d\n", t->type);
		abort();
	}
}

static TermNode *var_dup(const TermNode *v)
{
	TermNode *ret, **dup;
	dup = NULL;
	for (ret = term_dup(v); v; v = v->next, dup = &(*dup)->next) {
		if (!dup) {
			dup = &ret;
		} else {
			*dup = term_dup(v);
		}
	}
	return ret;
}

static TermNode *poly_dup(const TermNode *p)
{
	TermNode *ret, **dup;
	dup = NULL;
	for (ret = term_dup(p); p; p = p->next, dup = &(*dup)->next) {
		if (!dup) {
			dup = &ret;
		} else {
			*dup = term_dup(p);
		}
		TermNode **vdup = &(*dup)->u.vars;
		for (TermNode *vt = p->u.vars; vt; vt = vt->next) {
			*vdup = term_dup(vt);
			vdup = &(*vdup)->next;
		}
	}
	return ret;
}

// Multiply `src` to `dest`--both should point directly to `VAR_TERM`s.
// This is analagous to `add_poly` function, as it is merging two sorted
// `VAR_TERM` lists.
static void mul_var(TermNode **dest, TermNode *src)
{
	TermNode **p = dest;
	while (src) {
		if (!*p) {
			*p = src;
			return;
		}
		// Again, prioritize reverse-lexicographically.
		int cmp = -strcmp((*p)->hd.name, src->hd.name);
		if (cmp > 0) {
			p = &(*p)->next;
		} else if (cmp < 0) {
			TermNode *tmp = src->next;
			src->next = *p;
			*p = src;
			src = tmp;
		} else {
			(*p)->u.pow += src->u.pow;
			p = &(*p)->next;
			TermNode *tmp = src;
			src = src->next;
			// No dedicated function to free a single `VAR_TERM`.
			free(tmp->hd.name);
			free(tmp);
		}
	}
}

// Multiply `src` to `dest`.
// Uses distributive law to multiply.
// Argument passed to `src` must not be used after `mul_poly` is called.
void mul_poly(TermNode **dest, TermNode *src)
{
	// `dup` points to the head pointer initially, and then points to the
	// copy of `*dest` afterwords (only if there are more than one terms
	// in `src` to apply distributive law).
	// `dup` is there only to keep a pointer later to be assigned to `p`.
	TermNode **dup, **p;
	for (dup = p = dest; src; p = dup) {
		if (src->next) {
			TermNode *tmp = poly_dup(*dup);
			// Get a fresh slot for a head pointer container.
			dup = malloc(sizeof *dup);
			*dup = tmp;
		}
		TermNode *svars = src->u.vars;
		// Multiply a term `*src` to each of the terms in `*p`.
		for (TermNode **tmp = p; *tmp; tmp = &(*tmp)->next) {
			mul_coeff(*tmp, src);
			if (svars) {
				mul_var(&(*tmp)->u.vars, var_dup(svars));
			}
		}
		if (p != dest) {
			add_poly(dest, *p);
			free(p); // Allocated by `dup`.
		}
		TermNode *tmp = src;
		src = src->next;
		free_term(tmp);
	}
}

// Release a single `COEFF_TERM` and/or all linked `VAR_TERM`s.
// Does not recursively release `COEFF_TERM` terms. For that purpose, use
// `free_poly`.
static void free_term(TermNode *t)
{
	if (!t) {
		return;
	}
	switch (t->type) {
	case ICOEFF_TERM:
	case RCOEFF_TERM:
		free_term(t->u.vars);
		break;
	case VAR_TERM:
		free(t->hd.name);
		free_term(t->next);
		break;
	default:
		fprintf(stderr, "unexpected node type %d\n", t->type);
		abort();
	}
	free(t);
}

static void print_var(const TermNode *v)
{
	for (; v; v = v->next) {
		int p = v->u.pow;
		if (p == 1) {
			printf("%s ", v->hd.name);
		} else {
			printf("%s^%d ", v->hd.name, p);
		}
	}
}

void print_poly(const TermNode *p)
{
	while (p) {
		if (p->type == ICOEFF_TERM) {
			if (p->hd.ival != 1 || !p->u.vars) {
				printf("%ld ", p->hd.ival);
			}
		} else {
			printf("%lf ", p->hd.rval);
		}
		print_var(p->u.vars);
		p = p->next;
		if (p) {
			printf("+ ");
		}
	}
	putchar('\n');
}

// Release a polynomial, i.e., `COEFF_TERM` typed `TermNode` linked together.
void free_poly(TermNode *p)
{
	if (!p) {
		return;
	}
	free_poly(p->next);
	free_term(p);
}
