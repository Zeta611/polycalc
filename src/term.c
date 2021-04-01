#include "term.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tgmath.h>

#define ncmp(x, y) (((x) > (y)) - ((x) < (y)))

// Forward declarations for static functions
static int var_cmp(const TermNode *t1, const TermNode *t2);
static void add_coeff(TermNode *dest, const TermNode *src);
static void mul_coeff(TermNode *dest, const TermNode *src);
static void div_coeff(TermNode *dest, const TermNode *src);

static bool zero(TermNode *t);
static void reduce0(TermNode **p);

static TermNode *term_dup(const TermNode *t);
static TermNode *var_dup(const TermNode *v);
static TermNode *poly_dup(const TermNode *p);

static void mul_var(TermNode **dest, TermNode *src);

static void pow_num(TermNode **dest, TermNode *src);
static void ipow_poly(TermNode **dest, long exp);

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

TermNode *var_term(char *name, long pow)
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

int coeff_cmp(const TermNode *p1, const TermNode *p2)
{
	switch (p1->type) {
	case ICOEFF_TERM:
		switch (p2->type) {
		case ICOEFF_TERM:
			return ncmp(p1->hd.ival, p2->hd.ival);
		case RCOEFF_TERM:
			return ncmp(p1->hd.ival, p2->hd.rval);
		default:
			fprintf(stderr, "unexpected node type %d\n", p2->type);
			abort();
		}
	case RCOEFF_TERM:
		switch (p2->type) {
		case ICOEFF_TERM:
			return ncmp(p1->hd.rval, p2->hd.ival);
		case RCOEFF_TERM:
			return ncmp(p1->hd.rval, p2->hd.rval);
		default:
			fprintf(stderr, "unexpected node type %d\n", p2->type);
			abort();
		}
	default:
		fprintf(stderr, "unexpected node type %d\n", p1->type);
		abort();
	}
}

int poly_cmp(const TermNode *p1, const TermNode *p2)
{
	if (!p1 && !p2) {
		return 0;
	} else if (p1 && !p2) {
		return 1;
	} else if (!p1 && p2) {
		return -1;
	}
	int cmp = var_cmp(p1->u.vars, p2->u.vars);
	if (cmp) {
		return cmp;
	}
	// `p1` and `p2` have the same highest-order term ignoring coefficients.
	cmp = coeff_cmp(p1, p2);
	if (cmp) {
		return cmp;
	}
	return poly_cmp(p1->next, p2->next);
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

// `src` should be guaranteed to contain a non-zero number.
static void div_coeff(TermNode *dest, const TermNode *src)
{
	switch (dest->type) {
	case ICOEFF_TERM:
		switch (src->type) {
		case ICOEFF_TERM:
			if (dest->hd.ival % src->hd.ival) {
				dest->type = RCOEFF_TERM;
				dest->hd.rval =
				    dest->hd.ival / (double)src->hd.ival;
			} else {
				dest->hd.ival /= src->hd.ival;
			}
			return;
		case RCOEFF_TERM:
			dest->type = RCOEFF_TERM;
			dest->hd.rval = dest->hd.ival / src->hd.rval;
			return;
		default:
			fprintf(stderr, "unexpected node type %d\n", src->type);
			abort();
		}
	case RCOEFF_TERM:
		switch (src->type) {
		case ICOEFF_TERM:
			dest->hd.rval /= src->hd.ival;
			return;
		case RCOEFF_TERM:
			dest->hd.rval /= src->hd.rval;
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

static bool zero(TermNode *t)
{
	return (t->type == ICOEFF_TERM && t->hd.ival == 0) ||
	       (t->type == RCOEFF_TERM && t->hd.rval == 0);
}

// Remove zero-terms from `*p`. If `*p` is equivalent to 0, it reduces to a
// single coefficient term of value 0.
static void reduce0(TermNode **p)
{
	TermNode **hd = p;
	while (*p) {
		if (zero(*p)) {
			TermNode *del = *p;
			*p = del->next;
			free_term(del);
		} else {
			p = &(*p)->next;
		}
	}
	if (!*hd) {
		// `*p` was equivalent to 0, and every term has been removed.
		*hd = icoeff_term(0);
	}
}

// Add `src` to `dest`.
// This is essentially merging two linked lists.
// Argument passed to `src` must not be used after `add_poly` is called.
// `TermNode`s composing the polynomial represented by `src` are either rewired
// to `dest` accordingly or completely released from memory.
bool add_poly(TermNode **dest, TermNode *src)
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
			break;
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
	reduce0(dest);
	return true;
}

// Subtract `src` to `dest`.
// Argument passed to `src` must not be used after `sub_poly` is called.
bool sub_poly(TermNode **dest, TermNode *src)
{
	bool success = true;
	success = success && neg_poly(src);
	success = success && add_poly(dest, src);
	return success;
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
	TermNode *hd, **dup;
	dup = NULL;
	for (hd = term_dup(v); v; v = v->next, dup = &(*dup)->next) {
		if (!dup) {
			dup = &hd;
		} else {
			*dup = term_dup(v);
		}
	}
	return hd;
}

static TermNode *poly_dup(const TermNode *p)
{
	TermNode *hd, **dup;
	dup = NULL;
	for (hd = term_dup(p); p; p = p->next, dup = &(*dup)->next) {
		if (!dup) {
			dup = &hd;
		} else {
			*dup = term_dup(p);
		}
		TermNode **vdup = &(*dup)->u.vars;
		for (TermNode *vt = p->u.vars; vt; vt = vt->next) {
			*vdup = term_dup(vt);
			vdup = &(*vdup)->next;
		}
	}
	return hd;
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
bool mul_poly(TermNode **dest, TermNode *src)
{
	// `dup` points to the head pointer initially, and then points to the
	// copy of `*dest` afterwords (only if there are more than one term
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
		for (TermNode **i = p; *i; i = &(*i)->next) {
			mul_coeff(*i, src);
			if (svars) {
				mul_var(&(*i)->u.vars, var_dup(svars));
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
	reduce0(dest);
	return true;
}

// Divide `src` to `dest`.
// Argument passed to `src` must not be used after `div_poly` is called.
bool div_poly(TermNode **dest, TermNode *src)
{
	bool success = true;
	if (src->u.vars) {
		printf("Division by a polynomial is not supported.\n");
		success = false;
		goto src_cleanup;
	}
	if (zero(src)) {
		printf("Division by ZERO.\n");
		success = false;
		goto src_cleanup;
	}
	for (; *dest; dest = &(*dest)->next) {
		div_coeff(*dest, src);
	}
src_cleanup:
	free_poly(src);
	return success;
}

// Both `*dest` and `src` should be number terms.
static void pow_num(TermNode **dest, TermNode *src)
{
	switch ((*dest)->type) {
	case ICOEFF_TERM:
		switch (src->type) {
		case ICOEFF_TERM:
			if (src->hd.ival < 0) {
				(*dest)->type = RCOEFF_TERM;
				(*dest)->hd.rval =
				    pow((*dest)->hd.ival, src->hd.ival);
			} else {
				(*dest)->hd.ival =
				    pow((*dest)->hd.ival, src->hd.ival);
			}
			return;
		case RCOEFF_TERM:
			(*dest)->type = RCOEFF_TERM;
			(*dest)->hd.rval = pow((*dest)->hd.ival, src->hd.rval);
			return;
		default:
			fprintf(stderr, "unexpected node type %d\n", src->type);
			abort();
		}
	case RCOEFF_TERM:
		switch (src->type) {
		case ICOEFF_TERM:
			(*dest)->hd.rval = pow((*dest)->hd.rval, src->hd.ival);
			return;
		case RCOEFF_TERM:
			(*dest)->hd.rval = pow((*dest)->hd.rval, src->hd.rval);
			return;
		default:
			fprintf(stderr, "unexpected node type %d\n", src->type);
			abort();
		}
	default:
		fprintf(stderr, "unexpected node type %d\n", (*dest)->type);
		abort();
	}
}

// `exp` should be a positive integer.
static void ipow_poly(TermNode **dest, long exp)
{
	if (exp < 2) {
		return;
	}
	TermNode *dup;
	if (exp % 2) {
		dup = poly_dup(*dest);
		ipow_poly(dest, exp - 1);
	} else {
		ipow_poly(dest, exp / 2);
		dup = poly_dup(*dest);
	}
	mul_poly(dest, dup);
}

// Exponentiate `src` to `dest`.
// Argument passed to `src` must not be used after `pow_poly` is called.
bool pow_poly(TermNode **dest, TermNode *src)
{
	bool success = true;
	if (src->u.vars) {
		printf("Exponentiation with a polynomial is not supported.\n");
		success = false;
		goto src_cleanup;
	}
	if (!(*dest)->u.vars) { // `*dest` is a number term.
		pow_num(dest, src);
		goto src_cleanup;
	}
	if (src->type == RCOEFF_TERM) {
		printf("Exponentiation with a polynomial and a real number is "
		       "not supported.\n");
		success = false;
		goto src_cleanup;
	}

	long exp = src->hd.ival;
	if (exp < 0) {
		printf("Exponentiation with a polynomial and a negative "
		       "integer is not supported.\n");
		success = false;
	} else if (exp == 0) {
		TermNode *tmp = *dest;
		*dest = icoeff_term(1);
		free_poly(tmp);
	} else {
		ipow_poly(dest, exp);
	}
src_cleanup:
	free_poly(src);
	return success;
}

// Negate `dest`.
bool neg_poly(TermNode *dest)
{
	for (; dest; dest = dest->next) {
		switch (dest->type) {
		case ICOEFF_TERM:
			dest->hd.ival = -dest->hd.ival;
			break;
		case RCOEFF_TERM:
			dest->hd.rval = -dest->hd.rval;
			break;
		default:
			fprintf(stderr, "unexpected node type %d\n",
				dest->type);
			abort();
		}
	}
	return true;
}

// Release a single `COEFF_TERM` and/or all linked `VAR_TERM`s.
// Does not recursively release linked `COEFF_TERM` terms. For that purpose, use
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

// Print a polynomial pointed by `p`.
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
